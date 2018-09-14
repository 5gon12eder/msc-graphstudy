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
    'BZ2_FILTER',
    'GNUPLOT',
    'GZIP_FILTER',
    'IMAGE_MAGICK',
    'find_all_tools_eagerly',
    'find_tool_lazily',
    'get_cache_directory',
    'get_gnuplot_environment',
    'get_gnuplot_options',
]

import logging
import os
import random
import shlex
import shutil
import subprocess
import tempfile

from .errors import *
from .utility import *

GNUPLOT      = object()
IMAGE_MAGICK = object()
GZIP_FILTER  = object()
BZ2_FILTER   = object()

_TOOL_CACHE = dict()

_TOOL_SPECS = {
    GNUPLOT      : ('GNUPLOT',      'gnuplot'  ),
    IMAGE_MAGICK : ('IMAGE_MAGICK', 'convert'  ),
    GZIP_FILTER  : ('ZCAT',         'gzip -dc' ),
    BZ2_FILTER   : ('BZCAT',        'bzip2 -dc'),
}

_GNUPLOT_OPTIONS = None

def find_tool_lazily(tool, split_tokens=False):
    wrap = (lambda s : shlex.split(s)) if split_tokens else (lambda s : s)
    try:
        return wrap(_TOOL_CACHE[tool])
    except KeyError:
        pass
    (envvar, default) = _TOOL_SPECS[tool]
    result = _TOOL_CACHE[tool] = _find_tool(envvar, default)
    return wrap(result)

def find_all_tools_eagerly():
    _get_cache_directory()
    for (key, val) in _TOOL_SPECS.items():
        _TOOL_CACHE[key] = _find_tool(*val)

def _find_tool(envvar, default):
    for prefix in [ 'MSC_', '' ]:
        envval = os.getenv(prefix + envvar)
        if envval is not None:
            command = _get_safe_command(envval, abspath=True, what=envvar)
            logging.info("Using {:s}={!r} (environment)".format(envvar, command))
            return command
    (prog, *flags) = shlex.split(default)
    prog = shutil.which(prog)
    if not prog:
        logging.warning("Cannot find tool {:s} (falling back to {!r})".format(envvar, default))
        return _get_safe_command(default, what=envvar)
    else:
        command = _get_safe_command(default, what=envvar, tokens=[ prog, *flags ])
        logging.info("Using {:s}={!r} (default)".format(envvar, command))
        return command

def _get_safe_command(string, abspath=False, what='???', tokens=None):
    assert isinstance(string, str)
    if tokens is None: tokens = shlex.split(string)
    if not tokens:
        raise SanityError("Command is empty: {!s}={!r}".format(what, string))
    if abspath and not os.path.isabs(tokens[0]):
        raise SanityError("First token is not an absolute path: {!s}={!r}".format(what, string))
    escaped = ' '.join(map(shlex.quote, tokens))
    if escaped != ' '.join(tokens):
        raise SanityError("Cowardly refusing to use an unsafe command {!s}={!r}".format(what, string))
    return escaped

_CACHE_DIRECTORY = list()  # poor-man's optional type that can hold None as a value

def get_cache_directory(fallback : str = None):
    if not _CACHE_DIRECTORY:
        _CACHE_DIRECTORY.append(_get_cache_directory())
    directory = get_one(_CACHE_DIRECTORY)
    return directory if directory is not None else fallback

def _get_cache_directory():
    envvar = 'MSC_CACHE_DIR'
    directory = os.getenv(envvar)
    if not directory:
        return None
    if not os.path.isabs(directory) or not os.path.isdir(directory):
        raise ConfigError("Not an absolute path of an existing directory: {!s}={!r}".format(envvar, directory))
    if not os.path.exists(os.path.join(directory, 'CACHEDIR.TAG')):
        logging.warning("Cache directory {!r} contains no {:s} file (see {:s} for an explanation)".format(
            directory, 'CACHEDIR.TAG', 'http://www.brynosaurus.com/cachedir/'))
    logging.info("Using cache directory {:s}={!r}".format(envvar, directory))
    return directory

def get_gnuplot_options():
    global _GNUPLOT_OPTIONS
    if _GNUPLOT_OPTIONS is None:
        command = find_tool_lazily(GNUPLOT).split()
        options = list()
        escaped = ' '.join(shlex.quote(w) for w in command)
        timeout = 10.0
        cookie = random.randint(1, 1000)
        testscript = (
            "set print '-'\n"
            "print sprintf({!r}, {!r})\n"
            "exit\n"
        ).format("The answer is %d!", cookie)
        expected = "The answer is {:d}!".format(cookie)
        kwargs = {
            'stdout'  : subprocess.PIPE,
            'stderr'  : subprocess.PIPE,
            'cwd'     : tempfile.gettempdir(),
            'env'     : get_gnuplot_environment(),
            'input'   : testscript.encode('ascii'),
            'timeout' : timeout,
        }
        for spellings in [ [ '--default-settings', '--default' ] ]:
            for opt in spellings:
                cmd = command + [ opt ]
                try:
                    result = subprocess.run(cmd, **kwargs)
                except subprocess.TimeoutExpired:
                    (ok, why) = (False, "timeout expired after {:.3f} seconds".format(timeout))
                else:
                    if result.returncode != 0:
                        (ok, why) = (False, "quit with non-zero return code {:d}".format(result.returncode))
                    elif result.stderr:
                        (ok, why) = (False, "standard error output was {:,d} bytes long".format(len(result.stderr)))
                    elif result.stdout.decode('ascii', errors='replace').strip() != expected:
                        (ok, why) = (False, "expected standard output not found")
                    else:
                        (ok, why) = (True, None)
                ans = "yes" if ok else "no ({:s})".format(why)
                logging.info("Checking whether {!r} accepts the {!r} option ...  {:s}".format(escaped, opt, ans))
                if ok:
                    options.append(opt)
                    break
        logging.info("Using the following Gnuplot options: {!r}".format(options))
        _GNUPLOT_OPTIONS = options
    return _GNUPLOT_OPTIONS

def get_gnuplot_environment():
    """
    Get an environment that makes Gnuplot behave in a more foreseeable manner.
    It is still important to supply the `--default` option.
    """
    env = dict(os.environ)
    for envvar in [ 'GNUTERM', 'GNUHELP', 'FIT_SCRIPT', 'FIT_LOG', 'GNUPLOT_LIB' ]:
        env[envvar] = ''  # Make sure we have something to delete ...
        del env[envvar]   # ... and then delete it!
    env['FIT_LOG'] = os.devnull
    return env
