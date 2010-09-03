/*     Foma: a finite-state toolkit and library.                             */
/*     Copyright © 2008-2009 Mans Hulden                                     */

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

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include "foma.h"

#define INITIAL_AGENDA_SIZE 100
#define INITIAL_HEAP_SIZE 10

#define BITMASK(b) (1 << ((b) & 7))
#define BITSLOT(b) ((b) >> 3)
#define BITSET(a,b) ((a)[BITSLOT(b)] |= BITMASK(b))
#define BITCLEAR(a,b) ((a)[BITSLOT(b)] &= ~BITMASK(b))
#define BITTEST(a,b) ((a)[BITSLOT(b)] & BITMASK(b))
#define BITNSLOTS(nb) ((nb + CHAR_BIT - 1) / CHAR_BIT)
#define min(X, Y)  ((X) < (Y) ? (X) : (Y))

static int calculate_h(int *intword, int currpos, int state);
static struct astarnode *node_delete_min();
void node_insert(int wordpos, int fsmstate, int g, int h, int in, int out, int parent);

extern int g_med_limit, g_med_cutoff;
static int maxdepth;

static int bytes_per_letter_array;
static uint8_t *letterbits, *nletterbits;

struct astarnode {
    short int wordpos;
    int fsmstate;
    short int f;
    short int g;
    short int h;
    int in;
    int out;
    int parent;
};

static struct astarnode *agenda = NULL;
static int astarcount;
static int heapcount;
static int heap_size;  
static int agenda_size;
static int *heap = NULL;

void print_sym(int sym, struct sigma *sigma) {    
    while (sigma != NULL) {
        if (sigma->number == sym) {
            printf("%s",sigma->symbol);
            break;
        }
        sigma = sigma->next;
    }
}

void print_match(struct astarnode *node, struct sigma *sigma, char *word) {
    int sym, i, wordlen;
    struct astarnode *n;
    printf("\n");
    int_stack_clear();
    wordlen = strlen(word);
    for (n = node; n != NULL ; n = agenda+(n->parent)) {
        if (n->in == 0 && n->out == 0)
            break;
        if (n->parent == -1)
            break;
        int_stack_push(n->in);
    }
    while (!(int_stack_isempty())) {
        sym = int_stack_pop();
        if (sym > 2) {
            print_sym(sym, sigma);
        }
        if (sym == 0) {
            printf("*");
        }
        if (sym == 2) {
            printf("@");
        }
    }
    printf("\n");
    for (n = node; n != NULL ; n = agenda+(n->parent)) {
        if (n->in == 0 && n->out == 0)
            break;
        if (n->parent == -1)
            break;
        else
            int_stack_push(n->out);
    }
    for (i = 0; !(int_stack_isempty()); ) {
        sym = int_stack_pop();
        if (sym > 2) {
            print_sym(sym, sigma);
            i++;
        }
        if (sym == 0) {
            printf("*");
        }
        if (sym == 2) {
            if (i > wordlen) {
                printf("*");
            } else {
                printf("%c",*(word+i));
                i++;
            }
        }
    }
    printf("\n");
    printf("Cost[f]: %i\n\n", node->g);
}

