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

__all__ = [ 'do_metrics' ]

import logging
import math
import os.path

from .configuration import *
from .constants import *
from .errors import *
from .manager import *
from .utility import *

_METRIC_PROGS = {
    Metrics.STRESS_KK          : 'stress',
    Metrics.STRESS_FIT_NODESEP : 'stress',
    Metrics.STRESS_FIT_SCALE   : 'stress',
    Metrics.CROSS_COUNT        : 'huang',
    Metrics.CROSS_RESOLUTION   : 'huang',
    Metrics.ANGULAR_RESOLUTION : 'huang',
    Metrics.EDGE_LENGTH_STDEV  : 'huang',
}

_METRIC_FLAGS = {
    Metrics.STRESS_KK          : [ ],
    Metrics.STRESS_FIT_NODESEP : [ '--fit-nodesep' ],
    Metrics.STRESS_FIT_SCALE   : [ '--fit-scale' ],
    Metrics.CROSS_COUNT        : [ ],
    Metrics.CROSS_RESOLUTION   : [ ],
    Metrics.ANGULAR_RESOLUTION : [ ],
    Metrics.EDGE_LENGTH_STDEV  : [ ],
}

_HUANG_TOOL_HOWTO = {
    'cross-count'        : Metrics.CROSS_COUNT,
    'cross-resolution'   : Metrics.CROSS_RESOLUTION,
    'angular-resolution' : Metrics.ANGULAR_RESOLUTION,
    'edge-length-stdev'  : Metrics.EDGE_LENGTH_STDEV,
}

_METRIC_HOWTO = {
    Metrics.STRESS_KK          : { 'stress' : Metrics.STRESS_KK },
    Metrics.STRESS_FIT_NODESEP : { 'stress' : Metrics.STRESS_FIT_NODESEP },
    Metrics.STRESS_FIT_SCALE   : { 'stress' : Metrics.STRESS_FIT_SCALE },
    Metrics.CROSS_COUNT        : _HUANG_TOOL_HOWTO,
    Metrics.CROSS_RESOLUTION   : _HUANG_TOOL_HOWTO,
    Metrics.ANGULAR_RESOLUTION : _HUANG_TOOL_HOWTO,
    Metrics.EDGE_LENGTH_STDEV  : _HUANG_TOOL_HOWTO,
}

assert all(m in _METRIC_PROGS for m in Metrics)
assert all(m in _METRIC_FLAGS for m in Metrics)
assert all(m in _METRIC_HOWTO for m in Metrics)
assert all(m in howto.values() for (m, howto) in _METRIC_HOWTO.items())

def do_metrics(mngr : Manager, badlog : BadLog):
    for (metr, sizes) in mngr.config.desired_metrics.items():
        _compute_all_metrics(mngr, metr, sizes, badlog)

def _compute_all_metrics(mngr : Manager, metr : Metrics, sizes : set, badlog):
    for row in mngr.sql_select('Layouts'):
        graphid = row['graph']
        if _get_graph_size(mngr, graphid) not in sizes:
            continue
        layoutid = row['id']
        layoutfile = row['file']
        if mngr.sql_select('Metrics', layout=layoutid, metric=metr):
            continue
        if badlog.get_bad(Actions.METRICS, layoutid, metr):
            logging.notice("Skipping computation of metric {:s} for layout {!s}".format(metr.name, layoutid))
            continue
        logging.info("Computing metric {m:s} for layout {l!s} ...".format(m=metr.name, l=layoutid))
        try:
            _compute_metric(mngr, metr, layoutid, layoutfile)
        except RecoverableError as e:
            badlog.set_bad(Actions.METRICS, layoutid, metr, msg=str(e))
            logging.error("Cannot compute metric {metr:s} for layout {lid!s} of graph {gid!s}: {!s}".format(
                e, metr=metr.name, lid=layoutid, gid=graphid))

def _compute_metric(mngr : Manager, metr : Metrics, layoutid : Id, layoutfile : str):
    cmd = [ os.path.join(mngr.abs_bindir, 'src', 'metrics', _METRIC_PROGS[metr]) ]
    cmd.extend(_METRIC_FLAGS[metr])
    cmd.append('--meta=STDIO')
    cmd.append(layoutfile)
    meta = mngr.call_graphstudy_tool(cmd, meta='stdout', deterministic=True)
    _insert_metric(mngr, metr, layoutid, meta)

def _insert_metric(mngr : Manager, metr : Metrics, layoutid : Id, meta : dict):
    with mngr.sql_ctx as curs:
        for (key, dst) in _METRIC_HOWTO[metr].items():
            value = meta.get(key)
            if not (isinstance(value, int) or isinstance(value, float)) or not math.isfinite(value):
                logging.error("JSON data contains non-finite value: {:s}={!r}".format(key, value))
                if dst is metr: raise RecoverableError("Non-finite value obtained")
            if mngr.sql_select_curs(curs, 'Metrics', metric=dst, layout=layoutid):
                if dst is metr:
                    mngr.sql_exec_curs(curs, 'UPDATE `Metrics` SET `value` = ? WHERE `metric` = ? AND `layout` = ?',
                                       value, metr, layout=layoutid)
            else:
                mngr.sql_insert_curs(curs, 'Metrics', metric=dst, layout=layoutid, value=value)

def _get_graph_size(mngr : Manager, graphid : id) -> GraphSizes:
    return GraphSizes.classify(get_one(row['nodes'] for row in mngr.sql_select('Graphs', id=graphid)))
