# -*- coding: utf-8 -*-

#     Foma: a finite-state toolkit and library.                             #
#     Copyright © 2008-2014 Mans Hulden                                     #

#     This file is part of foma.                                            #

#     Foma is free software: you can redistribute it and/or modify          #
#     it under the terms of the GNU General Public License version 2 as     #
#     published by the Free Software Foundation.                            #

#     Foma is distributed in the hope that it will be useful,               #
#     but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#     GNU General Public License for more details.                          #

#     You should have received a copy of the GNU General Public License     #
#     along with foma.  If not, see <http://www.gnu.org/licenses/>.         #

from sys import maxint
from ctypes import *
from ctypes.util import find_library
#import os

fomalibpath = find_library('foma')
foma = cdll.LoadLibrary(fomalibpath)

class FSTstruct(Structure):

    _fields_ = [("name", c_char * 40), ("arity", c_int), ("arccount", c_int), ("statecount", c_int), ("linecount", c_int), ("finalcount", c_int), ("pathcount", c_longlong), ("is_deterministic", c_int), ("is_pruned", c_int), ("is_minimized", c_int), ("is_epsilon_free", c_int), ("is_loop_free", c_int), ("is_completed", c_int), ("arcs_sorted_in", c_int), ("arcs_sorted_out", c_int), ("fsm_state", c_void_p), ("sigma", c_void_p), ("medlookup", c_void_p)]
        
foma_fsm_parse_regex = foma.fsm_parse_regex
foma_fsm_parse_regex.restype = POINTER(FSTstruct)
foma_apply_init = foma.apply_init
foma_apply_init.restype = c_void_p
foma_apply_clear = foma.apply_clear
foma_apply_words = foma.apply_words
foma_apply_words.restype = c_char_p
foma_apply_lower_words = foma.apply_lower_words
foma_apply_lower_words.restype = c_char_p
foma_apply_upper_words = foma.apply_upper_words
foma_apply_upper_words.restype = c_char_p
foma_apply_down = foma.apply_down
foma_apply_down.restype = c_char_p
foma_apply_up = foma.apply_up
foma_apply_up.restype = c_char_p
foma_fsm_count = foma.fsm_count
foma_fsm_topsort = foma.fsm_topsort
foma_fsm_topsort.restype = POINTER(FSTstruct)
foma_fsm_minimize = foma.fsm_minimize
foma_fsm_minimize.restype = POINTER(FSTstruct)
foma_fsm_union = foma.fsm_union
foma_fsm_union.restype = POINTER(FSTstruct)
foma_fsm_intersect = foma.fsm_intersect
foma_fsm_intersect.restype = POINTER(FSTstruct)
foma_fsm_minus = foma.fsm_minus
foma_fsm_minus.restype = POINTER(FSTstruct)
foma_fsm_compose = foma.fsm_compose
foma_fsm_compose.restype = POINTER(FSTstruct)
foma_fsm_concat = foma.fsm_concat
foma_fsm_concat.restype = POINTER(FSTstruct)
foma_fsm_copy = foma.fsm_copy
foma_fsm_copy.restype = POINTER(FSTstruct)
foma_fsm_complement = foma.fsm_complement
foma_fsm_complement.restype = POINTER(FSTstruct)
foma_fsm_lower = foma.fsm_lower
foma_fsm_lower.restype = POINTER(FSTstruct)
foma_fsm_upper = foma.fsm_upper
foma_fsm_upper.restype = POINTER(FSTstruct)
foma_fsm_minimize = foma.fsm_minimize
foma_fsm_minimize.restype = POINTER(FSTstruct)
foma_fsm_destroy = foma.fsm_destroy
foma_fsm_equivalent = foma.fsm_equivalent
foma_fsm_equivalent.restype = c_int
foma_fsm_isempty = foma.fsm_isempty
foma_fsm_isempty.restype = c_int
foma_fsm_flatten = foma.fsm_flatten
foma_fsm_flatten.restype = POINTER(FSTstruct)

"""Define functions."""
foma_add_defined = foma.add_defined
foma_add_defined.restype = c_int
defined_networks_init = foma.defined_networks_init
defined_networks_init.restype = c_void_p
defined_functions_init = foma.defined_functions_init
defined_functions_init.restype = c_void_p

"""Trie functions."""
fsm_trie_init = foma.fsm_trie_init
fsm_trie_init.restype = c_void_p
fsm_trie_add_word = foma.fsm_trie_add_word
fsm_trie_done = foma.fsm_trie_done
fsm_trie_done.restype = POINTER(FSTstruct)

