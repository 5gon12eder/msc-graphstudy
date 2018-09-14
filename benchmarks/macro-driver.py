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
import re
import statistics
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
        prog='macro-driver',
        usage="%(prog)s -M MANIFEST -D DIR ... [OPTION ...] [--] [NAME ...]",
        description=(
            """
            Runs macro-benchmarks.  Macro-benchmarks involve running an executable and measuring the total wall-time it
            takes to run.  By compiling the same input repetitively, statistical results are obtained.
            """
        ),
        epilog=regretful_epilog,
        add_help=False,
    )
    (pos, ess, sta, sup) = add_argument_groups(ap)
    with TerminalSizeHack():
        ns = ap.parse_args(args)
    if ns.alert is not None and ns.history is None:
        print("macro-driver: warning: --alert has no effect without --history", file=sys.stderr)
    reporter = Reporter(use_color(ns.color))
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
        loader = MacroManifestLoader()
        config = loader.load(ns.manifest)
        with History(ns.history, create=ns.update) as histo:
            mcr = MacroCollectionRunner(
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

class MacroManifestLoader(ManifestLoader):

    __PH_PATTERN = re.compile(r'[A-Z0-9]+')

    def _validate_stanza(self, name, definition):
        for (key, value) in definition.items():
            if key == 'description':
                if type(value) is not str:
                    raise InvalidManifestError(name + "." + key + ": Expected a string")
            elif key == 'command':
                if not _is_list_of_str(value):
                    raise InvalidManifestError(name + "." + key + ": Expected an array of strings")
            elif key == 'environment':
                if type(value) is not dict:
                    raise InvalidManifestError(name + "." + key + ": Expected an object")
                self.__validate_environment(name, value)
            elif key == 'setup':
                if type(value) is not dict:
                    raise InvalidManifestError(name + "." + key + ": Expected an object")
                self.__validate_setup(name, value)
            elif key in {'stdin', 'stdout', 'stderr'}:
                if type(value) is not str:
                    raise InvalidManifestError(name + "." + key + ": Expected a string")
            elif key == 'expect':
                if type(value) is not int:
                    raise InvalidManifestError(name + "." + key + ": Expected an integer")
            elif key == 'require':
                if type(value) is not dict:
                    raise InvalidManifestError(name + "." + key + ": Expected an object")
                self.__validate_require(name, value)
            elif type(key) is str:
                raise InvalidManifestError(name + "." + key + ": Unknown attribute")
            else:
                raise InvalidManifestError()

    def __validate_environment(self, name, environment):
        for (key, val) in environment.items():
            if not type(key) is str:
                raise InvalidManifestError()
            if not (val is  None or type(val) is str):
                raise InvalidManifestError(name + ".environment: Expected a string or null: " + key)

    def __validate_setup(self, name, setup):
        for key in setup.keys():
            if not type(key) is str:
                raise InvalidManifestError()
            if not MacroManifestLoader.__PH_PATTERN.match(key):
                raise InvalidManifestError(name + ".setup: Not a valid placeholder: " + key)
        dummymap = {key : '' for key in setup.keys()}
        for (key, value) in setup.items():
            if value is None:
                pass
            elif type(value) is str:
                self.__validate_placeholder_expansion(value, dummymap, name + ".setup." + key)
            elif type(value) is list:
                if not all(type(s) is str for s in value):
                    raise InvalidManifestError(name + ".setup." + key + ": Expected an array of strings")
                if any('{' + key + '}' in s for s in value):
                    raise InvalidManifestError(name + ".setup." + key + ": Self-reference in recipe")
                if not value or not value[0].strip():
                    raise InvalidManifestError(name + ".setup." + key + ": Not a valid command")
                for text in value:
                    self.__validate_placeholder_expansion(text, dummymap, name + ".setup." + key)
            else:
                raise InvalidManifestError(name + ".setup." + key + ": Expected 'null', a string or an array")

    def __validate_require(self, name, require):
        for (key, value) in require.items():
            if key == 'empty':
                if not _is_list_of_str(value):
                    raise InvalidManifestError(name + ".require.empty: Expected an array of strings")
            elif key == 'equal':
                if not (_is_list_of_str(value) and len(value) == 2):
                    raise InvalidManifestError(name + ".require.empty: Expected an array of two strings")
            elif type(key) is str:
                raise InvalidManifestError(name + ".require." + key + ": Unknown attribute")
            else:
                raise InvalidManifestError()

    def __validate_placeholder_expansion(self, text, dummymap, location=None):
        try:
            text.format_map(dummymap)
        except KeyError as e:
            raise InvalidManifestError(location + ": Undefined placeholder: " + str(e))
        except ValueError as e:
            raise InvalidManifestError(location + ": Error during placeholder expansion: " + str(e))

class MacroCollectionRunner(CollectionRunner):

    def _run_single(self, name, stanza):
        with tempfile.TemporaryDirectory() as tempdir:
            runner = MacroBenchmarkRunner(stanza, tempdir=tempdir, constraints=self.constraints, logger=self.logger)
            return runner.run()

class MacroBenchmarkRunner(BenchmarkRunner):

    def __init__(self, stanza, tempdir=None, constraints=None, logger=None):
        super().__init__(constraints=constraints, logger=logger)
        assert tempdir is not None
        self.__tempdir = tempdir
        self.__filemap = dict()
        self.__generators = list()
        self.__setup(stanza.get('setup', dict()))
        self.__stdin = self.__resolve_file(stanza.get('stdin'))
        self.__stdout = self.__resolve_file(stanza.get('stdout'))
        self.__stderr = self.__resolve_file(stanza.get('stderr'))
        self.__expect = stanza.get('expect', 0)
        self.__cmd = [ self.__expand_placeholders(arg) for arg in stanza['command'] ]
        self.__env = None
        if 'environment' in stanza:
            self.__env = dict(os.environ)
            for (var, val) in stanza['environment'].items():
                if val is None: del self.__env[var]
                else: self.__env[var] = self.__expand_placeholders(val)
        if 'require' in stanza:
            raise NotImplementedError(__name__)
        self.__t0 = None
        self.__min_samples = 3.0 / constraints.quantile + constraints.warmup

    def __setup(self, setup):
        togenerate = list()
        for (key, recipe) in setup.items():
            if recipe is None:
                # just a name
                path = os.path.join(self.__tempdir, key.lower())
            elif isinstance(recipe, str):
                # static source file
                path = recipe
            elif isinstance(recipe, list):
                # dynamically gnerated file
                path = os.path.join(self.__tempdir, key.lower())
                togenerate.append((recipe, path))
            else:
                raise AssertionError()
            self.__filemap[key] = path
        for (recipe, path) in togenerate:
            cmd = [ recipe[0] ]
            cmd.extend(map(self.__expand_placeholders, recipe[1:]))
            self.__generators.append(Generator(cmd, out=path, logger=self.logger))

    def __resolve_file(self, setting=None):
        if setting is None:
            return os.devnull
        else:
            return self.__expand_path_or_placeholder(setting)

    def __expand_placeholders(self, pattern):
        return pattern.format_map(self.__filemap)

    def __expand_path_or_placeholder(self, pattern):
        path = self.__expand_placeholders(pattern)
        if not os.path.isabs(path):
            pass
        return path

    def run(self):
        self.__t0 = time.time()
        self.__prepare()
        timings = list()
        while True:
            (expired, remaining) = self.__get_expired_and_remaining_timeout()
            t = None if expired else self.__run_once(timeout=remaining)
            if t is not None:
                timings.append(t)
            if len(timings) >= self.__min_samples:
                thetimings = self.__get_data(timings)
                n = len(thetimings)
                (mean, stdev) = _mean_stdev(thetimings)
                if stdev < self.constraints.significance * mean:
                    return Result(mean, stdev, n)
                elif t is None:
                    why = "Timing results did not converge within {:.2f} seconds".format(self.constraints.timeout)
                    return Result(mean, stdev, n, reason=why)
                elif self.constraints.repetitions is not None and len(timings) >= self.constraints.repetitions:
                    why = "Timing results did not converge within {:d} runs".format(self.constraints.repetitions)
                    return Result(mean, stdev, n, reason=why)
            elif t is None:
                raise TimeoutFailure(self.constraints.timeout)

    def __run_once(self, timeout=None):
        (status, t) = (None, None)
        try:
            with open(self.__stdin, 'rb') as stdin:
                with open(self.__stdout, 'wb') as stdout, open(self.__stderr, 'wb') as stderr:
                    self.logger(
                        "Running " + repr(self.__cmd) + " with " + repr({
                            'stdin' : self.__stdin,
                            'stdout' : self.__stdout,
                            'stderr' : self.__stderr,
                        }))
                    t0 = time.perf_counter()
                    proc = subprocess.Popen(
                        self.__cmd, env=self.__env, stdin=stdin, stdout=stdout, stderr=stderr
                    )
                    try:
                        proc.communicate(timeout=timeout)
                    except subprocess.TimeoutExpired:
                        proc.kill()
                        proc.communicate()
                        return None
                    t1 = time.perf_counter()
                    status = proc.returncode
                    t = t1 - t0
                    self.logger("Process exited with return code {:d} after {:.4f} seconds".format(status, t))
        except OSError as e:
            raise Failure(e)
        if status != self.__expect:
            raise Failure("Process exited with error code {:d} instead of {:d}".format(status, self.__expect))
        return t

    def __prepare(self):
        for gen in self.__generators:
            (expired, remaining) = self.__get_expired_and_remaining_timeout()
            if not expired:
                gen.run(env=self.__env, timeout=remaining)
            else:
                raise TimeoutFailure()

    def __get_data(self, values):
        data = values[self.constraints.warmup : ]
        data.sort()
        n = round(self.constraints.quantile * len(data))
        return data[ : n]

    def __get_expired_and_remaining_timeout(self):
        assert self.__t0 is not None
        if self.constraints.timeout is None:
            return (False, None)
        remaining = self.constraints.timeout - (time.time() - self.__t0)
        expired = (remaining <= 0.0)
        return (expired, remaining)

class Generator(object):

    def __init__(self, cmd, out, logger=None):
        self.__cmd = cmd
        self.__out = out
        self.__logger = logger if logger is not None else lambda x : None

    def run(self, env=None, timeout=None):
        status = None
        t0 = time.time()
        try:
            with open(self.__out, 'wb') as ostr:
                self.__logger("Running " + repr(self.__cmd) + " with " + repr({'stdout' : self.__out}))
                proc = subprocess.Popen(
                    self.__cmd,
                    env=env,
                    stdin=subprocess.DEVNULL,
                    stdout=ostr,
                    stderr=subprocess.DEVNULL
                )
                try:
                    proc.communicate(timeout=timeout)
                except subprocess.TimeoutExpired:
                    proc.kill()
                    proc.communicate()
                    raise TimeoutFailure()
                status = proc.returncode
        except OSError as e:
            raise Failure(e)
        t1 = time.time()
        self.__logger("Process exited with return code {:d} after {:.4f} seconds".format(status, t1 - t0))
        if status != 0:
            raise Failure("Generator failed with error code {:d}: ".format(status) + ' '.join(self.__cmd))

def _is_list_of_str(obj):
    return type(obj) is list and all(type(s) is str for s in obj)

def _mean_stdev(values):
    mean = statistics.mean(values)
    stdev = statistics.stdev(values, xbar=mean)
    return (mean, stdev)

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
