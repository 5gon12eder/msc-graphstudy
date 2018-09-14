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

__all__ = [ 'do_graphs' ]

import collections
import logging
import os

from .configuration import *
from .constants import *
from .errors import *
from .imports import *
from .manager import *
from .utility import *

_GEN_PROGS = {
    Generators.LINDENMAYER : 'lindenmayer',
    Generators.QUASI3D     : 'quasi',
    Generators.QUASI4D     : 'quasi',
    Generators.QUASI5D     : 'quasi',
    Generators.QUASI6D     : 'quasi',
    Generators.GRID        : 'grid',
    Generators.TORUS1      : 'grid',
    Generators.TORUS2      : 'grid',
    Generators.MOSAIC1     : 'mosaic',
    Generators.MOSAIC2     : 'mosaic',
    Generators.BOTTLE      : 'bottle',
    Generators.TREE        : 'tree',
}

_GEN_FLAGS = {
    Generators.LINDENMAYER : [ ],
    Generators.QUASI3D     : [ '--hyperdim=3' ],
    Generators.QUASI4D     : [ '--hyperdim=4' ],
    Generators.QUASI5D     : [ '--hyperdim=5' ],
    Generators.QUASI6D     : [ '--hyperdim=6' ],
    Generators.GRID        : [ ],
    Generators.TORUS1      : [ '--torus=1' ],
    Generators.TORUS2      : [ '--torus=2' ],
    Generators.MOSAIC1     : [ ],
    Generators.MOSAIC2     : [ '--symmetric' ],
    Generators.BOTTLE      : [ ],
    Generators.TREE        : [ ],
}

assert all(g in _GEN_PROGS for g in Generators if not g.imported)
assert all(g in _GEN_FLAGS for g in Generators if not g.imported)

class _BucketList(object):

    def __init__(self):
        self.__wanted = collections.defaultdict(int)

    def __bool__(self):
        return None in self.__wanted.values() or sum(self.__wanted.values()) > 0

    def items(self):
        return filter(lambda kv : kv[1] is not None and kv[1] > 0, self.__wanted.items())

    def all_items(self):
        return self.__wanted.items()

    def pick(self):
        top = max(self.__wanted.items(), default=None, key=(lambda kv : kv[1] if kv[1] is not None else -1))
        if top is None:
            return None
        return top[0]

    def request(self, size : GraphSizes, count : int = None):
        if count is None:
            self.__wanted[size] = None
        elif count > 0:
            self.__wanted[size] += count

    def change(self, size : GraphSizes, count : int = None):
        self.__wanted[size] = count

    def offer(self, size : GraphSizes):
        remaining = self.__wanted.get(size, 0)
        return remaining is None or remaining > 0

    def decrement(self, size : GraphSizes):
        if self.__wanted[size] is not None:
            self.__wanted[size] -= 1

    def total(self):
        if None in self.__wanted.values():
            return None
        else:
            return sum(self.__wanted.values())

    def discard_unbounded_requests(self):
        keys = { k for (k, v) in self.__wanted.items() if v is None }
        for key in keys:
            del self.__wanted[key]

def do_graphs(mngr : Manager, badlog : BadLog):
    worklist = collections.defaultdict(_BucketList)
    for (gen, size, num) in mngr.config.desired_graphs:
        worklist[gen].request(size, num)
    for (gen, bl) in worklist.items():
        _update_bucket_list(mngr, gen, bl)
    for (gen, bl) in filter(lambda kv : kv[1], worklist.items()):
        _generate_graphs(mngr, gen, bl, mngr.config.import_sources)

def _update_bucket_list(mngr : Manager, gen : Generators, bl : _BucketList):
    quick = _quick_archive_import_eh()
    for (size, count) in list(bl.all_items()):
        if size.high_end is None:
            prev = sum(row[0] for row in mngr.sql_exec(
                "SELECT COUNT() FROM `Graphs` WHERE `generator` = ? AND `nodes` > ?", (gen, size.low_end)
            ))
        else:
            prev = sum(row[0] for row in mngr.sql_exec(
                "SELECT COUNT() FROM `Graphs` WHERE `generator` = ? AND `nodes` BETWEEN ? AND ?",
                (gen, size.low_end, size.high_end - 1)
            ))
        if count is None:
            if quick:
                logging.info("{:d} of * ({:.2f} %) {:s} graphs from {:s} already exist, no more needed"
                             .format(prev, 100.0, size.name, gen.name))
            else:
                logging.info("{:d} of * {:s} graphs from {:s} already exist".format(prev, size.name, gen.name))
        else:
            needed = max(0, count - prev)
            logging.info("{:d} of {:d} ({:.2f} %) {:s} graphs from {:s} already exist, {:d} more needed".format(
                prev, count, 100.0 * prev / count, size.name, gen.name, needed
            ))
            bl.change(size, needed)
    if quick:
        bl.discard_unbounded_requests()

def _generate_graphs(mngr : Manager, gen : Generators, bl : _BucketList, impsrc=None):
    wellimpsrc = get_well_known_import_sources()
    try:
        if gen is Generators.IMPORT and impsrc is not None:
            nars = len(impsrc)
            logging.info("Looking for {:s} graphs in any of {:d} configured sources".format(gen.name, nars))
            for src in impsrc:
                _gen_import(mngr, gen, src, bl)
        elif gen.imported:
            src = wellimpsrc[gen]
            _gen_import(mngr, gen, src, bl)
        else:
            _gen_generic(mngr, gen, bl)
    except RecoverableError:
        logging.error("Cannot generate / import {!s} graphs".format(gen.name))

