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
    'BuildError',
    'CONFIG_NAME_FILE',
    'CONFIG_TEST_FILE',
    'Error',
    'ForEachCfg',
    'Main',
    'find_build_tool',
    'looks_like_build_directory',
    'pretend',
    'report',
]

import datetime
import email.utils
import enum
import math
import os
import shlex
import shutil
import subprocess
import sys
import time

CONFIG_NAME_FILE = '.config-name.txt'
CONFIG_TEST_FILE = '.config-test.json'

PROGRAM_NAME = 'maintainer'

class Error(Exception): pass
class BuildError(Error): pass

class _BuildStatus(enum.IntEnum):

    FAILED  = -1
    SKIPPED =  0
    PASSED  = +1

def get_top_level_source_directory(directory='.'):
    if not os.path.exists(os.path.join(directory, '.msc-graphstudy')):
        report("This script must be run from within the top-level source directory")
        raise Error()
    return directory

def get_vpath_and_vardeps_root():
    vpathroot = os.getenv('MSC_VPATH_BUILD_ROOT', os.path.abspath('./build/'))
    vardepsroot = os.getenv('MSC_VAR_DEPS_ROOT', os.path.expanduser('~/var/'))
    for (envvar, envval) in [ ('MSC_VPATH_BUILD_ROOT', vpathroot), ('MSC_VAR_DEPS_ROOT', vardepsroot) ]:
        if not os.path.isabs(envval):
            report("Not an absolute path (please set {:s} accordingly): {!r}".format(envvar, envval))
            raise Error()
        if not os.path.isdir(envval):
            report("Directory does not exist (please set {:s} accordingly): {!r}".format(envvar, envval))
            raise Error()
    return (vpathroot, vardepsroot)

def find_build_tool(envvar, default):
    envval = os.getenv(envvar, default)
    if os.path.isabs(envval):
        return envval
    which = shutil.which(envval)
    return which if which is not None else envval

def looks_like_build_directory(directory, cfgname):
    assert cfgname is not None
    thename = None
    try:
        with open(os.path.join(directory, CONFIG_NAME_FILE), 'r') as istr:
            for line in filter(None, map(str.strip, istr)):
                if not line.startswith('#'):
                    thename = line
                    break
    except OSError:
        pass
    return thename == cfgname

def report(message, prefix=''):
    print('{:s}: {:s}{:s}'.format(PROGRAM_NAME, prefix, message), file=sys.stderr)

def pretend(*args, stdin=None, stdout=None, stderr=None):
    words = [ shlex.quote(s) for s in args ]
    if stdin is not None:
        words.append('<' + shlex.quote(stdin))
    if stdout is not None:
        words.append('1>' + shlex.quote(stdout))
    if stderr is not None:
        words.append('2>' + shlex.quote(stderr))
    report(' '.join(words), prefix='# ')

class PipeGuard(object):

    STDOUT_FILE = 'buildlog.out'
    STDERR_FILE = 'buildlog.err'

    def __init__(self, directory : str, quiet : bool):
        self.quiet = quiet
        self.directory = directory
        self.stdout = None
        self.stderr = None

    @property
    def stdout_name(self):
        return None if self.stdout is None else __class__.STDOUT_FILE

    @property
    def stderr_name(self):
        return None if self.stderr is None else __class__.STDERR_FILE

    def __enter__(self, *args):
        if self.quiet:
            self.stdout = open(os.path.join(self.directory, __class__.STDOUT_FILE), 'wb')
            self.stderr = open(os.path.join(self.directory, __class__.STDERR_FILE), 'wb')
            report("Redirecting standard (error) output to {!r} and {!r} respectively".format(
                __class__.STDOUT_FILE, __class__.STDERR_FILE
            ))
        else:
            for filename in [ __class__.STDOUT_FILE, __class__.STDERR_FILE ]:
                try: os.remove(os.path.join(self.directory, filename))
                except FileNotFoundError: pass
        return self

    def __exit__(self, *args):
        if self.quiet:
            self.stdout.close()
            self.stderr.close()
            self.stdout = None
            self.stderr = None

