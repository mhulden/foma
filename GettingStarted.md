# Foma #

The foma interface is meant to facilitate the compilation of regular expressions into finite automata and transducers (finite-state machines/FSMs).  Complex machines are normally compiled into FSMs by specifying them through a number of script files containing regular expressions and FSM definitions.  Once FSMs are built with foma, they can be stored in binary or text files and used for other purposes through the foma [API](APIExamples.md), the command-line [flookup](FlookupDocumentation.md) utility, or other tools.

## Launching foma ##

Assuming _foma_ resides in your path, launching 'foma' should bring up a greeting and a prompt:

```
mylinux$ foma

Foma, version 0.9.13alpha
Copyright © 2008-2010 Mans Hulden
This is free software; see the source code for copying conditions.
There is ABSOLUTELY NO WARRANTY; for details, type "help license"

Type "help" to list all commands available.
Type "help <topic>" or help "<operator>" for further help.

foma[0]:
```

The number 0 following the prompt indicates the number of FSMs currently compiled and stored on an internal stack.


## Compiling regular expressions ##

Regular expressions are compiled into FSMs with the _regex_ command.  For example:

```
foma[0]: regex a b*;
263 bytes. 2 states, 2 arcs, Cyclic.
foma[1]:
```

will compile the regular expression `a* b` into a finite-state machine which is added on top of the stack (which is why the prompt now reads `[1]:` there is one FSM on the stack).  Note that regular expressions must end in a semicolon.


## Examining FSMs ##


The command `print net` (or just the abbreviated `net`) will print out information about the FSM on top of the stack:

```
foma[1]: net
Sigma: a b
Size: 2.
Net: 6B8B4567
Flags: deterministic pruned minimized epsilon_free 
Arity: 1 
Ss0:	a -> fs1.
fs1:	b -> fs1.
foma[1]:
```

Here the interface reports some properties of the FSM that the regular expression yielded: the alphabet (`a b`), the size of the alphabet (2 symbols), an arbitrary name, and properties of the FSM (deterministic, pruned, minimized, epsilon free).  Also the fact that the FSM is an automaton as opposed to a transducer is seen from the arity (1).

### Graphical display ###

Assuming the requisite software is installed (Graphviz/an image viewer) automata/transducers can also be displayed graphically through the `view net` (or just `view`) command:

```
foma[1]: view
```

which would bring up a window showing the FSM on top of the stack:

