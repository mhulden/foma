# Notes on installing foma #

## From source ##

Get the source tarball, unpack it, or get the svn version with

```
svn co http://foma.googlecode.com/svn/trunk/foma/
```

> and run

```
make; sudo make install
```

This should work on most Unix boxes, including Mac OSX. In case you have an older version of flex and bison and the source refuses to build, you can try updating the pre-built parser files with

```
touch *.c *.h
```

before running `make` if `flex` or `bison` complains. Note that this only works with the tarball distribution since the svn doesn't contain the pre-built lexer and parser files.

To build, _foma_ requires the _readline_ and _zlib_ libraries, which should be present on most systems.  It also uses `flex` (>=2.5.35) and `bison` for its parser. However, the tarball distribution comes with the parser files pregenerated.  For building from the svn, `flex` and `bison` are needed on the system.

## Linux binaries ##

If you have trouble compiling on linux, you can try the pre-built binaries for linux (32 and 64 bit). They include statically linked executables (`foma`,`flookup`,`cglookup`) and the `include/` and `lib/`-files.  These two directories you can ignore if you're not developing with the API.

## Mac OSX (binary) ##

Unpack the binary. Move `foma` and `flookup` to a suitable binary directory in your path. E.g. open a terminal in a directory with the files and do:

```
sudo cp ./foma /usr/bin/
sudo cp ./flookup /usr/bin/
```

The build is compiled for OSX versions >= 10.4.

## Windows ##

The windows binary versions are compiled using _cygwin_ (but do not require _cygwin_ to run). However, this means, the `system` command within `foma` doesn't work outside _cygwin_.