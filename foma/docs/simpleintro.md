# The foma FST compiler

The *foma* compiler is essentially a tool for converting regular expressions to finite automata and transducers.  It supports a variety of operations (many more than are found in search-regex formalisms such as the *Python* `re` module).  The interface also includes tools for performing various tests on automata and transducers, passing words through transducers (doing translations), and importing and exporting transducers in various formats.

The *foma* program runs with a read-eval-print loop (REPL), like IPython/Jupyter.  That means that each command given is executed and the output is printed, and a new prompt is displayed.  You can run scripts of commands by either launching foma with the `foma -l` flag, or by typing `source filename` inside foma.
Input text files must be encoded as ASCII or UTF-8 without byte order mark.

## Foma basics

When you start *foma*, it should respond with a title page, something like:

    Foma, version 0.9.18alpha
    ...
    Type "help" to list all commands available.
    Type "help <topic>" or help "<operator>" for further help.
    foma[0]:

If you type `help` at the prompt, you'll be given a long terse list of help topics.  This tutorial is intended to alleviate the learning curve with foma.

## Compiling regular expressions

There are two commands `def` and `regex` that invoke the regular expression compiler.  Saying

`regex REGULAR-EXPRESSION ;`

simply compiles the regular expression into an automaton/transducer and gives you the result.  For example

`regex  a|b|c|d ;`

would compile the regular expression in question and return the following:

    371 bytes. 2 states, 4 arcs, 4 paths.
    foma[1]:

The result says that the (minimal) finite-state machine that corresponds to the regex has 2 states, 4 arcs, and that there are 4 valid paths through it.  This result gets stored on an internal stack, which is what the number inside the prompt `foma[1]:` refers to: you now have one finite-state machine stored on the internal stack.

The `def` or `define` command has the following schema:

`def NAME REGEX ;`

For example, to define a regular language that contains the words `cat` and `dog` and give that language a name `Animals`, we could issue:

`def Animals  c a t | d o g ;`

Note the spaces between the symbols.  Those are necessary, and will be explained below.  When doing a `def` you can subsequently use the defined label in other regular expressions, for example:

`regex Animals+ ;`

would now re-use your earlier definition of `Animals` and this regular expression is equivalent to saying:

`regex [c a t | d o g]+ ;`

To get an overview of what regular expressions you have defined (so called language constants), you can saying

`print defined`

in the foma interface.


### Viewing automata/transducers: `view`

If you have the *GraphViz* tool installed, you should be able to view the result of the last issued regex by typing `view`.  (Note that this only works on UNIXes such as Linux or OSX, not Windows).  It is often very helpful to view the result of your regular expressions graphically and reason about them in two modalities.

For a simpler view of any transducer, you can type `net`, which gives an ASCII listing of the transitions and basic information about the last issued regex.

You can also get information about the alphabet of a transducer by the command `sigma`.

### Basic FST testing

#### words

If you issue the command `words` you'll get a list of all the words accepted by an automaton.  For example:

    foma[0]: regex c a t | d o g ;
    455 bytes. 6 states, 6 arcs, 2 paths.
    foma[1]: words
    cat
    dog

#### pairs

With transducers, the `pairs` command is often useful.  It prints all the input/output pairs from a transducer.

    foma[0]: regex c:d a:o t:g;
    407 bytes. 4 states, 3 arcs, 1 path.
    foma[1]: pairs
    cat	dog


#### upper-words and lower-words

These commands print out only the input-side or output-side sets from a transducer.  Of course, since automata are treated like repeating transducers, there is no difference in behavior for automata.

    foma[0]: regex c:d a:o t:g;
    407 bytes. 4 states, 3 arcs, 1 path.
    foma[1]: upper-words
    cat
    foma[1]: lower-words
    dog


#### Testing automata/transducers

The `down` command will enter a special prompt (which you can exit with `CTRL-D`) and allows you to give a transducer an input, yielding the translation output:

    foma[0]: regex c:d a:o t:g;
    407 bytes. 4 states, 3 arcs, 1 path.
    foma[1]: down
    apply down> cat
    dog
    apply down> dog
    ???

If the transducer doesn't accept the input, then `???` is printed.

There is a equivalent `up` command, which works the same way, but in the inverse direction.

## Regex basics

