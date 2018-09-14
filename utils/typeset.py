#! /usr/bin/python3
#! -*- coding:utf-8; mode:python; -*-

# Copyright (C) 2018 Moritz Klammler <moritz@klammler.eu>
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

__all__ = [ ]

import argparse
import email.utils
import fnmatch
import glob
import hashlib
import html
import logging
import math
import os
import shlex
import shutil
import subprocess
import sys
import time

from email.utils import formatdate as format_rfc5322

PROGRAM = 'typeset'

TOOLS = dict()

TOOLFLAGS = dict()

TOOLSPEC = {
    'LATEX'     : 'latex',
    'PDFLATEX'  : 'pdflatex',
    'XELATEX'   : 'xelatex',
    'LUALATEX'  : 'lualatex',
    'BIBTEX'    : 'bibtex',
    'BIBER'     : 'biber',
    'MAKEINDEX' : 'makeindex',
}

class Error(Exception): pass

def main():
    ap = argparse.ArgumentParser(
        PROGRAM,
        description="Runs a TeX tolchain in an out-of-tree build.",
        epilog="Build tools can be specified via environment variables, otherwise fallbacks are used.",
    )
    ap.add_argument('deps', metavar='FILE', nargs='+', help="main TeX file and additional dependencies")
    ap.add_argument('--srcdir', metavar='DIR', type=ap_directory, default=os.curdir, help="source directory")
    ap.add_argument('--bindir', metavar='DIR', type=ap_directory, default=os.curdir, help="build directory")
    ap.add_argument('--jobname', metavar='NAME', help="explicit LaTeX job name")
    ap.add_argument(
        '--tex', required=False,
        choices=[ 'LATEX', 'PDFLATEX', 'XELATEX', 'LUALATEX' ],
        help="TeX engine to use",
    )
    ap.add_argument(
        '--bib', required=False,
        choices=[ 'BIBTEX', 'BIBER' ],
        help="Bibliography driver to use",
    )
    ap.add_argument(
        '--idx', required=False,
        choices=[ 'MAKEINDEX' ],
        help="Index driver to use",
    )
    ap.add_argument(
        '--htmlreport', metavar='FILE', default=os.getenv('MSC_TEX_REPORT_HTML'),
        help="format log files as HTML report and save it as FILE (overrides environment variable MSC_TEX_REPORT_HTML)"
    )
    ap.add_argument(
        '--logfiles', metavar='GLOB', default=os.getenv('MSC_TEX_LOGFILES', '*.log'),
        help="globing pattern for log files (overrides environment variable MSC_TEX_LOGFILES, default: '*.log')"
    )
    ap.add_argument(
        '--source-date-epoch', metavar='PATTERN', nargs='?', const='*',
        help=(
            "set SOURCE_DATE_EPOCH to the last modification time of dependencies matching globbing expression PATTERN"
            + " (default: %(default)r)"
        )
    )
    ap.add_argument('--max-print-line', dest='columns', metavar='N', type=int, help="set line wrap for TeX's log file")
    ap.add_argument('--find-tools', action='store_true', help="exit after looking for build tools")
    ap.add_argument('--find-deps', action='store_true', help="exit after looking for dependencies")
    ap.add_argument('--prep-deps', action='store_true', help="satisfy dependencies but don't run any tools")
    ap.add_argument('-1', '--one-shot', action='store_true', help="always stop after the first TeX run")
    try:
        envcols = os.getenv('COLUMNS')
        os.environ['COLUMNS'] = str(shutil.get_terminal_size().columns)
        ns = ap.parse_args()
    finally:
        if envcols is None: del os.environ['COLUMNS']
        else: os.environ['COLUMNS'] = envcols
        del envcols
    if ns.tex is not None:
        TOOLS[ns.tex] = find_executable(ns.tex, TOOLSPEC.get(ns.tex))
        TOOLFLAGS['TEX'] = get_tool_flags('TEXFLAGS')
    if ns.bib is not None:
        TOOLS[ns.bib] = find_executable(ns.bib, TOOLSPEC.get(ns.bib))
        TOOLFLAGS['BIB'] = get_tool_flags('BIBFLAGS')
    if ns.idx is not None:
        TOOLS[ns.idx] = find_executable(ns.idx, TOOLSPEC.get(ns.idx))
        TOOLFLAGS['IDX'] = get_tool_flags('IDXFLAGS')
    if ns.find_tools:
        raise SystemExit()
    if None in TOOLS.values():
        raise Error("Cannot find one or more required build tools")
    dependencies = find_dependencies(ns.deps, srcdir=ns.srcdir, bindir=ns.bindir)
    if ns.find_deps:
        raise SystemExit()
    if None in dependencies.values():
        raise Error("Cannot find one or more dependencies")
    prepare_dependencies(dependencies, workdir=ns.bindir)
    if ns.prep_deps:
        raise SystemExit()
    try:
        run_toolchain(
            ns.deps[0],
            workdir=ns.bindir, jobname=ns.jobname, columns=ns.columns,
            tex=ns.tex, bib=ns.bib, idx=ns.idx, oneshot=ns.one_shot,
            epoch=get_last_mtime(ns.deps, pattern=ns.source_date_epoch),
        )
    finally:
        if ns.htmlreport is not None:
            save_html_report(workdir=ns.bindir, pattern=ns.logfiles, output=ns.htmlreport)

