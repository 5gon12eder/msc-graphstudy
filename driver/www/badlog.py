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
import itertools
import random

from . import *
from ..configuration import *
from ..constants import *
from ..utility import *
from .impl_common import *

def serve(this, url):
    root = ET.Element('root')
    append_common_all_constants(root, Actions, Layouts, LayWorse, LayInter, Properties, Metrics)
    with BadLog(logfile=this.server.graphstudy_manager.badlog, readonly=True) as bl:
        with Child(root, 'filename') as child:
            if bl.timestamp is not None:
                child.set('timestamp', rfc5322(bl.timestamp))
            child.text = bl.filename
        if bl.timestamp is not None:
            for act in Actions:
                keys = {
                    Actions.LAYOUTS:    [ 'graph-id', 'layout' ],
                    Actions.LAY_WORSE:  [ 'parent', 'method' ],
                    Actions.LAY_INTER:  [ 'parent-1st', 'parent-2nd', 'method' ],
                    Actions.PROPERTIES: [ 'layout-id', 'property' ],
                    Actions.METRICS:    [ 'layout-id', 'metric' ],
                }.get(act, list())
                number = count(bl.iterate(act))
                records = list(itertools.islice(bl.iterate(act), 100))
                random.shuffle(records)
                with Child(root, 'problems', action=act.name, count=fmtnum(number)) as node:
                    for (args, msg) in records:
                        with Child(node, 'problem') as child:
                            child.text = msg
                            for (k, v) in zip(keys, args):
                                child.set(k, _fmtkey(v))
    this.send_tree_xml(ET.ElementTree(root), transform='/xslt/badlog.xsl')

def _fmtkey(key):
    if isinstance(key, str):
        return key
    elif isinstance(key, Id):
        return str(key)
    elif isinstance(key, enum.Enum):
        return key.name
    else:
        raise TypeError(repr(type(key)))
