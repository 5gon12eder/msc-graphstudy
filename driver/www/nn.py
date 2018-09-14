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
import http
import itertools
import logging
import math
import random
import statistics
import time
import typing
import urllib.parse

from . import *
from ..alternatives import *
from ..constants import *
from ..features import *
from ..model import *
from ..nn import *
from ..quarry import *
from ..utility import *
from .impl_common import *

def _serve_features(this, url=None):
    query = urllib.parse.parse_qs(url.query)
    (lhs, rhs) = _get_layout_ids_from_query(query, manager=this.server.graphstudy_manager)
    try:
        with open(this.server.graphstudy_manager.nn_features, 'rb') as istr:
            timestamp, features = unpickle_objects(istr, typing.List, error=_500)
    except FileNotFoundError:
        raise HttpError(http.HTTPStatus.NOT_FOUND, "Apparently, no model was built yet.")
    if lhs and rhs:
        (graphid, graphfeat, lhsfeat, rhsfeat) = _load_feature_rows(this.server.graphstudy_manager, lhs, rhs)
        if set(attribute_projection('name', features['graph'])) != set(graphfeat.keys()):
            raise HttpError(http.HTTPStatus.INTERNAL_SERVER_ERROR, "The graph features have changed!")
        expected_layout_features = set(attribute_projection('name', features['layout']))
        if set(lhsfeat.keys()) != expected_layout_features or set(rhsfeat.keys()) != expected_layout_features:
            raise HttpError(http.HTTPStatus.INTERNAL_SERVER_ERROR, "The layout features have changed!")
        del expected_layout_features
    root = ET.Element('root')
    append_child(root, 'timestamp').text = rfc5322(timestamp)
    if lhs and rhs:
        with Child(root, 'this') as node:
            append_child(node, 'graph').text = str(graphid)
            append_child(node, 'lhs').text = str(lhs)
            append_child(node, 'rhs').text = str(rhs)
    for (of, vector) in features.items():
        with Child(root, 'features', of=of) as node:
            for (idx, feat) in enumerate(vector):
                with Child(node, 'feature', index=fmtnum(idx)) as child:
                    append_child(child, 'name').text = feat.name
                    append_child(child, 'mean').text = fmtnum(feat.mean)
                    append_child(child, 'stdev').text = fmtnum(feat.stdev)
                    if lhs and rhs:
                        def getit(info):
                            diff = value_or(info[feat.name], math.nan) - feat.mean
                            if math.isfinite(feat.stdev) and feat.stdev > 0.0: return diff / feat.stdev
                            return math.nan if diff == 0.0 else math.inf
                        if of == 'graph':
                            append_child(child, 'this').text = fmtnum(getit(graphfeat))
                        if of == 'layout':
                            (lhsval, rhsval) = (getit(lhsfeat), getit(rhsfeat))
                            append_child(child, 'this-lhs').text = fmtnum(lhsval)
                            append_child(child, 'this-rhs').text = fmtnum(rhsval)
                            append_child(child, 'this-diff').text = fmtnum(lhsval - rhsval)
    this.send_tree_xml(ET.ElementTree(root), transform='/xslt/nn-features.xsl')

def _load_feature_rows(mngr, lhs, rhs):
    assert isinstance(lhs, Id) and isinstance(rhs, Id)
    with mngr.sql_ctx as curs:
        gridl = get_one_or(r['graph'] for r in mngr.sql_select_curs(curs, 'Layouts', id=lhs))
        gridr = get_one_or(r['graph'] for r in mngr.sql_select_curs(curs, 'Layouts', id=rhs))
        if gridl is None:
            raise HttpError(http.HTTPStatus.NOT_FOUND, "No layout with ID {!s} found.".format(lhs))
        if gridr is None:
            raise HttpError(http.HTTPStatus.NOT_FOUND, "No layout with ID {!s} found.".format(rhs))
        try:
            [ graphid ] = {gridl, gridr }
        except ValueError:
            raise HttpError(
                http.HTTPStatus.BAD_REQUEST,
                "The layouts {!s} and {!s} are from different graphs ({!s} and {!s}).".format(lhs, rhs, gridl, gridr)
            )
        features_graph = get_graph_features(mngr, curs, graphid, verbose=True)
        features_lhs = get_layout_features(mngr, curs, lhs, verbose=True)
        features_rhs = get_layout_features(mngr, curs, rhs, verbose=True)
        return (graphid, features_graph, features_lhs, features_rhs)

