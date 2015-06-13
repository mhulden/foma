/*   Foma: a finite-state toolkit and library.                                 */
/*   Copyright © 2008-2015 Mans Hulden                                         */

/*   This file is part of foma.                                                */

/*   Licensed under the Apache License, Version 2.0 (the "License");           */
/*   you may not use this file except in compliance with the License.          */
/*   You may obtain a copy of the License at                                   */

/*      http://www.apache.org/licenses/LICENSE-2.0                             */

/*   Unless required by applicable law or agreed to in writing, software       */
/*   distributed under the License is distributed on an "AS IS" BASIS,         */
/*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  */
/*   See the License for the specific language governing permissions and       */
/*   limitations under the License.                                            */

#include <stdlib.h>
#include "foma.h"

struct fsm *fsm_reverse(struct fsm *net) {
    struct fsm *revnet;
    struct fsm_construct_handle *revh;
    struct fsm_read_handle *inh;
    int i;

    inh = fsm_read_init(net);
    revh = fsm_construct_init(net->name);
    fsm_construct_copy_sigma(revh, net->sigma);

    while (fsm_get_next_arc(inh)) {
	fsm_construct_add_arc_nums(revh, fsm_get_arc_target(inh)+1, fsm_get_arc_source(inh)+1, fsm_get_arc_num_in(inh), fsm_get_arc_num_out(inh));
    }

    while ((i = fsm_get_next_final(inh)) != -1) {
	fsm_construct_add_arc_nums(revh, 0, i+1, EPSILON, EPSILON);
    }
    while ((i = fsm_get_next_initial(inh)) != -1) {
	fsm_construct_set_final(revh, i+1);
    }
    fsm_construct_set_initial(revh, 0);
    fsm_read_done(inh);
    revnet = fsm_construct_done(revh);
    revnet->is_deterministic = 0;
    revnet->is_epsilon_free = 0;
    fsm_destroy(net);
    return(revnet);
}
