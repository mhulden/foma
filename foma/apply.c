/*     Foma: a finite-state toolkit and library.                             */
/*     Copyright © 2008-2010 Mans Hulden                                     */

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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "foma.h"

#define RANDOM 1
#define ENUMERATE 2
#define MATCH 4
#define UP 8
#define DOWN 16
#define LOWER 32
#define UPPER 64
#define SPACE 128

#define FAIL 0
#define SUCCEED 1

#define DEFAULT_OUTSTRING_SIZE 2048
#define DEFAULT_LIMIT 1000
#define DEFAULT_STACK_SIZE 128

extern int g_obey_flags;
extern int g_show_flags;
extern int g_print_space;
extern int g_print_pairs;

static int apply_append(struct apply_handle *h, int cptr, int sym);
static char *apply_net(struct apply_handle *h);
static void apply_create_statemap(struct apply_handle *h,struct fsm *net);
static void apply_create_sigarray(struct apply_handle *h,struct fsm *net);
static void apply_create_sigmatch(struct apply_handle *h);
static int apply_match_str(struct apply_handle *h,int symbol, int position);
static void apply_add_flag(struct apply_handle *h,char *name);
static int apply_check_flag(struct apply_handle *h,int type, char *name, char *value);
static void apply_clear_flags(struct apply_handle *h);

static void apply_stack_clear(struct apply_handle *h);
static int apply_stack_isempty(struct apply_handle *h);
static void apply_stack_pop (struct apply_handle *h);
static void apply_stack_push (struct apply_handle *h,int sptr, int sipos, int sopos, int szeroes, char *sflagname, char *sflagvalue, int sflagneg);
static void apply_force_clear_stack(struct apply_handle *h);

static void apply_force_clear_stack(struct apply_handle *h) {

    /* Make sure stack is empty and marks reset */
    if (!apply_stack_isempty(h)) {
	*(h->marks+(h->gstates+h->ptr)->state_no) = -1;
	while (!apply_stack_isempty(h)) {
	    apply_stack_pop(h);
	    *(h->marks+(h->gstates+h->ptr)->state_no) = -1;	    
	}
	h->iterator = 0;
	h->iterate_old = 0;
	apply_stack_clear(h);
    }
}

char *apply_enumerate(struct apply_handle *h) {

    char *result = NULL;

    if (h->last_net == NULL || h->last_net->finalcount == 0)
        return (NULL);
    
    if (h->iterator == 0) {
        h->iterate_old = 0;
	apply_force_clear_stack(h);
        result = apply_net(h);
	if ((h->mode & RANDOM) != RANDOM)
	  (h->iterator)++;
    } else {
        h->iterate_old = 1;
        result = apply_net(h);
    }
    return(result);
}

char *apply_words(struct apply_handle *h) {
    h->mode = DOWN + ENUMERATE + LOWER + UPPER;
    return(apply_enumerate(h));
}

char *apply_upper_words(struct apply_handle *h) {
    h->mode = DOWN + ENUMERATE + UPPER;
    return(apply_enumerate(h));
}

char *apply_lower_words(struct apply_handle *h) {
    h->mode = DOWN + ENUMERATE + LOWER;
    return(apply_enumerate(h));
}

char *apply_random_words(struct apply_handle *h) {
    apply_clear_flags(h);
    h->mode = DOWN + ENUMERATE + LOWER + UPPER + RANDOM;
    return(apply_enumerate(h));
}

char *apply_random_lower(struct apply_handle *h) {
    apply_clear_flags(h);
    h->mode = DOWN + ENUMERATE + LOWER + RANDOM;    
    return(apply_enumerate(h));
}

char *apply_random_upper(struct apply_handle *h) {
    apply_clear_flags(h);
    h->mode = DOWN + ENUMERATE + UPPER + RANDOM;
    return(apply_enumerate(h));
}

void apply_free_sigmatch(struct apply_handle *h) {
    struct sigmatch *sigm, *sigp;
    int i;
    if (h->sigmatch == NULL)
        return;
    for (i=0; i < h->sigmatch_size; i++) {

        sigm = ((h->sigmatch)+i)->next;
        while (sigm != NULL) {
            sigp = sigm;
            sigm = sigm->next;
            xxfree(sigp);
        }
    }
    xxfree(h->sigmatch);
    h->sigmatch = NULL;
    h->sigmatch_size = 0;
}

