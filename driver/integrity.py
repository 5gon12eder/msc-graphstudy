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

import collections
import datetime
import fnmatch
import glob
import logging
import math
import os
import shlex
import shutil
import stat
import sys
import time

from .cli import *
from .configuration import *
from .constants import *
from .errors import *
from .manager import *
from .utility import *

PROGRAM_NAME = 'integrity'

_REQUIRED_FOR_GRAPH = [
    'id',
    'generator',
    'file',
    'nodes',
    'edges',
]

_REQUIRED_FOR_LAYOUT = [
    'id',
    'graph',
    'file',
    'fingerprint',
]

_REQUIRED_FOR_INTER_LAYOUTS = [
    'id',
    'parent1st',
    'parent2nd',
    'method',
    'rate',
]

_REQUIRED_FOR_WORSE_LAYOUTS = [
    'id',
    'parent',
    'method',
    'rate',
]

_REQUIRED_FOR_PROPERTY_DISC = [
    'layout',
    'size',
    'minimum',
    'maximum',
    'mean',
    'rms',
    'entropyIntercept',
    'entropySlope',
]

_REQUIRED_FOR_PROPERTY_CONT = [
    'layout',
    'size',
    'minimum',
    'maximum',
    'mean',
    'rms',
]

_REQUIRED_FOR_HISTOGRAM = [
    'property',
    'layout',
    'file',
    'binning',
    'bincount',
    'binwidth',
    'entropy',
]

_REQUIRED_FOR_SLIDING_AVERAGE = [
    'property',
    'layout',
    'file',
    'sigma',
    'points',
    'entropy',
]

# If the limit cannot be expressed exactly as binary / decimal number, use this tolerance.
_LIMIT_TOLERANCE = 1.0 / (1 << 30)

_PROPERTY_LIMITS = {
    Properties.RDF_GLOBAL  : (0.0, None),
    Properties.RDF_LOCAL   : (0.0, None),
    Properties.ANGULAR     : (0.0, 2.0 * math.pi + _LIMIT_TOLERANCE),
    Properties.EDGE_LENGTH : (0.0, None),
    Properties.PRINCOMP1ST : (None, None),
    Properties.PRINCOMP2ND : (None, None),
    Properties.TENSION     : (0.0, None),
}

assert all(p in _PROPERTY_LIMITS for p in Properties)

