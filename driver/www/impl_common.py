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
    'append_common_all_constants',
    'append_common_graph',
    'append_common_property',
    'get_bool_from_query',
    'get_int_from_query',
    'get_informal_layout_name',
    'get_informal_layout_name_curs',
]

from . import *
from ..constants import *
from ..utility import *

_ENUM_TAG_NAMES = {
    Actions    : ('all-actions',    'action'),
    Binnings   : ('all-binnings',   'binning'),
    Generators : ('all-generators', 'generator'),
    GraphSizes : ('all-sizes',      'size'),
    Layouts    : ('all-layouts',    'layout'),
    LayInter   : ('all-lay-inter',  'lay-inter'),
    LayWorse   : ('all-lay-worse',  'lay-worse'),
    Properties : ('all-properties', 'property'),
    Metrics    : ('all-metrics',    'metric'),
    Tests      : ('all-tests',      'test'),
    TestStatus : ('all-status',     'status'),
}

_ENUM_EXTRA_ATTRS = {
    Properties : [ ('localized', lambda const : format_bool(const.localized)) ],
}

def append_common_all_constants(parent, *enumtypes):
    for enumtype in enumtypes:
        (ptag, ctag) = _ENUM_TAG_NAMES[enumtype]
        with Child(parent, ptag) as node:
            for const in enumtype:
                with Child(node, ctag, key=str(const.value)) as child:
                    for (attr, getter) in _ENUM_EXTRA_ATTRS.get(enumtype, []):
                        child.set(attr, getter(const))
                    child.text = const.name

def append_common_graph(parent, row, tag='graph'):
    gen = row['generator']
    size = GraphSizes.classify(row['nodes'])
    node = append_child(parent, tag, id=str(row['id'])) if tag is not None else parent
    append_child(node, 'generator').text = gen.name
    append_child(node, 'size').text = size.name
    append_child(node, 'nodes').text = fmtnum(row['nodes'])
    append_child(node, 'edges').text = fmtnum(row['edges'])

def append_common_property(prop, parent, rows, tagname='property-data'):
    commonkeys = { 'size', 'minimum', 'maximum', 'mean', 'rms' }
    if not rows:
        return
    if prop.localized:
        raise NotImplementedError("Sorry, I cannot handle this")
    with Child(parent, tagname, property=prop.name, kind=kind.name) as child:
        for row in rows:
            with Child(child, 'histogram') as histo:
                binning = row['binning']
                append_child(histo, 'binning').text = binning.name
                for key in commonkeys | { 'bincount', 'binwidth' }:
                    append_child(histo, key).text = fmtnum(row[key])

def get_bool_from_query(query, key, default=False):
    try:
        [ value ] = query[key]
    except KeyError:
        return default
    except ValueError:
        raise HttpError(http.HTTPStatus.BAD_REQUEST, "Parameter {!r} must be passed at most once.".format(key))
    try:
        return parse_bool(value)
    except ValueError:
        raise HttpError(http.HTTPStatus.BAD_REQUEST, "Parameter {!r} must be set to a boolean.".format(key))

def get_int_from_query(query, key, default=None, negative=False, positive=False):
    try:
        [ value ] = query[key]
    except KeyError:
        return default
    except ValueError:
        raise HttpError(http.HTTPStatus.BAD_REQUEST, "Parameter {!r} must be passed at most once.".format(key))
    try:
        value = int(value)
    except ValueError:
        raise HttpError(http.HTTPStatus.BAD_REQUEST, "Parameter {!r} must be set to an integer.".format(key))
    if value <= 0 and positive:
        raise HttpError(http.HTTPStatus.BAD_REQUEST, "Parameter {!r} must be a positive integer.".format(key))
    if value < 0 and not negative:
        raise HttpError(http.HTTPStatus.BAD_REQUEST, "Parameter {!r} must be a non-negative integer.".format(key))
    return value

def get_informal_layout_name(mngr, layoutid) -> str:
    with mngr.sql_ctx as curs:
        return get_informal_layout_name_curs(mngr, curs, layoutid)

def get_informal_layout_name_curs(mngr, curs, layoutid) -> str:
    assert isinstance(layoutid, Id)
    proper = get_one(mngr.sql_select_curs(curs, 'Layouts', id=layoutid))['layout']
    if proper is not None:
        return proper.name
    for row in mngr.sql_select_curs(curs, 'InterLayouts', id=layoutid):
        name1 = get_informal_layout_name_curs(mngr, curs, row['parent1st'])
        name2 = get_informal_layout_name_curs(mngr, curs, row['parent2nd'])
        return "({:s}){:d}:{:s}({:s})".format(name1, round(100.0 * row['rate']), row['method'].name, name2)
    for row in mngr.sql_select_curs(curs, 'WorseLayouts', id=layoutid):
        name = get_informal_layout_name_curs(mngr, curs, row['parent'])
        return "({:s})+{:d}:{:s}".format(name, round(100.0 * row['rate']), row['method'].name)
    return "???"
