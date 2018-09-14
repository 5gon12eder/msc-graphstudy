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
import binascii
import collections
import hashlib
import json
import os
import shutil
import sys
import tarfile
import tempfile
import time
import urllib.parse
import urllib.request
import zipfile

from email.utils import formatdate as format_rfc5322

PROGRAM = 'download'
ENVVAR_TRACE  = 'MSC_TRACE_DOWNLOADS'
ENVVAR_CACHE  = 'MSC_CACHE_DIR'

def main():
    ap = argparse.ArgumentParser(
        prog=PROGRAM,
        description="Downloads files and optionally extracts them from archives.",
        epilog="Items will always be extracted as regular files using the default umask.",
    )
    ap.add_argument(
        'urls', metavar='URL', nargs='+',
        help="URL of the file to download (multiple URLs specify redundant sources)",
    )
    ap.add_argument(
        '-o', '--output', metavar='FILE',
        help="save the downloaded file as FILE",
    )
    ap.add_argument(
        '-s', '--checksum', metavar='ALGO:HASH', action='append', type=parse_checksum_option,
        help="verify that downloaded file has checksum HASH using digest ALGO",
    )
    ap.add_argument(
        '-x', '--extract', metavar='SRC[:DST]', action='append', type=parse_extract_option,
        help="extract item SRC as DST from the archive",
    )
    ap.add_argument(
        '-f', '--format', metavar='FMT', choices=[ 'zip', 'tar' ], type=str.lower,
        help="format of the archive",
    )
    ap.add_argument(
        '-t', '--trace', metavar='FILE', type=argparse.FileType('a'), default=os.getenv(ENVVAR_TRACE),
        help="log downloads in FILE (overrides environment variable {:s})".format(ENVVAR_TRACE),
    )
    ap.add_argument(
        '-c', '--cache', metavar='DIR', default=os.getenv(ENVVAR_CACHE),
        help="use DIR as a download cache (overrides environment variable {:s})".format(ENVVAR_CACHE),
    )
    ns = invoke_argument_parser(ap)
    if not ns.cache:
        ns.cache = None
    elif not os.path.isdir(ns.cache):
        info("Ignoring {!s}={!r} (not an absolute path to an existing directory)".format(ENVVAR_CACHE, ns.cache))
        ns.cache = None
    elif not os.path.isabs(ns.cache):
        ns.cache = os.path.abspath(ns.cache)
    if ns.extract is not None and ns.format is None:
        ns.format = guess_archive_format(ns.urls)
        if ns.format is None:
            info("Cannot determine archive format (please specify via --format option)")
            raise SystemExit(True)
    cachefiles = None if not ns.cache or not ns.checksum else [
        os.path.join(ns.cache, 'downloads', '{:s}-{:s}.oct'.format(k, hexhash(v))) for (k, v) in ns.checksum
    ]
    if serve_from_cache(ns, cachefiles=cachefiles):
        return
    if serve_from_download(ns, cachefiles=cachefiles):
        return
    raise SystemExit(True)

def serve_from_cache(ns, cachefiles=None):
    if cachefiles is None: return False
    for filename in filter(os.path.isfile, cachefiles):
        info("Found file {!r} in cache".format(filename))
        if ns.extract is not None:
            extract_files(filename, ns.extract, format=ns.format)
        if ns.output:
            info("Saving file as {!r}".format(ns.output))
            shutil.copyfile(filename, ns.output)
        return True
    info("File not found in cache")
    return False

def serve_from_download(ns, cachefiles=None):
    checksums = [ ] if ns.checksum is None else list(ns.checksum)
    with tempfile.TemporaryDirectory(dir=os.path.dirname(os.path.abspath(ns.output)) if ns.output else None) as tempdir:
        info("Using temporary directory {!r}".format(tempdir))
        tempname = os.path.join(tempdir, 'download.oct')
        try:
            traces = list()
            for url in ns.urls:
                trace = { 'url' : url }
                traces.append(trace)
                if try_download(url, checksums=checksums, tempname=tempname, traceinfo=trace): break
            else:
                return False
        finally:
            if ns.trace is not None:
                info("Appending download trace to {!r} ...".format(ns.trace.name))
                for trace in traces:
                    print(json.dumps(trace, sort_keys=True), file=ns.trace)
        if ns.extract is not None:
            extract_files(tempname, ns.extract, format=ns.format)
        if cachefiles is not None:
            add_to_cache(tempname, cachefiles)
        if ns.output is not None:
            info("Saving temporary file as {!r}".format(ns.output))
            os.rename(tempname, ns.output)
        return True

