# Basic API usage #

## Headers, compiling ##

The headers for the API need to be included in your code:

```
#include "fomalib.h"
```

When compiling, link with `-lfoma` (dynamic). For example

```
gcc -o myfomatest myfomatest.c -lfoma
```

You can also use the static library `libfoma.a`. When linking statically, you may need to link with `-lz` as well (or its static version libz.a). For example:

```
gcc -o myfomatest myfomatest.c /usr/lib/libfoma.a -lz
```

## Data structures ##

The basic data structure for FSTs is `struct fsm *`.  All functions that manipulate FSTs use this for its arguments.  Also, all functions, except `fsm_copy(struct fsm *net);` are destructive to the arguments.  That is, if you do, for example:

```
newnet = fsm_union(net1, net2);
```

and you later need the arguments `net1` and `net2`, you need to copy them since `fsm_union()` destroys them and frees the memory associated with them, i.e.:

```
newnet = fsm_union(fsm_copy(net1), fsm_copy(net2));
```

To clear a FSM structure and free associated memory, call:

```
int fsm_destroy(struct fsm *net);
```

## Loading/saving FSTs ##

There are two functions for loading FSTs in the binary format (which `foma` uses with `load/save stack`). The simplest one is:

```
struct fsm *fsm_read_binary_file(char *filename);
```

which reads one FST from a file. The example below illustrates the usage.

```
#include <stdio.h>
#include "fomalib.h"

int main() {

  struct fsm *net;
  struct apply_handle *ah;
  char *result;

  net = fsm_read_binary_file("myfst.foma");
  ah = apply_init(net);
  result = apply_down(ah, "inputword");
  while (result != NULL) {
    printf("%s\n", result);
    result = apply_down(ah, NULL);
  }
}
```

Note that there's no error checking in the above: `fsm_read_binary_file()` and `apply_init()` both return `NULL` if there's an error.

If you've stored several FSTs in one file (which is possible with the `foma` interface's `save stack` command, you can retrieve all of them with the functions:

```
fsm_read_binary_handle fsm_read_binary_file_multiple_init(char *filename);
struct fsm *fsm_read_binary_file_multiple(fsm_read_binary_handle fsrh);
```

In this case, one needs to iterate:

```
struct fsm *net;
fsm_read_binary_handle fsrh;

if ((fsrh = fsm_read_binary_file_multiple_init(infilename)) == NULL) {
    perror("File error");
    exit(EXIT_FAILURE);
}
while ((net = fsm_read_binary_file_multiple(fsrh)) != NULL) {
  /* Do something with each net */
}

```

Symmetrically, there is a function

```
int fsm_write_binary_file(struct fsm *net, char *filename);
```

to write a FST to a file. The function returns 0 on success:

```
...

if ((fsm_write_binary_file(net, "outfile.foma")) != 0) {
    perror("File error");
}

```


## Passing strings through FSTs ##

To pass words through a FST, first call `apply_init()` and save the result in a handle.

```
struct fsm *mynet;
struct apply_handle *ah;

ah = apply_init(mynet);
```

This initializes the necessary data structures, and words can then be passed through the transducer with either:

```
char *apply_down(struct apply_handle *h, char *word);
char *apply_up(struct apply_handle *h, char *word);
```

These functions return `NULL` immediately if the word is not accepted by the automaton or transducer. Otherwise they return the output.

If a transducer, for example, returns multiple outputs for one input, repeated calls to `apply_up/down()` with `*word` set to `NULL` retrieves all possibilities. That is, the first call should be done with `*word` set to the input word, and subsequent calls with `NULL`:

```
    char *result = apply_down(ah, "inputword");
    while (result != NULL) {
        /* Do something with result */
        printf("%s\n", result);
        /* Get further results */
        result = apply_down(ah, NULL);
    }
```

The `char *` results which are returned by `apply_up()` and `apply_down()` are guaranteed to persist at least until the next call to these functions with the same handle. If you need to hang on to a string past subsequent calls, you need to make a copy.

After no more applications are needed for a given FST, use

```
void apply_clear(struct apply_handle *h);
```

to clear the memory in the data structures with apply.  Note that

```
int fsm_destroy(struct fsm *net);
```

does not do this. Once you're done with applying and with a FST altogether you should therefore call:

```
struct fsm *net;
struct apply_handle *ah;

...

apply_clear(ah);
fsm_destroy(net);
```

