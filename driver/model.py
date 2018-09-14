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

__all__ = [ 'do_model', 'Oracle' ]

import collections
import contextlib
import itertools
import json
import logging
import math
import os
import random
import statistics
import sys
import tempfile
import time
import warnings

import numpy as np
from scipy.optimize import minimize as scipy_opt_minimize
_keras = None

from .alternatives import *
from .cli import *
from .configuration import *
from .constants import *
from .errors import *
from .features import *
from .manager import *
from .nn import *
from .quarry import *
from .utility import *

_MIN_SIGNIFICANCE = 0.05

_FP_TYPE = 'float32'

_DEBUG_DIR_ENVVAR = 'MSC_MODEL_DEBUGDIR'

_INVALID_TIME_STAMP = math.nan

_LAYOUT_RATINGS = {
    Layouts.NATIVE         : +1.00,
    Layouts.FMMM           : +1.00,
    Layouts.STRESS         : +1.00,
    Layouts.RANDOM_UNIFORM : -1.00,
    Layouts.RANDOM_NORMAL  : -1.00,
    Layouts.PHANTOM        : -1.00,
}

_MeanStdev = collections.namedtuple('MeanStdev', [ 'mean', 'stdev' ])

def _import_3d_party_libraries():
    global _keras
    logging.info("Importing {!r} ...".format('keras'))
    with tempfile.TemporaryFile(mode='w+', prefix='driver-', suffix='.stderr') as temp:
        with contextlib.redirect_stderr(temp):
            import keras
            _keras = keras
        temp.flush()
        temp.seek(0)
        stderrlines = temp.readlines()
    if stderrlines:
        logging.info("{:d} lines of diagnostics were produced while importing the module:".format(len(stderrlines)))
        for (i, line) in enumerate(map(str.rstrip, stderrlines)):
            logging.info("[{:4d}] {:s}".format(i + 1, line))

def _log_discard_too_old(filename, timestamp, noolder, what="stuff"):
    limit = datetime.fromtimestamp(noolder) if math.isfinite(noolder) else "now"
    logging.notice("Discarding NN {:s} loaded from {!r} with time-stamp {!s} older than {!s}".format(
        what, filename, timestamp, limit))

class _DataSet(object):

    def __init__(self, lhs, rhs, aux, out):
        assert [ len(lhs.shape), len(rhs.shape), len(aux.shape), len(out.shape) ] == [ 2, 2, 2, 2 ]
        assert lhs.shape[0] == rhs.shape[0] == aux.shape[0] == out.shape[0]
        assert lhs.shape[1] == rhs.shape[1]
        assert out.shape[1] == 1
        self.lhs = lhs
        self.rhs = rhs
        self.aux = aux
        self.out = out

    def __len__(self):
        return self.out.shape[0]

    def bias(self):
        pos = int((self.out > 0.0).sum())
        neg = int((self.out < 0.0).sum())
        bias = float(self.out.mean())
        return { 'pos' : pos, 'neg' : neg, 'bia' : bias }

def _save_weighted_model(mngr, model):
    logging.info("Saving NN model architecture to file {!r} ...".format(mngr.nn_model))
    with open(mngr.nn_model, 'w') as ostr:
        ostr.write(model.to_yaml())
    logging.info("Saving NN model weights to file {!r} ...".format(mngr.nn_weights))
    model.save_weights(mngr.nn_weights)

def _restore_weighted_model(mngr):
    logging.info("Trying to load NN model architecture from file {!r} ...".format(mngr.nn_model))
    try:
        with open(mngr.nn_model, 'r') as ostr:
            yaml = ostr.read()
    except FileNotFoundError:
        logging.error("Cannot load NN model architecture")
        return None
    model = _keras.models.model_from_yaml(yaml)
    logging.info("Trying to load NN model weights from file {!r} ...".format(mngr.nn_weights))
    try:
        model.load_weights(mngr.nn_weights)
    except OSError:  # For whatever reason, no FileNotFoundError but some other OSError is raised here.
        logging.error("Cannot load NN model weights")
        return None
    return model

def _save_features(mngr, layout, graph, exception=FatalError):
    logging.info("Saving NN features to file {!r} ...".format(mngr.nn_features))
    with mngr.sql_ctx as curs:
        lf = _layout_features(mngr, curs)
        gf = _graph_features(mngr, curs)
    features = dict()
    for (feat, of, norm) in [ (lf, 'layout', layout), (gf, 'graph', graph) ]:
        vector = features[of] = list()
        for (name, (mean, stdev)) in zip(feat, norm):
            vector.append(NNFeature(float(mean), float(stdev), name=name))
    with open(mngr.nn_features, 'wb') as ostr:
        pickle_objects(ostr, features)

