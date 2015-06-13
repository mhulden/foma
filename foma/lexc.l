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
%top{
#define YY_BUF_SIZE 16777216
}
%{
#include <stdio.h>
#include "foma.h"
#include "lexc.h"

#define SOURCE_LEXICON 0
#define TARGET_LEXICON 1
#define YY_USER_ACTION lexccolumn += lexcleng;
static int lexentries;
extern int lexclex();
static struct defined_networks *olddefines;
extern int my_yyparse(char *my_string, int lineno, struct defined_networks *defined_nets, struct defined_functions *defined_funcs);
extern struct fsm *current_parse;
static char *tempstr;
int lexccolumn = 0;

struct fsm *fsm_lexc_parse_string(char *string, int verbose) {

   olddefines = g_defines;
   YY_BUFFER_STATE my_string_buffer;
   my_string_buffer = lexc_scan_string(string);
   lexentries = -1;
   lexclineno = 1;
   lexc_init();
   if (lexclex() != 1) {
     if (lexentries != -1) {
         printf("%i\n",lexentries);
     }       
   } 
   lexc_delete_buffer(my_string_buffer);
   g_defines = olddefines;
   return(lexc_to_fsm());
}

struct fsm *fsm_lexc_parse_file(char *filename, int verbose) {
  char *mystring;
  mystring = file_to_mem(filename);
  return(fsm_lexc_parse_string(mystring, verbose));
}

void lexc_trim(char *s) {
  /* Remove trailing ; and = and space and initial space */
  int i,j;
  for (i = strlen(s)-1; *(s+i) == ';' || *(s+i) == '=' || *(s+i) == ' ' || *(s+i) == '\t'; i--)
    *(s+i) = '\0';  
  for (i=0; *(s+i) == ' ' || *(s+i) == '\t' || *(s+i) == '\n'; i++) {
  }
  for (j=0; *(s+i) != '\0'; i++, j++) {
    *(s+j) = *(s+i);
  }
  *(s+j) = *(s+i);  
}

%}

 /* Nonreserved = anything except ; < > ! or space */

NONRESERVED [\001-\177]{-}[\011\012\014\015\040\041\042\045\073\074\076]|[\300-\337][\200-\277]|[\340-\357][\200-\277][\200-\277]|[\360-\367][\200-\277][\200-\277][\200-\277]|[\045][\001-\177]|[\045][\300-\337][\200-\277]|[\045][\340-\357][\200-\277][\200-\277]|[\045][\360-\367][\200-\277][\200-\277][\200-\277]

INFOSTRING [\001-\177]{-}[\042\012\015]|[\300-\337][\200-\277]|[\340-\357][\200-\277][\200-\277]|[\360-\367][\200-\277][\200-\277][\200-\277]

INSIDEREGEX [\001-\177]{-}[\073\173\175\042\045\076]|[\300-\337][\200-\277]|[\340-\357][\200-\277][\200-\277]|[\360-\367][\200-\277][\200-\277][\200-\277]|(@>)|(>@)|(->)|(=>)

INSIDEDEFREGEX [\001-\177]{-}[\073\173\175\042\045]|[\300-\337][\200-\277]|[\340-\357][\200-\277][\200-\277]|[\360-\367][\200-\277][\200-\277][\200-\277]

SPACE  [\040]|[\011]|[\014]

ANY    [\001-\177]|[\300-\337][\200-\277]|[\340-\357][\200-\277][\200-\277]|[\360-\367][\200-\277][\200-\277][\200-\277]

%x MCS LEXICON DEF LEXENTRIES INSIDEREGEX REGEX REGEXB REGEXQ DEFREGEX DEFREGEXB DEFREGEXQ EATUPINFO
%%

 /* Files begin with one of these three identifiers */
<*>Multichar_Symbols {
  BEGIN(MCS);
}

<*>Definitions {
    BEGIN(DEF);
}

 /* This line needs to be above the space glob */
 /* otherwise spaces get eaten up in a regex */
<REGEX>({INSIDEREGEX}|%{ANY})* {
  yymore();
}

<*>{SPACE}+ { }
<*>[\015]?\n { lexclineno++; lexccolumn = 1;}
 /* Multichar definitions */

 /* A Multichar definition can contain anything except nonescaped space */
