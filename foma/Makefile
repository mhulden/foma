prefix = /usr/local
exec_prefix = $(prefix)
libdir = $(exec_prefix)/lib
bindir = $(exec_prefix)/bin
includedir = $(prefix)/include

VERSION = 0.9.14
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
CFLAGS = -O3 -Wall -D_GNU_SOURCE -std=c99 -fvisibility=hidden
FOMAOBJS = foma.o stack.o iface.o lex.interface.o
FLOOKUPOBJS = flookup.o
LIBOBJS = int_stack.o define.o determinize.o apply.o rewrite.o lexcread.o topsort.o flags.o minimize.o reverse.o extract.o sigma.o io.o structures.o constructions.o coaccessible.o utf8.o spelling.o dynarray.o mem.o stringhash.o trie.o lex.lexc.o lex.yy.o lex.cmatrix.o regex.tab.o

all: libfoma foma flookup

foma: $(FOMAOBJS) $(LIBOBJS)
	$(CC) $(CFLAGS) $(FOMAOBJS) $(LIBOBJS) $(LDFLAGS) -o $@

flookup: $(FLOOKUPOBJS) $(LIBOBJS)
	$(CC) $(CFLAGS) $(FLOOKUPOBJS) $(FLOOKUPLDFLAGS) -o $@

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

libfoma: $(LIBOBJS)
	$(AR) $(ARFLAGS) $(STATICLIB) $(LIBOBJS)
	$(RANLIB) $(STATICLIB)
	$(CC) $(CFLAGS) -shared -Wl,$(DFLAG),$(SHAREDLIBM) -o $(SHAREDLIBV) $(LIBOBJS) $(LDFLAGS)

install: foma libfoma
	-@if [ ! -d $(exec_prefix) ]; then mkdir -p $(exec_prefix); fi
	-@if [ ! -d $(includedir)  ]; then mkdir -p $(includedir); fi
	-@if [ ! -d $(libdir)      ]; then mkdir -p $(libdir); fi
	cp fomalib.h fomalibconf.h $(includedir);
	chmod 644 $(includedir)/fomalib.h
	cp foma $(bindir)
	cp $(LIBS) $(libdir)
	cd $(libdir); chmod 755 $(LIBS); \
	if test -f $(libdir)/$(SHAREDLIB); then rm  $(libdir)/$(SHAREDLIB); fi
	if test -f $(libdir)/$(SHAREDLIBM); then rm  $(libdir)/$(SHAREDLIBM); fi
	cd $(libdir); ln -s $(SHAREDLIBV) $(SHAREDLIB); cd $(libdir);	\
	ln -s $(SHAREDLIBV) $(SHAREDLIBM); (ldconfig || true)		\
	>/dev/null 2>&1; \

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

clean:
	$(RM) foma flookup $(FOMAOBJS) $(LIBOBJS) $(FLOOKUPOBJS) regex.tab.h regex.tab.c regex.output lex.yy.c lex.lexc.c lex.interface.c lex.cmatrix.c *.so* *.dylib* *.a
