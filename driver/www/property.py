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

import http
import logging
import urllib.parse

from . import *
from ..constants import *
from ..utility import *
from .impl_common import *

def serve(this, graphid, property, vicinity, url=Ellipsis):
    assert (vicinity is None) != (property.localized)
    query = urllib.parse.parse_qs(url.query)
    try:
        selected_layouts = frozenset(validate_layout_id(token) for token in query['layouts'])
    except KeyError:
        selected_layouts = None
    with this.server.graphstudy_manager.sql_ctx as curs:
        graphrow = get_one_or(this.server.graphstudy_manager.sql_select_curs(curs, 'Graphs', id=graphid))
        if not graphrow:
            raise HttpError(http.HTTPStatus.NOT_FOUND, "No graph with ID {!s} exists.".format(graphid))
        root = ET.Element('root')
        with Child(root, 'property') as node:
            if property.localized:
                node.set('vicinity', fmtnum(vicinity))
            node.text = property.name
        append_common_graph(root, graphrow)
        append_common_all_constants(root, Layouts, Properties, Binnings)
        _append_scaled_histogram_stuff(
            this.server.graphstudy_manager, curs, graphid, selected_layouts, property, vicinity, root
        )
    this.send_tree_xml(ET.ElementTree(root), transform='/xslt/property.xsl')

def _append_scaled_histogram_stuff(mngr, curs, graphid, selected_layouts, prop, vicinity, root):
    with Child(root, 'all-bincounts') as node:
        for val in FIXED_COUNT_BINS:
            append_child(node, 'bincount').text = fmtnum(val)
    with Child(root, 'all-vicinities') as node:
        for val in VICINITIES:
            append_child(node, 'vicinity').text = fmtnum(val)
    with Child(root, 'layouts') as los:
        all_layout_ids = set()
        query = "SELECT * FROM `Layouts` WHERE `graph` = ? AND `layout` NOTNULL ORDER BY `layout`"
        for layoutrow in mngr.sql_exec_curs(curs, query, singleton(graphid)):
            layoutid = layoutrow['id']
            all_layout_ids.add(layoutid)
            kwargs = { 'layout' : layoutid, 'property' : prop, 'vicinity' : vicinity }
            with Child(los, 'layout-data', id=str(layoutid), layout=layoutrow['layout'].name) as lo:
                for row in mngr.sql_select_curs(curs, 'Histograms', **kwargs, binning=Binnings.SCOTT_NORMAL_REFERENCE):
                    attrs = { 'bincount' : fmtnum(row['bincount']), 'binning' : Binnings.SCOTT_NORMAL_REFERENCE.name }
                    append_child(lo, 'entropy', **attrs).text = fmtnum(row['entropy'])
                entropy_fixed = {
                    r['bincount'] : r['entropy'] for r in
                    mngr.sql_select_curs(curs, 'Histograms', **kwargs, binning=Binnings.FIXED_COUNT)
                }
                for bc in FIXED_COUNT_BINS:
                    attrs = { 'bincount' : fmtnum(bc), 'binning' : Binnings.FIXED_COUNT.name }
                    try: value = entropy_fixed[bc]
                    except KeyError: pass
                    else: append_child(lo, 'entropy', **attrs).text = fmtnum(value)
                selected = count(lo.findall('entropy')) if selected_layouts is None else layoutid in selected_layouts
                lo.set('selected', format_bool(selected))
    if selected_layouts is not None:
        for lid in selected_layouts - all_layout_ids:
            raise HttpError(http.HTTPStatus.NOT_FOUND, "No layout with ID {!s} exists.".format(graphid))