class Checker(object):

    def __init__(self, mngr, journal=None, repair=False, fast=False):
        self.__mngr = mngr
        self.__journal = journal
        self.__repair = bool(repair)
        self.__issues = None
        self.__iterprefix = None
        self.__files = None if fast else self.__index_files()

    def __call__(self, iteration=None):
        self.__mngr.sql_exec("PRAGMA foreign_keys = OFF", tuple())
        self.__issues = 0
        self.__iterprefix = '' if iteration is None else '[{:d}] '.format(iteration)
        self.__check_tempdirs()
        self.__check_graphs()
        self.__check_layouts()
        self.__check_properties()
        self.__check_princomp()
        self.__check_test_scores()
        return self.__issues

    @property
    def issues(self):
        return self.__issues

    def __bemoan(self, message):
        self.__issues += 1
        if self.__journal is None:
            logging.warning(self.__iterprefix + message)
        else:
            print(self.__iterprefix + message, file=self.__journal)

    def __remove_from_db(self, curs, query, *params):
        logging.debug(query.replace('?', '%s') % tuple(sqlrepr(p) for p in params))
        if self.__repair:
            self.__mngr.sql_exec_curs(curs, query, params)

    def __remove_from_fs(self, filename, recursive=False):
        logging.debug("Removing file {!r}".format(filename))
        if self.__repair:
            if recursive:
                shutil.rmtree(filename)
                pattern = os.path.join(glob.escape(filename), '**')
                for thing in list(filter(lambda s : fnmatch.fnmatch(s, pattern), self.__files.keys())):
                    assert not os.path.exists(thing)
                    del self.__files[thing]
            else:
                try: os.remove(filename)
                except FileNotFoundError: pass
            del self.__files[filename]

    def __index_files(self):
        topdir = os.curdir
        tagfile = os.path.join(topdir, DATA_ROOT_TAG_FILE)
        entries = dict()
        verbose = os.name == 'posix' and sys.stderr.isatty()
        def sysout(msg):
            sys.stderr.buffer.write(msg)
            sys.stderr.buffer.flush()
        def report_progress(count):
            clock = datetime.timedelta(seconds=round(time.time() - t0))
            sysout(b'\033[G\033[K' + "counted {:,d} items in {!s}".format(count, clock).encode('ascii'))
        def traverse(root):
            for item in os.scandir(root):
                entries[os.path.relpath(os.path.normpath(item.path), start=topdir)] = item
                if verbose and len(entries) & 0x3ff == 0: report_progress(len(entries))
                if item.is_dir(): traverse(item.path)
        logging.info("Scanning directory tree {!r} (this might take a while) ...".format(os.path.abspath(topdir)))
        if not os.path.exists(tagfile):
            raise ValueError("File {!r} does not exist".format(os.path.abspath(tagfile)))
        try:
            if verbose: sysout(b'\033[?25l')
            t0 = time.time()
            traverse(topdir)
            t1 = time.time()
        finally:
            if verbose: sysout(b'\033[G\033[K\033[?25h')
        elapsed = datetime.timedelta(seconds=round(t1 - t0))
        logging.info("Scan complete, indexed {:,d} entries in {!s}".format(len(entries), elapsed))
        return entries

    def __check_tempdirs(self):
        if self.__files is None:
            return
        for (key, item) in list(filter(lambda kv: fnmatch.fnmatch(kv[0], 'temp-*'), self.__files.items())):
            if item.is_dir():
                self.__bemoan("Orphaned temporary directory {!r}".format(key))
                self.__remove_from_fs(key, recursive=True)
            else:
                self.__bemoan("Orphaned temporary file {!r}".format(key))
                self.__remove_from_fs(key)

    def __check_graphs(self):
        self.__check_table('Graphs', _REQUIRED_FOR_GRAPH)
        self.__check_files('Graphs', os.path.join(glob.escape(self.__mngr.graphsdir), '*'))

    def __check_layouts(self):
        self.__check_table('Layouts', _REQUIRED_FOR_LAYOUT, crossrefs={ 'graph' : ('Graphs', 'id') })
        self.__check_files('Layouts', os.path.join(glob.escape(self.__mngr.layoutdir), '*', '*'))
        self.__check_inter_layouts()
        self.__check_worse_layouts()
        with self.__mngr.sql_ctx as curs:
            for row in self.__mngr.sql_select_curs(curs, 'Layouts', layout=None):
                layoutid = row['id']
                filename = row['file']
                if self.__mngr.sql_select_curs(curs, 'Graphs', id=row['graph'], poisoned=True):
                    continue
                if self.__mngr.sql_select_curs(curs, 'InterLayouts', id=layoutid):
                    continue
                if self.__mngr.sql_select_curs(curs, 'WorseLayouts', id=layoutid):
                    continue
                self.__bemoan("Irregular layout {!s} neither transformed nor from poisoned graph".format(layoutid))
                self.__remove_from_db(curs, "DELETE FROM `Layouts` WHERE `id` = ?", layoutid)
                self.__remove_from_fs(filename)
            for row in self.__mngr.sql_select_curs(curs, 'Graphs', poisoned=True):
                graphid = row['id']
                if self.__mngr.sql_select_curs(curs, 'Layouts', graph=graphid, layout=object):
                    self.__bemoan("Poisoned graph {!s} shall not have regular layouts".format(graphid))
                    self.__remove_from_db(curs, "DELETE FROM `Layouts` WHERE `graph` = ? AND `layout` NOTNULL", graphid)

    def __check_inter_layouts(self):
        crossrefs = { 'id' : ('Layouts', 'id'), 'parent1st' : ('Layouts', 'id'), 'parent2nd' : ('Layouts', 'id') }
        self.__check_table('InterLayouts', _REQUIRED_FOR_INTER_LAYOUTS, crossrefs=crossrefs)
        self.__check_transformed_parents('InterLayouts', [ 'parent1st', 'parent2nd' ], what="interpolated")

    def __check_worse_layouts(self):
        crossrefs = { 'id' : ('Layouts', 'id'), 'parent' : ('Layouts', 'id') }
        self.__check_table('WorseLayouts', _REQUIRED_FOR_WORSE_LAYOUTS, crossrefs=crossrefs)
        self.__check_transformed_parents('WorseLayouts', [ 'parent' ], what="worsened")

    def __check_transformed_parents(self, table, parentcolumns, what="transformed"):
        with self.__mngr.sql_ctx as curs:
            for row in self.__mngr.sql_select_curs(curs, table):
                layoutid = row['id']
                for parentid in (row[col] for col in parentcolumns):
                    for bad in self.__mngr.sql_select_curs(curs, 'Layouts', id=parentid, layout=None):
                        self.__bemoan("Parents of {what:s} layout {!s} shall be regular".format(layoutid, what=what))
                        self.__remove_from_db(curs, "DELETE FROM `Layouts` WHERE `id` = ?", layoutid)

    def __check_properties(self):
        self.__check_property_tables()
        self.__check_property_files()
        self.__check_properties_outer()
        self.__check_properties_inner()

    def __check_property_tables(self):
        crossrefs = { 'layout' : ('Layouts', 'id') }
        self.__check_table('PropertiesDisc', _REQUIRED_FOR_PROPERTY_DISC, crossrefs)
        self.__check_table('PropertiesCont', _REQUIRED_FOR_PROPERTY_CONT, crossrefs)
        self. __check_table('Histograms', _REQUIRED_FOR_HISTOGRAM, crossrefs)
        self. __check_table('SlidingAverages', _REQUIRED_FOR_SLIDING_AVERAGE, crossrefs)
        with self.__mngr.sql_ctx as curs:
            for tab in [ 'PropertiesDisc', 'PropertiesCont' ]:
                delete = collections.defaultdict(set)
                for row in self.__mngr.sql_select_curs(curs, tab):
                    prop = row['property']
                    layoutid = row['layout']
                    (minimum, maximum) = _PROPERTY_LIMITS[prop]
                    for col in [ 'minimum', 'maximum', 'mean', 'rms' ]:
                        value = row[col]
                        if not _isbetween(value, minimum, maximum):
                            delete[layoutid].add(prop)
                            self.__bemoan("Layout {!s} property {:s} table {!r} column {!r} is off-limits: {:.20E}"
                                          .format(layoutid, prop.name, tab, col, value))
            for (layoutid, properties) in delete.items():
                if not properties: continue
                propset = ', '.join(str(prop.value) for prop in properties)
                template = "DELETE FROM `{table:s}` WHERE `layout` = ? AND `property` IN (" + propset + ")"
                self.__remove_from_db(curs, template.format(table=tab), layoutid)
                if table == 'PropertiesDisc':
                    self.__remove_from_db(curs, template.format(table='Histograms'), layoutid)

    def __check_property_files(self):
        for prop in Properties:
            directorypattern = os.path.join(glob.escape(self.__mngr.propsdir), '**', glob.escape(enum_to_json(prop)))
            pattern_disc = os.path.join(directorypattern, 'histogram*')
            pattern_cont = os.path.join(directorypattern, 'gaussian*')
            pattern_both = os.path.join(directorypattern, '*')
            self.__check_files('Histograms', pattern_disc, where={ 'property' : prop })
            self.__check_files('SlidingAverages', pattern_cont, where={ 'property' : prop })
            if self.__files is not None:
                for filename in list(filter(lambda s : fnmatch.fnmatch(s, pattern_both), self.__files.keys())):
                    disc = fnmatch.fnmatch(filename, pattern_disc)
                    cont = fnmatch.fnmatch(filename, pattern_cont)
                    if disc and cont:
                        logging.alert("Filename {!r} matches both patterns {!r} and {!r}".format(
                            filename, pattern_disc, pattern_cont
                        ))
                    if not disc and not cont:
                        (what, size) = _get_file_info(self.__files[filename])
                        self.__bemoan("Stray file {!r} ({:s}, {:,d} bytes)".format(filename, what, size))
                        self.__remove_from_fs(filename)

    def __check_properties_outer(self):
        for (tabouter, tabinner) in [ ('PropertiesDisc', 'Histograms'), ('PropertiesCont', 'SlidingAverages') ]:
            with self.__mngr.sql_ctx as curs:
                delete = set()
                for row in self.__mngr.sql_select_curs(curs, tabouter):
                    layoutid = row['layout']
                    property = row['property']
                    vicinity = row['vicinity']
                    # NB: Any vicinity is fine for the following query as we only insert "real" histograms.
                    if not self.__mngr.sql_select_curs(curs, tabinner, layout=layoutid, property=property):
                        vicinitystring = 'N/A' if vicinity is None else str(vicinity)
                        self.__bemoan("Layout {!s} property {:s} vicinity {:s} mentioned in table {!r} but not {!r}"
                                      .format(layoutid, property.name, vicinitystring, tabouter, tabinner ))
                        delete.add((layoutid, property))
                template = "DELETE FROM `{table:s}` WHERE `layout` = ? AND `property` = ?"
                for (layoutid, property) in delete:
                    for table in [ tabouter, tabinner ]:
                        self.__remove_from_db(curs, template.format(table=table), layoutid, property)

    def __check_properties_inner(self):
        for (tabouter, tabinner) in [ ('PropertiesDisc', 'Histograms'), ('PropertiesCont', 'SlidingAverages') ]:
            with self.__mngr.sql_ctx as curs:
                for row in self.__mngr.sql_select_curs(curs, tabinner):
                    layoutid = row['layout']
                    property = row['property']
                    vicinity = row['vicinity']
                    if not self.__mngr.sql_select_curs(
                            curs, tabouter, layout=layoutid, property=property, vicinity=vicinity
                    ):
                        vicinitystring = 'N/A' if vicinity is None else str(vicinity)
                        self.__bemoan("Layout {!s} property {:s} vicinity {:s} mentioned in table {!r} but not {!r}"
                                      .format(layoutid, property.name, vicinitystring, tabinner, tabouter))
                        template = "DELETE FROM `{table:s}` WHERE `layout` = ? AND `property` = ?"
                        for table in [ tabouter, tabinner ]:
                            self.__remove_from_db(curs, template.format(table=table), layoutid, property)

    def __check_princomp(self):
        vecabs = lambda v : math.sqrt(sum(x**2 for x in v))
        vecdot = lambda v1, v2 : sum(x1 * x2 for (x1, x2) in zip(v1, v2))
        getter = lambda r : (r['x'], r['y'])
        with self.__mngr.sql_ctx as curs:
            delete = set()
            for layoutid in index_projection('id', self.__mngr.sql_select_curs(curs, 'Layouts')):
                major = get_one_or(map(getter, self.__mngr.sql_select_curs(curs, 'MajorAxes', layout=layoutid)))
                minor = get_one_or(map(getter, self.__mngr.sql_select_curs(curs, 'MinorAxes', layout=layoutid)))
                if major is not None:
                    if vecabs(major) != 0.0 and abs(1.0 - vecabs(major)) > 1.0E-3:
                        self.__bemoan("Major component of layout {!s} is not normalized".format(layoutid))
                        delete.add(layoutid)
                if minor is not None:
                    if vecabs(minor) != 0.0 and abs(1.0 - vecabs(minor)) > 1.0E-3:
                        self.__bemoan("Minor component of layout {!s} is not normalized".format(layoutid))
                        delete.add(layoutid)
                if major is not None and minor is not None and abs(vecdot(major, minor)) > 1.0E-3:
                    self.__bemoan("Principial components of layout {!s} are not orthogonal".format(layoutid))
                    delete.add(layoutid)
                for (axis, axtb, prop) in [
                        (major, 'MajorAxes', Properties.PRINCOMP1ST),
                        (minor, 'MinorAxes', Properties.PRINCOMP2ND),
                ]:
                    for table in [ 'PropertiesDisc', 'PropertiesCont' ]:
                        if axis is None and self.__mngr.sql_select_curs(curs, table, layout=layoutid, property=prop):
                            self.__bemoan("Layout {!s} has entry in table {!r} but not in {!r}"
                                          .format(layoutid, table, axtb))
                            delete.add(layoutid)
            for layoutid in delete:
                self.__remove_from_db(curs, "DELETE FROM `MajorAxes` WHERE `layout` = ?", layoutid)
                self.__remove_from_db(curs, "DELETE FROM `MinorAxes` WHERE `layout` = ?", layoutid)
                template = "DELETE FROM `{table:s}` WHERE `layout` = ? AND `property` = ?"
                for tab in [ 'PropertiesDisc', 'PropertiesCont' ]:
                    for prop in [ Properties.PRINCOMP1ST, Properties.PRINCOMP2ND ]:
                        self.__remove_from_db(curs, template.format(table=tab), layoutid, prop)

    def __check_metrics(self):
        required = [ 'metric', 'layout', 'value' ]
        crossrefs = { 'layout' : ('Layouts', 'id') }
        self.__check_table('Metrics', required=required, crossrefs=crossrefs)

    def __check_test_scores(self):
        required = [ 'lhs', 'rhs', 'test', 'value' ]
        crossrefs = { 'lhs' : ('Layouts', 'id'), 'rhs' : ('Layouts', 'id') }
        self.__check_table('TestScores', required=required, crossrefs=crossrefs)

    def __check_table(self, table : str, required : list = None, crossrefs : dict = None):
        if required is None: required = list()
        if crossrefs is None: crossrefs = dict()
        logging.info("Checking database table {!r} ...".format(table))
        with self.__mngr.sql_ctx as curs:
            nullcount = 0
            for row in self.__mngr.sql_select_curs(curs, table):
                nullcount += count(filter(None, map(lambda col : row[col] is None, required)))
                for col in crossrefs.keys():
                    (reftab, refcol) = crossrefs[col]
                    if not self.__mngr.sql_select_curs(curs, reftab, **{ refcol : row[col] }):
                        self.__bemoan("Dangling reference from {:s}.{:s} to {:s}.{:s} on key {!r}".format(
                            table, col, reftab, refcol, row[col]))
                        self.__remove_from_db(curs, "DELETE FROM `{:s}` WHERE `{:s}` = ?".format(table, col), row[col])
            if nullcount:
                self.__bemoan("{:,d} required values are missing from table {!r}".format(nullcount, table))
            for col in required:
                self.__remove_from_db(curs, "DELETE FROM `{:s}` WHERE `{:s}` ISNULL".format(table, col))

    def __check_files(self, table, pattern, column='file', where=None):
        if self.__files is None:
            return
        if where is None:
            where = dict()
        assert column not in where
        where[column] = object
        with self.__mngr.sql_ctx as curs:
            files_in_db = set(index_projection(column, self.__mngr.sql_select_curs(curs, table, **where)))
            logging.info("{:,d} files referenced in {:s}.{:s}".format(len(files_in_db), table, column))
            files_in_fs = set(filter(lambda s : fnmatch.fnmatch(s, pattern), self.__files.keys()))
            logging.info("{:,d} files match {!r}".format(len(files_in_fs), pattern))
            for filename in files_in_db - files_in_fs:
                self.__bemoan("File does not exist: {:s}".format(filename))
                self.__remove_from_db(curs, "DELETE FROM `{:s}` WHERE `{:s}` = ?".format(table, column), filename)
            for filename in files_in_fs - files_in_db:
                (what, size) = _get_file_info(self.__files[filename])
                self.__bemoan("Stray file {!r} ({:s}, {:,d} bytes)".format(filename, what, size))
                self.__remove_from_fs(filename)

