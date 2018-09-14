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
    'Child',
    'ET',
    'ET_HAVE_LXML',
    'EtError',
    'HttpError',
    'append_child',
    'format_bool',
    'link_xslt',
    'parse_bool',
    'validate_graph_id',
    'validate_id',
    'validate_layout_id',
]

import html
import http

from ..constants import *

try:
    from lxml import etree as ET
    ET_HAVE_LXML = True
    EtError = ET.Error
except ModuleNotFoundError:
    import xml.etree.ElementTree as ET
    ET_HAVE_LXML = False
    EtError = ET.ParseError

def append_child(parent, tagname, **kwargs):
    element = ET.SubElement(parent, tagname)
    for (name, value) in kwargs.items():
        attrname = name.lstrip('_').replace('_', '-')
        element.set(attrname, value)
    return element

class Child(object):

    def __init__(self, parent, tagname, **kwargs):
        self.__element = append_child(parent, tagname, **kwargs)

    def __enter__(self, *args):
        return self.__element

    def __exit__(self, *args):
        pass

def link_xslt(root : ET.Element, href : str = None):
    if not ET_HAVE_LXML:
        raise NotImplementedError("Please install the LXML package")
    pseudoattrs = { 'type' : 'text/xsl', 'href' : href }
    value = ' '.join("{:s}='{:s}'".format(k, html.escape(v, quote=True)) for (k, v) in pseudoattrs.items())
    root.addprevious(ET.ProcessingInstruction('xml-stylesheet', value))

class _NoXslt(object):

    def __init__(self, *args, **kwargs):
        raise NotImplementedError("Please install the LXML package")

if not hasattr(ET, 'XSLT'):
    setattr(ET, 'XSLT', _NoXslt)

_BOOL_LITERALS_TRUE = frozenset([ 'true', 'yes', 'on' ])
_BOOL_LITERALS_FALSE = frozenset([ 'false', 'no', 'off' ])

def parse_bool(text : str):
    canonical = text.lower().strip()
    if canonical in _BOOL_LITERALS_TRUE:
        return True
    if canonical in _BOOL_LITERALS_FALSE:
        return False
    number = int(text)
    if number < 0:
        raise ValueError(text)
    return number > 0

def format_bool(value : bool):
    return str(int(bool(value)))

def validate_id(token, what="graph or layout"):
    try:
        theid = Id(token)
        if theid: return theid
    except ValueError:
        pass
    msg = "The string {!r} cannot possibly be a valid {:s} ID.".format(token, what)
    raise HttpError(http.HTTPStatus.BAD_REQUEST, msg)

def validate_graph_id(token):
    return validate_id(token, what="graph")

def validate_layout_id(token):
    return validate_id(token, what="layout")

class HttpError(Exception):

    def __init__(self, code : int, text : str = None):
        self.__code = code
        self.__text = text

    @property
    def code(self):
        return self.__code

    @property
    def what(self):
        return self.__text
