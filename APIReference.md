# API function reference #

## Destructive functions ##

All API functions that perform operations on `(struct fsm *net)` are destructive, and will free all memory associated with the FSM, with the exception of `*fsm_copy()`.

## `struct fsm *fsm_determinize(struct fsm *net)` ##

Determinizes the FSM using a subset construction.  If `net->is_deterministic` is 1, no action is taken.  Transducers are determinized with respect to their input/output pairs (i.e. in a FSA sense).

## `struct fsm *fsm_epsilon_remove(struct fsm *net)` ##

Performs ε-removal on the FSM through a subset construction.  The result may still be non-deterministic, but is guaranteed to be epsilon-free.  One-sided ε-symbols may remain in transducers.

## `struct fsm *fsm_minimize(struct fsm *net)` ##

Minimizes the FSM through either a variant of Hopcroft's algorithm (the default), or through calculating det(rev(det(rev(L)))), i.e. Brzozowski's method.  The input FSM is first made coaccessible.  Also, if the input FSM is not deterministic, it will be determinized before minimization.

## `struct fsm *fsm_coaccessible(struct fsm *net)` ##

Prunes a FSM to make it coaccessible.

## `struct fsm *fsm_topsort(struct fsm *net)` ##

Topologically sorts a FSM.  Cyclic FSMs cannot be topologically sorted and the algorithm will terminate as soon as a cycle is discovered.  The function also updates the `net->pathcount` of the FSM.  For cyclic FSMs, the value PATHCOUNT\_CYCLIC is given.

## `struct fsm *fsm_parse_regex(char *regex)` ##

Compiles a regular expression and returns the corresponding FSM.

## `struct fsm *fsm_reverse(struct fsm *net)` ##

Reverses a FSM by inverting transitions and marking all initial states final, and vice versa.  The result may be non-deterministic and non-minimal.

## `struct fsm *fsm_invert(struct fsm *net)` ##

Inverts a FSM by reversing the input/output labels.

## `struct fsm *fsm_lower(struct fsm *net)` ##

Extracts the lower side (range) from a FSM, and returns an FSA.  The result may be non-deterministic and non-minimal.

## `struct fsm *fsm_upper(struct fsm *net)` ##

Extracts the upper side (domain) from a FSM, and returns an FSA.  The result may be non-deterministic and non-minimal.

## `struct fsm *fsm_kleene_star(struct fsm *net)` ##

Performs Kleene closure on a FSM by a) adding a new initial state (which is also final), and transitions from each final state to this new state, as well as ε-transitions from the new initial state to each formerly initial state.

## `struct fsm *fsm_kleene_plus(struct fsm *net)` ##

Performs positive Kleene closure on a FSM by a) adding a new initial state (which is nonfinal), and transitions from each final state to this new state, as well as ε-transitions from the new initial state to each formerly initial state.

## `struct fsm *fsm_optionality(struct fsm *net)` ##

Alias for `fsm_union(net,fsm_empty_string())`

## `struct fsm *fsm_boolean(int value)` ##

Creates a FSM representing the boolean value `value`.  If value is `1`, a FSM representing the empty string (ε) is returned, otherwise a FSM representing the empty set (∅) is returned.

## `struct fsm *fsm_concat(struct fsm *net1, struct fsm *net2)` ##

Concatenates the two FSMs **net1 and**net2 by (a) adding ε-transitions from each final state in `*net1` to initial states in `*net2`, and making each final state in `*net1` nonfinal.  The result may be nondeterministic and nonminimal.

Regular expression equivalent: `X Y`

## `struct fsm *fsm_concat_n(struct fsm *net1, int n)` ##

Concatenates the FSM `*net1` with itself `n` times.  The result may	be nondeterministic and nonminimal.

Regular expression equivalent: `X^n`

## `struct fsm *fsm_concat_m_n(struct fsm *net1, int m, int n)` ##

Concatenates the FSM `*net1` with itself between `m` and `n` times.  The result may be nondeterministic and nonminimal.

Regular expression equivalent: `X^{m,n}`

## `struct fsm *fsm_union(struct fsm *net_1, struct fsm *net_2)` ##

Calculates the union of two FSMs by adding a new initial state (which is nonfinal), along with ε-transitions from this state to initial states of each of the two FSMs.  The result may be nondeterministic and nonminimal.

Regular expression equivalent: `X | Y`

## `struct fsm *fsm_priority_union_upper(struct fsm *net1, struct fsm *net2)` ##

Upper-side priority union of two FSMs.

Regular expression equivalent: `X .P. Y`

## `struct fsm *fsm_priority_union_lower(struct fsm *net1, struct fsm *net2)` ##

Lower-side priority union of two FSMs.

