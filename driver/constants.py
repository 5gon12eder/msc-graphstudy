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

import binascii
import enum
import itertools
import logging
import math
import os

__all__ = [
    # constants
    'FIXED_COUNT_BINS',
    'FIXED_COUNT_BINS_LOG_2',
    'GRAPH_FILE_SUFFIX',
    'HUANG_METRICS',
    'LAYOUTS_FALLBACK_COLORS',
    'LAYOUTS_PREFERRED_COLORS',
    'LAYOUT_FILE_SUFFIX',
    'PRIMARY_COLOR',
    'PROPERTY_QUANTITIES',
    'SECONDARY_COLOR',
    'TANGO_COLORS',
    'VICINITIES',
    'VICINITIES_LOG_2',
    # types
    'Actions',
    'Binnings',
    'Generators',
    'GraphSizes',
    'Id',
    'LayInter',
    'LayWorse',
    'Layouts',
    'LogLevels',
    'Properties',
    'Kernels',
    'Metrics',
    'Tests',
    'TestStatus',
    # functions
    'enum_from_json',
    'enum_to_json',
    'sqlrepr',
    'translate_enum_name',
]

GRAPH_FILE_SUFFIX = '.xml.gz'
LAYOUT_FILE_SUFFIX = '.xml.gz'

TANGO_COLORS = {
    'Butter'      : (0xFCE94F, 0xEDD400, 0xC4A000),
    'Orange'      : (0xFCAF3E, 0xF57900, 0xCE5C00),
    'Chocolate'   : (0xE9B96E, 0xC17D11, 0x8F5902),
    'Chameleon'   : (0x8AE234, 0x73D216, 0x4E9A06),
    'Sky Blue'    : (0x729FCF, 0x3465A4, 0x204A87),
    'Plum'        : (0xAD7FA8, 0x75507B, 0x5C3566),
    'Scarlet Red' : (0xEF2929, 0xCC0000, 0xA40000),
    'Aluminium'   : (0xEEEEEC, 0xD3D7CF, 0xBAbDB6, 0x888A85, 0x555753, 0x2E3436),
}

PRIMARY_COLOR = TANGO_COLORS['Chameleon'][1]
SECONDARY_COLOR = TANGO_COLORS['Scarlet Red'][1]

FIXED_COUNT_BINS_LOG_2 = list(range(3, 10))
FIXED_COUNT_BINS = [ 2**i for i in FIXED_COUNT_BINS_LOG_2 ]

VICINITIES_LOG_2 = list(range(10))
VICINITIES = [ 2**i for i in VICINITIES_LOG_2 ]

class Id(object):

    r"""

    A "default constructed" ID object represents no ID.

        >>> Id() == Id()
        True
        >>> Id() != Id()
        False
        >>> bool(Id())
        False
        >>> len(Id())
        0
        >>> str(Id())
        ''
        >>> repr(Id())
        'Id()'
        >>> Id().data
        b''

    IDs can be constructed from any byte sequence ...

        >>> Id(b'hello')
        Id(68656c6c6f)
        >>> bool(Id(b'hello'))
        True
        >>> len(Id(b'hello'))
        5
        >>> str(Id(b'hello'))
        '68656c6c6f'
        >>> repr(Id(b'hello'))
        'Id(68656c6c6f)'

    ... or a hex string:

        >>> Id('deadbeef')
        Id(deadbeef)
        >>> Id('deadbeef') == Id('DEAD BEEF')
        True

    IDs are equal if and only if they refer to the same byte sequence.

        >>> Id('cafe') == Id(b'\xCA\xFE')
        True
        >>> Id('cafe') != Id(b'\xCA\xFE')
        False
        >>> Id('cafe') == Id('cafe00')
        False
        >>> Id('cafe') != Id('cafe00')
        True

    """

    def __init__(self, token=None):
        if token is None:
            self.__id = b''
        elif isinstance(token, bytes):
            self.__id = bytes(token)
        elif isinstance(token, str):
            self.__id = bytes.fromhex(token)
        else:
            raise TypeError("IDs can only be constructed from 'str' and 'bytes' objects")

    def __bool__(self):
        return bool(self.__id)

    def __len__(self):
        return len(self.__id)

    def __str__(self):
        return binascii.hexlify(self.__id).decode('ascii')

    def __bytes__(self):
        return self.__id

    def __repr__(self):
        return 'Id({!s})'.format(self)

    def __eq__(self, other):
        return type(other) is type(self) and self.__id == other.__id

    def __ne__(self, other):
        return not (self == other)

    def __hash__(self):
        return hash(self.__id)

    @property
    def data(self):
        return self.__id

    def getkey(self):
        """
        Use this as a sort key if you need a list of IDs to be in deterministic order.

            >>> ids = [ Id('4a'), Id('fb'), Id('7d'), Id('6f'), Id('19') ]
            >>> sorted(ids, key=Id.getkey)
            [Id(19), Id(4a), Id(6f), Id(7d), Id(fb)]

        """
        return self.__id

