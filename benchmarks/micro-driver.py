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

import argparse
import os.path
import subprocess
import sys
import tempfile
import time

from lib.cli import (
    TerminalSizeHack,
    add_argument_groups,
    regretful_epilog,
    use_color,
)

from lib.fancy import (
    Reporter,
)

from lib.history import (
    History,
)

from lib.manifest import (
    InvalidManifestError,
    ManifestLoader,
)

from lib.runner import (
    BenchmarkRunner,
    CollectionRunner,
    Constraints,
    Failure,
    Result,
    TimeoutFailure,
)

def main(args):
    ap = argparse.ArgumentParser(
        prog='micro-driver',
        usage="%(prog)s -M MANIFEST -D DIR ... [OPTION ...] [--] [NAME ...]",
        description="""
        Runs micro-benchmarks.  Micro-benchmarks are small programs that execute a certain component of the library and
        measure its performance.  This script only orchestrates those programs; it doesn't do any timings or statistics
        on its own.  This is expected to be done by the executed programs which shall print their results in a suitable
        format.
        """,
        epilog=regretful_epilog,
        add_help=False,
    )
    add_argument_groups(ap)
    with TerminalSizeHack():
        ns = ap.parse_args(args)
    reporter = Reporter(use_color(ns.color), unit='us')
    if ns.alert is not None and ns.history is None:
        print("micro-driver: warning: --alert has no effect without --history", file=sys.stderr)
    constraints = Constraints(
        timeout=ns.timeout,
        repetitions=ns.repetitions,
        quantile=ns.quantile,
        significance=ns.significance,
        warmup=ns.warmup
    )
    if ns.info:
        reporter.print_info()
    try:
        loader = MicroManifestLoader()
        config = loader.load(ns.manifest)
        with History(ns.history, create=ns.update) as histo:
            mcr = MicroCollectionRunner(
                constraints=constraints,
                logger=lambda m : reporter.print_notice(m) if ns.verbose else None
            )
            return mcr.run_collection(
                selection=ns.benchmarks, config=config, histo=histo,
                update=ns.update, report=reporter, alert=ns.alert
            )
    except (AssertionError, TypeError):
        raise
    except KeyboardInterrupt:
        reporter.print_error("Canceled by keyboard interrupt")
        return 128 + 2
    except Exception as e:
        reporter.print_error(str(e))
        return 1

class MicroManifestLoader(ManifestLoader):

    def _validate_stanza(self, name, definition):
        if 'command' not in definition:
            raise InvalidManifestError(name + ": The 'command' attribute is required")
        for (key, value) in definition.items():
            if key == 'description':
                if type(value) is not str:
                    raise InvalidManifestError(name + "." + key + ": Expected a string")
            elif key == 'command':
                if type(value) is not list or not all(type(s) is str for s in value):
                    raise InvalidManifestError(name + "." + key + ": Expected an array of strings")
                if not value or not value[0].strip():
                    raise InvalidManifestError(name + "." + key + ": Command cannot be empty")
            elif type(key) is str:
                raise InvalidManifestError(name + "." + key + ": Unknown attribute")
            else:
                raise InvalidManifestError()

class MicroCollectionRunner(CollectionRunner):

    def __init__(self, constraints=None, logger=None):
        super().__init__(constraints=constraints, logger=logger)

    def _run_single(self, name, stanza):
        runner = MicroBenchmarkRunner(stanza, constraints=self.constraints, logger=self.logger)
        return runner.run()

class MicroBenchmarkRunner(BenchmarkRunner):

    def __init__(self, stanza, constraints=None, logger=None):
        super().__init__(constraints=constraints, logger=logger)
        self.__cmd = stanza['command']

    def run(self):
        try:
            self.logger("Running " + repr(self.__cmd) + " and capturing stdout")
            proc = subprocess.Popen(
                self.__cmd,
                stdin=subprocess.DEVNULL,
                stdout=subprocess.PIPE,
                stderr=subprocess.DEVNULL,
                env=self.constraints.as_environment(os.environ)
            )
            try:
                (stdout, stderr) = proc.communicate(timeout=self.constraints.timeout)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.communicate()
                raise TimeoutFailure(self.constraints.timeout)
        except OSError as e:
            raise Failure(e)
        if proc.returncode != 0:
            raise Failure("Benchmark exited with error code {:d}".format(proc.returncode))
        try:
            return Result.from_string(stdout.decode())
        except ValueError as e:
            raise Failure(e)

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