def _restore_features(mngr, noolder=None, exception=FatalError):
    logging.info("Trying to restore NN features from file {!r} ...".format(mngr.nn_features))
    try:
        with open(mngr.nn_features, 'rb') as istr:
            timestamp, features = unpickle_objects(istr, dict)
    except FileNotFoundError:
        return _INVALID_TIME_STAMP, None, None
    if noolder is None or timestamp.timestamp() >= noolder:
        with mngr.sql_ctx as curs:
            expected_layout_features = _layout_features(mngr, curs)
            expected_graph_features = _graph_features(mngr, curs)
        actual_layout_features = [ feat.name for feat in features['layout'] ]
        actual_graph_features = [ feat.name for feat in features['graph'] ]
        if actual_layout_features != expected_layout_features or actual_graph_features != expected_graph_features:
            raise exception("The stored data was fitted to a different than the currently used model")
        layout = np.array([ [ feat.mean, feat.stdev ] for feat in features['layout'] ], dtype=_FP_TYPE).reshape(-1, 2)
        graph  = np.array([ [ feat.mean, feat.stdev ] for feat in features['graph']  ], dtype=_FP_TYPE).reshape(-1, 2)
        return timestamp.timestamp(), layout, graph
    else:
        _log_discard_too_old(mngr.nn_features, timestamp, noolder, what="features")
        return _INVALID_TIME_STAMP, None, None

def _save_test_score(mngr, curs, test, values, info):
    assert values.shape == (len(info), 1)
    results = list()
    for (i, (lhs, rhs)) in enumerate(info):
        value = float(values[i][0])
        results.append((lhs, rhs, test, value))
    curs.executemany("INSERT INTO `TestScores` (`lhs`, `rhs`, `test`, `value`) VALUES (?, ?, ?, ?)", iter(results))

def _restore_testscore(mngr, noolder=None):
    raise NotImplementedError("YAGNI")

def _get_layout_features_curs(mngr, curs, layoutid, cache=None):
    try: return _cache_get(cache, layoutid)
    except KeyError: pass
    return _cache_put(cache, layoutid, get_layout_features(mngr, curs, layoutid, na=math.nan))

def _get_graph_features_curs(mngr, curs, graphid, cache=None):
    try: return _cache_get(cache, graphid)
    except KeyError: pass
    return _cache_put(cache, graphid, get_graph_features(mngr, curs, graphid, na=math.nan))

def _cache_get(cache, key):
    if cache is None:
        raise KeyError()
    return cache[key]

def _cache_put(cache, key, value):
    if cache is not None:
        cache[key] = value
    return value

def _do_compile_model(mngr, model):
    model.compile(loss='mse', optimizer='sgd')

def _layout_features(mngr, curs):
    return list(get_layout_features(mngr, curs, None, verbose=True).keys())

def _graph_features(mngr, curs):
    return list(get_graph_features(mngr, curs, None, verbose=True).keys())

def _get_sql_view_columns(mngr, view, exception=FatalError):
    info = mngr.sql_exec("PRAGMA table_info(`{:s}`)".format(view), tuple())
    if len(info) <= 1:
        raise SanityError("The view {!r} seems to contain no data at all".format(view))
    return [ info[i]['name'] for i in range(1, len(info)) ]

class Oracle(object):

    def __init__(self, mngr, exception=FatalError):
        _import_3d_party_libraries()
        self.__manager = mngr
        self.__model = _restore_weighted_model(self.__manager)
        if self.__model is None:
            raise exception("There is no NN model to be used")
        timestamp, self.__lonorm, self.__grnorm = _restore_features(self.__manager, exception=exception)
        if self.__lonorm is None or self.__grnorm is None:
            raise exception("There are no NN freatures to be used")

    def __call__(self, layoutpairs, exception=FatalError, bidirectional=False):
        n = len(layoutpairs)
        forward = self.__load_data(layoutpairs, exception=exception)
        self.__predict(forward)
        if not bidirectional:
            return list(float(forward.out[i][0]) for i in range(n))
        backward = _DataSet(forward.rhs, forward.lhs, forward.aux, np.zeros((n, 1), dtype=_FP_TYPE))
        self.__predict(backward)
        return list((float(forward.out[i][0]), float(backward.out[i][0])) for i in range(n))

    def __load_data(self, layoutpairs, exception=FatalError):
        n = len(layoutpairs)
        logging.info("Loading data for {:,d} layout pairs from database ...".format(n))
        t0 = time.time()
        lhs = list()
        rhs = list()
        aux = list()
        with self.__manager.sql_ctx as curs:
            for (lhslid, rhslid) in layoutpairs:
                graphid = _get_same_graph_id_curs(self.__manager, curs, lhslid, rhslid, exception=exception)
                lhs.append(_get_layout_features_curs(self.__manager, curs, lhslid))
                rhs.append(_get_layout_features_curs(self.__manager, curs, rhslid))
                aux.append(_get_graph_features_curs(self.__manager, curs, graphid))
        lhs = _normalize_data(self.__lonorm, np.array(lhs, dtype=_FP_TYPE).reshape(n, self.__lonorm.shape[0]))
        rhs = _normalize_data(self.__lonorm, np.array(rhs, dtype=_FP_TYPE).reshape(n, self.__lonorm.shape[0]))
        aux = _normalize_data(self.__grnorm, np.array(aux, dtype=_FP_TYPE).reshape(n, self.__grnorm.shape[0]))
        out = np.zeros((n, 1), dtype=_FP_TYPE)  # TODO: Not the smartest move ...
        t1 = time.time()
        logging.info("Data acquisition took {:.3f} seconds".format(t1 - t0))
        return _DataSet(lhs, rhs, aux, out)

    def __predict(self, data):
        logging.info("Running NN model to classify {:,d} layout pairs ...".format(len(data)))
        t0 = time.time()
        prediction = self.__model.predict([ data.lhs, data.rhs, data.aux ])
        # For whatever reason, predict() returns no Numpy array but a Python list if the array is empty...
        if len(prediction) > 0:
            assert data.out.shape == prediction.shape
            np.copyto(data.out, prediction)
        t1 = time.time()
        logging.info("Classification took {:.3f} seconds".format(t1 - t0))
        return data.out