int apply_med(struct fsm *net, char *word) {

    _Bool hascm;
    int wordlen, i, g, h, curr_g, curr_pos, curr_state, target, in, out, curr_agenda_offset;
    int *intword, lines, curr_node_has_match;
    int nummatches;
    int delcost, subscost, inscost, wbits;
    int nodes_expanded, *cm, maxsigma;
    uint8_t *expanded;

    struct astarnode *curr_node;
    struct state_array *state_array;
    struct fsm_state *curr_ptr;

    fsm_create_letter_lookup(net);

    bytes_per_letter_array = net->medlookup->bytes_per_letter_array;
    nletterbits = net->medlookup->nletterbits;
    letterbits = net->medlookup->letterbits;

    state_array = map_firstlines(net);
    delcost = subscost = inscost = 1;
    cm = net->medlookup->confusion_matrix;
    maxsigma = sigma_max(net->sigma)+1;

    if (cm == NULL) {
        hascm = 0;
    } else {
        hascm = 1;
    }
    nodes_expanded = 0;

    if (agenda == NULL) {
        agenda = xxmalloc(sizeof(struct astarnode)*INITIAL_AGENDA_SIZE);
        agenda->f = -1;
        agenda_size = INITIAL_AGENDA_SIZE;
    }
    if (heap == NULL) {
        heap = xxmalloc(sizeof(int)*INITIAL_HEAP_SIZE);
        heap_size = INITIAL_HEAP_SIZE;
        *heap = 0; /* Points to sentinel */
    }
    
    astarcount = 1;
    heapcount = 0;
    wordlen = strlen(word);
    intword = xxmalloc(sizeof(int)*(wordlen+1));

    /* intword -> sigma numbers of word */
    for (i=0; i < wordlen; i++) {
        *(intword+i) = sigma_find((xxstrndup((word+i),1)), net->sigma);
        if (*(intword+i) == -1) {
            *(intword+i) = IDENTITY;
        }
    }
    *(intword+i) = -1; /* sentinel */
    
    /* Insert (0,0) g = 0 */
    
    h = calculate_h(intword, 0, 0);
    /* Mark expanded nodes */
    wbits = BITNSLOTS(wordlen+1);
    expanded = xxcalloc(wbits*net->statecount,sizeof(uint8_t));

    /* Root node */
    node_insert(0,0,0,h,0,0,-1);
    nummatches = 0;

    for(;;) {

        curr_node = node_delete_min();
        /* Need to save this in case we realloc and print_match() */
        curr_agenda_offset = curr_node-agenda;
        if (curr_node == NULL) {
            printf("Reached cutoff of %i.\n", g_med_cutoff);
            goto out;
        }
        /* Check if we already expanded this node */
        if (BITTEST(expanded+wbits*curr_node->fsmstate, curr_node->wordpos)) {
            curr_state = curr_node->fsmstate;
            curr_ptr = (state_array+curr_state)->transitions;
            if (!curr_ptr->final_state || !(curr_node->wordpos == wordlen)) {
                //continue;
            }
            /* If we didn't, mark it as expanded */
        } else {
            BITSET(expanded+wbits*curr_node->fsmstate, curr_node->wordpos);
        }

        nodes_expanded++;

        if (curr_node->f > g_med_cutoff) {
            printf("Reached cutoff of %i\n", g_med_cutoff);
            goto out;
        }

        curr_pos = curr_node->wordpos;
        curr_state = curr_node->fsmstate;
        curr_g = curr_node->g;
        
        lines = 0;
        curr_node_has_match = 0;

        for (curr_ptr = (state_array+curr_state)->transitions ; ;) {
            if (curr_ptr->state_no == -1) {
                break;
            }
            lines++;
            target = curr_ptr->target;
            if (curr_ptr->final_state && curr_pos == wordlen) {
                if (curr_node_has_match == 0) {
                    /* Found a match */
                    curr_node_has_match = 1;
                    print_match(agenda+curr_agenda_offset, net->sigma, word);
                    nummatches++;
                    if (nummatches == g_med_limit) {
                        goto out;
                    }
                }
            }

            if (curr_ptr->target == -1 && curr_pos == wordlen)
                break;
            if (curr_ptr->target == -1 && lines == 1)
                goto insert;
            if (curr_ptr->target == -1)
                break;

            target = curr_ptr->target;
            /* Add nodes to edge:0, edge:input, 0:edge */
            
            /* Delete a symbol from input */
            in = curr_ptr->in;
            out = 0;
            g = hascm ? curr_g + *(cm+in*maxsigma) : curr_g + delcost;

            h = calculate_h(intword, curr_pos, curr_ptr->target);

            if ((curr_pos == wordlen) && (curr_ptr->final_state == 0) && (h == 0)) {
                h = 1;
            }

            if (g+h <= g_med_cutoff)
                node_insert(curr_pos, target, g, h, in, out, curr_agenda_offset);
            
            if (curr_pos == wordlen)
                goto skip;

            /* Match/substitute */
            in = curr_ptr->in;
            out = *(intword+curr_pos);
            if (in != out) {
                g = hascm ? curr_g + *(cm+in*maxsigma+out) : curr_g + subscost;
            } else {
                g = curr_g;
            }
           
            h = calculate_h(intword, curr_pos+1, curr_ptr->target);
            if ((g+h) <= g_med_cutoff) {
                node_insert(curr_pos+1, target, g, h, in, out, curr_agenda_offset);
            }
        insert:
            /* Insert a symbol into input */
            /* Can only be done once per state */

            if (lines == 1) {

                in = 0;
                out = *(intword+curr_pos);
                
                g = hascm ? curr_g + *(cm+out) : curr_g + inscost;
                h = calculate_h(intword, curr_pos+1, curr_state);
                
                if (g+h <= g_med_cutoff)
                    node_insert(curr_pos+1, curr_state, g, h, in, out, curr_agenda_offset);

            }
            if (curr_ptr->target == -1)
                break;
        skip:
            if ((curr_ptr+1)->state_no == (curr_ptr)->state_no)
                curr_ptr++;
            else
                break;
        }
    }
 out:
     xxfree(expanded);
     return(nummatches);
}