def _serve_testscore(this, url=None):
    with this.server.graphstudy_manager.sql_ctx as curs:
        query = urllib.parse.parse_qs(url.query)
        graphical = get_bool_from_query(query, 'graphical')
        special = _get_special_test_case_selection_from_query(query)
        results = collections.defaultdict(dict)
        for r in this.server.graphstudy_manager.sql_select_curs(curs, 'TestScores'):
            pair = (r['lhs'], r['rhs'])
            results[pair][r['test']] = value_or(r['value'], 0.0)
        if not results:
            raise HttpError(http.HTTPStatus.NOT_FOUND, "Apparently, no model was built yet.")
        laymap = dict()
        for (lhs, rhs) in results.keys():
            laymap[lhs] = None
            laymap[rhs] = None
        for layoutid in laymap.keys():
            laymap[layoutid] = get_informal_layout_name(this.server.graphstudy_manager, layoutid)
        root = ET.Element('root')
        append_common_all_constants(root, Tests, TestStatus)
        with Child(root, 'special-selection') as node:
            for (test, choice) in special.items():
                append_child(node, 'only', test=test.name, when=fmtnum(int(choice)))
        with Child(root, 'test-cases') as node:
            resultskeys = list(enumerate(results.keys()))
            random.shuffle(resultskeys)
            for (idx, pair) in resultskeys:
                (lhsid, rhsid) = pair
                expected = results[pair].get(Tests.EXPECTED, 0.0)
                outcomes = dict()
                for (test, value) in results[pair].items():
                    if test is not Tests.EXPECTED:
                        outcomes[test] = NNTestResult(lhsid, rhsid, expected, value)
                if any(k not in outcomes or bool(outcomes[k]) != v for (k, v) in special.items()):
                    continue
                with Child(node, 'test-case', index=fmtnum(idx), expected=fmtnum(expected)) as child:
                    with Child(child, 'lhs') as lhs:
                        if laymap[lhsid] is not None: lhs.set('informal', laymap[lhsid])
                        lhs.text = str(lhsid)
                    with Child(child, 'rhs') as rhs:
                        if laymap[rhsid] is not None: rhs.set('informal', laymap[rhsid])
                        rhs.text = str(rhsid)
                    for (test, result) in outcomes.items():
                        with Child(child, 'value', name=test.name) as grandchild:
                            grandchild.set('error', fmtnum(result.error))
                            grandchild.set('status', result.status.name)
                            grandchild.text = fmtnum(result.actual)
        stylesheet = '/xslt/nn-testscore-graphical.xsl' if graphical else '/xslt/nn-testscore.xsl'
        this.send_tree_xml(ET.ElementTree(root), transform=stylesheet)

def _serve_demo(this, url):
    query = urllib.parse.parse_qs(url.query)
    pairs = list()
    with this.server.graphstudy_manager.sql_ctx as curs:
        explicit = _get_layout_ids_from_query(query, manager=this.server.graphstudy_manager)
        randcount = _get_random_count_from_query(query)
        if explicit != (None, None):
            pairs.append(explicit)
        if randcount > 0:
            rand = random.Random()
            pairs.extend(_get_random_layout_pairs(this.server.graphstudy_manager, rand, randcount))
            if not pairs:
                raise HttpError(http.HTTPStatus.NOT_FOUND, "There are no layouts to begin with.")
        root = ET.Element('root')
        if pairs:
            t0 = time.time()
            oracle = Oracle(this.server.graphstudy_manager, exception=_500)
            t1 = time.time()
            wisdom = oracle(pairs, exception=_500, bidirectional=True)
            t2 = time.time()
            with Child(root, 'timings') as node:
                append_child(node, 'load').text = fmtnum(t1 - t0)
                append_child(node, 'eval').text = fmtnum(t2 - t1)
                append_child(node, 'total').text = fmtnum(t2 - t0)
            with Child(root, 'results') as node:
                for ((lhs, rhs), (ans, san)) in zip(pairs, wisdom):
                    with Child(node, 'result', random=format_bool((lhs, rhs) != explicit)) as child:
                        with Child(child, 'lhs') as lhsnode:
                            lhsnode.set('informal', get_informal_layout_name(this.server.graphstudy_manager, lhs))
                            lhsnode.text = str(lhs)
                        with Child(child, 'rhs') as rhsnode:
                            rhsnode.set('informal', get_informal_layout_name(this.server.graphstudy_manager, rhs))
                            rhsnode.text = str(rhs)
                        append_child(child, 'value', name=Tests.NN_FORWARD.name).text = fmtnum(+ans)
                        append_child(child, 'value', name=Tests.NN_REVERSE.name).text = fmtnum(-san)
                        for test in filter(Tests.is_alternative, Tests):
                            ctx = get_alternative_context(this.server.graphstudy_manager, curs, test)
                            val = get_alternative_value(this.server.graphstudy_manager, curs, ctx, lhs, rhs)
                            append_child(child, 'value', name=test.name).text = fmtnum(val)
    this.send_tree_xml(ET.ElementTree(root), transform='/xslt/nn-demo.xsl')

