/*     Foma: a finite-state toolkit and library.                             */
/*     Copyright © 2008-2014 Mans Hulden                                     */

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

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "foma.h"
#define MAX_F_RECURSION 100
extern int yyerror();
extern int yylex();
extern int my_yyparse(char *my_string, int lineno, struct defined_networks *defined_nets, struct defined_functions *defined_funcs);
struct fsm *current_parse;
int rewrite, rule_direction;
int substituting = 0;
static char *subval1, *subval2;
struct fsmcontexts *contexts;
struct fsmrules *rules;
struct rewrite_set *rewrite_rules;
static struct fsm *fargs[100][MAX_F_RECURSION];  /* Function arguments [number][frec] */
static int frec = -1;                            /* Current depth of function recursion */
static char *fname[MAX_F_RECURSION];             /* Function names */
static int fargptr[MAX_F_RECURSION];             /* Current argument no. */
/* Variable to produce internal symbols */
unsigned int g_internal_sym = 23482342;

void add_function_argument(struct fsm *net) {
    fargs[fargptr[frec]][frec] = net;
    fargptr[frec]++;
}

void declare_function_name(char *s) {
    if (frec > MAX_F_RECURSION) {
        printf("Function stack depth exceeded. Aborting.\n");
        exit(1);
    }
    fname[frec] = xxstrdup(s);
    xxfree(s);
}

struct fsm *function_apply(struct defined_networks *defined_nets, struct defined_functions *defined_funcs) {
    int i, mygsym, myfargptr;
    char *regex;
    char repstr[13], oldstr[13];
    if ((regex = find_defined_function(defined_funcs, fname[frec],fargptr[frec])) == NULL) {
        fprintf(stderr, "***Error: function %s@%i) not defined!\n",fname[frec], fargptr[frec]);
        return NULL;
    }
    regex = xxstrdup(regex);
    mygsym = g_internal_sym;
    myfargptr = fargptr[frec];
    /* Create new regular expression from function def. */
    /* and parse that */
    for (i = 0; i < fargptr[frec]; i++) {
        sprintf(repstr,"%012X",g_internal_sym);
        sprintf(oldstr, "@ARGUMENT%02i@", (i+1));
        streqrep(regex, oldstr, repstr);
        /* We temporarily define a network and save argument there */
        /* The name is a running counter g_internal_sym */
        add_defined(defined_nets, fargs[i][frec], repstr);
        g_internal_sym++;
    }
    my_yyparse(regex,1,defined_nets, defined_funcs);
    for (i = 0; i < myfargptr; i++) {
        sprintf(repstr,"%012X",mygsym);
        /* Remove the temporarily defined network */
        remove_defined(defined_nets, repstr);
        mygsym++;
    }
    xxfree(fname[frec]);
    frec--;
    xxfree(regex);
    return(current_parse);
}

void add_context_pair(struct fsm *L, struct fsm *R) {
    struct fsmcontexts *newcontext;
    newcontext = xxcalloc(1,sizeof(struct fsmcontexts));
    newcontext->left = L;
    newcontext->right = R;
    newcontext->next = contexts;
    contexts = newcontext;
}

void clear_rewrite_ruleset(struct rewrite_set *rewrite_rules) {
    struct rewrite_set *rule, *rulep;
    struct fsmcontexts *contexts, *contextsp;
    struct fsmrules *r, *rp;
    for (rule = rewrite_rules; rule != NULL; rule = rulep) {

	for (r = rule->rewrite_rules ; r != NULL; r = rp) {
	    fsm_destroy(r->left);
	    fsm_destroy(r->right);
	    fsm_destroy(r->right2);
	    rp = r->next;
	    xxfree(r);
	}
	
	for (contexts = rule->rewrite_contexts; contexts != NULL ; contexts = contextsp) {

	    contextsp = contexts->next;
	    fsm_destroy(contexts->left);
	    fsm_destroy(contexts->right);
	    fsm_destroy(contexts->cpleft);
	    fsm_destroy(contexts->cpright);
	    xxfree(contexts);
	}
       	rulep = rule->next;
	//fsm_destroy(rules->cpunion);
	xxfree(rule);
    }
}

