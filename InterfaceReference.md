# Foma interface reference #

# System & Help commands #

## apropos `<`string`>` ##

Search on-line help for `<`string`>`.

## echo `<`string`>` ##

Echo a string.

## help license ##

Prints license.

## help warranty ##

Prints warranty information.

## system `<`cmd`>` ##

Execute a system command.

## quit ##

Exit foma.


# Regular expression commands #

## define `<`name`>` `<`r.e.`>` ##

Label the network compiled from a regular expression.  The command `define `<`name`>`` with no regular expression given will pop the top network off the stack and label that.

## define `<`fname`>`(`<`v1,..,vn`>`) ##

Define a function of name `<`fname`>` with prototype (`<`v1,..,vn`>`).  For example

```
define PerfectShuffle(X,Y) [X .o. [? 0:?]*].l & [Y .o. [0:? ?]*].l;
```

Defines the function PerfectShuffle() which takes two arbitrary regular languages as input and yields their perfect shuffle. So one can now say things like:

```
regex PerfectShuffle(a b c, d e f);
```

which would produce a FSM that contains only the word `adbecf`.

## regex `<`regex`>` ##

Read and compile a regular expression.

## name net `<`string`>` ##

Names top FSM.

## push (defined) `<`name`>` ##

Adds a defined FSM to top of stack.  This is equivalent to `regex `<`name`>`;`

## undefine `<`name`>` ##

Remove `<`name`>` from defined networks and free memory associated with it.

# Application commands #

## apply up / up ##

Enter apply up mode, after which words entered are applied against the network on the top of the stack in inverse direction.  `CTRL-D` exits the mode.

## apply up `<`string`>` / up `<`string`>` ##

Apply a single string in the inverse direction to the top network on stack.

## apply down / down ##

Enter apply down mode, after which words entered are applied against the network on the top of the stack.  `CTRL-D` exits the mode.

## apply down `<`string`>` / down `<`string`>` ##

Apply a single string to the top network on stack.

## apply med / med ##

