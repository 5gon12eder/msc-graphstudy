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

__all__ = [ 'do_properties' ]

import itertools
import logging
import math
import os
import sqlite3

from .configuration import *
from .constants import *
from .errors import *
from .manager import *
from .utility import *

_DATA_FILE_SUFFIX = '.txt.gz'

_PROPERTY_PROGS = {
    Properties.ANGULAR     : 'angular',
    Properties.RDF_GLOBAL  : 'rdf-global',
    Properties.RDF_LOCAL   : 'rdf-local',
    Properties.EDGE_LENGTH : 'edge-length',
    Properties.PRINCOMP1ST : 'princomp',
    Properties.PRINCOMP2ND : 'princomp',
    Properties.TENSION     : 'tension',
}

_PROPERTY_FLAGS = {
    Properties.ANGULAR     : [ ],
    Properties.RDF_GLOBAL  : [ ],
    Properties.RDF_LOCAL   : [ ],
    Properties.EDGE_LENGTH : [ ],
    Properties.PRINCOMP1ST : [ '-1' ],
    Properties.PRINCOMP2ND : [ '-2' ],
    Properties.TENSION     : [ ],
}

assert all(p in _PROPERTY_PROGS for p in Properties)
assert all(p in _PROPERTY_FLAGS for p in Properties)

_PROPERTY_TABLES_OUTER = {
    Kernels.RAW      : None,
    Kernels.BOXED    : 'PropertiesDisc',
    Kernels.GAUSSIAN : 'PropertiesCont',
}

_PROPERTY_TABLES_INNER = {
    Kernels.RAW      : None,
    Kernels.BOXED    : 'Histograms',
    Kernels.GAUSSIAN : 'SlidingAverages',
}

assert all(k in _PROPERTY_TABLES_OUTER for k in Kernels)
assert all(k in _PROPERTY_TABLES_INNER for k in Kernels)

def do_properties(mngr : Manager, badlog : BadLog):
    moderately_useful_helper = lambda kern : (kern, _PROPERTY_TABLES_OUTER[kern])
    (kerndisc, tabdisc) = moderately_useful_helper(Kernels.BOXED)
    (kerncont, tabcont) = moderately_useful_helper(Kernels.GAUSSIAN)
    n = get_one(r[0] for r in mngr.sql_exec("SELECT COUNT(*) FROM `Layouts`", tuple()))
    tally = 0
    emptyset = set()
    for (graphid, graphsize) in map(lambda r : (r['id'], GraphSizes.classify(r['nodes'])), mngr.sql_select('Graphs')):
        for (layoutid, layoutfile) in map(lambda r : (r['id'], r['file']), mngr.sql_select('Layouts', graph=graphid)):
            tally += 1
            kwargs = { 'graphid' : graphid, 'layoutid' : layoutid, 'layoutfile' : layoutfile, 'progress' : tally / n }
            for prop in Properties:
                if graphsize in mngr.config.desired_properties_disc.get(prop, emptyset):
                    if not mngr.sql_select(tabdisc, layout=layoutid, property=prop):
                        _compute_prop_outer(mngr, prop, kerndisc, badlog, **kwargs)
                if graphsize in mngr.config.desired_properties_cont.get(prop, emptyset):
                    if not mngr.sql_select(tabcont, layout=layoutid, property=prop):
                        _compute_prop_outer(mngr, prop, kerncont, badlog, **kwargs)

def _compute_prop_outer(
        mngr : Manager, prop : Properties, kern : Kernels, badlog : BadLog,
        graphid : Id = None, layoutid : Id = None, layoutfile : str = None, progress : float = None
):
    what = { Kernels.BOXED : "discrete" , Kernels.GAUSSIAN : "continuous" }.get(kern, "other")
    if badlog.get_bad(Actions.PROPERTIES, layoutid, prop):
        logging.notice("Skipping computation of {:s} property {:s} for layout {!s}".format(what, prop.name, layoutid))
        return
    prefix = "[{:6.2f} %] ".format(100.0 * min(1.0, progress)) if progress is not None else ""
    logging.info(prefix + "Computing {:s} property {:s} for layout {!s} ...".format(what, prop.name, layoutid))
    directory = _get_directory(mngr, layoutid, prop, kern, make=True)
    try:
        _compute_prop_inner(mngr, prop, kern, layoutid, layoutfile, directory=directory)
    except RecoverableError as e:
        badlog.set_bad(Actions.PROPERTIES, layoutid, prop, msg=str(e))
        logging.error("Cannot compute {:s} property {:s} for layout {!s} of graph {!s}: {!s}"
                      .format(what, prop.name, layoutid, graphid, e))