void add_rewrite_rule() {
    struct rewrite_set *new_rewrite_rule;
    if (rules != NULL) {
        new_rewrite_rule = xxmalloc(sizeof(struct rewrite_set));
        new_rewrite_rule->rewrite_rules = rules;
        new_rewrite_rule->rewrite_contexts = contexts;
        new_rewrite_rule->next = rewrite_rules;
        new_rewrite_rule->rule_direction = rule_direction;
        new_rewrite_rule->cpunion = NULL;

        rewrite_rules = new_rewrite_rule;
        rules = NULL;
        contexts = NULL;
        rule_direction = 0;
    }
}

void add_rule(struct fsm *L, struct fsm *R, struct fsm *R2, int type) {
    struct fsm *test;
    struct fsmrules *newrule;
    rewrite = 1;
    newrule = xxmalloc(sizeof(struct fsmrules));

    if ((type & ARROW_DOTTED) != 0) {
        newrule->left = fsm_minus(fsm_copy(L), fsm_empty_string());
    } else {
        newrule->left = L;
    }
    newrule->right = R;
    newrule->right2 = R2;
    newrule->next = rules;
    newrule->arrow_type = type;
    if ((type & ARROW_DOTTED) != 0) {
        newrule->arrow_type = type - ARROW_DOTTED;
    }

    rules = newrule;

    if ((type & ARROW_DOTTED) != 0) {
        /* Add empty [..] -> B for dotted rules (only if LHS contains the empty string) */
        test = fsm_intersect(L,fsm_empty_string());
        if (!fsm_isempty(test)) {
            newrule = xxmalloc(sizeof(struct fsmrules));
            newrule->left = test;
            newrule->right = fsm_copy(R);
            newrule->right2 = fsm_copy(R2);
            newrule->next = rules;
            newrule->arrow_type = type;
            rules = newrule;
        } else {
	    //fsm_destroy(test);
	}
    }
}


%}

%union {
     char *string;
     struct fsm *net;
     int  type;
}

%pure-parser
%expect 686
%parse-param { void *scanner }
%parse-param { struct defined_networks *defined_nets }
%parse-param { struct defined_functions *defined_funcs } /* Assume yyparse is called with this argument */
%lex-param   { yyscan_t *scanner } /* Call flex functions with this argument      */
%locations
%initial-action {
    clear_quantifiers();
    rewrite = 0;
    contexts = NULL;
    rules = NULL;
    rewrite_rules = NULL;
    rule_direction = 0;
    substituting = 0;
};

/* precedence
   \ `                      Term complement, Substitution
   :                        High-precedence crossproduct operator
   + * ^ .1 .2 .u .l .i .r  Kleene plus and star, iteration, upper-lower, invert and reverse
   ~ $ $. $?                Complement, containments
   / /// \\\ /\/            Ignore, Quotients
                            Concatenation (no overt operator)
   > <                      Precede and follow
   | & - .P. .p.            Union, intersect, minus, priority unions
   => -> (->) @-> etc.      Rule operators
   <>                       Shuffle
   _ args
   .x. .o. .O.              Crossproduct and composes
*/

%token <net> NET
%token <string> END LBRACKET RBRACKET LPAREN RPAREN ENDM ENDD CRESTRICT CONTAINS CONTAINS_OPT_ONE CONTAINS_ONE XUPPER XLOWER FLAG_ELIMINATE IGNORE_ALL IGNORE_INTERNAL CONTEXT NCONCAT MNCONCAT MORENCONCAT LESSNCONCAT DOUBLE_COMMA COMMA SHUFFLE PRECEDES FOLLOWS RIGHT_QUOTIENT LEFT_QUOTIENT INTERLEAVE_QUOTIENT UQUANT EQUANT VAR IN IMPLIES BICOND EQUALS NEQ SUBSTITUTE SUCCESSOR_OF PRIORITY_UNION_U PRIORITY_UNION_L LENIENT_COMPOSE TRIPLE_DOT LDOT RDOT FUNCTION SUBVAL ISUNAMBIGUOUS ISIDENTITY ISFUNCTIONAL NOTID LOWERUNIQ LOWERUNIQEPS ALLFINAL UNAMBIGUOUSPART AMBIGUOUSPART AMBIGUOUSDOMAIN EQSUBSTRINGS LETTERMACHINE MARKFSMTAIL MARKFSMTAILLOOP MARKFSMMIDLOOP MARKFSMLOOP ADDSINK LEFTREWR FLATTEN SUBLABEL CLOSESIGMA CLOSESIGMAUNK

