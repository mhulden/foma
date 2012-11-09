## Process this file with autoconf to produce configure script

## Copyright (C) 2012 Måns Huldén

## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.

## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.

## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Information on package
FOMA_NAME=foma
FOMA_MAJOR=0
FOMA_MINOR=9
FOMA_EXTENSION=16
FOMA_VERSION=$FOMA_MAJOR.$FOMA_MINOR.$FOMA_EXTENSION

# Following things just need to be at top of all configure.ac files:

# AC_INIT sets up project metadata:
# first field the name of the "software"
# second field is version identifier iso 8901 date is good if you have no other
#   versioining schemes in mind
# third is email address for bug reports
# fourth is name of packages made by make dist, excluding .tar.gz
# fifth is the project web address
AC_INIT([foma], [$FOMA_VERSION], [mans.hulden@gmail.com], [foma],
        [https://code.google.com/p/foma/])
# AC_REVISION is used to get SVN revision into bug reports
AC_REVISION([$Revision:$])
# These configuration dirs hide some extra cruft
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
# AM_INIT_AUTOMAKE sets up make conventions
AM_INIT_AUTOMAKE([-Wall foreign])

# For automake >= 1.12
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

AC_SUBST([LIBFOMA_MAJOR], [$FOMA_MAJOR])
AC_SUBST([LIBFOMA_MINOR], [$FOMA_MINOR])
AC_SUBST([LIBFOMA_EXTENSION], [$FOMA_EXTENSION])
AC_SUBST([LIBFOMA_VERSION], [$FOMA_VERSION])
AC_SUBST([LIBFOMA_NAME], [foma])

# Some extra hoopla to avoid AC setting CFLAGS to -g -O2:
: ${CFLAGS=""}

# Checks for programs
LT_INIT
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CPP
AC_PROG_YACC
AC_PROG_LEX
AM_PROG_LIBTOOL

AC_PROG_INSTALL

# Make sure zlib is available, and that the required function is there:
AC_SEARCH_LIBS([gzprintf], [z], [ ],
		 [AC_MSG_FAILURE([no working zlib library was found])])

# config files
AC_CONFIG_FILES([Makefile
                 src/Makefile])

AC_OUTPUT

# Print a summary at the end:
cat <<EOF
./configure ready. To build, test and install:
    make
    make check
    [sudo] make install
EOF
