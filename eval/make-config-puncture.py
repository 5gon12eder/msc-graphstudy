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
import os
import sys

def main():
    ap = argparse.ArgumentParser(description="Creates a 'punctures.cfg' file.")
    ap.add_argument(
        '-x', '--exclude', metavar='PROP', type=_property, action='append', default=list(),
        help="exclude property PROP"
    )
    ap.add_argument(
        '-y', '--include', metavar='PROP', type=_property, action='append', default=list(),
        help="include property PROP"
    )
    ap.add_argument('-o', '--output', metavar='FILE', help="write output to FILE")
    ns = ap.parse_args()
    if bool(ns.include) == bool(ns.exclude):
        raise RuntimeError("Please use either the '--include' or the '--exclude' option (but not both or none)")
    if ns.include:
        punctures = set(Properties) - set(ns.include)
    if ns.exclude:
        punctures = set(ns.exclude)
    if ns.output is None or ns.output == '-':
        print_punctures(punctures)
    else:
        with open(ns.output, 'w') as ostr:
            print_punctures(punctures, ostr)

def _property(token):
    try:
        return Properties[token.upper().replace('-', '_')]
    except KeyError:
        raise argparse.ArgumentTypeError("Unknown property")

def print_punctures(punctures, ostr=None):
    print("#! /bin/false", file=ostr)
    print("#! -*- coding:utf-8; mode:conf-space; -*-", file=ostr)
    print("", file=ostr)
    for prop in sorted(punctures):
        print(prop.name, file=ostr)

def canonical(s):
    return s.strip().replace('-', '_').upper()

if __name__ == '__main__':
    if os.path.exists('.msc-graphstudy'):
        sys.path.append(os.getcwd())
    from driver.constants import *
    main()
