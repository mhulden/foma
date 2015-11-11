# News #

Binary and tarball [downloads are now here](https://bitbucket.org/mhulden/foma/downloads).

  * June 13, 2015: License change to Apache 2.0
  * June 12, 2015: foma 0.9.18 released. Many bugfixes.

# Summary #

Foma is a compiler, programming language, and C library for constructing finite-state automata and transducers for various uses. It has specific support for many natural language processing applications such as producing morphological analyzers. Although NLP applications are probably the main use of foma, it is sufficiently generic to use for a large number of purposes.

The foma interface is similar to the Xerox [xfst](http://www.fsmbook.com) interface, and supports most of the commands and the regular expression syntax in xfst. Many grammars written for xfst compile out-of-the-box with foma.

The library contains efficient implementations of all classical automata/transducer algorithms: determinization, minimization, epsilon-removal, composition, boolean operations. Also, more advanced construction methods are available: context restriction, quotients, first-order regular logic, transducers from replacement rules, etc.

## Features ##

  * Xerox-compatible regular expression and scripting syntax (xfst/lexc)
  * Separate C API for constructing and handling automata
  * Import/export from Xerox/AT&T/OpenFST tools
  * Separate utility (flookup) for applying automata with various strategies
  * Supports flag diacritics
  * Contains functions for constraining reduplication (`_`eq())
  * Supports first-order regular logic expressions
  * Pre-built binaries available for Windows/Linux/OSX (see Downloads page)

## Documentation ##

See the

  * [Wiki](http://code.google.com/p/foma/w/list) on this site
  * [Tutorial slides and files from LREC 2010](http://foma.sourceforge.net/lrec2010/index.html)

## Cite ##

If you use foma, you can use the following citation for attribution. Please note that the details regarding the functionality of foma in that article are somewhat obsolete by now.

```
@inproceedings{hulden2009,
  title={Foma: a finite-state compiler and library},
  author={Hulden, Mans},
  booktitle={Proceedings of the 12th Conference of the European Chapter of the Association for Computational Linguistics},
  pages={29--32},
  year={2009},
  organization={Association for Computational Linguistics}
}
```