def try_download(url, checksums, tempname, traceinfo):
    hashes = { algo : hashlib.new(algo) for (algo, chksum) in checksums }
    def update_hashers(chunk):
        for hasher in hashes.values():
            hasher.update(chunk)
    with open(tempname, 'wb') as ostr:
        info("Downloading {!r} ...".format(url))
        try:
            t0 = time.time()
            traceinfo['date'] = format_rfc5322(t0, localtime=True)
            with urllib.request.urlopen(url) as istr:
                size = copy_iostreams(istr, ostr, callback=update_hashers)
            t1 = time.time()
            info("Download complete; fetched {size:,d} bytes in {time:,.3f} seconds ({rate[0]:.3f} {rate[1]:s})"
                 .format(size=size, time=(t1 - t0), rate=get_human_rate(size, t1 - t0)))
            traceinfo['size'] = size
            traceinfo['time'] = t1 - t0
        except urllib.error.HTTPError as e:
            what = traceinfo['error'] = str(e)
            info(what)
            return False
        except OSError as e:
            info(e.strerror)
            return False
    hashes = { algo : hasher.digest() for (algo, hasher) in hashes.items() }
    for (algo, expected) in checksums:
        actual = hashes[algo]
        traceinfo['digest-' + algo] = hexhash(actual)
        if expected == actual:
            info("Verifying {:s} checksum".format(algo.upper()), "OK")
        else:
            info("Verifying {:s} checksum".format(algo.upper()), "FAILED")
            info("Expected: " + hexhash(expected))
            info("Actual:   " + hexhash(actual))
            traceinfo['error'] = "{:s} checksum does not match".format(algo.upper())
            return False
    return True

def extract_files(filename, filelist, format=None):
    if format is None:
        assert not filelist
    elif format == 'zip':
        return extract_files_zip(filename, filelist)
    elif format == 'tar':
        return extract_files_tar(filename, filelist)
    else:
        raise ValueError(format)

def extract_files_zip(filename, filelist):
    with zipfile.ZipFile(filename, 'r') as ar:
        for (src, dst) in filelist:
            info_ext(src, dst)
            try:
                item = ar.getinfo(src)
            except KeyError:
                info("Member not found in archive", src)
                raise SystemExit(True)
            with ar.open(item, 'r') as istr:
                with open(dst, 'wb') as ostr:
                    copy_iostreams(istr, ostr)

def extract_files_tar(filename, filelist):
    def wrap(iostr, name):
        if iostr is None:
            info("Cannot extract non-file member", name)
            raise SystemExit(True)
        return iostr
    with tarfile.open(filename, mode='r:*') as ar:
        for (src, dst) in filelist:
            info_ext(src, dst)
            try:
                item = ar.getmember(src)
            except KeyError:
                info("Member not found in archive", src)
                raise SystemExit(True)
            with wrap(ar.extractfile(item), name=src) as istr:
                with open(dst, 'wb') as ostr:
                    copy_iostreams(istr, ostr)

def add_to_cache(tempname, cachefiles):
    for (i, filename) in enumerate(cachefiles):
        if not os.path.isfile(filename):
            info("Adding {!r} to download cache ...".format(filename))
            os.makedirs(os.path.dirname(filename), exist_ok=True)
            if i == 0:
                 shutil.copyfile(tempname, filename)
            else:
                 os.link(cachefiles[0], filename)

def info_ext(src, dst):
    if src == dst:
        info("Extracting file {!r} ...".format(src))
    else:
        info("Extracting file {!r} into file {!r} ...".format(src, dst))