class FSTnetworkdefinitions(object):

    def __init__(self):
        self.defhandle = defined_networks_init(None)

class FSTfunctiondefinitions(object):

    def __init__(self):
        self.deffhandle = defined_functions_init(None)
        
                
class FST(object):

    networkdefinitions = FSTnetworkdefinitions()
    functiondefinitions = FSTfunctiondefinitions()

    @classmethod
    def define(cls, definition, name):
        """Defines an FSM constant; can be supplied regex or existing FSM."""
        name = cls.encode(name)
        if isinstance(definition, FST):
            retval = foma.add_defined(c_void_p(cls.networkdefinitions.defhandle), foma_fsm_copy(definition.fsthandle), c_char_p(name))
        elif isinstance(definition, basestring):
            regex = cls.encode(definition)
            retval = foma.add_defined(c_void_p(cls.networkdefinitions.defhandle), foma_fsm_parse_regex(c_char_p(regex), c_void_p(cls.networkdefinitions.defhandle), c_void_p(cls.functiondefinitions.deffhandle)), c_char_p(name))
        else:
            raise ValueError("Expected str, unicode, or FSM")
         
    @classmethod
    def wordlist(cls, wordlist, minimize = True):
        """Create FSM directly from wordlist.
           Returns a trie-shaped deterministic automaton if not minimized."""
        th = fsm_trie_init()
        for w in wordlist:
            thisword = cls.encode(w)
            fsm_trie_add_word(c_void_p(th), c_char_p(thisword))
        fsm = cls()
        fsm.fsthandle = fsm_trie_done(c_void_p(th))
        if minimize:
            fsm.fsthandle = foma_fsm_minimize(fsm.fsthandle)
        return fsm

    @staticmethod
    def encode(string):
        """Makes sure str and unicode are converted."""
        if isinstance(string, unicode):
            return string.decode('utf-8')
        elif isinstance(string, str):
            return string
        else:
            return str(string)
            #raise ValueError("Expected str or unicode")
            
    def __init__(self, regex = False):
        if regex:
            self.regex = self.encode(regex)
            self.fsthandle = foma_fsm_parse_regex(c_char_p(self.regex), c_void_p(self.networkdefinitions.defhandle), c_void_p(self.functiondefinitions.deffhandle))
            if not self.fsthandle:
                raise ValueError("Syntax error in regex")
        else:
            self.fsthandle = None
        self.getitemapplyer = None

    def __getitem__(self, key):
        if not self.fsthandle:
            raise KeyError('FST not defined')
        if not self.getitemapplyer:
            self.getitemapplyer = foma_apply_init(self.fsthandle)
        result = []
        output = foma_apply_down(c_void_p(self.getitemapplyer), c_char_p(self.encode(key)))
        while True:
            if output == None:
                return result
            else:
                result.append(output)
                output = foma_apply_down(c_void_p(self.getitemapplyer), None)
            
    def __del__(self):
        if self.fsthandle:
            #print "DESTROY"
            foma_fsm_destroy(self.fsthandle)

    def __str__(self):
        if not self.fsthandle:
            raise ValueError('FSM not defined')
        foma_fsm_count(self.fsthandle)
        s  = 'Name: %s\n' % self.fsthandle.contents.name
        s += 'States: %i\n' % self.fsthandle.contents.statecount
        s += 'Transitions: %i\n' % self.fsthandle.contents.arccount
        s += 'Final states: %i\n' % self.fsthandle.contents.finalcount
        s += 'Deterministic: %i\n' % self.fsthandle.contents.is_deterministic
        s += 'Minimized: %i\n' % self.fsthandle.contents.is_minimized
        s += 'Arity: %i\n' % self.fsthandle.contents.arity
        return s

    def __len__(self):
        if self.fsthandle:
            if self.fsthandle.contents.pathcount == -3: # UNKNOWN
                self.fsthandle = foma_fsm_topsort(self.fsthandle)
            if self.fsthandle.contents.pathcount == -1: # CYCLIC
                raise ValueError("FSM is cyclic")
            if self.fsthandle.contents.pathcount == -2: # OVERFLOW
                return maxint
            return self.fsthandle.contents.pathcount
        else:
            raise ValueError("FSM not defined")
        
    def __add__(self, other):
        return self.concat(other)

    def __sub__(self, other):
        return self.minus(other)
    
    def __le__(self, other):
        if self.fsthandle and other.fsthandle:
            return bool(c_int(foma_fsm_isempty(foma_fsm_minimize(foma_fsm_minus(foma_fsm_copy(self.fsthandle),foma_fsm_copy(other.fsthandle))))))
        else:
            raise ValueError('Undefined FST')

    def __lt__(self, other):
        if self.fsthandle and other.fsthandle:
            return (not self.__eq__(other)) and bool(c_int(foma_fsm_isempty(foma_fsm_minimize(foma_fsm_minus(foma_fsm_copy(self.fsthandle),foma_fsm_copy(other.fsthandle))))))
        else:
            raise ValueError('Undefined FST')        
        
    def __or__(self, other):
        return self.union(other)

    def __and__(self, other):
        return self.intersect(other)

    def __eq__(self, other):
        if self.fsthandle and other.fsthandle:
            return bool(c_int(foma_fsm_equivalent(foma_fsm_copy(self.fsthandle), foma_fsm_copy(other.fsthandle))))
        else:
            raise ValueError('Undefined FST')
    
    def __ne__(self, other):
        return not(self.__eq__(other))

    def __contains__(self, word):
        af = self.apply_down(word)
        try:
            i = af.next()
            return True
        except StopIteration:
            return False
                
    def __call__(self, other):
        if isinstance(other, basestring):
            return FST("{" + other + "}").compose(self)
        else:
            return other.compose(self)
    
    def __invert__(self):
        new = FST()
        new.fsthandle = self._fomacallunary(foma_fsm_complement)
        return new

    def __iter__(self):
        return self._apply(foma_apply_upper_words)
    
    def _apply(self, applyf, word = None):
        if not self.fsthandle:
            raise ValueError('FST not defined')
        applyerhandle = foma_apply_init(self.fsthandle)
        if word:
            output = applyf(c_void_p(applyerhandle), c_char_p(self.encode(word)))
        else:
            output = applyf(c_void_p(applyerhandle))
        while True:
            if output == None:
                foma_apply_clear(c_void_p(applyerhandle))
                return
            else:
                yield output
            if word:
                output = applyf(c_void_p(applyerhandle), None)
            else:
                output = applyf(c_void_p(applyerhandle))
                    
    def words(self):
        return self._apply(foma_apply_words)

    def lowerwords(self):
        return self._apply(foma_apply_lower_words)
                    
    def upperwords(self):
        return self._apply(foma_apply_upper_words)
        
    def apply_down(self, word):
        return self._apply(foma_apply_down, word)

    def apply_up(self, word):
        if self.fsthandle:
            return self._apply(foma_apply_up, word)
        else:
            raise ValueError('Undefined FST')

    def _fomacallunary(self, func, minimize = True):
        if self.fsthandle:
            handle = func(foma_fsm_copy(self.fsthandle))
            if minimize:
                handle = foma_fsm_minimize(handle)
            return handle
        else:
            raise ValueError('Undefined FST')
        
    def _fomacallbinary(self, other, func, minimize = True):
        if self.fsthandle and other.fsthandle:
            handle = func(foma_fsm_copy(self.fsthandle), foma_fsm_copy(other.fsthandle))
            if minimize:
                handle = foma_fsm_minimize(handle)
            return handle
        else:
            raise ValueError('Undefined FST')
        
    def union(self, other, minimize = True):
        new = FST()
        new.fsthandle = self._fomacallbinary(other, foma_fsm_union, minimize)
        return new

    def intersect(self, other, minimize = True):
        new = FST()
        new.fsthandle = self._fomacallbinary(other, foma_fsm_intersect, minimize)
        return new

    def minus(self, other, minimize = True):
        new = FST()
        new.fsthandle = self._fomacallbinary(other, foma_fsm_minus, minimize)
        return new

    def concat(self, other, minimize = True):
        new = FST()
        new.fsthandle = self._fomacallbinary(other, foma_fsm_concat, minimize)
        return new

    def compose(self, other, minimize = True):
        new = FST()
        new.fsthandle = self._fomacallbinary(other, foma_fsm_compose, minimize)
        return new
    
    def lower(self, minimize = True):
        new = FST()
        new.fsthandle = self._fomacallunary(foma_fsm_lower, minimize)
        return new

    def upper(self, minimize = True):
        new = FST()
        new.fsthandle = self._fomacallunary(foma_fsm_upper, minimize)
        return new

    def flatten(self):
        new = FST()
        eps_sym = FST('□')
        new.fsthandle = foma_fsm_flatten(foma_fsm_copy(self.fsthandle), foma_fsm_copy(eps_sym.fsthandle))
        return new
    
