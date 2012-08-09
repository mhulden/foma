VERSION = $(PACKAGE_VERSION)
LIBRARY_RELEASE=0
LIBRARY_VERSION=9

# Local variables:
YACC = bison -d -t -v
LEX = flex -8
LEXCLEX = flex -8 --prefix=lexc
LEXIFACE = flex -8 --prefix=interface
LEXCMATRIX = flex -8 --prefix=cmatrix

# Linker flags:
AM_LDFLAGS = -lreadline -lz -ltermcap -static
AM_CFLAGS = -O3 -Wall -D_GNU_SOURCE -std=c99 -fvisibility=hidden -fPIC
AM_CPPFLAGS = -I${top_srcdir}/src

LDADD = $(top_builddir)/src/libfoma.la

# Main targets
lib_LTLIBRARIES = libfoma.la
bin_PROGRAMS = foma flookup cgflookup

# And their source files:
h_sources = fomalib.h fomalibconf.h
c_sources = apply.c coaccessible.c constructions.c \
			define.c determinize.c dynarray.c \
			extract.c flags.c int_stack.c io.c \
			lex.cmatrix.c lexc.h lex.lexc.c lex.yy.c lexcread.c \
			mem.c minimize.c \
			regex.tab.h regex.tab.c reverse.c rewrite.c \
			sigma.c spelling.c stringhash.c structures.c \
			topsort.c trie.c utf8.c

library_includedir = $(includedir)/foma
library_include_HEADERS = $(h_sources)

# Autotools variable bindings for the main targets:
libfoma_la_SOURCES = $(h_sources) $(c_sources)
libfoma_la_LDFLAGS= -version-info $(LIBRARY_VERSION) -release $(LIBRARY_RELEASE)

foma_SOURCES = foma.c foma.h iface.c lex.interface.c stack.c
foma_LDADD   = libfoma.la
#foma_LDFLAGS = -llibfoma.la $(FOMA_LIBS)

flookup_SOURCES = flookup.c
flookup_LDADD   = libfoma.la
flookup_LDFLAGS = -lz

cgflookup_SOURCES = cgflookup.c
cgflookup_LDADD   = libfoma.la
cgflookup_LDFLAGS = -lz

# Make sure the regex header file is built before it is included:
BUILT_SOURCES = regex.tab.h

lex.yy.c: regex.l regex.tab.h
	$(LEX) regex.l

lex.lexc.c: lexc.l
	$(LEXCLEX) $<

lex.interface.c: interface.l
	$(LEXIFACE) $<

lex.cmatrix.c: cmatrix.l
	$(LEXCMATRIX) $<

regex.tab.c regex.tab.h: regex.y
	$(YACC) $<

# clean-local:
# 	rm -f *.tab.* *.output lex.*

EXTRA_DIST= regex.l regex.y \
			cmatrix.l interface.l \
			lexc.l
