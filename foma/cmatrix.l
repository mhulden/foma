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