def _get_graph_id_curs(mngr, curs, layoutid, exception=FatalError):
    row = get_one_or(mngr.sql_select_curs(curs, 'Layouts', id=layoutid))
    if row is None:
        raise exception("The layout {!s} does not exist".format(layoutid))
    return row['graph']

def _get_same_graph_id_curs(mngr, curs, lhslid, rhslid, exception=FatalError):
    lhsgid = _get_graph_id_curs(mngr, curs, lhslid, exception=exception)
    rhsgid = _get_graph_id_curs(mngr, curs, rhslid, exception=exception)
    if lhsgid != rhsgid:
        logging.notice("Layout {!s} corresponds to graph {!s}".format(lhslid, lhsgid))
        logging.notice("Layout {!s} corresponds to graph {!s}".format(rhslid, rhsgid))
        raise exception("The layouts {!s} and {!s} correspond to different graphs".format(lhslid, rhslid))
    return lhsgid

def _get_debugdir():
    directory = os.getenv(_DEBUG_DIR_ENVVAR)
    if directory is None:
        return None
    if not os.path.isabs(directory):
        logging.warning("Ignoring non-absolute directory {:s}={!r}".format(_DEBUG_DIR_ENVVAR, directory))
        return None
    if not os.path.isdir(directory):
        logging.warning("Ignoring non-existant directory {:s}={!r}".format(_DEBUG_DIR_ENVVAR, directory))
        return None
    return directory

def _setup(mngr : Manager):
    _import_3d_party_libraries()
    os.makedirs(mngr.nndir, exist_ok=True)

def do_model(mngr : Manager, badlog : BadLog):
    _setup(mngr)
    with mngr.sql_ctx as curs:
        lf = _layout_features(mngr, curs)
        gf = _graph_features(mngr, curs)
    model = _build_model(mngr, inputs=len(lf), auxinputs=len(gf))
    (training, testing, traininfo, testinfo) = _load_training_and_testing_data(mngr)
    _train_model(model, training)
    _train_alternatives(mngr, training, traininfo)
    predictions = _test_model(model, testing)
    _save_weighted_model(mngr, model)
    logging.info("Saving NN test scores ...")
    with mngr.sql_ctx as curs:
        curs.execute("DELETE FROM `TestScores`", tuple())
        _save_test_score(mngr, curs, Tests.EXPECTED, testing.out, testinfo)
        _save_test_score(mngr, curs, Tests.NN_FORWARD, predictions, testinfo)
    _test_alternatives(mngr, testinfo)
    _write_test_summary(mngr, filename=os.getenv('MSC_NN_TEST_SUMMARY'))

