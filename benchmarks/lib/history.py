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
    'HistoricResult',
    'History',
    'ResultAggregation',
]

import os.path
import sqlite3
import time

_SCHEMA = """
CREATE TABLE IF NOT EXISTS `Benchmarks` (
    `name`         TEXT     UNIQUE PRIMARY KEY,
    `description`  TEXT
);

CREATE TABLE IF NOT EXISTS `Results` (
    `name`         TEXT     REFERENCES `Benchmarks.name` ON DELETE CASCADE,
    `mean`         REAL     NOT NULL CHECK (`mean` >= 0.0),
    `stdev`        REAL     NOT NULL CHECK (`stdev` >= 0.0),
    `n`            INTEGER  NOT NULL CHECK (`n` > 0),
    `timestamp`    INTEGER  NOT NULL
);
"""

class ResultAggregation(object):

    """
    @brief
        A simple record that holds together a benchmark description and its
        historic results.

    @member resuts : [HistoricResult]
        historic benchmark results

    @member description : str | NoneType
        optional description

    """

    def __init__(self, results=[], description=None):
        self.results = results
        self.description = description

class HistoricResult(object):

    """
    @brief
        A simple record for a historic benchmark result.

    @member mean : float
        mean execution time in seconds

    @member stdev : float
        standard deviation of execution time in seconds

    @property relative_stdev : float
        relative standard deviation or `None` if undefined

    @member n : int
        sample size

    @member timestamp : int
        POSIX time-stamp when the benchmark was executed

    """

    def __init__(self, mean, stdev, n, timestamp):
        self.mean = mean
        self.stdev = stdev
        self.n = n
        self.timestamp = timestamp

    @property
    def relative_stdev(self):
        try:
            return self.stdev / self.mean
        except ArithmeticError:
            return None