Regular expression equivalent: `X .p. Y`

## `struct fsm *fsm_intersect(struct fsm *net1, struct fsm *net2)` ##

Calculates the intersection of two FSMs by taking the lazy cross product of the two machines.

Regular expression equivalent: `X & Y`

## `struct fsm *fsm_compose(struct fsm *net1, struct fsm *net2)` ##

Calculates the composition of two FSMs by taking the lazy cross product of the two machines, together with a filtering mechanism for avoiding multiple, functionally identical paths.

Regular expression equivalent: `X .o. Y`

## `struct fsm *fsm_lenient_compose(struct fsm *net1, struct fsm *net2)` ##

Calculates the 'lenient' composition of two FSMs.

Regular expression equivalent: `X .O. Y`, or `[X .o. Y] .P. Y`

## `struct fsm *fsm_cross_product(struct fsm *net1, struct fsm *net2)` ##

Calculates the (unfiltered/naive) cross-product of two FSA.

Regular expression equivalent: `X:Y`

## `struct fsm *fsm_shuffle(struct fsm *net1, struct fsm *net2)` ##

Calculates the asynchronous product of two FSMs.  The result may be both nondeterministic and nonminimal.

## `struct fsm *fsm_precedes(struct fsm *net1, struct fsm *net2)` ##

Regular expression equivalent: `X<Y`

## `struct fsm *fsm_follows(struct fsm *net1, struct fsm *net2)` ##

Regular expression equivalent: `X>Y`

## `struct fsm *fsm_symbol(char *symbol)` ##

Returns a FSM that accepts a single symbol only, corresponding to the string `*symbol`.

## `struct fsm *fsm_explode(char *symbol)` ##

Returns a FSM that accepts a single string.  This is done by breaking up the string `*symbol` into single UTF-8 symbols, which are concatenated.

Regular expression equivalent: {string}

## `struct fsm *fsm_complete(struct fsm *net)` ##

Completes a FSM (automaton) so that each state has an outgoing transition for each symbol in the alphabet.

## `struct fsm *fsm_complement(struct fsm *net)` ##

Returns a FSM that is the complement of the argument FSM.  Not defined for transducers.

Regular expression equivalent: `~X`

## `struct fsm *fsm_term_negation(struct fsm *net1)` ##

Regular expression equivalent: `\X`

## `struct fsm *fsm_minus(struct fsm *net1, struct fsm *net2)` ##

Returns a FSM that contains all the paths from `*net1`, except those present in `*net2`.

Regular expression equivalent: `X - Y`

## `struct fsm *fsm_simple_replace(struct fsm *net1, struct fsm *net2)` ##

Compiles a simple, obligatory, replacement rule, without context specifications.

Regular expression equivalent: `X -> Y`

## `struct fsm *fsm_context_restrict(struct fsm *X, struct fsmcontexts *LR)` ##

Calculates the context restriction of center language `*X`.  Multiple contexts are stored in `*LR`.

Regular expression equivalent: `X => L1 _ R1, ..., Ln _ Rn`

## `struct fsm *fsm_contains(struct fsm *net)` ##

Returns a FSM that contains at least one string from the argument FSM.

Regular expression equivalent: `$X`

## `struct fsm *fsm_contains_opt_one(struct fsm *net)` ##

Returns a FSM that contains one string or no strings from the argument FSM.

Regular expression equivalent: `$?X`

## `struct fsm *fsm_contains_one(struct fsm *net)` ##

Returns a FSM that contains exactly one string from the argument FSM.

Regular expression equivalent: `$.X`

## `struct fsm *fsm_ignore(struct fsm *net1, struct fsm *net2, int operation)` ##

Returns a FSM where arbitrarily interspersed strings from the second argument language are present in the first argument language.

The variable operation is one of `OP_IGNORE_ALL`, or `OP_IGNORE_INTERNAL`

Regular expression equivalent: X/Y (with `OP_IGNORE_ALL`), X./.Y (with `OP_IGNORE_INTERNAL`)

## `struct fsm *fsm_quotient_right(struct fsm *net1, struct fsm *net2)` ##

Calculates the right quotient of the argument FSMs.

Regular expression equivalent: `X /// Y`

## `struct fsm *fsm_quotient_left(struct fsm *net1, struct fsm *net2)` ##

Calculates the left quotient of the argument FSMs.

Regular expression equivalent: `X\\\Y`

## `struct fsm *fsm_quotient_interleave(struct fsm *net1, struct fsm *net2)` ##

Returns a FSM containing all the strings one can intersperse to the second argument that produce a string from the first argument FSM.

Regular expression equivalent: `X /\/ Y`

## `struct fsm *fsm_substitute_label(struct fsm *net, char *original, struct fsm *substitute)` ##