Enter apply med mode, after which words entered are matched against the closest words in the network on top of the stack.  Apply med is currently only intended for automata and not transducers.  The minimum edit distance cost is by default 1 for all substitution/insertion/deletion operations (Levenshtein distance).  The `load cmatrix` command can be used to specify and associate different costs with different operations. `CTRL-D` exits the mode. This is an implementation of the algorithm described in [Hulden (2009b)](#References.md).

A confusion matrix can be specified separately (see `read cmatrix` below).

## apply med `<`string`>` / med ##

Applies med search to a single string.


# Tests on FSM properties #

All FSM properties tests return either 0 or 1, depending on whether the test returns TRUE or FALSE.


## test equivalent / equ ##

Test if the top two FSMs are equivalent.  Transducers are treated like FSA consisting of symbol pairs and the test for these may yield false negatives (but not false positives).

There does not exist a general algorithm for testing equivalence of (non-functional) transducers.  For functional transducers 'S' and 'T', however, equivalence can be tested by first testing if the domains are equivalent (`S.u` and `T.u`), and subsequently testing if `T.i .o. S` is an identity relation (`test identity`).  S and T are equivalent iff both tests are true. For example:

```
foma[0]: define S a:0 0:b;
defined S: 279 bytes. 3 states, 2 arcs, 1 path.
foma[0]: define T a:b;
defined T: 220 bytes. 2 states, 1 arcs, 1 path.
foma[0]: regex T.i .o. S;
279 bytes. 3 states, 2 arcs, 1 path.
foma[1]: test identity
1 (1 = TRUE, 0 = FALSE)
foma[1]: regex S.u;
237 bytes. 2 states, 1 arcs, 1 path.
foma[2]: regex T.u;
194 bytes. 2 states, 1 arcs, 1 path.
foma[3]: equ
1 (1 = TRUE, 0 = FALSE)
```

Hence, the transducers are equivalent.

## test functional / tfu ##

Test if the top FST is functional (single-valued), i.e. is a function.  That is, whether for each input string in the domain of the transducer yields precisely one output string.  The algorithm is documented in [Hulden(2009a)](#References.md).

## test identity / tid ##

Test if top FST represents identity relations only, i.e. where every string in the input is always mapped to itself.  Note that the mapping need not be synchronous, so for example `a:0 0:a` is an identity transducer.

## test lower-universal / tlu ##

Test if the output side (range) if the FSM is Σ`*`.

## test upper-universal / tuu ##

Test if the input side (domain) if the FSM is Σ`*`.

## test non-null / tnn ##

Test if top machine is not the empty language (∅).

## test null / tnu ##

Test if top machine is the empty language (∅).

## test sequential / tseq ##

Tests if the top FSM is sequential, i.e. if (1) for every state there is only one arc outgoing for a given input symbol, and (2) 0:x transitions never co-occur with other non-epsilon-input transitions in the same state. Hence, `a:b|a:c` is not sequential, but `0:a 0:b` is, while `0:a | b` again is not.


## test star-free / tsf ##

Test if top FSM is star-free, i.e. expressible using a regular expression that only uses the operators union, concatenation, and negation (complement).

## test unambiguous / tunam ##

Test if top FST is unambiguous, i.e. if there are two different successful paths with the same strings on the input side.  See also 'test functional'.


# I/O commands #


## export cmatrix `<`filename`>` ##

Export the confusion matrix as a (weighted) AT&T transducer.

## load stack `<`filename`>` / load `<`filename`>` ##

Loads networks and pushes them on the stack.

## load defined `<`filename`>` / loadd `<`filename`>` ##

Restores defined networks from file.

## read att `<`filename`>` ##

Read a file in AT&T FSM format and add to top of stack.

## read cmatrix `<`filename`>` ##

Read a confusion matrix and associate it with the network on top of the stack.

The format of the confusion matrix should be clear from the following example confusion matrix file:

```
Insert 1
Substitute 2
Delete 1
Cost 1
a:b c:d
Cost 3
:x x: x:y
```

The above snippet specifies a matrix where the default insertion cost is 1 unit, the default substitution cost is 2 units, and the default deletion cost is 1 unit.  Also, substituting an `a` with a `b` costs 1 unit, as does substituting a `c` for a `d`.  Inserting an `x` costs 3 units, as does deleting an `x` and substituting an `x` for a `y`.

All costs must be positive integer values.  A cost specification that involves symbols not found in the alphabet of the top network are not included in the matrix, but are warned about.


## read prolog `<`filename`>` ##

Reads prolog format file.

## read lexc `<`filename`>` ##

Read and compile lexc format file

## read spaced-text `<`filename`>` ##

Compile space-separated words/word-pairs separated by newlines into a FST

## read text `<`filename`>` ##

Compile a list of words separated by newlines into an automaton.

## save defined `<`filename`>` ##

Save all defined networks to binary file.

## save stack `<`filename`>` ##

Save stack to binary file.

## source `<`file`>` ##

Read and compile script file.

## write prolog (`>` filename) ##

Writes top network on stack to prolog format file, or stdout if no filename is given.

## write att (`>` `<`filename`>`) ##

Writes top network to AT&T format file, or stdout if no filename is given.  The variable `att-epsilon` controls how the _epsilon_-symbol is represented.


# Regular expression operations on stack #

The following operators are equivalent in functionality to already existing [regular expression operations](RegularExpressionReference.md).  However, when issued in the interface they operate on the FSMs already on the stack, replacing the stack with the result.  For example, `union net` will calculate the union of all FSMs on the stack and replace the entire stack with the result.

In general, these commands are far less useful than labeling FSMs using define and subsequently issuing new define or regex commands, but are retained for compatibility reasons.

## compose net ##

Composes all networks on stack, from top to bottom and replaces stack with result.

### concatenate ###
Concatenates networks on stack, from top to bottom and replaces stack with result.

## crossproduct net ##

Calculate the cross-product of top two FSMs on stack, and replaces these with result.

## ignore net ##

Applies ignore to top two FSMs on stack.

## intersect net ##

Intersects all FSMs on stack and replaces stack with result.

## invert net ##

Inverts top FSM on stack.

## lower-side net ##

Takes lower projection of top FSM on stack.

## negate net ##

Complements top FSM on stack.

## one-plus net ##

Calculates the Kleene plus of top FSM on stack.

## reverse net ##

Reverses top FSM on stack.

## shuffle net ##

Calculates the asynchronous (shuffle) product of the top two FSMs on stack and replaces these with result.

## zero-plus net ##

Calculates the Kleene closure of top FSM on stack.


# Miscellaneous operations on FSMs #

## ambiguous upper / ambiguous ##

Returns a FSM containing the input words which have multiple paths in a transducer.  For example a regular expression `a:a | a:b` yields an ambiguous path on the input side with respect to the string `a`.  Ambiguous upper from this FSM would produce another FSM that accepts only `a`.  The built-in function `_ambdom(L)` can be used in regular expressions to produce the same effect.

## compact sigma ##

Removes symbols from a FSM that have exactly the same distribution as @, producing a new FSM with possibly fewer transitions.  For example, the regular expression `a|?` would produce a FSM with two identical transitions, `@` and `a`.  Removing the transition with `a` and removing the symbol `a` from the alphabet will yield, from a functional point of view, an identical machine.

## complete net ##

Completes the FSM on top of the stack.  This implies adding transitions so that there is an outgoing transition from each state on each symbol in the alphabet.  This may also require the addition of a 'dead state', a nonfinal state with transitions to itself on every symbol in the alphabet.

## determinize net / det ##

Determinizes the top FSM on the stack using the subset construction algorithm.  Generally, most FSMs are determinized and minimized by default if they are produced from regular expressions.  If the FSM on the top of the stack is already deterministic, no action is taken.

## eliminate flag `<`name`>` ##

Eliminates flag diacritic symbols from the top FSM on the stack and produces an equivalent FSM that does not rely on the flags of name `<`name`>`.

## eliminate flags ##

Eliminates _all_ flag diacritic symbols from the top FSM on the stack and produces an equivalent FSM that does not rely on flags at all.

## extract ambiguous ##

Extracts the ambiguous paths of a transducer.

## extract unambiguous ##

Extracts the unambiguous paths of a transducer.

## label net ##

Extracts all attested symbol pairs from top FSM.  A new two-state FSM is created that accepts all symbol pairs appearing in the original FSM.

## letter machine ##

Converts top FSM to an equivalent 'letter machine' where no multicharacter symbols exist.  This implies expanding the multichacter symbols in the original FSM and creating new states for each UTF-8 symbol.

## minimize net ##

Minimizes top FSM using a variant of Hopcroft's algorithm (if the variable `hopcroft-min` is set), or Brzozowski's algorithm (otherwise).  Transducers are minimized in the FSA sense, i.e. as if each symbol pair was a single symbol.

## prune net ##

Makes top network on stack 'coaccessible'.  This means only those states _s_ are retained where a) there is a path from _s_ to a final state, and b) there is a path from an initial state to _s_.  Foma automatically returns coaccessible FSMs when compiling them from regular expressions.

