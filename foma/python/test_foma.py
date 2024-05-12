#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Test cases for Foma Python bindings.
"""

import os
import pytest
import subprocess
from foma import FST


def test_load_fst():
    fst = FST.load('ate.fsm')
    assert isinstance(fst, FST)


def test_apply_fst(eat_fst):
    result, = eat_fst.apply_up('ate')
    assert result == 'eat+V+Past'


def test_apply_down(eat_fst):
    result, = eat_fst.apply_down('eat+V+3P+Sg')
    assert result == 'eats'


@pytest.fixture
def eat_fst():
    return FST.load('ate.fsm')


GOODJS = r"""var myNet = new Object;
myNet.t = new Object;
myNet.f = new Object;
myNet.s = new Object;

myNet.t[0 + '|' + '[NO\'UN]'] = [{1:'[NO\'UN]'}];
myNet.t[1 + '|' + '[VERB]'] = [{2:'[VERB]'}];
myNet.t[2 + '|' + 'c'] = [{5:''},{3:'d'}];
myNet.t[3 + '|' + 'a'] = [{4:'o'}];
myNet.t[4 + '|' + 't'] = [{7:'g'}];
myNet.t[5 + '|' + 'a'] = [{6:''}];
myNet.t[6 + '|' + 't'] = [{7:''}];
myNet.t[7 + '|' + 'ROTFLMAO🤣'] = [{8:'ROTFLMAO🤣'}];
myNet.f[8] = 1;
myNet.s['ROTFLMAO🤣'] = 3;
myNet.s['[NO\'UN]'] = 4;
myNet.s['[VERB]'] = 5;
myNet.s['a'] = 6;
myNet.s['c'] = 7;
myNet.s['d'] = 8;
myNet.s['g'] = 9;
myNet.s['o'] = 10;
myNet.s['t'] = 11;
myNet.maxlen = 10 ;
"""


def test_js():
    foma2js = os.path.join(os.path.dirname(__file__), "foma2js.py")
    testfoma = os.path.join(os.path.dirname(__file__), os.path.pardir,
                            "tests", "testjs.foma")
    out = subprocess.run(["python", foma2js, testfoma],
                         capture_output=True, check=True)
    js = out.stdout.decode()
    assert js.strip() == GOODJS.strip()