/* Frees memory associated with applies */
void apply_clear(struct apply_handle *h) {
    if (h->statemap != NULL) {
        xxfree(h->statemap);
        h->statemap = NULL;
    }
    if (h->sigmatch != NULL) {
        apply_free_sigmatch(h);
    }
    if (h->marks != NULL) {
        xxfree(h->marks);
        h->marks = NULL;
    }
    if (h->searchstack != NULL) {
        xxfree(h->searchstack);
        h->searchstack = NULL;
    }
    if (h->sigs != NULL) {
        xxfree(h->sigs);
        h->sigs = NULL;
    }
    if (h->flag_lookup != NULL) {
        xxfree(h->flag_lookup);
        h->flag_lookup = NULL;
    }
    h->last_net = NULL;
    h->iterator = 0;
    xxfree(h->outstring);
    xxfree(h);
}

char *apply_updown(struct apply_handle *h, char *word) {

    char *result = NULL;

    if (h->last_net == NULL || h->last_net->finalcount == 0)
        return (NULL);
    
    if (word == NULL) {
        h->iterate_old = 1;
        result = apply_net(h);
    }
    else if (word != NULL) {
        h->iterate_old = 0;
        h->instring = word;
        apply_create_sigmatch(h);

	/* Remove old marks if necessary TODO: only pop marks */
	apply_force_clear_stack(h);
        result = apply_net(h);
    }
    return(result);
}

char *apply_down(struct apply_handle *h, char *word) {

    h->mode = DOWN;
    return(apply_updown(h, word));

}

char *apply_up(struct apply_handle *h, char *word) {

    h->mode = UP;
    return(apply_updown(h, word));

}

struct apply_handle *apply_init(struct fsm *net) {
    struct apply_handle *h;

    srand((unsigned int) time(NULL));
    h = malloc(sizeof(struct apply_handle));
    /* Init */

    h->iterate_old = 0;
    h->iterator = 0;
    h->instring = NULL;
    h->flag_list = NULL;
    h->sigmatch = NULL;    
    h->flag_lookup = NULL;

    h->last_net = net;

    h->outstring = xxmalloc(sizeof(char)*DEFAULT_OUTSTRING_SIZE);
    h->outstringtop = DEFAULT_OUTSTRING_SIZE;
    *(h->outstring) = '\0';
    h->gstates = net->states;
    h->gsigma = net->sigma;
    h->printcount = 1;
    apply_create_statemap(h, net);
    h->searchstack = xxmalloc(sizeof(struct searchstack) * DEFAULT_STACK_SIZE);
    h->apply_stack_top = DEFAULT_STACK_SIZE;
    apply_stack_clear(h);
    apply_create_sigarray(h, net);
    return(h);
}

int apply_stack_isempty (struct apply_handle *h) {
    if (h->apply_stack_ptr == 0) {
	return 1;
    }
    return 0;
}

void apply_stack_clear (struct apply_handle *h) {
    h->apply_stack_ptr = 0;
}

void apply_stack_pop (struct apply_handle *h) {
    (h->apply_stack_ptr)--;
    h->ptr = (h->searchstack+h->apply_stack_ptr)->offset;
    h->ipos = (h->searchstack+h->apply_stack_ptr)->ipos;
    h->opos = (h->searchstack+h->apply_stack_ptr)->opos;
    h->zeroes = (h->searchstack+h->apply_stack_ptr)->zeroes;
    if (h->has_flags && (h->searchstack+h->apply_stack_ptr)->flagname != NULL) {
	/* Restore flag */
	for (h->flist = h->flag_list; h->flist != NULL; h->flist = h->flist->next) {
	    if (strcmp(h->flist->name, (h->searchstack+h->apply_stack_ptr)->flagname) == 0) {
		break;
	    }
	}
	if (h->flist == NULL)
	    perror("***Nothing to pop\n");
	h->flist->value = (h->searchstack+h->apply_stack_ptr)->flagvalue;
	h->flist->neg = (h->searchstack+h->apply_stack_ptr)->flagneg;
    }
}