int calculate_h(int *intword, int currpos, int state) {
    int i, j, hinf, hn, curr_sym;
    uint8_t *bitptr, *nbitptr;
    hinf = 0;
    hn = 0;

    bitptr = state*bytes_per_letter_array + letterbits;
    nbitptr = state*bytes_per_letter_array + nletterbits;

   /* For n = inf */
    if (*(intword+currpos) == -1)
        return 0;
    for (i = currpos; *(intword+i) != -1; i++) {
        curr_sym = *(intword+i);
        if (!BITTEST(bitptr, curr_sym)) {
            hinf++;
        }
    }
    /* For n = maxdepth */
    if (*(intword+currpos) == -1)
        return 0;
    for (i = currpos, j = 0; j < maxdepth && *(intword+i) != -1; i++, j++) {
        curr_sym = *(intword+i);
        if (!BITTEST(nbitptr, curr_sym)) {
            hn++;
        }
    }
    return(hinf > hn ? hinf : hn);
}

struct astarnode *node_delete_min() {
    int i, child;
    struct astarnode *firstptr, *lastptr;
    if (heapcount == 0) {
        return NULL;
    }
 
   /* We find the min from the heap */

    firstptr = agenda+heap[1];
    lastptr = agenda+heap[heapcount];
    heapcount--;
    
    /* Adjust heap */
    for (i = 1; (i<<1) <= heapcount; i = child) {
        child = i<<1;
        
        /* If right child is smaller (higher priority) than left child */
        if (child != heapcount && 
            ((agenda+heap[child+1])->f < (agenda+heap[child])->f || 
             ((agenda+heap[child+1])->f <= (agenda+heap[child])->f && 
              (agenda+heap[child+1])->wordpos > (agenda+heap[child])->wordpos))) {
            child++;
        }
        
        /* If child has lower priority than last element */
        if ((agenda+heap[child])->f < lastptr->f || 
            ((agenda+heap[child])->f <= lastptr->f && 
             (agenda+heap[child])->wordpos > lastptr->wordpos)) {
            
            heap[i] = heap[child];
        } else {
            break;
        }
    }
    heap[i] = (lastptr-agenda);
    return(firstptr);
}

