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

__all__ = [ 'do_layouts' ]

import itertools
import logging
import os

from .configuration import *
from .constants import *
from .errors import *
from .manager import *
from .utility import *

_LAYOUT_PROGS = {
    Layouts.NATIVE             : None,
    Layouts.FMMM               : 'force',
    Layouts.STRESS             : 'force',
    Layouts.DAVIDSON_HAREL     : 'force',
    Layouts.SPRING_EMBEDDER_KK : 'force',
    Layouts.PIVOT_MDS          : 'force',
    Layouts.SUGIYAMA           : 'sugiyama',
    Layouts.PHANTOM            : 'phantom',
    Layouts.RANDOM_UNIFORM     : 'random',
    Layouts.RANDOM_NORMAL      : 'random',
}

_LAYOUT_FLAGS = {
    Layouts.NATIVE             : None,
    Layouts.FMMM               : [ '--algorithm=FMMM' ],
    Layouts.STRESS             : [ '--algorithm=STRESS' ],
    Layouts.DAVIDSON_HAREL     : [ '--algorithm=DAVIDSON_HAREL' ],
    Layouts.SPRING_EMBEDDER_KK : [ '--algorithm=SPRING_EMBEDDER_KK' ],
    Layouts.PIVOT_MDS          : [ '--algorithm=PIVOT_MDS' ],
    Layouts.SUGIYAMA           : [ ],
    Layouts.PHANTOM            : [ ],
    Layouts.RANDOM_UNIFORM     : [ '--distribution=UNIFORM' ],
    Layouts.RANDOM_NORMAL      : [ '--distribution=NORMAL' ],
}

assert all(l in _LAYOUT_PROGS for l in Layouts)
assert all(l in _LAYOUT_FLAGS for l in Layouts)

def do_layouts(mngr : Manager, badlog : BadLog):
    graph_rows = mngr.sql_select('Graphs')
    layout_keys = sorted(mngr.config.desired_layouts.keys())
    total = len(graph_rows) * len(layout_keys)
    for (i, row) in enumerate(graph_rows):
        graphid = row['id']
        graphfilename = row['file']
        size = GraphSizes.classify(row['nodes'])
        for (j, layout) in enumerate(layout_keys):
            progress = 100.0 * (i * len(layout_keys) + j + 1) / total
            sizes_desired = mngr.config.desired_layouts.get(layout) or set()
            if size in sizes_desired and not mngr.sql_select('Layouts', graph=graphid, layout=layout):
                if badlog.get_bad(Actions.LAYOUTS, graphid, layout):
                    logging.notice("Skipping {:s} layout for graph {!s}".format(layout.name, graphid))
                    continue
                try:
                    if layout is Layouts.NATIVE:
                        if row['native']:
                            _lay_native(mngr, graphid, graphfilename, progress=progress)
                    else:
                        _lay_generic(mngr, layout, graphid, graphfilename=graphfilename, progress=progress)
                except RecoverableError as e:
                    badlog.set_bad(Actions.LAYOUTS, graphid, layout, msg=str(e))
                    logging.error("Cannot make {:s} layout for graph {!s}: {!s}".format(layout.name, graphid, e))
    _ensure_layout_fingerprints(mngr)

def _lay_native(mngr: Manager, graphid : Id, graphfilename : str, progress : float = None):
    with mngr.sql_ctx as curs:
        layoutid = mngr.make_unique_layout_id(curs)
        filename = mngr.make_layout_filename(graphid, layoutid, layout=Layouts.NATIVE)
        fingerprint = get_one_or(r['fingerprint'] for r in mngr.sql_select_curs(curs, 'Graphs', id=graphid))
        mngr.sql_insert_curs(
            curs, 'Layouts', id=layoutid, graph=graphid, layout=Layouts.NATIVE, file=filename, fingerprint=fingerprint
        )
    directory = os.path.dirname(filename)
    os.makedirs(directory, exist_ok=True)
    symlinktarget = os.path.relpath(graphfilename, start=directory)
    logging.info("Creating symbolic link: {!r} -> {!r}".format(filename, symlinktarget))
    os.symlink(symlinktarget, filename)

def _lay_generic(mngr : Manager, layout : Layouts, graphid : Id, graphfilename : str, progress : float = None):
    assert layout is not Layouts.NATIVE
    prefix = "" if progress is None else "[{:6.2f} %] ".format(progress)
    logging.info(prefix + "Generating {:s} layout for graph {!s} ...".format(layout.name, graphid))
    with mngr.make_tempdir() as tempdir:
        cmd = [ os.path.join(mngr.abs_bindir, 'src', 'layouts', _LAYOUT_PROGS[layout]) ]
        cmd.append('--output={:s}'.format(os.path.join(tempdir, enum_to_json(layout) + LAYOUT_FILE_SUFFIX)))
        cmd.append('--meta={:s}'.format('STDIO'))
        cmd.extend(_LAYOUT_FLAGS[layout])
        cmd.append(graphfilename)
        meta = mngr.call_graphstudy_tool(cmd, meta='stdout')
        with mngr.sql_ctx as curs:
            layoutid = mngr.make_unique_layout_id(curs)
            layoutfilename = mngr.make_layout_filename(graphid, layoutid, layout)
            mngr.sql_insert_curs(
                curs, 'Layouts', id=layoutid, graph=graphid, layout=layout, file=layoutfilename,
                width=meta.get('width'), height=meta.get('height'),
                seed=encoded(meta.get('seed')), fingerprint=prepare_fingerprint(meta.get('layout')),
            )
        logging.debug("Renaming layout file {!r} to {!r}".format(meta['filename'], layoutfilename))
        os.makedirs(os.path.dirname(layoutfilename), exist_ok=True)
        os.rename(meta['filename'], layoutfilename)

def _ensure_layout_fingerprints(mngr : Manager):
    with mngr.sql_ctx as curs:
        total = get_one(mngr.sql_exec_curs(curs, "SELECT COUNT() FROM `Layouts`", tuple()))[0]
        without = { r['id'] : r['file'] for r in mngr.sql_select_curs(curs, 'Layouts', fingerprint=None) }
    logging.notice("{:,d} of {:,d} ({:.2f} %) layouts in the database have no associated fingerprint".format(
        len(without), total, (100.0 * len(without) / total if total > 0 else 0.0)
    ))
    for (i, (layout, filename)) in enumerate(without.items()):
        progress = 100.0 * (i + 1) / len(without)
        logging.info("[{:6.2f} %] Coputing fingerprint for layout {!s} ...".format(progress, layout))
        command = [
            os.path.join(mngr.abs_bindir, 'src', 'utility', 'fingerprint'), '--layout', '--meta=STDIO', filename
        ]
        try:
            meta = mngr.call_graphstudy_tool(command, meta='stdout')
            fingerprint = prepare_fingerprint(meta.get('layout'))
            if not fingerprint: raise RecoverableError("No fingerprint")
        except RecoverableError as e:
            logging.error("Cannot compute fingerprint for layout {!s}: {!s}".format(layout, e))
            continue
        mngr.sql_exec(
            "UPDATE `Layouts` SET `fingerprint` = ?, `width` = ?, `height` = ?  WHERE `id` = ?",
            (fingerprint, meta.get('width'), meta.get('height'), layout)
        )