Splices into all transitions containing the symbol `*original` in `*net` the FSM `*substitute`.  See `substitute defined` in the //foma// interface.

## `struct fsm *fsm_substitute_symbol(struct fsm *net, char *original, char *substitute)` ##

Substitutes transitions with symbol `*original` with `*substitute` in FSM `*net`

Regular expression equivalent: ```[X,Y,Z]``

## `struct fsm *fsm_universal()` ##

Returns a FSM containing the universal language Σ**.**

Regular expression equivalent: `?*` or Σ

## `struct fsm *fsm_empty_set()` ##

Returns a FSM that represents the empty set, i.e. containing a single initial state, no transitions, and no final state.

Regular expression equivalent: ∅ or `\?`

## `struct fsm *fsm_empty_string()` ##

Returns a FSM that contains only the empty string.

Regular expression equivalent: `0`

## `struct fsm *fsm_identity()` ##

Returns a FSM that accepts any single symbol.

Regular expression equivalent: `?` or Σ

## `struct fsm *fsm_quantifier(char *string)` ##

## `struct fsm *fsm_logical_eq(char *string1, char *string2)` ##

## `struct fsm *fsm_logical_precedence(char *string1, char *string2)` ##

## `struct fsm *fsm_lowerdet(struct fsm *net)` ##

Changes the lower-side transitions on `*net` to arbitrary symbols in such a way that the lower side is forced to be deterministic.

Regular expression equivalent: `_loweruniq(X)`

## `struct fsm *fsm_lowerdeteps(struct fsm *net)` ##

Changes the lower-side transitions on `*net` to arbitrary symbols in such a way that the lower side is forced to be deterministic.  ε-symbols on the output side are not changed, however.

Regular expression equivalent: `_loweruniqeps(X)`

## `struct fsm *fsm_markallfinal(struct fsm *net)` ##

Modifies the argument FSM so that all states are marked final.

Regular expression equivalent: `_allfinal(X)`

## `struct fsm *fsm_extract_nonidentity(struct fsm *net)` ##

Returns a FSA containing all the words in the argument FSM that do not map to themselves (are not in an identity relation).

Regular expression equivalent: `_notid(X)`

## `struct fsm *fsm_extract_ambiguous_domain(struct fsm *net)` ##

Returns an automaton containing all input words in the domain of the argument FSM have an ambiguous path through it.

Regular expression equivalent: `_ambdom(X)`

## `struct fsm *fsm_extract_ambiguous(struct fsm *net)` ##

Returns an FSM containing only those paths in the argument FSM that are ambiguous with respect to the input side.

Regular expression equivalent: `_ambpart(X)`

## `struct fsm *fsm_extract_unambiguous(struct fsm *net)` ##

Returns an FSM containing only those paths in argument FSM that are unambiguous.  That is, a mapping in the argument FSM is preserved only if its input string has a unique path through the transducer.

Regular expression equivalent: `_unambpart(X)`

## `struct fsm *fsm_sigma_net(struct fsm *net)` ##

Returns a FSM that accept all and only those single-symbol words formed by the alphabet of the argument FSM.  See the interface command `sigma net`.

## `struct fsm *fsm_sigma_pairs_net(struct fsm *net)` ##

Returns a FSM that accept all and only those symbol pairs present on transitions in the argument FSM.  See the interface command `label net`.

## `struct fsm *fsm_equal_substrings(struct fsm *net, struct fsm *left, struct fsm *right)` ##

## `struct fsm *fsm_letter_machine(struct fsm *net)` ##

Creates the equivalent letter machine from the argument FSM.  A letter machine is a FSM where every transition contains maximally one UTF-8 symbol.

Regular expression equivalent: `_lm(X)_

## `struct fsm *fsm_copy(struct fsm *net)` ##

Returns a copy of the argument FSM.

## Transducer/automaton application ##

Normally, when applying words through a transducer, `apply_init()` is called first, which returns a handle.  Subsequently, `apply_down()` or `apply_up()` is called with the handle and the word as arguments.  These functions are iterators and will produce multiple yields if the word argument is `NULL` on subsequent calls.  When the input word no longer yields any outputs, `NULL` is returned.  See FAQ for examples.


## `struct apply_handle *apply_init(struct fsm *net)` ##

Returns a handle which needs to be passed to the apply functions.

## `void apply_clear(struct apply_handle *h)` ##

Frees memory associated with a handle.


## `char *apply_down(struct apply_handle *h, char *word)` ##

Returns output words from a transducer with the input `*word`. `NULL` is returned when no more outputs are possible.  Call with `*word` set to the input word the first time, and `NULL` to retrieve subsequent outputs (if there are any).

