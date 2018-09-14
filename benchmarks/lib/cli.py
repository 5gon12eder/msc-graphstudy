#! /usr/bin/python3
#! -*- coding:utf-8; mode:python; -*-

# Copyright (C) 2016 Moritz Klammler <moritz.klammler@student.kit.edu>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
# documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS
# OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

__all__ = [
    'TerminalSizeHack',
    'add_alert',
    'add_argument_groups',
    'add_benchmarks',
    'add_color',
    'add_constraints',
    'add_help',
    'add_history_mandatory',
    'add_history_optional',
    'add_info',
    'add_manifest',
    'add_verbose',
    'regretful_epilog',
    'use_color',
]

import argparse
import os
import shutil
import sys

class TerminalSizeHack(object):

    """
    @brief
        A context manager to temporairly set the `LINES` and `COLUMNS` variables in `os.environ` to the current terminal
        size.

    The `argparse` module from the standard library doesn't determine the terminal size very wisely.  Wrapping the call
    to the `parse_args` method of `argparse.ArgumentParser` inside a context guarded by this `class` will cause it to
    use the correct terminal size.

    This issue was alread reported in 2011 but the patch apparently hasn't found its way into the standard library yet.

        https://bugs.python.org/issue13041

    """

    def __init__(self):
        self.__lines = None
        self.__columns = None

    def __enter__(self):
        self.__lines = os.environ.get('LINES')
        self.__columns = os.environ.get('COLUMNS')
        size = shutil.get_terminal_size()
        os.environ['LINES'] = str(size.lines)
        os.environ['COLUMNS'] = str(size.columns)

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.__lines is None:
            del os.environ['LINES']
        else:
            os.environ['LINES'] = self.__lines
        if self.__columns is None:
            del os.environ['COLUMNS']
        else:
            os.environ['COLUMNS'] = self.__columns

regretful_epilog = """
The command-line interface of this script is optimized for generality, not for quick use.  If you plan to run it on a
regular basis, you probably want to create a small wrapper script that invokes it with the arguments that are
appropriate for your intents and purposes.
"""

def add_argument_groups(ap):
    pos = ap.add_argument_group(title="Positional Aruments", description="")
    ess = ap.add_argument_group(title="Essential Options", description="")
    sta = ap.add_argument_group(title="Statistical Options", description="")
    sup = ap.add_argument_group(title="Supplementary Options", description="")
    add_benchmarks(pos)
    add_manifest(ess)
    add_constraints(sta)
    add_history_optional(sta)
    add_alert(sta)
    add_info(sup)
    add_verbose(sup)
    add_color(sup)
    add_help(sup)
    return (pos, ess, sta, sup)

def add_benchmarks(arg):
    arg.add_argument(
        'benchmarks', metavar='NAME', nargs='*',
        help="""
        Run benchmarks for NAMEs.  If no benchmarks are selected this way, then the entire suite will be run.
        """,
    )

def add_manifest(arg):
    arg.add_argument(
        '-M', '--manifest', metavar='FILE', required=True, type=_ArgInput(),
        help="""
        Read benchmark descriptions from the Json manifest in FILE.  The special file-name '-' can be used to read from
        standard input.
        """
    )

def add_history_optional(arg):
    arg.add_argument(
        '-H', '--history', metavar='FILE', default=None, type=_ArgRealFile(existing=False),
        help="""
        Use the history database in FILE which must be a readable and writeable file.  If it does not exist, it will be
        created.  The default if this option is not given is to not use a history database.
        """
    )
    arg.add_argument(
        '-N', '--no-update', dest='update', action='store_false',
        help="""
        Only use the history database to compute the trend of the results but don't store the new results in it.
        """
    )

def add_history_mandatory(arg):
    arg.add_argument(
        '-H', '--history', metavar='FILE', required=True, type=_ArgRealFile(existing=False),
        help="""
        Use the history database in FILE which must be a readable and writeable file.  If it does not exist, it will be
        created.
        """
    )

def add_alert(arg):
    arg.add_argument(
        '-A', '--alert', metavar='FACTOR', type=_ArgNumber(float, lambda x : x >= 0.0, "non-negative real number"),
        help="""
        Alert for performance regressions greater than FACTOR sigma.  This feature is only available when a history
        database is used.  In this case, the benchmark results will be compared to the current-best result in the
        database and if a regression by more than FACTOR sigma is detected, an alert is issued and the script will
        report with an exit status indicating failure.  The sigma is computed as sqrt(s1^2 + s2^2) where s1 is the
        standard deviation of the current-best result in the database and s2 is the standard deviation of the current
        run.  Meaningful thresholds are non-negative real numbers.  Values less than 1 are allowed but probably not
        useful.
        """
    )

