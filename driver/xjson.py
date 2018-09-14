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
    'XJsonError',
    'load_xjson_file',
    'load_xjson_string',
]

import json
import logging

from .errors import *

class XJsonError(Error):

    def __init__(self, filename=None):
        if filename is not None:
            super().__init__("Cannot read / decode JSON file {!r}".format(filename))
        else:
            super().__init__("Cannot read / decode JSON file")

def load_xjson_file(source):
    if isinstance(source, str):
        try:
            with open(source, 'r') as istr:
                return load_xjson_file(istr)
        except (OSError, UnicodeDecodeError) as e:
            logging.error("{:s}: {!s}".format(source, e))
            raise XJsonError()
    filename = getattr(source, 'name', '/dev/stdin')
    try:
        return _load_xjson_pp(map(_preprocess_line, source), filename=filename)
    except (OSError, UnicodeDecodeError) as e:
            logging.error("{:s}: {!s}".format(filename, e))
            raise XJsonError()
    return _load_xjson_ppline(pplines, filename=filename)

def load_xjson_string(text, filename=None):
    return _load_xjson_pp(map(_preprocess_line, text.splitlines()), filename=filename)

def _load_xjson_pp(ppiter, filename=None):
    jsontext = '\n'.join(ppiter)
    if all(map(str.isspace, jsontext)):
        return None
    try:
        return json.loads(jsontext)
    except json.JSONDecodeError as e:
        filename = filename if filename is not None else '/dev/stdin'
        logging.error("{:s}:{:d}:{:d}: JSON syntax error: {!s}".format(filename, e.lineno, e.colno, e.msg))
        raise XJsonError(filename=filename)

def _preprocess_line(line):
    (prefix, slashes, suffix) = line.partition('//')
    return '' if slashes == '//' and not prefix.strip() else line.rstrip()