static void apply_stack_push (struct apply_handle *h, int sptr, int sipos, int sopos, int szeroes, char *sflagname, char *sflagvalue, int sflagneg) {
    if (h->apply_stack_ptr == h->apply_stack_top) {
	h->searchstack = xxrealloc(h->searchstack, sizeof(struct searchstack)* ((h->apply_stack_top)*2));
	if (h->searchstack == NULL) {
	  perror("Apply stack full!!!\n");
	  exit(0);
	}
	h->apply_stack_top *= 2;
    }
    (h->searchstack+h->apply_stack_ptr)->offset     = sptr;
    (h->searchstack+h->apply_stack_ptr)->ipos       = sipos;
    (h->searchstack+h->apply_stack_ptr)->opos       = sopos;
    (h->searchstack+h->apply_stack_ptr)->zeroes     = szeroes;
    (h->searchstack+h->apply_stack_ptr)->flagname   = sflagname;
    (h->searchstack+h->apply_stack_ptr)->flagvalue  = sflagvalue;
    (h->searchstack+h->apply_stack_ptr)->flagneg    = sflagneg;

    (h->apply_stack_ptr)++;
    
}

void apply_reset_enumerator(struct apply_handle *h) {
    int statecount, i;
    statecount = h->last_net->statecount;
    for (i=0; i < statecount; i++) {
	*(h->marks+i) = -1;
    }
    h->iterator = 0;
    h->iterate_old = 0;
}

char *apply_net(struct apply_handle *h) {

    char *fname, *fvalue;
    int eatupi, eatupo, symin, symout, f, fneg; 
    int m, vcount;
    
/*     We perform a basic DFS on the graph, with two minor complications:       */

/*     1. We keep a mark for each state which indicates whether it is seen      */
/*        on the current "run."  If we reach a marked state, we terminate.      */
/*        As we pop a position, we also unmark the state we came from.          */
/*        If we're matching against a string, we terminate if the mark          */
/*        is set and ipos is the same as last time we saw the flag, i.e.        */
/*        we've traversed a loop without consuming an input symbol              */
 
/*     2. If the graph has flags, we push the previous flag value when          */
/*        traversing a flag-modifying arc.  This is because a flag may          */
/*        may have been set during the previous "run" and may not apply.        */
/*        Since we're doing a DFS, we can be sure to return to the previous     */
/*        global flag state by just remembering that last flag change.          */
    
    if (h->iterate_old == 1) {
        goto resume;
    }

    h->ptr = 0; h->ipos = 0; h->opos = 0; h->zeroes = 0;
    apply_stack_clear(h);

    /* "The use of four-letter words like goto can occasionally be justified */
    /*  even in the best of company." Knuth (1974). */

    goto L2;

    while(!apply_stack_isempty(h)) {
	
	apply_stack_pop(h);

	if ((h->gstates+h->ptr)->state_no != (h->gstates+h->ptr+1)->state_no) {
	    *(h->marks+(h->gstates+h->ptr)->state_no) = -1;
	    continue;
	}
	(h->ptr)++;

	/* Follow arc & push old position */

    L1:
	for (f=0,h->curr_ptr = h->ptr; (h->gstates+h->curr_ptr)->state_no == (h->gstates+h->ptr)->state_no && (h->gstates+h->curr_ptr)-> in != -1; (h->curr_ptr)++) {
	    f = 0;

	    /* Select one random arc to follow out of all outgoing arcs */

	    if ((h->mode & RANDOM) == RANDOM) {
		vcount = 0;
		for (h->curr_ptr = h->ptr;  (h->gstates+h->curr_ptr)->state_no == (h->gstates+h->ptr)->state_no && (h->gstates+h->curr_ptr)-> in != -1; (h->curr_ptr)++) {
		    vcount++;
		}
		if (vcount > 0) {
		  h->curr_ptr = h->ptr + (rand() % vcount) ;
		} else {
		  h->curr_ptr = h->ptr;
		}
	    }

	    symin  = (((h->mode)&DOWN) == DOWN) ? (h->gstates+h->curr_ptr)->in  : (h->gstates+h->curr_ptr)->out;
	    symout = (((h->mode)&DOWN) == DOWN) ? (h->gstates+h->curr_ptr)->out : (h->gstates+h->curr_ptr)->in;
	    
	    if ((eatupi = apply_match_str(h, symin, h->ipos)) != -1) {

		eatupo = apply_append(h, h->curr_ptr, symout);
		if (g_obey_flags && h->has_flags && ((h->flag_lookup+symin)->type & (FLAG_UNIFY|FLAG_CLEAR|FLAG_POSITIVE|FLAG_NEGATIVE))) {
		    fname = h->flag_list->name;
		    fvalue = h->oldflagvalue;
		    fneg = h->oldflagneg;
                    
/* 		    printf("Pushing: [%s][%s][%i]\n",fname,fvalue,fneg); */
		} else {
		    fname = fvalue = NULL;
		    fneg = 0;
		}

                apply_stack_push(h, h->curr_ptr, h->ipos, h->opos, h->zeroes, fname, fvalue, fneg);

		// if (eatupi == 0 && ((h->gstates+h->curr_ptr)->target == (h->gstates+h->curr_ptr)->state_no))
                if (eatupi == 0)
		    (h->zeroes)++;
		f = 1;
                h->ptr = *(h->statemap+(h->gstates+h->curr_ptr)->target);
		h->ipos = (h->ipos)+eatupi;
		h->opos = (h->opos)+eatupo;
		break;
	    }
	}

	/* There were no more edges on this vertex? */
	if (!f) {
	    /* Unmark, break */
	    *(h->marks+(h->gstates+h->ptr)->state_no) = -1;
	    continue;
	}
	
	/* Print accumulated string */

    L2:
	if ((h->gstates+h->ptr)->final_state == 1 && ((((h->mode) & ENUMERATE) == ENUMERATE) || (h->ipos == strlen(h->instring)))) {
            /* Stick a 0 to endpos to avoid getting old accumulated gunk strings printed */
            *(h->outstring+h->opos) = '\0';
	    if (((h->mode) & RANDOM) == RANDOM) {
		/* To end or not to end */
		if (!(rand() % 2)) {
		    apply_stack_clear(h);
		    h->iterator = 0;
		    h->iterate_old = 0;
                    return(h->outstring);
		}
            } else {
                //printf("printing [%s]\n",outstring);
                return(h->outstring);
            }
        }

    resume:
	
	/* Check Mark */
	m = *(h->marks+(h->gstates+h->ptr)->state_no);
	
	/* If we're randomly generating strings, we randomly decide whether or not */
	/* to obey the restriction against looping */
	
	if ((m != -1)  && (((h->mode) & ENUMERATE) == ENUMERATE)) {
	  continue;
	} else if ((m != -1) && (((h->mode) & ENUMERATE) != ENUMERATE) && (h->zeroes) > (strlen(h->instring)+1)) {
     	    continue;
        }

	/* Mark */
	if ((h->mode & RANDOM) != RANDOM) {
	  *(h->marks+(h->gstates+h->ptr)->state_no) = 1;
	  *(h->marks+(h->gstates+h->ptr)->state_no) = h->ipos;
	}

	goto L1;
    }
    /* Catch unprinted random which didn't stop at a final state */
    if ((h->mode & RANDOM) == RANDOM) {
	  apply_stack_clear(h);
	  h->iterator = 0;
	  h->iterate_old = 0;
          return(h->outstring);    
    }
    
    apply_stack_clear(h);
    return NULL;
}

