#! /usr/bin/python3
#! -*- coding:utf-8; mode:python; -*-

# Copyright (C) 2018 Moritz Klammler <moritz.klammler@student.kit.edu>
#
# Copying and distribution of this file, with or without modification, are permitted in any medium without royalty
# provided the copyright notice and this notice are preserved.  This file is offered as-is, without any warranty.

import argparse
import json
import math

def main():
    ap = argparse.ArgumentParser(description="Convert Json meta data into Gnuplot input")
    ap.add_argument('src', metavar='FILE', help="JSON file with meta data to read")
    ap.add_argument('-o', '--output', metavar='FILE', type=argparse.FileType('w'), help="data file to write")
    ap.add_argument('-r', '--regression', metavar='FILE', type=argparse.FileType('w'), help="regression file to write")
    ap.add_argument('-l', '--label', metavar='NAME', help="write regression data using this label")
    ns = ap.parse_args()
    with open(ns.src, 'r') as istr:
        info = json.load(istr)
    label = None if ns.label is None else ns.label.replace('-', '_')
    # Regression file
    fname = 'f' if label is None else 'f_' + label
    c0name = 'c0' if label is None else 'c0_' + label
    c1name = 'c1' if label is None else 'c1_' + label
    print("#! /usr/bin/gnuplot", file=ns.regression)
    print("#! -*- coding:utf-8; mode:shell-script; -*-", file=ns.regression)
    print("", file=ns.regression)
    print("{:s}(x) = {:s} + {:s} * x".format(fname, c0name, c1name), file=ns.regression)
    print(
        "fit {fun:s}(x) {dat!r} via {c0:s}, {c1:s}".format(fun=fname, c0=c0name, c1=c1name, dat=ns.output.name),
        file=ns.regression
    )
    # Data file
    print("#! /usr/bin/cat", file=ns.output)
    print("#! -*- coding:utf-8; mode:fundamental; -*-", file=ns.output)
    print("", file=ns.output)
    for subinfo in info['data']:
        vicinity = subinfo['vicinity']
        for subsubinfo in subinfo['data']:
            entropy = subsubinfo['entropy']
            print("{:20.10E} {:20.10E}".format(math.log2(vicinity), entropy), file=ns.output)

if __name__ == '__main__':
    main()