void node_insert(int wordpos, int fsmstate, int g, int h, int in, int out, int parent) {
    int i,j,f;
    /* We add the node in the array */
    i = astarcount;
    if (i >= (agenda_size-1)) {
        agenda_size *= 2;
        agenda = xxrealloc(agenda, sizeof(struct astarnode)*agenda_size);
    }
    f = g + h;
    (agenda+i)->wordpos = wordpos;
    (agenda+i)->fsmstate = fsmstate;
    (agenda+i)->f = f;
    (agenda+i)->g = g;
    (agenda+i)->h = h;
    (agenda+i)->in = in;
    (agenda+i)->out = out;
    (agenda+i)->parent = parent;
    astarcount++;

    /* We also put the ptr on the heap */
    heapcount++;

    if (heapcount == heap_size-1) {
        heap = xxrealloc(heap, sizeof(int)*heap_size*2);
        //printf("Heaprealloc\n");
        heap_size *= 2;
    }
    /*                                     >= makes fifo */
    // for (j = heapcount; (agenda+heap[j/2])->f > f; j /= 2) {
    for (j = heapcount; ( (agenda+heap[j>>1])->f > f) || ((agenda+heap[j>>1])->f >= f && (agenda+heap[j>>1])->wordpos <= wordpos) ; j = j >> 1) {
        heap[j] = heap[j>>1];
    }
    heap[j] = i;
}

/* For keeping track of the strongly connected components */
/* when doing the DFS                                     */

struct sccinfo {
    int index;
    int lowlink;
    int on_t_stack;
};

void letterbits_union(int v, int vp, uint8_t *ptr) {
    int i;
    uint8_t *vptr, *vpptr;
    vptr = ptr+(v*bytes_per_letter_array);
    vpptr = ptr+(vp*bytes_per_letter_array);
    for (i=0; i < bytes_per_letter_array; i++) {
        *(vptr+i) = *(vptr+i)|*(vpptr+i);
    }
}

void letterbits_copy(int source, int target, uint8_t *ptr) {
    int i;
    uint8_t  *sourceptr, *targetptr;
    sourceptr = ptr+(source*bytes_per_letter_array);
    targetptr = ptr+(target*bytes_per_letter_array);
    for (i=0; i < bytes_per_letter_array; i++) {
        *(targetptr+i) = *(sourceptr+i);
    }
}

void letterbits_add(int v, int symbol, uint8_t *ptr) {
    uint8_t *vptr;
    vptr = ptr+(v*bytes_per_letter_array);
    BITSET(vptr, symbol);
}

/* Creates, for each state, a list of symbols that can be matched             */
/* somewhere in subsequent paths (the case n = inf)                           */
/* This is needed for h() of A*-search in approximate matching                */
/* This is done by a DFS where each state gets the properties                 */
/* of the arcs it has as well as a copy of the properties of the target state */
/* At the same time we keep track of the strongly connected components        */
/* And copy the properties from each root of the SCC to the children          */
/* The SCC part is required for the algorithm to work with cyclic graphs      */
/* This is basically a modification of Tarjan's (1972) SCC algorithm          */
/* However, it's converted from recursive form to iterative form using        */
/* goto statements.  Here's the original recursive specification:             */

/* Input: FSM = (V,E)                                                        */
/* Output: bitvector v.letters for each state                                */
/* index = 1                                    DFS node number counter      */

/* procedure Mark(v)                                                         */
/*   v.index = index                            Set the depth index for v    */
/*   v.lowlink = index                                                       */
/*   index++                                                                 */
/*   push(v)                                    Push v on the stack          */
/*   forall edge = (v, v') in E do              Consider successors of v     */
/*     v.letters |= v'.letters | edge           Copy target v'.letters       */
/*     if (v'.index == 0)                       Was successor v' visited?    */
/*       Mark(v')                               Recurse                      */
/*       v.lowlink = min(v.lowlink, v'.lowlink)                              */
/*     elseif (v' is on stack)                  Was successor v' in stack S? */
/*       v.lowlink = min(v.lowlink, v'.index)                                */
/*   if (v.lowlink == v.index)                  Is v the root of a SCC?      */
/*     do                                                                    */
/*       pop(v')                                                             */
/*       v'.letters = v.letters                                              */
/*     while (v' != v)                                                       */

