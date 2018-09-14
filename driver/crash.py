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
    'dump_current_exception_trace',
]

import logging
import os
import sys
import tempfile
import traceback

def dump_current_exception_trace():
    (extype, exvalue, extrace) = sys.exc_info()
    (tracefd, tracefilename) = tempfile.mkstemp(prefix='driver-', suffix='.trace')
    logging.critical("Writing crash report to file {!r} ...".format(tracefilename))
    with os.fdopen(tracefd, 'w') as ostr:
        traceback.print_exc(file=ostr)
