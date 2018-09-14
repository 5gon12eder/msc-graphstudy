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
import datetime
import doctest
import importlib
import pkgutil
import sys
import time

class _Status(object):

    def __init__(self, failed=0, attempted=0):
        assert failed <= attempted
        self.failed = failed
        self.attempted = attempted

    def update(self, other):
        self.failed += other.failed
        self.attempted += other.attempted

    @property
    def passed(self):
        return self.attempted - self.failed

    def print_report(self, ostr, elapsed, prefix='', color=False):
        print(prefix + "{:6d}  tests run in {}".format(self.attempted, elapsed), file=ostr)
        if self.attempted > 0:
            (redon, redoff) = ('\033[31m', '\033[39m') if color and self.failed else ('', '')
            (greenon, greenoff) = ('\033[32m', '\033[39m') if color and self.attempted else ('', '')
            relpassed = 100.0 * self.passed / self.attempted
            relfailed = 100.0 * self.failed / self.attempted
            print(prefix + greenon + "{:6d}  ({:6.2f} %) passed".format(self.passed, relpassed) + greenoff, file=ostr)
            print(prefix + redon + "{:6d}  ({:6.2f} %) failed".format(self.failed, relfailed) + redoff, file=ostr)

def _recursively_run_doctests(components, gstatus, verbose=False, color=False, level=0):
    modcount = 0
    pkgqname = '.'.join(components)
    pkgpath = '/'.join(components)
    oldindent = '\t' * level
    indent = '\t' * (level + 1)
    (purpleon, purpleoff) = ('\033[35m', '\033[39m') if color else ('', '')
    print(oldindent + "Running doctests for package {!r} ...".format(pkgqname), file=sys.stderr)
    print(file=sys.stderr)
    worklist = [(None, '__init__', False)]
    worklist.extend(pkgutil.iter_modules([pkgpath]))
    for (finder, name, ispkg) in worklist:
        subcomponents = [*components, name]
        subqname = '.'.join(subcomponents)
        subpath = '/'.join(subcomponents)
        if ispkg:
            modcount += _recursively_run_doctests(
                subcomponents, gstatus, verbose=verbose, color=color, level=(level + 1)
            )
            continue
        modcount += 1
        print(indent + "Running doctests for module {!r} ...".format(subqname), file=sys.stderr)
        module = importlib.import_module(subqname, package=pkgqname)
        t0 = time.perf_counter()
        sys.stdout.write(purpleon)
        result = doctest.testmod(module, name=subqname, verbose=verbose)
        sys.stdout.write(purpleoff)
        t1 = time.perf_counter()
        status = _Status(failed=result.failed, attempted=result.attempted)
        gstatus.update(status)
        status.print_report(sys.stderr, datetime.timedelta(seconds=round(t1 - t0)), prefix=indent, color=color)
        print(file=sys.stderr)
    return modcount

if __name__ == '__main__':
    ap = argparse.ArgumentParser(description="Run doctests for the driver.", add_help=False)
    ap.add_argument('-c', '--color', action='store_true', help="use ANSI escape sequences to colorize output")
    ap.add_argument('-v', '--verbose', action='store_true', help="verbose output")
    ap.add_argument('--help', action='help', help="show usage information and exit")
    ap.add_argument('--version', action='version', version=__file__, help="show version information and exit")
    ns = ap.parse_args()
    status = _Status(0, 0)
    t0 = time.perf_counter()
    modcount = _recursively_run_doctests(__package__.split('.'), status, verbose=ns.verbose, color=ns.color)
    t1 = time.perf_counter()
    print("Completed running doctests for {:d} modules".format(modcount))
    status.print_report(sys.stderr, datetime.timedelta(seconds=round(t1 - t0)), color=ns.color)
    sys.exit(status.failed > 0)
