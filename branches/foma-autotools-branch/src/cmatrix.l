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

%option noyywrap
%option nounput
%option noinput
%{

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "foma.h"

extern int cmatrixlex();
static struct fsm *mnet;
static int currcost;
static char *instring;

void my_cmatrixparse(struct fsm *net, char *my_string) {
   				       
   YY_BUFFER_STATE my_string_buffer;

   currcost = 1;
   my_string_buffer = cmatrix_scan_string(my_string);
   mnet = net;
   cmatrix_init(mnet);
   cmatrixlex();
   cmatrix_delete_buffer(my_string_buffer);
}

%}

ANYUTF      [\001-\177]|[\300-\337].|[\340-\357]..|[\360-\367]...
NOCOLON     ([\001-\177]{-}[\011\040\012\014\072])|[\300-\337].|[\340-\357]..|[\360-\367]...
SP          [\040]|[\011]|[\012]|[\014]

%x SUB DEL INS COST OUTSTRING

%%

Substitute{SP}+/[0-9]+ { BEGIN(SUB);  }
Delete{SP}+/[0-9]+     { BEGIN(DEL);  }
Insert{SP}+/[0-9]+     { BEGIN(INS);  }
Cost{SP}+/[0-9]+       { BEGIN(COST); }
^#.* { }

:{NOCOLON}+ {
  cmatrix_set_cost(mnet, NULL, cmatrixtext+1, currcost);
}

{NOCOLON}+: {
  *(cmatrixtext+strlen(cmatrixtext)-1) = '\0';
  cmatrix_set_cost(mnet, cmatrixtext, NULL, currcost);
}

{NOCOLON}+:/{NOCOLON}+  {
  instring = xxstrndup(cmatrixtext, strlen(cmatrixtext)-1);
  BEGIN(OUTSTRING);

}
<OUTSTRING>{NOCOLON}+ {
  cmatrix_set_cost(mnet, instring, cmatrixtext, currcost);
  BEGIN(INITIAL);
}

<SUB>[0-9]+ {
  cmatrix_default_substitute(mnet, atoi(cmatrixtext));
  BEGIN(INITIAL);
}
<DEL>[0-9]+ {
  cmatrix_default_delete(mnet, atoi(cmatrixtext));
  BEGIN(INITIAL);
}
<INS>[0-9]+ {
  cmatrix_default_insert(mnet, atoi(cmatrixtext));
  BEGIN(INITIAL);
}

<COST>[0-9]+ {
  currcost = atoi(cmatrixtext);
  BEGIN(INITIAL);
}

<*>[\012|\040] { }