int apply_append (struct apply_handle *h, int cptr, int sym) {

    char *astring, *bstring, *pstring;
    int symin, symout, len, alen, blen;

    symin = (h->gstates+cptr)->in;
    symout = (h->gstates+cptr)->out;
    astring = *((h->sigs)+symin);
    bstring = *((h->sigs)+symout);

    if (symin == UNKNOWN) 
        astring = "?";
    if (symout == UNKNOWN)
        bstring = "?";
    if (symin == IDENTITY)
        astring = "@";
    if (symout == IDENTITY)
        bstring = "@";
    if (symin == EPSILON)
        astring = "0";
    if (symout == EPSILON)
        bstring = "0";

    alen = strlen(astring);
    blen = strlen(bstring);

    while (alen + blen + h->opos + 3 >= h->outstringtop) {
	h->outstring = xxrealloc(h->outstring, sizeof(char) * ((h->outstringtop) * 2));
	(h->outstringtop) *= 2;
    }

    if ((h->has_flags) && !g_show_flags && (h->flag_lookup+symin)->type) {
	astring = ""; alen = 0;
    }
    if (h->has_flags && !g_show_flags && (h->flag_lookup+symout)->type) {
	bstring = ""; blen = 0;
    }
    if (((h->mode) & ENUMERATE) == ENUMERATE) {
	/* Print both sides separated by colon */
	/* if we're printing "words" */
	if (((h->mode) & (UPPER | LOWER)) == (UPPER|LOWER)) {

	    if (astring == bstring) {
		strcpy(h->outstring+h->opos, astring);
		len = alen;
	    } else {
		strcpy(h->outstring+h->opos, astring);
		strcpy(h->outstring+h->opos+alen,":");
		strcpy(h->outstring+h->opos+alen+1,bstring);
		len = alen+blen+1;
	    }
	}

	/* Print one side only */
	if (((h->mode) & (UPPER|LOWER)) != (UPPER|LOWER)) {

	    if (symin == EPSILON) {
		astring = ""; alen = 0;
	    }
	    if (symout == EPSILON) {
		bstring = ""; blen = 0;
	    }
	    if (((h->mode) & (UPPER|LOWER)) == UPPER) {
		pstring = astring; 
		len = alen;
	    } else {
		pstring = bstring;
		len = blen;
	    }
	    strcpy(h->outstring+h->opos, pstring);
	}
    }
    if (((h->mode) & ENUMERATE) != ENUMERATE) {
	/* Print pairs is ON and symbols are different */
	if (g_print_pairs && (symin != symout)) {

	    if (symin == UNKNOWN && ((h->mode) & DOWN) == DOWN)
		strncpy(astring, h->instring+h->ipos, 1);
	    if (symout == UNKNOWN && ((h->mode) & UP) == UP)
		strncpy(bstring, h->instring+h->ipos, 1);
	    strcpy(h->outstring+h->opos, "<");
	    strcpy(h->outstring+h->opos+1, astring);
	    strcpy(h->outstring+h->opos+alen+1,":");
	    strcpy(h->outstring+h->opos+alen+2,bstring);
	    strcpy(h->outstring+h->opos+alen+blen+2,">");
	    len = alen+blen+3;
	}

	else if (sym == IDENTITY) {
	    /* Apply up/down */
	    strncpy(h->outstring+h->opos, h->instring+h->ipos, 1);
	    strncpy(h->outstring+h->opos+1,"", 1);
	    len = 1;
	} else if (sym == EPSILON) {
	    return(0);
	} else {
	    if (((h->mode) & DOWN) == DOWN) {
		pstring = bstring;
		len = blen;
	    } else {
		pstring = astring;
		len = alen;
	    }
	    strcpy(h->outstring+h->opos, pstring);
	}
    }
    if (g_print_space && len > 0) {
	strcpy(h->outstring+h->opos+len, " ");
	len++;
    }
    return(len);
}

