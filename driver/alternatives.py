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
    'AlternativeContext',
    'AlternativeContextHuang',
    'get_alternative_context',
    'get_alternative_value',
]

import collections
import itertools
import logging
import math
import os
import statistics

from .constants import *
from .quarry import *
from .utility import *

class AlternativeContext(object):

    def __init__(self, test):
        self.__test = test

    @property
    def test(self):
        return self.__test

class AlternativeContextHuang(AlternativeContext):

    def __init__(self):
        super().__init__(Tests.HUANG)
        self.weights = {
            Metrics.CROSS_COUNT        : +0.25,
            Metrics.CROSS_RESOLUTION   : -0.25,
            Metrics.ANGULAR_RESOLUTION : -0.25,
            Metrics.EDGE_LENGTH_STDEV  : +0.25,
        }
        self.__graph_cache = None
        self.__stats_cache = None
        self.__value_cache = None

    def __getstate__(self):
        return self.weights

    def __setstate__(self, state):
        self.__init__()
        self.weights.update(state)

    def populate_cache(self, mngr, curs):
        logging.info("Populating cache for Huang context ...")
        self.__graph_cache = dict()
        self.__stats_cache = dict()
        self.__value_cache = dict()
        graphs = collections.defaultdict(set)
        bigbag = collections.defaultdict(dict)
        for row in mngr.sql_select_curs(curs, 'Layouts'):
            graphid = row['graph']
            layoutid = row['id']
            self.__graph_cache[layoutid] = graphid
            graphs[graphid].add(layoutid)
        query = 'SELECT * FROM `Metrics` WHERE `metric` in (' + ', '.join(str(m.value) for m in self.metrics) + ')'
        for row in mngr.sql_exec_curs(curs, query, tuple()):
            if row['value'] is not None:
                bigbag[row['layout']][row['metric']] = row['value']
        for (graphid, layouts) in graphs.items():
            if len(layouts) < 3: continue
            for metric in self.metrics:
                try:
                    values = [ bigbag[lid][metric] for lid in layouts ]
                except KeyError:
                    (mean, stdev) = (None, None)
                else:
                    mean = statistics.mean(values)
                    stdev = statistics.stdev(values, xbar=mean)
                if graphid not in self.__stats_cache:
                    self.__stats_cache[graphid] = dict()
                self.__stats_cache[graphid][metric] = (mean, stdev)
        self.__value_cache = dict(bigbag)

    def get_cached_graph_id(self, *layoutids):
        assert len(layoutids) > 0
        if self.__graph_cache is None:
            return None
        return get_one({ self.__graph_cache.get(lid) for lid in layoutids })

    def get_cached_mean_and_stdev(self, *layoutids, graphid=None):
        assert (graphid is None) == (len(layoutids) > 0)
        if graphid is None:
            graphid = self.get_cached_graph_id(*layoutids)
        if graphid is not None and self.__stats_cache is not None:
            return self.__stats_cache[graphid]

    def get_cached_value(self, layoutid, metric):
        try:
            return self.__value_cache[layoutid][metric]
        except KeyError:
            return None

    @property
    def metrics(self):
        return sorted(self.weights.keys())

def get_alternative_context(mngr, curs, test, new=False):
    if test is Tests.HUANG:
        if not new and os.path.exists(mngr.alt_huang_params):
            with open(mngr.alt_huang_params, 'rb') as istr:
                logging.info("Loading {:s} parameters from file {!r} ...".format(test.name, istr.name))
                timestamp, ctx = unpickle_objects(istr, AlternativeContextHuang)
            return ctx
        return AlternativeContextHuang()
    return AlternativeContext(test)

def get_alternative_value(mngr, curs, ctx, lhsid, rhsid):
    if ctx.test is Tests.HUANG:
        return _get_alternative_value_huang(mngr, curs, ctx, lhsid, rhsid)
    else:
        return _get_alternative_value_stress(mngr, curs, ctx, lhsid, rhsid)

def _get_alternative_value_huang(mngr, curs, ctx, lhsid, rhsid):
    def xval(rows):
        row = get_one_or(rows)
        if row is not None: return row['value']
    stats = ctx.get_cached_mean_and_stdev(lhsid, rhsid)
    if stats is None:
        graphid = _get_graph_id(mngr, curs, lhsid, rhsid)
        values = collections.defaultdict(list)
        for lid in map(lambda r : r['id'], mngr.sql_select_curs(curs, 'Layouts', graph=graphid)):
            for row in filter(lambda r : r['value'] is not None, mngr.sql_select_curs(curs, 'Metrics', layout=lid)):
                values[row['metric']].append(row['value'])
        stats = { m : (statistics.mean(values[m]), statistics.stdev(values[m])) for m in ctx.metrics }
    lhsvals = { m : xval(mngr.sql_select_curs(curs, 'Metrics', layout=lhsid, metric=m)) for m in ctx.metrics }
    rhsvals = { m : xval(mngr.sql_select_curs(curs, 'Metrics', layout=rhsid, metric=m)) for m in ctx.metrics }
    if None in itertools.chain(lhsvals.values(), rhsvals.values(), itertools.chain.from_iterable(stats.values())):
        return None
    divor0 = lambda x, y : x / y if y > 0.0 else 0.0
    lhsval = sum(divor0(ctx.weights[m] * (lhsvals[m] - stats[m][0]), stats[m][1]) for m in ctx.metrics)
    rhsval = sum(divor0(ctx.weights[m] * (rhsvals[m] - stats[m][0]), stats[m][1]) for m in ctx.metrics)
    magnitude = abs(lhsval + rhsval) / 2.0
    offset = (lhsval - rhsval) / magnitude
    return math.tanh(offset)

def _get_graph_id(mngr, curs, lhs, rhs):
    graphs = set()
    graphs.add(get_one(mngr.sql_select_curs(curs, 'Layouts', id=lhs))['graph'])
    graphs.add(get_one(mngr.sql_select_curs(curs, 'Layouts', id=rhs))['graph'])
    assert len(graphs) == 1
    return next(iter(graphs))

def _get_alternative_value_stress(mngr, curs, ctx, lhsid, rhsid):
    metr = {
        Tests.STRESS_KK          : Metrics.STRESS_KK,
        Tests.STRESS_FIT_NODESEP : Metrics.STRESS_FIT_NODESEP,
        Tests.STRESS_FIT_SCALE   : Metrics.STRESS_FIT_SCALE,
    }[ctx.test]
    lhsrow = get_one_or(mngr.sql_select_curs(curs, 'Metrics', layout=lhsid, metric=metr))
    rhsrow = get_one_or(mngr.sql_select_curs(curs, 'Metrics', layout=rhsid, metric=metr))
    if lhsrow is not None and rhsrow is not None:
        lhsval = lhsrow['value']
        rhsval = rhsrow['value']
        magnitude = abs(lhsval + rhsval) / 2.0
        offset = (lhsval - rhsval) / magnitude
        return math.tanh(offset)
