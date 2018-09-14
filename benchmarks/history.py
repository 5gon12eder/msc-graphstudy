#! /usr/bin/python3
#! -*- coding:utf-8; mode:python; -*-

# Copyright (C) 2016 Moritz Klammler <moritz.klammler@student.kit.edu>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
# documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS
# OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

import argparse
import os.path
import statistics
import sys
import time

from lib.cli import (
    TerminalSizeHack,
    add_help,
    add_history_mandatory,
)

from lib.history import (
    History,
)

def main(args):
    ap = argparse.ArgumentParser(
        prog='history',
        usage="%(prog)s -H DATABASE ACTION ...",
        description="Manage the history database of benchmark results.",
        epilog="Use 'ACTION --help' to get the help for ACTION.",
        allow_abbrev=False,
        add_help=False
    )
    actions = ap.add_subparsers(title="Actions", dest='action', metavar="")
    mandatory = ap.add_argument_group(title="Mandatory Parameters", description="")
    supplementary = ap.add_argument_group(title="Supplementary Parameters", description="")
    add_history_mandatory(mandatory)
    add_help(supplementary)
    sub_list = actions.add_parser(
        'list', usage='list', add_help=False,
        help=(
              "List all benchmarks in the database with their names and"
            + " optional descriptions."
        ),
    )
    add_help(sub_list)
    sub_export = actions.add_parser(
        'export', usage='export NAME [TOL]', add_help=False,
        help=(
              "Export history data for a benchmark in a text format that can,"
            + " for example, be given to Gnuplot or some other data-processing"
            + " software."
        ),
    )
    sub_export.add_argument(
        'name', metavar='NAME', help="Name of the benchmark to export."
    )
    sub_export.add_argument(
        'tol', metavar='TOL', type=float, nargs='?', default=None,
        help=(
            "Treat all results with relative standard deviation greater than"
            + " TOL times the median as outliers."
        )
    )
    add_help(sub_export)
    sub_drop = actions.add_parser(
        'drop', usage='drop NAME', add_help=False,
        help=(
              "Irrecoverably remove all data for a benchmark from the"
            + " database."
        )
    )
    sub_drop.add_argument(
        'name', metavar='NAME', help="Name of the benchmark to drop."
    )
    add_help(sub_drop)
    sub_drop_since = actions.add_parser(
        'drop-since', usage='drop-since TIME', add_help=False,
        help=(
              "Irrecoverably remove all results since a time point from the"
            + " database. "
        ),
    )
    sub_drop_since.add_argument(
        'time', metavar='TIME', type=int,
        help=(
            "POSIX time-stamp specifying the point after which results should"
            + " be dropped.  You can use the 'date' command line utility to"
            + " translate human friendly notions of time-points into POSIX"
            + " time stamps."
        ),
    )
    add_help(sub_drop_since)
    with TerminalSizeHack():
        ns = ap.parse_args(args)
    with History(ns.history, create=True) as histo:
        if not histo:
            raise RuntimeError("Database does not exist")
        if ns.action == 'list':
            _action_list(histo)
        elif ns.action == 'export':
            _action_export(histo, ns.name, ns.tol)
        elif ns.action == 'drop':
            histo.drop_benchmark(ns.name)
        elif ns.action == 'drop-since':
            histo.drop_since(ns.time)
        elif ns.action is None:
            print("Database is OK, there is nothing to do", file=sys.stderr)
        else:
            raise AssertionError(ns.action)

def _action_list(histo):
    summary = histo.get_descriptions()
    for name in sorted(summary.keys()):
        desc = summary[name]
        if desc is None:
            print(name)
        else:
            print('{:20s}  {:s}'.format(name, desc))

def _action_export(histo, name, tol=None):
    bench = histo.get_benchmark_results(name)
    medstdev = median_rel_stdev(bench.results)
    notok = lambda hr : is_outlier(hr, medstdev, tol)
    printhdr = lambda k, v : print('## {:30s}{}'.format(k + ':', v))
    printhdr("Benchmark Name", name)
    if bench.description is not None:
        printhdr("Description", bench.description)
    printhdr("No. of Data Points", len(bench.results))
    printhdr("Eliminated Outliers", count(filter(notok, bench.results)))
    if medstdev is not None:
        printhdr("Median of Rel. Std. Dev.", '{:.4g}'.format(medstdev))
    if bench.results:
        timefmt = '%a, %d %b %Y %H:%M:%S %z'  # RFC 2822
        first = time.localtime(min(hr.timestamp for hr in bench.results))
        last = time.localtime(max(hr.timestamp for hr in bench.results))
        printhdr("First Data Point", time.strftime(timefmt, first))
        printhdr("Last Data Point", time.strftime(timefmt, last))
    else:
        raise RuntimeError("No data points")
    print()
    print('#   {:>14s}{:>16s}{:>16s}{:>16s}'.format("timestamp", "mean / s", "stdev / s", "N"))
    print()
    for hr in sorted(bench.results, key=lambda r : r.timestamp):
        print('{:<2s}{:16d}{:16.3e}{:16.3e}{:16d}'.format(
            '#' if notok(hr) else '',
            hr.timestamp, hr.mean, hr.stdev, hr.n
        ))

def median_rel_stdev(results):
    if not results:
        return None
    return statistics.median([hr.relative_stdev for hr in results])

def is_outlier(hr, medstdev, tol):
    if None in [hr.relative_stdev, medstdev, tol]:
        return None
    return hr.relative_stdev > tol * medstdev

def count(seq):
    return sum(1 for x in seq)

if __name__ == '__main__':
    try:
        sys.exit(main(sys.argv[1:]))
    except (AssertionError, TypeError):
        raise
    except Exception as e:
        print("error:", e, file=sys.stderr)
        sys.exit(1)
