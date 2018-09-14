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
    # types
    'Integer',
    'Override',
    'Power',
    'Real',
    'Static',
    # functions
    'GetNth',
    'GetNthOr',
    'change_file_name_ext',
    'count',
    'encoded',
    'prepare_fingerprint',
    'fmtnum',
    'get_first',
    'get_first_or',
    'get_one',
    'get_one_or',
    'get_second',
    'get_second_or',
    'is_sorted',
    'make_ld_library_path',
    'replace_list',
    'rfc5322',
    'singleton',
    'value_or',
    'index_projection',
    'attribute_projection',
]

import datetime
import email.utils
import enum
import itertools
import math
import operator
import os.path

from .constants import *

_AUDIENCE_ECMA_SCRIPT = 1
_AUDIENCE_HUMAN = 2

class Integer(int):

    def __init__(self, value, precision=3):
        assert isinstance(value, int)
        assert isinstance(precision, int) and precision > 0
        self.__value = value
        self.__precision = precision

    @property
    def value(self):
        return self.__value

    def __repr__(self):
        return _format_number(self.__value, self.__precision, _AUDIENCE_ECMA_SCRIPT)

    def __str__(self):
        return _format_number(self.__value, self.__precision, _AUDIENCE_HUMAN)

class Real(float):

    def __init__(self, value, precision=3):
        assert isinstance(value, float)
        assert isinstance(precision, int) and precision > 0
        self.__value = value
        self.__precision = precision

    @property
    def value(self):
        return self.__value

    def __repr__(self):
        return _format_number(self.__value, self.__precision, _AUDIENCE_ECMA_SCRIPT)

    def __str__(self):
        return _format_number(self.__value, self.__precision, _AUDIENCE_HUMAN)

_METRIX_PREFIXES = {
    -3 : 'm', -6 : '\u03BC', -9 : 'n', -12 : 'p', -15 : 'f', -18 : 'a', -21 : 'z', -24 : 'y',
    +3 : 'k', +6 : 'M',      +9 : 'G', +12 : 'T', +15 : 'P', +18 : 'E', +21 : 'Z', +24 : 'Y',
    0 : None,
}

def _format_number(value, precision, audience):
    """

    Integers for humans:

        >>> _format_number(42, 0, _AUDIENCE_HUMAN)
        '42'
        >>> _format_number(42000, 0, _AUDIENCE_HUMAN)
        '42k'
        >>> _format_number(42000, 3, _AUDIENCE_HUMAN)
        '42,000'
        >>> _format_number(12345, 2, _AUDIENCE_HUMAN)
        '12k'
        >>> _format_number(123, 100, _AUDIENCE_HUMAN)
        '123'

    Reals for humans:

        >>> from math import pi as PI
        >>> [_format_number(PI * 10**i, 2, _AUDIENCE_HUMAN) for i in range(5)]
        ['3.14', '31.42', '314.16', '3.14k', '31.42k']

    """
    if isinstance(value, int) or math.isfinite(value):
        if audience == _AUDIENCE_ECMA_SCRIPT:
            return value.hex() if isinstance(value, float) else hex(value)
        if audience == _AUDIENCE_HUMAN:
            digits10 = lambda x : 1 if x == 0 else math.ceil(math.log10(abs(x)))
            decimals = 0 if isinstance(value, int) else precision
            magnitude = 0 if value == 0 else 3 * math.floor((digits10(value) + decimals - precision) / 3)
            if abs(value) >= 1: magnitude = max(0, magnitude)
            while pow(10, magnitude) > abs(value) and magnitude != 0: magnitude -= 3
            return '{:,.{dec}f}{:s}'.format(value / pow(10, magnitude), _METRIX_PREFIXES[magnitude] or '', dec=decimals)
        raise AssertionError(audience)
    elif math.isinf(value) and value > 0:
        return '+Infinity'
    elif math.isinf(value) and value < 0:
        return '-Infinity'
    elif math.isnan(value):
        return 'NaN'
    else:
        raise AssertionError(value)

class Power(object):

    """
    You can raise some base by some exponent, look:

        >>> p = Power(2, 5)
        >>> p.base
        2
        >>> p.exponent
        5
        >>> p.value
        32

    It has a pretty representation:

        >>> Power(2, 5)
        2**5 = 32

    And it can be used like a normal float, if need be:

        >>> float(Power(2, 5))
        32.0

    """

    def __init__(self, base, exponent):
        self.base = base
        self.exponent = exponent

    @property
    def value(self):
        return self.base**self.exponent

    def __repr__(self):
        return '{:g}**{:g} = {:g}'.format(self.base, self.exponent, self.value)

    def __float__(self):
        return float(self.value)

