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

%option noinput
%option reentrant bison-bridge
%option bison-locations
%option yylineno
%{
#include <stdio.h>
#include "regex.tab.h"
#include "foma.h"
#define YY_USER_ACTION yylloc->first_line = yylloc->last_line = yylineno; yylloc->first_column = yycolumn; yylloc->last_column = yycolumn+yyleng-1; yycolumn += yyleng;

#define MAX_PARSE_DEPTH 100

struct parser_vars {
  int rewrite;
  int rule_direction;
  struct fsmcontexts *contexts;
  struct fsmrules *rules;
  struct rewrite_set *rewrite_rules;
  char   *ystring;
  struct fsm *ynet;
  int ytype;
};

struct parser_vars parservarstack[MAX_PARSE_DEPTH];
int g_parse_depth = 0;

extern int yyparse();
extern int get_iface_lineno(void);
extern int rewrite, rule_direction, substituting;
extern struct fsmcontexts *contexts;
extern struct fsmrules *rules;
extern struct rewrite_set *rewrite_rules;
extern struct fsm *current_parse;

char *yyget_text(yyscan_t yyscanner);
char *tempstr, *tempstr2;
int yylex_init (yyscan_t* scanner);
int yylex_destroy (yyscan_t scanner);
int my_yyparse(char *my_string, int lineno);

int yywrap(yyscan_t scanner) {return 1; }

int yyerror(YYLTYPE* yylloc, yyscan_t scanner, char *msg) {
   if(yylloc->first_line)
     fprintf(stderr, "%d.%d-%d.%d: error: ", yylloc->first_line, yylloc->first_column, yylloc->last_line, yylloc->last_column);
   printf("%s%s at '%s'.\n", "***", msg, yyget_text(scanner));
   return 1;
}

struct fsm *fsm_parse_regex(char *regex) {
  char *newregex;
  current_parse = NULL;
  newregex = xxmalloc(sizeof(char)*(strlen(regex)+2));
  strcpy(newregex, regex);
  strcat(newregex, ";");
  if (my_yyparse(newregex,1) == 0) {
    xxfree(newregex);
    return(fsm_minimize(current_parse));
  } else {
    xxfree(newregex);
    return(NULL);
  }
}

struct fsm *fsm_parse_regex_string(char *regex) {
  current_parse = NULL;
  if (my_yyparse(regex,1) == 0) {
    xxfree(regex);
    return(fsm_minimize(current_parse));
  } else {
    xxfree(regex);
    return(NULL);
  }
}

void yyset_lineno (int line_number, yyscan_t yyscanner);

int my_yyparse(char *my_string, int lineno) {
   int yyp;
   yyscan_t scanner;
   
   yylex_init(&scanner);
   YY_BUFFER_STATE my_string_buffer;
   my_string_buffer = yy_scan_string(my_string, scanner);
   yyset_lineno(lineno, scanner);
   if (g_parse_depth > 0) {
      if (g_parse_depth >= MAX_PARSE_DEPTH) {
        fprintf(stderr,"Exceeded parser stack depth.  Self-recursive call?\n");
        return 1;
      }
      /* Save variables on stack */
      parservarstack[g_parse_depth].rewrite = rewrite;
      parservarstack[g_parse_depth].rule_direction = rule_direction;
      parservarstack[g_parse_depth].contexts = contexts;
      parservarstack[g_parse_depth].rules = rules;
      parservarstack[g_parse_depth].rewrite_rules = rewrite_rules;     
   }
   g_parse_depth++;
   yyp = yyparse(scanner);
   g_parse_depth--;
   if (g_parse_depth > 0) {
     /* Restore parse variables */
     rewrite        = parservarstack[g_parse_depth].rewrite;
     rule_direction = parservarstack[g_parse_depth].rule_direction;
     contexts       = parservarstack[g_parse_depth].contexts;
     rules          = parservarstack[g_parse_depth].rules;
     rewrite_rules  = parservarstack[g_parse_depth].rewrite_rules;
   }
   yy_delete_buffer(my_string_buffer, scanner);
   yylex_destroy(scanner);
   return yyp;
}


%}