void fsm_create_letter_lookup(struct fsm *net) {

    int num_states, num_symbols, index, v, vp, copystate, i, j;
    struct fsm_state *curr_ptr;
    struct state_array *state_array;
    struct sccinfo *sccinfo;
    int depth;
    maxdepth = 2;
    if (net->medlookup != NULL && net->medlookup->letterbits != NULL) {
        return;
    }
    printf("Calculating heuristic [h]\n");


    fsm_count(net);
    num_states = net->statecount;
    num_symbols = sigma_max(net->sigma);
    state_array = map_firstlines(net);
    
    bytes_per_letter_array = BITNSLOTS(num_symbols+1);
    letterbits = xxcalloc(bytes_per_letter_array*num_states,sizeof(uint8_t));
    nletterbits = xxcalloc(bytes_per_letter_array*num_states,sizeof(uint8_t));

    if (net->medlookup == NULL)
        net->medlookup = xxcalloc(1,sizeof(struct medlookup));

    net->medlookup->bytes_per_letter_array = bytes_per_letter_array;
    net->medlookup->letterbits = letterbits;
    net->medlookup->nletterbits = nletterbits;
    
    if (net->medlookup->confusion_matrix == NULL) {
        printf("Using Levenshtein distance.\n");
    } else {
        printf("Using confusion matrix.\n");
    }

    sccinfo = xxcalloc(num_states,sizeof(struct sccinfo));
    
    index = 1;
    curr_ptr = net->states;
    goto l1;

    /* Here we go again, converting a recursive algorithm to an iterative one */
    /* by gotos */

    while(!ptr_stack_isempty()) {

        curr_ptr = ptr_stack_pop();

        v = curr_ptr->state_no; /* source state number */
        vp = curr_ptr->target;  /* target state number */

        /* T: v.letterlist = list_union(v'->list, current edge label) */

        letterbits_union(v, vp, letterbits);         /* add v' target bits to v */
        letterbits_add(v, curr_ptr->in, letterbits); /* add current arc label to v */

        (sccinfo+v)->lowlink = min((sccinfo+v)->lowlink,(sccinfo+vp)->lowlink);

        if ((curr_ptr+1)->state_no != curr_ptr->state_no) {
            goto l4;
        } else {
            goto l3;
        }
        
    l1:
        v = curr_ptr->state_no;
        vp = curr_ptr->target;  /* target */
        /* T: v.lowlink = index, index++, Tpush(v) */
        (sccinfo+v)->index = index;
        (sccinfo+v)->lowlink = index;
        index++;
        int_stack_push(v);
        (sccinfo+v)->on_t_stack = 1;
        /* if v' not visited (is v'.index set) */

        /* If done here, check lowlink, pop */

        if (vp == -1)
            goto l4;
    l2:
        letterbits_add(v, curr_ptr->in, letterbits);
        if ((sccinfo+vp)->index == 0) {
            /* push (v,e) ptr on stack */
            ptr_stack_push(curr_ptr);
            curr_ptr = (state_array+(curr_ptr->target))->transitions;
            /* (v,e) = (v',firstedge), goto init */
            goto l1;
            /* if v' visited */
            /* T: v.lowlink = min(v.lowlink, v'.lowlink), union v.list with e, move to next edge in v, goto loop */
        } else if ((sccinfo+vp)->on_t_stack) {
            (sccinfo+v)->lowlink = min((sccinfo+v)->lowlink,(sccinfo+vp)->lowlink);
        }
        /* If node is visited, copy its bits */
        letterbits_union(v,vp,letterbits);

    l3:
        if ((curr_ptr+1)->state_no == curr_ptr->state_no) {
            curr_ptr++;
            v = curr_ptr->state_no;
            vp = curr_ptr->target;  /* target */
            goto l2;
        }

        
        /* T: if lastedge, v.lowlink == v.index, pop Tstack until v is popped and copy v.list to others */
        /* Copy all bits from root of SCC to descendants */
    l4:
        if ((sccinfo+v)->lowlink == (sccinfo+v)->index) {
            //printf("\nSCC: [%i] ",v);
            while((copystate = int_stack_pop()) != v) {
                (sccinfo+copystate)->on_t_stack = 0;
                letterbits_copy(v, copystate, letterbits);
                //printf("%i ", copystate);
            }
            (sccinfo+v)->on_t_stack = 0;
            //printf("\n");
        }
    }
    
    for (i=0; i < num_states; i++) {
        //printf("State %i: ",i);
        for (j=0; j <= num_symbols; j++) {
            if (BITTEST(letterbits+(i*bytes_per_letter_array),j)) {
                //printf("[%i]",j);
            }
        }    
        //printf("\n");
    }
    int_stack_clear();

    /* We do the same thing for some finite n (up to maxdepth) */
    /* and store the result in nletterbits                     */

    for (v=0; v < num_states; v++) {
        ptr_stack_push((state_array+v)->transitions);
        int_stack_push(0);
        while (!ptr_stack_isempty()) {
            curr_ptr = ptr_stack_pop();
            depth = int_stack_pop();
        looper:
            if (depth == maxdepth)
                continue;
            if (curr_ptr->in != -1) {
                letterbits_add(v, curr_ptr->in, nletterbits);
            }
            if (curr_ptr->target != -1) {
                if (curr_ptr->state_no == (curr_ptr+1)->state_no) {
                    ptr_stack_push(curr_ptr+1);
                    int_stack_push(depth);
                }
                depth++;
                curr_ptr = (state_array+(curr_ptr->target))->transitions;
                goto looper;
            }
        }
    }
    for (i=0; i < num_states; i++) {
        //printf("State %i: ",i);
        for (j=0; j <= num_symbols; j++) {
            if (BITTEST(nletterbits+(i*bytes_per_letter_array),j)) {
                //printf("[%i]",j);
            }
        }
        //printf("\n");
    }
    xxfree(state_array);
    xxfree(sccinfo);
}

