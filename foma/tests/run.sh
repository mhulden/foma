#!/bin/sh
set -ex

foma -q -f test-leaky-redefine.foma || exit 1;
foma -q -f test-segfault-eliminate.foma || exit 1;
if ! foma -q -f test-error-rendering.foma  2>&1 | grep -q 'syntax error'
then
  exit 1
fi
foma -q -f test-segfault-long-name > /dev/null || exit 1
foma -q -f test-segfault-empty-fst.foma > /dev/null || exit 1;
foma -q -f test-leaky-test.foma > /dev/null || exit 1;
