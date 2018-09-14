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

all = [
    'BenchmarkRunner',
    'CollectionRunner',
    'Failure',
    'Result',
    'TimeoutFailure',
]

import datetime
import math
import os.path
import time

class Result(object):

    def __init__(self, mean : float, stdev : float, n : int, reason : str = None):
        self.mean = mean
        self.stdev = stdev
        self.n = n
        self.reason = reason

    @classmethod
    def from_string(cls, text):
        try:
            (w0, w1, w2) = text.split()
            mean = float(w0)
            stdev = float(w1)
            n = int(w2)
        except ValueError:
            raise ValueError("Benchmark result string not in format '%g %g %d'")
        return cls(mean, stdev, n)

class CollectionRunner(object):

    def __init__(self, constraints=None, logger=None):
        assert constraints is not None
        self.constraints = constraints
        self.logger = logger if logger is not None else lambda x : None

    def run_collection(self, config, histo, report, selection=None, update=False, alert=None, constraints=None):
        assert None not in [config, histo, report]
        if not selection:
            selection = sorted(config.keys())
        successes = set()
        failures = set()
        alerts = set()
        report.print_prolog("Running suite of {:d} benchmarks ...".format(len(selection)))
        if len(selection) > len(set(selection)):
            report.print_warning("List of benchmarks to run contains duplicates")
        t0 = time.time()
        report.print_header()
        for key in selection:
            try:
                bench = config[key]
            except KeyError:
                report.print_error("No definition for this benchmark in the manifest file", name=key)
                failures.add(key)
                continue
            description = bench.get('description')
            try:
                res = self._run_single(key, bench)
                successes.add(key)
            except Failure as e:
                failures.add(key)
                report.print_error(str(e), name=key)
                continue
            baseline = histo.get_best(key)
            if update:
                histo.register(key, description)
                histo.append(key, res.mean, res.stdev, res.n)
            trend = _get_trend((res.mean, res.stdev), baseline)
            alerted = None if None in [alert, trend] else (trend >= alert)
            if alerted:
                alerts.add(key)
            report.print_row(key, res, trend=trend, alerted=alerted, description=description)
            if res.reason is not None:
                report.print_warning(res.reason, name=key)
        report.print_footer()
        t1 = time.time()
        elapsed = datetime.timedelta(seconds=math.ceil(t1 - t0 + 1.0))
        report.print_epilog("Completed run of benchmark suite in " + str(elapsed))
        report.print_epilog("")
        report.print_epilog("{:6d} successful completions".format(len(successes)))
        report.print_epilog("{:6d} hard failures".format(len(failures)))
        if histo is not None and alert is not None:
            report.print_epilog("{:6d} regression alerts (threshold was {:.2f} sigma)".format(len(alerts), alert))
        report.print_epilog("")
        return len(failures | alerts)

    def _run_single(self, name : str, stanza : dict) -> Result:
        raise NotImplementedError(__name__)

class BenchmarkRunner(object):

    def __init__(self, constraints=None, logger=None):
        self.constraints = constraints
        self.logger = logger if logger is not None else lambda x : None

    def run(self) -> Result:
        raise NotImplementedError(__name__)

class Failure(Exception):

    pass

class TimeoutFailure(Failure):

    def __init__(self, timeout=None):
        super().__init__(
            "Timeout ({:.2f} s) expired before a result could be obtained".format(timeout)
            if timeout is not None else "Timeout expired before a result could be obtained"
        )

class Constraints(object):

    def __init__(self,
                 timeout      : float = None,
                 repetitions  : int   = None,
                 quantile     : float = None,
                 significance : float = None,
                 warmup       : int   = None):
        assert timeout is None or timeout > 0.0
        self.__timeout = timeout
        assert repetitions is None or repetitions > 3
        self.__repetitions = repetitions
        assert 0.0 < quantile <= 1.0
        self.__quantile = quantile
        assert significance > 0.0
        self.__significance = significance
        assert warmup >= 0
        self.__warmup = warmup

    @property
    def timeout(self):
        return self.__timeout

    @property
    def repetitions(self):
        return self.__repetitions

    @property
    def quantile(self):
        return self.__quantile

    @property
    def significance(self):
        return self.__significance

    @property
    def warmup(self):
        return self.__warmup

    def as_environment(self, env : dict = None, hexfloat : bool = False) -> dict:
        ftos = float.hex if hexfloat else str
        if env is None:
            env = dict()
        if self.__timeout is not None:
            env['BENCHMARK_TIMEOUT'] = ftos(self.__timeout)
        if self.__repetitions is not None:
            env['BENCHMARK_REPETITIONS'] = str(self.__repetitions)
        env['BENCHMARK_QUANTILE'] = ftos(self.__quantile)
        env['BENCHMARK_SIGNIFICANCE'] = ftos(self.__significance)
        env['BENCHMARK_WARMUP'] = str(self.__warmup)
        return env

def _get_trend(current, best=None):
    """
    @brief
        Computes the trend of a benchmark compared to its historical best result and determines whether this is
        alerting.

    If the historic best result is unknown (`best` is `None` or a tuple containg `None`s), then nothing can be computed,
    and `(None, False)` is `return`ed immediately.

    Otherwise, the difference of the current and the historic best result is computed along with the standard deviation
    of this difference.  Finally, the quotient of the different and its standard deviation is `return`ed.

    If any of the computations fails (for example, because the standard deviation is zero), `None` is `return`ed.

    @param current : (float, float)
        mean and standard deviation of the current result

    @param best : (float, float) | (NoneType, NoneType) | NoneType
        mean and standard deviation of the historical best result or `None` if unknown

    @returns float | NoneType
        the relative trend or `None`if it cannot be computed

    """
    if best is None or None in best:
        return None
    try:
        diff = current[0] - best[0]
        sigma = math.sqrt(current[1]**2 + best[1]**2)
        return diff / sigma
    except ArithmeticError:
        return None