class ForEachCfg(object):

    def __init__(self, keep_going=False, quiet=False, dry=False):
        self.keep_going = bool(keep_going)
        self.quiet = bool(quiet)
        self.dry = bool(dry)
        self.srcdir = get_top_level_source_directory()
        (self.vpathroot, self.vardepsroot) = get_vpath_and_vardeps_root()

    def _perform_task(self, cfgname, builddir=None, vardepsdir=None, **kwargs):
        raise NotImplementedError()

    def __call__(self, configurations, **kwargs):
        summary = { cfg : [ _BuildStatus.SKIPPED, None ] for cfg in configurations }
        for cfgname in configurations:
            builddir = os.path.join(self.vpathroot, cfgname)
            vardepsdir = os.path.join(self.vardepsroot, cfgname)
            if not looks_like_build_directory(builddir, cfgname):
                report("The directory {!r} does not exist or is not a build directory for configuration {!r}".format(
                    builddir, cfgname))
                continue
            t0 = datetime.datetime.now()
            report("Entering directory {!r} at {:s}".format(builddir, _rfc5322(t0)))
            try:
                with PipeGuard(builddir, self.quiet) as self.__guard:
                    self._perform_task(cfgname, builddir=builddir, vardepsdir=vardepsdir, **kwargs)
                summary[cfgname][0] = _BuildStatus.PASSED
            except BuildError:
                summary[cfgname][0] = _BuildStatus.FAILED
                if not self.keep_going: break
            finally:
                t1 = datetime.datetime.now()
                summary[cfgname][1] = t1 - t0
                report("Leaving directory {!r} at {:s}".format(builddir, _rfc5322(t1)))
        if not (self.dry or _print_summary(summary)):
            raise Error()

    def _run_build_tool(self, command, ldpath=None):
        subenv = dict(os.environ)
        if ldpath is not None:
            ldpaths = [ ldpath ]
            ldpaths.extend(filter(None, map(str.strip, os.getenv('LD_LIBRARY_PATH', '').split(':'))))
            subenv['LD_LIBRARY_PATH'] = ':'.join(ldpaths)
        if self.dry:
            pretend(*command, stdout=self.__guard.stdout_name, stderr=self.__guard.stderr_name)
            return
        report("Running {!r} ...".format(command))
        t0 = time.time()
        result = subprocess.run(
            command,
            cwd=self.__guard.directory,
            env=subenv,
            stdin=subprocess.DEVNULL,
            stdout=self.__guard.stdout,
            stderr=self.__guard.stderr,
        )
        t1 = time.time()
        report("Command completed with status {:d} after {:.2f} seconds".format(result.returncode, t1 - t0))
        if result.returncode != 0:
            raise BuildError()

def _print_summary(summary, color=None):
    if color is None: color = os.name == 'posix' and sys.stderr.isatty()
    (boldon, boldoff) = ('\033[1m', '\033[22m') if color else ('', '')
    coloron = { _BuildStatus.FAILED : '\033[31m', _BuildStatus.PASSED : '\033[32m' } if color else { }
    coloroff = '\033[39m' if color else ('', '')
    prettystatus = lambda st : (coloron.get(st, ''), st.name, coloroff)
    prettytime = lambda dt : 'N/A' if dt is None else str(datetime.timedelta(seconds=round(dt.seconds)))
    tabmetafmt = '| {0}{{:24s}}{1} | {0}{{}}{{:16s}}{{}}{1} | {0}{{:16s}}{1} |'
    tabhdrfmt = tabmetafmt.format(boldon, boldoff)
    tabrowfmt = tabmetafmt.format('', '')
    tabhrule = '+' + '-' * 26 + '+' + '-' * 18 + '+' + '-' * 18 + '+'
    report(tabhrule)
    report(tabhdrfmt.format("configuration", '', "status", '', "elapsed"))
    report(tabhrule)
    if all(map(lambda p : p[0] is _BuildStatus.SKIPPED, summary.values())):
        overall = _BuildStatus.SKIPPED
    elif _BuildStatus.FAILED in map(lambda p : p[0], summary.values()):
        overall = _BuildStatus.FAILED
    elif _BuildStatus.PASSED in map(lambda p : p[0], summary.values()):
        overall = _BuildStatus.PASSED
    else:
        raise AssertionError()
    total = (
        None if all(dt is None for (st, dt) in summary.values())
        else sum((dt for (st, dt) in summary.values() if dt is not None), datetime.timedelta(0))
    )
    for cfgname in sorted(summary.keys()):
        (status, elapsed) = summary[cfgname]
        assert (status == _BuildStatus.SKIPPED) == (elapsed is None)
        report(tabrowfmt.format(cfgname, *prettystatus(status), prettytime(elapsed)))
    report(tabhrule)
    report(tabhdrfmt.format("total", *prettystatus(overall), prettytime(total)))
    report(tabhrule)
    return overall is _BuildStatus.PASSED

class Main(object):

    def __init__(self, prog):
        global PROGRAM_NAME
        PROGRAM_NAME = prog
        self.started = None
        self.finished = None
        self.status = None
        self.msg_on_success = "Maintenance work succeeded"
        self.msg_on_error = "Maintenance work failed"

    @property
    def elapsed(self):
        if self.started is not None and self.finished is not None:
            dt = self.finished - self.started
            return datetime.timedelta(days=dt.days, seconds=math.ceil(dt.seconds))

    def __enter__(self, *args):
        self.started = datetime.datetime.now()
        report("Job started at {:s}".format(_rfc5322(self.started)))

    def __exit__(self, extyp, exval, traceback):
        self.finished = datetime.datetime.now()
        if extyp is not None:
            report("Job failed at {:s}".format(_rfc5322(self.finished)))
            self.status = True
        else:
            report("Job completed at {:s}".format(_rfc5322(self.finished)))
            self.status = False
        report("Elapsed time: {!s}".format(self.elapsed))
        report(self.msg_on_error if self.status else self.msg_on_success)
        if extyp is not None and issubclass(extyp, Error):
            return True

def _rfc5322(dt : datetime.datetime):
    return email.utils.formatdate(dt.timestamp(), localtime=True)