/* Match a symbol from sigma against the current position in string */
/* Return the number of symbols consumed in input string */
/* For flags, we consume 0 symbols of the input string, naturally */

int apply_match_str(struct apply_handle *h, int symbol, int position) {
    struct sigmatch *sigm;
    if (((h->mode) & ENUMERATE) == ENUMERATE) {
	if (h->has_flags && (h->flag_lookup+symbol)->type) {
	    if (!g_obey_flags) {
		return 0;
	    }
	    if (apply_check_flag(h,(h->flag_lookup+symbol)->type, (h->flag_lookup+symbol)->name, (h->flag_lookup+symbol)->value) == SUCCEED) {
		return 0;
	    } else {
		return -1;
	    }
	    
	}
	return(0);
	return (strlen(*(h->sigs+symbol)));
    }
    
    /* If symbol is a flag, we need to check consistency */

    if (h->has_flags && (h->flag_lookup+symbol)->type) {
	if (!g_obey_flags) {
	    return 0;
	}
	if (apply_check_flag(h,(h->flag_lookup+symbol)->type, (h->flag_lookup+symbol)->name, (h->flag_lookup+symbol)->value) == SUCCEED) {
/* 	    printf("CF: [%i][%s][%s]", (flag_lookup+symbol)->type, (flag_lookup+symbol)->name, (flag_lookup+symbol)->value); */
	    return 0;
	} else {
	    return -1;
	}
    }

    for (sigm = h->sigmatch+position; sigm != NULL; sigm = sigm->next) {
	if (sigm->signumber == symbol)
	    return (sigm->consumes);
    }
    return -1;
}

void apply_create_statemap(struct apply_handle *h, struct fsm *net) {
    int i;
    struct fsm_state *fsm;
    fsm = net->states;
    h->statemap = xxmalloc(sizeof(int)*net->statecount);
    h->marks = xxmalloc(sizeof(int)*net->statecount);

    for (i=0; i < net->statecount; i++) {
	*(h->statemap+i) = -1;
	*(h->marks+i) = -1;
    }
    for (i=0; (fsm+i)->state_no != -1; i++) {
	if (*(h->statemap+(fsm+i)->state_no) == -1) {
	    *(h->statemap+(fsm+i)->state_no) = i;
	}
    }
}