![![](http://foma.googlecode.com/svn/wiki/gettingstarted-view.png)](http://foma.googlecode.com/svn/wiki/gettingstarted-view.png)

## The stack ##

As mentioned, _foma_ stores all compiled regular expressions on an internal stack and the prompt shows how many FSMs are currently stored.  The stack can be cleared with the `clear` command.  Removing only the top machine is done with the `pop` command.  Most interface commands that operate on FSMs (for example, `view net`) always operate on the top machine stored on the stack.


## Special symbols & Unicode ##

Foma treats all input and output as UTF-8 and handles no other encodings.  Almost all Basic Latin symbols except the alphanumeric symbols are special and need to be escaped if their literal values are intended.  Escaping a symbol can be done by either prefixing it with a percent sign, or enclosing the symbol in quotes.  For example the regular expression:

```
foma[0]: regex "%"|%$;
```

produces an automaton that accepts two words: `%` and `$`.

Outside the Basic Latin Unicode range no escaping needs to be done, with the exception of a few special operators (see the section on [Reserved Symbols in foma](RegularExpressionReference#Reserved_symbols.md)).


## Printing the words that a FSM accepts ##

The command `print words` (or just `words`) will print out all the words that the top FSM on the stack accepts.


```
foma[0]: regex a b c | d e ;
317 bytes. 5 states, 5 arcs, 2 paths.
foma[1]: words
abc
de
foma[1]:
```

If the FSM is cyclic and accepts an infinite number of words, only a selection of them will be printed.  For example:

```
foma[0]: regex [a b c | d e]*;
389 bytes. 4 states, 5 arcs, Cyclic.
foma[1]: words

abc
de
foma[1]:
```

## Testing words ##

The commands `down` or `up` (the direction only matters for transducers) will move foma to a special prompt mode where words can be tested against the FSM on top of the stack.  The key `CTRL^D` reverts to the regular prompt.  For example:

```
foma[0]: regex a b* c;
257 bytes. 3 states, 3 arcs, Cyclic.
foma[1]: down
apply down> a
???
apply down> abbc
abbc
apply down> ^D
foma[1]:
```

In the above example, the regular expression `a b* c` was first compiled.  Then the words `a` and `abbc` were tested against the FSM, and only the latter was accepted.


## Multicharacter symbols in foma ##

A frequent source of confusion with _foma_ regular expressions is that _foma_ allows for multicharacter symbols (or string symbols) in regular expressions.  This is very different from conventional regular expression formalisms that only allow single-symbol symbols.  In foma, symbols _must_ be separated by spaces if they are to be intended as separate.  For example:

```
regex cat;
```

will produce a FSM that accepts a single word that consists of the single symbol `cat`.  Most of the time, this is not desirable, and what was really intended was:

```
regex c a t;
```

which will produce a FSM that accepts only one word consisting of three symbols `c`, `a`, and `t`.

One can also avoid having to type spaces between the symbols by enclosing words in curly brackets, which foma will always interpret as conjoined separate symbols, i.e.:

```
regex {cat};
```


## The special symbols ? and 0 in regexes ##

In regular expressions, the special symbol ? refers to _any_ single symbol whatsoever.  For example:

```
regex ? a;
```

will produce a FSM that accepts words consisting of any single symbol followed by the symbol `a`.

The symbol `0` always corresponds to the _epsilon_-symbol (ε), referring to the empty string.  The lower case greek symbol epsilon, ε, can also be used for this purpose.

For example, the regular expression:

```
regex a b | 0;
```

will produce a FSM that accepts two strings: the string `ab` and the _empty_ string.  The epsilon symbol and the empty string is used more frequently with transducers.


## The special symbol @ in FSMs ##


On the transitions of FSMs, the special symbol `@` is used to indicate any symbol outside the alphabet of that FSM.  This symbol has no special meaning in regular expressions, however, and should not be used unless the literal symbol `@` is meant.  The symbol `@` often comes about as the result of using the any symbol `?` in regular expressions.  For example the regular expression:

```
regex ?* a ?* ;
```

refers to any word that contains at least one `a`-symbol somewhere.  This regular expression compiles into the following FSM:

![![](http://foma.googlecode.com/svn/wiki/gettingstarted-at-sign.png)](http://foma.googlecode.com/svn/wiki/gettingstarted-at-sign.png)


## The define command ##

The command `define` can be used to label FSMs.  These labels can be reused in regular expressions.  The format of the `define`-command is as follows:

```
define name regular-expression ;
```

For example:

```
foma[0]: define Words c a t | d o g | m o u s e;
defined Words: 631 bytes. 10 states, 11 arcs, 3 paths.
foma[0]: regex Words "-" Words;
849 bytes. 20 states, 23 arcs, 9 paths.
foma[1]: words
cat-cat
cat-dog
cat-mouse
dog-cat
dog-dog
dog-mouse
mouse-cat
mouse-dog
mouse-mouse
```

In the above, we first defined a FSM **Words** to consist of the three words `cat`, `dog`, and `mouse`.  After that, we issued a new regular expression that reused the prior definition of **Words** in it.

Foma expands defined networks dynamically.  That means that had we never defined **Words** and simply issued the command:

```
foma[0]: regex Words "-" Words;
299 bytes. 4 states, 3 arcs, 1 path.
foma[1]: words
Words-Words
foma[1]:
```

we would produced an entirely different FSM: namely one that accepts a single word: **Words-Words**.

Define commands can of course be used in subsequent defines as well.  For example, consider:

```
foma[0]: define C [b|d|f|g|h|k|l|m|n|p|r|s|t|v] ;
defined C: 783 bytes. 2 states, 14 arcs, 14 paths.
foma[0]: define V [a|e|i|o|u];
defined V: 405 bytes. 2 states, 5 arcs, 5 paths.
foma[0]: define Syllable C* V C*;
defined Syllable: 1.2 kB. 2 states, 33 arcs, Cyclic.
foma[0]: define PhonWord Syllable+;
defined PhonWord: 1.3 kB. 2 states, 38 arcs, Cyclic.
foma[0]:
```

Here we first defined a set of consonants and vowels, called **C** and **V**, respectively.  Using those definitions, we defined a **Syllable** as any number of **C\*s followed by a**V**, followed again by any number of**C\*s.  Finally we defined `PhonWord` as one or more `Syllables`.

Issuing the `define` command without any regular expression removes the top FSM off the stack and labels it, for example:

```
foma[0]: regex c a t | d o g | m o u s e ;
631 bytes. 10 states, 11 arcs, 3 paths.
foma[1]: define Words;
defined Words: 631 bytes. 10 states, 11 arcs, 3 paths.
foma[0]:
```

defines a FSM called **Words** which is based on the topmost FSM on the stack.

### Examining the defined FSMs ###

The command `print defined` lists all the labeled FSMs currently stored.  After the above `C-V-Syllable-PhonWord` definitions, for example, the command produces the following list:

```
foma[0]: print defined
C            783 bytes. 2 states, 14 arcs, 14 paths.
V            405 bytes. 2 states, 5 arcs, 5 paths.
Syllable     1.2 kB. 2 states, 33 arcs, Cyclic.
PhonWord     1.3 kB. 2 states, 38 arcs, Cyclic.
```


### Saving defined FSMs ###

A set of defined networks can be stored into a binary file using the `save defined` command, and restored by `load defined`.  Often it is a good idea to store the original regular expressions from which the definitions are built, however, since these are not stored with the definitions.


## Script files ##

Small regular expressions and short definitions can be compiled well in the foma interface.  For larger projects, however, it is a good idea to store the definitions and regular expressions in script file, which is loaded into foma either at startup or using the `source` command.  For example, suppose we had a file called `words.foma` containing the following lines:


```
define C [b|d|f|g|h|k|l|m|n|p|r|s|t|v] ;
define V [a|e|i|o|u];
define Syllable C* V C*;
define PhonWord Syllable+;
```

we can compile these by starting foma with:

```
foma -l words.foma
```

in which case the file will be compiled line-by-line as if the commands had been issued in the interface.

We can also start up foma and do:

```
foma[0]: source words.foma
```


## Saving networks ##

### Binary format ###

To be able to use a FSM in e.g. [flookup](FlookupDocumentation.md), it needs to be save in a binary format.  The `save stack` command will store all FSMs on the stack in one file.  For basic usage, only a single FSM should be put in each file.  The [flookup](FlookupDocumentation.md) utility does permit special actions if multiple FSMs are stored in one file, but the simplest course of action is to put one FSM in one file.  For example:

```
foma[0]: regex a b c -> d e f ;
822 bytes. 5 states, 25 arcs, Cyclic.
foma[1]: save stack abc.foma
Writing to file abc.foma.
```

Will store the one transducer on the stack into the file `abc.foma`.  This can now be loaded back into `foma` with

```
foma[1]: load abc.foma
822 bytes. 5 states, 25 arcs, Cyclic.
foma[2]:
```

Or, it can be used externally from the [flookup](FlookupDocumentation.md) utility, as in this simple example:

```
mylinux$ echo "abc" | echo "abc" | flookup -ix abc.foma
def
```

### Textual formats ###

The commands `write att filename` and `write prolog filename` both store the top network on the stack in different fairly self-explanatory text-based formats: the `AT&T` format, or the Xerox prolog-format.  Also, `write dot` can be used to generate a `dot` file of the current FSM for use with the `AT&T` GraphViz tools.  This is the same `dot`-layout that the `view`-command uses.

### Function definitions ###

Foma has some 50 built-in operators and functions (see the regular expression [reference](RegularExpressionReference.md)) for a more complete list).  Apart from the built-in functions, users can define their own operators with the define command.

For example, suppose we needed to calculate the **derivative** of a FSM and realized there is no available operator to do this.  The derivative of a language **L** with respect to a symbol **a** is the remainder of the language after **a** has been consumed.  For example, the derivative of

```
[f o o | b a r]*
```

with respect to **f** is:


```
[o o] [f o o | b a r]*
```

This `derivative`-operator can be defined for subsequent use by the command:

```
define D(L,S) [L .o. S:0 ?*].l;
```

Now, we can use the two-argument `D()`-expression in any regular expressions subsequently authored. For example:

```
regex D([f o o | b a r]*, f);
```

will return a FSM equivalent to `[o o] [f o o | b a r]*`, as per the definition.

These definitions may be overloaded. That is, in the above example, a one-argument version of `D()` could be defined separately which will be kept distinct from the two-argument version.

#### Built-in functions ####

Foma also contains a number of built-in functions that work exactly like defined functions.  These, however, all begin with an underscore.  For example:

```
regex _allfinal(a b c);
```

will produce the automaton representing the language `a b c`, except all its states are marked final.