%token <type> ARROW DIRECTION

%type <net> network networkA n0 network1 network2 network3 network4 network5 network6 network7 network8 network9 network10 network11 network12 fstart fmid fend sub1 sub2

%left COMPOSE CROSS_PRODUCT HIGH_CROSS_PRODUCT COMMA SHUFFLE PRECEDES FOLLOWS LENIENT_COMPOSE
%left UNION INTERSECT MINUS
%left COMPLEMENT
%left KLEENE_STAR KLEENE_PLUS REVERSE INVERSE
%left TERM_NEGATION

/* Regular expression grammar */
%%

start: regex
|      regex start

regex:
network END                        { current_parse = $1;              }

network: networkA { }
| network COMPOSE networkA         { $$ = fsm_compose($1,$3);         }
| network LENIENT_COMPOSE networkA { $$ = fsm_lenient_compose($1,$3); }
| network CROSS_PRODUCT networkA   { $$ = fsm_cross_product($1,$3);   }

networkA: n0 { if (rewrite) { add_rewrite_rule(); $$ = fsm_rewrite(rewrite_rules); clear_rewrite_ruleset(rewrite_rules); } rewrite = 0; contexts = NULL; rules = NULL; rewrite_rules = NULL; }

n0: network1 { }
| n0 CONTEXT n0          { $$ = NULL; add_context_pair($1,$3);}
| n0 CONTEXT             { add_context_pair($1,fsm_empty_string()); }
| n0 CONTEXT COMMA n0    { add_context_pair($1,fsm_empty_string()); }
| CONTEXT                { add_context_pair(fsm_empty_string(),fsm_empty_string());}
| CONTEXT DOUBLE_COMMA n0 { add_rewrite_rule(); add_context_pair(fsm_empty_string(),fsm_empty_string());}
| n0 CONTEXT DOUBLE_COMMA n0 { add_rewrite_rule(); add_context_pair($1,fsm_empty_string());}
| CONTEXT COMMA n0       { add_context_pair(fsm_empty_string(),fsm_empty_string());}
| CONTEXT n0             { add_context_pair(fsm_empty_string(),$2); }
| CONTEXT n0 COMMA n0    { add_context_pair(fsm_empty_string(),$2); }
| n0 CONTEXT n0 COMMA n0 { add_context_pair($1,$3); }
| n0 CRESTRICT n0        { $$ = fsm_context_restrict($1,contexts); fsm_clear_contexts(contexts);}
| n0 ARROW n0            { add_rule($1,$3,NULL,$2); if ($1->arity == 2) { printf("Error: LHS is transducer\n"); YYERROR;}}
| n0 ARROW               { add_rule($1,NULL,NULL,$2); }

| LDOT n0 RDOT ARROW n0  { add_rule($2,$5,NULL,$4|ARROW_DOTTED); if ($5 == NULL) { YYERROR;}}
| LDOT RDOT ARROW n0  { add_rule(fsm_empty_string(),$4,NULL,$3|ARROW_DOTTED); if ($4 == NULL) { YYERROR;}}
| LDOT n0 RDOT ARROW n0 COMMA n0 { add_rule($2,$5,NULL,$4|ARROW_DOTTED);}
| LDOT RDOT ARROW n0 COMMA n0 { add_rule(fsm_empty_string(),$4,NULL,$3|ARROW_DOTTED);}
| LDOT n0 RDOT ARROW n0 DIRECTION n0 { add_rule($2,$5,NULL,$4|ARROW_DOTTED); rule_direction = $6;}
| LDOT RDOT ARROW n0 DIRECTION n0 { add_rule(fsm_empty_string(),$4,NULL,$3|ARROW_DOTTED); rule_direction = $5;}
| n0 ARROW n0 COMMA n0 { add_rule($1,$3,NULL,$2);}
| n0 ARROW COMMA n0 { add_rule($1,NULL,NULL,$2);}
| n0 ARROW n0 DIRECTION n0 { add_rule($1,$3,NULL,$2); rule_direction = $4;}
| n0 ARROW DIRECTION n0 { add_rule($1,NULL,NULL,$2); rule_direction = $3;}

