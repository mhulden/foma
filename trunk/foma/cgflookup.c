/*     Foma: a finite-state toolkit and library.                             */
/*     Copyright © 2008-2012 Mans Hulden                                     */

/*     This file is part of foma.                                            */

/*     Foma is free software: you can redistribute it and/or modify          */
/*     it under the terms of the GNU General Public License version 2 as     */
/*     published by the Free Software Foundation.                            */

/*     Foma is distributed in the hope that it will be useful,               */
/*     but WITHOUT ANY WARRANTY; without even the implied warranty of        */
/*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         */
/*     GNU General Public License for more details.                          */

/*     You should have received a copy of the GNU General Public License     */
/*     along with foma.  If not, see <http://www.gnu.org/licenses/>.         */

#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <limits.h>
#include <getopt.h>
#include <unistd.h>
#include <wctype.h>
#include <locale.h>
#include "fomalib.h"

#define LINE_LIMIT 262144
#define UDP_MAX 65535
#define FLOOKUP_PORT 6062

static char *usagestring = "Usage: cgflookup [-h] [-a] [-i] [-s \"separator\"] [-w \"wordseparator\"] [-v] [-x] [-b] [-I <#|#k|#m|f>] <binary foma file>\n";

static char *helpstring = 
"Applies words from stdin to a foma transducer/automaton read from a file and prints results to stdout.\n"

"If the file contains several nets, inputs will be passed through all of them (simulating composition) or applied as alternates if the -a flag is specified (simulating priority union: the first net is tried first, if that fails to produce an output, then the second is tried, etc.).\n\n"
"Options:\n\n"
"-h\t\tprint help\n"
"-a\t\ttry alternatives (in order of nets loaded, default is to pass words through each)\n"
"-b\t\tunbuffered output (flushes output after each input word, for use in bidirectional piping)\n"
"-i\t\tinverse application (apply down instead of up)\n"
"-I indextype\tindex arcs with indextype (one of -I f -I #k -I #m or -I #)\n"
"\t\t(usually slower than the default except for states > 1,000 arcs)\n"
"\t\t  -I # will index all states containing # arcs or more\n"
"\t\t  -I NUMk will index states from densest to sparsest until reaching mem limit of # kB\n"
"\t\t  -I NUMM will index states from densest to sparsest until reaching mem limit of # MB\n"
"\t\t  -I f will index flag-containing states only\n"
"-q\t\tdon't sort arcs before applying (usually slower, except for really small, sparse automata)\n"
"-s \"separator\"\tchange input/output separator symbol (default is TAB)\n"
"-u \"separator\"\tmark uppercase words with <*>\n"
"-w \"separator\"\tchange words separator symbol (default is LF)\n"
    "-v\t\tprint version number\n";

struct lookup_chain {
    struct fsm *net;
    struct apply_handle *ah;
    struct lookup_chain *next;
    struct lookup_chain *prev;
};

#define DIR_DOWN 0
#define DIR_UP 1

static char buffer[2048];
static int  apply_alternates = 0, numnets = 0, direction = DIR_UP, results, buffered_output = 1, index_arcs = 0, index_flag_states = 0, index_cutoff = 0, index_mem_limit = INT_MAX, mark_uppercase = 0;
static char *separator = "\t", *wordseparator = "", *line, *indent = "\t";
static FILE *INFILE;
static struct lookup_chain *chain_head, *chain_tail, *chain_new, *chain_pos;
static fsm_read_binary_handle fsrh;

static char *(*applyer)() = &apply_up;  /* Default apply direction = up */
static void handle_line(char *s);
static void app_print(char *result);
static char *get_next_line();

void app_print(char *result) {
    wchar_t testuc[1];    // Temp storage to test uc of first letter in string
    if (result == NULL) {
	fprintf(stdout, "\"<%s>\"\n", line);
    } else {
	/* Or format string first */
	if (mark_uppercase) {
	    mbstowcs(testuc, line, 1);
	    if (iswupper(*testuc)) {
		fprintf(stdout,"%s%s <*>\n",indent, result);
	    } else {
		fprintf(stdout,"%s%s\n",indent, result);
	    }
	} else {
	    fprintf(stdout,"%s%s\n",indent, result);
	}
    }
}

