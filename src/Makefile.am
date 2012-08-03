VERSION = 0.9.16
CC = gcc
RANLIB = ranlib
YACC = bison -d -t -v
LEX = flex -8
LEXCLEX = flex -8 --prefix=lexc
LEXIFACE = flex -8 --prefix=interface
LEXCMATRIX = flex -8 --prefix=cmatrix
RM = /bin/rm -f
LDFLAGS = -lreadline -lz -ltermcap
FLOOKUPLDFLAGS = libfoma.a -lz
CFLAGS = -O3 -Wall -D_GNU_SOURCE -std=c99 -fvisibility=hidden -fPIC
FOMAOBJS = foma.o stack.o iface.o lex.interface.o
LIBOBJS = int_stack.o define.o determinize.o apply.o rewrite.o lexcread.o topsort.o flags.o minimize.o reverse.o extract.o sigma.o io.o structures.o constructions.o coaccessible.o utf8.o spelling.o dynarray.o mem.o stringhash.o trie.o lex.lexc.o lex.yy.o lex.cmatrix.o regex.tab.o

all: libfoma foma flookup cgflookup

foma: $(FOMAOBJS) $(LIBOBJS)
	$(CC) $(CFLAGS) $(FOMAOBJS) $(LIBOBJS) $(LDFLAGS) -o $@

flookup: flookup.o $(LIBOBJS)
	$(CC) $(CFLAGS) flookup.o $(FLOOKUPLDFLAGS) -o $@

cgflookup: cgflookup.o $(LIBOBJS)
	$(CC) $(CFLAGS) cgflookup.o $(FLOOKUPLDFLAGS) -o $@

STATICLIB = libfoma.a

UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
SHAREDLIB = libfoma.dylib
SHAREDLIBV = libfoma.dylib.$(VERSION)
SHAREDLIBM = libfoma.dylib.0
AR = libtool
ARFLAGS = -static -o
DFLAG = -dylib_install_name
else 
SHAREDLIB = libfoma.so
SHAREDLIBV = libfoma.so.$(VERSION)
SHAREDLIBM = libfoma.so.0
AR = ar
ARFLAGS = cru
DFLAG = -soname
endif

LIBS = $(SHAREDLIBV) $(STATICLIB)

libfoma: $(SHAREDLIBV)

$(SHAREDLIBV): $(LIBOBJS)
	$(AR) $(ARFLAGS) $(STATICLIB) $(LIBOBJS)
	$(RANLIB) $(STATICLIB)
	$(CC) $(CFLAGS) -shared -Wl,$(DFLAG),$(SHAREDLIBM) -o $(SHAREDLIBV) $(LIBOBJS) $(LDFLAGS)

$(OBJS): foma.h

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

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

clean-local:
	rm -f *.tab.* *.output lex.*
