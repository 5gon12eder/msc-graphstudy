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

__all__ = [
    # types
    'ImportSource',
    'DirectoryImportSource',
    'NullImportSource',
    'TarImportSource',
    'UrlImportSource',
    # functions
    'get_import_source_from_json',
    'get_well_known_import_sources',
]

import dbm
import fnmatch
import hashlib
import inspect
import io
import json
import logging
import os
import pathlib
import re
import shutil
import tarfile
import tempfile
import urllib.request

from .constants import *
from .errors import *
from .resources import *
from .tools import *
from .xjson import *

class ImportSource(object):

    def __init__(self, format : str = None, compression : str = None, layout : bool = False, simplify : bool = False):
        self.format = format
        self.compression = compression
        self.layout = layout
        self.simplify = simplify
        self._candidates = None

    def __enter__(self):
        self._candidates = set()
        self._set_up()
        return self

    def __exit__(self, *args):
        self._tear_down()
        self._candidates = None

    def __len__(self) -> int:
        return len(self._candidates)

    def __iter__(self):
        for cand in self._candidates:
            yield cand

    @property
    def name(self) -> str:
        """
        Returns an informal string representation of the archive source, such as a file name.
        """
        raise NotImplementedError()

    def _set_up(self):
        """
        This function will be called on context entry.  Derived classes can perform their set up logic here.  They
        should be careful to call `super()._set_up()` as the very *first* thing, though.
        """
        pass

    def _tear_down(self):
        """
        This function will be called on context exit.  Derived classes can perform their tear down logic here.  They
        should be careful to call `super()._tear_down()` as the very *last* thing, though.
        """
        pass

    def get(self, candidate : object) -> io.BytesIO:
        """
        Opens the specified candidate (which must be one previously obtained by iterating over the same source) and
        returns a file-like object in binary mode that can (and should) be used in a `with` statement.
        """
        raise NotImplementedError()

    def prettyname(self, item : object) -> str:
        """
        Returns a string that informally identifies the candidate `item` which must have been obtained from iterating
        the archive members.
        """
        assert isinstance(item, str)
        return item

    def is_likely_read_error(self, exc : Exception) -> bool:
        """
        Tests whether an exception was likely caused by a reading error from this source.
        """
        return False

class NullImportSource(ImportSource):

    def __init__(self):
        super().__init__()

    @property
    def name(self):
        return 'NULL'

    def get(self, candidate : str):
        raise ValueError(candidate)

class DirectoryImportSource(ImportSource):

    def __init__(self,
                 directory : str,
                 format : str,
                 compression : str = None,
                 pattern : str = '*',
                 recursive : bool = False,
                 layout : bool = False,
                 simplify : bool = False,
    ):
        super().__init__(format=format, compression=compression, layout=layout, simplify=simplify)
        self.__pattern = pattern
        self.__recursive = recursive
        self.__directory = os.path.expanduser(os.path.expandvars(directory))
        if '$' in self.__directory or self.__directory.startswith('~'):
            raise FatalError("Cannot expand all variables in directory name: {!r}".format(directory))

    def _set_up(self):
        super()._set_up()
        try:
            self.__scan_directory(self.__directory)
        except OSError as e:
            logging.error("Cannot index local archive {!r}: {!s}".format(self.name, e))
            raise RecoverableError()

    def _tear_down(self):
        super()._tear_down()

    def __scan_directory(self, directory):
        # Yes, we will run into a ENFILE / EMFILE condition if the directory hierarchy is excessively deep.
        with os.scandir(directory) as items:
            for item in items:
                subpath = os.path.relpath(item.path, start=self.__directory)
                if item.is_dir():
                    if self.__recursive:
                        self.__scan_directory(item.path)
                elif fnmatch.fnmatch(subpath, self.__pattern):
                    if item.is_file():
                        self._candidates.add(item.path)
                    else:
                        logging.warning("Ignoring {!r} which is neither a regular file nor a directory".format(subpath))

    @property
    def name(self):
        return self.__directory

    def get(self, item : str):
        if item not in self._candidates:
            raise ValueError(item)
        return open(item, 'rb')

    def is_likely_read_error(self, exc):
        return isinstance(exc, OSError)

