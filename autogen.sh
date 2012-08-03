#!/bin/sh

# Minimal autogen.sh file.

aclocal -I m4 --install
autoreconf -i
automake --add-missing