## `char *apply_up(struct apply_handle *h, char *word)` ##

Inverse of `apply_down()`.

## `char *apply_upper_words(struct apply_handle *h)` ##

Returns all the words on the input side of the transducer (needs to be called once for every word). Returns `NULL` after the last word.

## `char *apply_lower_words(struct apply_handle *h)` ##

Returns all the words on the output side of the transducer (needs to be called once for every word). Returns `NULL` after the last word.

## `char *apply_words(struct apply_handle *h)` ##


Returns all the words of the transducer (needs to be called once for every word). Returns `NULL` after the last word.

## `char *apply_random_lower(struct apply_handle *h)` ##

Returns a random word from the output side.

## `char *apply_random_upper(struct apply_handle *h)` ##

Returns a random word from the input side.

## `char *apply_random_words(struct apply_handle *h)` ##

Returns a random "pair" from the transducer.

## `void apply_reset_enumerator(struct apply_handle *h)` ##

Resets an enumerator (apply random/words) to start from the beginning.

## `void apply_index(struct apply_handle *h, int inout, int densitycutoff, int mem_limit, int flags_only)` ##

Indexes a transducer for faster lookup.  Set inout to `APPLY_INDEX_INPUT` or `APPLY_INDEX_OUTPUT`. `densitycutoff` controls the minimum number of arcs that a state needs to have to be indexed.  `mem_limit` sets a cap of the maximum memory usage of the index in bytes (set to e.g. `INT_MAX` if you don't care).  `flags_only` is an option that when permits indexing of only those states that have flag diacritics.

## Indexing vs. binary search ##

In general, indexing is not worthwhile unless a transducer has quite a few arcs for a state.  Normally, a minimum of roughly 1,000 arcs are needed for indexed search to be significantly faster than binary search (which has the advantage of consuming no extra memory in the runtime).  Note that binary search is always used if arcs are sorted in the right direction (call `fsm_sort_arcs(struct fsm *net, 1)` to sort the input arcs, and `fsm_sort_arcs(struct fsm *net, 2)` to sort the output arcs before calling `apply_init()`.

## `void apply_set_show_flags(struct apply_handle *h, int value)` ##

Controls whether flags are shown in the output. Default is no.

## `void apply_set_obey_flags(struct apply_handle *h, int value)` ##

Controls whether flags are obeyed. Default is yes.

## Minimum edit distance functions ##

The minimum edit distance functions attempt to find the closest approximate match to a given input word from an automaton using an `A*`-search.  Transducers are not supported.  Also, automata need to have single-symbol alphabets (i.e. every transition must be a single utf8 symbol).  The functionality is similar to the `apply_up()` and `apply_down()` functions: first, an initialization is performed by calling `apply_med_init()` which returns a handle, which is passed to `apply_med()`.  This function is an iterator which keeps returning matches in order of cost (lowest to highest).  After each match is returned `apply_get_cost()` can be called to retrieve the MED cost of the last match returned.

## `struct apply_med_handle *apply_med_init(struct fsm *net)` ##

Initialized structures for MED functions.  An automaton is supplied.

## `char *apply_med(struct apply_med_handle *medh, char *word)` ##

Finds the closest approximate match to `* word` in the automaton previously initialized to the handle `* medh`.

## `void apply_med_set_heap_max(struct apply_med_handle *medh, int max)` ##

Caps the maximum number of bytes `apply_med()` can use for its heap when searching. Default is 256kB.

## `void apply_med_set_med_limit(struct apply_med_handle *medh, int max)` ##

Sets the maximum number of words `apply_med()` will return. Default is 4.

## `void apply_med_set_med_cutoff(struct apply_med_handle *medh, int max)` ##

Caps the maximum distance that `apply_med()` will search for matches. Default is 15.

## `int apply_med_get_cost(struct apply_med_handle *medh)` ##

Returns the MED cost of the last approximate match.

## `void apply_med_set_align_symbol(struct apply_med_handle *medh, char *align)` ##

Sets the alignment symbol that represents insertions and deletions.  This needs to be set if one wants to retrieve the alignment as well. Default is no padding.  If the symbol is set to, e.g. `-`, approximate matches will return string pairs like:

```
instring:  c-t
outstring: cat
```

## `char *apply_med_get_instring(struct apply_med_handle *medh)` ##

Retrieves the input string for the last call to `apply_med()`.  Note that this string might not be same string as was provided as input to `apply_med()` if an alignment symbol is specified by `apply_med_set_align_symbol()`.

## `char *apply_med_get_outstring(struct apply_med_handle *medh)` ##

Retrieves the output string for the last call to `apply_med()`.  This is equal to the string returned by `apply_med()`.