def get_last_mtime(dependencies, pattern=None):
    if pattern is None:
        return None
    mtime = lambda f : round(os.stat(f, follow_symlinks=True).st_mtime)
    return max(map(mtime, fnmatch.filter(dependencies, pattern)), default=None)

def ap_directory(text):
    if os.path.isdir(text):
        return text
    else:
        raise argparse.ArgumentTypeError("directory does not exist: {!r}".format(text))

def find_executable(envvar, default):
    executable = shutil.which(os.getenv(envvar, default))
    logging.info("Looking for build tool {:s} ...  {:s}".format(envvar, quote(executable)))
    return executable

def get_tool_flags(envvar):
    flags = os.getenv(envvar, '').split()
    logging.info("Looking for {:s} ...  {:s}".format(envvar, ' '.join(quote(arg) for arg in flags)))
    return flags

def find_dependencies(deps, srcdir=None, bindir=None):
    prefixes = list()
    for prefix in [ bindir, srcdir ]:
        if prefix is not None and prefix not in prefixes:
            prefixes.append(prefix)
    lookup = dict()
    for relative in deps:
        found = lookup[relative] = find_dependency(relative, prefixes)
        logging.debug("Looking for dependency {:s} ...  {:s}".format(relative, quote(found)))
    return lookup

def find_dependency(depend, prefixes):
    for prefix in prefixes:
        path = os.path.join(prefix, depend)
        if depend.endswith(os.path.sep):
            if os.path.isdir(path):
                return path
        else:
            if os.path.isfile(path):
                return path

def prepare_dependencies(depsmap, workdir=None):
    for (formal, actual) in depsmap.items():
        texinput = os.path.join(workdir, formal)
        if not os.path.exists(texinput):
            linkname = texinput.rstrip(os.path.sep)
            linkdir = os.path.dirname(linkname)
            relative = os.path.relpath(actual, start=linkdir)
            logging.info("Creating symbolic link {:s} --> {:s}".format(quote(formal), quote(relative)))
            os.makedirs(linkdir, exist_ok=True)
            os.symlink(relative, linkname, target_is_directory=formal.endswith(os.path.sep))

