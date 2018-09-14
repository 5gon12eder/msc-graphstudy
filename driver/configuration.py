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
    'BadLog',
    'Configuration',
]

import glob
import io
import logging
import os.path

from .constants import *
from .errors import *
from .imports import *
from .quarry import *
from .utility import *
from .xjson import *

class BadLog(object):

    def __init__(self, logfile=None, readonly=False):
        self.__filename = logfile
        self.__readonly = bool(readonly) if logfile is not None else None
        self.__badstuff = { k : dict() for k in Actions }
        self.__timestamp = None

    @property
    def filename(self):
        return self.__filename

    @property
    def timestamp(self):
        return self.__timestamp

    def __enter__(self):
        if self.__filename is not None:
            logging.info("Loading \"bad\" log file {!r} ...".format(self.__filename))
            try:
                with open(self.__filename, 'rb') as istr:
                    self.__timestamp, bs = unpickle_objects(istr, dict)
                    for (k, v) in self.__badstuff.items():
                        v.update(bs.get(k, dict()))
            except FileNotFoundError:
                pass
            except OSError as e:
                raise FatalError(str(e))
        return self

    def __exit__(self, *args):
        if self.__filename is not None and not self.__readonly:
            logging.info("Saving \"bad\" log file {!r} ...".format(self.__filename))
            try:
                try:
                    os.rename(self.__filename, self.__filename + '~')
                except FileNotFoundError:
                    pass
                with open(self.__filename, 'wb') as ostr:
                    pickle_objects(ostr, self.__badstuff)
            except OSError as e:
                raise FatalError(str(e))

    def get_bad(self, act : Actions, *args):
        return self.__badstuff[act].get(tuple(args))

    def set_bad(self, act : Actions, *args, msg=None):
        assert not self.__readonly
        if not isinstance(msg, str):
            raise TypeError("Message must be of type 'str' not {!r}".format(type(msg)))
        if not msg:
            raise ValueError("Message must not be the empty string")
        self.__badstuff[act][tuple(args)] = msg

    def iterate(self, act : Actions):
        return self.__badstuff[act].items()

class Configuration(object):

    def __init__(self, configdir):
        filelist = list()
        self.configdir = configdir
        self.import_sources = _CfgImports(self.configdir)(filelist)
        self.desired_graphs = _CfgGraphs(self.configdir)(filelist)
        self.desired_layouts = _CfgLayouts(self.configdir)(filelist)
        self.desired_lay_inter = _CfgLayInter(self.configdir)(filelist)
        self.desired_lay_worse = _CfgLayWorse(self.configdir)(filelist)
        self.desired_properties_disc = _CfgProperties(self.configdir, suffix='disc')(filelist)
        self.desired_properties_cont = _CfgProperties(self.configdir, suffix='cont')(filelist)
        self.puncture = _CfgPuncture(self.configdir)(filelist)
        self.desired_metrics = _CfgMetrics(self.configdir)(filelist)
        self.use_badlog = False
        for other in set(glob.glob(os.path.join(glob.escape(self.configdir), '*.cfg'))) - set(filelist):
            logging.warning("Unrecognized configuration file {!r}".format(other))
        self.__check_puncture('MSC_PUNCTURE')

    def __check_puncture(self, envvar):
        envval = os.getenv(envvar)
        if envval is not None:
            try:
                envval = int(envval)
            except ValueError:
                logging.warning("Ignoing bogous value of environment variable {!s}={!r}".format(envvar, envval))
                envval = None
        if envval is None:
            logging.warning("Environment variable {!s} is not set; cannot check punctures".format(envvar))
            return
        logging.info("Checking that {1:d} properties are punctured ({0!s}={1!r})".format(envvar, envval))
        if envval != len(self.puncture):
            raise SanityError("Expected {:d} punctured properties but found {:d}".format(envval, len(self.puncture)))