def _load_training_and_testing_data(
        mngr : Manager,
        testfraction : float = 0.2,  # fraction of graphs to reserve for testing
        persist : bool = True,       # persist feature vectors
        strict : bool = True,        # reject nonsensically small data sets
):
    logging.info("Loading training / testing data from database ...")
    t0 = time.time()
    grdata = dict()
    lodata = dict()
    lhs = list()
    rhs = list()
    aux = list()
    out = list()
    info = list()
    selection = list()
    unique_pairs = set()
    with mngr.sql_ctx as curs:
        grdims = len(get_graph_features(mngr, curs, None))
        lodims = len(get_layout_features(mngr, curs, None))
        layout_fingerprints = { r['id'] : r['fingerprint'] for r in mngr.sql_select_curs(curs, 'Layouts') }
        datafilter = lambda seq : filter(lambda t : abs(t[3]) >= _MIN_SIGNIFICANCE, seq)
        collection = list()
        collection.extend(datafilter(_gather_proper(mngr, curs)))
        collection.extend(datafilter(_gather_inter(mngr, curs)))
        collection.extend(datafilter(_gather_worse(mngr, curs)))
        allgraphids = frozenset(t[2] for t in collection)
        if strict and len(allgraphids) < 2:
            raise FatalError("Please, I need at least two graphs, you only gave me {:d}".format(len(collection)))
        while True:
            testgraphids = frozenset(gid for gid in allgraphids if random.random() < testfraction)
            if len(allgraphids) < 2 or 0 < len(testgraphids) < len(allgraphids): break
        for (lhsid, rhsid, graphid, rank) in collection:
            if layout_fingerprints[lhsid] == layout_fingerprints[rhsid]:
                logging.debug("Seemingly identical pair ({!s}, {!s}, {:+.4f})".format(lhsid, rhsid, rank))
                continue
            pair = (lhsid, rhsid)
            if pair in unique_pairs:
                continue
            unique_pairs.add(pair)
            lhs.append(_get_layout_features_curs(mngr, curs, lhsid, cache=lodata))
            rhs.append(_get_layout_features_curs(mngr, curs, rhsid, cache=lodata))
            aux.append(_get_graph_features_curs(mngr, curs, graphid, cache=grdata))
            out.append([ rank ])
            info.append((lhsid, rhsid))
            selection.append(graphid not in testgraphids)
    count = len(out)
    if strict and count < 10:
        raise FatalError("There is no point in training a neural network with only {:,d} data points".format(count))
    grdata = np.array(list(grdata.values()), dtype=_FP_TYPE).reshape(len(grdata), grdims)
    lodata = np.array(list(lodata.values()), dtype=_FP_TYPE).reshape(len(lodata), lodims)
    if not mngr.config.puncture:
        _check_for_non_finite_feature_vectors(grdata, what="graph")
        _check_for_non_finite_feature_vectors(lodata, what="layout")
    grnorm = _get_normalizers(grdata)
    lonorm = _get_normalizers(lodata)
    lhs = _normalize_data(lonorm, np.array(lhs, dtype=_FP_TYPE).reshape(len(lhs), lodims))
    rhs = _normalize_data(lonorm, np.array(rhs, dtype=_FP_TYPE).reshape(len(rhs), lodims))
    aux = _normalize_data(grnorm, np.array(aux, dtype=_FP_TYPE).reshape(len(aux), grdims))
    out = np.array(out, dtype=_FP_TYPE).reshape(len(out), 1)
    selection = np.array(selection, dtype='bool')
    training = _DataSet(lhs[ selection], rhs[ selection], aux[ selection], out[ selection])
    testing  = _DataSet(lhs[~selection], rhs[~selection], aux[~selection], out[~selection])
    traininfo = [ info[i] for i in range(count) if     selection[i] ]
    testinfo  = [ info[i] for i in range(count) if not selection[i] ]
    t1 = time.time()
    logging.info("Data acquisition took {:.3f} seconds".format(t1 - t0))
    if persist:
        _save_features(mngr, layout=lonorm, graph=grnorm)
    _dump_corpus_infos(training, testing, directory=_get_debugdir())
    return (training, testing, traininfo, testinfo)

def _check_for_non_finite_feature_vectors(vectors, what="???", badlimit=0.01):
    """
    Counts the column vectors of `vectors` (a two-dimensional real NumPy array) that contain non-finite entries and
    checks whether their relative number exceeds `badlimit`.  If so, a `FatalError` is raised.  `badlimit` may be set to
    `None` to omit this check.  Returns the relative number of vectors with non-finite entris.

        >>> round(100.0 * _check_for_non_finite_feature_vectors(np.zeros((0, 0), dtype=_FP_TYPE)))
        0

        >>> round(100.0 * _check_for_non_finite_feature_vectors(np.zeros((0, 7), dtype=_FP_TYPE)))
        0

        >>> round(100.0 * _check_for_non_finite_feature_vectors(np.zeros((7, 0), dtype=_FP_TYPE)))
        0

        >>> round(100.0 * _check_for_non_finite_feature_vectors(np.zeros((7, 12), dtype=_FP_TYPE)))
        0

        >>> vectors = np.array([[1.0, 2.0], [math.nan, 4.0], [5.0, 6.0]], dtype=_FP_TYPE)
        >>> round(100.0 * _check_for_non_finite_feature_vectors(vectors, badlimit=None))
        33

        >>> vectors = np.array([[1.0, 2.0], [math.nan, math.nan], [5.0, 6.0]], dtype=_FP_TYPE)
        >>> round(100.0 * _check_for_non_finite_feature_vectors(vectors, badlimit=None))
        33

        >>> vectors = np.array([[math.inf]], dtype=_FP_TYPE)
        >>> round(100.0 * _check_for_non_finite_feature_vectors(vectors, badlimit=1.0))
        100

        >>> vectors = np.array([[math.inf, 14.92], [14.92, 14.92]], dtype=_FP_TYPE)
        >>> round(100.0 * _check_for_non_finite_feature_vectors(vectors, badlimit=0.1234, what="happy"))
        Traceback (most recent call last):
        driver.errors.FatalError: 50.00 % of the happy feature vectors contain non-finite entries (limit: 12.34 %)

        >>> vectors = np.array([
        ...     [       0.0,        0.0,        0.0,        0.0,        0.0],  # all finite
        ...     [  math.nan,        0.0,        0.0,        0.0,        0.0],  # one non-finite
        ...     [       0.0,        0.0,  +math.inf,  -math.inf,        0.0],  # multiple non-finite
        ...     [  math.nan,   math.nan,   math.nan,   math.nan,   math.nan],  # all non-finite
        ...     [       1.0,        2.0,        3.0,        4.0,        5.0],  # all finite
        ... ], dtype=_FP_TYPE)
        >>> round(100.0 * _check_for_non_finite_feature_vectors(vectors, badlimit=1.0))
        60

    """
    assert vectors.dtype == _FP_TYPE
    n = int(vectors.shape[0])
    m = int(vectors.shape[1])
    matrix = np.zeros((m, 1), dtype=_FP_TYPE)
    matrix.fill(1.0)
    badcount = (~np.isfinite(np.matmul(vectors, matrix))).sum()
    relative = float(badcount) / float(n) if n > 0 else 0.0
    logging.info("{:,d} of {:,d} {:s} feature vectors ({:.2f} %) contain non-finite entries"
                 .format(badcount, n, what, 100.0 * relative))
    if badlimit is not None and relative > badlimit:
        raise FatalError("{:.2f} % of the {:s} feature vectors contain non-finite entries (limit: {:.2f} %)"
                         .format(100.0 * relative, what, 100.0 * badlimit))
    return relative