ANYUTF      [\001-\177]|[\300-\337].|[\340-\357]..|[\360-\367]...
 /* Reserved multicharacter symbols are a little tricky to define */
 /* what we're doing is excluding some combinations of multibyte sequences */
 /* using the {-} construct in flex */
NONRESERVED [0-9A-Za-z\?\'\=]|[\300-\301].|[\302]([\000-\377]{-}[\254])|[\303]([\000-\377]{-}[\227])|[\304-\315].|[\316]([\000-\377]{-}[\243\265])|[\317-\337].|[\340-\341]..|[\342][\000-\200].|[\342][\201][\000-\377]{-}[\273]|[\342][\202][\000-\377]{-}[\201\202]|[\342][\203-\205][\000-\377]|[\342][\206]([\000-\377]{-}[\222\224])|[\342][\207].|[\342][\210]([\000-\377]{-}[\200\203\205\210\230\245\247\250\251\252])|[\342][\211]([\000-\377]{-}[\240\244\245\272\273])|[\342][\212-\377].|[\343-\357]..|[\360-\367]...
BRACED      [{]([^}]|[\300-\337].|[\340-\357]..|[\360-\367]...)+[}]

%x DEFI QTD QTDEND UQ EQ ENDQ

%%

 /* we're matching braced strings */

{BRACED} {
  yylval_param->net = fsm_explode(yytext);
  return NET;
}

 /* Read binary file */
([\100]["][^"]+[\042]) {
  tempstr = xxstrndup(yytext+2,yyleng-3);
  yylval_param->net = fsm_read_binary_file(tempstr);
  xxfree(tempstr);
  if (yylval_param->net != NULL) {
    return NET;
  }
}

 /* Read regex from file */
([\100]re["][^"]+[\042]) {
  tempstr = xxstrndup(yytext+4,yyleng-5);
  tempstr2 = file_to_mem(tempstr);
  xxfree(tempstr);
  if (tempstr2 != NULL) {
    yylval_param->net = fsm_parse_regex_string(tempstr2);
    if (yylval_param->net != NULL) {
       return NET;
     }  
  }
}

 /* Read text file */
([\100]txt["][^"]+[\042]) {
  tempstr = xxstrndup(yytext+5,yyleng-6);
  yylval_param->net = fsm_read_text_file(tempstr);
  xxfree(tempstr);
  if (yylval_param->net != NULL) {
    return NET;
  }
}

 /* Read spaced text file */
([\100]stxt["][^"]+[\042]) {
  tempstr = xxstrndup(yytext+6,yyleng-7);
  yylval_param->net = fsm_read_spaced_text_file(tempstr);
  xxfree(tempstr);
  if (yylval_param->net != NULL) {
    return NET;
  }
}

(\^[0-9]+) {
  //yylval_param->string = xxstrdup(yytext+1);
  yylval_param->string = yytext+1;
  return NCONCAT;
}

(\^\{[0-9]+[ ]*,[ ]*[0-9]+\}) {
  //yylval_param->string = xxstrdup(yytext+2);
  yylval_param->string = yytext+2;
  return MNCONCAT;
}
(\^\>[0-9]+) {
  yylval_param->string = yytext+2;
//  yylval_param->string = xxstrdup(yytext+2);
  return MORENCONCAT;
}
(\^\<[0-9]+) {
  //  yylval_param->string = xxstrdup(yytext+2);
 yylval_param->string = yytext+2;
  return LESSNCONCAT;
}

((\ |\t)+) {

}

 /* Start of universal quantifier */
[\050][\342][\210][\200]/(([\000-\177]{-}[\051])|[\300-\337].|[\340-\357]..|[\360-\367]...)+[\051] {
 BEGIN(UQ);
}
 
 /* Start of existential quantifier */
[\050][\342][\210][\203]/(([\000-\177]{-}[\051])|[\300-\337].|[\340-\357]..|[\360-\367]...)+[\051] {
 BEGIN(EQ);
}


<UQ>(([\000-\177]{-}[\051])|[\300-\337].|[\340-\357]..|[\360-\367]...)+/[\051] {
 /* Add quantifier to quantifier symbol table */

 //yylval_param->string = xxstrdup(yytext);
 yylval_param->string = yytext;
 add_quantifier(yytext);
 BEGIN(ENDQ);
 return(UQUANT);
}

