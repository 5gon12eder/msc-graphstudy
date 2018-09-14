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
    'pickle_objects',
    'unpickle_objects',
]

import datetime
import logging
import pickle

from .errors import *

def pickle_objects(ostr, *objects, timestamp=True, error=FatalError):
    def fail(msg):
        name = getattr(istr, 'name', '/dev/stdout')
        more = " + 1" if timestamp else ""
        general = "Cannot pickle {:d}{:s} objects to file {!r}".format(len(typeinfo), more, name)
        logging.error(general + ": " + msg)
        raise error(general)
    try:
        if timestamp is True:
            pickle.dump(datetime.datetime.now(), ostr)
        for obj in objects:
            pickle.dump(obj, ostr)
        ostr.flush()
    except (OSError, pickle.PickleError) as e:
        fail(str(e))

def unpickle_objects(istr, *typeinfo, timestamp=True, error=FatalError):
    def fail(msg):
        name = getattr(istr, 'name', '/dev/stdin')
        more = " + 1" if timestamp else ""
        general = "Cannot unpickle {:d}{:s} objects from file {!r}".format(len(typeinfo), more, name)
        logging.error(general + ": " + msg)
        raise error(general)
    objects = list()
    try:
        if timestamp is True:
            t = pickle.load(istr)
            if not isinstance(t, datetime.datetime):
                fail("Expected timestamp at beginning of file not found")
            logging.debug("Unpickled time-stamp {!s}".format(t))
            objects.append(t)
        for (i, typ) in enumerate(typeinfo):
            o = pickle.load(istr)
            if typ is None:
                pass
            elif type(typ) is type:
                if not isinstance(o, typ):
                    fail("Object #{:d} has unexpected type {!r}".format(i + 1, type(o).__name__))
            else:
                pass  # TODO: Can we check generics from the typing module?
            objects.append(o)
        if istr.read(10):
            fail("There's more to the pickle")
        return objects
    except (OSError, pickle.PickleError) as e:
        fail(str(e))