def _gather_proper(mngr, curs):
    for grow in mngr.sql_select_curs(curs, 'Graphs'):
        graphid = grow['id']
        layouts = {
            r['id'] : r['layout'] for r in mngr.sql_select_curs(curs, 'Layouts', graph=graphid, layout=object)
        }
        for (lhs, rhs) in itertools.combinations(list(layouts.keys()), 2):
            if random.random() > 0.5:
                (lhs, rhs) = (rhs, lhs)
            rank = _rank_layouts(layouts[lhs], layouts[rhs])
            if rank is not None:
                assert -1.0 <= rank <= +1.0
                yield (lhs, rhs, graphid, rank)

def _gather_inter(mngr, curs):
    for method in LayInter:
        graphs = dict()
        lines = collections.defaultdict(dict)
        for row in mngr.sql_select_curs(curs, 'InterLayouts', method=method):
            parents = (row['parent1st'], row['parent2nd'])
            graphs[parents] = _get_graph_id(mngr, curs, row['id'])
            lines[parents][row['rate']] = row['id']
        for parents in lines.keys():
            lines[parents][0.0] = parents[0]
            lines[parents][1.0] = parents[1]
        for parents in graphs.keys():
            graphid = graphs[parents]
            prank = _rank_layout_ids(mngr, curs, parents[0], parents[1])
            if prank is None:
                continue
            for (r1, r2) in itertools.combinations(lines[parents].keys(), 2):
                if random.random() > 0.5:
                    (r1, r2) = (r2, r1)
                rank = (r2 - r1) * prank
                assert -1.0 <= rank <= +1.0
                yield (lines[parents][r1], lines[parents][r2], graphid, rank)

def _gather_worse(mngr, curs):
    maxima = _get_worse_max(mngr, curs)
    for method in LayWorse:
        graphs = dict()
        lines = collections.defaultdict(dict)
        for row in mngr.sql_select_curs(curs, 'WorseLayouts', method=method):
            parent = row['parent']
            graphs[parent] = _get_graph_id(mngr, curs, row['id'])
            lines[parent][row['rate']] = row['id']
        for parent in lines.keys():
            lines[parent][0.0] = parent
        for (parent, graphid) in graphs.items():
            prank = _rank_layout_id(mngr, curs, parent)
            if prank is None:
                continue
            for (r1, r2) in itertools.combinations(lines[parent].keys(), 2):
                if random.random() > 0.5:
                    (r1, r2) = (r2, r1)
                r1norm = r1 / maxima[method]
                r2norm = r2 / maxima[method]
                rank = (r1norm - r2norm) * prank if prank > 0.0 else 0.0
                assert -1.0 <= rank <= +1.0
                yield (lines[parent][r1], lines[parent][r2], graphid, rank)

def _get_graph_id(mngr, curs, layoutid : Id) -> Id:
    return get_one(mngr.sql_select_curs(curs, 'Layouts', id=layoutid))['graph']

def _rank_layout_id(mngr, curs, layoutid : Id) -> float:
    return _rank_layout(_get_layout(mngr, curs, layoutid))

def _rank_layout_ids(mngr, curs, lhsid : Id, rhsid : Id) -> float:
    return _rank_layouts(_get_layout(mngr, curs, lhsid), _get_layout(mngr, curs, rhsid))

