#! /usr/bin/python3
#! -*- coding:utf-8; mode:python; -*-

# Copyright (C) 2018 Moritz Klammler <moritz.klammler@student.kit.edu>
#
# Copying and distribution of this file, with or without modification, are permitted in any medium without royalty
# provided the copyright notice and this notice are preserved.  This file is offered as-is, without any warranty.

import argparse
import json
import math
import subprocess

def main():
    ap = argparse.ArgumentParser(description="Call picture tool with principal components from JSON file")
    ap.add_argument('-1', '--major', metavar='FILE', help="JSON file to read 1st component from")
    ap.add_argument('-2', '--minor', metavar='FILE', help="JSON file to read 2nd component from")
    ap.add_argument('-i', '--input', metavar='FILE', help="GraphML file to draw")
    ap.add_argument('-o', '--output', metavar='FILE', help="TeX file to write")
    ap.add_argument('picture', metavar='EXECUTABLE', help="executable to run")
    ns = ap.parse_args()
    cmd = [ ns.picture, '--tikz' ]
    if ns.output is not None:
        cmd.append('--output={:s}'.format(ns.output))
    if ns.major is not None:
        major = get_princomp(ns.major)
        cmd.append('--major=({:.20E}, {:.20E})'.format(*major))
    if ns.minor is not None:
        minor = get_princomp(ns.minor)
        cmd.append('--minor=({:.20E}, {:.20E})'.format(*minor))
    if ns.input is not None:
        cmd.append(ns.input)
    stat = subprocess.run(cmd)
    raise SystemExit(stat.returncode)

def get_princomp(filename):
    with open(filename, 'r') as istr:
        info = json.load(istr)
    n = info['size']
    stdev = math.sqrt(n / (n - 1) * info['rms']**2 - info['mean']**2)
    (x, y) = info['component']
    return (stdev * x, stdev * y)

if __name__ == '__main__':
    main()