| n0 DOUBLE_COMMA n0  { add_rewrite_rule();}

| n0 ARROW TRIPLE_DOT     { add_rule($1,fsm_empty_string(),fsm_empty_string(),$2);}
| n0 ARROW TRIPLE_DOT n0  { add_rule($1,fsm_empty_string(),$4,$2);}
| n0 ARROW n0 TRIPLE_DOT  { add_rule($1,$3,fsm_empty_string(),$2);}
| n0 ARROW n0 TRIPLE_DOT n0 { add_rule($1,$3,$5,$2);}
| n0 ARROW n0 TRIPLE_DOT n0 COMMA n0 { add_rule($1,$3,$5,$2);}
| n0 ARROW n0 TRIPLE_DOT COMMA n0 { add_rule($1,$3,fsm_empty_string(),$2);}
| n0 ARROW TRIPLE_DOT n0 COMMA n0 { add_rule($1,fsm_empty_string(),$4,$2);}
| n0 ARROW TRIPLE_DOT COMMA n0  { add_rule($1,fsm_empty_string(),fsm_empty_string(),$2);}
| n0 ARROW n0 TRIPLE_DOT n0 DIRECTION n0 { add_rule($1,$3,$5,$2); rule_direction = $6;}
| n0 ARROW TRIPLE_DOT n0 DIRECTION n0 { add_rule($1,fsm_empty_string(),$4,$2); rule_direction = $5;}
| n0 ARROW n0 TRIPLE_DOT DIRECTION n0 { add_rule($1,$3,fsm_empty_string(),$2); rule_direction = $5;}
| n0 ARROW TRIPLE_DOT DIRECTION n0 { add_rule($1,fsm_empty_string(),fsm_empty_string(),$2); rule_direction = $4;}


network1: network2 { }
| network1 SHUFFLE network2   { $$ = fsm_shuffle($1,$3);  }
| network1 PRECEDES network2  { $$ = fsm_precedes($1,$3); }
| network1 FOLLOWS network2   { $$ = fsm_follows($1,$3);  }

network2: network3 { };

network3: network4 { };

network4: network5 { }
| network4 UNION network5            { $$ = fsm_union($1,$3);                     }
| network4 PRIORITY_UNION_U network5 { $$ = fsm_priority_union_upper($1,$3);      }
| network4 PRIORITY_UNION_L network5 { $$ = fsm_priority_union_lower($1,$3);      }
| network4 INTERSECT network5        { $$ = fsm_intersect($1,$3);                 }
| network4 MINUS network5            { $$ = fsm_minus($1,$3);                     }
| network4 IMPLIES network5          { $$ = fsm_union(fsm_complement($1),$3);     }
| network4 BICOND network5           { $$ = fsm_intersect(fsm_union(fsm_complement(fsm_copy($1)),fsm_copy($3)), fsm_union(fsm_complement(fsm_copy($3)),fsm_copy($1))); fsm_destroy($1); fsm_destroy($3);}

network5: network6  { }
| network5 network6 { $$ = fsm_concat($1,$2); }
| VAR IN network5   { $$ = fsm_ignore(fsm_contains(fsm_concat(fsm_symbol($1),fsm_concat($3,fsm_symbol($1)))),union_quantifiers(),OP_IGNORE_ALL); }

| VAR EQUALS VAR    { $$ = fsm_logical_eq($1,$3); }
| VAR NEQ VAR       { $$ = fsm_complement(fsm_logical_eq($1,$3)); }
| VAR PRECEDES VAR  { $$ = fsm_logical_precedence($1,$3); }
| VAR FOLLOWS VAR   { $$ = fsm_logical_precedence($3,$1); }

