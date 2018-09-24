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

__all__ = [ 'DATA_ROOT_TAG_FILE', 'Manager' ]

import io
import json
import logging
import math
import os
import secrets
import shutil
import signal
import sqlite3
import string
import subprocess
import sys
import tempfile
import time

from .constants import *
from .errors import *
from .resources import *
from .tools import *
from .utility import *
from .xjson import *

DATA_ROOT_TAG_FILE = 'DATADIR.TAG'

class Manager(object):

    def __init__(self, absbindir, absdatadir, timeout=None, config=None):
        def chkabs(s): assert os.path.isabs(s); return s
        assert config is not None
        self.__config = config
        self.__abs_bindir = chkabs(absbindir)
        self.__abs_datadir = chkabs(absdatadir)
        self.__timeout = timeout
        self.__db_connection = None
        self.__old_cwd = None
        _register_sqlite_types()

    @property
    def config(self):
        return self.__config

    @property
    def abs_bindir(self):
        return self.__abs_bindir

    @property
    def abs_datadir(self):
        return self.__abs_datadir

    @property
    def oldcwd(self):
        return self.__old_cwd

    @property
    def timeout(self):
        return self.__timeout

    @property
    def graphsdir(self):
        return 'graphs'

    @property
    def layoutdir(self):
        return 'layouts'

    @property
    def propsdir(self):
        return 'properties'

    @property
    def nndir(self):
        return 'model'

    @property
    def database(self):
        return 'graphstudy.db'

    @property
    def badlog(self):
        return 'badlog.pickle'

    @property
    def nn_features(self):
        return os.path.join(self.nndir, 'features.pickle')

    @property
    def nn_model(self):
        return os.path.join(self.nndir, 'model.yaml')

    @property
    def nn_weights(self):
        return os.path.join(self.nndir, 'weights.hdf5')

    @property
    def alt_huang_params(self):
        return os.path.join(self.nndir, 'huang.pickle')

    def __enter__(self):
        self.__old_cwd = os.getcwd()
        make_tag = False
        if not os.path.exists(self.abs_datadir):
            logging.info("Creating directory {!r} ...".format(self.abs_datadir))
            os.mkdir(self.abs_datadir)  # Fails if exists
            make_tag = True
        logging.debug("Entering directory {!r}".format(self.abs_datadir))
        os.chdir(self.abs_datadir)
        if make_tag:
            logging.info("Creating tag file {!r} ...".format(DATA_ROOT_TAG_FILE))
            with open(DATA_ROOT_TAG_FILE, 'w') as ostr: pass
        if not os.path.exists(DATA_ROOT_TAG_FILE):
            raise FatalError("Directory {!r} contains no {!r} file".format(self.abs_datadir, DATA_ROOT_TAG_FILE))
        logging.info("Connecting to database {!r} ...".format(self.database))
        self.__db_connection = sqlite3.connect(self.database, detect_types=sqlite3.PARSE_DECLTYPES)
        with get_resource_as_stream('schemata.sql') as istr:
            schemata = istr.read().decode()
        with self.__db_connection as curs:
            curs.executescript(schemata)
        self.__db_connection.row_factory = sqlite3.Row
        return self

    def __exit__(self, *args):
        if self.__db_connection is not None:
            logging.debug("Closing database connection")
            self.__db_connection.close()
            self.__db_connection = None
        if self.__old_cwd is not None:
            logging.debug("Leaving directory {!r}".format(self.abs_datadir))
            os.chdir(self.__old_cwd)
            self.__old_cwd = None

    def clean_all(self):
        assert os.path.exists(os.path.join(self.abs_datadir, DATA_ROOT_TAG_FILE))
        logging.warning("Recrusively deleting data root directory {!r} ...".format(self.abs_datadir))
        shutil.rmtree(self.abs_datadir)

    def clean_graphs(self):
        assert os.path.realpath(os.getcwd()) == os.path.realpath(self.abs_datadir)
        logging.notice("Deleting all existing graphs ...")
        with self.sql_ctx as curs:
            curs.execute("DROP TABLE `Graphs`")
        if os.path.isdir(self.graphsdir):
            shutil.rmtree(self.graphsdir)

    def clean_layouts(self):
        assert os.path.realpath(os.getcwd()) == os.path.realpath(self.abs_datadir)
        logging.notice("Deleting all existing layouts ...")
        with self.sql_ctx as curs:
            curs.execute("DROP TABLE `Layouts`")
        if os.path.isdir(self.layoutdir):
            shutil.rmtree(self.layoutdir)

    def clean_inter(self):
        assert os.path.realpath(os.getcwd()) == os.path.realpath(self.abs_datadir)
        logging.notice("Deleting all existing inter layouts ...")
        with self.sql_ctx as curs:
            filemap = { r['id'] : r['file'] for r in self.sql_select_curs(curs, 'Layouts', layout=None) }
            ids = { r['id'] for r in self.sql_select_curs(curs, 'InterLayouts') }
            files = filter(None, (filemap.get(lid) for lid in ids))
            curs.executemany("DELETE FROM `Layouts` WHERE `id` = ?", map(singleton, ids))
            curs.execute("DROP TABLE `InterLayouts`")
        for f in files:
            logging.debug("Deleting inter layout file {!r} ...".format(f))
            try: os.remove(f)
            except FileNotFoundError: pass

    def clean_worse(self):
        assert os.path.realpath(os.getcwd()) == os.path.realpath(self.abs_datadir)
        logging.notice("Deleting all existing worse layouts ...")
        with self.sql_ctx as curs:
            filemap = { r['id'] : r['file'] for r in self.sql_select_curs(curs, 'Layouts', layout=None) }
            ids = { r['id'] for r in self.sql_select_curs(curs, 'WorseLayouts') }
            files = filter(None, (filemap.get(lid) for lid in ids))
            curs.executemany("DELETE FROM `Layouts` WHERE `id` = ?", map(singleton, ids))
            curs.execute("DROP TABLE `WorseLayouts`")
        for f in files:
            logging.debug("Deleting worse layout file {!r} ...".format(f))
            try: os.remove(f)
            except FileNotFoundError: pass

    def clean_properties(self):
        assert os.path.realpath(os.getcwd()) == os.path.realpath(self.abs_datadir)
        logging.notice("Deleting all existing properties ...")
        tables = [ 'PropertiesDisc', 'PropertiesCont', 'Histograms', 'SlidingAverages', 'MajorAxes', 'MinorAxes' ]
        with self.sql_ctx as curs:
            for table in tables:
                curs.execute("DROP TABLE `{table:s}`".format(table=table))
        if os.path.isdir(self.propsdir):
            shutil.rmtree(self.propsdir)

    def clean_metrics(self):
        assert os.path.realpath(os.getcwd()) == os.path.realpath(self.abs_datadir)
        logging.notice("Deleting all existing metrics ...")
        with self.sql_ctx as curs:
            curs.execute("DROP TABLE `Metrics`")

    def clean_model(self):
        assert os.path.realpath(os.getcwd()) == os.path.realpath(self.abs_datadir)
        logging.notice("Deleting existing model ...")
        with self.sql_ctx as curs:
            curs.execute("DROP TABLE `TestScores`")
        if os.path.isdir(self.nndir):
            shutil.rmtree(self.nndir)

    def make_unique_layout_id(self, curs):
        for i in range(100):
            token = Id(secrets.token_bytes(16))
            if not self.sql_select_curs(curs, 'Layouts', id=token):
                return token
        raise SanityError("Unable to generate a unique layout ID token")

    def make_graph_filename(self, graphid : Id, generator : Generators = None):
        assert isinstance(graphid, Id)
        basename = str(graphid) if generator is None else '{!s}-{:s}'.format(graphid, enum_to_json(generator))
        return os.path.join(self.graphsdir, basename + GRAPH_FILE_SUFFIX)

    def make_layout_filename(self, graphid : Id, layoutid : Id, layout : Layouts = None):
        assert isinstance(graphid, Id) and isinstance(layoutid, Id)
        basename = str(layoutid) if layout is None else '{!s}-{:s}'.format(layoutid, enum_to_json(layout))
        return os.path.join(self.layoutdir, str(graphid), basename + LAYOUT_FILE_SUFFIX)

    def make_tempdir(self):
        return tempfile.TemporaryDirectory(dir=os.curdir, prefix='temp-')

    @property
    def sql_ctx(self):
        logging.debug("Starting SQL transaction")
        return self.__db_connection

    def sql_exec(self, query, params):
        with self.sql_ctx as curs:
            return self.sql_exec_curs(curs, query, params)

    def sql_exec_curs(self, curs, query, params):
        assert curs is not None
        params = params if type(params) is tuple else tuple(params)
        logging.debug("Executing SQL query: " + query)
        logging.debug("Bound variables are: " + '(' + ', '.join(map(sqlrepr, params)) + ')')
        rows = curs.execute(query, params).fetchall()
        logging.debug("Fetched {:d} rows from SQL database".format(len(rows)))
        return rows

    def sql_select(self, table, **kwargs):
        with self.sql_ctx as curs:
            return self.sql_select_curs(curs, table, **kwargs)

    def sql_select_curs(self, curs, table, **kwargs):
        assert curs is not None
        query = "SELECT * FROM `{:s}`".format(table)
        if kwargs:
            def fmtclause(key, val):
                if   val is Ellipsis: return None
                elif val is None:     return "`{:s}` ISNULL".format(key)
                elif val is object:   return "`{:s}` NOTNULL".format(key)
                else:                 return "`{:s}` = ?".format(key)
            query += " WHERE " + " AND ".join(filter(None, (fmtclause(k, v) for (k, v) in kwargs.items())))
        paramiter = filter(lambda o : o not in [ Ellipsis, None, object ], kwargs.values())
        return self.sql_exec_curs(curs, query, paramiter)

    def sql_insert(self, table, **kwargs):
        with self.sql_ctx as curs:
            return self.sql_insert_curs(curs, table, **kwargs)

    def sql_insert_curs(self, curs, table, **kwargs):
        assert curs is not None
        query = (
            "INSERT INTO `{:s}` ".format(table)
            + '(' + ', '.join('`{:s}`'.format(k) for k in kwargs.keys()) + ')'
            + " VALUES "
            + '(' + ', '.join('?' for x in kwargs) + ')'
        )
        return self.sql_exec_curs(curs, query, kwargs.values())

    def call_graphstudy_tool(self, cmd, meta=None, stdin=None, stdout=None, deterministic=False):
        assert meta != 'stdout' or stdout is None
        subenv = dict(os.environ)
        kwargs = {
            'timeout' : self.timeout,
            'stdout'  : subprocess.PIPE if meta == 'stdout' or stdout is True else stdout or subprocess.DEVNULL,
            'stderr'  : subprocess.PIPE,
            'env'     : subenv,
        }
        _handle_popen_stdin(kwargs, stdin)
        if deterministic:
            subenv['MSC_RANDOM_SEED'] = ''.join('0' for i in range(48))
        logging.debug("Executing command {!r} ...".format(cmd))
        t0 = time.time()
        try:
            result = subprocess.run(cmd, **kwargs)
        except subprocess.TimeoutExpired:
            logging.error("Command did not complete until timeout ({:.3f} seconds) expired".format(self.timeout))
            raise RecoverableError("External program was killed")
        except OSError as e:
            logging.error(str(e))
            raise RecoverableError("External program could not be executed")
        t1 = time.time()
        logging.debug("Command completed with status {:d} after {:.3f} seconds".format(result.returncode, t1 - t0))
        if result.returncode == 0:
            self.__record_exec_time(cmd[0], t1 - t0)
        else:
            _log_data(result.stderr, description="Standard error output")
            try:
                signame = signal.Signals(-result.returncode).name;
                raise RecoverableError("External program crashed (killed by {:s})".format(signame))
            except ValueError:
                raise RecoverableError("External program crashed")
        if meta is None: jsondata = None
        elif meta == 'stdout': jsondata = result.stdout
        elif meta == 'stderr': jsondata = result.stderr
        else: raise ValueError(meta)
        if jsondata is not None:
            try:
                jsontext = jsondata.decode()
            except UnicodeError as e:
                logging.error(str(e))
                _log_data(jsondata)
                raise RecoverableError("Cannot decode meta output of external tool as Unicode")
            try:
                meta = load_xjson_string(jsontext)
                if meta is None: raise RecoverableError("External tool produced no JSON output")
                return meta
            except XJsonError as e:
                _log_data(jsondata)
                raise RecoverableError("Cannot parse meta output of external tool as JSON")
        if stdout is True:
            assert result.stdout is not None
            return result.stdout

    def call_gnuplot(self, script, stdout=None):
        cmd = find_tool_lazily(GNUPLOT, split_tokens=True)
        cmd.extend(get_gnuplot_options())
        kwargs = {
            'input' : script.encode(),
            'stderr' : subprocess.PIPE,
        }
        _handle_popen_stdout(kwargs, stdout)
        logging.debug("Executing command {!r} ...".format(cmd))
        t0 = time.time()
        try:
            status = subprocess.run(cmd, **kwargs, env=get_gnuplot_environment())
        except OSError as e:
            logging.error(str(e))
            raise RecoverableError("External program could not be executed")
        t1 = time.time()
        logging.debug("Gnuplot completed with status {:d} after {:.3f} seconds".format(status.returncode, t1 - t0))
        if status.returncode == 0:
            self.__record_exec_time(cmd[0], t1 - t0)
        else:
            _log_data(status.stderr, description="Standard error output")
            _log_data(script, description="Offending Gnuplot script")
            raise RecoverableError("Gnuplot crashed")
        return status.stdout

    def call_image_magick(self, args, stdin=None, stdout=None):
        cmd = find_tool_lazily(IMAGE_MAGICK, split_tokens=True)
        cmd.extend(args)
        kwargs = dict()
        _handle_popen_stdin(kwargs, stdin)
        _handle_popen_stdout(kwargs, stdout)
        kwargs['stderr'] : subprocess.PIPE
        logging.debug("Executing command {!r} ...".format(cmd))
        t0 = time.time()
        try:
            status = subprocess.run(cmd, **kwargs)
        except OSError as e:
            logging.error(str(e))
            raise RecoverableError("External program could not be executed")
        t1 = time.time()
        logging.debug("ImageMagick completed with status {:d} after {:.3f} seconds".format(status.returncode, t1 - t0))
        if status.returncode == 0:
            self.__record_exec_time(cmd[0], t1 - t0)
        else:
            _log_data(status.stderr, description="Standard error output")
            raise RecoverableError("ImageMagick crashed")
        return status.stdout

    def idmatch(self, *args, **kwargs):
        with self.sql_ctx as curs:
            return self.idmatch_curs(curs, *args, **kwargs)

    def idmatch_curs(self, curs, table, prefix, fingerprint=False, minlength=None, exception=ValueError):
        assert table in [ 'Graphs', 'Layouts' ]
        assert not fingerprint or table == 'Layouts'
        (column, what) = ('fingerprint', "fingerprint") if fingerprint else ('id', "ID")
        prefix = prefix.strip()
        if not prefix:
            raise exception("The empty string is not an unambiguous prefix for an {:s}".format(what))
        if any(c not in string.hexdigits for c in prefix):
            raise exception("The string {!r} cannot possibly be a prefix of any {:s}".format(prefix, what))
        if minlength is not None and len(prefix) < minlength:
            raise exception("Prefix {!s} is shorter than {:d} bytes".format(prefix, minlength))
        parameters = singleton(prefix.upper() + '%')
        query = "SELECT `id` FROM `{tab:s}` WHERE hex(`{col:s}`) LIKE ?".format(tab=table, col=column)
        rows = self.sql_exec_curs(curs, query, parameters)
        if not rows:
            raise exception("Prefix {!s} does not match any {:s}".format(prefix, what))
        if len(rows) > 1:
            raise exception("Prefix {!s} is ambiguous (matches {:d} {:s}s)".format(prefix, len(rows), what))
        return get_one(get_one(rows))

    def __record_exec_time(self, program, time):
        tool = os.path.basename(program)
        assert type(tool) is str
        assert type(time) is float
        self.sql_insert('ToolPerformance', tool=tool, time=time)