void apply_create_sigarray(struct apply_handle *h, struct fsm *net) {
    struct sigma *sig;
    struct fsm_state *fsm;
    
    int i, maxsigma;
    
    fsm = net->states;

    maxsigma = sigma_max(net->sigma);

    h->sigs = xxmalloc(sizeof(char **)*(maxsigma+1));
    h->has_flags = 0;
    h->flag_list = NULL;

    for (sig = h->gsigma; sig != NULL && sig->number != -1; sig = sig->next) {
	if (flag_check(sig->symbol)) {
	    h->has_flags = 1;
	    apply_add_flag(h, flag_get_name(sig->symbol));
/* 	    printf("Isflag: [%s]\n",sig->symbol); */
/* 	    printf("Type: [%i]\n",flag_get_type(sig->symbol)); */
/* 	    printf("Name: [%s]\n",flag_get_name(sig->symbol)); */
/* 	    printf("Value: [%s]\n",flag_get_value(sig->symbol)); */
	}
	//	*(h->sigs+(sig->number)) = xxstrdup(sig->symbol);
	*(h->sigs+(sig->number)) = sig->symbol;
    }

    if (h->has_flags) {

	h->flag_lookup = xxmalloc(sizeof(struct flag_lookup)*(maxsigma+1));
	for (i=0; i <= maxsigma; i++) {
	    (h->flag_lookup+i)->type = 0;
	    (h->flag_lookup+i)->name = NULL;
	    (h->flag_lookup+i)->value = NULL;
	}
	for (sig = h->gsigma; sig != NULL ; sig = sig->next) {
	    if (flag_check(sig->symbol)) {
		(h->flag_lookup+sig->number)->type = flag_get_type(sig->symbol);
		(h->flag_lookup+sig->number)->name = flag_get_name(sig->symbol);
		(h->flag_lookup+sig->number)->value = flag_get_value(sig->symbol);		
	    }
	}
    }
}

/* We need to know which symbols in sigma we can match for all positions */
/* in the input string.  Alternatively, if there is no input string as is the case */
/* when we just list the words or randomly search the graph, we can match */
/* any symbol in sigma. */

/* We create an array that points to a linked list of all the symbols we can match */
/* at any given position of the input string, together with the information about */
/* how many symbols we consume if we do match */

void apply_create_sigmatch(struct apply_handle *h) {
    struct sigma *sig;
    struct sigmatch *tmp_sig = NULL;
    int i, thismatch;

    if (((h->mode) & ENUMERATE) == ENUMERATE) { return; }

    /* We create a sigmatch array only in case we match against a real string */

    if (h->sigmatch != NULL) {
      apply_free_sigmatch(h);
    }
    h->sigmatch_size = strlen(h->instring)+1;
    h->sigmatch = xxmalloc(sizeof(struct sigmatch)*h->sigmatch_size);

    /* We can always match EPSILON */
    for (i=0; i <= strlen(h->instring); i++) {
	(h->sigmatch+i)->signumber = EPSILON;
	(h->sigmatch+i)->consumes = 0;
	(h->sigmatch+i)->next = NULL;
    }

    for (i=0; i <= strlen(h->instring); i++) {
	thismatch = 0;
	for (sig = h->gsigma; sig != NULL ; sig = sig->next) {
	    if (sig->number == EPSILON || sig->number == UNKNOWN) {
		continue;
	    }
	    else if (strncmp((h->instring+i), sig->symbol, strlen(sig->symbol)) == 0) {
		/* add to list */
		if (thismatch == 0)
		    thismatch = 1;
		tmp_sig = xxmalloc(sizeof(struct sigmatch));
		tmp_sig->next = (h->sigmatch+i)->next;
		(h->sigmatch+i)->next = tmp_sig;
		
		tmp_sig->signumber = sig->number;
		tmp_sig->consumes = strlen(sig->symbol);
	    }
	}
	if (thismatch == 0 && i < strlen(h->instring)) {
	    /* Add ? to the list */
	    tmp_sig = xxmalloc(sizeof(struct sigmatch));
	    tmp_sig->next = (h->sigmatch+i)->next;
	    tmp_sig->signumber = UNKNOWN;
	    tmp_sig->consumes = 1;
	    (h->sigmatch+i)->next = tmp_sig;
	    tmp_sig = xxmalloc(sizeof(struct sigmatch));
	    tmp_sig->next = (h->sigmatch+i)->next;
	    tmp_sig->signumber = IDENTITY;
	    tmp_sig->consumes = 1;
	    (h->sigmatch+i)->next = tmp_sig;
	}
    }
}

