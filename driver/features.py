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

__all__ = [ 'get_graph_features', 'get_layout_features' ]

import collections
import math

from .constants import *
from .utility import *

_NAMESEP = ':'
_NIL = [ None ]

_USE_OUTER_SIZE = False

_OUTER_COLUMNS_COMMON = [ 'mean', 'rms' ]
_OUTER_COLUMNS_EXTRA_DISC = [ 'entropyIntercept', 'entropySlope' ]
_OUTER_COLUMNS_EXTRA_CONT = [ ]

_INNER_COLUMNS_DISC = [ ]
_INNER_COLUMNS_CONT = [ 'entropy' ]

_PROPERTY_ALIASES = { p : ''.join(map(str.capitalize, p.name.split('_'))) for p in Properties }
_PROPERTY_ALIASES[Properties.PRINCOMP1ST] = 'PrinComp1st'
_PROPERTY_ALIASES[Properties.PRINCOMP2ND] = 'PrinComp2nd'
assert all(p in _PROPERTY_ALIASES for p in Properties)

def get_graph_features(mngr, curs, graphid, verbose=False, na=None):
    return _convert_result(_emit_graph_features(mngr, curs, graphid, na=na), verbose=verbose)

def get_layout_features(mngr, curs, layoutid, propsdisc=None, propscont=None, puncture=None, verbose=False, na=None):
    assert (propsdisc is None) == (propscont is None)
    if propsdisc is None and propscont is None:
        propsdisc = frozenset(mngr.config.desired_properties_disc.keys())
        propscont = frozenset(mngr.config.desired_properties_cont.keys())
    if puncture is None:
        puncture = mngr.config.puncture
    return _convert_result(_emit_layout_features(
        mngr, curs, layoutid=layoutid, propsdisc=propsdisc, propscont=propscont, puncture=puncture, na=na
    ), verbose=verbose)

def _convert_result(sequence, verbose=False):
    return collections.OrderedDict(sequence) if verbose else list(map(get_second, sequence))

def _emit_graph_features(mngr, curs, graphid, na=None):
    row = get_one_or(mngr.sql_select_curs(curs, 'Graphs', id=graphid))
    yield ('logNodes', _ilog(row['nodes']) if row is not None else na)
    yield ('logEdges', _ilog(row['edges']) if row is not None else na)

def _emit_layout_features(mngr, curs, layoutid, propsdisc, propscont, puncture, na=None):
    get_wrapper = lambda prop : (lambda x : na) if prop in puncture else (lambda x : x)
    for (prop, what) in [ (Properties.PRINCOMP1ST, 'major'), (Properties.PRINCOMP2ND, 'minor') ]:
        if prop in propsdisc or prop in propscont:
            __ = get_wrapper(prop)
            (singular, plural) = ( what.capitalize() + suffix for suffix in [ 'Axis', 'Axes' ] )
            row = get_one_or(mngr.sql_select_curs(curs, plural, layout=layoutid))
            yield (singular + _NAMESEP + 'x', __(row['x'] if row is not None else na))
            yield (singular + _NAMESEP + 'y', __(row['y'] if row is not None else na))
    for (prop, __) in map(lambda p : (p, get_wrapper(p)), sorted(Properties)):
        for kind in '' + ('D' if prop in propsdisc else '') + ('C' if prop in propscont else ''):
            (table1, table2) = {
                'D' : ('PropertiesDisc', 'Histograms'),
                'C' : ('PropertiesCont', 'SlidingAverages')
            }[kind]
            lastvalsouter = dict()
            for vicinity in VICINITIES if prop.localized else _NIL:
                alias = _NAMESEP.join(filter(None, [ _PROPERTY_ALIASES[prop], str(value_or(vicinity, '')), kind ]))
                row = get_one_or(mngr.sql_select_curs(curs, table1, layout=layoutid, property=prop, vicinity=vicinity))
                if _USE_OUTER_SIZE:
                    yield (alias + _NAMESEP + 'logSize', __(_ilog(row['size']) if row is not None else na))
                columns = list(_OUTER_COLUMNS_COMMON)
                if kind == 'D': columns.extend(_OUTER_COLUMNS_EXTRA_DISC)
                if kind == 'C': columns.extend(_OUTER_COLUMNS_EXTRA_CONT)
                for col in columns:
                    if prop is Properties.EDGE_LENGTH and col == 'mean': continue
                    name = alias + _NAMESEP + col
                    yield (name, __(row[col] if row is not None else na))
                lastvalsinner = dict(lastvalsouter)
                for scale in { 'D' : FIXED_COUNT_BINS, 'C' : _NIL }[kind]:
                    kwargs = {
                        'layout'   : layoutid,
                        'property' : prop,
                        'vicinity' : vicinity,
                    }
                    if kind == 'D':
                        kwargs['binning'] = Binnings.FIXED_COUNT
                        kwargs['bincount'] = scale
                    shrow = get_one_or(mngr.sql_select_curs(curs, table2, **kwargs))
                    shcols = { 'D' : _INNER_COLUMNS_DISC, 'C' : _INNER_COLUMNS_CONT }[kind]
                    for shcol in shcols:
                        shname = alias + _NAMESEP + shcol
                        if scale is not None: shname += _NAMESEP + str(scale)
                        shval = None if shrow is None else shrow[shcol]
                        if shval is not None: lastvalsinner[shcol] = shval
                        yield (shname, __(lastvalsinner.get(shcol, na)))
                for (k, v) in lastvalsinner.items():
                    if v is not None: lastvalsouter[k] = v

def _ilog(n : int) -> float:
    return math.log(max(1.0 / math.e, n))