The regular expression formalism in `foma` differs somewhat from standard formalisms.  It inherits a regular expression syntax called *Xerox* regular expressions.  The main differences between standard regular expressions and this formalism is given below.

### Main differences

The main differences and things to look out for can be summarized as follows:

* All regular expressions need to end in `;`
* Brackets `[` and `]` are used for grouping.  That means that a standard regular expression `(a|b)*` should be written `[a|b]*`.  There are no "character classes", so something like `[a-d]` must be explicitly listed in a union in *foma*: `[a|b|c|d]`
* Parentheses, in turn, denote "optionality".  So an expression such as `c a t (s)` would denote the two words `cat` and `cats` since whatever is inside the parentheses is "optional".  This comes from linguistic usage of parentheses.
* Multi-character symbols are used: these are strings that are themselves single symbols (see below).
* The wildcard symbol is `?`, not `.`.  For example, the set of words that end in `a` can be described as `?* a`
* Apart from union `|`, other logical operators are allowed, such as intersection `&` and complement `~`.  Since regular languages really denote sets of strings, we can perform set operations on them.  For example, the set of words that begin and end with `a` could be describes as `a ?* & ?* a`.
* Special symbols are escaped either by placing them in quotes, e.g. `"_"` to talk about an underscore symbol, or by prepending with a parenthesis symbol `%`, for example `%_`
* Transducer operations are included: the cross-product `:`, composition `.o.` and different string rewriting operators `->` are commonly used.
* `0` represents the epsilon (ε) -symbol.  This is mostly used in creating transducers.  For example, a transducer that accepts `a` as input and outputs nothing would be denoted by the regex `regex a:0 ;`

#### Multi-character symbols

*Foma* allows entire strings to be single atomic symbols.  This means that the two regular expressions below will give a different result:

    regex cat ;
    regex c a t ;

Here, the first one contains the single-symbol word `cat` whereas the second one contains a word made out of three symbols, `c`, `a`, and `t`.  Compile the two regular expressions, and then issue the command `view` to see the difference when you view it as an automaton/transducer.

Multi-character symbols are almost **never used** except for grammatical tags such as `[Noun]` or `[Sg]` or `[Pl]`.  The logic is that such tags really aren't strings, but denote atomic ideas and so should be treated as single symbols.  Morphological FSTs also become smaller if atomic symbols are used for tags.


-----
**Exercise 1**:

Create a regular expression denoting the set that contains the words `cat`, `dog`, `horse`, `cats`, `dogs`, `horses` (i.e. six words in total).  Try to not repeat the `s`-part in your regular expression. Use the `view` command to examine the resulting automaton/transducer.

-----

Commonly used notation:

| Expression   | Meaning |
|--------------|---------|
|`[ ]`         | Grouping        |
|`?`           | Wildcard        |
|`?*`          | Zero or more of anything        |
|`a`           | single symbol        |
|`\a`          | any symbol except `a`        |
|`\C`          | any symbol except on in the language `C`        |     
|`.#.`         | word edge (in rewrite rules)        |
| <code>[a&#124;b]</code>      | `a` or `b`        |
| <code>[C&#124;.#.]</code>    | `C` or word boundary        |
| `a*`         | zero or more `a`s        |
|`A .o. B`     | compose `A` and `B`        |


## Transducers: the cross-product

The cross-product operator `:` is the simplest way to create a transducer from a regular expression.  For example:

    foma[0]: regex a:b;
    228 bytes. 2 states, 1 arc, 1 path.

compiles a transducer that accepts `a` as its input, and outputs `b`.  (View this transducer graphically).

Note that the `:` binds tighter than any other operator.  This means that if you want to pair up strings, you'll need to indicate grouping by parentheses.  For example, to define a transducer that maps `cat` to `dog`, you should do

    foma[0]: regex [c a t]:[d o g];

And not `regex c a t:d o g ;` , since in this case, you get a transducer that maps `catog` to `cadog` (why?).

Another example, here's a transducer that deletes the first and last letters of every input word:

    foma[0]: regex ?:0 ?* ?:0;
    338 bytes. 3 states, 3 arcs, Cyclic.
    foma[1]: down
    apply down> cats
    at

Another example: here's a transducer that maps `cat[Sg]` to `cat` (foreshadowing some kind of crude morphological analysis).  Note the escaping of the `"[Sg]"`, which is now a single symbol.  

    foma[0]: regex  c a t "[Sg]":0 ;
    374 bytes. 5 states, 4 arcs, 1 path.
    foma[1]: up
    apply up> cat
    cat[Sg]

Note also that in the example, we ask (by applying `up`) what the input-side equivalent of output-side `cat` is.

-----
**Exercise 2**:

Use the cross-product operation to create a transducer that maps `c a t [Sg]` to `c a t` and `c a t [Pl]` to `c a t s` and `d o g [Sg]` to `d o g` and `d o g [Pl]` to `d o g s`.  Note that spaces are given here deliberately only to show that `[Sg]` and `[Pl]` should be multi-character symbols, whereas the rest should be single symbols.

-----

## Rewrite rule basics

Rewrite rules are often used when modeling morphophonology.  These have the following basic format.

`LHS -> RHS || LC _ RC ;`

where `LHS` is left-hand-side, `RHS` is right-hand-side, `LC` is left-context, and `RC` is right-context.

A regular expression such as:

    regex a -> b || c _ d ;

would create a transducer that converts all instances of `a` to `b`, but only if that `a` occurs between `c` and `d`.

The `LC` and `RC` are optional, so you could say:

    regex a -> 0 || _ b ;

creating a transducer that deletes `a`-symbols when they occur before a `b`.

You can also drop the context altogether, and say:

    regex x -> y ;

Which creates a transducer that maps all `x`-symbols to `y`, always, passing everything else through untouched.

### Word boundaries in rewrite rules

The special symbol `.#.` is used to refer to edges of words.  For example, the following transducer deletes all `x`-symbols, but only at the beginning of a word.  Everything else is passed through as-is.

    regex x -> 0 || .#. _ ;

### Epenthesis (insertion) rules

There is a special type of rule, called epenthesis rules is used whenever you want to insert something (from nothing) in a special position.  This is denoted by `[..]` as the `LHS`.  For example:

    regex [..] -> x || y _ z ;

would result in a transducer that inserts `x` between `y` and `z`, as seen below:

    foma[1]: down
    apply down> yzyz
    yxzyxz


## Composition: `.o.`

If you have two (or more) transducers, they can be joined by composition.  For example:

    foma[0]: regex a -> b .o. b -> c ;
    416 bytes. 1 state, 4 arcs, Cyclic.

Creates the composition of the two above transducers, mapping `a` directly to `c`.  Question: what does `c` map to in the inverse direction (do `up` and then `c`)? Why?

Normally, individual transducers are given names with `def`, and then composed.  For example:

    foma[0]: def RuleA a -> b || c _ d;
    defined RuleA: 634 bytes. 4 states, 16 arcs, Cyclic.
    foma[0]: def RuleB b -> x || c _ ;
    defined RuleB: 480 bytes. 2 states, 8 arcs, Cyclic.
    foma[0]: regex RuleA .o. RuleB;
    708 bytes. 4 states, 19 arcs, Cyclic.

-----
**Exercise 3**:

First create a transducer `Lexicon` that does the following: maps `c a t [Sg]` to `c a t` and `c a t [Pl]` to `c a t + s` and `b u s [Sg]` to `b u s` and `b u s [Pl]` to `b u s + s`.  Now that you have this, create a rewrite rule that translates all instances of `+` to `e` whenever it's surrounded by `s` on both sides.  Def this rule to have the name `EInsert`.  Now compose `Lexicon` and `EInsert` by issuing

    regex Lexicon .o. EInsert ;

You should get a resulting transducer that maps `b u s [Pl]` to `b u s e s` and `c a t [Pl]` to `c a t s`.  Now, you can create a third transducer `Cleanup` that always deletes all `+`-symbols and compose that in last.

    regex Lexicon .o. EInsert .o. Cleanup ;

You should now have your first complete morphological toy grammar.  Test it with `up` and the word `buses` and `cats`.

-----

## Common mistakes

* Not separating symbols by spaces; writing `regex cat|dog;` instead of `regex c a t | d o g;`.
* Forgetting the semicolon at the end of a regular expressions.
* Not keeping track of binding: see `help precedence` in foma to see which operators bind in what order.