class Actions(enum.IntEnum):

    GRAPHS     = 10
    LAYOUTS    = 20
    LAY_WORSE  = 30
    LAY_INTER  = 31
    PROPERTIES = 40
    METRICS    = 45
    MODEL      = 50

class Binnings(enum.IntEnum):

    NONE                   = 0
    FIXED_WIDTH            = 1
    FIXED_COUNT            = 2
    SCOTT_NORMAL_REFERENCE = 3

class Generators(enum.IntEnum):

    # Imports from NIST Matrix Market
    SMTAPE      = -23
    PSADMIT     = -22
    GRENOBLE    = -21
    BCSPWR      = -20
    # Imports from Graph Drawing
    RANDDAG     = -12
    NORTH       = -11
    ROME        = -10
    # Generic Imports
    IMPORT      =   0
    # Non-Import Generators
    LINDENMAYER =  10
    QUASI3D     =  23
    QUASI4D     =  24
    QUASI5D     =  25
    QUASI6D     =  26
    GRID        =  30
    TORUS1      =  31
    TORUS2      =  32
    MOSAIC1     =  41
    MOSAIC2     =  42
    BOTTLE      =  50
    TREE        =  60
    RANDGEO     =  70

    @property
    def imported(self):
        return self.value <= 0

class GraphSizes(enum.IntEnum):

    TINY   = 1
    SMALL  = 2
    MEDIUM = 3
    LARGE  = 4
    HUGE   = 5

    @property
    def low_end(self):
        """
        Get the smallest size of a graph falling into this size category:

            >>> GraphSizes.MEDIUM.low_end
            100

        """
        return _GRAPH_SIZE_ATTRIBUTES[self].lower

    @property
    def high_end(self):
        """
        Get the smallest size of a graph /not/ falling into this size category:

            >>> GraphSizes.MEDIUM.high_end
            1000

        """
        return _GRAPH_SIZE_ATTRIBUTES[self].upper

    @property
    def target(self):
        """
        Get the target size of a graph falling into this size category.  This is the size to ask a generator for when a
        graph of this size category is desired.

            >>> GraphSizes.MEDIUM.target
            433
            >>> GraphSizes.MEDIUM.low_end < GraphSizes.MEDIUM.target < GraphSizes.MEDIUM.high_end
            True

        """
        return _GRAPH_SIZE_ATTRIBUTES[self].target

    @classmethod
    def classify(cls, nodes):
        """
        Classify a graph according to its number of nodes.

            >>> GraphSizes.classify(300).name
            'MEDIUM'

        """
        assert isinstance(nodes, int) and nodes >= 0
        for z in reversed(cls):
            if nodes >= z.low_end:
                return z

class _GraphSizeAttributes(object):

    def __init__(self, lower, upper):
        self.lower = lower
        self.upper = upper

    @property
    def target(self):
        return round(((math.sqrt(self.lower) + math.sqrt(self.upper)) / 2)**2)

_GRAPH_SIZE_ATTRIBUTES = {
    GraphSizes.TINY   : _GraphSizeAttributes (      0,     10 ),
    GraphSizes.SMALL  : _GraphSizeAttributes (     10,    100 ),
    GraphSizes.MEDIUM : _GraphSizeAttributes (    100,   1000 ),
    GraphSizes.LARGE  : _GraphSizeAttributes (   1000, 100000 ),
    GraphSizes.HUGE   : _GraphSizeAttributes ( 100000,   None ),
}

assert all(z in _GRAPH_SIZE_ATTRIBUTES for z in GraphSizes)

class Layouts(enum.IntEnum):

    # native
    NATIVE              =  0
    # proper
    FMMM                = 10
    STRESS              = 11
    DAVIDSON_HAREL      = 12
    SPRING_EMBEDDER_KK  = 13
    PIVOT_MDS           = 14
    SUGIYAMA            = 21
    # garbage
    RANDOM_UNIFORM      =  -1
    RANDOM_NORMAL       =  -2
    PHANTOM             = -10

    @property
    def garbage(self):
        return self.value < 0

    @property
    def proper(self):
        return not self.garbage