void cmatrix_print_att(struct fsm *net, FILE *outfile) {
    int lsymbol, i, j, *cm, maxsigma;
    maxsigma = sigma_max(net->sigma) + 1;
    cm = net->medlookup->confusion_matrix;

    lsymbol = 0 ;

    for (i = 0; i < maxsigma ; i++) {        
        for (j = 0; j < maxsigma ; j++) {
            if ((i != 0 && i < 3) || (j != 0 && j < 3)) { continue; }
            if (i == 0 && j != 0) {
                fprintf(outfile,"0\t0\t%s\t%s\t%i\n", "@0@", sigma_string(j, net->sigma), *(cm+i*maxsigma+j));
            } else if (j == 0 && i != 0) {
                fprintf(outfile,"0\t0\t%s\t%s\t%i\n", sigma_string(i,net->sigma), "@0@", *(cm+i*maxsigma+j));
            } else if (j != 0 && i != 0) {
                fprintf(outfile,"0\t0\t%s\t%s\t%i\n", sigma_string(i,net->sigma),sigma_string(j, net->sigma), *(cm+i*maxsigma+j));
            }
        }
    }
    fprintf(outfile,"0\n");
}

void cmatrix_print(struct fsm *net) {
    int lsymbol, i, j, *cm, maxsigma;
    char *thisstring;
    struct sigma *sigma;
    maxsigma = sigma_max(net->sigma) + 1;
    cm = net->medlookup->confusion_matrix;

    lsymbol = 0 ;
    for (sigma = net->sigma; sigma != NULL; sigma = sigma->next) {
        if (sigma->number < 3)
            continue;
        lsymbol = strlen(sigma->symbol) > lsymbol ? strlen(sigma->symbol) : lsymbol;
    }
    printf("%*s",lsymbol+2,"");
    printf("%s","0 ");

    for (i = 3; ; i++) {
        if ((thisstring = sigma_string(i, net->sigma)) != NULL) {
            printf("%s ",thisstring);
        } else {
            break;
        }
    }

    printf("\n");

    for (i = 0; i < maxsigma ; i++) {        
        for (j = 0; j < maxsigma ; j++) {
            if (j == 0) {
                if (i == 0) {
                    printf("%*s",lsymbol+1,"0");
                    printf("%*s",2,"*");
                } else {
                    printf("%*s",lsymbol+1, sigma_string(i, net->sigma));
                    printf("%*d",2,*(cm+i*maxsigma+j));
                }
                j++;
                j++;
                continue;
            }
            if (i == j) {
                printf("%*s",strlen(sigma_string(j, net->sigma))+1,"*");
            } else {
                printf("%*d",strlen(sigma_string(j, net->sigma))+1,*(cm+i*maxsigma+j));
            }
        }
        printf("\n");
        if (i == 0) {
            i++; i++;
        }
    }
}

