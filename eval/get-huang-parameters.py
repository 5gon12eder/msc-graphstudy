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
import json
import os
import pickle
import sys

def main():
    ap = argparse.ArgumentParser(description="Extracts Huang parameters")
    ap.add_argument('-o', '--output', metavar='FILE', required=True, help="write parameters to FILE")
    ap.add_argument('-f', '--file', metavar='FILE', required=True, help="find parameters in JSON file FILE")
    ns = ap.parse_args()
    with open(ns.file, 'rb') as istr:
        print("Loading Huang parameters from file {!r} ...".format(istr.name), file=sys.stderr)
        timestamp = pickle.load(istr)
        params = pickle.load(istr)
    print("Huang parameters were saved on {!s}".format(timestamp), file=sys.stderr)
    info = dict()
    for (k, v) in params.weights.items():
        info[k.name.lower().replace('_', '-')] = (v, 0.0)
    with open(ns.output, 'w') as ostr:
        print("Writing Huang parameters to file {!r} ...".format(ostr.name), file=sys.stderr)
        json.dump(info, ostr, indent=4)
        ostr.write('\n')

if __name__ == '__main__':
    sys.path.append(os.getcwd())
    main()