class _Config(object):

    def __init__(self, directory=None):
        self.__configdir = directory

    @property
    def configfile(self):
        if self._basename is None:
            return '/dev/stdin'
        if self.__configdir is None:
            return self._basename
        return os.path.join(self.__configdir, self._basename)

    def __call__(self, filelist=None):
        if self.__configdir is None:
            return self.default()
        try:
            with open(self.configfile, 'r') as istr:
                if filelist is not None:
                    filelist.append(self.configfile)
                logging.info("Reading configuration file {!r} ...".format(self.configfile))
                return self.parse(istr)
        except FileNotFoundError:
            return self.default()

    @property
    def _basename(self) -> str:
        return None

    def default(self) -> object:
        raise NotImplementedError()

    def parse(self, istr : io.TextIOBase) -> object:
        raise NotImplementedError()

class _ConfigBySize(_Config):

    def _do_parse(self, what, istr, whatname="key"):
        desired = dict()
        cr = ConfigReader(istr, filename=self.configfile)
        for line in cr:
            (head, *tail) = line.split()
            try: thing = what[head]
            except KeyError: cr.failure("Unknown {:s}: {:s}".format(whatname, head))
            if thing in desired:
                cr.failure("Duplicate row for {:s}: {:s}".format(whatname, thing.name))
            szspecs = list()
            for word in tail:
                if word == '...':
                    szspecs.append(None)
                else:
                    try: szspecs.append(GraphSizes[word])
                    except KeyError: cr.failure("Unknown graph size: " + word)
            worktemp = list()
            if szspecs:
                if szspecs[0] is None: szspecs.insert(0, min(GraphSizes))
                if szspecs[-1] is None: szspecs.append(max(GraphSizes))
                for (i, spec) in enumerate(szspecs):
                    if szspecs[i] is None:
                        lo = szspecs[i - 1]
                        hi = szspecs[i + 1]
                        if lo > hi: cr.failure("{:s} ... {:s} is not a valid range".format(lo.name, hi.name))
                        worktemp.extend(z for z in GraphSizes if lo <= z <= hi)
                    else:
                        worktemp.append(spec)
            desired[thing] = frozenset(worktemp)
        return desired

class _ConfigByRate(_Config):

    def _do_parse(self, what, istr):
        cr = ConfigReader(istr, filename=self.configfile)
        desired = dict()
        for words in map(str.split, cr):
            (head, *tail) = words
            try:
                method = what[head]
            except KeyError:
                cr.failure("Unknown method: {:s}".format(head))
            if method in desired:
                cr.failure("Duplicate row for method {:s}".format(method.name))
            rates = list()
            for w in tail:
                try:
                    r = float(w)
                except ValueError:
                    cr.failure("Not a floating-point value: {!s}".format(W))
                if 0.0 <= r <= 1.0:
                    rates.append(r)
                else:
                    cr.failure("Transformation rates must be in the unit interval (note: {!r})".format(r))
            desired[method] = rates
        return desired

class _CfgImports(_Config):

    @property
    def _basename(self):
        return 'imports.json'

    @Override(_Config)
    def parse(self, istr):
        spec = load_xjson_file(istr)
        sources = list()
        if isinstance(spec, list):
            for item in spec:
                sources.append(get_import_source_from_json(item, filename=self.configfile))
        elif isinstance(spec, dict):
            sources.append(get_import_source_from_json(spec, filename=self.configfile))
        else:
            assert spec is None
        return sources

    @Override(_Config)
    def default(self):
        return list()

class _CfgGraphs(_Config):

    @property
    def _basename(self):
        return 'graphs.cfg'

    @Override(_Config)
    def parse(self, istr):
        """
        Illustrative Example:

            >>> text = '''
            ... # This line is a comment
            ...
            ...                    SMALL  MEDIUM
            ... LINDENMAYER           42       9
            ... QUASI3D                1       0  # zero is fine
            ... '''
            >>> with io.StringIO(text) as istr:
            ...     desired = _CfgGraphs().parse(istr)
            ...
            >>> for (g, z, n) in desired:
            ...     print('({:s}, {:s}, {:d})'.format(g.name, z.name, n))
            ...
            (LINDENMAYER, SMALL, 42)
            (LINDENMAYER, MEDIUM, 9)
            (QUASI3D, SMALL, 1)
            (QUASI3D, MEDIUM, 0)

        """
        cr = ConfigReader(istr, filename=self.configfile)
        desired = list()
        sizes = None
        for line in cr:
            words = line.split()
            if sizes is None:  # first line
                sizes = list()
                for w in words:
                    try: sizes.append(GraphSizes[w])
                    except KeyError: cr.failure("Unknown graph size: " + w)
            else:
                (head, *tail) = words
                try: gen = Generators[head]
                except KeyError: cr.failure("Unknown graph generator: " + head)
                if len(tail) != len(sizes):
                    cr.failure("Expected {:d} columns but found {:d}".format(len(sizes), len(tail)))
                for (size, word) in zip(sizes, tail):
                    try: num = None if word == '*' else int(word)
                    except ValueError: cr.failure("Not a valid integer: " + word)
                    if not (num is None or num >= 0): cr.failure("Number of graphs cannot be negative")
                    desired.append((gen, size, num))
        return desired

    @Override(_Config)
    def default(self):
        answer = list()
        answer.extend((gen, GraphSizes.SMALL,  3) for gen in Generators if not gen.imported)
        answer.extend((gen, GraphSizes.MEDIUM, 2) for gen in Generators if not gen.imported)
        answer.append((Generators.ROME, GraphSizes.SMALL, 5))
        return answer