network6: network7 { }
| network6 IGNORE_ALL network7           { $$ = fsm_ignore($1,$3, OP_IGNORE_ALL);          }
| network6 IGNORE_INTERNAL network7      { $$ = fsm_ignore($1,$3, OP_IGNORE_INTERNAL);     }
| network6 RIGHT_QUOTIENT network7       { $$ = fsm_quotient_right($1,$3);                 }
| network6 LEFT_QUOTIENT network7        { $$ = fsm_quotient_left($1,$3);                  }
| network6 INTERLEAVE_QUOTIENT network7  { $$ = fsm_quotient_interleave($1,$3);            } 

network7: network8 { }
| COMPLEMENT network7         { $$ = fsm_complement($2);       }
| CONTAINS network7           { $$ = fsm_contains($2);         }
| CONTAINS_ONE network7       { $$ = fsm_contains_one($2);     }
| CONTAINS_OPT_ONE network7   { $$ = fsm_contains_opt_one($2); }

network8: network9 { }

network9: network10 { }
| network9 KLEENE_STAR                  { $$ = fsm_kleene_star(fsm_minimize($1)); }
| network9 KLEENE_PLUS                  { $$ = fsm_kleene_plus($1); }
| network9 REVERSE                      { $$ = fsm_determinize(fsm_reverse($1)); }
| network9 INVERSE                      { $$ = fsm_invert($1); }
| network9 XUPPER                       { $$ = fsm_upper($1); }
| network9 XLOWER                       { $$ = fsm_lower($1); }
| network9 FLAG_ELIMINATE               { $$ = flag_eliminate($1, NULL); }
| network9 HIGH_CROSS_PRODUCT network10 { $$ = fsm_cross_product($1,$3); }

| network9 NCONCAT        { $$ = fsm_concat_n($1,atoi($2)); }
| network9 MORENCONCAT    { $$ = fsm_concat(fsm_concat_n(fsm_copy($1), atoi($2)),fsm_kleene_plus(fsm_copy($1))); fsm_destroy($1); }
| network9 LESSNCONCAT    { $$ = fsm_concat_m_n($1,0,atoi($2)-1); }
| network9 MNCONCAT       { $$ = fsm_concat_m_n($1,atoi($2),atoi(strstr($2,",")+1)); }

network10: network11 { }
| TERM_NEGATION network10 { $$ = fsm_term_negation($2); }

network11: NET { $$ = $1;}
| network12 { $$ = $1; }
| UQUANT LPAREN network RPAREN { $$ = fsm_complement(fsm_substitute_symbol(fsm_intersect(fsm_quantifier($1),fsm_complement($3)),$1,"@_EPSILON_SYMBOL_@")); purge_quantifier($1); }
| EQUANT network {  $$ = fsm_substitute_symbol(fsm_intersect(fsm_quantifier($1),$2),$1,"@_EPSILON_SYMBOL_@"); purge_quantifier($1); }
| LPAREN network RPAREN { if (count_quantifiers()) $$ = $2; else {$$ = fsm_optionality($2);} }
| LBRACKET network RBRACKET { $$ = $2; }
| SUCCESSOR_OF VAR COMMA VAR RPAREN {$$ = fsm_concat(fsm_universal(),fsm_concat(fsm_symbol($2),fsm_concat(fsm_universal(),fsm_concat(fsm_symbol($2),fsm_concat(union_quantifiers(),fsm_concat(fsm_symbol($4),fsm_concat(fsm_universal(),fsm_concat(fsm_symbol($4),fsm_universal())))))))); }
| SUCCESSOR_OF VAR COMMA network RPAREN {$$ = fsm_concat(fsm_universal(),fsm_concat(fsm_symbol($2),fsm_concat(fsm_universal(),fsm_concat(fsm_symbol($2),fsm_concat(fsm_ignore($4,union_quantifiers(),OP_IGNORE_ALL),fsm_universal()))))); }
| SUCCESSOR_OF network COMMA VAR RPAREN {$$ = fsm_concat(fsm_universal(),fsm_concat(fsm_ignore($2,union_quantifiers(),OP_IGNORE_ALL),fsm_concat(fsm_symbol($4),fsm_concat(fsm_universal(),fsm_concat(fsm_symbol($4),fsm_universal()))))); }
| sub1 sub2 {$$ = fsm_substitute_symbol($1,subval1,subval2); substituting = 0; xxfree(subval1); xxfree(subval2); subval1 = subval2 = NULL;}

