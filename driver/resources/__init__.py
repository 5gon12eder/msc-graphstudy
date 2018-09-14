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
    'get_resource_as_stream',
]

import os.path
import warnings

try:
    from pkg_resources import resource_stream as _resource_stream
except ModuleNotFoundError:
    warnings.warn("Package 'pkg_resources' not available; loading resources from ZIP archives will not work")
    def _resource_stream(package, filename):
        if package != __package__:
            raise NotImplementedError("Cannot load resource from foreign package; please install setuptools")
        return open(os.path.join(os.path.dirname(__file__), filename), 'rb')

def get_resource_as_stream(filename):
    """
    This is (not) how it is supposed to work:

        >>> with get_resource_as_stream('__init__.py') as istr:
        ...     for line in map(lambda l : l.decode().strip(), istr):
        ...         if not line: break
        ...         print(line)
        ...
        #! /usr/bin/python3
        #! -*- coding:utf-8; mode:python; -*-

    """
    return _resource_stream(__package__, filename)