void apply_add_flag(struct apply_handle *h, char *name) {
    struct flag_list *flist, *flist_prev;
    if (h->flag_list == NULL) {
	flist = h->flag_list = xxmalloc(sizeof(struct flag_list));
    } else {
	for (flist = h->flag_list; flist != NULL; flist_prev = flist, flist = flist->next) {
	    if (strcmp(flist->name, name) == 0) {
		return;
	    }
	}
	flist = xxmalloc(sizeof(struct flag_list));
	flist_prev->next = flist;
    }
    flist->name = name;
    flist->value = NULL;
    flist->neg = 0;
    flist->next = NULL;
    return;
}

void apply_clear_flags(struct apply_handle *h) {
    struct flag_list *flist;
    for (flist = h->flag_list; flist != NULL; flist = flist->next) {
	flist->value = NULL;
	flist->neg = 0;
    }
    return;
}

/* Check for flag consistency by looking at the current states of */
/* flags in flist */

int apply_check_flag(struct apply_handle *h, int type, char *name, char *value) {
    struct flag_list *flist, *flist2;
    for (flist = h->flag_list; flist != NULL; flist = flist->next) {
	if (strcmp(flist->name, name) == 0) {
	    break;
	}
    }
    h->oldflagvalue = flist->value;
    h->oldflagneg = flist->neg;

    if (type == FLAG_UNIFY) {
	if (flist->value == NULL) {
	    flist->value = xxstrdup(value);
	    return SUCCEED;
	}
	else if (strcmp(value, flist->value) == 0 && flist->neg == 0) {
	    return SUCCEED;	    
	} else if (strcmp(value, flist->value) != 0 && flist->neg == 1) {
	    flist->value = xxstrdup(value);
	    flist->neg = 0;
	    return SUCCEED;
	}  
	return FAIL;
    }

    if (type == FLAG_CLEAR) {
	flist->value = NULL;
	flist->neg = 0;
	return SUCCEED;
    }

    if (type == FLAG_DISALLOW) {
	if (value == NULL && flist->value == NULL) {
	    return SUCCEED;
	}
	if (value != NULL && flist->value == NULL) {
	    return FAIL;
	}
	if (value == NULL && flist->value != NULL) {
	    return FAIL;
	}
	if (strcmp(value, flist->value) != 0) {
            if (flist->neg == 1)
                return FAIL;
            return SUCCEED;
	}
	if (strcmp(value, flist->value) == 0 && flist->neg == 1) {
            return SUCCEED;
        }
        return FAIL;
    }

    if (type == FLAG_NEGATIVE) {
	flist->value = value;
	flist->neg = 1;
	return SUCCEED;
    }

    if (type == FLAG_POSITIVE) {
	flist->value = value;
	flist->neg = 0;
	return SUCCEED;
    }

    if (type == FLAG_REQUIRE) {
	if (value == NULL) {
	    if (flist->value == NULL) {
		return FAIL;
	    } else {
		return SUCCEED;
	    }
	} else {
	    if (flist->value == NULL) {
		return FAIL;
	    }
	    if (strcmp(value, flist->value) != 0) {
		return FAIL;
	    } else {
                if (flist->neg == 1) {
                    return FAIL;
                }
		return SUCCEED;
	    }
	}       	
    }

    if (type == FLAG_EQUAL) {
	for (flist2 = h->flag_list; flist2 != NULL; flist2 = flist2->next) {
	    if (strcmp(flist2->name, value) == 0) {
		break;
	    }
	}

	if (flist2 == NULL && flist->value != NULL)
	    return FAIL;
	if (flist2 == NULL && flist->value == NULL) {
	    return SUCCEED;
	}
	if (flist2->value == NULL || flist->value == NULL) {
	    if (flist2->value == NULL && flist->value == NULL && flist->neg == flist2->neg) {
		return SUCCEED;
	    } else {
		return FAIL;
	    }
	}  else if (strcmp(flist2->value, flist->value) == 0 && flist->neg == flist2->neg) {
	    return SUCCEED;
	}
	return FAIL;	
    }
    printf("***Don't know what do with flag [%i][%s][%s]\n", type, name, value);
    return FAIL;
}