class LayInter(enum.IntEnum):

    LINEAR         = 1
    XLINEAR        = 2

class LayWorse(enum.IntEnum):

    FLIP_NODES     = 1
    FLIP_EDGES     = 2
    MOVLSQ         = 3
    PERTURB        = 4

LAYOUTS_PREFERRED_COLORS = {
    Layouts.NATIVE             : (TANGO_COLORS['Scarlet Red'][1], TANGO_COLORS['Scarlet Red'][2]),
    Layouts.FMMM               : (TANGO_COLORS['Sky Blue'   ][1], TANGO_COLORS['Sky Blue'   ][2]),
    Layouts.STRESS             : (TANGO_COLORS['Sky Blue'   ][1], TANGO_COLORS['Sky Blue'   ][2]),
    Layouts.DAVIDSON_HAREL     : (TANGO_COLORS['Sky Blue'   ][1], TANGO_COLORS['Sky Blue'   ][2]),
    Layouts.SPRING_EMBEDDER_KK : (TANGO_COLORS['Sky Blue'   ][1], TANGO_COLORS['Sky Blue'   ][2]),
    Layouts.PIVOT_MDS          : (TANGO_COLORS['Sky Blue'   ][1], TANGO_COLORS['Sky Blue'   ][2]),
    Layouts.SUGIYAMA           : (TANGO_COLORS['Plum'       ][1], TANGO_COLORS['Plum'       ][2]),
    Layouts.PHANTOM            : (TANGO_COLORS['Chocolate'  ][1], TANGO_COLORS['Chocolate'  ][2]),
    Layouts.RANDOM_UNIFORM     : (TANGO_COLORS['Chocolate'  ][1], TANGO_COLORS['Chocolate'  ][2]),
    Layouts.RANDOM_NORMAL      : (TANGO_COLORS['Chocolate'  ][1], TANGO_COLORS['Chocolate'  ][2]),
}

LAYOUTS_FALLBACK_COLORS = (TANGO_COLORS['Chameleon'][1], TANGO_COLORS['Chameleon'][2])

assert all(l in LAYOUTS_PREFERRED_COLORS for l in Layouts)

class Properties(enum.IntEnum):

    RDF_GLOBAL  = 1
    RDF_LOCAL   = 2
    ANGULAR     = 3
    EDGE_LENGTH = 4
    PRINCOMP1ST = 5
    PRINCOMP2ND = 6
    TENSION     = 7

    @property
    def localized(self):
        return self in _LOCALIZED_PROPERTIES

_LOCALIZED_PROPERTIES = frozenset([Properties.RDF_LOCAL])

class Kernels(enum.IntEnum):

    RAW      = 0
    BOXED    = 1
    GAUSSIAN = 2

PROPERTY_QUANTITIES = {
    Properties.ANGULAR     : ('rad', "angle between adjacent edges"),
    Properties.RDF_GLOBAL  : ('%', "node distance divided by average edge length"),
    Properties.RDF_LOCAL   : ('%', "node distance divided by average edge length"),
    Properties.EDGE_LENGTH : ('%', "edge length divided by average edge length"),
    Properties.PRINCOMP1ST : ('%', "node position divided by average edge length"),
    Properties.PRINCOMP2ND : ('%', "node position divided by average edge length"),
    Properties.TENSION     : ('%', "Euclidian layout distance divided by graph-theoretical distance"),
}

class Metrics(enum.IntEnum):

    STRESS_KK          = 10
    STRESS_FIT_NODESEP = 11
    STRESS_FIT_SCALE   = 12
    CROSS_COUNT        = 20
    CROSS_RESOLUTION   = 30
    ANGULAR_RESOLUTION = 40
    EDGE_LENGTH_STDEV  = 50

HUANG_METRICS = { Metrics.CROSS_COUNT, Metrics.CROSS_RESOLUTION, Metrics.ANGULAR_RESOLUTION, Metrics.EDGE_LENGTH_STDEV }

class Tests(enum.IntEnum):

    EXPECTED           =  0
    NN_FORWARD         =  1
    NN_REVERSE         =  2
    STRESS_KK          = 10
    STRESS_FIT_NODESEP = 11
    STRESS_FIT_SCALE   = 12
    HUANG              = 20

    def is_alternative(self):
        return self.value >= 10

class TestStatus(enum.IntEnum):

    UNKNOWN        =  0
    TRUE_POSITIVE  = +1
    TRUE_NEGATIVE  = +2
    FALSE_POSITIVE = -1
    FALSE_NEGATIVE = -2

