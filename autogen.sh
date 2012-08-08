#!/bin/sh

# Minimal autogen.sh file.

if test x$(uname -s) = xDarwin; then glibtoolize --force; else libtoolize --force; fi
aclocal
autoreconf -i
automake --add-missing