<EQ>(([\000-\177]{-}[\051])|[\300-\337].|[\340-\357]..|[\360-\367]...)+/[\051] {
 /* Add quantifier to quantifier symbol table */
 //yylval_param->string = xxstrdup(yytext);
 yylval_param->string = yytext;
 add_quantifier(yytext);
 BEGIN(ENDQ);
 return(EQUANT);
}

<ENDQ>[\051] {BEGIN(INITIAL);}

 /* Start of a quoted sequence of symbols */
[\042]/([\300-\337].|[\340-\357]..|[\360-\367]...|[\001-\041]|[\043-\177])+[\042] {
  BEGIN(QTD);
}

 /* Stuff that goes inside " ", including UTF8 \uHHHH sequences */
<QTD>([\300-\337].|[\340-\357]..|[\360-\367]...|[\001-\041]|[\043-\177])+/[\042] {
  decode_quoted(yytext);										  
  yylval_param->net = fsm_symbol(yytext);
  BEGIN(QTDEND);
  return NET;
}

 /* Disregard end quote */
<QTDEND>[\042] { BEGIN(INITIAL);}

 /* Different epsilon variants: "" or [] or \epsilon */
[\042][\042]|\[\]|[\316][\265] {
  yylval_param->net = fsm_empty_string();
  return NET;
}
 /* The empty set */
[\342][\210][\205] {
   yylval_param->net = fsm_empty_set();
   return NET;
}

 /* Sigma */
[\316][\243] {
  yylval_param->net = fsm_identity();  
  return NET;
}
 /* TODO: This S(x,y) conflicts with function definitions. */
 /*  Need to resolve this somehow. */