class _CfgLayouts(_ConfigBySize):

    @property
    def _basename(self):
        return 'layouts.cfg'

    @Override(_Config)
    def parse(self, istr):
        """
        Illustrative Example:

            >>> text = '''
            ... # This line is a comment
            ...
            ... RANDOM_NORMAL    MEDIUM ...         # all sizes from MEDIUM upwards
            ... RANDOM_UNIFORM   ... MEDIUM         # all sizes up to and including MEDIUM
            ... NATIVE           ...                # all sizes
            ... FMMM             LARGE TINY         # exactly those sizes (order irrelevent)
            ... STRESS           SMALL ... LARGE    # all sizes from SMALL to LARGE (both inclusive)
            ... '''
            >>> with io.StringIO(text) as istr:
            ...     desired = _CfgLayouts().parse(istr)
            ...
            >>> for k in sorted(desired.keys()):
            ...     print('{:16s}[{:s}]'.format(k.name, ' '.join(z.name for z in sorted(desired[k]))))
            ...
            RANDOM_NORMAL   [MEDIUM LARGE HUGE]
            RANDOM_UNIFORM  [TINY SMALL MEDIUM]
            NATIVE          [TINY SMALL MEDIUM LARGE HUGE]
            FMMM            [TINY LARGE]
            STRESS          [SMALL MEDIUM LARGE]

        """
        return self._do_parse(Layouts, istr, whatname="layout")

    @Override(_Config)
    def default(self):
        answer = dict()
        answer[Layouts.NATIVE ] = frozenset(GraphSizes)
        answer[Layouts.FMMM   ] = frozenset(z for z in GraphSizes if z >= GraphSizes.MEDIUM)
        answer[Layouts.STRESS ] = frozenset(z for z in GraphSizes if z <  GraphSizes.MEDIUM)
        answer[Layouts.PHANTOM] = frozenset(GraphSizes)
        return answer

class _CfgLayInter(_ConfigByRate):

    @property
    def _basename(self):
        return 'interpolation.cfg'

    @Override(_Config)
    def parse(self, istr):
        """
        Illustrative Example:

            >>> text = '''
            ... # This line is a comment
            ... # This is an effectively empty file
            ... '''
            >>> with io.StringIO(text) as istr:
            ...     _CfgLayInter().parse(istr)
            ...
            {}

        """
        return self._do_parse(LayInter, istr)

    @Override(_Config)
    def default(self):
        return { li : [ 0.15, 0.85 ] for li in LayInter }

class _CfgLayWorse(_ConfigByRate):

    @property
    def _basename(self):
        return 'worsening.cfg'

    @Override(_Config)
    def parse(self, istr):
        """
        Illustrative Example:

            >>> text = '''
            ... # This line is a comment
            ...
            ... FLIP_NODES                           # not at all (basically the same as leaving the line out)
            ... MOVLSQ        0.123 0.34
            ... '''
            >>> with io.StringIO(text) as istr:
            ...     desired = _CfgLayWorse().parse(istr)
            ...
            >>> for k in sorted(desired.keys()):
            ...     print('{:16s}[{:s}]'.format(k.name, ' '.join(format(p, '.3f') for p in sorted(desired[k]))))
            ...
            FLIP_NODES      []
            MOVLSQ          [0.123 0.340]

        """
        return self._do_parse(LayWorse, istr)

    @Override(_Config)
    def default(self):
        return { lw : [ 0.1, 0.2, 0.5 ] for lw in LayWorse }

