#! /usr/bin/python3
#! -*- coding:utf-8; mode:python; -*-

# Copyright (C) 2018 Moritz Klammler <moritz.klammler@student.kit.edu>
#
# Copying and distribution of this file, with or without modification, are permitted in any medium without royalty
# provided the copyright notice and this notice are preserved.  This file is offered as-is, without any warranty.

import argparse
import json
import sys

def main():
    ap = argparse.ArgumentParser(description="Convert Json meta data into TeX definitions.")
    ap.add_argument('src', metavar='FILE', nargs='?', default='-', help="JSON file with generator meta data to read")
    ap.add_argument('--rdf-local', metavar='FILE', help="JSON file with RDF_LOCAL meta data to read")
    ap.add_argument('-o', '--output', dest='dst', metavar='FILE', default='-', help="TeX file to write")
    ns = ap.parse_args()
    if not ns.src or ns.src == '-':
        info = json.load(sys.stdin)
    else:
        with open(ns.src, 'r') as istr:
            info = json.load(istr)
    if ns.rdf_local is not None:
        if ns.rdf_local == '-':
            raise ValueError("Please specify a regular file for the '--rdf-local=FILE' option")
        with open(ns.rdf_local, 'r') as istr:
            info['diameter'] = json.load(istr).get('diameter')
    if not ns.dst or ns.dst == '-':
        write_texdef(info, sys.stdout)
    else:
        with open(ns.dst, 'w') as ostr:
            write_texdef(info, ostr)

def write_texdef(info, stream):
    print(r'\def\GraphNodes{\ensuremath{', format_integer(info['nodes']), '}}', sep='', file=stream)
    print(r'\def\GraphEdges{\ensuremath{', format_integer(info['edges']), '}}', sep='', file=stream)
    try:
        print(r'\def\GraphDiameter{\ensuremath{', format_integer(info['diameter']), '}}', sep='', file=stream)
    except KeyError:
        pass

def format_integer(value):
    if value is None:
        return r'\text{\textbf{??}}'
    return format(round(value), ',d').replace(',', '\\,')

if __name__ == '__main__':
    main()
