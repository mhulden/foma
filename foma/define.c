#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "foma.h"

struct defined   *defines;
struct definedf  *defines_f;

/** Find a defined symbol from the symbol table */
struct fsm *find_defined (char *string) {
  struct defined *defined;
  for (defined = defines; defined != NULL; defined = defined->next) {
    if (strcmp(string, defined->name) == 0) {
      return(defined->net);
    }
  }
  return NULL;
}

struct defined *get_defines() {
    return(defines);    
}
struct definedf *get_defines_f() {
    return(defines_f);    
}

/* Removes a defined function from the list */
/* Returns 0 on success, 1 if the definition did not exist */

int remove_defined (char *string) {
  struct defined *defined, *defined_prev;
  int exists = 0;
  defined_prev = NULL;
  /* Undefine all */
  if (string == NULL) {
      for (defined = defines; defined != NULL; ) {
          fsm_destroy(defined->net);
          defined_prev = defined;
          defined = defined->next;
	  xxfree(defined_prev->name);
          xxfree(defined_prev);
          defines = NULL;
      }
      return(0);
  }
  for (defined = defines; defined != NULL; defined = defined->next) {
      if (strcmp(defined->name, string) == 0) {
          exists = 1;
          break;
      }
      defined_prev = defined;
  }
  if (exists == 0) { 
      //if (mode == 1) printf("*** Error: %s has no network definition.\n", string);
      return 1;
  }
  if (defined_prev != NULL) {
      defined_prev->next = defined->next;
  } else {
      defines = defined->next;
  }
  fsm_destroy(defined->net);
  xxfree(defined->name);
  xxfree(defined);
  return(0);
}

/* Add a function to list of defined functions */
char *find_defined_function(char *name, int numargs) {
    struct definedf *df;
    for (df = defines_f ; df != NULL; df = df->next) {
        if (strcmp(df->name, name) == 0 && df->numargs == numargs) {
            return(df->regex);
        }
    }
    return(NULL);
}

int add_defined_function(char *name, char *regex, int numargs) {

    struct definedf *df;
    if (find_defined_function(name, numargs) == NULL) {
        df = xxmalloc(sizeof(struct definedf));
        df->name = xxstrdup(name);
        df->regex = xxstrdup(regex);
        df->numargs = numargs;
        df->next = defines_f;
        defines_f = df;
        printf("defined %s@%i)\n", name, numargs);
    } else {
        for (df = defines_f; df != NULL; df = df->next) {
            if (strcmp(df->name, name) == 0 && df->numargs == numargs) {
                df->regex = xxstrdup(regex);
                printf("redefined %s@%i)\n", name, numargs);
                break;
            }
        }
    }
    return 1;
}

/* Add a network to list of defined networks */
/* Returns 0 on success or 1 on redefinition */

int add_defined (struct fsm *net, char *string) {
  struct defined *defined, *defined_prev;
  int redefine;

  redefine = 0;
  if (net == NULL) { return 0; }
  fsm_count(net);
  if (defines == NULL) {
    defined = xxmalloc(sizeof(struct defined));
    defines = defined;
    defined->next = NULL;
  } else {
    for (defined = defines; defined != NULL; defined = defined->next) {
      defined_prev = defined;
      if (strcmp(defined->name, string) == 0) {
	redefine = 1;
	break;
      }
    }
    if (redefine == 0) {
      defined_prev->next = xxmalloc(sizeof(struct defined));
      defined = defined_prev->next;
      defined->next = NULL;
    }
  }
  if (redefine) {
      fsm_destroy(defined->net);
      xxfree(defined->name);
  }
  defined->name = xxstrdup(string);
  defined->net = net;
  return(redefine);
}
