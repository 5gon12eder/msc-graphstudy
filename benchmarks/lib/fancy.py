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
    'Ansi',
    'NoAnsi',
    'Reporter',
]

import shutil
import sys
import textwrap

class Ansi(object):

    BOLD = '\033[1m'
    NOBOLD = '\033[22m'
    RED  = '\033[31m'
    GREEN  = '\033[32m'
    YELLOW = '\033[33m'
    BLUE = '\033[34m'
    MAGENTA = '\033[35m'
    CYAN = '\033[36m'
    NOCOLOR = '\033[39m'
    REVERSE = '\033[7m'
    NOREVERSE = '\033[27m'
    OFF = '\033[m'

class NoAnsi(object):

    BOLD = ''
    NOBOLD = ''
    RED  = ''
    GREEN  = ''
    YELLOW = ''
    BLUE = ''
    MAGENTA = ''
    CYAN = ''
    NOCOLOR = ''
    REVERSE = ''
    NOREVERSE = ''
    OFF = ''

class Reporter(object):

    def __init__(self, color, unit='s'):
        self.__ansi = Ansi if color else NoAnsi
        self.__width = max(79, shutil.get_terminal_size().columns)
        self.__separator = '-' * self.__width
        self.__unit = unit
        self.__unit_factor = {
             's' : 1.0,
            'ms' : 1.0e-3,
            'us' : 1.0e-6,
            'ns' : 1.0e-9,
        }[unit]

    def print_header(self):
        print(self.__separator)
        print('{} {:<20s}{:>12s}{:>12s}{:>8s}{:>12s}   {:s}{}'.format(
            self.__ansi.BOLD,
            "id", "mean / " + self.__unit, "stdev / " + self.__unit, "N",
            "trend", "description",
            self.__ansi.NOBOLD
        ))
        print(self.__separator)

    def print_footer(self):
        print(self.__separator)

    def print_prolog(self, text):
        print(text)

    def print_epilog(self, text):
        print(text)

    def print_row(self, name, result, trend=None, alerted=None, description=None):
        short_name = self.__shorten_name(name)
        short_description = self.__shorten_description(description)
        umean = result.mean / self.__unit_factor
        ustdev = result.stdev / self.__unit_factor
        if trend is None:
            print(' {:20s}{:>12s}{:>12s}{:8d}{:>12s}   {:s}'.format(
                short_name, _fmttime(umean), _fmttime(ustdev), result.n, 'n/a',
                short_description
            ))
        else:
            if alerted or alerted is None and trend >= 1.0:
                trendon = self.__ansi.RED
            elif trend <= -1.0:
                trendon = self.__ansi.GREEN
            else:
                trendon = self.__ansi.NOCOLOR
            trendoff = self.__ansi.NOCOLOR
            print(' {:20s}{:>12s}{:>12s}{:8d}{}{:+12.2f}{}   {:s}'.format(
                short_name, _fmttime(umean), _fmttime(ustdev), result.n,
                trendon, trend, trendoff, short_description
            ))

    def print_error(self, message, name=None):
        ansion = self.__ansi.BOLD + self.__ansi.RED
        ansioff = self.__ansi.NOBOLD + self.__ansi.NOCOLOR
        self.__print_log(message, level='error', name=name, ansion=ansion, ansioff=ansioff)

    def print_warning(self, message, name=None):
        ansion = self.__ansi.BOLD + self.__ansi.YELLOW
        ansioff = self.__ansi.NOBOLD + self.__ansi.NOCOLOR
        self.__print_log(message, level='warning', name=name, ansion=ansion, ansioff=ansioff)

    def print_notice(self, message, name=None):
        ansion = self.__ansi.BLUE
        ansioff = self.__ansi.NOCOLOR
        self.__print_log(message, level='notice', name=name, ansion=ansion, ansioff=ansioff)

    def __print_log(self, message, level=None, name=None, ansion=None, ansioff=None):
        prefix = ''
        if level is not None:
            prefix += level + ': '
        if name is not None:
            prefix += name + ': '
        print(ansion, prefix, message, ansioff, file=sys.stderr, sep='')

    def print_info(self):
        tw = textwrap.TextWrapper(width=self.__width)
        print("")
        print(tw.fill(
            "The following table summarizes the results of running the"
            + " benchmark suite.  It has one row per benchmark and the"
            + " following columns."
        ))
        print("")
        self.__print_column_info(
            'id',
            "Name of the benchmark as defined in the manifest file.  If the"
            + " name is too long to display, it will be truncated and '...'"
            + " appended.")
        print("")
        self.__print_column_info(
            'mean',
            "Result in of running the benchmark.  This is the average over"
            + " several runs.  The unit of this column is given in its header"
            + " and is either seconds or a fraction of seconds (such as"
            + " milliseconds) depending on what is more appropriate."
        )
        print("")
        self.__print_column_info(
            'stdev',
            "Absolute standard deviation of the benchmark result"
        )
        print("")
        self.__print_column_info(
            'N',
            "Number of times the benchmark was run.  If you select to use only"
            + " parts of the data by setting a non-zero warmup or a quantile"
            + " less than one, the samples discarded dues to these constraints"
            + " will already be excluded from the number shown."
        )
        print("")
        self.__print_column_info(
            'trend',
            "Performance compared to the historically best result expressed in"
            + " terms of sigmas.  If the result of this run was m +/- s and the"
            + " historically best result was M +/- S then the 'trend' column"
            + " will show the quantity (m - M) / sqrt(s^2 + S^2)."
        )
        print("")
        self.__print_column_info(
            'description',
            "Description of the benchmark as provided in the manifest file. "
            + " If the text is too long to display, it will be truncated and"
            + " '...' appended."
        )
        print("")

    def __print_column_info(self, title, explanation):
        indentfmt = self.__ansi.BOLD + '  {:<20s}  ' + self.__ansi.NOBOLD
        tw = textwrap.TextWrapper(
            width=self.__width,
            initial_indent=indentfmt.format(title),
            subsequent_indent=indentfmt.format('')
        )
        for line in tw.wrap(explanation):
            print(line)

    def __shorten_name(self, text=None):
        return _shorten(text, 20)

    def __shorten_description(self, text=None):
        return _shorten(text, self.__width - 68)

def _shorten(text, limit):
    assert limit > 5
    if text is None:
        return ''
    if len(text) <= limit:
        return text
    else:
        return text[: limit - 5].rstrip() + ' ...'

def _fmttime(secs, limit=0.001, fmt='.3f'):
    fmt = '{:' + fmt + '}'
    if secs < limit:
        return '< ' + fmt.format(limit)
    return fmt.format(secs)
