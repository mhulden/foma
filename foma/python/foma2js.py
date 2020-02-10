#!/usr/bin/env python
"""Converts foma file to js array for use with Javascript runtime
Outputs a js array of all the transitions, indexed in the
input direction. This array can be passed to the js function
foma_apply_down() in foma_apply_down.js for stand-alone
transducer application."""

import sys
import os
import re
import argparse
import gzip
from enum import Enum
from collections import defaultdict


class Mode(Enum):
    NONE = 0
    PROPS = 1
    SIGMA = 2
    STATES = 4


def main():
    argparser = argparse.ArgumentParser(description=sys.modules[__name__].__doc__)
    argparser.add_argument('-n', '--name', help='Array variable name', default='myNet')
    argparser.add_argument('file', help='Foma file')
    args = argparser.parse_args()
    with gzip.open(args.file) as gzfile:
        lines = [line.decode('utf-8').strip() for line in gzfile.readlines()]
    mode = Mode.NONE
    num_nets = 0
    version = 0.0
    longest_symbol_length = 0
    props = {}
    sigma = {}
    trans = defaultdict(list)
    finals = {}
    RE_FOMA_NET = re.compile(r'.*##foma-net ([0-9]+\.[0-9]+)##')
    RE_PROPS = re.compile(r'##props##')
    RE_SIGMA = re.compile(r'##sigma##')
    RE_STATES = re.compile(r'##states##')
    RE_END = re.compile(r'##end##')
    for line in lines:
        match = RE_FOMA_NET.match(line)
        if match:
            version = float(match.group(1))
            num_nets += 1
            if num_nets > 1:
                raise ValueError('Only one network per file supported')
            continue
        match = RE_PROPS.match(line)
        if match:
            mode = Mode.PROPS
            continue
        match = RE_SIGMA.match(line)
        if match:
            mode = Mode.SIGMA
            continue
        match = RE_STATES.match(line)
        if match:
            mode = Mode.STATES
            continue
        match = RE_END.match(line)
        if match:
            mode = Mode.NONE
            continue
        if mode is Mode.PROPS:
            prop_names = ['arity', 'arccount', 'statecount', 'linecount', 'finalcount', 'pathcount', 'is_deterministic', 'is_pruned', 'is_minimized', 'is_epsilon_free', 'is_loop_free', 'extras', 'name']
            props = {k: v for k, v in zip(prop_names, line.split(' '))}
        elif mode is Mode.STATES:
            #state in out target final
            transitions = tuple(map(int, line.split(' ')))
            if transitions[0] == -1:
                continue
            if transitions[1] == -1 and len(transitions) == 4:
                arr_state, _, _, arr_final = transitions
                if arr_final == 1:
                    finals[arr_state] = 1
                continue
            if len(transitions) == 5:
                arr_state, arr_in, arr_out, arr_target, arr_final = transitions
                if arr_final == 1:
                    finals[arr_state] = 1
            elif len(transitions) == 4:
                arr_state, arr_in, arr_target, arr_final = transitions
                arr_out = arr_in
                if arr_final == 1:
                    finals[arr_state] = 1
            elif len(transitions) == 3:
                arr_in, arr_out, arr_target = transitions
            elif len(transitions) == 2:
                arr_in, arr_target = transitions
                arr_out = arr_in
            trans_key = '{}|{}'.format(arr_state, sigma[arr_in])
            trans_value = '{{{}:\'{}\'}}'.format(arr_target, sigma[arr_out])
            trans[trans_key].append(trans_value)
        elif mode is Mode.SIGMA:
            tokens = line.split(' ')
            number = int(tokens[0])
            symbol = tokens[1] if len(tokens) > 1 else ''
            if symbol == '@_EPSILON_SYMBOL_@':
                symbol = ''
            if symbol == '@_IDENTITY_SYMBOL_@':
                symbol = '@ID@'
            if symbol == '@_UNKNOWN_SYMBOL_@':
                symbol = '@UN@'
            if '\'' in symbol:
                symbol = symbol.replace('\'', '\\\'')
            sigma[number] = symbol
            if number > 2 and len(symbol) > longest_symbol_length:
                longest_symbol_length = len(symbol)
        elif mode is Mode.NONE:
            raise ValueError('Format error')
        
    print('var {} = new Object;'.format(args.name))
    print('{}.t = Array;'.format(args.name))
    print('{}.f = Array;'.format(args.name))
    print('{}.s = Array;'.format(args.name))
    print()

    for key in trans:
        state, inp = key.split('|')
        if inp == '@UN@':
            inp = '@ID@'
        print('{}.t[{} + \'|\' + \'{}\'] = [{}];'.format(args.name, state, inp, ','.join(trans[key])))

    for i in range(0, int(props['statecount'])+1):
        if i in finals:
            print('{}.f[{}] = 1;'.format(args.name, i))

    for i in range(3, len(sigma)):
        if i in sigma:
            print('{}.s[\'{}\'] = {};'.format(args.name, sigma[i], i))

    print('{}.maxlen = {} ;'.format(args.name, longest_symbol_length))


if __name__ == "__main__":
    main()
