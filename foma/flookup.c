/*     Foma: a finite-state toolkit and library.                             */
/*     Copyright © 2008-2011 Mans Hulden                                     */

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
#include <stdio.h>
#include <limits.h>
#include <getopt.h>
#include "fomalib.h"

#define LINE_LIMIT 262144

char *usagestring = "Usage: flookup [-h] [-i] [-s \"separator\"] [-v] [-x] <binary foma file>\n";
char *helpstring = "Applies words from stdin to a foma transducer/automaton read from a file.\nIf the file contains several nets, inputs will be passed through all of them (simulating composition).\nOptions:\n-h\t\tprint help\n-i\t\tinverse application (apply down instead of up)\n-s \"separator\"\tchange input/output separator symbol (default is tab)\n-v\t\tprint version number\n-x\t\tdon't echo input string";

struct lookup_chain {
    struct fsm *net;
    struct apply_handle *ah;
    struct lookup_chain *next;
    struct lookup_chain *prev;
};

#define DIR_DOWN 0
#define DIR_UP 1

void app_print(int echo, char *line, char *separator, char *result) {
    if (result == NULL) {
	if (echo == 1)
	    printf("%s%s",line, separator);
	printf("+?\n");
	return;

    } else {
	if (echo == 1)
	    printf("%s%s",line, separator);
	printf("%s\n", result);		
    }
}

int main(int argc, char *argv[]) {
    
    int opt, echo = 1, apply_in_chain = 0, numnets = 0, direction = DIR_UP, results;
    char *infilename, line[LINE_LIMIT], *result, *tempstr, *separator = "\t";
    struct fsm *net;
    FILE *INFILE;   
    struct lookup_chain *chain_head, *chain_tail, *chain_new, *chain_pos;
    fsm_read_binary_handle fsrh;

    char *(*applyer)() = &apply_up; /* Default apply direction */

    while ((opt = getopt(argc, argv, "chis:vx")) != -1) {
        switch(opt) {
        case 'c':
	    apply_in_chain = 1;
	    break;
        case 'h':
	    printf("%s%s\n", usagestring,helpstring);
            exit(0);
        case 'i':
	    direction = DIR_DOWN;
	    applyer = &apply_down;
	    break;
	case 's':
	    separator = strdup(optarg);
	    break;
        case 'v':
	    printf("flookup 1.0 (foma library version %s)\n", fsm_get_library_version_string());
	    exit(0);
        case 'x':
	    echo = 0;
	    break;
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
	chain_new->net = net;
	chain_new->ah = apply_init(net);
	chain_new->next = NULL;
	chain_new->prev = NULL;
	if (chain_tail == NULL) {
	    chain_tail = chain_head = chain_new;
	} else if (direction == DIR_DOWN) {
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

    INFILE = stdin;
    /* Apply each line */
    while (fgets(line, LINE_LIMIT, INFILE) != NULL) {	
	line[strcspn(line, "\n")] = '\0'; /* chomp */
	results = 0;
	/* Get result from chain */

	for (chain_pos = chain_head, tempstr = line;  ; chain_pos = chain_pos->next) {

	    result = applyer(chain_pos->ah, tempstr);

	    if (result != NULL && chain_pos != chain_tail) {
		tempstr = result;
		continue;
	    }
	    if (result != NULL && chain_pos == chain_tail) {
		do {
		    results++;
		    app_print(echo, line, separator, result);
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
	if (results == 0) {
	    app_print(echo, line, separator, NULL);
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
    exit(0);
}