def _log_data(data, description="Bogus data"):
    logging.notice(description + " was {:d} bytes long".format(len(data)))
    try:
        text = data if isinstance(data, str) else data.decode()
        for (i, line) in enumerate(map(str.rstrip, text.splitlines())):
            logging.notice("{:6d}: {:s}".format(i + 1, line))
    except UnicodeError as e:
        (dumpfd, dumpfilename) = tempfile.mkstemp(prefix='dump-', suffix='.dat')
        logging.error(description + " could not be decoded as Unicode data;"
                      + " dumping binary data to file {!r} ...".format(dumpfilename))
        with os.fdopen(dumpfd, 'wb') as ostr:
            ostr.write(data)

def _handle_popen_stdin(kwargs, stdin):
    assert 'stdin' not in kwargs
    assert 'input' not in kwargs
    if stdin is None or stdin is False:
        kwargs['stdin'] = subprocess.DEVNULL
    elif isinstance(stdin, bytes):
        kwargs['input'] = stdin
    elif isinstance(stdin, str):
        raise TypeError("No textual input, please!")
    elif _has_functional_fileno_attribute(stdin):
        kwargs['stdin'] = stdin
    elif isinstance(stdin, io.BytesIO):
        kwargs['input'] = stdin.getbuffer()
    else:
        kwargs['input'] = stdin.read()
    assert ('stdin' in kwargs) != ('input' in kwargs)