def _get_layout(mngr, curs, layoutid : Id) -> Layouts:
    return get_one(mngr.sql_select_curs(curs, 'Layouts', id=layoutid))['layout']

def _build_model(mngr, inputs : int, auxinputs : int):
    logging.info("Constructing NN model ...")
    t0 = time.time()
    kwargs = {
        'kernel_initializer' : 'truncated_normal',
        'bias_initializer'   : 'zeros',
        'dtype'              : _FP_TYPE,
    }
    lhsin = _keras.layers.Input(shape=singleton(inputs), dtype=_FP_TYPE, name='lhsin')
    rhsin = _keras.layers.Input(shape=singleton(inputs), dtype=_FP_TYPE, name='rhsin')
    auxin = _keras.layers.Input(shape=singleton(auxinputs), dtype=_FP_TYPE, name='auxin')
    nn = _make_half_model(inputs, kwargs)
    lhs = nn(lhsin)
    rhs = nn(rhsin)
    sub = _keras.layers.Subtract(dtype=_FP_TYPE, name='sub')([ lhs, rhs ])
    aux = _keras.layers.Dense(auxinputs, **kwargs, activation='linear', name='aux')(auxin)
    cat = _keras.layers.Concatenate(dtype=_FP_TYPE, name='cat')([ sub, aux ])
    out = _keras.layers.Dense(1, **kwargs, activation='tanh', name='out')(cat)
    model = _keras.models.Model([ lhsin, rhsin, auxin ], out, name='total')
    debugdir = _get_debugdir()
    if debugdir is not None:
        logging.notice("Dumping auxiliary information (hint: {:s}={!r}) ...".format(_DEBUG_DIR_ENVVAR, debugdir))
        _dump_model_infos(model, nn, directory=debugdir)
    t1 = time.time()
    logging.info("Construction took {:.3f} seconds".format(t1 - t0))
    logging.info("Compiling NN model ...")
    t0 = time.time()
    _do_compile_model(mngr, model)
    t1 = time.time()
    logging.info("Compilation took {:.3f} seconds".format(t1 - t0))
    return model

def _make_half_model(inputs : int, kwargs : dict, hidden_dims : int = None, output_dims : int = None):
    if (hidden_dims is None) != (output_dims is None):
        warnings.warn("You'd better be explicit about the dimension of none or all of the NN layers", RuntimeWarning)
    hidden_dims = value_or(hidden_dims, round(2.0 * math.sqrt(inputs)))
    output_dims = value_or(output_dims, round(1.5 * math.sqrt(inputs)))
    layers = list()
    layers.append(_keras.layers.Input(shape=singleton(inputs), dtype=_FP_TYPE, name='in'))
    layers.append(_keras.layers.Dropout(0.50, dtype=_FP_TYPE, name='do1')(layers[-1]))
    layers.append(_keras.layers.Dense(hidden_dims, **kwargs, activation='linear', name='l1')(layers[-1]))
    layers.append(_keras.layers.Dropout(0.25, dtype=_FP_TYPE, name='do2')(layers[-1]))
    layers.append(_keras.layers.Dense(output_dims, **kwargs, activation='relu', name='l2')(layers[-1]))
    (il, *hidden, ol) = layers
    return _keras.models.Model(inputs=il, outputs=ol, name='shared')

def _train_model(model, data : _DataSet, epochs : int = 100):
    logging.info("Training NN model for {:,d} epochs ...".format(epochs))
    t0 = time.time()
    history = model.fit([ data.lhs, data.rhs, data.aux ], data.out, verbose=0, validation_split=0.25, epochs=epochs)
    t1 = time.time()
    logging.info("Training took {:.3f} seconds".format(t1 - t0))

def _test_model(model, data : _DataSet) -> np.array:
    logging.info("Testing NN model ...")
    t0 = time.time()
    predictions = model.predict([ data.lhs, data.rhs, data.aux ])
    assert predictions.shape == data.out.shape
    errors = predictions - data.out
    errmean = errors.mean()
    errstdev = errors.std()
    testsize = predictions.size
    hits = (predictions * data.out >= 0.0).sum()
    t1 = time.time()
    logging.info("Testing took {:.3f} seconds".format(t1 - t0))
    logging.info("Success rate: {:,d} of {:,d} ({:.3f} %)".format(hits, testsize, 100.0 * hits / testsize))
    logging.info("Prediction error: {:.3f} % +/- {:.3f} %".format(100.0 * errmean, 100.0 * errstdev))
    return predictions

def _get_worse_max(mngr, curs):
    return {
        r[0] : r[1] for r in mngr.sql_exec_curs(
            curs, "SELECT `method`, MAX(`rate`) FROM `WorseLayouts` GROUP BY `method`", tuple()
        )
    }

def _rank_layout(layout : Layouts) -> float:
    assert layout is None or type(layout) is Layouts
    return _LAYOUT_RATINGS.get(layout)