def copy_iostreams(istr, ostr, callback=None):
    """
    >>> import io
    >>> import random
    >>>
    >>> text = "The number on the gate and the number on the door and the next house over is a grocery store."
    >>> src = io.StringIO(text)
    >>> dst = io.StringIO()
    >>> copy_iostreams(src, dst) == len(text)
    True
    >>> dst.getvalue()
    'The number on the gate and the number on the door and the next house over is a grocery store.'
    >>>
    >>> data = bytes(random.randint(0x00, 0xFF) for i in range(12345))
    >>> src = io.BytesIO(data)
    >>> dst = io.BytesIO()
    >>> buffer = bytearray()
    >>> copy_iostreams(src, dst, callback=(lambda b : buffer.extend(b)))
    12345
    >>> data == dst.getvalue()
    True
    >>> data == buffer
    True
    """
    chunksize = 0x2000  # This used to be Python's internal default buffer size so it might be a good choice
    size = 0
    while True:
        chunk = istr.read(chunksize)
        size += len(chunk)
        ostr.write(chunk)
        if callback is not None:
            callback(chunk)
        if not chunk:
            return size

def invoke_argument_parser(ap, *args, **kwargs):
    envcols = os.getenv('COLUMNS')
    try:
        os.environ['COLUMNS'] = str(shutil.get_terminal_size().columns)
        return ap.parse_args(*args, **kwargs)
    finally:
        if envcols is None:
            del os.environ['COLUMNS']
        else:
            os.environ['COLUMNS'] = envcols

def parse_checksum_option(value):
    (algo, colon, chksum) = value.partition(':')
    if ':' != colon:
        raise argparse.ArgumentTypeError("Expected argument in ALGO:HASH format")
    algo = algo.strip().lower()
    chksum = chksum.strip().lower()
    if algo not in hashlib.algorithms_available:
        raise argparse.ArgumentTypeError("Unknown cryptographic checksum algorithm: {!r}".format(algo))
    try:
        chksum = bytes.fromhex(chksum)
    except ValueError:
        raise argparse.ArgumentTypeError("Invalid hex digest: {!r}".format(chksum))
    return (algo, chksum)

def parse_extract_option(value):
    if sum(1 for c in value if c == ':') <= 1:
        (src, colon, dst) = value.partition(':')
        src = src.strip()
        dst = dst.strip()
        if src and dst: return (src, dst)
        if src: return (src, src)
    raise argparse.ArgumentTypeError("Expected argument in SRC[:DST] format")

def guess_archive_format(urls):
    filepaths = list(map(lambda url : urllib.parse.unquote(urllib.parse.urlparse(url).path).lower(), urls))
    for (suffix, fmt) in [ ('.zip', 'zip'), ('.tar', 'tar') ]:
        if filepaths and all(url.endswith(suffix) for url in filepaths):
            return fmt

RateInfo = collections.namedtuple('RateInfo', [ 'rate', 'unit' ])

def get_human_rate(size, time):
    """
    >>> get_human_rate(0, 0.0)
    (0.0, '---')
    >>> get_human_rate(100, 0.0)
    (0.0, '---')
    >>> get_human_rate(0, 10.0)
    (0.0, '---')
    >>> get_human_rate(100, 1.0)
    (100.0, 'B/s')
    >>> get_human_rate(100 * 2**10, 2.0)
    (50.0, 'KiB/s')
    >>> get_human_rate(3 * 2**20, 1.5)
    (2.0, 'MiB/s')
    """
    if not (size > 0 and time > 0.0):
        return (0.0, '---')
    rate = size / time
    for unit in [ 'B/s', 'KiB/s', 'MiB/s', 'GiB/s', 'TiB/s', 'PiB/s', 'EiB/s', 'ZiB/s', 'YiB/s' ]:
        if rate < 1024: break
        rate /= 1024
    return (rate, unit)

def hexhash(hashbytes):
    return binascii.hexlify(hashbytes).decode('ascii')

def info(*args):
    print(PROGRAM, *args, sep=': ', file=sys.stderr)

if __name__ == '__main__':
    main()