class TarImportSource(ImportSource):

    def __init__(self,
                 url : str,
                 format : str,
                 compression : str = None,
                 cache : str = None,
                 checksum : str = None,
                 pattern : str = '*',
                 layout : bool = False,
                 simplify : bool = False,
    ):
        super().__init__(format=format, compression=compression, layout=layout, simplify=simplify)
        self.__tarurl = url
        self.__cache = _get_cache_file_name(cache)
        self.__checksum = checksum
        self.__pattern = pattern
        self.__tempfile = None
        self.__tarball = None

    def _set_up(self):
        super()._set_up()
        good = False
        try:
            if self.__cache is not None:
                try:
                    self.__tempfile = open(self.__cache, 'rb')
                    logging.info("Found tar archive {!r} in cache file {!r} ...".format(self.__tarurl, self.__cache))
                    self.__copy_and_verify(self.__tempfile, None)
                except FileNotFoundError:
                    pass
            if self.__tempfile is None:
                if self.__cache is None:
                    self.__tempfile = tempfile.TemporaryFile()
                    logging.info("Saving tar archive {!r} to anonymous temporary file ...".format(self.__tarurl))
                else:
                    self.__tempfile = open(self.__cache, 'w+b')
                    logging.info("Saving tar archive {!r} to cache file {!r} ...".format(self.__tarurl, self.__cache))
                with urllib.request.urlopen(self.__tarurl) as istr:
                    self.__copy_and_verify(istr, self.__tempfile)
            self.__tempfile.seek(0)
            self.__tarball = tarfile.open(name=self.name, fileobj=self.__tempfile, mode='r:*')
            self.__scan_tarball()
            good = True
        except (OSError, tarfile.TarError) as e:
            logging.error("Cannot index tar archive {!r}: {!s}".format(self.name, e))
            raise RecoverableError()
        finally:
            if not good:
                self._tear_down(call_super=False)

    def _tear_down(self, call_super=True):
        if self.__tarball is not None:
            self.__tarball.close()
            self.__tarball = None
        if self.__tempfile is not None:
            self.__tempfile.close()
            self.__tempfile = None
        if call_super:
            super()._tear_down()

    def __copy_and_verify(self, istr, ostr=None):
        chunksize = 0x2000  # This used to be Python's internal default buffer size so it might be a good choice
        if self.__checksum is None:
            if ostr is not None:
                shutil.copyfileobj(istr, ostr)
        else:
            (hasher, expected) = self.__get_hasher()
            while True:
                # Reading from an arbitrary file-like object into a reusable buffer turns out to be so much hassle in
                # Python that I just gave up and allocate a new chunk of memory for each iteration.
                chunk = istr.read(chunksize)
                if not chunk: break
                hasher.update(chunk)
                if ostr is not None: ostr.write(chunk)
            archive = getattr(istr, 'name', self.name)
            logging.info("Archive {!r} has {:s} checksum {:s}".format(archive, hasher.name.upper(), hasher.hexdigest()))
            if hasher.digest() != expected:
                raise SanityError("Archive {!r} has wrong {:s} checksum".format(archive, hasher.name.upper()))

    def __get_hasher(self):
        assert self.__checksum is not None
        (algo, colon, chksum) = self.__checksum.partition(':')
        if colon != ':': raise FatalError("Invalid checksum: {!r}".format(self.__checksum))
        try:
            hasher = hashlib.new(algo.lower())
        except ValueError:
            raise FatalError("Unknown cryptographic hash function: {!r}".format(algo))
        try:
            expected = bytes.fromhex(chksum)
        except ValueError:
            raise FatalError("Invalid hex-encoded message digest: {!r}".format(chksum))
        return (hasher, expected)

    def __scan_tarball(self):
        for item in self.__tarball:
            if fnmatch.fnmatch(item.name, self.__pattern):
                if item.isfile():
                    self._candidates.add(item)
                elif item.isdir():
                    pass  # Tar archives are not really hierarchically, there is nothing to do with a directory entry
                else:
                    # We do not bother handling link entries.  It would be difficult and open us up to symlink attacks.
                    # There is no real reason why a graph collection should contain links and the ones we use don't.
                    what = { tarfile.LNKTYPE : "hard link", tarfile.SYMTYPE : "symbolic link" }.get(item.type, "entry")
                    logging.warning("Ignoring matching {:s} {!r} (type = {:d}) in tar archive {!r}".format(
                        item.name, what, int(item.type), self.name))

    @property
    def name(self):
        return self.__tarurl

    def get(self, item : str):
        assert item in self._candidates
        return self.__tarball.extractfile(item)

    def prettyname(self, item) -> str:
        assert item in self._candidates
        return item.name

    def is_likely_read_error(self, exc):
        return any(map(lambda typ : isinstance(exc, typ), [ OSError, tarfile.TarError ]))

