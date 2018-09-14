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

from . import *
from ..constants import *
from ..utility import *
from .impl_common import *

def serve(this, graphid, url=None):
    graphrow = get_one_or(this.server.graphstudy_manager.sql_select('Graphs', id=graphid))
    if graphrow is None:
        raise HttpError(http.HTTPStatus.NOT_FOUND, "No graph with ID {!s} exists.".format(graphid))
    root = ET.Element('root')
    append_child(root, 'id').text = str(graphid)
    append_common_all_constants(root, Layouts)
    append_common_graph(root, graphrow, tag=None)
    this.send_tree_xml(ET.ElementTree(root), transform='/xslt/graph.xsl')
