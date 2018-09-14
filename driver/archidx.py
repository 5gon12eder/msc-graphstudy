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

__all__ = [ ]

import argparse
import collections
import logging
import math
import os.path
import statistics
import sys

from .cli import *
from .configuration import *
from .constants import *
from .errors import *
from .imports import *
from .manager import *
from .utility import *

PROGRAM_NAME = 'archidx'

class _Indexer(object):

    def __init__(self, mngr):
        self.__manager = mngr

    def __call__(self, generators, ostr):
        well_known_import_sources = get_well_known_import_sources()
        configured_import_sources = mngr.config.import_sources
        self.__index = dict()
        for gen in generators:
            if gen is Generators.IMPORT and configured_import_sources is not None:
                for src in configured_import_sources:
                    graphs = self.__index_ar(gen, src)
                    self.__print_report(gen, src, graphs, ostr)
            elif gen.imported:
                src = well_known_import_sources[gen]
                graphs = self.__index_ar(gen, src)
                self.__print_report(gen, src, graphs, ostr)
            else:
                pass

    def __index_ar(self, gen : Generators, src : ImportSource):
        assert gen.imported
        executable = os.path.join(self.__manager.abs_bindir, 'src', 'generators',  'import')
        cmd = [ executable, '--output=NULL', '--meta=STDIO', '--format={:s}'.format(src.format) ]
        if src.layout: cmd.append('--layout')
        cmd.append('STDIO' if src.compression is None else 'STDIO:' + src.compression)
        graphs = list()
        try:
            with src as archive:
                archlen = len(archive)  # This is a potentially expensive operation so do it only once
                logging.info("{:s} archive {!r} contains {:d} graphs in total".format(gen.name, archive.name, archlen))
                for (i, cand) in enumerate(archive):
                    pretty = archive.prettyname(cand)
                    logging.info("[{:6.2f} %] Processing {:s} graph {:d} of {:d} from {!r} of {!r} ...".format(
                        100.0 * i / archlen, gen.name, i + 1, archlen, pretty, archive.name))
                    try:
                        with archive.get(cand) as istr:
                            meta = self.__manager.call_graphstudy_tool(cmd, meta='stdout', stdin=istr)
                    except Exception as e:
                        if not isinstance(e, RecoverableError) and not archive.is_likely_read_error(e): raise
                        logging.error("Cannot import graph {!r} from {!r}: {!s}".format(pretty, archive.name, e))
                        continue
                    graphs.append(meta)
                logging.info("[{:6.2f} %] Processed {:d} {:s} graphs in {!r}".format(
                    100.0, archlen, gen.name, archive.name))
        except RecoverableError as e:
            logging.error("Cannot index {:s} archive {!r}: {!s}".format(gen.name, src.name, e))
        return graphs

    def __print_report(self, gen : Generators, src : ImportSource, graphs, ostr):
        notzero = lambda x : x if x > 0 else math.nan
        _mean  = lambda items : float(statistics.mean(items))  if len(items) >= 1 else math.nan
        _stdev = lambda items : float(statistics.stdev(items)) if len(items) >= 3 else math.nan
        nodes = [meta['nodes'] for meta in graphs]
        edges = [meta['edges'] for meta in graphs]
        sparsities = [meta['edges'] / notzero(meta['nodes'])**2 for meta in graphs]
        summary = list()
        summary.append(("generator", gen.name))
        summary.append(("archive", src.name))
        summary.append(("total graphs", len(graphs)))
        summary.append(("unique graph fingerprints", len(set(meta['graph'] for meta in graphs))))
        summary.append(("unique layout fingerprints", len(set(filter(None, (m.get('layout') for m in graphs))))))
        summary.append(("average number of nodes", _mean(nodes), _stdev(nodes)))
        summary.append(("average number of edges", _mean(edges), _stdev(edges)))
        summary.append(("average sparsity", _mean(sparsities), _stdev(sparsities)))
        sizes = collections.Counter(map(GraphSizes.classify, nodes))
        for size in sorted(sizes.keys()):
            summary.append((size.name, sizes[size]))
        self.__print_report_summary(summary, len(graphs), ostr)

    def __print_report_summary(self, summary, total, ostr):
        for row in summary:
            if len(row) == 2:
                (header, value) = row
                if isinstance(value, int):
                    if total > 0:
                        print('{:30s} {:10d}  {:10.2f} %'.format(header, value, 100.0 * value / total), file=ostr)
                    else:
                        print('{:30s} {:10d}'.format(header, value), file=ostr)
                else:
                    assert isinstance(value, str)
                    print('{:30s} {:s}'.format(header, value), file=ostr)
            elif len(row) == 3:
                (header, mean, stdev) = row
                assert isinstance(mean, float) and isinstance(stdev, float)
                print('{:30s} {:>10s}  +/- {:>8s}'.format(header, str(Real(mean)), str(Real(stdev))), file=ostr)
            else:
                assert False
        print(file=ostr)

class Main(AbstractMain):

    def _run(self, ns):
        config = Configuration(ns.configdir)
        ostr = None
        try:
            if ns.output == '-':
                ostr = sys.stdout
            else:
                ostr = open(ns.output, 'w')
                logging.info("Writing report to {!r} ...".format(ns.output))
            with Manager(absbindir=ns.bindir, absdatadir=ns.datadir, config=config) as mngr:
                indexer = _Indexer(mngr)
                indexer(ns.generators, ostr)
        finally:
            if ostr is not None and ostr is not sys.stdout:
                ostr.close()

    def _argparse_hook_before(self, ap):
        ap.add_argument(
            'generators', metavar='GEN', type=parse_generator, nargs='*',
            help="index only the specified generators (default: index all)"
        )
        ap.add_argument(
            '--output', '-o', metavar='FILE', type=str, default='-',
            help="write output to FILE (default: standard output)"
        )

    def _argparse_hook_after(self, ns):
        ns.generators = frozenset(ns.generators if ns.generators else Generators)

def parse_generator(text):
    try:
        g = enum_from_json(Generators, text)
        if g.imported: return g
    except ValueError:
        pass
    valid = ' '.join(enum_to_json(g) for g in Generators if g.imported)
    raise argparse.ArgumentTypeError("Valid choices are: " + valid)

if __name__ == '__main__':
    kwargs = {
        'prog' : PROGRAM_NAME,
        'usage' : "%(prog)s [-o FILE] [SOURCE...]",
        'description' : "Shows an index of a graph import source (archive).",
    }
    with Main(**kwargs) as app:
        app(sys.argv[1 : ])
