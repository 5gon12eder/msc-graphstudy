#! /usr/bin/python3
#! -*- coding:utf-8; mode:python; -*-

# Copyright (C) 2018 Moritz Klammler <moritz.klammler@student.kit.edu>
#
# Copying and distribution of this file, with or without modification, are permitted in any medium without royalty
# provided the copyright notice and this notice are preserved.  This file is offered as-is, without any warranty.

import argparse
import collections
import contextlib
import glob
import json
import math
import os
import re
import statistics
import sys

from email.utils import formatdate as format_rfc5322

BASELINE = Ellipsis
LAZYOK = None
LAZYOK_ENVVAR = 'MSC_LAZY_EVAL_OKAY'

def main():
    ap = argparse.ArgumentParser(description="Formats success rates of competing metrics as as TeX code.")
    ap.add_argument('src', metavar='FILE', help="read from JSON file FILE")
    ap.add_argument('-p', '--pattern', metavar='GLOB', help="find results of individual runs in files matching GLOB")
    ap.add_argument('-o', '--output', metavar='FILE', type=argparse.FileType('w'), help="write to FILE")
    ap.add_argument('-r', '--rename', metavar='FILE', type=argparse.FileType('r'),
                    help="rename enums according to mapping read from FILE")
    ap.add_argument('-t', '--test', metavar='TEST', dest='tests', action='append', type=_test,
                    help="only consider TEST (may be repeated)")
    ns = ap.parse_args()
    try:
        with open(ns.src, 'r') if ns.src != '-' else contextlib.nullcontext(sys.stdin) as istr:
            info = json.load(istr)
    except FileNotFoundError:
        check_lazy_okay(ns.src)
        info = dict()
        info[const.enum_to_json(BASELINE)] = collections.defaultdict(dummy_analysis)
        info[const.enum_to_json(BASELINE)]['test-runs'] = 0
    renamings = { t : t.name for t in const.Tests }
    if ns.rename is not None:
        load_rename_table(ns.rename, renamings)
    tests = list(ns.tests) if ns.tests else sorted(const.enum_from_json(const.Tests, k) for k in info.keys())
    results = dict()
    for test in tests:
        key = const.enum_to_json(test)
        if key in info:
            results[test] = analysis_dict2tuple(info[key]['success'])
        else:
            bemoan(test.name, "No test data available")
            check_lazy_okay()
            results[test] = (math.nan, math.nan)
    if ns.pattern is None:
        individuals = None
    else:
        individuals = collections.defaultdict(list)
        for filename in glob.glob(ns.pattern):
            with open(filename, 'r') as istr:
                subinfo = json.load(istr)
            for (key, values) in subinfo.items():
                test = const.enum_from_json(const.Tests, key)
                total = sum(values.values())
                success = (values['true-positive'] + values['true-negative']) / total if total > 0 else math.nan
                individuals[test].append(success)
        if not individuals:
            bemoan("Pattern did not match any files", repr(ns.pattern))
            check_lazy_okay()
    kwargs = {
        'individuals'  : individuals,
        'renamings'    : renamings,
        'base_runs'    : info[const.enum_to_json(BASELINE)]['test-runs'],
        'base_success' : analysis_dict2tuple(info[const.enum_to_json(BASELINE)]['success']),
    }
    write_output(ns.output, results, **kwargs)

def write_output(ostr, results, individuals, renamings, base_runs, base_success):
    scriptname = os.path.basename(__file__)
    timestamp = format_rfc5322(localtime=True)
    print("% -*- coding:utf-8; mode:latex; -*-", file=ostr)
    print("", file=ostr)
    print("%% THIS IS A GENERATED FILE; PLEASE DO NOT EDIT IT MANUALLY!", file=ostr)
    print("%% Generated by {!s} on {!s}".format(scriptname, timestamp), file=ostr)
    print("", file=ostr)
    for (test, (mean, stdev)) in sorted(results.items(), key=(lambda kv : kv[1][0]), reverse=True):
        enumname = renamings[test]
        assert re.match(r'^[A-Z][0-9A-Z_]*$', enumname)
        texname = enumname.replace('_', '\\_')
        if individuals is not None and BASELINE in individuals:
            differences = [ individuals[BASELINE][i] - individuals[test][i] for i in range(base_runs) ]
            try:
                diff_mean = statistics.mean(differences)
                diff_stdev = statistics.stdev(differences, xbar=diff_mean)
            except statistics.StatisticsError:
                (diff_mean, diff_stdev) = (math.nan, math.nan)
        else:
            diff_mean = base_success[0] - results[test][0]
            diff_stdev = math.sqrt(base_success[1]**2 + results[test][1]**2)
        argtokens = ''.join(r"{" + format(100.0 * x, '.2f') + r"}" for x in [ mean, stdev, diff_mean, diff_stdev ])
        print(r"\CompetingMetricResult[\enum{" + texname + r"}]" + argtokens, file=ostr)

def dummy_analysis():
    return { 'mean' : math.nan, 'stdev' : math.nan }

def analysis_dict2tuple(info):
    return (info['mean'], info['stdev'])

def load_rename_table(istr, table):
    for line in filter(None, map(str.strip, map(lambda s : s.partition('#')[0], istr))):
        [ old, new ] = line.split()
        table[const.Tests[old]] = new

def check_lazy_okay(filename=None):
    if not LAZYOK:
        if filename is not None:
            bemoan("No such file or directory", filename)
        bemoan("Cannot continue due to missing evaluation data (set {!s}={!r} to continue anyway)"
               .format(LAZYOK_ENVVAR, 1), fatal=True)

def get_lazy_okay():
    assert LAZYOK is None
    envval = os.getenv(LAZYOK_ENVVAR, 0)
    try:
        return int(envval) > 0
    except ValueError:
        bemoan(LAZYOK_ENVVAR, "Not a valid integer", repr(envval))
        return False

def bemoan(*msg, fatal=False):
    script = os.path.basename(__file__)
    print(script, *msg, sep=": ", file=sys.stderr)
    if fatal: raise SystemExit(True)

def _test(token):
    try:
        return const.enum_from_json(const.Tests, token)
    except ValueError as e:
        raise argparse.ArgumentTypeError(str(e))

if __name__ == '__main__':
    if os.path.exists('.msc-graphstudy'):
        sys.path.append(os.getcwd())
    import driver.constants as const
    BASELINE = const.Tests.NN_FORWARD
    LAZYOK = get_lazy_okay()
    main()
