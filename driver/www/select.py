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

__all__ = [ 'serve' ]

import enum
import http
import re
import urllib.parse

from . import *
from ..constants import *
from ..utility import *
from .impl_common import *

_VALID_SQL_TABLE_NAME_PATTERN = re.compile(r'^[_]?[A-Z][A-Za-z0-9]*$')

def serve(self, url):
    all_tables = frozenset(map(get_one, self.server.graphstudy_manager.sql_exec(
        "SELECT `name` FROM `sqlite_master` WHERE type = 'table'", tuple()
    )))
    all_views = frozenset(map(get_one, self.server.graphstudy_manager.sql_exec(
        "SELECT `name` FROM `sqlite_master` WHERE type = 'view'", tuple()
    )))
    all_tables_and_views = all_tables | all_views
    assert all(_VALID_SQL_TABLE_NAME_PATTERN.match(tab) for tab in all_tables_and_views)
    query = urllib.parse.parse_qs(url.query)
    onlycount = get_bool_from_query(query, 'count')
    selected_tables = query.get('table') or list()
    selected_tables = all_tables if '*' in selected_tables else frozenset(selected_tables)
    for tab in selected_tables:
        if not _VALID_SQL_TABLE_NAME_PATTERN.match(tab):
            raise HttpError(http.HTTPStatus.BAD_REQUEST, "SQL injection attacks are not supported.")
        if tab not in all_tables_and_views:
            raise HttpError(http.HTTPStatus.NOT_FOUND, "No table or view {!r} exists in the database.".format(tab))
    n = get_int_from_query(query, 'rows', default=(100 if len(selected_tables) == 1 else 10))
    root = ET.Element('root')
    with Child(root, 'all-tables') as at:
        for tab in sorted(all_tables):
            append_child(at, 'table').text = tab
        for tab in sorted(all_views):
            append_child(at, 'view').text = tab
    if selected_tables:
        with Child(root, 'query-results') as qr:
            for tab in sorted(selected_tables):
                with self.server.graphstudy_manager.sql_ctx as curs:
                    rowcount = get_one(self.server.graphstudy_manager.sql_exec_curs(
                        curs, "SELECT COUNT() FROM `{:s}`".format(tab), tuple()
                    ))[0]
                    k = min(1024, max(1, round(n * 1024 / rowcount))) if rowcount > 0 else 1024
                    somerows = self.server.graphstudy_manager.sql_exec_curs(
                        curs, "SELECT * FROM `{:s}` WHERE ABS(RANDOM()) % 1024 < ? LIMIT ?".format(tab), (k, n)
                    ) if not onlycount else None
                with Child(qr, 'table', name=tab, rows=fmtnum(rowcount)) as tb:
                    if somerows:
                        with Child(tb, 'columns') as co:
                            for key in somerows[0].keys():
                                typ = _get_type(somerows, key)
                                if typ:
                                    append_child(co, 'column', _type=typ).text = key
                                else:
                                    append_child(co, 'column').text = key
                    if somerows:
                        for row in somerows:
                            with Child(tb, 'row') as ro:
                                for key in row.keys():
                                    _add_row_value(ro, row[key])
    self.send_tree_xml(ET.ElementTree(root), transform='/xslt/select.xsl')

def _add_row_value(xmlrow, value):
    if value is None:
        append_child(xmlrow, 'value', null='true')
    elif isinstance(value, Id):
        append_child(xmlrow, 'value').text = str(value)
    elif isinstance(value, enum.IntEnum):
        append_child(xmlrow, 'value', key=str(value.value)).text = value.name
    elif isinstance(value, bool):
        append_child(xmlrow, 'value', key=str(int(value))).text = str(value).upper()
    elif isinstance(value, int) or isinstance(value, float):
        append_child(xmlrow, 'value').text = fmtnum(value)
    elif isinstance(value, str):
        append_child(xmlrow, 'value').text = value
    elif isinstance(value, bytes):
        append_child(xmlrow, 'value').text = ''.join(
            c if ord(' ') <= ord(c) <= ord('~') else '\ufffd' for c in value.decode('ascii', errors='replace')
        )
    else:
        raise TypeError()

def _get_type(rows, key):
    value = next(filter(lambda x : x is not None, map(lambda r : r[key], rows)), None)
    if isinstance(value, str):            return 's'
    elif isinstance(value, bool):         return 'd'
    elif isinstance(value, int):          return 'd'
    elif isinstance(value, float):        return 'f'
    elif isinstance(value, enum.IntEnum): return 'd'
    elif isinstance(value, Id):           return 's'
    else:                                 return None