(_S\() { return SUCCESSOR_OF; }

(_isunambiguous\()   { return ISUNAMBIGUOUS;   }
(_isidentity\()      { return ISIDENTITY;      } 
(_isfunctional\()    { return ISFUNCTIONAL;    }
(_notid\()           { return NOTID;           }
(_lm\()              { return LETTERMACHINE;   }
(_loweruniq\()       { return LOWERUNIQ;       }
(_loweruniqeps\()    { return LOWERUNIQEPS;    }
(_allfinal\()        { return ALLFINAL;        }
(_unambpart\()       { return UNAMBIGUOUSPART; }
(_ambpart\()         { return AMBIGUOUSPART;   }
(_ambdom\()          { return AMBIGUOUSDOMAIN; }
(_eq\()              { return EQSUBSTRINGS;    }
(_marktail\()        { return MARKFSMTAIL;     }
(_addfinalloop\()    { return MARKFSMTAILLOOP; }
(_addnonfinalloop\() { return MARKFSMMIDLOOP;  }
(_addloop\()         { return MARKFSMLOOP;     }
(_addsink\()         { return ADDSINK;         }
(_leftrewr\()        { return LEFTREWR;        }
(_flatten\()         { return FLATTEN;         }

({NONRESERVED}+\() {
   yylval_param->string = xxstrdup(yytext);
   //yylval_param->string = yytext;
   return FUNCTION;
}

 /* The set of nonreserved symbols, or % followed by any UTF8 character */

({NONRESERVED}|\%{ANYUTF})+ {
  int i,j, skip, escaped;
  if ((strncmp(yytext,"=",1) == 0) && (count_quantifiers() > 0)) {
   int i;
   /* Copy yytext because unput() trashes yytext */
   char *yycopy = xxstrdup(yytext);
   for ( i = yyleng - 1; i > 0; --i )
     unput( yycopy[i] );
   xxfree(yycopy);
   return EQUALS;
  }

  for (escaped=0, i=0,j=0;*(yytext+i);) {
    *(yytext+j) = *(yytext+i);
    if (*(yytext+i) == '%') /* Skip escaping percent sign */ {
      i++;
      escaped++;
    }
    for(skip = utf8skip(yytext+i)+1; skip > 0; skip--) {
	*(yytext+j) = *(yytext+i);
        i++; j++;
    }
  }
  *(yytext+j) = *(yytext+i);
  if (substituting) {
    yylval_param->string = xxstrdup(yytext);
    //yylval_param->string = yytext;
    return SUBVAL;
  }
  //  yylval_param->string = xxstrdup(yytext);
  yylval_param->string = yytext;
  if(find_defined(yytext) != NULL) {
    yylval_param->net = fsm_copy(find_defined(yytext));
  } else if (find_quantifier(yytext) != NULL) {
      return VAR;
  } else {
    if (!escaped && strcmp(yytext, "0") == 0)
      yylval_param->net = fsm_empty_string();
    else if (!escaped && strcmp(yytext, "?") == 0)
      yylval_param->net = fsm_identity();
    else
      yylval_param->net = fsm_symbol(yytext);
  }
  return NET;
}

\.#\. { yylval_param->net = fsm_symbol(".#."); return NET;    }
(\_)                       { return CONTEXT;             }
\.(u|1)|[\342][\202][\201] { return XUPPER;              }
\.(l|2)|[\342][\202][\202] { return XLOWER;              }
(\.f)                      { return FLAG_ELIMINATE;      }
\[\./[^#]                  { return LDOT;                }
(\.\])                     { return RDOT;                }
([\342][\206][\222])       { return IMPLIES;             }
([\342][\206][\224])       { return BICOND;              }
([\xe2][\x88][\x88])       { return IN;                  }
(\~|[\302][\254])          { return COMPLEMENT;          }
(\.o\.|[\342][\210][\230]) { return COMPOSE;             }
(\.O\.)                    { return LENIENT_COMPOSE;     }
(\.P\.)                    { return PRIORITY_UNION_U;    }
(\.p\.)                    { return PRIORITY_UNION_L;    }
(<>|[\342][\210][\245])    { return SHUFFLE;             }
(<)                        { return PRECEDES;            }
(>)                        { return FOLLOWS;             }
(\.\.\.)                   { return TRIPLE_DOT;          }
(\-\>) {                     yylval_param->type = ARROW_RIGHT; return ARROW;}
(\(\-\>\)) {                 yylval_param->type = ARROW_RIGHT | ARROW_OPTIONAL; return ARROW;}
(\<\-) {                     yylval_param->type = ARROW_LEFT; return ARROW;}
(\(\<\-\)) {                 yylval_param->type = ARROW_LEFT | ARROW_OPTIONAL; return ARROW;}
(\<\-\>) {                   yylval_param->type = ARROW_LEFT|ARROW_RIGHT; return ARROW;}
(\(\<\-\>\)) {               yylval_param->type = ARROW_LEFT|ARROW_RIGHT|ARROW_OPTIONAL; return ARROW;}
(@\-\>) { yylval_param->type = ARROW_RIGHT|ARROW_LONGEST_MATCH|ARROW_LEFT_TO_RIGHT; return ARROW;}
(\(@\-\>\)) { yylval_param->type = ARROW_OPTIONAL|ARROW_RIGHT|ARROW_LONGEST_MATCH|ARROW_LEFT_TO_RIGHT; return ARROW;}
(@\>) { yylval_param->type = ARROW_RIGHT|ARROW_SHORTEST_MATCH|ARROW_LEFT_TO_RIGHT; return ARROW;}
(\(@\>\)) { yylval_param->type = ARROW_OPTIONAL|ARROW_RIGHT|ARROW_SHORTEST_MATCH|ARROW_LEFT_TO_RIGHT; return ARROW;}
(\-\>@) { yylval_param->type = ARROW_RIGHT|ARROW_LONGEST_MATCH|ARROW_RIGHT_TO_LEFT; return ARROW;}
(\(\-\>@\)) { yylval_param->type = ARROW_OPTIONAL|ARROW_RIGHT|ARROW_LONGEST_MATCH|ARROW_RIGHT_TO_LEFT; return ARROW;}
(\>@) { yylval_param->type = ARROW_RIGHT|ARROW_SHORTEST_MATCH|ARROW_RIGHT_TO_LEFT; return ARROW;}
(\(\>@\)) { yylval_param->type = ARROW_OPTIONAL|ARROW_RIGHT|ARROW_SHORTEST_MATCH|ARROW_RIGHT_TO_LEFT; return ARROW;}
(\<\-@) { yylval_param->type = ARROW_LEFT|ARROW_LONGEST_MATCH|ARROW_LEFT_TO_RIGHT; return ARROW;}
(\(\<\-@\)) { yylval_param->type = ARROW_OPTIONAL|ARROW_LEFT|ARROW_LONGEST_MATCH|ARROW_LEFT_TO_RIGHT; return ARROW;}
(\<@) { yylval_param->type = ARROW_LEFT|ARROW_SHORTEST_MATCH|ARROW_LEFT_TO_RIGHT; return ARROW;}
(\(\<@\)) { yylval_param->type = ARROW_OPTIONAL|ARROW_LEFT|ARROW_SHORTEST_MATCH|ARROW_LEFT_TO_RIGHT; return ARROW;}
(@\<\-) { yylval_param->type = ARROW_LEFT|ARROW_LONGEST_MATCH|ARROW_RIGHT_TO_LEFT; return ARROW;}
(\(@\<\-\)) { yylval_param->type = ARROW_OPTIONAL|ARROW_LEFT|ARROW_LONGEST_MATCH|ARROW_RIGHT_TO_LEFT; return ARROW;}
(@\<) { yylval_param->type = ARROW_LEFT|ARROW_SHORTEST_MATCH|ARROW_RIGHT_TO_LEFT; return ARROW;}
(\(@\<\)) { yylval_param->type = ARROW_OPTIONAL|ARROW_RIGHT|ARROW_SHORTEST_MATCH|ARROW_RIGHT_TO_LEFT; return ARROW;}

(\=\>) {                     return CRESTRICT;           }
([\140]) {                   return SUBSTITUTE;          }
(\|\|) {                     yylval_param->type = OP_UPWARD_REPLACE;    return DIRECTION; }
(\/\/) {                     yylval_param->type = OP_RIGHTWARD_REPLACE; return DIRECTION; }
(\\\\) {                     yylval_param->type = OP_LEFTWARD_REPLACE;  return DIRECTION; }
(\\\/) {                     yylval_param->type = OP_DOWNWARD_REPLACE;  return DIRECTION; }
(:) {                        return HIGH_CROSS_PRODUCT;  }
(\.x\.|[\303][\227]) {       return CROSS_PRODUCT;       }
(,) {                        return COMMA;               }
(,,) {                       return DOUBLE_COMMA;        }
(\.\/\.) {                   return IGNORE_INTERNAL;     }
(\/) {                       return IGNORE_ALL;          }
(\/\/\/) {                   return RIGHT_QUOTIENT;      }
(\\\\\\) {                   return LEFT_QUOTIENT;       }
(\/\\\/) {                   return INTERLEAVE_QUOTIENT; }
(\\) {                       return TERM_NEGATION;       }
(\-) {                       return MINUS;               }
(\$\?) {                     return CONTAINS_OPT_ONE;    }
(\$\.) {                     return CONTAINS_ONE;        }
(\$) {                       return CONTAINS;            }
(\+) {                       return KLEENE_PLUS;         }
(\*) {                       return KLEENE_STAR;         }
\.i|[\342][\201][\273][\302][\271] { return INVERSE;     }
(\.r) {                      return REVERSE;             }
(\[) {                       return LBRACKET;            }
(\]) {                       return RBRACKET;            }
[\342][\211][\272] {         return PRECEDES;            }
[\342][\211][\273] {         return FOLLOWS;             }
[\342][\211][\240] {         return NEQ;                 }
(\() {                       return LPAREN;              }
(\)) {                       return RPAREN;              }
(;m) {                       return ENDM;                } 
(;d) {                       return ENDD;                }
(;)  {                       return END;                 }
(\||[\342][\210][\250]|[\342][\210][\252]) { return UNION;     }
(\&|[\342][\210][\247]|[\342][\210][\251]) { return INTERSECT; }

((#|!).*\n) { yycolumn = 1; }
(\n+) {  }
(\r+) {  }
(.) { }
