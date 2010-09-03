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

#include <stdlib.h>
#include "foma.h"

/* int dfa_sort_cmp2(const struct fsm_state *a, const struct fsm_state *b) { */
/*   return (a->state_no - b->state_no); */
/* } */

struct fsm *fsm_reverse(struct fsm *net) {
    struct fsm *revnet;
    struct fsm_construct_handle *revh;
    struct fsm_read_handle *inh;
    int i;

    inh = fsm_read_init(net);
    revh = fsm_construct_init(net->name);
    fsm_construct_copy_sigma(revh, net->sigma);
    while (fsm_get_next_arc(inh)) {
	fsm_construct_add_arc_nums(revh, fsm_get_arc_target(inh), fsm_get_arc_source(inh), fsm_get_arc_num_in(inh), fsm_get_arc_num_out(inh));
    }
    while ((i = fsm_get_next_final(inh)) != -1) {
	fsm_construct_set_initial(revh, i);
    }
    while ((i = fsm_get_next_initial(inh)) != -1) {
	fsm_construct_set_final(revh, i);
    }
    fsm_read_done(inh);
    revnet = fsm_construct_done(revh);
    revnet->is_deterministic = 0;
    return(revnet);
}

struct fsm *fsm_reverse_old(struct fsm *net) {
  struct fsm_state *fsm, *rev_fsm;
  int i,j, statecount, arccount = 0, numstarts = 0;
  _Bool *finals, *starts;
  statecount = 0;
  fsm = net->states;

  /* Check if we only have one state with no arcs */
  if ((fsm)->target == -1 && (fsm+1)->state_no == -1) {
    return(net);
  }

  /* Create final states array */
  for (i=0; (fsm+i)->state_no != -1; i++) {
    if ((fsm+i)->state_no > statecount) {
      statecount = (fsm+i)->state_no;
    }
    arccount++;
  }
  statecount++;

  finals = xxmalloc_atomic((sizeof(_Bool))*statecount);
  starts = xxmalloc_atomic((sizeof(_Bool))*statecount);

  for (i = 0; i<statecount; i++) {
    finals[i] = 0;
    starts[i] = 0;
  }

  /* push start states on stack */
  for (i=0; (fsm+i)->state_no != -1; i++) {
    if (((fsm+i)->start_state == 1)) {
      numstarts++;
      starts[(fsm+i)->state_no] = 1;
    }
    if (((fsm+i)->final_state == 1)) {
      finals[(fsm+i)->state_no] = 1;
    }
  }

  rev_fsm = xxmalloc(sizeof(struct fsm_state)*(arccount+numstarts+1));

  /* Reverse arcs */
  for (i=0, j=0 ; (fsm+i)->state_no != -1 ; i++) {
    if ((fsm+i)->target == -1) {
      continue;
    }
    if (finals[(fsm+i)->target] == 1) {
      (rev_fsm+j)->start_state = 1;
    } else {
      (rev_fsm+j)->start_state = 0;
    }
    if (starts[(fsm+i)->target] == 1) {
      (rev_fsm+j)->final_state = 1;
    } else {
      (rev_fsm+j)->final_state = 0;
    }
    (rev_fsm+j)->in = (fsm+i)->in;
    (rev_fsm+j)->out = (fsm+i)->out;
    (rev_fsm+j)->target = (fsm+i)->state_no;
    (rev_fsm+j)->state_no = (fsm+i)->target;
    j++;
  }
  
  /* Go through old start states */

  for (i=0; i<j; i++) {
    starts[(rev_fsm+i)->state_no] = 0;
  }
  for (i=0; i < statecount; i++) {
    if (starts[i] == 1) {
      (rev_fsm+j)->state_no = i;
      (rev_fsm+j)->in = -1;
      (rev_fsm+j)->out = -1;
      (rev_fsm+j)->target = -1;
      (rev_fsm+j)->final_state = 1;
      /* Wrong */
      (rev_fsm+j)->start_state = finals[(rev_fsm+j)->state_no];
      j++;
    }
  }

  (rev_fsm+j)->state_no = -1;
  (rev_fsm+j)->in = -1;
  (rev_fsm+j)->out = -1;
  (rev_fsm+j)->target = -1;
  (rev_fsm+j)->final_state = -1;
  (rev_fsm+j)->start_state = -1;

  qsort(rev_fsm, find_arccount(rev_fsm), sizeof(struct fsm_state), sort_cmp);
  net->states = rev_fsm;

  xxfree(starts);
  xxfree(finals);
  xxfree(fsm);
  fsm_update_flags(net, UNK, UNK, UNK, UNK, UNK, UNK);
  return(net);
}