def _rank_layouts(lhs : Layouts, rhs : Layouts) -> float:
    l = _rank_layout(lhs)
    r = _rank_layout(rhs)
    if l is None or r is None:
        return None
    return (r - l) / 2.0

def _get_normalizers(data):
    assert len(data.shape) == 2
    n = data.shape[1]
    stats = np.zeros((n, 2), dtype=_FP_TYPE)
    for i in range(n):
        feature = data[:, i]
        missing = np.isnan(feature)
        valcount = (~missing).sum()
        stats[i][0] = feature[~missing].mean() if valcount >= 1 else math.nan
        stats[i][1] = feature[~missing].std()  if valcount >= 3 else math.nan
    return stats

def _normalize_data(norm, data):
    assert len(norm.shape) == 2
    assert len(data.shape) == 2
    assert norm.shape == (data.shape[1], 2)
    n = norm.shape[0]
    for i in range(n):
        feature = data[:, i]
        (mean, stdev) = norm[i]
        if not math.isnan(mean):
            feature -= mean
        if math.isfinite(stdev) and stdev > 0.0:
            feature /= stdev
        np.nan_to_num(feature, copy=False)
    return data

def _dump_model_infos(*models, directory : str = None):
    if directory is None:
        return
    for model in models:
        txtfile = os.path.join(directory, model.name + '.txt')
        logging.info("Writing NN model summary file {!r} ...".format(txtfile))
        with open(txtfile, 'w') as ostr:
            model.summary(print_fn=(lambda s : print(s, file=ostr)))
        pdffile = os.path.join(directory, model.name + '.pdf')
        logging.info("Writing NN model plot to file {!r} ...".format(pdffile))
        try:
            _keras.utils.plot_model(model, show_shapes=True, to_file=pdffile)
        except ImportError as e:
            logging.critical(str(e))

def _dump_corpus_infos(training, testing, directory : str = None):
    count = len(training) + len(testing)
    percentage = (lambda n : 100.0 * n / count) if count > 0 else (lambda n : math.nan)
    bias_training = training.bias()
    bias_testing  = testing.bias()
    logging.info("Loaded corpus with {:,d} labeled pairs".format(count))
    logging.info("Using {:,d} events ({:.3f} %) for training".format(len(training), percentage(len(training))))
    logging.info("Using {:,d} events ({:.3f} %) for testing ".format(len(testing ), percentage(len(testing ))))
    logging.info("Training set has {pos:,d} / {neg:,d} distribution (bias = {bia:.5f})".format(**bias_training))
    logging.info("Testing  set has {pos:,d} / {neg:,d} distribution (bias = {bia:.5f})".format(**bias_testing ))
    if directory is not None:
        summary = {
            'size' : count,
            'positive' : bias_training['pos'] + bias_testing['pos'],
            'negitive' : bias_training['neg'] + bias_testing['neg'],
            'bias' : (bias_training['bia'] + bias_testing['bia']) / 2.0,
        }
        jsonfile = os.path.join(directory, 'corpus.json')
        logging.info("Writing data corpus summary file {!r} ...".format(jsonfile))
        with open(jsonfile, 'w') as ostr:
            json.dump(summary, ostr, indent=4)
            ostr.write('\n')

def _train_alternatives(mngr : Manager, data : _DataSet, info : list):
    logging.info("Training alternative {:s} ...".format(Tests.HUANG.name))
    t0 = time.time()
    _train_alternative_huang(mngr, data, info)
    t1 = time.time()
    logging.info("Training {:s} took {:.3f} seconds".format(Tests.HUANG.name, t1 - t0))

def _train_alternative_huang(mngr : Manager, data : _DataSet, info : list):
    ctx, matrix, weights = _load_huang_context_with_matrix_and_weights_vector(mngr, info)
    def function(x):
        n = len(info)
        theweights = (x / abs(x).sum()).reshape(-1, 1)
        prediction = np.matmul(matrix, theweights)
        failurerate = ((prediction.reshape(-1) * data.out.reshape(-1)) < 0).sum() / n
        logging.debug("{:8.3f} % failures in {:,d} for weights [{:s}]".format(
            100.0 * failurerate, n, ', '.join(format(w, '.5f') for w in theweights.reshape(-1))
        ))
        return failurerate
    result = scipy_opt_minimize(function, weights, tol=0.5E-3, method='Nelder-Mead')
    if not result.success:
        raise FatalError("Numeric optimization for {:s} parameters did not converge".format(Tests.HUANG.name))
    normal = abs(result.x).sum()
    for (i, m) in enumerate(ctx.metrics):
        ctx.weights[m] = float(result.x[i]) / normal
    logging.info("Final HUANG parameters (found after {:,d} iterations):".format(getattr(result, 'nit', 0)))
    for m in ctx.metrics: logging.info("  {:30s} {:10.5f}".format(m.name, ctx.weights[m]))
    logging.info("Final failure rate was {:.3f} %".format(100.0 * getattr(result, 'fun', math.nan)))
    with open(mngr.alt_huang_params, 'wb') as ostr:
        logging.info("Saving {:s} parameters to file {!r} ...".format(Tests.HUANG.name, ostr.name))
        pickle_objects(ostr, ctx)

