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

__all__ = [ 'do_lay_inter' ]

import itertools
import logging
import os

from .configuration import *
from .constants import *
from .errors import *
from .manager import *
from .utility import *

_INTER_PROGS = {
    LayInter.LINEAR  : 'interpol',
    LayInter.XLINEAR : 'interpol',
}

_INTER_FLAGS = {
    LayInter.LINEAR  : [ ],
    LayInter.XLINEAR : [ '--clever' ],
}

def do_lay_inter(mngr : Manager, badlog : BadLog):
    precision = 1000.0
    c2d = lambda x : round(precision * x)
    d2c = lambda x : x / precision
    for graphid in map(lambda r : r['id'], mngr.sql_select('Graphs', poisoned=False)):
        layouts = { r['id'] : r['file'] for r in mngr.sql_select('Layouts', graph=graphid, layout=object) }
        for (id1, id2) in itertools.combinations(sorted(layouts.keys(), key=Id.getkey), 2):
            for inter in mngr.config.desired_lay_inter.keys():
                want = set(map(c2d, mngr.config.desired_lay_inter[inter]))
                have = { c2d(r['rate']) for r in mngr.sql_select('InterLayouts', method=inter, parent1st=id1) }
                need = set(map(d2c, want - have))
                if not need:
                    continue
                if badlog.get_bad(Actions.LAY_INTER, id1, id2, inter):
                    logging.info("Skipping interpolation between layouts {!s} and {!s} via {:s} ...".format(
                        id1, id2, inter.name))
                    continue
                logging.info("Interpolating between layouts {!s} and {!s} in {:d} steps via {:s} ...".format(
                    id1, id2, len(need), inter.name))
                try:
                    files = (layouts[id1], layouts[id2])
                    _interpolate_generic(mngr, inter, graphid, (id1, id2), rates=need, files=files)
                except RecoverableError as e:
                    badlog.set_bad(Actions.LAY_INTER, id1, id2, inter, msg=str(e))
                    logging.error("Cannot interpolate between layouts {!s} and {!s} via {:s}: {!s}".format(
                        id1, id2, inter.name, e))

def _interpolate_generic(mngr : Manager, inter : LayInter, graphid : Id, parents : tuple, rates : list, files : list):
    with mngr.make_tempdir() as tempdir:
        pattern = os.path.join(tempdir, '%' + LAYOUT_FILE_SUFFIX)
        cmd = [ os.path.join(mngr.abs_bindir, 'src', 'bitrans', _INTER_PROGS[inter]) ]
        cmd.append('--output={:s}'.format(pattern))
        cmd.append('--meta={:s}'.format('STDIO'))
        cmd.extend(_INTER_FLAGS[inter])
        cmd.extend('--rate={:.10f}'.format(r) for r in rates)
        cmd.append(files[0])
        cmd.append(files[1])
        meta = mngr.call_graphstudy_tool(cmd, meta='stdout')
        for data in meta['data']:
            seed = encoded(meta.get('seed'))
            fingerprint = prepare_fingerprint(data.get('layout'))
            _add_inter_layout(mngr, inter, data, graphid=graphid, parents=parents, seed=seed, fingerprint=fingerprint)

def _add_inter_layout(
        mngr : Manager, inter : LayInter, data : dict, graphid : Id, parents : tuple,
        seed : bytes = None, fingerprint : bytes = None,
):
    with mngr.sql_ctx as curs:
        thisid = mngr.make_unique_layout_id(curs)
        filename = mngr.make_layout_filename(graphid, thisid)
        mngr.sql_insert_curs(
            curs, 'Layouts', id=thisid, graph=graphid, layout=None, file=filename,
            width=data.get('width'), height=data.get('height'), seed=seed, fingerprint=fingerprint,
        )
        mngr.sql_insert_curs(
            curs, 'InterLayouts', id=thisid, parent1st=parents[0], parent2nd=parents[1], method=inter, rate=data['rate']
        )
    scratch = data['filename']
    logging.debug("Renaming interpolated layout file {!r} to {!r}".format(scratch, filename))
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    os.rename(scratch, filename)
