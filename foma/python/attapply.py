# -*- coding: utf-8 -*-

"""
Utility for reading .att format automata and transducers and doing apply up or down, 
written in Python.  Also supports weighted FSMs.  In the weighted case the apply routines
return the strings in order cheapest first.  The apply functions work as a generator,
yielding output strings as long as there is a valid transduction.  Input files are 
standard AT&T format, with the last column being optionally a weight.  The files can be
compressed with gzip.  The apply generator produces tuples of (output, weight). For
unweighted automata/transducers, the weight is always 0.0.

Example usage:

>>> import attapply
>>> t = attapply.ATTFST('spanish.att.gz')  # Spanish morphological analyzer
>>> list(t.apply(u'tuviera', dir = 'up'))  # Analyze
[(u'tener[Subj][Imp][Form1][1P][Sg]', 0.0), (u'tener[Subj][Imp][Form1][3P][Sg]', 0.0)]

>>> list(t.apply('tener[Subj][Imp][Form1][1P][Sg]')) # Generate, default dir is 'down'
[(u'tuviera', 0.0)]

Author: Mans Hulden
License: Apache (Version 2.0)
Last Update: 11/07/2016
"""

import codecs, gzip
from heapq import *

class State:
    def __init__(self):
        self.final = False
        self.finalweight = 0.0
        self.transitionsin = {}
        self.transitionsout = {}
        
    def add_transition(self, target, input, output, weight):
        if input not in self.transitionsin:
            self.transitionsin[input] = []
        if output not in self.transitionsout:
            self.transitionsout[output] = []
        self.transitionsin[input].append((output, target, weight))
        self.transitionsout[output].append((input, target, weight))
    
    def get_transitions(self, symbol, dir = 'down'):
        if dir == 'down':
            s = self.transitionsin
        else:
            s = self.transitionsout
        if symbol in s:
            return s[symbol]
        else:
            return []
    
    def set_final(self, finalweight):
        self.final = True
        self.finalweight = finalweight

class ATTFST:

    def __init__(self, attfile, epsilon_symbol = u'@0@', identity_symbol = u'@_IDENTITY_SYMBOL_@', unknown_symbol = '@_UNKNOWN_SYMBOL_@'):

        """Reads an AT&T file (possibly gzipped) and inits data structures
        to apply() can be called.  If the AT&T file contains the special symbols
        epsilon, identity (repeat unknown) or unknown (one-sided unknown), these
        can be specified. The defaults are what foma produces with write att."""

        self.epsilon_symbol = epsilon_symbol
        self.identity_symbol = identity_symbol
        self.unknown_symbol = unknown_symbol        
        try:
            lines = [line.rstrip('\n') for line in codecs.getreader('utf-8')(gzip.open(attfile), errors='replace')]
        except:
            lines = [line.rstrip('\n') for line in codecs.open(attfile, "r", encoding="utf-8")]
        self.states = {}
        self.alphabet = set()
        for l in lines:
            fields = l.split('\t')            
            if len(fields) > 3:
                fields = l.split('\t')                
                source = int(fields[0])
                target = int(fields[1])
                insym = self._map_syms(fields[2])
                outsym = self._map_syms(fields[3])
                if len(fields) > 4:
                    weight = float(fields[4])
                else:
                    weight = 0.0
                self.alphabet.add(insym)
                self.alphabet.add(outsym)                
                if int(source) not in self.states:
                    nss = State()       
                    self.states[source] = nss            
                if int(target) not in self.states:
                    nst = State()
                    self.states[target] = nst
                self.states[source].add_transition(target, insym, outsym, weight)
            elif len(fields) < 3 and len(fields) > 0:
                final = int(fields[0])
                finalweight = 0.0
                if len(fields) > 1:
                    finalweight = float(fields[1])
                if final not in self.states:
                    nss = State()
                    self.states[final] = nss
                self.states[final].set_final(finalweight)
                
    def _map_syms(self, s):
        if s == self.epsilon_symbol:
            return u''
        return s

    def tokenize(self, word):
        tokens = []
        start = 0
        while start < len(word):
            t = word[start]
            for length in range(1, len(word) - start + 1):
                if word[start:start+length] in self.alphabet:
                    t = word[start:start+length]
            tokens.append(t)
            start += len(t)
        return tokens
        
    def apply(self, word, dir = 'down', tokenizer = None, return_joined = True):
        
        """Main apply function. Tokenizer func can be passed to the function.
        If no tokenizer is given, the alphabet of the FSM is used for
        tokenization of the input string, longest-match (as in foma).
        By default, the dir = 'down'. The output is by default joined,
        but a list of tokens can also be produced if return_joined is
        False."""
        
        if tokenizer == None:
            w = self.tokenize(word)
        else:
            w = tokenizer(word)
        heap = []
        heappush(heap, (0.0, 0, [], 0, False)) # (cost, -pos, output, state, final_included), use negpos to serve as tiebreaker
        while len(heap) > 0:
            cost, negpos, output, state, final_included = heappop(heap)
            if final_included == True:
                if return_joined == True:
                    yield(''.join(output), cost)
                else:
                    yield(output, cost)
            else:
                if -negpos == len(w):
                    # only match epsilon or finaladd
                    if final_included == False:
                        for outsym, target, weight in self.states[state].get_transitions('', dir = dir):
                            heappush(heap, (cost + weight, negpos, output + [outsym], target, False))
                    if self.states[state].final == True:
                        heappush(heap, (cost + self.states[state].finalweight, negpos, output, state, True))
                else: # Match other symbols as well
                    nextsym = [w[-negpos]]
                    if w[-negpos] not in self.alphabet:
                        nextsym = [self.unknown_symbol, self.identity_symbol]
                    for ns in nextsym:
                        for outsym, target, weight in self.states[state].get_transitions(ns, dir = dir):
                            if outsym == self.identity_symbol:
                                outsym = w[-negpos]
                            elif outsym == self.unknown_symbol:
                                outsym = u'?'
                            heappush(heap, (cost + weight, negpos - 1, output + [outsym], target, False))
                    for outsym, target, weight in self.states[state].get_transitions('', dir = dir): # Epsilons
                        heappush(heap, (cost + weight, negpos, output + [outsym], target, False))
