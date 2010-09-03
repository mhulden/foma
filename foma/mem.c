#include "foma.h"
#include <stdlib.h>
#include <string.h>

/* Global variables */
int g_show_flags = 0;
int g_obey_flags = 1;
int g_flag_is_epsilon = 0;
int g_print_space = 0;
int g_print_pairs = 0;
int g_minimal = 1;
int g_name_nets = 0;
int g_print_sigma = 1;
int g_quit_on_fail = 1;
int g_quote_special = 0;
int g_recursive_define = 0;
int g_sort_arcs = 1;
int g_verbose = 1;
int g_minimize_hopcroft = 1;
int g_compose_tristate = 0;
int g_list_limit = 100;
int g_list_random_limit = 15;
int g_med_limit  = 3;
int g_med_cutoff = 15;
char *g_att_epsilon = "@0@";


char *xxstrndup(const char *s, size_t n) {
    char *r = NULL;
    const char *p = s;
    while(*p++ && n--);
    n = p - s - 1;
    r = (char *) xxmalloc(n + 1);
    if(r != NULL) {
        memcpy(r, s, n);
        r[n] = 0;
    }
    return r;
}

int next_power_of_two(int v) {
    int i;
    for (i=0; v > 0; i++)
        v = v >> 1;
    return (1 << i);
}

inline void *xxmalloc_atomic(size_t size) {
#ifdef USE_GC
    return GC_malloc_atomic(size);
#endif /* USE_GC */

#ifndef USE_GC
    return(malloc(size));
#endif /* USE_GC */
}

inline void *xxmalloc(size_t size) {
#ifdef USE_GC
    return GC_malloc(size);
#endif /* USE_GC */

#ifndef USE_GC
    return(malloc(size));
#endif /* USE_GC */
}

inline void xxfree(void *ptr) {
#ifdef USE_GC
    GC_free(ptr);
#endif /* USE_GC */

#ifndef USE_GC
    free(ptr);
#endif /* USE_GC */    
}

void *xxrealloc(void *ptr, size_t size) {
#ifdef USE_GC
    size_t old_size = GC_size(ptr);
    void *result; 
    if (size <= 10000)
        return(GC_realloc(ptr, size));
    if (size <= old_size)
        return(ptr);
    result = GC_malloc_ignore_off_page(size);
    if (result == 0)
        return(0);
    memcpy(result,ptr,old_size);
    GC_free(ptr);
    return(result);
    
#endif /* USE_GC */

#ifndef USE_GC
    return(realloc(ptr, size));
#endif /* USE_GC */    
}

inline void *xxcalloc(size_t nmemb, size_t size) {
#ifdef USE_GC
    return GC_malloc(size*nmemb);
#endif /* USE_GC */

#ifndef USE_GC
    return(calloc(nmemb,size));
#endif /* USE_GC */
}

inline char *xxstrdup(const char *s) {
    char *string;
#ifndef USE_GC
    return(strdup(s));
#endif /* USE_GC */
    string = xxmalloc((strlen(s)+1)*sizeof(char));
    strcpy(string, s);
    return(string);
}

void garbage_cleanup(void) {
#ifdef USE_GC    
    GC_dont_gc = 0; 
    GC_gcollect();
    GC_dont_gc = 0;
#endif /* USE_GC */
}