def _handle_popen_stdout(kwargs, stdout):
    assert 'stdout' not in kwargs
    if stdout is None or stdout is False:
        kwargs['stdout'] = subprocess.DEVNULL
    elif stdout is True:
        kwargs['stdout'] = subprocess.PIPE
    elif _has_functional_fileno_attribute(stdout):
        kwargs['stdout'] = stdout
    else:
        raise TypeError("Parameter 'stdout' must be None, False, True or an open file")

# Do you think this is a bad joke?  Alas, it's very true ...
def _has_functional_fileno_attribute(obj):
    try:
        fd = obj.fileno()
        return isinstance(obj.fileno(), int) and fd >= 0
    except Exception:
        return False

def _register_sqlite_types():
    sqlite3.register_converter('LOGICAL', lambda blob : bool(int(blob)))
    sqlite3.register_adapter(bool, lambda value : int(value).encode())
    sqlite3.register_converter('BLOB_ID', Id)
    sqlite3.register_adapter(Id, Id.getkey)
    sqlite3.register_converter('ENUM_GENERATORS', lambda blob : Generators(int(blob)))
    sqlite3.register_converter('ENUM_LAYOUTS', lambda blob : Layouts(int(blob)))
    sqlite3.register_converter('ENUM_LAY_INTER', lambda blob : LayInter(int(blob)))
    sqlite3.register_converter('ENUM_LAY_WORSE', lambda blob : LayWorse(int(blob)))
    sqlite3.register_converter('ENUM_PROPERTIES', lambda blob : Properties(int(blob)))
    sqlite3.register_converter('ENUM_BINNINGS', lambda blob : Binnings(int(blob)))
    sqlite3.register_converter('ENUM_METRICS', lambda blob : Metrics(int(blob)))
    sqlite3.register_converter('ENUM_TESTS', lambda blob : Tests(int(blob)))
    # No adapters are needed for the enumerations because enums /are/ integers.