def _compute_prop_inner(mngr : Manager, prop : Properties, kern : Kernels,
                        layoutid : Id, layoutfile : str, directory : str):
    with mngr.make_tempdir() as tempdir:
        cmd = [ _get_executable(mngr, prop) ]
        cmd.extend(_PROPERTY_FLAGS[prop])
        cmd.append('--kernel={:s}'.format(kern.name))
        if kern is Kernels.BOXED:
            cmd.extend('--bins={:d}'.format(b) for b in FIXED_COUNT_BINS)
        if prop.localized:
            cmd.extend('--vicinity={:.1f}'.format(v) for v in VICINITIES)
        filenamebase = 'histogram' if kern is Kernels.BOXED else kern.name.lower()
        filenamepattern = filenamebase + '-' + ('%-%' if prop.localized else '%') + _DATA_FILE_SUFFIX
        deterministic = prop in [ Properties.PRINCOMP1ST, Properties.PRINCOMP2ND ]
        cmd.append('--output={:s}'.format(os.path.join(tempdir, filenamepattern)))
        cmd.append('--meta=STDIO')
        cmd.append(layoutfile)
        meta = mngr.call_graphstudy_tool(cmd, meta='stdout', deterministic=deterministic)
        with mngr.sql_ctx as curs:
            if prop.localized:
                maxvic = None
                for item in meta['data']:
                    nullfiles = [ sub['filename'] is None for sub in item['data'] ]
                    if any(nullfiles) != all(nullfiles):
                        raise SanityError("Either all or no files shall be NULL")
                    if item['vicinity'] <= meta['diameter'] and any(nullfiles):
                        raise SanityError("Files shall not be NULL for vicinities up to the graph's diameter")
                    if item['vicinity'] > meta['diameter'] and maxvic is not None and not all(nullfiles):
                        raise SanityError("Files shall be NULL for all but one vicinity above the graph's diameter")
                    if maxvic is None and all(nullfiles): maxvic = item['vicinity']
                    _insert_data(mngr, curs, prop, kern, layoutid, item, directory=directory, tempdir=tempdir)
            else:
                _insert_data(mngr, curs, prop, kern, layoutid, meta, directory=directory, tempdir=tempdir)

def _insert_data(mngr, curs, prop, kern, layoutid=None, meta=None, directory=None, tempdir=None):
    tableouter = _PROPERTY_TABLES_OUTER[kern]
    tableinner = _PROPERTY_TABLES_INNER[kern]
    vicinity = _roundabout(meta.get('vicinity'), what="vicinity")
    valuesouter = {
        'layout'   : layoutid,
        'property' : prop,
        'vicinity' : vicinity,
        'size'     : meta['size'],
        'minimum'  : meta['minimum'],
        'maximum'  : meta['maximum'],
        'mean'     : meta['mean'],
        'rms'      : meta['rms'],
    }
    try:
        valuesouter['entropyIntercept'] = meta['entropy-intercept']
        valuesouter['entropySlope'] = meta['entropy-slope']
    except KeyError:
        pass
    renamings = list()
    for data in filter(lambda d : d['filename'] is not None, meta['data']):
        tempname = data['filename']
        filename = _temporary_to_persistent_name(tempname, tempdir=tempdir, directory=directory)
        renamings.append((tempname, filename))
        valuesinner = {
            'property' : prop,
            'vicinity' : vicinity,
            'layout'   : layoutid,
            'file'     : filename,
            'entropy' : data['entropy'],
        }
        if kern is Kernels.BOXED:
            valuesinner['binning'] = enum_from_json(Binnings, data.get('binning'))
            valuesinner['bincount'] = data['bincount']
            valuesinner['binwidth'] = data['binwidth']
        if kern is Kernels.GAUSSIAN:
            valuesinner['sigma'] = data['sigma']
            valuesinner['points'] = data['points']
        _sql_insert_curs(mngr, curs, tableinner, **valuesinner)
    _sql_insert_curs(mngr, curs, tableouter, **valuesouter)
    _maybe_insert_pca(mngr, curs, prop=prop, layoutid=layoutid, meta=meta)
    _rename_files_batch(renamings)

def _maybe_insert_pca(mngr, curs, prop=Ellipsis, layoutid=Ellipsis, meta=Ellipsis):
    table = { Properties.PRINCOMP1ST : 'MajorAxes', Properties.PRINCOMP2ND : 'MinorAxes' }.get(prop)
    if table is None:
        return
    (x, y) = meta['component']
    try:
        query = "INSERT OR REPLACE INTO `{table:s}` (`layout`, `x`, `y`) VALUES (?, ?, ?)"
        mngr.sql_exec_curs(curs, query.format(table=table), (layoutid, x, y))
    except sqlite3.IntegrityError as e:
        raise RecoverableError(e)

def _get_directory(mngr : Manager, layoutid : Id, prop : Properties, kern : Kernels, make : bool = False) -> str:
    fullid = str(layoutid)
    (head, tail) = (fullid[:2], fullid[2:])
    directory = os.path.join(mngr.propsdir, head, tail, enum_to_json(prop))
    if make:
        os.makedirs(directory, exist_ok=True)
    return directory

def _get_executable(mngr : Manager, prop : Properties):
    return os.path.join(mngr.abs_bindir, 'src', 'properties', _PROPERTY_PROGS[prop])

def _temporary_to_persistent_name(tempname : str, tempdir : str, directory : str) -> str:
    relative = os.path.relpath(tempname, start=tempdir)
    return os.path.join(directory, relative)

def _rename_files_batch(renamings):
    for (old, new) in renamings:
        logging.debug("Renaming file {!r} to {!r}".format(old, new))
        os.rename(old, new)

def _sql_insert_curs(mngr, curs, table, **kwargs):
    try:
        mngr.sql_insert_curs(curs, table, **kwargs)
    except sqlite3.IntegrityError as e:
        raise RecoverableError(e)

def _roundabout(x : float, what="value"):
    if x is None:
        return None
    asint = round(x)
    if x == asint:
        return asint
    raise SanityError("Non-integral {:s} {!r}".format(what, x))
