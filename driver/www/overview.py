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

import math

from . import *
from ..constants import *
from ..utility import *
from .impl_common import *

def serve(self, url, focus=None):
    root = ET.Element('root')
    append_common_all_constants(root, Generators, Layouts, Properties, GraphSizes)
    with Child(root, 'graphs') as gs:
        for row in self.server.graphstudy_manager.sql_select('Graphs'):
            layouts = {
                r['layout'] : r['id']
                for r in self.server.graphstudy_manager.sql_select('Layouts', graph=row['id'], layout=object)
            }
            with Child(gs, 'graph', _id=str(row['id'])) as gr:
                size = GraphSizes.classify(row['nodes'])
                nodes = row['nodes']
                edges = row['edges']
                try:
                    sparsity = row['edges'] / row['nodes']**2
                except ZeroDivisionError:
                    sparsity = math.nan
                gen = row['generator']
                append_child(gr, 'size').text = size.name
                append_child(gr, 'nodes').text = fmtnum(nodes)
                append_child(gr, 'edges').text = fmtnum(edges)
                append_child(gr, 'sparsity').text = fmtnum(sparsity)
                append_child(gr, 'generator').text = gen.name
                with Child(gr, 'layouts') as ls:
                    for layo in filter(lambda l : l in layouts, Layouts):
                        append_child(ls, 'layout', id=str(layouts[layo]), layout=layo.name)
    stylesheets = { 'graphs' : '/xslt/all-graphs.xsl', 'layouts' : '/xslt/all-layouts.xsl' }
    self.send_tree_xml(ET.ElementTree(root), transform=stylesheets.get(focus))