def change_file_name_ext(filename, newext):
    """
    Change the file name extension of a file name:

        >>> change_file_name_ext('~/Music/happysong.mp3', 'ogg')
        '~/Music/happysong.ogg'

    Add a file name extension to a file name that had none before:

        >>> change_file_name_ext('README', 'txt')
        'README.txt'

    File name extensions with multiple parts are handled properly:

        >>> change_file_name_ext('package.tar.gz.sig', 'asc')
        'package.asc'

    """
    assert newext and not newext.startswith('.')
    while True:
        (filename, oldext) = os.path.splitext(filename)
        if not oldext: break
    return filename + '.' + newext

def make_ld_library_path(prepend):
    """
    Get a list that contains the current library path with some additional items inserted at the beginning.  Note how
    all directory names are normalized:

        >>> os.environ['LD_LIBRARY_PATH'] = '/home/lib/foo:/var/lib/./././must/../have::::'
        >>> make_ld_library_path(['/home/lib/bar/', '/home/lib/baz'])
        ['/home/lib/bar', '/home/lib/baz', '/home/lib/foo', '/var/lib/have']

    If no prefix is given, nothing is done:

        >>> os.environ['LD_LIBRARY_PATH'] = '/home/lib/foo/:/var/lib/foo/'
        >>> make_ld_library_path(None) is None
        True

    `None` is not the same as an empty list:

        >>> os.environ['LD_LIBRARY_PATH'] = '/home/lib/foo/:/var/lib/foo/'
        >>> make_ld_library_path([])
        ['/home/lib/foo', '/var/lib/foo']

    Won't you dare using relative paths in your `LD_LIBRARY_PATH`:

        >>> os.environ['LD_LIBRARY_PATH'] = 'bad'
        >>> make_ld_library_path(['/this/is/fine'])
        Traceback (most recent call last):
        ValueError: LD_LIBRARY_PATH must only contain absolute paths
        >>>
        >>> os.environ['LD_LIBRARY_PATH'] = '/this/is/fine'
        >>> make_ld_library_path(['this/is/not/okay'])
        Traceback (most recent call last):
        ValueError: LD_LIBRARY_PATH must only contain absolute paths

    The environment variable need not be set:

        >>> del os.environ['LD_LIBRARY_PATH']
        >>> make_ld_library_path(['/home/lib/'])
        ['/home/lib']

    """
    if prepend is None:
        return None
    paths = list(filter(None, os.getenv('LD_LIBRARY_PATH', '').split(':')))
    newpath = [os.path.normpath(p) for p in itertools.chain(prepend, paths)]
    if not all(map(os.path.isabs, newpath)):
        raise ValueError("LD_LIBRARY_PATH must only contain absolute paths")
    return newpath

def is_sorted(seq, comp=None):
    """
    An empty range is sorted:

        >>> is_sorted([])
        True

    A range is sorted:

        >>> is_sorted(range(42))
        True

    Arbitrary garbage is not sorted:

        >>> is_sorted("The quick brown fox jumps over the leazg dog")
        False

    A reversed range is not sorted ...

        >>> is_sorted(reversed(range(10)))
        False

    ... unless compared with the right operator:

        >>> is_sorted(reversed(range(10)), comp=operator.gt)
        True

    """
    if comp is None:
        comp = operator.le
    previous = list()
    def predicate(rhs):
        if previous:
            lhs = previous[0]
            previous[0] = rhs
            return comp(lhs, rhs)
        else:
            previous.append(rhs)
            return True
    return all(map(predicate, seq))

def value_or(obj, default=None):
    return obj if obj is not None else default

def get_one(seq):
    items = seq if isinstance(seq, list) else list(seq)
    if len(items) != 1:
        raise AssertionError()
    return items[0]

def get_one_or(seq, default=None):
    """
    Get the sole item in a sequence:

        >>> get_one_or([42])
        42

        >>> get_one_or([42], default="strawberry")
        42

    Get nothing from nothing:

        >>> get_one_or([]) is None
        True

    Get something from nothing:

        >>> get_one_or([], default=42)
        42

    Don't get the first item from a longer sequence, though:

        >>> get_one_or([1, 2, 3])
        Traceback (most recent call last):
        AssertionError

    Sequences may be lazy:

        >>> get_one_or(range(1))
        0

    However, this is still not okay:

        >>> get_one_or(range(10))
        Traceback (most recent call last):
        AssertionError

    """
    first = default
    for (i, value) in enumerate(seq):
        if i > 0: raise AssertionError()
        first = value
    return first

def get_first(seq):
    for item in seq:
        return item
    raise AssertionError()

def get_first_or(seq, default=None):
    """
    Get the default value:

        >>> get_first_or([], default=42)
        42

    Get the first and only element:

        >>> get_first_or([42], default='X')
        42

    Get the first element:

        >>> get_first_or("abc", default=0)
        'a'

    """
    for item in seq:
        return item
    return default