class UrlImportSource(ImportSource):

    def __init__(self,
                 urls : list,
                 format : str,
                 compression : str = None,
                 layout : bool = False,
                 simplify : bool = False,
                 name : str = 'www',
                 cache : bool = False,
    ):
        super().__init__(format=format, compression=compression, layout=layout, simplify=simplify)
        self.__name = name
        self.__use_cache = bool(cache)
        self.__urls = list(urls)
        self.__cache_db = None
        for url in self.__urls:
            if not isinstance(url, str):
                raise ConfigError("URLs must be strings: {!r}".format(url))
        if not re.match(r'^\w+$', self.__name):
            raise ConfigError("The name of a URL import source must be a valid identifier: {!r}".format(self.__name))

    def _set_up(self):
        super()._set_up()
        self._candidates.update(self.__urls)
        if self.__use_cache:
            dbfile = _get_cache_file_name(self.__name + '.db')
            logging.info("Using cache database {!r} for URL archive {!r}".format(dbfile, self.__name))
            self.__cache_db = dbm.open(dbfile, 'c')
            logging.info("Database {!r} contains {:,d} entries".format(dbfile, len(self.__cache_db)))

    def _tear_down(self):
        if self.__cache_db is not None:
            self.__cache_db.close()
        super()._tear_down()

    @property
    def name(self):
        return self.__name

    def get(self, item : str):
        assert item in self._candidates
        if not self.__use_cache:
            logging.info("Downloading file {!r} ...".format(item))
            return urllib.request.urlopen(item)
        burl = item.encode()
        data = self.__cache_db.get(burl)
        if data is not None:
            logging.debug("Found {!r} in local cache ({:,d} bytes)".format(item, len(data)))
        else:
            logging.info("Downloading file {!r} into local cache ...".format(item))
            with urllib.request.urlopen(item) as istr:
                data = istr.read()
            logging.debug("Storing {!r} in local cache ({:,d} bytes)".format(item, len(data)))
            self.__cache_db[burl] = data
        return io.BytesIO(data)

    def is_likely_read_error(self, exc):
        # I'm not sure why but if the subprocess that reads from the `BytesIO` object fails and we throw an exception,
        # another `BufferError` is thrown.  It feels wrong to ignore it but then again, what else could we do about it?
        return any(map(lambda typ : isinstance(exc, typ), [ OSError, BufferError ]))

_IMPORT_SOURCE_FACTORIES = {
    'DIR' : DirectoryImportSource,
    'TAR' : TarImportSource,
    'URL' : UrlImportSource,
    None  : NullImportSource,
}