def run_toolchain(
        maindep, workdir=None, jobname=None, columns=None, tex=None, bib=None, idx=None, oneshot=False, epoch=None,
):
    if jobname is None:
        (root, ext) = os.path.splitext(maindep)
        jobname = root
    if epoch is not None:
        logging.info("SOURCE_DATE_EPOCH={!s}  # {:s}".format(epoch, format_rfc5322(epoch, localtime=True)))
    auxpattern = os.path.join(workdir, '*.aux') if workdir is not None else '*.aux'
    if tex is not None:
        tex_command = [ TOOLS[tex], *TOOLFLAGS['TEX'], '-interaction', 'batchmode', '-jobname', jobname, maindep ]
    if bib is not None:
        bib_command = [ TOOLS[bib], *TOOLFLAGS['BIB'], jobname ]
    if idx is not None:
        idx_command = [ TOOLS[idx], *TOOLFLAGS['IDX'], jobname ]
    oldchksum = None if oneshot else get_aux_checksum(auxpattern)
    if oldchksum is not None:
        logging.debug("Initial combined checksum of auxiliary files is {!s}".format(oldchksum))
    for i in range(10):
        if tex is not None:
            logging.info("Running {:s} ({:d}) ...".format(tex, i + 1))
            run_command(tex_command, workdir=workdir, what=tex, columns=columns, epoch=epoch)
        newchksum = None if oneshot else get_aux_checksum(auxpattern)
        if newchksum is not None:
            logging.debug("Combined checksum of auxiliary files is {!s}".format(newchksum))
        if i == 0 and (bib or idx):
            if bib is not None:
                logging.info("Running {:s} ...".format(bib))
                run_command(bib_command, workdir=workdir, what=bib, columns=columns, epoch=epoch,
                            stdout='{:s}-stdout.log'.format(bib.lower()),
                            stderr='{:s}-stderr.log'.format(bib.lower()),
                )
            if idx is not None:
                logging.info("Running {:s} ...".format(idx))
                run_command(idx_command, workdir=workdir, what=idx, columns=columns, epoch=epoch,
                            stdout='{:s}-stdout.log'.format(idx.lower()),
                            stderr='{:s}-stderr.log'.format(idx.lower()),
                )
        elif oldchksum == newchksum:
            break
        oldchksum = newchksum
    else:
        raise Error("Combined checksum of auxiliary files did not converge")

def run_command(command, workdir=None, columns=None, what=None, stdout=None, stderr=None, epoch=None):
    logging.debug("Running command {:s} <{:s} 1>{:s} 2>{:s}".format(
        ' '.join(map(quote, command)), quote('/dev/null'), quote(stdout or '/dev/null'), quote(stderr or '/dev/null')
    ))
    environ = dict(os.environ)
    if columns is not None:
        assert type(columns) is int
        environ['max_print_line'] = str(columns)
    if epoch is not None:
        assert type(epoch) is int and epoch >= 0
        environ['SOURCE_DATE_EPOCH'] = str(epoch)
    t0 = time.time()
    try:
        thestdout = open(stdout, 'wb') if stdout is not None else subprocess.DEVNULL
        thestderr = open(stderr, 'wb') if stderr is not None else subprocess.DEVNULL
        status = subprocess.run(
            command,
            cwd=workdir, env=environ,
            stdin=subprocess.DEVNULL, stdout=thestdout, stderr=thestderr
        )
    finally:
        if stdout is not None:
            thestdout.close()
        if stderr is not None:
            thestderr.close()
    t1 = time.time()
    elapsed = t1 - t0
    elapsed_minutes = math.floor(elapsed) // 60
    elspsed_seconds = elapsed - 60.0 * elapsed_minutes
    logging.info("Command completed with status {:d} after {:d} minutes and {:.3f} seconds".format(
        status.returncode, elapsed_minutes, elspsed_seconds))
    if status.returncode != 0:
        raise Error("Error running " + what if what is None else command[0])

def get_aux_checksum(pattern):
    hasher = hashlib.md5()
    for filename in sorted(glob.glob(pattern)):
        logging.debug("Hashing file {:s} ...".format(quote(filename)))
        hasher.update(filename.encode() + b'\0')
        with open(filename, 'rb') as istr:
            while True:
                chunk = istr.read(0x2000)
                if not chunk: break
                hasher.update(chunk)
    return hasher.hexdigest()

HTML_REPORT_CSS = """\
.listing pre {
\tfont-family: monospace;
\tfont-size: 10pt;
\tmargin: 0;
\tpadding: 0;
}

.listing pre a.line-number {
\tdisplay: inline-block;
\ttext-align: right;
\twidth: 4em;
\tmargin-right: 1em;
\tcolor: blue;
}

a.anchor {
\tcolor: white;
}

a.anchor:hover {
\tcolor: blue;
}

.alert {
\tcolor: red;
}
"""