### API access to minimum edit distance functions ###

The equivalent of the interface command `apply med` is also available through the API.  Using this functionality requires separate initialization.  The following example code illustrates the initialization of the minimum-edit distance functionality, setting some limit parameters, and finding the closest approximate match to a word.


```
    char *mystring, *result;
    struct apply_med_handle *medh;
    struct fsm *net;

    medh = apply_med_init(net);
                                                 
                                             /* Set some optional parameters:             */
    apply_med_set_heap_max(medh,4194304);    /* Don't grow heap more than this            */
    apply_med_set_med_limit(medh, 10);       /* Don't search for matches with cost > 10   */
    apply_med_set_med_cutoff(medh, 5);       /* Don't return more than 5 matches          */

    while (result = apply_med(medh, mystring)) {
        mystring = NULL; /* Call with NULL on subsequent calls to iterate */
        printf("%s", result);
    }
```

The above example code only retrieves the raw best matches for the input string provided. If we want to see the "alignment" of the input string and the candidate matches, as well as find the costs, we can do this like so:

```
    apply_med_set_align_symbol(medh, "-"); /* Using - to represent inserts and deletes */

    while (result = apply_med(medh, mystring)) {
        mystring = NULL;
        printf("%s\n%s\nCost:%i\n\n", result, apply_med_get_instring(medh), apply_med_get_cost(medh));
    }
```

Here, we use the fact that `apply_med_get_instring()` retrieves a padded version of the original string, and apply\_med\_get\_cost the MED cost associated with the search in question.  The results from `apply_med_get_instring()` and `apply_med_get_outstring()` are guaranteed to be unchanged only until the next call to `apply_med()` with the same handle.

The code above could print out something like (for an input word `cag`):

```
cat
cag
Cost: 1

cat-
ca-g
Cost: 2
```

Note that in contrast to the general `apply` functions, `apply_med()` requires that:

  * The input FSM be an automaton, not a transducer. Flag diacritics are also not supported (i.e. ignored).
  * The input FSM only contain single-symbol utf8 symbols in its alphabet (i.e. no multichar symbols such as `+NOUN`, etc).

## Compiling regular expressions ##

You can access the regular expression compiler with:

```
struct fsm *fsm_parse_regex(char *regex);
```

which returns an FST with the minimized and determinized FST resulting from compiling the regular expression.

The following short example program compiles the regex `{cat}|{dog}|{house}` and prints out all the words in the resulting FSM. It also illustrates the use of `apply_lower_words` (and `apply_upper_words`) to print out all the words accepted by an automaton:

```
#include <stdio.h>
#include "fomalib.h"

int main() {
  struct fsm *net;
  struct apply_handle *ah;
  char *result;

  net = fsm_parse_regex("{cat}|{dog}|{house}");
  ah = apply_init(net);
  while ((result = apply_lower_words(ah)) != NULL) {
    printf("%s\n",result);
  }
  apply_clear(ah);
  fsm_destroy(net);
}
```

Note that while `fsm_parse_regex()` minimizes its FSTs, it does not topologically sort them, and hence does not count the number of paths (as the `foma` interface compiler does). If you need this, do:

```
mynet = fsm_topsort(fsm_parse_regex("myregex"));
```

## Accessing FST states and transitions ##

You can access an FSTs individual states and transitions with the following iterator functions:

```
int fsm_get_next_arc(struct fsm_read_handle *handle);
int fsm_get_arc_source(struct fsm_read_handle *handle);
int fsm_get_arc_target(struct fsm_read_handle *handle);
char *fsm_get_arc_in(struct fsm_read_handle *handle);
char *fsm_get_arc_out(struct fsm_read_handle *handle);
int fsm_get_arc_num_in(struct fsm_read_handle *handle);
int fsm_get_arc_num_out(struct fsm_read_handle *handle);
int fsm_get_next_initial(struct fsm_read_handle *handle);
int fsm_get_next_final(struct fsm_read_handle *handle);
int fsm_get_next_state(struct fsm_read_handle *handle);
```

Before calling these functions, you need to call:

```
struct fsm_read_handle *fsm_read_init(struct fsm *net);
```

which returns a handle that is used when retrieving states and arcs. The function:

```
void fsm_read_done(struct fsm_read_handle *handle);
```

clears associated data structures.