int main(int argc, char *argv[]) {
    int opt, sortarcs = 1;
    char *infilename;
    struct fsm *net;

    setvbuf(stdout, buffer, _IOFBF, sizeof(buffer));

    while ((opt = getopt(argc, argv, "abhHiI:qs:uw:vx")) != -1) {
        switch(opt) {
        case 'a':
	    apply_alternates = 1;
	    break;
        case 'b':
	    buffered_output = 0;
	    break;
        case 'h':
	    printf("%s%s\n", usagestring,helpstring);
            exit(0);
        case 'i':
	    direction = DIR_DOWN;
	    applyer = &apply_down;
	    break;
        case 'q':
	    sortarcs = 0;
	    break;
	case 'I':
	    if (strcmp(optarg, "f") == 0) {
		index_flag_states = 1;
		index_arcs = 1;
	    } else if (strstr(optarg, "k") != NULL && strstr(optarg,"K") != NULL) {
		/* k limit */
		index_mem_limit = 1024*atoi(optarg);
		index_arcs = 1;
	    } else if (strstr(optarg, "m") != NULL && strstr(optarg,"M") != NULL) {
		/* m limit */
		index_mem_limit = 1024*1024*atoi(optarg);
		index_arcs = 1;
	    } else if (isdigit(*optarg)) {
		index_arcs = 1;
		index_cutoff = atoi(optarg);
	    }
	    break;
	case 's':
	    separator = strdup(optarg);
	    break;
	case 'u':
	    mark_uppercase = 1;
	    if (!setlocale(LC_CTYPE, "")) {
		fprintf(stderr, "Check uppercase flag is on, but can't set locale!\n");
	    }
	    break;
	case 'w':
	    wordseparator = strdup(optarg);
	    break;
        case 'v':
	    printf("cgflookup 1.03 (foma library version %s)\n", fsm_get_library_version_string());
	    exit(0);
	default:
            fprintf(stderr, "%s", usagestring);
            exit(EXIT_FAILURE);
	}
    }
    if (optind == argc) {
	fprintf(stderr, "%s", usagestring);
	exit(EXIT_FAILURE);
    }

    infilename = argv[optind];

    if ((fsrh = fsm_read_binary_file_multiple_init(infilename)) == NULL) {
        perror("File error");
	exit(EXIT_FAILURE);
    }
    chain_head = chain_tail = NULL;

    while ((net = fsm_read_binary_file_multiple(fsrh)) != NULL) {
	numnets++;
	chain_new = xxmalloc(sizeof(struct lookup_chain));	
	if (direction == DIR_DOWN && net->arcs_sorted_in != 1 && sortarcs) {
	    fsm_sort_arcs(net, 1);
	}
	if (direction == DIR_UP && net->arcs_sorted_out != 1 && sortarcs) {
	    fsm_sort_arcs(net, 2);
	}
	chain_new->net = net;
	chain_new->ah = apply_init(net);
	if (direction == DIR_DOWN && index_arcs) {
	    apply_index(chain_new->ah, APPLY_INDEX_INPUT, index_cutoff, index_mem_limit, index_flag_states);
	}
	if (direction == DIR_UP && index_arcs) {
	    apply_index(chain_new->ah, APPLY_INDEX_OUTPUT, index_cutoff, index_mem_limit, index_flag_states);
	}

	chain_new->next = NULL;
	chain_new->prev = NULL;
	if (chain_tail == NULL) {
	    chain_tail = chain_head = chain_new;
	} else if (direction == DIR_DOWN || apply_alternates == 1) {
	    chain_tail->next = chain_new;
	    chain_new->prev = chain_tail;
	    chain_tail = chain_new;
	} else {
	    chain_new->next = chain_head;
	    chain_head->prev = chain_new;
	    chain_head = chain_new;
	}
    }

    if (numnets < 1) {
	fprintf(stderr, "%s: %s\n", "File error", infilename);
	exit(EXIT_FAILURE);
    }

    /* Standard read from stdin */
    line = xxcalloc(LINE_LIMIT, sizeof(char));
    INFILE = stdin;
    while (get_next_line() != NULL) {
	results = 0;
	handle_line(line);
	if (results == 0) {
	    app_print(NULL);
	}
	fprintf(stdout, "%s", wordseparator);
	if (!buffered_output) {
	    fflush(stdout);
	}
    }
    /* Cleanup */
    for (chain_pos = chain_head; chain_pos != NULL; chain_pos = chain_head) {
	chain_head = chain_pos->next;
	if (chain_pos->ah != NULL) {
	    apply_clear(chain_pos->ah);
	}
	if (chain_pos->net != NULL) {
	    fsm_destroy(chain_pos->net);
	}
	xxfree(chain_pos);
    }
    if (line != NULL)
    	xxfree(line);
    exit(0);
}

char *get_next_line() {
    char *r;
    if ((r = fgets(line, LINE_LIMIT, INFILE)) != NULL) {
	line[strcspn(line, "\n\r")] = '\0';
    }
    return r;
}

void handle_line(char *s) {
    char *result, *tempstr;
    /* Apply alternative */
    results = 0;
    if (apply_alternates == 1) {
	for (chain_pos = chain_head, tempstr = s;   ; chain_pos = chain_pos->next) {
	    result = applyer(chain_pos->ah, tempstr);
	    if (result != NULL) {
		results++;
		if (results == 1) {
		    fprintf(stdout, "\"<%s>\"\n",line);
		}
		app_print(result);
		while ((result = applyer(chain_pos->ah, NULL)) != NULL) {
		    results++;
		    app_print(result);
		}
		break;
	    }
	    if (chain_pos == chain_tail) {
		break;
	    }
	}
    } else {
	    
	/* Get result from chain */
	for (chain_pos = chain_head, tempstr = s;  ; chain_pos = chain_pos->next) {		
	    result = applyer(chain_pos->ah, tempstr);		
	    if (result != NULL && chain_pos != chain_tail) {
		tempstr = result;
		continue;
	    }
	    if (result != NULL && chain_pos == chain_tail) {
		do {
		    results++;
		    if (results == 1) {
			fprintf(stdout, "\"<%s>\"\n",line);
		    }
		    app_print(result);
		} while ((result = applyer(chain_pos->ah, NULL)) != NULL);
	    }
	    if (result == NULL) {
		/* Move up */
		for (chain_pos = chain_pos->prev; chain_pos != NULL; chain_pos = chain_pos->prev) {
		    result = applyer(chain_pos->ah, NULL);
		    if (result != NULL) {
			tempstr = result;
			break;
		    }
		}
	    }
	    if (chain_pos == NULL) {
		break;
	    }
	}
    }
}