<MCS>{NONRESERVED}+ {
  lexc_add_mc(lexctext);
}

<*>(LEXICON|Lexicon){SPACE}+{NONRESERVED}+ {
  lexc_trim(lexctext+8);
  if (lexentries != -1) {
    printf("%i, ",lexentries);
  }
  printf("%s...",lexctext+8);
  fflush(stdout);
  lexentries = 0;
  lexc_set_current_lexicon(lexctext+8, SOURCE_LEXICON);
  BEGIN(LEXENTRIES);
}

 /* Grab info string */
<EATUPINFO>[\042]{INFOSTRING}*[\042]{SPACE}*; {
  BEGIN(LEXENTRIES);
}
 /* Target followed by info string */
<LEXENTRIES>{NONRESERVED}+{SPACE}+/[\042]{INFOSTRING}*[\042]{SPACE}*; {
    lexc_trim(lexctext);
    lexc_set_current_lexicon(lexctext, TARGET_LEXICON);
    lexc_add_word();
    lexc_clear_current_word();
    lexentries++;
    if (lexentries %10000 == 0) {
      printf("%i...",lexentries);
      fflush(stdout);
    }
    BEGIN(EATUPINFO);
}


 /* Regular entries contain anything (not starting with <) and end in a nonescaped SPACE */
<LEXENTRIES>{NONRESERVED}+ {
      lexc_set_current_word(lexctext);
}


<LEXENTRIES>{NONRESERVED}+{SPACE}*; {
    //printf("[%s]\n", lexctext);
    lexc_trim(lexctext);
    lexc_set_current_lexicon(lexctext, TARGET_LEXICON);
    lexc_add_word();
    lexc_clear_current_word();
    lexentries++;
    if (lexentries %10000 == 0) {
      printf("%i...",lexentries);
      fflush(stdout);
    }
}

 /* A REGEX entry begins and ends with a < , > */
<LEXENTRIES>[\074] {
  BEGIN(REGEX);
}
 /* \076 = > */
<REGEX>[\076] {
    *(lexctext+lexcleng-1) = ';';
    if (my_yyparse(lexctext, lexclineno, g_defines, NULL) == 0) {
       lexc_set_network(current_parse);
    }    
    BEGIN(LEXENTRIES);
}

<REGEX>[{] {
  BEGIN(REGEXB);
  yymore();
}
<REGEXB>[^}] {
  yymore();
}
<REGEXB>[}] {
  BEGIN(REGEX);
  yymore();
}
<REGEX>(["])* {
  BEGIN(REGEXQ);
  yymore();
}
<REGEXQ>([^"]*) {
  yymore();
}
<REGEXQ>([\042]) {
  BEGIN(REGEX);
  yymore();
}
<DEF>{NONRESERVED}+{SPACE}+={SPACE}+ {
    lexc_trim(lexctext);
    tempstr = xxstrdup(lexctext);
    BEGIN(DEFREGEX);
}
 /* \073 = ; */
<DEFREGEX>[\073] {
    if (my_yyparse(lexctext, lexclineno, g_defines, NULL) == 0) {
      add_defined(g_defines, fsm_topsort(fsm_minimize(current_parse)),tempstr);
    }
    xxfree(tempstr);
    BEGIN(DEF);
}
<DEFREGEX>({INSIDEDEFREGEX}|%{ANY})* {
  yymore();
}
<DEFREGEX>[{] {
  BEGIN(DEFREGEXB);
  yymore();
}
<DEFREGEXB>[^}] {
  yymore();
}
<DEFREGEXB>[}] {
  BEGIN(DEFREGEX);
  yymore();
}
<DEFREGEX>(["])* {
  BEGIN(DEFREGEXQ);
  yymore();
}
<DEFREGEXQ>([^"]*) {
  yymore();
}
<DEFREGEXQ>([\042]) {
  BEGIN(DEFREGEX);
  yymore();
}
<*>((!).*[\015]?(\n)) {  /* printf ("Comment: [%s]\n",lexctext); */  }

<*>(.) { printf("\n***Syntax error on line %i column %i at '%s'\n",lexclineno,lexccolumn,lexctext); return 1;}