To illustrate usage, the following is an example program that reads a regular expression from the command line argument and prints out the resulting transducer in an `AT&T`-like tabular format. It first compiles the regular expression, then iterates over transitions in the resulting FST and enumerates them, and then iterates over the final states, which are listed separately.

```
#include <stdio.h>
#include <stdlib.h>
#include "fomalib.h"

int main (int argc, char *argv[]) {
    struct fsm *net;
    struct fsm_read_handle *h;
    int final;
    
    if (argc < 2) {
      printf("Error: expected regular expression as argument.\n");
      exit(1);
    } 

    net = fsm_topsort(fsm_minimize(fsm_parse_regex(argv[1])));
    if (net == NULL) {
      exit(1);
    }
    h = fsm_read_init(net);

    while (fsm_get_next_arc(h)) {
        printf("%i\t%i\t%s\t%s\n",fsm_get_arc_source(h), fsm_get_arc_target(h),
                                  fsm_get_arc_in(h), fsm_get_arc_out(h));
    }
    while ((final = fsm_get_next_final(h)) != -1) {
        printf("%i\n", final);
    }    
    fsm_read_done(h);
    fsm_destroy(net);
}
```

Compile with:

```
gcc -o regex2att regex2att.c -lfoma
```

And run:

```
mylinux$ ./regex2att "[c a t | d o g]*"
0	3	c	c
0	1	d	d
1	2	o	o
2	0	g	g
3	4	a	a
4	0	t	t
0
```


## Constructing FSTs by listing states and transitions ##

Similarly to reading FST details, they can be constructed by first calling:

```
struct fsm_construct_handle *fsm_construct_init(char *name);
```

and then calling the various functions that add arcs and states and marks them final and initial. After this,

```
struct fsm *fsm_construct_done(struct fsm_construct_handle *handle);
```

returns the resulting FST

The following functions are available for construction:

```
void fsm_construct_set_final(struct fsm_construct_handle *handle, int state_no);
void fsm_construct_set_initial(struct fsm_construct_handle *handle, int state_no);
/* Add transition by strings */
void fsm_construct_add_arc(struct fsm_construct_handle *handle, int source, int target, char *in, char *out);
/* Add transition by numbers */
void fsm_construct_add_arc_nums(struct fsm_construct_handle *handle, int source, int target, int in, int out);
/* Add a symbol to the alphabet, returns number */
int fsm_construct_add_symbol(struct fsm_construct_handle *handle, char *symbol);
/*
/* Check if symbol exists in alphabet */
int fsm_construct_check_symbol(struct fsm_construct_handle *handle, char *symbol);
/* Copy the alphabet of an existing FST */
void fsm_construct_copy_sigma(struct fsm_construct_handle *handle, struct sigma *sigma);
```

Here's an example program that constructs a FST arc-by-arc and state-by-state, and then prints it out in AT&T format:

```
#include <stdio.h>
#include "fomalib.h"

int main() {
  struct fsm *net;
  struct fsm_construct_handle *h;

  h = fsm_construct_init("mynetname");
  /* Mark 0 and 1 final */
  fsm_construct_set_final(h,0);
  fsm_construct_set_final(h,1);
  /* Mark 0 initial */
  fsm_construct_set_initial(h,0);
  /* Add some transitions */
  fsm_construct_add_arc(h,0,1,"a","@_EPSILON_SYMBOL_@");
  fsm_construct_add_arc(h,0,1,"c","c");
  fsm_construct_add_arc(h,1,1,"@_UNKNOWN_SYMBOL_@","d");
  /* Done */
  net = fsm_construct_done(h);

  /* Print it out */
  net_print_att(net, stdout);
  fsm_destroy(net);
}
```

## Accessing low-level construction functions ##

The low-level construction function provide fine-grained control over the construction of automata and transducers.  See the [API reference](APIReference.md) for details.  Here's a brief example that calculates the same transducer as the regex `[a|b]* c`:

```

net = fsm_concat(fsm_kleene_star(fsm_union(fsm_symbol("a"),fsm_symbol("b"))), fsm_symbol("c"));
```

The returned net in this case is neither determinized nor minimized as `fsm_regex("[a|b]* c");` would do.  If you want this, you need to do something like:

```
net = fsm_minimize(net);
```

Here, there is no need to call `fsm_determinize()` first since minimization calls it automatically if it discovers that the input FSM is non-deterministic.  Likewise, some of the functions that require determinized inputs (e.g. FSM complementation) will automatically determinize the input arguments if need be.