def add_constraints(arg):
    arg.add_argument(
        '-T', '--timeout', metavar='SECS', type=_ArgNumber(float, lambda x : x > 0.0, "positive real number"),
        help="""
        Timeout per individual benchmark in seconds.  If a benchmark does not prduce a result within SECS seconds, give
        up.  If the benchmark did produce some result until then but the standard deviation did not converge as desired
        yet, a warning is issued and the result is used nonetheless.  Otherwise, if there is no result at all, an error
        is generated.
        """
    )
    arg.add_argument(
        '-R', '--repetitions', metavar='TIMES', default=100,
        type=_ArgNumber(int, lambda x : x > 3, "integer greater than 3"),
        help="""
        Maximum number of times to repeat a single benchmark.  If the standard deviation did not converge as desired
        after TIMES repetitions, a warning is issued and the result is used nonetheless.  The default value if this
        option is not given is to give up after %(default)d repetitions.
        """
    )
    arg.add_argument(
        '-S', '--significance', metavar='RATIO', default=0.2,
        type=_ArgNumber(float, lambda x : x > 0.0, "positive real"),
        help="""
        Desired relative standard deviation of the results.  Benchmarks will be run repetitively until the relative
        standard deviation converges below RATIO unless a timeout occurs or the maximum number of repetitions is
        exceeded earlier.  If this option is not given, a value of %(default).2f is used.
        """
    )
    arg.add_argument(
        '-Q', '--quantile', metavar='FRACTION', default=1.0,
        type=_ArgNumber(float, lambda x : 0 < x <= 1, "real number 0 < x <= 1"),
        help="""
        Only use the best FRACTION of the timing results.  This option can be useful to reduce noise in the benchmark
        results if you are doing other things on the computer while the benchmarks are running.  If this option is not
        used, it defaults to, %(default).2f meaning that all results will be used.  Valid quantiles are real numbers in
        the interval (0, 1].
        """
    )
    arg.add_argument(
        '-W', '--warmup', metavar='NUMBER', default=0,
        type=_ArgNumber(int, lambda x : x >= 0, "non-negative integer"),
        help="""
        Throw away the first NUMBER of the timing results.  This option can be useful to reduce caching effects.  If
        this option is not used, it defaults to %(default)d, meaning that no results will be thrown away.
        """
    )

def add_verbose(arg):
    arg.add_argument(
        '-V', '--verbose', action='store_true',
        help="""
        Produce verbose logging output that tells what the script is doing (or failing at).  This is useful for
        debbugging but otherwise really annoying.
        """
    )

def add_color(arg):
    arg.add_argument(
        '-C', '--color', metavar='WHEN', nargs='?', type=_ArgMaybe(), default='auto', const='yes',
        help="""
        Use ANSI escape sequences to produce pretty output.  This can improve the visual experience on terminals that
        support them but is strongly annoying if your terminal doesn't or if you want to pipe the output into a file.
        The values 'yes' and 'no' unconditionally enable or disable ANSI escape sequences respectively.  The value
        'auto' will cause the script to guess whether the output device is a terminal with support for ANSI escape
        sequences and use them if so.  This is the default behavior if this option is not used.  Specifying '--color'
        without an option is equivalent to '--color=yes'.
        """
    )

def add_info(arg):
    arg.add_argument(
        '-I', '--info', action='store_true',
        help="""
        Print additional information about how to interpret the summary table of benchmarks results that will be
        printed.
        """
    )

def add_help(arg):
    arg.add_argument('-?', '--help', action='help', help="Show this help message and exit.")

def use_color(when):
    if when == 'auto':
        if os.name != 'posix':
            return False
        return all(os.isatty(f.fileno()) for f in [sys.stdout, sys.stderr])
    elif when == 'yes':
        return True
    elif when == 'no':
        return False
    else:
        raise ValueError(when)

class _ArgInput(object):

    def __init__(self):
        pass

    def __call__(self, text):
        if text != '-' and not os.path.exists(text):
            raise argparse.ArgumentTypeError("No such file: " + text)
        else:
            return text

class _ArgRealFile(object):

    def __init__(self, existing=True):
        self.__existing = existing

    def __call__(self, text):
        if text == '-':
            raise argparse.ArgumentTypeError("Cannot use standard input, sorry")
        elif self.__existing and not os.path.isfile(text):
            raise argparse.ArgumentTypeError("Not a regular file: " + text)
        else:
            return text

class _ArgNumber(object):

    def __init__(self, typ, check, description):
        self.__type = typ
        self.__check = check
        self.__description = description

    def __call__(self, text):
        try:
            value = self.__type(text)
            if self.__check(value):
                return value
        except ValueError:
            pass
        raise argparse.ArgumentTypeError("Not a " + self.__description + ": " + text)

class _ArgMaybe(object):

    def __call__(self, text):
        maybe = text.lower()
        if maybe not in {'no', 'yes', 'auto'}:
            raise argparse.ArgumentTypeError("Please say 'yes', 'no' or 'auto' but not this: " + text)
        return maybe