void cmatrix_init(struct fsm *net) {
    int maxsigma, *cm, i, j;
    if (net->medlookup == NULL) {
        net->medlookup = xxcalloc(1,sizeof(struct medlookup));
    }
    maxsigma = sigma_max(net->sigma)+1;
    cm = xxcalloc(maxsigma*maxsigma, sizeof(int));
    net->medlookup->confusion_matrix = cm;
    for (i = 0; i < maxsigma; i++) {
        for (j = 0; j < maxsigma; j++) {
            if (i == j)
                *(cm+i*maxsigma+j) = 0;
            else
                *(cm+i*maxsigma+j) = 1;
        }
    } 
}

void cmatrix_default_substitute(struct fsm *net, int cost) {
    int i, j, maxsigma, *cm;
    cm = net->medlookup->confusion_matrix;
    maxsigma = sigma_max(net->sigma)+1;
    for (i = 1; i < maxsigma; i++) {
        for (j = 1; j < maxsigma; j++) {
            if (i == j) {
                *(cm+i*maxsigma+j) = 0;                
            } else {
                *(cm+i*maxsigma+j) = cost;
            }
        }
    } 
}

void cmatrix_default_insert(struct fsm *net, int cost) {
    int i, maxsigma, *cm;
    cm = net->medlookup->confusion_matrix;
    maxsigma = sigma_max(net->sigma)+1;
    for (i = 0; i < maxsigma; i++) {
        *(cm+i) = cost;
    }
}

void cmatrix_default_delete(struct fsm *net, int cost) {
    int i, maxsigma, *cm;
    cm = net->medlookup->confusion_matrix;
    maxsigma = sigma_max(net->sigma)+1;
    for (i = 0; i < maxsigma; i++) {
        *(cm+i*maxsigma) = cost;
    }
}

void cmatrix_set_cost(struct fsm *net, char *in, char *out, int cost) {
    int i, o, maxsigma, *cm;
    cm = net->medlookup->confusion_matrix;
    maxsigma = sigma_max(net->sigma) + 1;
    if (in == NULL) {
        i = 0;
    } else {
        i = sigma_find(in, net->sigma);
    }
    if (out == NULL) {
        o = 0;
    } else {
        o = sigma_find(out, net->sigma);
    }
    if (i == -1) {
        printf("Warning, symbol '%s' not in alphabet\n",in);
        return;
    }
    if (o == -1) {
        printf("Warning, symbol '%s' not in alphabet\n",out);
        return;
    }
    *(cm+i*maxsigma+o) = cost;
}