_JSON_TYPENAMES = {
    int        : 'INTEGER',  # aka NUMBER
    float      : 'REAL',     # aka NUMBER
    bool       : 'BOOLEAN',
    str        : 'STRING',
    list       : 'ARRAY',
    dict       : 'OBJECT',
    type(None) : 'NULL',
}

def get_well_known_import_sources():
    with get_resource_as_stream('imports.json') as istr:
        specs = load_xjson_string(istr.read().decode('utf-8'))
    sources = dict()
    for (gen, spec) in map(lambda kv : (Generators[kv[0]], kv[1]), specs.items()):
        assert gen.imported
        sources[gen] = get_import_source_from_json(spec)
    return sources

def get_import_source_from_json(obj, filename=None):
    """
    Brain-dead factory function for import sources:

        >>> # You'd normally load this data from a JSON file, hence the name:
        >>> spec = { 'type' : 'TAR', 'format' : 'graphml', 'url' : 'https://www.example.com/big-graph-library.tar.gz' }
        >>> isrc = get_import_source_from_json(spec)
        >>> isrc.name
        'https://www.example.com/big-graph-library.tar.gz'

    The input data is validated:

        >>> isrc = get_import_source_from_json("Make me a sandwich")
        Traceback (most recent call last):
        driver.errors.ConfigError: Invalid import source definition file
        >>>
        >>> isrc = get_import_source_from_json(dict())
        Traceback (most recent call last):
        driver.errors.ConfigError: Invalid import source definition file
        >>>
        >>> isrc = get_import_source_from_json({ 'type' : 42 })
        Traceback (most recent call last):
        driver.errors.ConfigError: Invalid import source definition file
        >>>
        >>> isrc = get_import_source_from_json({ 'type' : 'XYZ' })
        Traceback (most recent call last):
        driver.errors.ConfigError: Invalid import source definition file

    If the type is `None`, a `NullImportSource` is fabricated:

        >>> isrc = get_import_source_from_json({ 'type' : None })
        >>> isrc.name
        'NULL'

    It allows no parameters:

        >>> isrc = get_import_source_from_json({ 'type' : None, 'file' : '/dev/null' })
        Traceback (most recent call last):
        driver.errors.ConfigError: Invalid import source definition file

    Other source types do require parameters:

        >>> isrc = get_import_source_from_json({ 'type' : 'DIR', 'format' : 'ogml', 'directory' : '/tmp/graphs/' })
        >>> isrc.name
        '/tmp/graphs/'
        >>>
        >>> isrc = get_import_source_from_json({ 'type' : 'DIR' })
        Traceback (most recent call last):
        driver.errors.ConfigError: Invalid import source definition file

    Those must be of the correct types:

        >>> isrc = get_import_source_from_json(
        ...     { 'type' : 'DIR', 'format' : 'ogml', 'directory' : '/tmp/graphs/', 'recursive' : True }
        ... )
        >>> isrc.name
        '/tmp/graphs/'
        >>>
        >>> isrc = get_import_source_from_json(
        ...     { 'type' : 'DIR', 'format' : 'ogml', 'directory' : '/tmp/graphs/', 'recursive' : [ ] }
        ... )
        Traceback (most recent call last):
        driver.errors.ConfigError: Invalid import source definition file

    """
    def throw(message=None, types=False, usage=False):
        if message is not None: logging.error(message)
        if types: _log_types_notice()
        if usage: _log_usage_notice(typename, mandatory, optional, parameters)
        raise ConfigError("Invalid import source definition file", filename=filename)
    def throw_t(message=None) : throw(message, types=True)
    def throw_u(message=None) : throw(message, usage=True)
    if not isinstance(obj, dict):
        throw("Import source definition must be of type OBJECT not {:s}".format(_JSON_TYPENAMES.get(type(obj), '???')))
    try: typ = obj['type']
    except KeyError: throw("Import source definition has no 'type' attribute")
    errmsg = _check_attribute_type('type', typ, str, nullable=True)
    if errmsg: throw(errmsg)
    typename = 'null' if typ is None else typ
    try: factory = _IMPORT_SOURCE_FACTORIES[typ]
    except KeyError: throw_t("Unknown import source type {!r}".format(typename))
    parameters = inspect.getfullargspec(factory)
    mandatory, optional = _get_mandatory_and_optional_parameters(parameters)
    arguments = { k : v for (k, v) in obj.items() if k != 'type' }
    missing = ', '.join(attr for attr in mandatory if attr not in arguments)
    unknown = ', '.join(attr for attr in arguments if attr not in parameters.args)
    if missing:
        throw_u("Missing attributes for import source definition of type {:s}: {:s}".format(typename, missing))
    if unknown:
        throw_u("Invalid attributes for import source definition of type {:s}: {:s}".format(typename, unknown))
    for (key, val) in arguments.items():
        nullable = (optional.get(key, object) is None)
        error = _check_attribute_type(key, val, parameters.annotations[key], typename=typename, nullable=nullable)
        if error: throw_u(error)
    return factory(**arguments)

