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

import collections
import datetime
import math
import statistics

from . import *
from ..utility import *

def serve(self, url):
    perfstats = _get_perf_stats_cooked(self)
    root = ET.Element('root')
    for (tool, *values) in sorted(perfstats, key=(lambda ps : ps[3]), reverse=True):
        with Child(root, 'tool', name=tool) as tl:
            append_child(tl, 'cnt').text = fmtnum(values[0])
            append_child(tl, 'abs').text = fmtnum(values[1])
            append_child(tl, 'rel').text = fmtnum(values[2])
            append_child(tl, 'min').text = fmtnum(values[3])
            append_child(tl, 'max').text = fmtnum(values[4])
            append_child(tl, 'med').text = fmtnum(values[5])
            append_child(tl, 'avg').text = fmtnum(values[6])
    self.send_tree_xml(ET.ElementTree(root), transform='/xslt/perfstats.xsl')

def _get_perf_stats_cooked(self):
    tooltimes = _get_perf_stats_raw(self)
    total = sum(sum(times) for times in tooltimes.values())
    result = list()
    for (tool, times) in tooltimes.items():
        assert len(times) > 0
        _cnt = len(times)
        _abs = sum(times)
        _rel = _abs / total if total > 0.0 else math.nan
        _min = min(times)
        _max = max(times)
        _med = statistics.median(times)
        _avg = statistics.mean(times)
        result.append((tool, _cnt, _abs, _rel, _min, _max, _med, _avg))
    return result

def _get_perf_stats_raw(self):
    result = collections.defaultdict(list)
    for row in self.server.graphstudy_manager.sql_select('ToolPerformance'):
        result[row['tool']].append(row['time'])
    return result