def _get_random_layout_pairs(mngr, rand, count=1):
    pairs = set()
    with mngr.sql_ctx as curs:
        graphs = [ r['id'] for r in mngr.sql_select_curs(curs, 'Graphs') ]
        while graphs and len(pairs) < count:
            graphid = rand.choice(graphs)
            layouts = sorted((r['id'] for r in mngr.sql_select_curs(curs, 'Layouts', graph=graphid)), key=Id.getkey)
            for pair in filter(lambda p : p not in pairs, itertools.combinations(layouts, 2)):
                pairs.add(pair)
                break
            else:
                graphs.remove(graphid)
    return pairs

def _get_random_count_from_query(query):
    try:
        values = list(map(int, query['random']))
        if all(n >= 0 for n in values):
            return sum(values)
    except KeyError:
        return 0
    except ValueError:
        pass
    raise HttpError(http.HTTPStatus.BAD_REQUEST, "Parameter 'random' must be set to a non-negative integer.")

def _get_layout_ids_from_query(query, manager=None):
    lhs = query.get('lhs', [ ])
    rhs = query.get('rhs', [ ])
    if not lhs and not rhs:
        return (None, None)
    elif len(lhs) > 1:
        raise HttpError(http.HTTPStatus.BAD_REQUEST, "Parameters 'lhs' must not be used more than once.")
    elif len(rhs) > 1:
        raise HttpError(http.HTTPStatus.BAD_REQUEST, "Parameters 'rhs' must not be used more than once.")
    elif len(lhs) != len(rhs):
        raise HttpError(http.HTTPStatus.BAD_REQUEST, "Parameters 'lhs' and 'rhs' must be used together.")
    elif manager is None:
        getter = lambda s : validate_layout_id(get_one(s))
        return (getter(lhs), getter(rhs))
    else:
        with manager.sql_ctx as curs:
            getter = lambda s : manager.idmatch_curs(curs, 'Layouts', get_one(s), exception=_404)
            return (getter(lhs), getter(rhs))

def _get_special_test_case_selection_from_query(query):
    selection = dict()
    for (key, val) in query.items():
        if key.lower().startswith('only-'):
            name = key[len('only-'):]
            try:
                test = enum_from_json(Tests, name)
            except ValueError:
                raise HttpError(http.HTTPStatus.BAD_REQUEST, "Unknown test {!r} selected".format(name))
            try:
                [ choice ] = val
            except ValueError:
                raise HttpError(http.HTTPStatus.BAD_REQUEST, "Parameter {!r} must be used at most once".format(key))
            if not choice:
                # TODO: Can this ever happen at all?
                raise HttpError(http.HTTPStatus.BAD_REQUEST, "Parameter {!r} must not be empty".format(key))
            try:
                selection[test] = parse_bool(choice)
            except ValueError:
                raise HttpError(http.HTTPStatus.BAD_REQUEST, "Parameter {!r} must be set to a boolean".format(key))
    return selection

def _500(msg):
    return HttpError(http.HTTPStatus.INTERNAL_SERVER_ERROR, msg)

def _404(msg):
    return HttpError(http.HTTPStatus.NOT_FOUND, msg)

class _NotFound(object):

    def __call__(self, *args, **kwargs):
        raise HttpError(http.HTTPStatus.NOT_FOUND)

serve = collections.defaultdict(_NotFound)
serve['features'] = _serve_features
serve['testscore'] = _serve_testscore
serve['demo'] = _serve_demo