def _get_file_info(info):
    st = info.stat(follow_symlinks=False)
    if stat.S_ISSOCK(st.st_mode):
        what = "socket"
    elif stat.S_ISLNK(st.st_mode):
        what = "symbolic link" if os.path.exists(info.path) else "broken symbolic link"
    elif stat.S_ISREG(st.st_mode):
        what = "regular file"
    elif stat.S_ISBLK(st.st_mode):
        what = "block special device"
    elif stat.S_ISDIR(st.st_mode):
        what = "directory"
    elif stat.S_ISCHR(st.st_mode):
        what = "character special device"
    elif stat.S_ISIFO(st.st_mode):
        what = "FIFO"
    elif stat.S_ISDOOR(st.st_mode):
        what = "door"
    elif stat.S_ISPORT(st.st_mode):
        what = "event port"
    elif stat.S_ISWHT(st.st_mode):
        what = "whiteout"
    else:
        what = "unknown"
    return (what, st.st_size)

def _isbetween(value, lower, upper):
    if lower is not None and value < lower:
        return False
    if upper is not None and value > upper:
        return False
    return True

class Main(AbstractMain):

    def _run(self, ns):
        config = Configuration(ns.configdir)
        ostr = None
        try:
            if ns.journal is not None:
                ostr = open(ns.journal, 'w')
            with Manager(absbindir=ns.bindir, absdatadir=ns.datadir, config=config) as mngr:
                chkr = Checker(mngr, repair=bool(ns.repair), fast=ns.fast, journal=ostr)
                if not ns.repair:
                    chkr()
                for i in range(1, 1 + ns.repair):
                    logging.notice("Starting check and repair round {:,d} (limit is {:,d}) ...".format(i, ns.repair))
                    if chkr(iteration=i) == 0:
                        logging.notice("No issues found in check and repair round {:,d} ...".format(i))
                        break
                    else:
                        logging.notice("{:,d} issues found in check and repair round {:,d} ...".format(chkr.issues, i))
                    ostr.flush()
                if chkr.issues:
                    raise SanityError("{:,d} remaining issues".format(chkr.issues))
            logging.notice("Consider pruning empty directories by running an equivalent of the following command:")
            logging.notice("find {:s} -type d -empty -delete".format(shlex.quote(ns.datadir)))
        finally:
            if ostr is not None:
                ostr.close()
                logging.notice("A report was written to {!r}".format(ns.journal))

    def _argparse_hook_before(self, ap):
        ag = ap.add_argument_group("Modus Operandi")
        ag.add_argument('--repair', '-r', metavar='N', type=int, nargs='?', default=0, const=1,
                        help="attempt to fix any detected issues (repeat up to N times)")
        ag.add_argument('--journal', '-j', metavar='FILE', help="write issues to FILE")
        ag.add_argument('--fast', '-f', action='store_true', help="skip expensive checks")

if __name__ == '__main__':
    kwargs = {
        'prog' : PROGRAM_NAME,
        'usage' : "%(prog)s [--repair[=N]]",
        'description' : "Performs some integrity checks on the database and file system storage.",
    }
    with Main(**kwargs) as app:
        app(sys.argv[1 : ])
