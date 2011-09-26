/*     Foma: a finite-state toolkit and library.                             */
/*     Copyright © 2008-2011 Mans Hulden                                     */

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
%option yylineno
%top{
#define YY_BUF_SIZE 16777216
}
%{
#include <stdio.h>
#include "foma.h"
#define RE 0 /* regex  */
#define DE 1 /* define */

char funcdef[500];

struct func_args {
   char *arg; 
   int argno;
   struct func_args *next;
};

struct func_args *func_args = NULL;
struct apply_handle *ah;

static char *func_name;
static char *tempstr;
static char *result;
static char *lexcfilein;
static char *cmatrixfilein;
static int pmode, i;
static int func_arg_no;
static int olddef;
static int applydir;
static struct fsm *tempnet;
extern int promptmode;
extern int apply_direction;
extern int g_list_limit;

extern int my_yyparse(char *my_string, int lineno);
extern void my_cmatrixparse(struct fsm *net, char *my_string);
extern struct fsm *current_parse;
extern struct fsm *fsm_lexc_parse_string(char *string);
extern int interfacelex();
extern struct fsm *current_parse;
extern void lexc_trim(char *s);

int input_is_file;

int get_iface_lineno(void) {
  return(yylineno);
}

void clear_func_args(void) {
    struct func_args *fa, *fp;  
    for (fa = func_args; fa != NULL; ) {
	xxfree(fa->arg);
	fp = fa;
	fa = fa->next;
	xxfree(fp);
    }
    func_args = NULL;
}

void add_func_arg(char *s) {
  struct func_args *fa;
  fa = xxmalloc(sizeof(struct func_args));
  fa->arg = xxstrdup(s);
  fa->next = func_args;
  fa->argno = func_arg_no;
  func_arg_no++;
  func_args = fa;
}

char *rep_func_arg(char *s) {
  struct func_args *fa;  
  char *argstr; 
  for (fa = func_args; fa != NULL; fa = fa->next) {
    if (strcmp(fa->arg,s) == 0) {      
        argstr = xxmalloc(sizeof(char)*20);
        sprintf(argstr, "@ARGUMENT%02i@", fa->argno);
        return(argstr);
    }
  }
  return(s);
}

void my_interfaceparse(char *my_string) {

   YY_BUFFER_STATE my_string_buffer;
   my_string_buffer = interface_scan_string(my_string);
   interfacelineno = 1;
   func_args = NULL;
   interfacelex();
   //interface_delete_buffer(my_string_buffer);
}

%}

