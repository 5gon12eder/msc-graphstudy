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

__all__ = [ 'AbstractMain' ]

import argparse
import datetime
import logging
import math
import os
import shutil
import sys

from .constants import *
from .crash import *
from .errors import *
from .tools import *
from .utility import *

LOG_LEVEL_ENVVAR = 'MSC_LOG_LEVEL'

LOG_LEVEL_DEFAULT = LogLevels.NOTICE

class AbstractMain(object):

    def __init__(self, prog=None, **kwargs):
        apkwargs = dict(kwargs)
        apkwargs['prog'] = prog
        apkwargs['add_help'] = False
        self.__prog = prog
        self.__argument_parser = argparse.ArgumentParser(**apkwargs)
        self.__name_space = None

    @property
    def prog(self):
        return self.__prog

    def __enter__(self, *args):
        logging.basicConfig(format='%(levelname)s: %(message)s', level=LOG_LEVEL_DEFAULT.value)
        logging.captureWarnings(False)
        (columns, lines) = shutil.get_terminal_size()
        os.environ['COLUMNS'] = str(columns)
        os.environ['LINES'] = str(lines)
        logging.debug("Terminal size determined to be {:d} columns and {:d} lines".format(columns, lines))
        return self

    def __exit__(self, *args):
        del os.environ['COLUMNS']
        del os.environ['LINES']

    def __call__(self, args):
        try:
            self.__parse_cli_args(args)
            self.__do_run()
        except Error as e:
            logging.error(str(e))
            raise SystemExit(True)
        except KeyboardInterrupt:
            logging.critical("Aborted by SIGINT")
            raise SystemExit(True)
        except Exception as e:
            logging.critical("An unexpected exception of type {:s} occurred".format(type(e).__name__))
            dump_current_exception_trace()
            raise SystemExit(True)

    def _run(self, ns):
        raise NotImplementedError()

    def __add_common_cli_params(self):
        paths = self.__argument_parser.add_argument_group("Paths")
        paths.add_argument('-C', '--configdir', metavar='DIR', type=os.path.abspath, default='config',
                           help="search for configuration files in DIR (default: '%(default)s')")
        paths.add_argument('-D', '--datadir', metavar='DIR', type=os.path.abspath, default='data',
                           help="root of the data directory (can be created, default: '%(default)s')")
        paths.add_argument('-B', '--bindir', metavar='DIR', type=os.path.abspath, default=os.curdir,
                           help="root of the build directory where to find executables (default: '%(default)s')")
        general = self.__argument_parser.add_argument_group("General")
        general.add_argument('-v', '--verbose', dest='_verbose', action='count', default=0,
                             help="increase the logging verbosity by one level (may be repeated and combined)")
        general.add_argument('-q', '--quiet', dest='_quiet', action='count', default=0,
                             help="decrease the logging verbosity by one level (may be repeated and combined)")
        general.add_argument(
            '--log-level', dest='_loglevel', metavar='LEVEL',
            default=_get_log_level_environment(),
            type=(lambda s : LogLevels.parse(s, errcls=argparse.ArgumentTypeError)),
            help=(
                "set the logging verbosity to one of the well-known syslog levels (by default, the value of the"
                " environment variable {envvar:s} is used which in turn defaults to {default:s})"
            ).format(envvar=LOG_LEVEL_ENVVAR, default=LOG_LEVEL_DEFAULT.name)
        )
        general.add_argument('--help', action='help',
                             help="show usage information and exit")
        general.add_argument('--version', action='version', version=self.prog,
                             help="show version information and exit")

    def __parse_cli_args(self, args):
        self._argparse_hook_before(self.__argument_parser)
        self.__add_common_cli_params()
        ns = self.__argument_parser.parse_args(args)
        loglevel = _adjust_log_level(ns._loglevel, ns._verbose - ns._quiet)
        logging.getLogger().setLevel(loglevel.value)
        #logging.getLogger().addHandler(logging.FileHandler('/tmp/driver.log'))
        del ns._loglevel
        del ns._verbose
        del ns._quiet
        ns.configdir = os.path.abspath(ns.configdir)
        ns.bindir = os.path.abspath(ns.bindir)
        ns.datadir = os.path.abspath(ns.datadir)
        self._argparse_hook_after(ns)
        self.__name_space = ns
        self.__argument_parser = None

    def _argparse_hook_before(self, ap):
        pass

    def _argparse_hook_after(self, ns):
        pass

    def __do_run(self):
        t_begin = datetime.datetime.now()
        _check_working_directory()
        find_all_tools_eagerly()
        logging.notice("Job started on {:s}".format(rfc5322(t_begin)))
        t_end = None
        try:
            self._run(self.__name_space)
        except:
            t_end = datetime.datetime.now()
            logging.notice("Job failed on {:s}".format(rfc5322(t_end)))
            raise
        else:
            t_end = datetime.datetime.now()
            logging.notice("Job completed successfully on {:s}".format(rfc5322(t_end)))
        finally:
            if t_end is not None:
                elapsed = t_end - t_begin
                elapsed = datetime.timedelta(days=elapsed.days, seconds=math.ceil(elapsed.seconds))
                logging.notice("Elapsed time: {!s}".format(elapsed))

