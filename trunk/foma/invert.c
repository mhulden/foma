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

struct fsm *fsm_invert(struct fsm *net) {
  struct fsm_state *fsm;
  int i, temp;

  fsm = net->states;
  for (i = 0; (fsm+i)->state_no != -1; i++) {
    temp = (fsm+i)->in;
    (fsm+i)->in = (fsm+i)->out;
    (fsm+i)->out = temp;
  }  
  return (net);
}