## sigma net ##

Extracts the alphabet and creates a two-state FSM that accepts all single symbols in the alphabet of the original FSM.

## sort net ##

Sorts arcs lexicographically on top FSM of stack.

## substitute defined ##

The command `substitute defined X for Y`, substitutes the (previously defined) network X at all arcs containing the symbol Y.  For example:

```
foma[0]: define Words [c a t | d o g];
defined Words: 447 bytes. 6 states, 6 arcs, 2 paths.
foma[0]: regex W W;
253 bytes. 3 states, 2 arcs, 1 path.
foma[1]: substitute defined Words for W
Substituted network 'Words' for 'W'.
569 bytes. 11 states, 12 arcs, 4 paths.
foma[1]: words
dogdog
dogcat
catdog
catcat
```

## substitute symbol ##

The command `substitute symbol X for Y` replaces all Y-transitions with X-transitions.  The regular expression ```[L, Y, X]`` can also be used for the same effect.

## twosided flag-diacritics ##

Changes flag symbols to always exist as identity pairs, otherwise retaining the behavior of the original FSM.  For example, a network such as:

```
Ss0:	`<`a:@U.FEATURE.VALUE@`>` -`>` fs1.
fs1:	(no arcs).
```

would be changed into:

```
Ss0:	`<`a:0`>` -`>` s1.
s1:	@U.FEATURE.VALUE@ -`>` fs2.
fs2:	(no arcs).
```


# Stack commands #

## clear stack / clear ##

Clears the stack.

## pop stack / pop ##

Remove top FSM from stack

## rotate stack ##

Rotates stack, i.e. moves the top FSM from the stack to the bottom.

## turn stack ##

Turns stack upside down.


# Printing commands #

## print cmatrix ##

Prints the confusion matrix associated with the top network in tabular format.

## print defined ##

Prints all defined symbols and functions

## print dot (`>`filename) ##

Prints top FSM in Graphviz dot format to filename, or to stdout if no filename is given.

## print lower-words / lower-words ##

Prints all words on acyclic paths on the output side of top FSM.

## print name ##

prints the name of the top FSM.  Unless FSMs are named through `name net`, a random name is given to each FSM.

## print net / net ##

Prints complete information about top FSM.

## print random-lower / random-lower ##

Prints a set of random words from output side.

## print random-upper / random-upper ##

Prints a set of random words from input side.

## print random-words / random-words ##

Prints a set of random words from top FSM.  If the FSM is a transducer, word-pairs will be printed.

## print sigma /sigma ##

Prints the alphabet of the top FSM.

## print size / size ##

Prints size information about top FSM.

## print shortest-string / pss ##

Prints the shortest string of the top FSM.

## print shortest-string-size / psz ##

Prints length of shortest string of top FSM.

## view net ##

Display a graphical view of top FSM on stack (if supported).



# Interface variables #

Interface variables are controlled by the command

```
set name-of-variable value
```

## show variables ##

Prints all variable/value pairs

## compose-tristate ##

Controls the type of filtering used in the composition algorithm.  A value of 0 implies a bistate composition strategy.  See [Hulden (2009a)](#References.md), or [Mohri, Pereira, and Riley (1996)](#References.md) for the gory details.

  * Possible values: 0/1
  * Default value: 0

## show-flags ##

Controls if flag diacritics are shown in output of apply up/apply down.


  * Possible values: 0/1
  * Default value: 0

## obey-flags ##

Controls whether flag diacritics are obeyed/ignored


  * Possible values: 0/1
  * Default value: 1


## minimal ##

Controls whether FSMs are automatically minimized after determinization.

  * Possible values: 0/1
  * Default value: 1


## print-pairs ##

Controls if both input and output sides are printed in apply up/down operations.


  * Possible values: 0/1
  * Default value: 1


## print-space ##

Controls if spaces are printed between each separate symbol in apply up/down.

  * Possible values: 0/1
  * Default value: 0


## print-sigma ##

Controls if alphabet is printed whenever `print net` is issued.

  * Possible values: 0/1
  * Default value: 1

## quit-on-fail ##

Controls whether to quit immediately when encountering an error loading a script.

  * Possible values: 0/1
  * Default value: 1


## recursive-define ##

Not implemented, dummy _xfst_-compatibility variable.


  * Possible values: 0/1
  * Default value: 0

## verbose ##

Controls verbosity of interface.


  * Possible values: 0/1
  * Default value: 1

## hopcroft-min ##

Controls whether to minimize FSMs using a variant [(Hulden, 2009a)](#References.md) of [Hopcroft's (1971)](#References.md) algorithm, or [Brzozowski's (1963)](#References.md) double-reversal-determinization minimization procedure.

  * Possible values: 0/1
  * Default value: 1


## med-limit ##

Controls the limit on number of matches in `apply med`

  * Possible value: 0-MAXINT
  * Default value: 3


## med-cutoff ##

Controls the cost at which `apply med` search is to terminate.

  * Possible value: 0-MAXINT
  * Default value: 15

## att-epsilon ##

Sets the epsilon-symbol-string to use when reading and writing AT&T format files.


  * Possible value: any string
  * Default value: `@0@`

# References #

Brzozowski, J. A. (1963). Canonical regular expressions and minimal state graphs for definite events. In _Proceedings of the Symposium on Mathematical Theory of Automata_, New York, NY, April 24-26, 1962, volume 12 of MRI Symposia Series, pages 529–561, Brooklyn, NY. Polytechnic Press of the Polytechnic Institute of Brooklyn. [(PDF version)](http://www.diku.dk/hjemmesider/ansatte/henglein/papers/brzozowski1962.pdf)

Hopcroft, J. (1971). An n log n algorithm for minimizing states in a finite automaton. _Technical Report STAN-CS-71-190_, Stanford University. [(PDF version)](ftp://db.stanford.edu/pub/cstr/reports/cs/tr/71/190/CS-TR-71-190.pdf)

Hulden, M. (2009a). _Finite state machine construction methods and algorithms for phonology and morphology_.  Ph.D. Thesis, University of Arizona.

Hulden, M. (2009b). Fast approximate string matching with finite automata.  _Procesamiento de Lenguaje Natural_ 43(1):57-64. [(PDF version)](http://rua.ua.es/dspace/bitstream/10045/11687/1/PLN_43_07.pdf)

Mohri, M., Pereira, F., and Riley, M. (1996). Weighted automata in text and speech processing. In _ECAI 96, 12th European Conference on Artificial Intelligence_. [(PS version)](http://www.cs.nyu.edu/~mohri/pub/ecai.ps)