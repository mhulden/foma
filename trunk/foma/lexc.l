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

%option noyywrap
%option nounput
%option noinput
%{
#include <stdio.h>
#include "foma.h"
#include "lexc.h"

#define SOURCE_LEXICON 0
#define TARGET_LEXICON 1

static int lexentries;
extern int lexclex();
extern struct defined *defines;
static struct defined *olddefines;
extern int my_yyparse(char *my_string, int lineno);
extern struct fsm *current_parse;
static char *tempstr;

struct fsm *fsm_lexc_parse_string(char *string) {

   olddefines = defines;
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
   defines = olddefines;
   return(lexc_to_fsm());
}

struct fsm *fsm_lexc_parse_file(char *filename) {
  char *mystring;
  mystring = file_to_mem(filename);
  return(fsm_lexc_parse_string(mystring));					       
}

void lexc_trim(char *s) {
  /* Remove trailing ; and = and space and initial space */
  int i,j;
  i = strlen(s);
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
NONRESERVED [\001-\177]{-}[\011\012\014\040\041\073\074\076]|[\300-\337][\200-\277]|[\340-\357][\200-\277][\200-\277]|[\360-\367][\200-\277][\200-\277][\200-\277]|[\045][\011]|[\045][\012]|[\045][\014]|[\045][\040]|[\045][\045]|[\045][\073]|[\045][\074]|[\045][\076]

INSIDEREGEX [\001-\177]{-}[\073\173\175\042\045\076]|[\300-\337][\200-\277]|[\340-\357][\200-\277][\200-\277]|[\360-\367][\200-\277][\200-\277][\200-\277]|(@>)|(>@)|(->)|(=>)

INSIDEDEFREGEX [\001-\177]{-}[\073\173\175\042\045]|[\300-\337][\200-\277]|[\340-\357][\200-\277][\200-\277]|[\360-\367][\200-\277][\200-\277][\200-\277]

SPACE  [\040]|[\011]|[\014]

ANY    [\001-\177]|[\300-\337][\200-\277]|[\340-\357][\200-\277][\200-\277]|[\360-\367][\200-\277][\200-\277][\200-\277]

%x MCS LEXICON DEF LEXENTRIES INSIDEREGEX REGEX REGEXB REGEXQ DEFREGEX DEFREGEXB DEFREGEXQ
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
<*>\n { lexclineno++; }
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

 /* Regular entries contain anything (not starting with <) and end in a nonescaped SPACE */
<LEXENTRIES>{NONRESERVED}+ {
      lexc_set_current_word(lexctext);
}

 /* A target has anything and ends in a semicolon */
<LEXENTRIES>{NONRESERVED}+{SPACE}*; {
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
    if (my_yyparse(lexctext, lexclineno) == 0) {
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
    if (my_yyparse(lexctext, lexclineno) == 0) {
      add_defined(fsm_topsort(fsm_minimize(current_parse)),tempstr);
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
<*>((!).*\n) { /* printf ("Comment: [%s]\n",lexctext); */ }

<*>(.) { printf("\n***Syntax error on line %i at '%s'\n",lexclineno,lexctext); return 1;}
