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

__all__ = [ 'do_lay_worse' ]

import logging
import os

from .configuration import *
from .constants import *
from .errors import *
from .manager import *
from .utility import *

_WORSE_PROGS = {
    LayWorse.FLIP_NODES : 'flip-nodes',
    LayWorse.FLIP_EDGES : 'flip-edges',
    LayWorse.MOVLSQ     : 'movlsq',
    LayWorse.PERTURB    : 'perturb',
}

_WORSE_FLAGS = {
    LayWorse.FLIP_NODES : [ ],
    LayWorse.FLIP_EDGES : [ ],
    LayWorse.MOVLSQ     : [ ],
    LayWorse.PERTURB    : [ ],
}

assert all(l in _WORSE_PROGS for l in LayWorse)
assert all(l in _WORSE_FLAGS for l in LayWorse)

def do_lay_worse(mngr : Manager, badlog : BadLog):
    precision = 1000.0
    c2d = lambda x : round(precision * x)
    d2c = lambda x : x / precision
    for row in mngr.sql_select('Layouts', layout=object):
        graphid = row['graph']
        layoutid = row['id']
        parentfile = row['file']
        for worse in mngr.config.desired_lay_worse.keys():
            want = set(map(c2d, mngr.config.desired_lay_worse[worse]))
            have = { c2d(r['rate']) for r in mngr.sql_select('WorseLayouts', method=worse, parent=layoutid) }
            need = set(map(d2c, want - have))
            if not need:
                continue
            if badlog.get_bad(Actions.LAY_WORSE, layoutid, worse):
                logging.info("Skipping worsening of layout {!s} via {:s} ...".format(layoutid, worse.name))
                continue
            logging.info("Worsening layout {!s} in {:d} steps via {:s} ...".format(layoutid, len(need), worse.name))
            try:
                _worsen_generic(mngr, worse, graphid, layoutid, rates=need, parentfile=parentfile)
            except RecoverableError as e:
                badlog.set_bad(Actions.LAY_WORSE, layoutid, worse, msg=str(e))
                logging.error("Cannot worsen layout {!s} via {:s}: {!s}".format(layoutid, worse.name, e))

def _worsen_generic(mngr : Manager, worse : LayWorse, graphid : Id, parentid : Id, rates : list, parentfile : str):
    with mngr.make_tempdir() as tempdir:
        pattern = os.path.join(tempdir, '%' + LAYOUT_FILE_SUFFIX)
        cmd = [ os.path.join(mngr.abs_bindir, 'src', 'unitrans', _WORSE_PROGS[worse]) ]
        cmd.append('--output={:s}'.format(pattern))
        cmd.append('--meta={:s}'.format('STDIO'))
        cmd.extend(_WORSE_FLAGS[worse])
        cmd.extend('--rate={:.10f}'.format(r) for r in rates)
        cmd.append(parentfile)
        meta = mngr.call_graphstudy_tool(cmd, meta='stdout')
        for data in meta['data']:
            seed = encoded(meta.get('seed'))
            fingerprint = prepare_fingerprint(data.get('layout'))
            _add_worse_layout(mngr, worse, data, graphid=graphid, layoutid=parentid, seed=seed, fingerprint=fingerprint)

def _add_worse_layout(
        mngr : Manager, worse : LayWorse, data : dict, graphid : Id, layoutid : Id,
        seed : bytes = None, fingerprint : bytes = None,
):
    with mngr.sql_ctx as curs:
        thisid = mngr.make_unique_layout_id(curs)
        filename = mngr.make_layout_filename(graphid, thisid)
        mngr.sql_insert_curs(
            curs, 'Layouts', id=thisid, graph=graphid, layout=None, file=filename,
            width=data.get('width'), height=data.get('height'), seed=seed, fingerprint=fingerprint,
        )
        mngr.sql_insert_curs(curs, 'WorseLayouts', id=thisid, parent=layoutid, method=worse, rate=data['rate'])
    scratch = data['filename']
    logging.debug("Renaming worse layout file {!r} to {!r}".format(scratch, filename))
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    os.rename(scratch, filename)
