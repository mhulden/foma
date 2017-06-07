"""
This is a simple helper tool for debugging foma-scripts
that are sequences of phonological rules meant to apply
in a certain order. It assumes a grammar is written as a
sequence of define-statements and ordered rewrite-rules
combined with a chain-statement (simulating composition
of the rules). It then passes words from STDIN through
the sequence of transducers and prints a decorated output
to STDOUT where rules that fire are shown between brackets.

Example:

----myscript.foma----
def  ARule a -> b || c _ d; # Rule one
def  BRule b -> c ||   _ d; # Rule two
chain ARule, BRule
----------------------

Now, 

$echo "cad" | python phonrule.py myscript.foma

produces the output

cad[ARule|Rule one]cbd[BRule|Rule two]ccd

Author: Mans Hulden
License: Apache (Version 2.0)
Last Update: 11/07/2016
"""

import sys
import re
from foma import *

import codecs

class Ruleset:

    def __init__(self):
        self.rules = {}
        self.comments = {}
        self.rc = []
        self.zerosymbols = ['"\u00b7"'] # Special symbols that behave like 0 in rules

    def rule_add(self, rulename, rule, commentline):
        """Compiles a rule, adds it to defined FSMs and stores a comment line
           and a rule name."""

        if '->' in rule:
            zeroes = '|'.join(self.zerosymbols)
            rule = '[~$[' + zeroes + '] .o. [' + rule + ']]/[' + zeroes + ']'

        FST.define(rule, rulename)
        myrule = FST(rule)
        self.rules[rulename] = myrule
        self.comments[rulename] = commentline

    def readrules(self, fomalines):
        """Reads foma rules either as define statements, or chain statements.
           The input lines may also consist of comments following a hash, e.g.
            define Crule C -> 0 || _ C C # Delete first of three C's
            define Vrule V -> 0 || _ V V # Delete first of three V's
            chain Crule, Vrule
        """
        for lineno, l in enumerate(fomalines):
            if 'define' in l or 'def ' in l:
                rulecom = l.split(' #')
                r = re.findall("(defi?n?e?)\s+(\S+)\s+([^;]+)", rulecom[0])
                if len(r[0]) != 3:
                    print "Syntax error on line %i" % lineno
                (_, rulename, rule) = r[0]
                if len(rulecom) > 1:
                    commentline = rulecom[1].strip()
                else:
                    commentline = ''
                self.rule_add(rulename, rule, commentline)
            if 'chain' in l:
                l = l.replace(';','')
                chain = re.findall('chain\s+(.*)', l)
                rc = chain[0].replace(' ','').split(',')
                self.rc = rc

    def applyrules(self, word, printall = True):
        """Apply a list of rules simulating composition.
           returns a string representation of the derivation."""
        output = [word]
        s = output[0]
        for rulename in self.rc:
            try:
                transducer = self.rules[rulename]
            except KeyError, e:
                key = rulename.decode("utf-8").encode("utf-8")
                print 'KeyError: Rule "%s" not found!' % key
                raise KeyError('Key not found: '+key);
            comment = self.comments[rulename]
            newoutput = transducer[output[0]]
            if newoutput[0] != output[0] or printall == True:
                if comment != '':
                    s += "[" + rulename + "|" + comment + "]"
                else:
                    s += "[" + rulename + "]"
                s += newoutput[0]
            output = newoutput
        return s

def main(argv):
    if len(sys.argv) < 2:
        print "Usage: phonrule.py [foma script]\n"
        sys.exit(1)
    fomafile = sys.argv[1]
    fomalines = [line.rstrip() for line in open(fomafile)]
    r = Ruleset()
    r.readrules(fomalines)
    if len(sys.argv) > 2:
        print r.applyrules(sys.argv[2], printall = False)
    else:
        for w in iter(sys.stdin.readline, ''):
            print r.applyrules(w.rstrip(), printall = False)
    sys.exit(1)

if __name__ == "__main__":
    main(sys.argv[1:])