NONRESERVED [0-9A-Za-z\?\'\=]|[\300-\301].|[\302]([\000-\377]{-}[\254])|[\303]([\000-\377]{-}[\227])|[\304-\315].|[\316]([\000-\377]{-}[\243\265])|[\317-\337].|[\340-\341]..|[\342][\000-\200].|[\342][201][\000-\377]{-}[\273]|[\342][\202][\000-\377]{-}[\201\202]|[\342][\203-\205][\000-\377][\342][\206]([\000-\377]{-}[\222\224])|[\342][\207].|[\342][\210]([\000-\377]{-}[\200\203\205\210\230\245\247\250\251\252])|[\342][\211]([\000-\377]{-}[\240\244\245\272\273])|[\342][\212-\377].|[\343-\357]..|[\360-\367]...

NONL       [\001-\177]{-}[\012\015]|[\300-\337].|[\340-\357]..|[\360-\367]...
ANY        [\001-\177]|[\300-\337].|[\340-\357]..|[\360-\367]...

NONE [\001-\177]{-}[\073\173\175\042\045]|[\300-\301].|[\302]([\000-\377]{-}[\254])|[\303]([\000-\377]{-}[\227])|[\304-\315].|[\316]([\000-\377]{-}[\243\265])|[\317-\337].|[\340-\341]..|[\342][\000-\205].|[\342][\206]([\000-\377]{-}[\222\224])|[\342][\207].|[\342][\210]([\000-\377]{-}[\200\203\205\210\230\245\247\250\251\252])|[\342][\211]([\000-\377]{-}[\240\244\245\272\273])|[\342][\212-\377].|[\343-\357]..|[\360-\367]...
NONESCAPED [\001-\177]{-}[\073\173\175\042\045]|[\300-\337].|[\340-\357]..|[\360-\367]...
INREGEX [\001-\177]{-}[\041\043\073\173\175\042\045]|[\300-\337].|[\340-\357]..|[\360-\367]...


BRACED     [{]([^}]|[\300-\337].|[\340-\357]..|[\360-\367]...)+[}]
QUOTED     [\042]([^"]|[\300-\337].|[\340-\357]..|[\360-\367]...)+["]
SP         [\040]|[\011]
NOSP       [^ \t];
NOSPEQ     [\001-\177]{-}[\040\075]|[\300-\337].|[\340-\357]..|[\360-\367]...

%x REGEX DEFI DEF SOURCE APPLY_DOWN APPLY_FILE_IN APPLY_MED APPLY_UP APPLY_P ELIMINATE_FLAG UNDEFINE RPL RLEXC RCMATRIX READ_TEXT READ_SPACED_TEXT SHOW_VAR SET_VAR SET_VALUE SAVE_STACK SAVE_DEFINED LOAD_STACK LOAD_DEFINED IGNORELINE REGEXQ REGEXB PUSH NAME_NET ECHO SYSTEM FUNC_1 FUNC_2 FUNC_3 FUNC_4 APROPOS HELP PRINT_NET_FILE PRINT_NET_NAME PRINT_NET_NAME_FILE PRINT_NET_NAME_FILE2 PRINT_DOT_FILE PRINT_DOT_NAME SUBSTITUTE_SYMBOL SUBSTITUTE_SYMBOL_2 SUBSTITUTE_SYMBOL_3 SUBSTITUTE_DEFINED SUBSTITUTE_DEFINED_2 SUBSTITUTE_DEFINED_3 WRITE_ATT_FILE WRITE_PROLOG_FILE RCOMMENT APPLY_FILE_EATUP APPLY_FILE_OUT ATT EXCMATRIX ASSERT_STACK

%%
%{ 
  if (YY_START == APPLY_P && promptmode == PROMPT_MAIN) 
    BEGIN(INITIAL); 
%}

^{SP}*(ambiguous{SP}+upper|ambiguous){SP}* { iface_ambiguous_upper(); }
^{SP}*(apply{SP}+)?down{SP}*/[^ \t<] { BEGIN(APPLY_DOWN);}
^{SP}*(apply{SP}+)?down{SP}* { if (iface_stack_check(1)) {promptmode = PROMPT_A; apply_direction = AP_D; ah = stack_get_ah(); BEGIN(APPLY_P);}}
^{SP}*(apply{SP}+)?down{SP}*[ \t]*<[ ]* { applydir = AP_D; BEGIN(APPLY_FILE_IN);}
^{SP}*(apply{SP}+)?med{SP}* { if (iface_stack_check(1)) {promptmode = PROMPT_A; apply_direction = AP_M; BEGIN(APPLY_P);}}
^{SP}*(apply{SP}+)?med{SP}*/[^ \t] { BEGIN(APPLY_MED);}
^{SP}*(apply{SP}+)?up{SP}*/[^ \t] { BEGIN(APPLY_UP);}
		  ^{SP}*(apply{SP}+)?up{SP}* { if (iface_stack_check(1)) {promptmode = PROMPT_A; apply_direction = AP_U; ah = stack_get_ah(); BEGIN(APPLY_P);}}
^{SP}*(apply{SP}+)?up{SP}*[ \t]*<[ ]* { applydir = AP_U; BEGIN(APPLY_FILE_IN);}
^{SP}*apr(o(p(os?)?)?)?{SP}+/[^ ] {BEGIN(APROPOS); }
^{SP}*assert-stack{SP} {BEGIN(ASSERT_STACK);}
^{SP}*clear({SP}+st(a(ck?)?)?)? {  stack_clear();}
^{SP}*comp(a(ct?)?){SP}+sig(ma?)? { iface_compact(); }
^{SP}*compl(e(te?)?)?({SP}+net?)?   { iface_complete();}
^{SP}*compo(se?)?({SP}+net?)? { iface_compose();}
^{SP}*conc(a(t(e(n(a(te?)?)?)?)?)?)?({SP}+net?)? {iface_conc();}
^{SP}*cross(p(r(o(d(u(ct?)?)?)?)?)?)?({SP}+net?)? { iface_crossproduct();}
^{SP}*de(f(i(ne?)?)?)?{SP}+ { pmode = DE; BEGIN(DEFI); }
^{SP}*{NOSPEQ}+{SP}*= { pmode = DE; lexc_trim(interfacetext); tempstr = xxstrdup(interfacetext); BEGIN(REGEX); }
^{SP}*det(e(r(m(i(n(i(ze?)?)?)?)?)?)?)?({SP}+net?)?	{ iface_determinize(); }
^{SP}*echo{SP} { BEGIN(ECHO); }
^{SP}*echo { printf("\n"); }
^{SP}*eliminate{SP}+flags { iface_eliminate_flags(); }
^{SP}*eliminate{SP}+flag{SP}+ { BEGIN(ELIMINATE_FLAG); }
^{SP}*export{SP}+cmatrix             { iface_print_cmatrix_att(NULL); }
^{SP}*export{SP}+cmatrix{SP}*>?{SP}*/[^ \t] { BEGIN(EXCMATRIX);          }
^{SP}*(extract{SP}+ambiguous|examb){SP}* { iface_extract_ambiguous(); }
^{SP}*(extract{SP}+unambiguous|exunamb){SP}* { iface_extract_unambiguous(); }
^{SP}*(h(e(l(p{SP}+)?)?)?)?(licen(c|s)e|warranty) { iface_warranty(); }
^{SP}*help{SP}* { iface_help(); }
^{SP}*help{SP}+/[^ ] { BEGIN(HELP); }
^{SP}*igno(re?)({SP}+net?)? { iface_ignore(); }
^{SP}*intersect({SP}+net?)? { iface_intersect();}
^{SP}*inv(e(rt?)?)?({SP}+net?)? { iface_invert(); }
^{SP}*label{SP}+net{SP}* { iface_label_net(); }
^{SP}*letter{SP}+machine{SP}* { iface_letter_machine(); }
^{SP}*(load{SP}+defined{SP}+|loadd{SP}+){SP}* { BEGIN(LOAD_DEFINED); }
^{SP}*(load{SP}+stack{SP}+?|load{SP}+){SP}* { BEGIN(LOAD_STACK); }
^{SP}*lower\-side({SP}+net?)? {  iface_lower_side();}
^{SP}*min(imize)?({SP}+net?)? { iface_minimize(); }
^{SP}*na(me?)?({SP}+net?)?{SP}+ { BEGIN(NAME_NET); }
^{SP}*neg(a(te?)?)?({SP}+net?)? { iface_negate(); }
^{SP}*on(e(\-(p(l(us?)?)?)?)?)?({SP}+net?)? { iface_one_plus();}
^{SP}*pop?|pop{SP}+st(a(ck?)?)? { iface_pop();}
^{SP}*(pr(i(nt?)?)?{SP}+)?cma(t(r(ix?)?)?)? { iface_print_cmatrix(); }
^{SP}*(pr(i(nt?)?)?{SP}+)?def(i(n(ed?)?)?)? { iface_print_defined(); }
^{SP}*(pr(i(nt?)?)?{SP}+)?dot{SP}* { iface_print_dot(NULL); }
^{SP}*(pr(i(nt?)?)?{SP}+)?dot{SP}*>{SP}*/[^ >] { BEGIN(PRINT_DOT_FILE); }
^{SP}*(pr(i(nt?)?)?{SP}+)?dot{SP}+/[^ >] { BEGIN(PRINT_DOT_NAME); }
^{SP}*(pr(i(nt?)?)?{SP}+)?low(e(r\-(w(o(r(ds?)?)?)?)?)?)? { iface_lower_words(-1); }
^{SP}*(pr(i(nt?)?)?{SP}+)?low(e(r\-(w(o(r(ds?)?)?)?)?)?)?{SP}+[0-9]+ { iface_lower_words(iface_extract_number(interfacetext)); }
^{SP}*(pr(i(nt?)?)?{SP}+)?na(me?)? { iface_print_name(); }
^{SP}*(pr(i(nt?)?)?{SP}+)?net{SP}* { iface_print_net(NULL,NULL); }
^{SP}*(pr(i(nt?)?)?{SP}+)?net{SP}*>{SP}*/[^ >] { BEGIN(PRINT_NET_FILE);      }
^{SP}*(pr(i(nt?)?)?{SP}+)?net{SP}+/[^ >\t] { BEGIN(PRINT_NET_NAME);      }
^{SP}*(pr(i(nt?)?)?{SP}+)?net{SP}+/[^ >\t][ \t]*>[ \t]*[^ >] { BEGIN(PRINT_NET_NAME_FILE); }
^{SP}*(pr(i(nt?)?)?{SP}+)?random\-lower { iface_random_lower(-1); }
^{SP}*(pr(i(nt?)?)?{SP}+)?random\-lower{SP}+[0-9]+ { iface_random_lower(iface_extract_number(interfacetext)); }
^{SP}*(pr(i(nt?)?)?{SP}+)?random\-upper { iface_random_upper(-1); }
^{SP}*(pr(i(nt?)?)?{SP}+)?random\-upper{SP}+[0-9]+ { iface_random_upper(iface_extract_number(interfacetext)); }
^{SP}*(pr(i(nt?)?)?{SP}+)?random\-words { iface_random_words(-1); }
^{SP}*(pr(i(nt?)?)?{SP}+)?random\-words{SP}+[0-9]+ { iface_random_words(iface_extract_number(interfacetext)); }
^{SP}*(pr(i(nt?)?)?{SP}+)?sig(ma?)? { iface_print_sigma(); }
^{SP}*(pr(i(nt?)?)?{SP}+)?size? { iface_print_stats(); }
^{SP}*(pr(i(nt?)?)?{SP}+)?stack-size? { printf("STACK SIZE: %d\n", stack_size()); }
^{SP}*(pr(i(nt?)?)?{SP}+)?upp(e(r(\-(w(o(r(ds?)?)?)?)?)?)?)? { iface_upper_words(-1); }
^{SP}*(pr(i(nt?)?)?{SP}+)?upp(e(r(\-(w(o(r(ds?)?)?)?)?)?)?)?{SP}+[0-9]+ { iface_upper_words(iface_extract_number(interfacetext)); }
^{SP}*(pr(i(nt?)?)?{SP}+)?words { iface_words(-1);}
^{SP}*((pr(i(nt?)?)?{SP}+)?shortest-string|pss) { iface_print_shortest_string();}
^{SP}*((pr(i(nt?)?)?{SP}+)?shortest-string-size|psz) { iface_print_shortest_string_size();}
^{SP}*(pr(i(nt?)?)?{SP}+)?words{SP}+[0-9]+ { iface_words(iface_extract_number(interfacetext));}
^{SP}*pru(ne?)?({SP}+net?)?  { iface_prune(); }
^{SP}*pu(sh?)?({SP}+def(i(n(ed?)?)?)?)?{SP}*  { BEGIN(PUSH); }
^{SP}*(quit|au{SP}+revoir|bye|exit|hyv[\303][\244]sti) { iface_quit(); }
^{SP}*(re(ad?)?{SP}+)?att{SP}*<?{SP}*|ratt{SP}*<?{SP}* { BEGIN(ATT); }
^{SP}*(re(ad?)?{SP}+)?cmatrix{SP}* { printf("Missing filename.\n"); }
^{SP}*(re(ad?)?{SP}+)?cmatrix{SP}*<?{SP}*/[^ \t] { BEGIN(RCMATRIX); }
^{SP}*(re(ad?)?{SP}+)?prol(og?)?{SP}*<?{SP}*|rpl{SP}*<?{SP}* { BEGIN(RPL); }
^{SP}*(re(ad?)?{SP}+)?lexc{SP}*<?{SP}* { BEGIN(RLEXC); }
^{SP}*(re(ad?)?{SP}+)?re(g(ex?)?)?{SP}+ { pmode = RE; BEGIN(REGEX); }
^{SP}*(re(ad?)?{SP}+)?spaced\-text{SP}*<?{SP}* { BEGIN(READ_SPACED_TEXT); }
^{SP}*(re(ad?)?{SP}+)?text{SP}*<?{SP}* { BEGIN(READ_TEXT); }
^{SP}*rev(e(r(se?)?)?)?({SP}+net?)?  { iface_reverse(); }
^{SP}*rot(a(te?)?)?({SP}+st(a(ck?)?)?)? { iface_rotate();}
^{SP}*(save{SP}+defined{SP}+|saved{SP}+)>?{SP}* { BEGIN(SAVE_DEFINED); }
^{SP}*(save{SP}+stack{SP}+?|ss{SP}+)>?{SP}* { BEGIN(SAVE_STACK); }
^{SP}*set[ ]+/[^ ]+[ ]+[^ ]+ { BEGIN(SET_VAR); }
^{SP}*show{SP}+var(i(a(b(l(es?)?)?)?)?)? { iface_show_variables();}
^{SP}*show({SP}+(v(a(r(i(a(b(l(e)?)?)?)?)?)?)?)?)?{SP}+/[^ ] { BEGIN(SHOW_VAR); }
^{SP}*shu(f(f(le?)?)?)?({SP}+net?)?  { iface_shuffle(); }
^{SP}*sigma{SP}+net{SP}* { iface_sigma_net(); }
^{SP}*so(rt?)?({SP}+net?)?  { iface_sort(); }
^{SP}*sou(r(ce?)?)?{SP}+ {BEGIN(SOURCE);}
^{SP}*sub(s(t(i(t(u(te?)?)?)?)?)?)?{SP}+def(i(n(ed?)?)?)?{SP}+ {BEGIN(SUBSTITUTE_DEFINED);}
^{SP}*sub(s(t(i(t(u(te?)?)?)?)?)?)?{SP}+sym(b(ol?)?)?{SP}+ {BEGIN(SUBSTITUTE_SYMBOL);}
^{SP}*sy(s(t(em?)?)?)?{SP}+ {BEGIN(SYSTEM);}
^{SP}*(test{SP}+unambiguous|tunam) {  iface_test_unambiguous(); }
^{SP}*(test{SP}+equivalent|equ) {  iface_test_equivalent(); }
^{SP}*(test{SP}+functional|tfu) {  iface_test_functional(); }
^{SP}*(test{SP}+identity|tid) {  iface_test_identity(); }
^{SP}*(test{SP}+non-null|tnn) {  iface_test_nonnull(); }
^{SP}*(test{SP}+null|tnu) {  iface_test_null(); }
^{SP}*(test{SP}+lower-universal|tlu) {  iface_test_lower_universal(); }
^{SP}*(test{SP}+sequential|tseq) {  iface_test_sequential(); }
^{SP}*(test{SP}+upper-univesal|tuu) {  iface_test_upper_universal(); }
^{SP}*tu(rn?)?({SP}+st(a(ck?)?)?)? {  iface_turn(); }
^{SP}*(twosided{SP}+flag-diacritics|tfd) {  iface_twosided_flags(); }
^{SP}*und(e(f(i(ne?)?)?)?)?{SP}+ {BEGIN(UNDEFINE);}
^{SP}*uni(on?)?({SP}+net?)? {iface_union();}
^{SP}*upper\-side({SP}+n(et?)?)? { iface_upper_side(); }
^{SP}*view({SP}+net?)? { iface_view(); }
^{SP}*(((wr(i(te?)?)?{SP}+)?prolog)|(wpl)){SP}* { iface_write_prolog(NULL); }
^{SP}*(((wr(i(te?)?)?{SP}+)?prolog)|(wpl)){SP}+>?{SP}*/[^ >] { BEGIN(WRITE_PROLOG_FILE); }
^{SP}*(((wr(i(te?)?)?{SP}+)?att)|(watt)){SP}* { iface_write_att(NULL); }
^{SP}*(((wr(i(te?)?)?{SP}+)?att)|(watt)){SP}+>?{SP}*/[^ >] { BEGIN(WRITE_ATT_FILE); }
^{SP}*ze(r(o(\-(p(l(us?)?)?)?)?)?)?({SP}+net?)? { iface_zero_plus(); }

<APPLY_FILE_IN>{NONL}+/[>] {
   tempstr = trim(strdup(interfacetext));
   BEGIN(APPLY_FILE_EATUP);
}

<APPLY_FILE_IN>[^>]+ {
   iface_apply_file(trim(interfacetext),NULL, applydir);
   BEGIN(INITIAL);
}
<APPLY_FILE_EATUP>[>][ ]* {
  BEGIN(APPLY_FILE_OUT);
}
<APPLY_FILE_OUT>{NONL}+ {
  iface_apply_file(tempstr, trim(interfacetext), applydir);
  xxfree(tempstr);
  BEGIN(INITIAL);
}

<APPLY_DOWN>{NONL}+ {
   iface_apply_down(interfacetext);
   BEGIN(INITIAL);
}
<APPLY_DOWN>{NONL}+ {
   iface_apply_down(interfacetext);
   BEGIN(INITIAL);
}
<APPLY_MED>{NONL}+ {
   iface_apply_med(interfacetext);
   BEGIN(INITIAL);
}
<APPLY_UP>{NONL}+ {
   iface_apply_up(interfacetext);
   BEGIN(INITIAL);
}
<NAME_NET>{NONL}+ {
  iface_name_net(trim(interfacetext));
  BEGIN(INITIAL);
}

<HELP>{NONL}+ {
  iface_help_search(trim(interfacetext));
  BEGIN(INITIAL);
}
<APROPOS>{NONL}+ {
  iface_apropos(trim(interfacetext));
  BEGIN(INITIAL);
}

<PUSH>{NONL}+ {
  if (find_defined(interfacetext) == NULL)
    printf("'%s' is not a defined symbol.\n",interfacetext);
  else
    stack_add(fsm_copy(find_defined(interfacetext)));
  BEGIN(INITIAL);
}

<APPLY_P>{NONL}+ {
  if (strcmp(interfacetext,"END;") == 0) {
    promptmode = PROMPT_MAIN;
    BEGIN(INITIAL);
  } else {
     if (apply_direction == AP_D) {

       result = apply_down(ah, interfacetext);
       if (result == NULL) {
         printf("???\n");
       } else {
          printf("%s\n",result);
       }
       for (i = g_list_limit; i > 0; i--) {
	 result = apply_down(ah, NULL);
         if (result == NULL)
           break;
         printf("%s\n",result);
       }
     }

     if (apply_direction == AP_M)
       apply_med(stack_find_top()->fsm, interfacetext);

     if (apply_direction == AP_U) {

       result = apply_up(ah, interfacetext);
       if (result == NULL) {
         printf("???\n");
       } else {
          printf("%s\n",result);
       }
       for (i = g_list_limit; i > 0; i--) {
	 result = apply_up(ah, NULL);
         if (result == NULL)
           break;
         printf("%s\n",result);
       }

     }
  }
}


<REGEX>(#|!) { yymore(); BEGIN(RCOMMENT); }

(#|!).* { }

<REGEX>(\.#) { yymore();}

<REGEX>({INREGEX}|%{ANY}) {
  yymore();
}

<RCOMMENT>[\012] { yymore(); BEGIN(REGEX); }
<RCOMMENT>{ANY}  { yymore(); }

<REGEX>(;) {
    if (my_yyparse(interfacetext, interfacelineno) == 0) {
      /* regex xxx line */
      if (pmode == RE) {
        stack_add(fsm_topsort(fsm_minimize(current_parse)));
      /* define XXX xxx line */
      } else if (pmode == DE) {
        tempnet = fsm_topsort(fsm_minimize(current_parse));
        olddef = add_defined(tempnet,tempstr);
        if (olddef) {
          printf("redefined %s: ",tempstr);
        } else {
          printf("defined %s: ",tempstr);
        }
        print_stats(tempnet);
        xxfree(tempstr);
      }
    }
    BEGIN(INITIAL);
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
<REGEX>(["]) {
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

<DEFI>[^ \t(]+/[\050] {
   func_name = xxmalloc(sizeof(char)*(strlen(interfacetext)+2));
   func_name = strcpy(func_name, interfacetext);
   strcat(func_name, "(");
   func_arg_no = 1;
   BEGIN(FUNC_1);
}

 /* Eat up parenthesis */
<FUNC_1>\( { BEGIN(FUNC_2); }

<FUNC_2>[ ] { }
<FUNC_2>[^ ,)]+/[ ]*, {
  add_func_arg(interfacetext);
}
<FUNC_2>[^,) ]+/[ ]*\) {
  add_func_arg(interfacetext);
  BEGIN(FUNC_3);
}

<FUNC_3>\)[ \t]*+             { funcdef[0] = '\0'; BEGIN(FUNC_4); }
<FUNC_4>{NONRESERVED}+ { strcat(funcdef, rep_func_arg(interfacetext)); }
<FUNC_4>{BRACED}       { strcat(funcdef, interfacetext); }
<FUNC_4>%{ANY}         { strcat(funcdef, interfacetext); }
<FUNC_4>{QUOTED}       { strcat(funcdef, interfacetext); }
<FUNC_4>;              { strcat(funcdef, interfacetext); add_defined_function(func_name, funcdef, (func_arg_no-1)); clear_func_args(); BEGIN(INITIAL); }
<FUNC_4>.|\.#\.        { strcat(funcdef, interfacetext); }


<DEFI>([\001-\010]|[\013-\037]|[\041-\047]|[\051-\072]|[\074-\177]|[\300-\337][\200-\277]|[\340-\357][\200-\277][\200-\277]|[\360-\367][\200-\277][\200-\277][\200-\277])+/[ \t]+ {
  tempstr = xxstrdup(interfacetext);
  BEGIN(REGEX);
}

<DEFI>(([\001-\010]|[\013-\037]|[\041-\047]|[\051-\072]|[\074-\177]|[\300-\337][\200-\277]|[\340-\357][\200-\277][\200-\277]|[\360-\367][\200-\277][\200-\277][\200-\277])+)[ \t]+?;? {

   tempnet = NULL;
   /* Define the top network on stack */
   if (iface_stack_check(1)) {
       tempnet = stack_pop();
       olddef = add_defined(tempnet,remove_trailing(interfacetext,';'));
       if (olddef) {
         printf("redefined %s: ",interfacetext);
       } else {
          printf("defined %s: ",interfacetext);
       }
       print_stats(tempnet);
     }
     BEGIN(INITIAL);
}

<UNDEFINE>{NONL}+ {
  remove_defined(remove_trailing(interfacetext,';'));
  BEGIN(INITIAL);
}

<PRINT_NET_FILE>{NONL}+ {
  iface_print_net(NULL,trim(interfacetext));
  BEGIN(INITIAL);
}
<PRINT_NET_NAME>{NONL}+ {
  iface_print_net(trim(interfacetext), NULL);
  BEGIN(INITIAL);
}
<PRINT_NET_NAME_FILE>[^ \t]+ {
  tempstr = strdup(trim(interfacetext));
  BEGIN(PRINT_NET_NAME_FILE2);
}
<PRINT_NET_NAME_FILE2>[> \t] { }
<PRINT_NET_NAME_FILE2>[^ \t>]+ { 
  iface_print_net(tempstr, trim(interfacetext));  
  BEGIN(INITIAL);
}
<WRITE_ATT_FILE>{NONL}+ {
  iface_write_att(trim(interfacetext));
  BEGIN(INITIAL);
}

<WRITE_PROLOG_FILE>{NONL}+ {
  iface_write_prolog(trim(interfacetext));
  BEGIN(INITIAL);
}

<PRINT_DOT_FILE>{NONL}+ {
  iface_print_dot(trim(interfacetext));
  BEGIN(INITIAL);
}

<SOURCE>{NONL}+ {
  if ((yyin = fopen(trim(interfacetext), "r" )) != NULL) {
    printf("Opening file '%s'.\n", trim(interfacetext));    
    input_is_file = 1;
    yypush_buffer_state(yy_create_buffer(yyin, YY_BUF_SIZE*2));
  } else {
    printf("Error opening file '%s'\n",trim(interfacetext));
  }
  BEGIN(INITIAL); 
}

<SHOW_VAR>{NONL}+ {
  iface_show_variable(interfacetext);
  BEGIN(INITIAL);
}
<SET_VAR>[^ ]+ {
  tempstr = xxstrdup(interfacetext);
  BEGIN(SET_VALUE);
}
<SET_VALUE>[^ \n]+ {
  iface_set_variable(tempstr,interfacetext);
  xxfree(tempstr);
  BEGIN(INITIAL);
}
<*>[ ] { }

<READ_SPACED_TEXT>{NONL}+ {
  iface_read_spaced_text(trim(interfacetext));
  BEGIN(INITIAL);
}

<READ_TEXT>{NONL}+ {
  iface_read_text(trim(interfacetext));
  BEGIN(INITIAL);
}

<SAVE_DEFINED>{NONL}+ {
  iface_save_defined(trim(interfacetext));
  BEGIN(INITIAL);
}
<SAVE_STACK>{NONL}+ {
  iface_save_stack(trim(interfacetext));
  BEGIN(INITIAL);
}
<LOAD_STACK>{NONL}+ {
  iface_load_stack(trim(interfacetext));
  BEGIN(INITIAL);
}
<LOAD_DEFINED>{NONL}+ {
  iface_load_defined(trim(interfacetext));
  BEGIN(INITIAL);
}
<SUBSTITUTE_SYMBOL>[^ \t]+ { tempstr = xxstrdup(interfacetext); BEGIN(SUBSTITUTE_SYMBOL_2);}
<SUBSTITUTE_SYMBOL_2>for{SP}+ {BEGIN(SUBSTITUTE_SYMBOL_3);}
<SUBSTITUTE_SYMBOL_3>{NONL}+ {
  iface_substitute_symbol(interfacetext, tempstr); 
  BEGIN(INITIAL);
}
<SUBSTITUTE_DEFINED>[^ \t]+ { tempstr = xxstrdup(interfacetext); BEGIN(SUBSTITUTE_DEFINED_2);}
<SUBSTITUTE_DEFINED_2>for{SP}+ {BEGIN(SUBSTITUTE_DEFINED_3);}
<SUBSTITUTE_DEFINED_3>{NONL}+ {
  iface_substitute_defined(interfacetext, tempstr); 
  BEGIN(INITIAL);
}

<ELIMINATE_FLAG>{NONL}+ {
  iface_eliminate_flag(interfacetext);
  BEGIN(INITIAL);
}

<SYSTEM>{NONL}+ {
  int ret;
  ret = system(interfacetext);
  BEGIN(INITIAL);
}

<ECHO>{NONL}+ {
  printf("%s\n",interfacetext);
  BEGIN(INITIAL);
}

<ASSERT_STACK>{NONL} {
    int level = strtoul(interfacetext, 0, 10);
    if (level != stack_size()) {
      fprintf(stderr, "Stack size %d not %d\n", stack_size(), level);
      exit(1);
    }
    BEGIN(INITIAL);
}

<ATT>{NONL}+ {
  iface_read_att(trim(interfacetext));
  BEGIN(INITIAL);
}

<RPL>{NONL}+ {
  iface_read_prolog(trim(interfacetext));
  BEGIN(INITIAL);
}

<RLEXC>{NONL}+ {
  if ((lexcfilein = file_to_mem(trim(interfacetext))) != NULL) {
     stack_add(fsm_lexc_parse_string(lexcfilein));
     xxfree(lexcfilein); 
  } else {
    printf("Error opening file '%s'.\n", interfacetext);
  }
  BEGIN(INITIAL);
}

<EXCMATRIX>{NONL}+ {
   iface_print_cmatrix_att(trim(interfacetext));
   BEGIN(INITIAL);
}

<RCMATRIX>{NONL}+ {
  if (iface_stack_check(1)) {
    if ((cmatrixfilein = file_to_mem(trim(interfacetext))) != NULL) {
       printf("Reading confusion matrix from file '%s'\n",interfacetext);
       my_cmatrixparse(stack_find_top()->fsm, cmatrixfilein);
       xxfree(cmatrixfilein);
    } else {
      perror("File error");
    }
  }
  BEGIN(INITIAL);
}

([\040]|[\011]|[\015]) { }

<*>[\012] { }
<*>[\015] { }

<INITIAL>[^#!] {
      if (!input_is_file)
        printf("Unknown command. Ignoring until end of line.\n");
      else 
        printf("***Unknown command '%s' on line %i. Aborting.\n",interfacetext,interfacelineno);
      return 1;
}

<IGNORELINE>.? { printf("SS\n");BEGIN(INITIAL);  }


<<EOF>> {
  yypop_buffer_state();
  if (!YY_CURRENT_BUFFER) {
     yyterminate();
  }
}
