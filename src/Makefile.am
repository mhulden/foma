VERSION = $(PACKAGE_VERSION)
LIBRARY_RELEASE=0
LIBRARY_VERSION=9

# Local variables:
# RANLIB = ranlib
YACC = bison -d -t -v
LEX = flex -8
LEXCLEX = flex -8 --prefix=lexc
LEXIFACE = flex -8 --prefix=interface
LEXCMATRIX = flex -8 --prefix=cmatrix

# Linker flags:
AM_LDFLAGS = -lreadline -lz -ltermcap -static
AM_CFLAGS = -O3 -Wall -D_GNU_SOURCE -std=c99 -fvisibility=hidden -fPIC
AM_CPPFLAGS = -I${top_srcdir}/src

# FLOOKUPLDFLAGS = libfoma.a -lz

LDADD = $(top_builddir)/src/libfoma.la

# Main targets
lib_LTLIBRARIES = libfoma.la
bin_PROGRAMS = foma flookup cgflookup

h_sources = fomalib.h fomalibconf.h
c_sources = apply.c coaccessible.c constructions.c \
			define.c determinize.c dynarray.c \
			extract.c flags.c \
			int_stack.c io.c \
			lex.cmatrix.c lexc.h lex.lexc.c lex.yy.c lexcread.c \
			mem.c minimize.c \
			regex.tab.h regex.tab.c reverse.c rewrite.c \
			sigma.c spelling.c stringhash.c structures.c \
			topsort.c trie.c utf8.c

library_includedir = $(includedir)/foma
library_include_HEADERS = $(h_sources)

# And their source files:
libfoma_la_SOURCES = $(h_sources) $(c_sources)
libfoma_la_LDFLAGS= -version-info $(LIBRARY_VERSION) -release $(LIBRARY_RELEASE)

foma_SOURCES = foma.c foma.h iface.c lex.interface.c stack.c
foma_LDADD   = libfoma.la
#foma_LDFLAGS = -llibfoma.la $(FOMA_LIBS)

flookup_SOURCES = flookup.c

cgflookup_SOURCES = cgflookup.c

# Do we need this one?
BUILT_SOURCES = regex.tab.h

# Old code that should be cleaned away, but may be needed for reference for
# a while:
#
# FOMAOBJS = foma.o stack.o iface.o lex.interface.o
# LIBOBJS = int_stack.o define.o determinize.o apply.o rewrite.o lexcread.o topsort.o flags.o minimize.o reverse.o extract.o sigma.o io.o structures.o constructions.o coaccessible.o utf8.o spelling.o dynarray.o mem.o stringhash.o trie.o lex.lexc.o lex.yy.o lex.cmatrix.o regex.tab.o
# 
# all: libfoma foma flookup cgflookup
# 
# foma: $(FOMAOBJS) $(LIBOBJS)
# 	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@
# 
# flookup: flookup.o $(LIBOBJS)
# 	$(CC) $(CFLAGS) $< $(FLOOKUPLDFLAGS) -o $@
# 
# cgflookup: cgflookup.o $(LIBOBJS)
# 	$(CC) $(CFLAGS) $< $(FLOOKUPLDFLAGS) -o $@
# 
# STATICLIB = libfoma.a
# 
# UNAME = $(shell uname)
# 
# ifeq ($(UNAME), Darwin)
# SHAREDLIB = libfoma.dylib
# SHAREDLIBV = libfoma.dylib.$(VERSION)
# SHAREDLIBM = libfoma.dylib.0
# AR = libtool
# ARFLAGS = -static -o
# DFLAG = -dylib_install_name
# else 
# SHAREDLIB = libfoma.so
# SHAREDLIBV = libfoma.so.$(VERSION)
# SHAREDLIBM = libfoma.so.0
# AR = ar
# ARFLAGS = cru
# DFLAG = -soname
# endif
# 
# LIBS = $(SHAREDLIBV) $(STATICLIB)
# 
# libfoma: $(SHAREDLIBV)
# 
# $(SHAREDLIBV): $(LIBOBJS)
# 	$(AR) $(ARFLAGS) $(STATICLIB) $(LIBOBJS)
# 	$(RANLIB) $(STATICLIB)
# 	$(CC) $(CFLAGS) -shared -Wl,$(DFLAG),$(SHAREDLIBM) -o $(SHAREDLIBV) $(LIBOBJS) $(LDFLAGS)
# 
# $(OBJS): foma.h
#
#.c.o:
#	$(CC) $(CFLAGS) -c $< -o $@

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