class _CfgProperties(_ConfigBySize):

    def __init__(self, *args, suffix=None, **kwargs):
        super().__init__(*args, **kwargs)
        self.__suffix = suffix

    @property
    def _basename(self):
        if self.__suffix is None:
            return 'properties.cfg'
        return 'properties-{:s}.cfg'.format(self.__suffix)

    @Override(_Config)
    def parse(self, istr):
        """
        Illustrative Example:

            >>> text = '''
            ... # This line is a comment
            ...
            ... RDF_GLOBAL       ...                # all sizes
            ... RDF_LOCAL                           # don't comput this property but use it if available
            ... ANGULAR          TINY MEDIUM        # precisely these
            ... '''
            >>> with io.StringIO(text) as istr:
            ...     desired = _CfgProperties().parse(istr)
            ...
            >>> for k in sorted(desired.keys()):
            ...     print('{:16s}[{:s}]'.format(k.name, ' '.join(z.name for z in sorted(desired[k]))))
            ...
            RDF_GLOBAL      [TINY SMALL MEDIUM LARGE HUGE]
            RDF_LOCAL       []
            ANGULAR         [TINY MEDIUM]

        """
        return self._do_parse(Properties, istr, whatname="property")

    @Override(_Config)
    def default(self):
        if self.__suffix == 'disc':
            return { p : frozenset(GraphSizes) for p in Properties }
        return dict()

class _CfgPuncture(_Config):

    @property
    def _basename(self) -> str:
        return 'puncture.cfg'

    def default(self) -> frozenset:
        return frozenset()

    def parse(self, istr : io.TextIOBase) -> frozenset:
        """
        Illustrative Example:

            >>> text = '''
            ... # This line is a comment
            ...
            ... #RDF_GLOBAL
            ... RDF_LOCAL  # why not?
            ... ANGULAR
            ... '''
            >>> with io.StringIO(text) as istr:
            ...     desired = _CfgProperties().parse(istr)
            ...
            >>> print(' '.join(p.name for p in sorted(desired)))
            ...
            RDF_LOCAL ANGULAR

        """
        properties = list()
        cr = ConfigReader(istr, filename=self.configfile)
        for line in cr:
            try:
                [ token ] = line.split()
            except ValueError:
                cr.failure("Only one token per line, please")
            try:
                prop = Properties[token]
            except KeyError:
                cr.failure("Unknown property: {!r}".format(token))
            properties.append(prop)
        return frozenset(properties)

class _CfgMetrics(_ConfigBySize):

    @property
    def _basename(self):
        return 'metrics.cfg'

    @Override(_Config)
    def parse(self, istr):
        """
        Illustrative Example:

            >>> text = '''
            ... STRESS_KK
            ... STRESS_FIT_SCALE   SMALL LARGE
            ... '''
            >>> with io.StringIO(text) as istr:
            ...     desired = _CfgMetrics().parse(istr)
            ...
            >>> for k in sorted(desired.keys()):
            ...     print('{:20s}[{:s}]'.format(k.name, ' '.join(z.name for z in sorted(desired[k]))))
            ...
            STRESS_KK           []
            STRESS_FIT_SCALE    [SMALL LARGE]

        """
        return self._do_parse(Metrics, istr, whatname="metric")

    @Override(_Config)
    def default(self):
        return { p : frozenset(GraphSizes) for p in Metrics }

class ConfigReader(object):

    def __init__(self, stream, filename=None):
        self.__stream = stream
        self.__filename = filename if filename is not None else '/dev/stdin'
        self.__lineno = None

    def __iter__(self):
        for (lineno, line) in enumerate(map(lambda s : s.partition('#')[0].strip(), self.__stream)):
            if not line:
                continue
            self.__lineno = 1 + lineno
            yield line

    def failure(self, reason):
        logging.error("{:s}:{:d}: {:s}".format(self.__filename, self.__lineno, reason))
        raise ConfigError("Invalid configuration file", filename=self.__filename, lineno=self.__lineno)
