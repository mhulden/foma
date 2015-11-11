# Example scripts #

Here is a collection of short scripts intended to illustrate various uses of FSTs and how to compile/design them with `foma`.

## Syllabification ##

Syllabification — the task of segmenting a sequence of phones or phonemes into syllables or even smaller units — can in many cases be performed accurately and easily with phonological replacement rules.

In particular, the interaction of two fairly universal phonological generalizations, the [maximum onset principle](http://www.glottopedia.de/index.php/Maximal_Onset_Principle) and the [sonority sequencing principle](http://en.wikipedia.org/wiki/Sonority_Sequencing_Principle) can be modeled with the [shortest-match left-to-right replacement operator](RegularExpressionReference#Replacement_operators.md): `A @> B || L _ R`.

To compile a replacement rule that acts as a syllabifier, we generally need knowledge of three language-specific details:

  * The set of allowable onsets in the language
  * The set of allowable codas (can usually be specified as a mirror image of the onsets)
  * The set of allowable nuclei (vowels, diphthongs, possible syllabic consonants)

For English, the set of onsets would contain strings such as: _p_, _pl_, _sp_, _spr_, as these are all legal onsets (occurring in words such as pot, play, spot, sprout), but not _`*`pn_, _`*`pk_, _`*`ts_, as these are unattested. The set will differ from language to language; French, for instance, allows the onset _pn_ (e.g. _pneu_) which is illegal in English, while (standard) Finnish, for instance, allows the stop-fricative sequence _ts_ in onsets (e.g. _tsaari), etc._

The general principle of how to build a syllabifier for a given language can be illustrated in the following toy code for an imaginary language that follows the sonority hierarchy and maximum onset principle strictly:

```
# toysyllabify1.script
define V [a|e|i|o|u];
define Gli [w|y];
define Liq [r|l];
define Nas [m|n];
define Obs [p|t|k|b|d|g|f|v|s|z];

define Onset (Obs) (Nas) (Liq) (Gli); # Each element is optional.
define Coda  Onset.r;                 # Is mirror image of onset.

define Syllable Onset V Coda;
regex Syllable @> ... "." || _ Syllable;
```

The set of allowable onsets in the language are first defined as any sequence consisting of optionally an obstruent, a nasal, a liquid, and a glide, allowing onsets such as _p_, _fn_, _zl_, znl_, etc. This is assumed to be language particular._

Each syllable is then defined as a sequence **onset-vowel-coda**, after which the main rule rewrites syllables as themselves (the ... notation), followed by a period in the context where preceded by another syllable.  The fact that we are using the shortest-leftmost replacement operator produces a model of the **maximum onset** principle: by dividing each rewrite so as to minimize the size of the onset-vowel-coda unit, we are in effect minimizing the coda, and therefore maximizing the following onset. Also, the **sonority sequencing principle** is encoded in the definition of the **onsets**. The interaction of the two definitions guarantees a unique syllabification for each word:

```
apply down> abrakadabra
a.bra.ka.da.bra
```

`[`to make sure our syllabifier always provides a unique output for each input, we can use the [properties `test functional` command](InterfaceReference#Tests_on_FSM.md) in foma`]`

```
foma[1]: test functional
1 (1 = TRUE, 0 = FALSE)
```

### More details ###

The above example dealt with quite an impoverished language; one that contained no diphthongs and had a fairly regular set of allowable onsets. In actuality, most languages allow both diphthongs and sometimes even consonantal elements in the syllable nucleus. To handle these and other complications, we can model the syllabification as a two-step process where we first mark off the nuclei, and after that the syllables themselves.

```
# toysyllabify2.script
define Gli [w|y];
define Liq [r|l];
define Nas [m|n];
define Obs [p|t|k|b|d|g|f|v|s|z];

define LowV  [a|e|o];
define HighV [i|u];
define V [LowV | HighV];
define Nucleus [V | LowV HighV | HighV HighV - [u u] - [i i]];

define Onset (Obs) (Nas) (Liq) (Gli);
define Coda  Onset.r;

define MarkNuclei Nucleus @-> %{ ... %} ;

define Syllable Onset %{ Nucleus %} Coda;
define MarkSyllables Syllable @> ... "." || _ Syllable;
regex MarkNuclei .o. MarkSyllables .o. %{|%} -> 0;
```

This syllabifier now handles words such as:

```
apply down> autonomia
au.to.no.mi.a
```

Here, we defined the nuclei as (1) a single vowel, or (2) a combination of a low and high vowel, or (3) a combination of two high vowels, except if they are both identical (disallowing _uu_ and _ii_). We then mark off the nuclei in the input word with brackets `{ ... }`, and proceed as before, except the syllabifier now assumes that nuclei come pre-marked with brackets. The intermediate brackets are removed at the final composition. In other words, we get a composition sequence as follows:

```
Input Word:          a u     t   o     n   o     m   i       a
After MarkNuclei    {a u }   t { o }   n { o }   m { i }   { a } 
After MarkSyllables {a u } . t { o } . n { o } . m { i } . { a }
After Removing {}    a u   . t   o   . n   o   . m   i   .   a 
```

Naturally, if we want to retain the nuclei as marked after syllabifying, there is no need to remove the brackets as the final step.


### Further reading ###

The above principles can be used to model a variety of complications in the syllabification task that may include: interaction of morpheme boundaries and syllable boundaries; interaction of stress and syllables (see e.g. [Hammond, 2006](#References.md)); ambisyllabicity `[`the idea that a segment may belong to more than one syllable`]` (see [Kahn, 1976](#References.md)). [Hulden(2006)](#References.md) contains more details of how to model these phenomena in general with finite-state machines.

## Post-editing of transducers ##

Sometimes it is necessary to perform surgical tricks to already existing transducers. One common problem is to remove a certain set of mappings modeled by some transducer.  One commonly recurring theme is the problem of removing words that are shorter/longer/have more symbols X than some other set of words. Here is a concrete example, using a solution that is applicable to this type of problem.

Consider a morpheme identifier transducer that maps words to their lemmas and prefixes/suffixes based on a dictionary and a grammar. Such a morpheme identifier could perhaps, given an input word `deconstruction`, provide the output `de+construct+ion` (since `construct` is a verb, and the other parts are known affixes), or, alternatively, `deconstruct+ion` (since `deconstruct` is also a separately known word in our dictionary).  We might like to alter such a transducer to _only_ return `deconstruct+ion`, using the reasonable argument that if there exists a longer stand-alone word in our dictionary, it might be better to use that instead of overanalyzing the word.  The same goes for various analyses: we'd rather not analyze `incorporate` as `in+corporate`, and could avoid doing so by choosing the longest possible analysis for the stem whenever possible.  Here's a toy analyzer that does the above overgeneration, mapping those two words to all their possible analyses:

```
define M [{de} (0:"+") {construct} 0:"+" {ion}] | [ {in} (0:"+") {corporate} ] ;
```

On to the removal of extraneous analyses; one way to remove the superfluous analyses is to, starting from the original transducer `M`, first create a transducer `M2` that contains all the mappings of `M`, except every word on the output side contains _at least_ one more `+`-symbol than the original transducer `M`.  See (1) and (2) of the following figure.  Such a transducer `M2` can be produced easily by composing `M` with a simple `+`-adding transducer:

```
define M2 M .o. [?* 0:"+" ?*]+; # Add at least one + to every word
```

![![](http://foma.googlecode.com/svn/wiki/xyz.png)](http://foma.googlecode.com/svn/wiki/xyz.png)


Now, we can simply calculate `M - M2` which removes from M the mappings where there is an alternative output for the same input with fewer `+`-symbols:

```
foma[0]: regex M;
1021 bytes. 28 states, 30 arcs, 4 paths.
foma[1]: regex M - M2;
1001 bytes. 26 states, 26 arcs, 2 paths.

foma[13]: down
apply down> deconstruction
deconstruct+ion
apply down> incorporate
incorporate
apply down>
```

The above trick comes with a caveat: since we are subtracting transducers, we are operating under the assumption that we _know_ the alignment of the input-output symbols, and that an output-side `+` is always aligned with `0` on the input side.  Normally, this would be the case, but if the grammar were designed otherwise, we could not count on the subtraction working correctly.  Another caveat is that if the transducer `M` has various alternative mappings that are not aligned, say `WXYZ` could be mapped to `W+XYZ` or `WX+Y+Z`, adding a `+` to the output side of `W+XYZ` could never match the output `WX+Y+Z`, and so it would not be removed by the subtraction operation, even though it has more `+`-symbols in it.


## The Soundex algorithm ##

The following is an implementation of the Soundex algorithm in `foma`. Its purpose is to map phonetically similar names to the same letter+number sequence. See for example [The National Archives](http://www.archives.gov/research/census/soundex.html) for details.

Here's an alternative description of the algorithm in 4 steps (following Don Knuth in [TAOCP Volume 3](http://www-cs-faculty.stanford.edu/~uno/taocp.html) ([Knuth, 1998, p.394](#References.md)):

  * Retain the first letter of the name and drop all other occurrences of a, e, h, i, o, u, w, y.

  * Replace consonants after the first letter with numbers, as follows:

```
    b, f, p, v => 1
    c, g, j, k, q, s, x, z => 2
    d, t => 3
    l => 4
    m, n => 5
    r => 6
```

  * If two or more letters with the same number were adjacent in the original name (before step 1), or adjacent except for intervening h's and w's, omit all but the first.

  * Force the format into letter-number-number-number by padding with zeroes if necessary, or deleting extra numbers to the right.

Here's the code:

```
# soundex.script #
define s0 [a|e|h|i|o|u|w|y] ;
define s1 [b|f|p|v] ;
define s2 [c|g|j|k|q|s|x|z] ;
define s3 [d|t] ;
define s4 l ;
define s5 [m|n] ;
define s6 r ;

# Remove all but the first of identical sounds if they are adjacent
define Step1 [s1+ @-> 0 || s1 _ ,, 
              s2+ @-> 0 || s2 _ ,, 
              s3+ @-> 0 || s3 _ ,, 
              s4+ @-> 0 || s4 _ ,, 
              s5+ @-> 0 || s5 _ ,, 
              s6+ @-> 0 || s6 _ ];

# h,w rule
define Step2 s0 -> 0 || s0 [h|w] _ ,, 
             s1 -> 0 || s1 [h|w] _ ,, 
             s2 -> 0 || s2 [h|w] _ ,, 
             s3 -> 0 || s3 [h|w] _ ,, 
             s4 -> 0 || s4 [h|w] _ ,, 
             s5 -> 0 || s5 [h|w] _ ,, 
             s6 -> 0 || s5 [h|w] _ ;

define Del s0 -> 0 || .#. ?+ _ ;

# Map to numbers
define Step3 [s1 -> 1 , s2 -> 2, s3 -> 3, s4 -> 4 , s5 -> 5 , s6 -> 6 || ?+ _ ];

# Pad with zeroes, or shorten
define Step4 [..] -> %0^3 || _ .#. .o. [? -> 0 || ?^4 _ ];

regex Step1 .o. Step2 .o. Del .o. Step3 .o. Step4;
```

The implementation models the Soundex algorithm in an apparently circuitous way (although arguably more suitable for the formalism we're working in).

We first (Step1) remove all but the first of all adjacent sounds belonging to the same group, we then (Step2) remove the latter of two sounds belonging to the same group, disregarding intervening `h` or `w` after which we remove all sounds belonging to group 0 (`[a|e|h|i|o|u|w|y]`) (Del), and finally we convert the letters to numbers (Step3) and pad or delete (Step4).

Some things to note:

  * We use the left-to-right longest unconditional replacement `@->` in Step1 to capture the collapsing of an unbounded sequence of sounds belonging to the same group.

  * The padding/deletion is done by first adding three zeroes (always), and then removing all but the first four symbols.

Testing:

```
foma[1]: down
  apply down> ashcraft
  a261
  apply down> koskenniemi
  k255
  apply down> knuth
  k530
  apply down> karttunen
  k635
  apply down> pfister
  p236
  apply down> gutierrez
  g362
  apply down> tymczak
  t522
  apply down> jackson
  j250

```


---


## Longest common subsequence/substring ##

  * The [longest common substring problem](http://en.wikipedia.org/wiki/Longest_common_substring_problem) is to find the longest string (or strings) that is a substring (or are substrings) of two or more strings.

  * The [longest common subsequence problem](http://en.wikipedia.org/wiki/Longest_common_subsequence_problem), by contrast, is to find the longest subsequence common to all sequences. It differs from the substring problem in that the definition of "sequence" implies possible discontinuity.

For example, the longest common substring of `S = abcaa` and `T = dbcadaa` is: `bca` while the longest common subsequence is `bcaa`.

Note that in neither case is the answer necessarily unique.  The longest common subsequence for `S = abc` and `T = acb` is `ab` and `ac` (both 2 symbols long). Likewise, the longest common substring for  `S = abc` and `T = cba` are `a`, `b`, and `c`.

The following script illustrates the calculation of the longest common substrings and subsequences by declaring functions for that purpose.

```
# lcs.script #
define Substring(X) [X .o. ?:0* ?* ?:0*].l;
define Subsequence(X) [X .o. [?|?:0]*].l;
define Longest(X) X - [[X .o. ?:a* ?:0+].l .o. a:?*].l;
define LCSubstr(X,Y) [Longest(Substring(X) & Substring(Y))];
define LCSubseq(X,Y) [Longest(Subsequence(X) & Subsequence(Y))];
```

After running the script, we can solve such problems by:

```
foma[0]: regex LCSubstr({abcaa},{dbcadaa});
434 bytes. 4 states, 3 arcs, 1 path.
foma[1]: words
bca
```

and

```
foma[0]: regex LCSubseq({abcaa},{dbcadaa});
450 bytes. 5 states, 4 arcs, 1 path.
foma[1]: words
bcaa
```


---


## Numbers to numerals ##

Lauri Karttunen's script produces a transducer that maps numbers (between 0 and 100) to numerals and vice versa.

```
# NumbersToNumerals.script

# Copyright (C) 2004  Lauri Karttunen
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This script constructs a transducer that maps the English numerals
# "one", "two", "three", ..., "ninety-nine", to the corresponding numbers
#  "1", "2", "3", ... "99".


define OneToNine [1:{one} | 2:{two} | 3:{three} | 4:{four}  |5:{five} |
                  6:{six} | 7:{seven} | 8:{eight} | 9:{nine}];

define TeenTen [3:{thir} | 5:{fif} | 6:{six}|
                7:{seven} | 8:{eigh} | 9:{nine}];

define Teens [1:0 [{0}:{ten} | 1:{eleven} | 2:{twelve}|
                  [TeenTen | 4:{four}] 0:{teen}]];

define TenStem [2:{twen} | TeenTen | 4:{for}];

# TenStem is followed either by "ty" paired with a zero
# or by "ty-" mapped to an epsilon and folowed by a one
# number. Note that {0} means zero and not epsilon

define Tens [TenStem [{0}:{ty} | 0:{ty-} OneToNine]];

define OneToNinetyNine [ OneToNine | Teens | Tens ];

# Let's put the result on the stack for testing.

push OneToNinetyNine

echo
echo Print random numbers
print random-upper 5
echo
echo Print random numerals
print random-lower 5
echo
echo Map numeral to number
echo Input: seventy-nine
apply up seventy-nine
```


---



## Reduplication (example from Warlpiri) ##

The following toy grammar of Warlpiri reduplication shows how the [`\_eq()`](RegularExpressionReference#_eq(X,L,R).md) can be used to capture such phenomena. The description of the reduplication comes from [Nash(1980)](#References.md), pp. 142-144 and [Sproat (1992)](#References.md), p. 58, and the method from [Hulden (2009)](#References.md).

Warlpiri reduplication is an examle of "partial reduplication" where the reduplication process operates on the base form of a word and copies only a "prosodic foot" or "reduplication skeleton" from the base. The prosodic foot is of the shape C V (C) (C) V. This reduplication process yields, for example:


| **Base form** |   **Reduplicated form** |
|:--------------|:------------------------|
| pakarni       |   pakapakarni           |
| pangurnu      |   pangupangurnu         |
| wantimi       |   wantiwantimi          |

The script has a base lexicon. All words in the lexicon may be followed by the tag `+Redup`.

We first mark the prosodic template with `< ... >` and, if the tag `+Redup` is present, prefix a `< [C|V]* >` sequence  (i.e., anything surrounded by < and > ) after which we apply `_eq()` which filters out all substrings ... `< X >` ... `< Y >` ... where X is not equal to Y in content.

The idea is to get strings in the composition sequence such as:

```
# 1. pangurnu+Redup          <- Lexicon
# 2. <pangu>rnu+Redup        <- enclose initial prosodic foot in brackets
# 3. <...><pangu>rnu+Redup   <- insert a prefix of "anything" in brackets
#                               if +Redup is present
# 4. <pangu><pangu>rnu       <- after _eq()
# 5. pangupangurnu           <- after auxiliary symbol removal
```


```
# warlpiri.script #
define C p|{ng}|{rn}|{rl}|k|j|w|n|t|m;
define V a|e|i|o|u;
define Lexicon {pangurnu}|{tiirlparnkaja}|{wantimi}|{pakarni};
define Morphology Lexicon ("+Redup");
define MarkTemplate C V (C) (C) V -> "<" ... ">" || .#. _ ;
define InsertPrefix [..] ->  "<" [C|V]* ">" || .#. _ ?* "<" ?* "+Redup";
define RemoveTags "+Redup" -> 0;
define RemoveBrackets "<"|">" -> 0;
define PreEq Morphology .o. MarkTemplate .o. InsertPrefix .o. RemoveTags;
regex _eq(PreEq, "<" , ">") .o. RemoveBrackets;
```

Testing:

```
foma[1]: up
apply up> pakarni
pakarni
apply up> pakapakarni
pakarni+Redup
apply up> tiitiirlparnkaja
tiirlparnkaja+Redup
apply up>
```


---


## Date parsing and markup ##

The following example script, provided by Lauri Karttunen, illustrates a markup application that parses valid dates and tags them.

```
# DateParser.script

# Copyright (C) 2004  Lauri Karttunen
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# define date expressions from "Monday, January 1, 1" to "Sunday,
# December 31, 9999" and build a shallow parser to mark them

# This script constructs a parser that marks up expressions such
# "Friday", "October 15", "October 15, 2004", and
# "Friday, October 15, 2005" by wrapping then inside HTML
# tags "<DATE>Friday</DATE>", "<DATE>February 29, 2004</DATE>", etc.
# See the end of the script usage examples.

define OneToNine [1|2|3|4|5|6|7|8|9];
define ZeroToNine ["0"|OneToNine];
define Even [{0} | 2 | 4 | 6 | 8] ;
define Odd [1 | 3 | 5 | 7 | 9] ;
define N [Even | Odd];

define Day     [{Monday} | {Tuesday} | {Wednesday} | {Thursday} |
                {Friday} | {Saturday} | {Sunday}] ;

define Month29 {February};
define Month30 [{April} | {June} | {September} | {December}];
define Month31 [{January} | {March} | {May} | {July} | {August} |
                {October} | {December}] ;

define Month   [Month29 | Month30 | Month31];

# Numbers from 1 to 31
define Date    [OneToNine | [1 | 2] ZeroToNine | 3 [%0 | 1]] ;

# Numbers from 1 to 9999
define Year [OneToNine ZeroToNine^<4];

# Day or [Month and Date] with optional Day and Year
define AllDates [Day | (Day {, }) Month { } Date ({, } Year)];

# Constraints on dates 30 and 31

define MaxDays30 ~$[Month29 { 30}];
define MaxDays31 ~$[[Month29 | Month30] { 31}];

# Combining constraints on dates 30 and 31
define MaxDays  [MaxDays30 & MaxDays31];

# Divisible by 4
# Of single digit numbers, 4 and 8 are divisible by 4.
# In larger numbers divisible with 4, if the penultimate
# is even, the last number is 0, 4, or 8. If the penultimate
# is odd, the last number is 2 or 6.
define Div4  [4 | 8 | N* [Even [%0 | 4 | 8] | Odd [2 | 6]]];

# Leap years are divisible by 4 but we subtract centuries
# that are not divisible by 400. Note the double
# subtraction. [[N+ - Div4] {00}] includes 1900 but not 2000.
define LeapYear [Div4 - [[N+ - Div4] {00}]] ;

# Note the [.#. | \N] at the end of the right context.
# 2405 is not a leap year although the year 240 was (in principle).
define LeapDates {February 29, } => _ LeapYear [.#. | \N];

define ValidDates [AllDates & MaxDays & LeapDates];

define DateParser [ValidDates @-> "<DATE>" ... "</DATE>"];

echo
echo Testing DateParser

push DateParser

down Today is Wednesday, October 5, 2004.
down Yesterday was Tuesday.
down February 29, 2000 was a leap day.
down but February 29, 1900 was not a leap day.
down Next leap day is on February 29, 2008
```


---


## Constraint satisfaction problems ##

Many discrete-valued constraint satisfaction problems (CSPs) can easily be solved with automata and transducers. Below are a few examples.

### Harry Potter problem 1 ###

A classical constraint satisfaction problem appears in the book "Harry Potter and the Philosopher's Stone" (Rowling, 1997):

Hogwarts students Harry and Hermione are in a room with two doors. One door leads forward to the chamber where the Philosopher's Stone is kept and the other leads back to where they have just come from, but both doorways are blocked by flames. In the room, they find seven bottles in a row, and a note that says:

```
#1.           Danger lies before you, while safety lies behind,
##            Two of us will help you, whichever you would find.
##            One among us seven will let you move ahead,
##            Another will transport the drinker back instead.
##            Two among our number hold only nettle wine,
##            Three of us are killers, waiting hidden in line.
##            Choose, unless you wish to stay here forevermore,
##            To help you in your choice, we give you these clues four:
##            First, however slyly the poison tries to hide
#2.           You will always find some on nettle wine's left side;

#3.           Second, different are those who stand at either end,
#4.           But if you would move forward, neither is your friend;
##            Third, as you see clearly, all are different size,
#5.           Neither dwarf nor giant holds death in their insides;
#6.           Fourth, the second left and second on the right
##            Are twins once you taste them, though different at first sight.
```

Question 1:

  * Is it possible to figure out which bottles are which from the contents of the note?

We can solve this by representing the row of bottles as a seven-symbol string containing characters that represent the different types of bottles (B=back, F=forward, P=poison, W=wine). We then proceed to intersect the set of possible seven-symbol strings automata that represent the clues (i.e. constraints) given in the note:

```
# harrypotter1.script #
define Clue1  [W W] <> [F] <> [B] <> [P P P];
define Clue2  W => P _ ;
define Clue3  ~[W ?* W|P ?* P];
define Clue4  ~[F ?*|?* F];
define Clue5  ?*;
define Clue6  [? W ?* W ?|? P ?* P ?];
regex  [Clue1 & Clue2 & Clue3 & Clue4 & Clue5 & Clue6];
```

The clue numbers correspond to the numbering of the lines in the note above. For example, `Clue1` defines the set of strings (using the shuffle operator) that contains exactly three `P`s, two `W`s, one `F`, and one `B`. `Clue2` encodes the idea that there's always some poison to the left of the wine. `Clue5` doesn't constrain anything at all, and is only used for illustrative purposes, since it's hard to make anything out of it without seeing the bottles (which presumably the protagonists in the story do).

Running the script yields:

```
...
791 bytes. 22 states, 28 arcs, 8 paths.
```

So there are 8 possible solutions:

```
foma[1]: words
BPWFPPW
BPWPFPW
BPFPWPW
BPPWFPW
PWFPPWB
PWPFPWB
PPWFBPW
PPWBFPW
```

Hence, we conclude that the note in itself does not impart sufficient information to figure out the ordering of the bottles.

### Harry Potter problem 2 ###

However, a more interesting problem is this: in the book, Hermione does, without any doubt, work out where the bottles are.  In other words, she must have gotten an additional clue from seeing the sizes of the bottles (`Clue5`) — information which the reader doesn't have access to. Now, this leads to the following question, which is indeed solvable:

  * Question 2: in which position is the bottle that allows them to move back?

To solve this question, we must first find, (1) from all the possible orderings of the sizes of bottles, only those orders that allow one to unambiguously solve the problem (since this is what Hermione did). Further, we need to (2) see if the position of the `B` coincides in all these solutions.

To solve (1), we do as follows: we construct a transducer that maps the set of permutations of strings that contain one instance of each number, to the set of solutions. We call this transducer `NumToSolutions`. It maps, for example, `1234567` to four different strings:

```
apply down> 1234567
BPFPWPW
BPPWFPW
BPWFPPW
BPWPFPW
```

since that particular ordering of bottle sizes has four possible solutions.

Next, we extract from that transducer only the unambiguous paths; i.e. that part of the transducer `NumToSolutions` where for some input word there is only one path to a final state.  We do so with `foma`'s [`\_unambpart()`-function](RegularExpressionReference#Built-in_functions.md).

This resulting transducer (`NumToUnambiguousSolutions`) only contains words such as `2137654` on its input side, since that particular bottle ordering gives a unique solution to the problem:

```
apply down> 2137654
PWPFPWB
```

Given the transducer `NumToUnambiguousSolutions`, we simply compose it with a transducer that changes all other symbols to `X`, except the `B` symbol, which we are interested in. Now, the output side of this transducer will contain only strings of `X`s and `B`s, where the `B` is in the position we are looking for. Compiling the script produces the following output:

```
706 bytes. 8 states, 7 arcs, 1 path.
```

Hence, the answer is unique:

```
foma[1]: words
XXXXXXB
```

Therefore, the bottle that leads back must be in position 7.

Here is the script:

```
# harrypotter2.script #
define NUM [1|2|3|4|5|6|7];
define Input [1 <> 2 <> 3 <> 4 <> 5 <> 6 <> 7];
define Clue1   [W W] <> [F] <> [B] <> [P P P];
define Clue2   W => P NUM _ ;
define Clue3 ~[NUM W ?* W|NUM P ?* P];
define Clue4  ~[NUM F ?*|?* F];
define Clue5  ~$[1 P|7 P];
define Clue6  [NUM ? NUM W ?* NUM W NUM ?|NUM ? NUM P ?* P NUM ?];
define NumSolutions Input .o. [? 0:?]^7 .o. Clue1/NUM .o. Clue2 .o. Clue3 .o. Clue4 .o. Clue5 .o. Clue6;
define NumToSolutions NumSolutions .o. NUM -> 0;
define NumToUnambiguousSolutions _unambpart(NumToSolutions);
regex [NumToUnambiguousSolutions .o. ?:X* B ?:X*].l;
```

A technical detail in the script is that we change the intermediate representation a little bit: the inputs to the transducer are always seven-digit numbers (with every number occurring once), but we then interleave `B`,`F`,`P`, and `W` symbols after each number. This is done by the part:

```
define NumSolutions Input .o. [? 0:?]^7 .o. Clue1/NUM ...
```

We then proceed as before, but the constraints have been changed to account for the fact that we now have 14 symbol strings with digits and letters interleaved. This allows us to express the crucial extra clue that Hermione could have received:

```
#5.  Neither dwarf nor giant holds death in their insides;
```

which we encode as:

```
define Clue5  ~$[1 P|7 P];
```

Once we're done with producing all the solutions, we simply remove the numbers by composition with:

```
NUM -> 0;
```

### The 8-queens problem ###

The 8-queens problem asks how to place 8 queens on a chessboard without any of the queens attacking each other. We solve this by representing the chessboard as a string of letters (`E`=empty square, `Q`=queen, `M`=column separator), and intersect the set of possible queen configurations with constraints to the effect that no queen attack another one.

```
# 8queens.script #

define Boardwith8Queens [Q^8 <> E^56 <> M^16] & [M ?^8 M]*;

# This will contain 4,426,165,368 strings, such as:

# MQEEEEEEEM
# MEEEEQEEEM
# MEEEEEEEQM
# MEEEEEQEEM
# MEEQEEEEEM
# MEEEEEEQEM
# MEQEEEEEEM
# MEEEQEEEEM

# (the newlines are added here for clarity)

# Then we filter out the illegal configurations
# to end up with only the correct solutions

# No two queens on the same row

define Attack1 $[Q \M* Q]; 

# No two queens on columns or diagonals

define Attack2 $[Q [[?^9  & $.{MM}] - ?* M]*  [[?^8]  & $.{MM}] Q];
define Attack3 $[Q [[?^10 & $.{MM}] - ?* M]*  [[?^9]  & $.{MM}] Q];
define Attack4 $[Q [[?^11 & $.{MM}] - ?* M]*  [[?^10] & $.{MM}] Q];

define Solution [Boardwith8Queens & ~Attack1 & ~Attack2 & ~Attack3 & ~Attack4];
```

Compiling the script yields:

```
...
defined Solution: 49.0 kB. 3026 states, 3116 arcs, 92 paths.
```

That is, there are 92 solutions to the problem. We can then print out all of them with the command `words`.

### Zebra/Einstein puzzles ###

Here is a method by Lauri Karttunen [(Beesley and Karttunen, 2003)](#References.md) to solve the famous [Zebra/Einstein puzzles](http://en.wikipedia.org/wiki/Zebra_Puzzle).

"Let us assume that there are five houses of different colors next to each other on the same road. In each house lives a man of a different nationality. Every man has his favorite drink, his favorite brand of cigarettes, and keeps pets of a particular kind.

  1. The Englishman lives in the red house.
  1. The Swede keeps dogs.
  1. The Dane drinks tea.
  1. The green house is just to the left of the white one.
  1. The owner of the green house drinks coffee.
  1. The Pall Mall smoker keeps birds.
  1. The owner of the yellow house smokes Dunhills.
  1. The man in the center house drinks milk.
  1. The Norwegian lives in the first house.
  1. The Blend smoker has a neighbor who keeps cats.
  1. The man who smokes Blue Masters drinks beer.
  1. The man who keeps horses lives next to the Dunhill smoker.
  1. The German smokes Prince.
  1. The Norwegian lives next to the blue house.
  1. The Blend smoker has a neighbor who drinks water.

The question to be answered is: Who keeps fish?"

Apart from solving the problem, the script also produces pretty-printed output:

```
In the yellow house the Norwegian drinks water, smokes Dunhills, and keeps cats.
In the blue house the Dane drinks tea, smokes Blends, and keeps horses.
In the red house the Englishman drinks milk, smokes PallMalls, and keeps birds.
In the green house the German drinks coffee, smokes Princes, and keeps fish.
In the white house the Swede drinks beer, smokes BlueMasters, and keeps dogs.
```

```
# Copyright (C) 2004  Lauri Karttunen
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.

define Color [blue | green | red | white | yellow];
define Nationality [Dane | Englishman | German | Swede | Norwegian];
define Drink [beer | coffee | milk |tea | water];
define Cigarette [Blend | BlueMaster | Dunhill | PallMall | Prince];
define Pet [birds | cats | dogs | fish | horses];

define          House   [Color Nationality Drink Cigarette Pet]; 

define  C1       $[red Englishman];
# The Englishman lives in the red house.
define  C2      $[Swede ~$Pet dogs];
# The Swede keeps dogs.
define  C3      $[Dane tea];
# The Dane drinks tea.
define  C4      $[green ~$Color white];
# The green house is just to the left of the white one.
define  C5      $[green ~$Drink coffee];
# The owner of the green house drinks coffee.
define  C6      $[PallMall birds];
# The Pall Mall smoker keeps birds.
define  C7      $[yellow ~$Cigarette Dunhill];
# The owner of the yellow house smokes Dunhills.
define  C8      [House^2 ~$Drink milk ~$Drink House^2];
# The man in the center house drinks milk.
define  C9      [? Norwegian ?*];
# The Norwegian lives in the first house.
define  C10     $[Blend ? ~$Pet cats | cats ~$Cigarette Blend];
# The Blend smoker has a neighbor who keeps cats.
define  C11     $[horses ~$Cigarette Dunhill | Dunhill ? ~$Pet horses];
# The man who keeps horses lives next to the Dunhill smoker.
define  C12     $[beer BlueMaster];
# The man who smokes Blue Masters drinks beer.
define  C13     $[German ~$Cigarette Prince];
# The German smokes Prince.
define  C14     $[Norwegian ~$Color blue | blue ? ~$Nationality Norwegian];
# The Norwegian lives next to the blue house.
define  C15     $[Blend ~$Drink water | water ? ~$Cigarette Blend];
# The Blend smoker has a neighbor who drinks water. 

define           C16    $fish;

define           Solution [House^5 & C1 & C2 & C3 & C4 & C5 &
                 C6 & C7 & C8 & C9 & C10 &
                 C11 & C12 & C13 & C14 & C15 & C16]; 


# There is someone who keeps fish. 

define          Describe        [House -> "In the "... .o.
       Color -> ... " house " .o.
       Nationality -> "the " ... " " .o.
       Drink -> "drinks " ... ", " .o.
       Cigarette -> "smokes "... "s, and " .o.
       Pet -> "keeps " ... "." "\u000a"];

regex [Solution .o. Describe];

print lower-words
```


---


## Counting combinations ##

In some cases the ability to quickly count the number of paths in acyclic automata can be used to count combinations easily.

### Powerball ###

To win the jackpot in U.S. powerball one must choose 5 numbers out of 59 correctly, and also separately the powerball, one out of 39.  What are the odds of winning?

We can count the number of combinations simply by representing all the possible combinations as a single string of `a`s and `b`s (each symbol representing non-drawn and drawn numbers):

```
regex [a^54 <> b^5] [a^38 <> b];
```

We use the shuffle operator to produce all strings containing 54 `a`s and 5 `b`s, followed by the powerball part: all strings containing 38 `a`s and one `b`.

The answer is then given in the number of paths in the automaton:

```
11.4 kB. 407 states, 714 arcs, 195249054 paths.
```

The odds are 1 in 195,249,054.


## Statement logic ##

Interestingly enough, truth value of expressions in statement logic can be tested using any regular expression compiler. The idea is as follows:  a regular expression is compiled out of the statement, where every variable `P` is replaced with the language `[?* P ?*]` (the language that contains P), or in `foma` shorthand: `$P`.  The Boolean operations are left as they are.

The statement is a [tautology](http://en.wikipedia.org/wiki/Tautology_%28logic%29) if the resulting language is the universal language `?*`, a [contradiction](http://en.wikipedia.org/wiki/Contradiction) if the result is the empty language, and neither of the two if the the statement is a [contingency](http://en.wikipedia.org/wiki/Contingency_%28philosophy%29).

For example. We want to see if:

```
(P ∨ ¬P)
```

is a tautology. To do this, we compile

```
regex [$P ∨ ¬$P];
```

(note that `foma` provides the variants ∨ = `|` for union and ¬ = `~` for complement).

The result is:

```
foma[1]: regex [$P ∨ ¬$P];
281 bytes. 1 states, 2 arcs, Cyclic.
```

And now we can test if it's the universal language with the `tuu` command (if the input projection is the universal language):

```
foma[6]: tuu
1 (1 = TRUE, 0 = FALSE)
```

Since `foma` also understands implications and converts them to unions and negations, we can write statements almost as they are.  For example, to test if [Peirce's Law](http://en.wikipedia.org/wiki/Peirce%27s_law) is a tautology, i.e.:

```
((P → Q) → P) → P
```

we compile:

```
foma[0]: regex [[$P → $Q] → $P] → $P;
323 bytes. 1 states, 3 arcs, Cyclic.
foma[1]: tuu
1 (1 = TRUE, 0 = FALSE)
```

We can also use the same method to produce counterexamples to statements that are contingencies. Consider the fallacy of [affirming the consequent](http://en.wikipedia.org/wiki/Affirming_the_consequent):

```
(P → Q) → (Q → P)
```

We can now compile:

```
regex [$P → $Q] → [$Q → $P];
```

First we observe that it is not a tautology, since the result is not the universal language. A counterexample can be provided by negating (`negate`) the result, and printing out a string from the language (for example the shortest string in the language with `pss`):

```
foma[1]: negate
339 bytes. 2 states, 4 arcs, Cyclic.
foma[1]: pss
Q
```

We conclude that setting `P = false` and `Q = true` is a counterexample to the reasoning.

Here's a final example showing that [reductio ad absurdum](http://en.wikipedia.org/wiki/Proof_by_contradiction) is a valid form of reasoning by reducing it to a tautology:

```
(¬P → Q) ∧ (¬P → ¬Q)) → P
```

```
regex [[¬$P → $Q] ∧ [¬$P → ¬$Q]] → $P;
323 bytes. 1 states, 3 arcs, Cyclic.
foma[11]: tuu
1 (1 = TRUE, 0 = FALSE)
```


## References ##

Beesley, K. R. and Karttunen, L. (2003). _Finite State Morphology._ CSLI Publications, Stanford, CA.

Hammond, M. (2006). Prosodic Phonology. In Aarts, B. (ed.) _The Handbook of English Linguistics_.  Blackwell.

Hulden, M. (2006). Finite-state syllabification. In Yli-Jyrä, et. al (Eds.). _Finite-State Methods and Natural Language Processing_. LNAI 4002. Springer. [(PDF version)](http://dingo.sbs.arizona.edu/~mhulden/hulden_syllabification_2006.pdf)

Hulden, M. (2009) Finite-State Machine Construction Methods and Algorithms for Phonology and Morphology. PhD Thesis. University of Arizona.

Kahn, D. (1976). _Syllable-based Generalizations in English Phonology_. PhD thesis, MIT. [(PDF version)](http://www.ai.mit.edu/projects/dm/theses/kahn76.pdf)

Knuth, D. E. (1998). _The Art of Computer Programming: Sorting and Searching, volume 3._ Addison-Wesley.

Nash, D. G. (1980). _Topics in Warlpiri Grammar._ PhD thesis, MIT. [(PDF version)](http://dspace.mit.edu/bitstream/handle/1721.1/15974/07044631.pdf?sequence=1)

Rowling, J. K. (1997). _Harry Potter and the Philosopher's Stone_. Bloomsbury.

Sproat, R. (1992). _Computational Morphology._ MIT Press.