sub1: SUBSTITUTE LBRACKET network COMMA { $$ = $3; substituting = 1;                      }
sub2: SUBVAL COMMA SUBVAL RBRACKET      { subval1 = $2; subval2 = $4; }

network12: fend    { $$ = $1; } |
         ISIDENTITY   network RPAREN    { $$ = fsm_boolean(fsm_isidentity($2));   } |
         ISFUNCTIONAL network RPAREN    { $$ = fsm_boolean(fsm_isfunctional($2)); } |
         ISUNAMBIGUOUS network RPAREN   { $$ = fsm_boolean(fsm_isunambiguous($2)); } |
         NOTID network RPAREN           { $$ = fsm_extract_nonidentity(fsm_copy($2)); } |
         LOWERUNIQ network RPAREN       { $$ = fsm_lowerdet(fsm_copy($2)); } |
         LOWERUNIQEPS network RPAREN    { $$ = fsm_lowerdeteps(fsm_copy($2)); } |
         ALLFINAL network RPAREN        { $$ = fsm_markallfinal(fsm_copy($2)); } |
         UNAMBIGUOUSPART network RPAREN { $$ = fsm_extract_unambiguous(fsm_copy($2));      } |
         AMBIGUOUSPART network RPAREN   { $$ = fsm_extract_ambiguous(fsm_copy($2));        } |
         AMBIGUOUSDOMAIN network RPAREN { $$ = fsm_extract_ambiguous_domain(fsm_copy($2)); } |
         LETTERMACHINE network RPAREN   { $$ = fsm_letter_machine(fsm_copy($2)); }   |
         MARKFSMTAIL network COMMA network RPAREN { $$ = fsm_mark_fsm_tail($2,$4); } |
         MARKFSMTAILLOOP network COMMA network RPAREN { $$ = fsm_add_loop($2,$4,1); } |
         MARKFSMMIDLOOP network COMMA network RPAREN { $$ = fsm_add_loop($2,$4,0); } |
         MARKFSMLOOP network COMMA network RPAREN { $$ = fsm_add_loop($2,$4,2); } |
         ADDSINK network RPAREN { $$ = fsm_add_sink($2,1); } |
         LEFTREWR network COMMA network RPAREN { $$ = fsm_left_rewr($2,$4); } |
         FLATTEN network COMMA network RPAREN { $$ = fsm_flatten($2,$4); } |
         SUBLABEL network COMMA network COMMA network RPAREN { $$ = fsm_substitute_label($2, fsm_network_to_char($4), $6); } |
         CLOSESIGMA    network RPAREN { $$ = fsm_close_sigma(fsm_copy($2), 0); } |
         CLOSESIGMAUNK network RPAREN { $$ = fsm_close_sigma(fsm_copy($2), 1); } |
         EQSUBSTRINGS network COMMA network COMMA network RPAREN { $$ = fsm_equal_substrings($2,$4,$6); }
      
fstart: FUNCTION network COMMA
{ frec++; fargptr[frec] = 0 ;declare_function_name($1) ; add_function_argument($2); }
|       FUNCTION network       
{ frec++; fargptr[frec] = 0 ;declare_function_name($1) ; add_function_argument($2); }

fmid:   fstart network COMMA   { add_function_argument($2); }
|       fmid   network COMMA   { add_function_argument($2); }

fend:   fmid   network RPAREN  
{ add_function_argument($2); if (($$ = function_apply(defined_nets, defined_funcs)) == NULL) YYERROR; }
|       fstart network RPAREN  
{ add_function_argument($2); if (($$ = function_apply(defined_nets, defined_funcs)) == NULL) YYERROR; }
|       fstart         RPAREN  
{ if (($$ = function_apply(defined_nets, defined_funcs)) == NULL) YYERROR;}

%%