class LogLevels(enum.IntEnum):

    DEBUG     = logging.DEBUG
    INFO      = logging.INFO
    NOTICE    = logging.INFO + 5       # NOTICE is not defined by the logging module
    WARNING   = logging.WARNING
    ERROR     = logging.ERROR
    CRITICAL  = logging.CRITICAL
    ALERT     = logging.CRITICAL + 10  # ALERT is not defined by the logging module
    EMERGENCY = logging.CRITICAL + 20  # EMERGENCY is not defined by the logging module

    @classmethod
    def parse(cls, name, errcls=ValueError):
        """
        Logging levels can be selected by their well-known name.

            >>> LogLevels.parse('INFO')
            <LogLevels.INFO: 20>

        Leading and trailing white-space is insignificant ...

            >>> LogLevels.parse('  Warning ')
            <LogLevels.WARNING: 30>

        ... and so is case.

           >>> LogLevels.parse('alerT')
           <LogLevels.ALERT: 60>

        Abbrevation is acceptable ...

            >>> LogLevels.parse('EMERG')
            <LogLevels.EMERGENCY: 70>

        ... as long as the result is not ambiguous.

            >>> LogLevels.parse('E')
            Traceback (most recent call last):
            ValueError: Ambiguous logging level 'E' matches multiple names: ERROR, EMERGENCY

        If an invalid argument is passed, a `ValueError` will be raised.

            >>> LogLevels.parse('Chocolate')
            Traceback (most recent call last):
            ValueError: Unknown logging level: 'Chocolate'

        The exception class can be customized via the `errcls` parameter.

            >>> LogLevels.parse('Chocolate', errcls=RuntimeError)
            Traceback (most recent call last):
            RuntimeError: Unknown logging level: 'Chocolate'

        """
        canonical = name.strip().upper()
        candidates = [level for level in cls if level.name.startswith(canonical)]
        if not candidates:
            raise errcls("Unknown logging level: {!r}".format(name))
        elif len(candidates) > 1:
            matches = ', '.join(level.name for level in candidates)
            raise errcls("Ambiguous logging level {!r} matches multiple names: ".format(name) + matches)
        else:
            return candidates[0]

# We have hard-coded the intervals used by the logging module so better make sure we got this right.
assert list(LogLevels) == sorted(LogLevels)

def enum_from_json(cls, token):
    """
    String constants as they are used in the tool's JSON output can be parsed to enumeration constants:

        >>> enum_from_json(Binnings, 'fixed-count')
        <Binnings.FIXED_COUNT: 2>

    For invalid inputs, a `ValueError` will be raised:

        >>> enum_from_json(Binnings, '2nd-line')
        Traceback (most recent call last):
        ValueError: Enumerator 'Binnings' has no value '2nd-line'

    `None` is an acceptable input if and only if the enum has a zero value:

        >>> enum_from_json(Binnings, None)
        <Binnings.NONE: 0>
        >>> enum_from_json(GraphSizes, None)
        Traceback (most recent call last):
        ValueError: Enumerator 'GraphSizes' has no default value

    """
    assert issubclass(cls, enum.Enum)
    if token is None:
        try:
            return cls(0)
        except ValueError:
            raise ValueError("Enumerator {!r} has no default value".format(cls.__name__))
    else:
        try:
            return cls['_'.join(token.upper().split('-'))]
        except KeyError:
            raise ValueError("Enumerator {!r} has no value {!r}".format(cls.__name__, token))

def enum_to_json(const):
    """
    This is the reversed operation of `enum_from_json`.

        >>> enum_to_json(Generators.ROME)
        'rome'
        >>> enum_to_json(Binnings.SCOTT_NORMAL_REFERENCE)
        'scott-normal-reference'

    """
    assert isinstance(const, enum.Enum)
    return '-'.join(const.name.lower().split('_'))

def translate_enum_name(name, old : str = None, new : str = None, case : str = None):
    """
        >>> translate_enum_name('fancy-dancy', old='-', new='_', case='upper')
        'FANCY_DANCY'

        >>> translate_enum_name('FANCY_DANCY', old='_', new='-', case='lower')
        'fancy-dancy'

    """
    assert old in [ '_', '-' ]
    assert new in [ '_', '-' ]
    assert case in [ 'lower', 'upper' ]
    return new.join(map(getattr(str, case), filter(None, map(str.strip, name.split(old)))))

def sqlrepr(x):
    if x is None: return 'NULL'
    elif isinstance(x, enum.Enum): return x.name
    elif isinstance(x, Id): return "X'{!s}'".format(x)
    else: return repr(x)