def _check_working_directory():
    filename = os.path.join(os.getcwd(), '.msc-graphstudy')
    logging.debug("Checking for existence of file {!r} ...".format(filename))
    if not os.path.isfile(filename):
        raise SanityError("The driver script must be run from the top-level source directory")

def _get_log_level_environment():
    envval = os.environ.get(LOG_LEVEL_ENVVAR)
    if envval is None:
        return LOG_LEVEL_DEFAULT
    try:
        return LogLevels.parse(envval)
    except ValueError as e:
        logging.warning("Ignoring bogous value of environment variable {:s}: {!r}".format(LOG_LEVEL_ENVVAR, envval))
        logging.notice("Valid logging levels are: " + ', '.join(l.name for l in LogLevels))
        return LOG_LEVEL_DEFAULT

def _adjust_log_level(level, delta):
    """
    Keep verbosity:

        >>> assert _adjust_log_level(LogLevels.INFO, 0) is LogLevels.INFO

    Increase verbosity:

        >>> assert _adjust_log_level(LogLevels.WARNING, 1) is LogLevels.NOTICE
        >>> assert _adjust_log_level(LogLevels.WARNING, 2) is LogLevels.INFO
        >>> assert _adjust_log_level(LogLevels.WARNING, 3) is LogLevels.DEBUG
        >>> assert _adjust_log_level(LogLevels.WARNING, 9) is LogLevels.DEBUG
        >>> assert _adjust_log_level(LogLevels.DEBUG, 1) is LogLevels.DEBUG
        >>> assert _adjust_log_level(LogLevels.DEBUG, 1000) is LogLevels.DEBUG
        >>> assert _adjust_log_level(LogLevels.EMERGENCY, 1) is LogLevels.ALERT

    Decrease verbosity:

        >>> assert _adjust_log_level(LogLevels.WARNING, -1) is LogLevels.ERROR
        >>> assert _adjust_log_level(LogLevels.WARNING, -2) is LogLevels.CRITICAL
        >>> assert _adjust_log_level(LogLevels.WARNING, -4) is LogLevels.EMERGENCY
        >>> assert _adjust_log_level(LogLevels.WARNING, -9) is LogLevels.EMERGENCY
        >>> assert _adjust_log_level(LogLevels.EMERGENCY, -1) is LogLevels.EMERGENCY
        >>> assert _adjust_log_level(LogLevels.EMERGENCY, -1000) is LogLevels.EMERGENCY
        >>> assert _adjust_log_level(LogLevels.DEBUG, -1) is LogLevels.INFO
    """
    assert level in LogLevels
    assert isinstance(delta, int)
    values = list(LogLevels)
    if delta == 0: return level
    if delta < 0: values.reverse()
    ultimo = values[0]
    while values.pop() != level: pass
    try:
        return values[-abs(delta)]
    except IndexError:
        return ultimo

def _patch_logging_module(verbose=False):
    # We use this "self-made lambda" because using Python's built-in lambdas would capture the loop variables by-name
    # which is not what we want here.
    class Lambda(object):
        def __init__(self, level): self.__level = level
        def __call__(self, msg, *args, **kwargs): logging.log(self.__level, msg, *args, **kwargs)
    for (number, name) in map(lambda l : (l.value, l.name), LogLevels):
        if not hasattr(logging, name):
            if verbose:
                print("TRACE: Adding level {:s} ({:d}) to 'logging' module ...".format(name, number), file=sys.stderr)
            logging.addLevelName(number, name)
        else:
            # We do not add an attribute to the module here, just register a name, so this assertion must not be done if
            # we added the level ourselves.
            assert isinstance(getattr(logging, name), int)
        lowername = name.lower()
        if not hasattr(logging, lowername):
            if verbose:
                print("TRACE: Adding function '{:s}' to 'logging' module ...".format(lowername), file=sys.stderr)
            setattr(logging, lowername, Lambda(number))
        # This assertion is always done, even if we did not add the function.
        assert callable(getattr(logging, lowername))

_patch_logging_module()
