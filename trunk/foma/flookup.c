/*     Foma: a finite-state toolkit and library.                             */
/*     Copyright © 2008-2010 Mans Hulden                                     */

/*     This file is part of foma.                                            */

/*     Foma is free software: you can redistribute it and/or modify          */
/*     it under the terms of the GNU General Public License version 2 as     */
/*     published by the Free Software Foundation. */

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

char *usagestring = "Usage: flookup [-h] [-i] [-s \"separator\"] [-v] [-x] <binary foma file>\n";
char *helpstring = "Applies words from stdin to a foma transducer/automaton read from a file\nOptions:\n-h\t\tprint help\n-i\t\tinverse application (apply down instead of up)\n-s \"separator\"\tchange input/output separator symbol (default is tab)\n-v\t\tprint version number\n-x\t\tdon't echo input string";

int main(int argc, char *argv[]) {
    int opt, echo = 1;
    char *infilename, line[LINE_MAX], *result, *separator = "\t";
    struct fsm *net;
    struct apply_handle *ah;
    FILE *INFILE;
    char *(*applyer)() = &apply_up; /* Default apply direction */

    while ((opt = getopt(argc, argv, "his:vx")) != -1) {
        switch(opt) {
        case 'h':
	    printf("%s%s\n", usagestring,helpstring);
            exit(0);
        case 'i':
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
    net = fsm_read_binary_file(infilename);
    if (net == NULL) {
	fprintf(stderr, "%s: %s\n", "File error", infilename);
	exit(EXIT_FAILURE);
    }
    ah = apply_init(net);
    INFILE = stdin;
    while (fgets(line, LINE_MAX, INFILE) != NULL) {
	line[strcspn(line, "\n")] = '\0'; /* chomp */
	result = applyer(ah, line);
	if (result == NULL) {
	    if (echo == 1)
		printf("%s%s",line, separator);
	    printf("+?\n");
	    continue;
	} else {
	    if (echo == 1)
		printf("%s%s",line, separator);
	    printf("%s\n", result);		
	}
	for (;;) {
	    result = applyer(ah, NULL);
	    if (result == NULL)
		break;
	    if (echo == 1)
		printf("%s%s",line, separator);
	    printf("%s\n", result);
	}
    }
    if (ah != NULL) {
	apply_clear(ah);
    }
    if (net != NULL) {
	fsm_destroy(net);
    }
    exit(0);
}
