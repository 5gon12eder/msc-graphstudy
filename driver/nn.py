#! /usr/bin/python3
#! -*- coding:utf-8; mode:python; -*-

# Copyright (C) 2018 Karlsruhe Institute of Technology
# Copyright (C) 2018 Moritz Klammler <moritz.klammler@alumni.kit.edu>
#
# This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
# License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
# warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with this program.  If not, see
# <http://www.gnu.org/licenses/>.

__all__ = [
    'NNFeature',
    'NNTestResult',
]

from .constants import *

class NNFeature(object):

    def __init__(self, mean : float, stdev : float, name : str = None):
        assert type(mean) is float
        assert type(stdev) is float
        assert name is None or type(name) is str
        self.name = name
        self.mean = mean
        self.stdev = stdev

class NNTestResult(object):

    def __init__(self, lhs : Id, rhs : Id, exp : float, act : float):
        assert type(lhs) is Id
        assert type(rhs) is Id
        assert type(exp) is float
        assert type(act) is float
        self.lhs = lhs
        self.rhs = rhs
        self.expected = exp
        self.actual = act

    @property
    def error(self):
        return self.actual - self.expected

    @property
    def status(self):
        if self.expected < 0.0:
            if self.actual < 0.0: return TestStatus.TRUE_NEGATIVE
            if self.actual > 0.0: return TestStatus.FALSE_POSITIVE
        if self.expected > 0.0:
            if self.actual < 0.0: return TestStatus.FALSE_NEGATIVE
            if self.actual > 0.0: return TestStatus.TRUE_POSITIVE
        return TestStatus.UNKNOWN

    def __bool__(self):
        return self.status in [ TestStatus.TRUE_NEGATIVE, TestStatus.TRUE_POSITIVE ]