def _log_types_notice():
    typenames = [ 'null' if k is None else k for k in _IMPORT_SOURCE_FACTORIES.keys() ]
    logging.notice("Known import source types are (case-sensitive): " + ', '.join(typenames))

def _log_usage_notice(typename, mandatory, optional, argspec):
    logging.notice("Valid attributes for import source definition of type {:s} are (case-sensitive):".format(typename))
    (head, *tail) = argspec.args
    assert head == 'self'
    tablerowfmt = '| {:16s}| {:16s}| {:24s}|'
    tablehrule = '+' + '-' * 17 + '+' + '-' * 17 + '+' + '-' * 25 + '+'
    logging.notice(tablehrule)
    logging.notice(tablerowfmt.format("attribute", "type", "default"))
    logging.notice(tablehrule)
    for paramname in tail:
        paramtype = _JSON_TYPENAMES[argspec.annotations[paramname]]
        parametc = '' if paramname in mandatory else json.dumps(optional[paramname])
        logging.notice(tablerowfmt.format(paramname, paramtype, parametc))
    logging.notice(tablehrule)
    logging.notice("Attributes with no default value (not even null) are required")

def _get_mandatory_and_optional_parameters(argspec):
    (this, *mandatory) = argspec.args
    assert this == 'self'
    optional = dict()
    if argspec.defaults is not None:
        for default in reversed(argspec.defaults):
            optional[mandatory.pop()] = default
    return (mandatory, optional)

def _check_attribute_type(attribute : str, value : object, expected : type, typename='unknown', nullable=False):
    if isinstance(value, expected) or (nullable and (value is None)):
        return None
    elif nullable:
        return "Attribute {!r} of {:s} import source definition must be null or of type {:s} not {:s}".format(
            attribute, typename, _JSON_TYPENAMES[expected], _JSON_TYPENAMES.get(type(value), '???')
        )
    else:
        return "Attribute {!r} of {:s} import source definition must be of type {:s} not {:s}".format(
            attribute, typename, _JSON_TYPENAMES[expected], _JSON_TYPENAMES.get(type(value), '???')
        )

def _get_cache_file_name(filename):
    if filename is None:
        return None
    thefilename = os.path.expanduser(filename)
    if thefilename.startswith('~'):
        raise FatalError("Cannot expand user's home directory in {!r}".format(filename))
    if os.path.isabs(thefilename):
        return thefilename
    cachedir = get_cache_directory(fallback=tempfile.gettempdir())
    (dirname, basename) = os.path.split(thefilename)
    if dirname or not basename:
        logging.notice("The 'cache' attribute must be set to an absolute path or a simple file name")
        logging.notice("The cache directory is {!r}".format(cachedir))
        raise ConfigError("Invalid cache file name: {!r}".format(filename))
    return os.path.join(cachedir, basename)
