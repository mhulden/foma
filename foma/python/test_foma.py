#!/usr/bin/env python3
# -*- coding: UTF-8 -*-

"""
Test cases for Foma Python bindings.
"""

import pytest
from foma import FST


def test_load_fst():
    fst = FST.load('ate.fsm')
    assert isinstance(fst, FST)


def test_apply_fst(eat_fst):
    result, = eat_fst.apply_up('ate')
    assert result == 'eat+V+Past'


@pytest.fixture
def eat_fst():
    return FST.load('ate.fsm')
