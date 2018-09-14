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

import argparse
import datetime
import json
import math
import os
import re
import re
import sys
import time

from email.utils import formatdate as format_rfc5322

MAGICTOKEN = "3c709b10a5d47ba33d85337dd9110917"  # MD5('progress')

class Info(object):

    FIELDS = [ 'total', 'current', 'start' ]

    def __init__(self, total : float, start : float, current : float = 0.0):
        self.total = float(total)
        self.start = float(start)
        self.current = float(current)

    def dictionary(self):
        return { key : getattr(self, key) for key in self.__class__.FIELDS }

def main():
    ap = argparse.ArgumentParser(description="Manages progress in a global file.")
    ap.add_argument('-f', '--file', metavar='FILE', type=_file, required=True, help="maintain status in FILE")
    ap.add_argument('-t', '--total', metavar='N', type=_float_nn, action='append', help="set total amount of work to N")
    ap.add_argument('-u', '--update', metavar='N', type=_float_nn, action='append', help="update progress by N")
    ap.add_argument('message', metavar='TEXT', nargs='?', help="print TEXT as message")
    ns = ap.parse_args()
    ns.total = None if not ns.total else sum(ns.total)
    ns.update = None if not ns.update else sum(ns.update)
    datafile = os.getenv('MSC_EVAL_PROGRESS_REPORT')
    now = time.time()
    info = Info(total=(ns.total if ns.total is not None else 0.0), start=now)
    if ns.total is None:
        try:
            info = load_info(ns.file)
        except FileNotFoundError:
            _warning(" ".join('--{:s}={!r}'.format(key, val) for (key, val) in ns._get_kwargs() if val is not None))
            _warning("Cannot read progress file")
            raise SystemExit()
    elapsed = now - info.start
    progress = min(1.0, max(0.0, info.current / info.total)) if info.total > 0.0 else None
    remaining = (1.0 - progress) * (elapsed / progress) if progress is not None and 0.0 < progress <= 1.0 else None
    if ns.message is not None:
        print_message(ns.message, timestamp=now, progress=progress, elapsed=elapsed, remaining=remaining)
    if ns.update is not None:
        info.current += ns.update
    store_info(info, ns.file)
    update_progress_report(
        datafile, t0=info.start, t=now, progress=progress, remaining=remaining, create=(ns.total is not None)
    )

def load_info(filename):
    with open(filename, 'r') as istr:
        for line in filter(None, map(str.strip, istr)):
            firstline = line
            break
        else:
            raise RuntimeError("{:s}: File contains no non-empty lines".format(filename))
        if MAGICTOKEN not in firstline:
            raise RuntimeError("{:s}: Magic token {!s} not found in first line".format(filename, MAGICTOKEN))
        dictionary = json.load(istr)
    if not isinstance(dictionary, dict):
        raise RuntimeError("{:s}: Expected JSON object not found".format(filename))
    if set(dictionary.keys()) != set(Info.FIELDS):
        raise RuntimeError("{:s}: Expected JSON keys not found".format(filename))
    for name in Info.FIELDS:
        value = dictionary[name]
        if type(value) is not float or not math.isfinite(value) or not value >= 0.0:
            raise RuntimeError(
                "{:s}: Unexpected value for key {!r} (expected non-negative floating-point value)"
                .format(filename, name)
            )
    return Info(**dictionary)

def store_info(info, filename):
    with open(filename, 'w') as ostr:
        ostr.write('/* ' + MAGICTOKEN + ' */' + '\n')
        json.dump(info.dictionary(), ostr)
        ostr.write('\n')

def print_message(
        message : str,
        timestamp : float,
        progress : float = None,
        elapsed : float = None,
        remaining : float = None
):
    (before, after) = ('\033[1m', '\033[22m') if os.name == 'posix' and sys.stdout.isatty() else ('', '')
    def out(text):
        print(before, text, after, sep='')
    def fmtmsg(text):
        regex = re.compile(r'[%][(]([a-zA-Z0-9_-]*)[)]')
        return regex.sub(lambda m : m.group(1).upper().replace('-', '_'), text)
    def fmttabs(timestamp):
        return format_rfc5322(timestamp, localtime=True)
    def fmttdiff(timedelta):
        return str(datetime.timedelta(seconds=round(timedelta)))
    if progress is not None:
        out("[{:3.0f} %] {:s}".format(100.0 * progress, fmtmsg(message)))
    else:
        out("[ ... ] {:s}".format(fmtmsg(message)))
    if elapsed is not None:
        out("NOW: {:s} ({:s} elapsed)".format(fmttabs(timestamp), fmttdiff(elapsed)))
    else:
        out("NOW: {:s} (unknown time elapsed)".format(fmttabs(timestamp)))
    if remaining is not None:
        out("ETA: {:s} ({:s} remaining)".format(fmttabs(timestamp + remaining), fmttdiff(remaining)))
    else:
        dummytime = ''.join('?' if c.isalnum() else c for c in fmttabs(0))
        out("ETA: {:s} (unknown time remaining)".format(dummytime))

def update_progress_report(
        filename : str, t0 : float, t : float, progress : float = None, remaining : float = None, create : bool = False
):
    if filename is None:
        return
    if not filename or filename == '-':
        _warning("Progress report can only be written to a regular file")
        return
    mode = 'w' if create else 'a'
    __ = lambda x : x if x is not None else math.nan
    with open(filename, mode) as ostr:
        if create:
            print("#! /bin/false", file=ostr)
            print("#! -*- coding:utf-8; mode:config-space; -*-", file=ostr)
            print("", file=ostr)
            print("## Job started on {!s}".format(format_rfc5322(t0, localtime=True)), file=ostr)
            print("## {:>21s}{:>24s}{:>24s}".format("elapsed / s", "progress / 1", "remaining / s"), file=ostr)
            print("", file=ostr)
        print("{:24.10E}{:24.10E}{:24.10E}".format(t - t0, __(progress), t + __(remaining) - t0), file=ostr)

def _float_nn(token):
    try:
        x = float(token)
    except ValueError:
        pass
    else:
        if math.isfinite(x) and x >= 0.0: return x
    raise argparse.ArgumentTypeError("a non-negative floating-point value is required")

def _file(token):
    if token and token != '-': return token
    raise argparse.ArgumentTypeError("a regular filename (not '-') is required")

def _warning(message):
    print("progress.py:", message, file=sys.stderr)

if __name__ == '__main__':
    main()
