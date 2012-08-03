#!/bin/sh

# Minimal autogen.sh file.

aclocal
autoreconf -i
automake --add-missing