def save_html_report(workdir, pattern, output):
    makefileid = lambda idx : 'listing-' + chr(ord('a') + idx)
    makelineid = lambda lineno, fileidx=None : 'line-{:s}-{:d}'.format(chr(ord('a') + idx), i + 1)
    basename = lambda fn : os.path.relpath(fn, start=workdir)
    logfiles = sorted(glob.glob(os.path.join(glob.escape(workdir), pattern)))
    title = "Combined TeX Log-Files ({:s})".format(email.utils.formatdate(localtime=True))
    logging.info("Writing combined log file {:s} ...".format(quote(output)))
    with open(output, 'w', encoding='utf-8') as ostr:
        print('<!DOCTYPE html>', file=ostr)
        print('', file=ostr)
        print('<html>', file=ostr)
        print('\t<head>', file=ostr)
        print('\t\t<meta charset="UTF-8" />', file=ostr)
        print('\t\t<title>{:s}</title>'.format(html.escape(title)), file=ostr)
        print('\t\t<style type="text/css">', file=ostr)
        for line in map(str.rstrip, HTML_REPORT_CSS.splitlines()):
            print('\t\t\t' + html.escape(line), file=ostr)
        print('\t\t</style>', file=ostr)
        print('\t</head>', file=ostr)
        print('\t<body>', file=ostr)
        print('\t\t<h1>{:s}</h1>'.format(html.escape(title)), file=ostr)
        print('\t\t<p>This page shows the combined contents of {:,d} files (pattern: <code>{:s}</code>).</p>'.format(
            len(logfiles), html.escape(pattern)), file=ostr)
        print('\t\t<ul>', file=ostr)
        for (i, name) in enumerate(map(basename, logfiles)):
            print('\t\t\t<li><a href="#{id:s}">{:s}</a></li>'.format(html.escape(name), id=makefileid(i)), file=ostr)
        print('\t\t</ul>', file=ostr)
        erridx = 0
        for (idx, filename) in enumerate(logfiles):
            divid = makefileid(idx)
            with open(filename, 'r') as istr:
                size = os.fstat(istr.fileno()).st_size
                subtitle = "{:s} ({:,d} bytes)".format(basename(filename), size)
                print('\t\t<h2 id="{id:s}">{:s} <a class="anchor" href="#{id:s}">&sect;</a></h2>'.format(
                    html.escape(subtitle), id=html.escape(divid)), file=ostr)
                if size == 0:
                    print('\t\t<p><em>{:s}</em></p>'.format(html.escape("This file is empty.")), file=ostr)
                    continue
                print('\t\t<div class="listing">', file=ostr)
                for (i, line) in enumerate(map(str.rstrip, istr)):
                    alert = looks_like_error_line(line)
                    preid = makelineid(i, fileidx=idx)
                    print('\t\t\t<pre id="{id:s}"><a class="line-number" href="#{id:s}">{lineno:d}</a>'.format(
                        id=preid, lineno=(i + 1)), end='', file=ostr)
                    if alert:
                        erridx += 1
                        print('<span id="{id:s}" class="code alert">{:s}</span></pre>'.format(
                            html.escape(line), id=html.escape('error-' + str(erridx))), file=ostr)
                    else:
                        print('<span class="code">{:s}</span></pre>'.format(html.escape(line)), file=ostr)
                print('\t\t</div>', file=ostr)
        print('\t</body>', file=ostr)
        print('</html>', file=ostr)

def looks_like_error_line(line):
    if line.lstrip().startswith('!'):
        return True
    words = line.split()
    if not words:
        return False
    return words[0].lower() in { 'alert', 'critical', 'error', 'fatal' }

def quote(thing):
    if not thing:
        return ""
    else:
        return shlex.quote(thing)

if __name__ == '__main__':
    logging.basicConfig(format=(PROGRAM + ': %(levelname)s: %(message)s'), level=logging.INFO)
    try:
        sys.exit(main())
    except Error as e:
        logging.critical(str(e))
        sys.exit(True)