def _gen_generic(mngr : Manager, gen : Generators, bl : _BucketList):
    assert not gen.imported
    while bl:
        size = bl.pick()
        with mngr.make_tempdir() as tempdir:
            try:
                logging.info("Generating {:s} graph with {:s} ...".format(size.name, gen.name))
                meta = _call_generic_tool(mngr, gen, size, tempdir=tempdir)
            except RecoverableError as e:
                logging.error("Cannot generate graph: {!s}".format(e))
                continue
            actualsize = GraphSizes.classify(meta['nodes'])
            if actualsize != size:
                if bl.offer(actualsize):
                    logging.notice(
                        "Asked {:s} for a {:s} graph but got a {:s} one which is still useful though".format(
                            gen.name, size.name, actualsize.name))
                else:
                    logging.warning(
                        "Asked {:s} for a {:s} graph but got a {:s} one which I'll have to discard".format(
                            gen.name, size.name, actualsize.name))
                    continue
            if _insert_graph(mngr, gen, meta, tempdir=tempdir):
                bl.decrement(actualsize)

def _call_generic_tool(mngr : Manager, gen : Generators, size : GraphSizes, tempdir : str):
    executable = os.path.join(mngr.abs_bindir, 'src', 'generators',  _GEN_PROGS[gen])
    cmd = [ executable ]
    cmd.extend(_GEN_FLAGS[gen])
    cmd.append('--output={:s}'.format(os.path.join(tempdir, enum_to_json(gen) + GRAPH_FILE_SUFFIX)))
    cmd.append('--meta={:s}'.format('STDIO'))
    cmd.append('--nodes={:d}'.format(size.target))
    return mngr.call_graphstudy_tool(cmd, meta='stdout')

def _gen_import(mngr : Manager, gen : Generators, src : ImportSource, bl : _BucketList):
    assert gen.imported
    total = bl.total()
    with src as archive, mngr.make_tempdir() as tempdir:
        cmd = [
            os.path.join(mngr.abs_bindir, 'src', 'generators',  'import'),
            '--format={:s}'.format(src.format),
            '--output={:s}'.format(os.path.join(tempdir, enum_to_json(gen) + GRAPH_FILE_SUFFIX)),
            '--meta={:s}'.format('STDIO'),
        ]
        if src.layout: cmd.append('--layout')
        if src.simplify: cmd.append('--simplify')
        cmd.append('STDIO' if src.compression is None else 'STDIO:' + src.compression)  # Let me open that file for you
        archlen = len(archive)  # This is a potentially expensive operation so do it only once
        count = 0
        logging.info("{:s} archive {!r} contains {:d} graphs in total".format(gen.name, archive.name, archlen))
        for (i, cand) in enumerate(archive):
            if not bl: break
            progress = i / archlen if total is None else count / total
            pretty = archive.prettyname(cand)
            logging.info("[{:6.2f} %] Considering {:s} graph {:d} of {:d} from {!r} of {!r} ...".format(
                100.0 * progress, gen.name, i + 1, archlen, pretty, archive.name))
            try:
                with archive.get(cand) as istr:
                    meta = mngr.call_graphstudy_tool(cmd, meta='stdout', stdin=istr)
            except Exception as e:
                if not isinstance(e, RecoverableError) and not archive.is_likely_read_error(e): raise
                logging.error("Cannot import {:s} graph {!r}: {!s}".format(gen.name, pretty, e))
                continue
            actualsize = GraphSizes.classify(meta['nodes'])
            if not bl.offer(actualsize):
                logging.notice("Discarding {:s} {:s} graph (not wanted)".format(actualsize.name, gen.name))
                continue
            if _insert_graph(mngr, gen, meta, tempdir=tempdir):
                bl.decrement(actualsize)
                count += 1
        logging.info("[{:6.2f} %] Imported {:d} {:s} graphs from {!r}".format(100.0, count, gen.name, archive.name))
        for (size, diff) in bl.items():
            logging.warning("{:s} archive exhausted but {:d} {:s} graphs are still missing".format(
                gen.name, diff, size.name))

def _insert_graph(mngr : Manager, gen : Generators, meta : dict, tempdir : str):
    graphid = Id(meta['graph'])
    native = bool(meta.get('native', False))
    filename = mngr.make_graph_filename(graphid, generator=gen)
    with mngr.sql_ctx as curs:
        if mngr.sql_select_curs(curs, 'Graphs', id=graphid):
            logging.notice("Discarding {:s} graph {!s} (already exists)".format(gen.name, graphid))
            return False
        mngr.sql_insert_curs(
            curs, 'Graphs',
            id=graphid, generator=gen, file=filename, nodes=meta['nodes'], edges=meta['edges'],
            native=native, seed=encoded(meta.get('seed')), fingerprint=prepare_fingerprint(meta.get('layout')),
        )
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    logging.debug("Renaming graph file {!r} to {!r}".format(meta['filename'], filename))
    os.rename(meta['filename'], filename)
    return True

_QUICK_ARCHIVE_IMPORT = None

def _quick_archive_import_eh(__cache=[ ]):
    global _QUICK_ARCHIVE_IMPORT
    if _QUICK_ARCHIVE_IMPORT is None:
        _QUICK_ARCHIVE_IMPORT = False
        envvar = 'MSC_QUICK_ARCHIVE_IMPORT'
        envval = os.getenv(envvar)
        if envval is not None:
            try:
                quick = int(envval)
            except ValueError:
                logging.warning("Ignoing bogous value of environment variable {!s}={!r}".format(envvar, envval))
            else:
                _QUICK_ARCHIVE_IMPORT = (quick > 0)
                if _QUICK_ARCHIVE_IMPORT:
                    logging.notice("Archives won't be scanned if requested graph count is unbounded ({!s}={!r})"
                                   .format(envvar, quick))
    return _QUICK_ARCHIVE_IMPORT
