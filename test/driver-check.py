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

from sys import hexversion as PYTHON_HEXVERSION
if PYTHON_HEXVERSION < 0x30602f0:
    raise Exception("Your Python interpreter is too old; Python 3.6 or newer is required for this program to run")

import argparse
import binascii
import os.path
import sqlite3
import sys

def check_database_exists():
    if not os.path.isfile('graphstudy.db'):
        raise AssertionError("The database file does not exist")

def check_graphs():
    with sqlite3.connect('graphstudy.db') as conn:
        graphs = { r[0] : (r[1], r[2]) for r in conn.execute("SELECT `id`, `file`, `generator`  FROM `Graphs`") }
        # Print it so we still have something to debug after the test directory is deleted.
        for (k, v) in graphs.items():
            print('{:s} {:2d} {!r}'.format(b2s(k), v[1], v[0]))
        expected = 3
        actual = len(graphs)
        if expected != actual:
            raise AssertionError("Expected {:d} graphs in the database but found {:d}".format(expected, actual))
        expected_gens = { 0, 10, 23 }
        actual_gens = set(g for (f, g) in graphs.values())
        if expected_gens != actual_gens:
            raise AssertionError("Expected generators {!r} but found {!r}".format(expected_gens, actual_gens))

def check_graph_files():
    with sqlite3.connect('graphstudy.db') as conn:
        for row in conn.execute("SELECT `file`, `generator`  FROM `Graphs`"):
            if not os.path.isfile(row[0]):
                raise AssertionError("Graph file {!r} does not exist in the file system".format(row[0]))

def check_layouts():
    with sqlite3.connect('graphstudy.db') as conn:
        layouts = {
            r[0] : (r[1], r[2], r[3])
            for r in conn.execute("SELECT `id`, `graph`, `layout`, `file` FROM `Layouts` WHERE `layout` IS NOT NULL")
        }
        # Print it so we still have something to debug after the test directory is deleted.
        for (k, v) in layouts.items():
            print('{:s} {:s} {:2d} {!r}'.format(b2s(k), b2s(v[0]), v[1], v[2]))
        expected = 5
        actual = len(layouts)
        if expected != actual:
            raise AssertionError("Expected {:d} layouts in the database but found {:d}".format(expected, actual))

def b2s(b):
    return binascii.hexlify(b).decode('ascii')

ALL_CHECKS = [
    (check_database_exists, "Checking the graph database"),
    (check_graphs, "Checking the graphs in the database"),
    (check_graph_files, "Checking the graph files on disk"),
    (check_layouts, "Checking the layouts in the database"),
]

def main(args):
    ap = argparse.ArgumentParser(
        description="Checks the results of the driver test run in the current working directory.",
        epilog="This script is used by the build system but probably not useful when run by yourself.",
    )
    ns = ap.parse_args(args)
    failures = 0
    for (function, message) in ALL_CHECKS:
        print("{:s} ...".format(message), file=sys.stderr)
        try:
            function()
        except AssertionError as e:
            failures += 1
            (ansion, ansioff) = ('\033[31m', '\033[39m')
            print(ansion, "FAILED: {!s}".format(e), ansioff, sep='', file=sys.stderr)
    return failures > 0

if __name__ == '__main__':
    sys.exit(main(sys.argv[1 : ]))