def _test_alternatives(mngr : Manager, info : list):
    for test in filter(Tests.is_alternative, Tests):
        logging.info("Considering competing measure {:s} ...".format(test.name))
        values = np.zeros((len(info), 1), dtype=_FP_TYPE)
        with mngr.sql_ctx as curs:
            ctx = get_alternative_context(mngr, curs, test)
            if hasattr(ctx, 'populate_cache'):
                ctx.populate_cache(mngr, curs)
            for (i, (lhs, rhs)) in enumerate(info):
                values[i][0] = value_or(get_alternative_value(mngr, curs, ctx, lhs, rhs), math.nan)
            _check_for_non_finite_feature_vectors(values, what=ctx.test.name)
            _save_test_score(mngr, curs, test, values, info)

def _load_huang_context_with_matrix_and_weights_vector(mngr : Manager, info : list):
    ctx = AlternativeContextHuang()
    with mngr.sql_ctx as curs:
        ctx.populate_cache(mngr, curs)
    matrix = np.zeros((len(info), len(ctx.metrics)), dtype=_FP_TYPE)
    for (i, (lhs, rhs)) in enumerate(info):
        stats = ctx.get_cached_mean_and_stdev(lhs, rhs)
        for (j, mtr) in enumerate(ctx.metrics):
            (mean, stdev) = (None, None) if stats is None else stats[mtr]
            lhsval = ctx.get_cached_value(lhs, mtr)
            rhsval = ctx.get_cached_value(rhs, mtr)
            if lhsval is None or rhsval is None or None in (lhsval, rhsval, stdev):
                matrix[i][j] = math.nan
            elif stdev > 0.0:
                matrix[i][j] = (lhsval - rhsval) / stdev
            else:
                assert lhsval == rhsval
                matrix[i][j] = 0.0
    _check_for_non_finite_feature_vectors(matrix, what=ctx.test.name)
    np.nan_to_num(matrix, copy=False)
    weights = np.array([ ctx.weights[m] for m in ctx.metrics ], dtype=_FP_TYPE)
    return ctx, matrix, weights

def _get_same_graph_id(mapping, *layouts):
    graphs = { mapping[lid] for lid in layouts }
    assert len(graphs) == 1
    return next(iter(graphs))

def _write_test_summary(mngr, filename=None):
    if filename is None: return
    confusions = dict()
    results = collections.defaultdict(dict)
    for row in mngr.sql_select('TestScores'):
        results[(row['lhs'], row['rhs'])][row['test']] = row['value']
    for test in Tests:
        tally = { ts : 0 for ts in TestStatus }
        for (pair, rslt) in results.items():
            (lhs, rhs) = pair
            try: (exp, act) = (rslt[Tests.EXPECTED], rslt[test])
            except KeyError: continue
            tally[NNTestResult(lhs, rhs, exp, act).status] += 1
        confusions[enum_to_json(test)] = { enum_to_json(k) : v for (k, v) in tally.items() }
    with open(filename, 'w') as ostr:
        logging.info("Writing test summary to file {!r} ...".format(ostr.name))
        json.dump(confusions, ostr, indent=4)
        ostr.write('\n')

class _DebugMain(AbstractMain):

    def _run(self, ns):
        config = Configuration(ns.configdir)
        if ns.debugdir is not None:
            os.environ[_DEBUG_DIR_ENVVAR] = ns.debugdir
        with Manager(absbindir=ns.bindir, absdatadir=ns.datadir, config=config) as mngr:
            _setup(mngr)
            with mngr.sql_ctx as curs:
                lf = _layout_features(mngr, curs)
                gf = _graph_features(mngr, curs)
            if ns.model:
                _build_model(mngr, inputs=len(lf), auxinputs=len(gf))
            if ns.corpus:
                _load_training_and_testing_data(mngr, persist=False, strict=False)

    def _argparse_hook_before(self, ap):
        ag = ap.add_argument_group("Debugging")
        ag.add_argument('--model', action='store_true', help="build NN model (without training or testing it)")
        ag.add_argument('--corpus', action='store_true', help="load corpus of labeled data (without actually using it)")
        ag.add_argument('--debugdir', '-d', metavar='DIR', help="dump information into DIR")

    def _argparse_hook_after(self, ns):
        if _DEBUG_DIR_ENVVAR in os.environ and ns.debugdir is not None:
            logging.warning("Environment variable {!s} is set but will have no effect".format(_DEBUG_DIR_ENVVAR))

if __name__ == '__main__':
    kwargs = {
        'usage' : "%(prog)s [--model] [--corpus] [--debugdir=DIR]",
        'description' : "Partially runs the driver for the model without mutating the persistent database.",
    }
    with _DebugMain(**kwargs) as app:
        app(sys.argv[1 : ])