class History(object):

    """
    @brief
        Object providing the public interface for accessing the history
        database.

    Objects of this type can be used as context managers.  Actually, they can
    *only* be used as context managers as they won't initialize an SQL
    connection until their `__enter__` method is called.

    A `History` object is in either of two states, *connected* or *detached*.
    In connected state, meaningful operations are performed on the history
    database.  In detached state, all operations immediately `return` a special
    value and have no effect.  This allows using this `class` in situations
    where it is not clear whether a history database should be used or not
    until the user provides a file name without having to write numerous `if`s
    inline.

    """

    def __init__(self, filename=None, create=False):
        """
        @brief
            Initializes a `History` object.

        If the `filename` is `None`, the object will be in a detached state.
        Even if it is not, the connection to the database will not be created
        until the object is used as a context manager.

        @param filename : str | NoneType
            file name of the history database to connect to

        @param create : bool
            whether the database should be created if it does not exist

        """
        self.__filename = filename
        self.__conn = None
        self.__create = create

    def __enter__(self):
        """
        @brief
            Opens the SQL connection to the history database.

        In detached state, this function has no effect.

        """
        if self.__filename is not None:
            if self.__create or os.path.exists(self.__filename):
                self.__conn = sqlite3.connect(self.__filename)
                with self.__conn as conn:
                    conn.executescript(_SCHEMA)
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """
        @brief
            Closes the SQL connection to the history database.

        In detached state, this function has no effect.

        """
        if self.__conn is not None:
            self.__conn.close()
            self.__conn = None

    def __bool__(self):
        """
        @brief
            `return`s `True` in connected and `False` in detached state.

        @returns bool
            whether the `History` object is connected to an existing database

        """
        return self.__conn is not None

    def register(self, name, description=None):
        """
        @brief
            Adds a benchmark to the database.

        If the benchmark is already present, only the description will be
        updated.  This function has no effect in detached state.

        @param name : str
            unique name of the benchmark

        @param description : str | NoneType
            optional free-text description

        @returns bool
            `True` in connected and `False` in detached state

        """
        if self.__conn is None:
            return False
        with self.__conn as conn:
            conn.execute(
                "INSERT OR REPLACE INTO `Benchmarks` VALUES(?, ?)",
                (name, description)
            )
        return True

    def append(self, name, mean, stdev, n, timestamp=None):
        """
        @brief
            Adds a benchmark result to the database.

        The `name` of the benchmark must be `register()`ed before.  If the
        `timestamp` is `None`, the current time-stamp will be used.

        This function has no effect in detached state.

        @param name : str
            unique name of the benchmark

        @param mean : float
            mean execution time in seconds

        @param stdev : float
            standard deviation of execution time in seconds

        @param n : int
            sample size

        @param timestamp : int | NoneType
            POSIX time-stamp

        @returns bool
            `True` in connected and `False` in detached state

        """
        assert timestamp is None or type(timestamp) is int
        if self.__conn is None:
            return False
        if timestamp is None:
            timestamp = int(time.time())
        with self.__conn as conn:
            conn.execute("INSERT INTO `Results` VALUES(?, ?, ?, ?, ?)",
                         (name, mean, stdev, n, timestamp))
        return True

    def get_descriptions(self):
        """
        @brief
            `return`s the names and descriptions of all registred benchmarks.

        In detached state, this function has no effect and always `return`s
        `None`.

        @returns {str : str | NoneType} | NoneType
            map of benchmark names to descriptions

        """
        if self.__conn is None:
            return None
        with self.__conn as conn:
            curs = conn.execute("SELECT * FROM `Benchmarks`")
            return {row[0] : row[1] for row in curs}

    def get_benchmark_results(self, name):
        """
        @brief
            `return`s the full record of a single benchmark.

        In detached state, this function has no effect and always `return`s
        `None`.

        @param name : str
            name of the benchmark

        @returns ResultAggregation | NoneType
            benchmark data from the database

        @raises RuntimeError
            if a benchmark with that name does not exist in the database

        """
        if self.__conn is None:
            return None
        with self.__conn as conn:
            curs = conn.execute(
                "SELECT `description` FROM `Benchmarks` WHERE `name` = ?",
                _as_singleton(name)
            )
            try:
                description = next(curs)[0]
            except StopIteration:
                raise RuntimeError("No such benchmark")
            curs = conn.execute(
                "SELECT `mean`, `stdev`, `n`, `timestamp` FROM `Results` WHERE `name` = ?",
                _as_singleton(name)
            )
            results = [HistoricResult(r[0], r[1], r[2], r[3]) for r in curs]
            return ResultAggregation(description=description, results=results)

    def drop_benchmark(self, name):
        """
        @brief
            Deletes all records of a single benchmark from the database.

        In detached state, this function has no effect.

        @param name : str
            name of the benchmark

        @returns bool
            `True` in connected and `False` in detached state

        """
        if self.__conn is None:
            return False
        with self.__conn as conn:
            conn.execute(
                "DELETE FROM `Results` WHERE `name` = ?",
                _as_singleton(name)
            )
            conn.execute(
                "DELETE FROM `Benchmarks` WHERE `name` = ?",
                _as_singleton(name)
            )
            return True

    def drop_since(self, ts):
        """
        @brief
            Deletes all records that are not older than the given time-stamp
            from the database.

        In detached state, this function has no effect.

        @param ts : int
            POSIX time-stamp

        @returns bool
            `True` in connected and `False` in detached state

        """
        if self.__conn is None:
            return False
        with self.__conn as conn:
            conn.execute(
                "DELETE FROM `Results` WHERE `timestamp` >= ?",
                _as_singleton(ts)
            )
            return True

    def get_best(self, name):
        """
        @brief
            `return`s the historically best result for a given benchmark.

        In detached state, this function has no effect and always `return`s
        `None`.

        If the benchmark is known but has never been run yet, `(None, None)` is
        `return`ed.

        @param name : str
            name of the benchmark

        @returns (float, float) | (NoneType, NoneType) | NoneType
            (mean, stdev) tuple of best execution time in seconds

        @raises RuntimeError
            if a benchmark with that name does not exist in the database

        """
        if self.__conn is None:
            return None
        with self.__conn as conn:
            if not _count(conn.execute(
                    "SELECT count(`name`) FROM `Benchmarks` WHERE `name` = ?",
                    _as_singleton(name))):
                raise RuntimeError("No such benchmark")
            curs = conn.execute(
                "SELECT min(`mean`), `stdev` FROM `Results` WHERE `name` = ?",
                _as_singleton(name)
            )
            matches = list(curs)
            assert len(matches) <= 1
            return matches[0] if matches else (None, None)

def _as_singleton(x):
    """
    @brief
        `return`s a `tuple` that contains the given object.

    @param x
        any object

    @returns tuple
        tuple containing `x`

    """
    return (x,)

def _count(seq):
    """
    @brief
        `return`s the number of items in a sequence.

    If the sequence can only be iterated once, it is destroyed in the process.

    @param seq
        any iterable object

    @returns int
        number of elements in the sequence

    """
    return sum(1 for i in seq)