class GetNth(object):

    def __init__(self, n):
        self.__n = n

    def __call__(self, seq):
        for (i, item) in enumerate(seq):
            if i == self.__n:
                return item
        raise AssertionError()

class GetNthOr(object):

    def __init__(self, n):
        self.__n = n

    def __call__(self, seq, default=None):
        for (i, item) in enumerate(seq):
            if i == self.__n:
                return item
        return default

get_second = GetNth(1)
get_second_or = GetNthOr(1)

def replace_list(thelist, theitems):
    """
    Make a list change its values:

        >>> mylist = list('hello')
        >>> mylist
        ['h', 'e', 'l', 'l', 'o']
        >>> replace_list(mylist, range(7))
        >>> mylist
        [0, 1, 2, 3, 4, 5, 6]

    """
    thelist.clear()
    thelist.extend(theitems)

def count(seq):
    """
    Counts the number of items in a sequence:

        >>> count(range(10))
        10
        >>> count('hello')
        5

    """
    return sum(1 for x in seq)

def singleton(value):
    """
    Converts a value to a 1-tuple.
    This function encapsulates the ugly `(x,)` syntax hack which hampers readability, IMHO.
    """
    return (value,)

def fmtnum(val):
    """
    Formats a number:

        >>> fmtnum(None)
        ''
        >>> fmtnum(0)
        '0'
        >>> fmtnum(42)
        '42'
        >>> fmtnum(-55)
        '-55'
        >>> fmtnum(701**23)
        '282822797185951063895875514717257840657769715720236217123076986101'
        >>> fmtnum(math.nan)
        'NaN'
        >>> fmtnum(math.inf)
        'Infinity'
        >>> fmtnum(-math.inf)
        '-Infinity'
        >>> fmtnum(1.0)
        '1.00000000000000000000E+00'
        >>> fmtnum(math.pi)
        '3.14159265358979311600E+00'
        >>> format(-123456789.0, '.20E')
        '-1.23456789000000000000E+08'
        >>> fmtnum(+0.0)
        '0.00000000000000000000E+00'
        >>> fmtnum(-0.0)
        '-0.00000000000000000000E+00'
        >>> fmtnum(float(701**101))
        '2.61528976769121578284E+287'

    """
    if val is None:
        return ''
    elif isinstance(val, int):
        return format(val, 'd')
    elif isinstance(val, float):
        if math.isnan(val):
            return 'NaN'
        elif math.isinf(val):
            return '-Infinity' if val < 0.0 else 'Infinity'
        else:
            return format(val, '.20E')
    else:
        raise TypeError(type(val))

def encoded(text):
    """
    If `text is None` this function does nothing:

        >>> encoded(None) is None
        True

    But if `text` is a string it is encoded:

        >>> encoded('pizza')
        b'pizza'

    """
    if text is None:
        return None
    elif isinstance(text, str):
        return text.encode()
    else:
        raise TypeError()

def prepare_fingerprint(value):
    if value is None:
        return None
    elif isinstance(value, bytes):
        return value
    elif isinstance(value, str):
        return bytes.fromhex(value)
    else:
        raise TypeError(repr(type(value)))

def rfc5322(dt : datetime.datetime, localtime : bool = True) -> str:
    """
    Formats a datetime object as an RFC 5322 string:

        >>> dt = datetime.datetime(year=1969, month=7, day=21, hour=2, minute=56)
        >>> rfc5322(dt, localtime=False)
        'Mon, 21 Jul 1969 01:56:00 -0000'

    """
    return email.utils.formatdate(dt.timestamp(), localtime=localtime)

class Override(object):

    def __init__(self, parent=None):
        self.__parent = parent

    def __call__(self, method):
        if self.__parent is not None and not hasattr(self.__parent, method.__name__):
            raise AssertionError("Cannot override {!r} in {!r}".format(method.__name__, self.__parent.__name__))
        return method

class Static(object):

    def __init__(self, *args, **kwargs):
        raise AssertionError("This class shall not be instantiated")

def index_projection(index, container):
    """
    Maps an indexing operation onto a sequence.

        >>> list(index_projection(1, [(0, 1, 2), (0, 1), 'abcd']))
        [1, 1, 'b']

    """
    getter = lambda item : item[index]
    return map(getter, container)

def attribute_projection(attribute, container):
    """
    Maps a `getattr` operation onto a sequence.

        >>> list(attribute_projection('real', [0, 0.0, 1.0, 2j, 7]))
        [0, 0.0, 1.0, 0.0, 7]

    """
    getter = lambda item : getattr(item, attribute)
    return map(getter, container)
