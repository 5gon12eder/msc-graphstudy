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

import argparse
import collections
import datetime
import glob
import json
import math
import os
import pickle
import shlex
import statistics
import subprocess
import sys
import tempfile

class Confusion(object):

    def __init__(self, tn, fn, fp, tp, total):
        assert total >= 0
        if total > 0:
            def relative(n):
                assert n >= 0
                return float(n) / total
        elif total == 0:
            def relative(n):
                assert n == 0
                return math.nan
        else:
            raise AssertionError(repr(total))
        assert tn + fn + fp + tp <= total
        self.test_count = total
        self.true_negative = relative(tn)
        self.false_negative = relative(fn)
        self.false_positive = relative(fp)
        self.true_positive = relative(tp)
        self.condition_negative = relative(tn + fp)
        self.condition_positive = relative(tp + fn)
        self.prediction_negative = relative(tn + fn)
        self.prediction_positive = relative(tp + fp)
        self.success = relative(tn + tp)
        self.failure = relative(fn + fp)

def main():
    ap = argparse.ArgumentParser(
        description="Perfoms Monte Carlo cross validation and outputs a confusion matrix with estimated errors"
    )
    ap.add_argument('-r', '--runs', metavar='N', type=int, default=10, help="repeat N times")
    ap.add_argument('-t', '--test', metavar='TEST', dest='tests', action='append',
                    help="only consider test TEST (may be repeated)")
    ap.add_argument('-o', '--output', metavar='FILE', help="write confusion matrix to FILE")
    ap.add_argument('-p', '--huang', metavar='FILE', help="write Huang parameters to FILE")
    ap.add_argument('-d', '--directory', metavar='DIR', help="keep intermediate results in DIR (must exist)")
    ap.add_argument('-q', '--quiet', action='store_true', help="discard driver logging output")
    ap.add_argument('-s', '--single', action='store_true', help="produce backwards compatible output (deprecated)")
    ap.add_argument('--puncture', metavar='N', type=int, required=True, help="tell driver to expect N puncture")
    ap.add_argument('--datadir', metavar='DIR', default=os.curdir,
                    help="data directory used by the driver command (default: %(default)s)")
    ap.add_argument('cmd', nargs='+', help="command to execute for training and testing the model")
    ns = ap.parse_args()
    result, weights = cross_validate(
        ns.cmd, ns.runs,
        quiet=ns.quiet, directory=ns.directory, single=ns.single, puncture=ns.puncture,
        tests=({ s.lower().replace('_', '-') for s in ns.tests } if ns.tests else None),
        huangfile=(None if ns.huang is None else os.path.join(ns.datadir, 'model', 'huang.pickle')),
    )
    def writeout(obj, stream):
        json.dump(obj, stream, sort_keys=True, indent=4)
        stream.write('\n')
    if ns.output is None or ns.output == '-':
        writeout(result, sys.stdout)
    else:
        with open(ns.output, 'w') as ostr:
            writeout(result, ostr)
    if ns.huang is not None:
        if ns.huang == '-':
            writeout(weights, sys.stdout)
        with open(ns.huang, 'w') as ostr:
            writeout(weights, ostr)

def cross_validate(command, runs, quiet=False, tests=None, single=False, directory=None, huangfile=None, puncture=None):
    results = collections.defaultdict(list)
    huangweights = collections.defaultdict(list)
    print("Performing cross validation via random subsampling in {:d} runs".format(runs), file=sys.stderr)
    #print(' '.join(map(shlex.quote, command)), file=sys.stderr)
    with tempfile.TemporaryDirectory() as tmpdir:
        print("Using temporary directory {!r}".format(tmpdir), file=sys.stderr)
        if directory is None:
            directory = tmpdir
        for old in sorted(glob.glob(os.path.join(glob.escape(directory), 'xeval-*.json'))):
            print("DEL {:s}".format(old), file=sys.stderr)
            os.remove(old)
        subprocenv = dict(os.environ, MSC_PUNCTURE=str(puncture))
        subprockwargs = {
            'check'  : True,
            'stdin'  : subprocess.DEVNULL,
            'stdout' : subprocess.DEVNULL,
            'stderr' : subprocess.DEVNULL if quiet else None,
            'env'    : subprocenv,
        }
        for i in range(runs):
            print("Running cross validation {:d} / {:d} ...".format(i + 1, runs), file=sys.stderr)
            jsonfile = subprocenv['MSC_NN_TEST_SUMMARY'] = os.path.join(directory, 'xeval-{:03d}.json'.format(i + 1))
            subprocess.run(command, **subprockwargs)
            with open(jsonfile, 'r') as istr:
                summary = json.load(istr)
            infokeys = tests if tests is not None else set(summary.keys())
            for test in infokeys:
                results[test].append(get_confusion(summary[test]))
            if huangfile is not None:
                with open(huangfile, 'rb') as istr:
                    timestamp = pickle.load(istr)
                    for (k, v) in pickle.load(istr).weights.items():
                        huangweights[k.name.lower().replace('_', '-')].append(v)
    print("Combining {:d} cross validation results".format(runs), file=sys.stderr)
    weights = None if huangfile is None else combine_huang_weights(huangweights)
    return combine_confusions(results, single=single), weights

def get_confusion(summary):
    return Confusion(
        tn=summary['true-negative'],
        fn=summary['false-negative'],
        fp=summary['false-positive'],
        tp=summary['true-positive'],
        total=sum(summary.values())
    )

def combine_confusions(confusions, single=False):
    results = dict()
    for test in confusions.keys():
        info = dict()
        info['test-runs'] = len(confusions[test])
        attributes = [
            'test-count', 'success', 'failure',
            'true-negative', 'false-negative', 'false-positive', 'true-positive',
            'condition-positive', 'condition-negative',
            'prediction-positive', 'prediction-negative',
        ]
        for what in attributes:
            (mean, stdev) = make_statistic(confusions[test], what.replace('-', '_'))
            info[what] = { 'mean' : mean, 'stdev' : stdev }
        results[test] = info
    if single:
        assert len(results) == 1
        return next(iter(results.values()))
    return results

def combine_huang_weights(huangweights, keys=None):
    if keys is None:
        keys = list(huangweights.keys())
    results = dict()
    for key in keys:
        values = list(huangweights[key])
        mean = statistics.mean(values)
        stdev = statistics.stdev(values, xbar=mean)
        results[key] = { 'values' : values, 'mean' : mean, 'stdev' : stdev }
    return results

def make_statistic(confusions, key):
    values = [ getattr(c, key) for c in confusions ]
    mean = statistics.mean(values)
    stdev = statistics.stdev(values, xbar=mean)
    return (mean, stdev)

def get_one(seq):
    [ x ] = seq
    return x

if __name__ == '__main__':
    if os.path.exists('.msc-graphstudy'):
        sys.path.append(os.getcwd())
    main()
