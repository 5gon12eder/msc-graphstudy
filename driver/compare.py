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

__all__ = [ ]

import sys

from .cli import *
from .configuration import *
from .constants import *
from .errors import *
from .manager import *
from .model import *

PROGRAM_NAME = 'compare'

class Main(AbstractMain):

    def _run(self, ns):
        config = Configuration(ns.configdir)
        with Manager(absbindir=ns.bindir, absdatadir=ns.datadir, config=config) as mngr:
            id1 = mngr.idmatch('Layouts', ns.id1, exception=FatalError)
            id2 = mngr.idmatch('Layouts', ns.id2, exception=FatalError)
            pairs = [ (id1, id2), (id2, id1) ]
            oracle = Oracle(mngr)
            answers = oracle(pairs)
        for ((lhs, rhs), ans) in zip(pairs, answers):
            print("{!s} {!s} {:+10.5f}".format(lhs, rhs, ans))

    def _argparse_hook_before(self, ap):
        arguments = ap.add_argument_group("Arguments")
        arguments.add_argument('id1', metavar='ID1', type=str, help="ID of the 1st layout to compare")
        arguments.add_argument('id2', metavar='ID2', type=str, help="ID of the 2nd layout to compare")

if __name__ == '__main__':
    kwargs = {
        'prog' : PROGRAM_NAME,
        'usage' : "%(prog)s ID1 ID2",
        'description' : (
            "Compares the layouts given by ID1 and ID2 and prints a positive number if ID1 is determined to likely be"
            " a better layout (of the same graph!) than ID2 or a negative number if the opposite is true."
        ),
    }
    with Main(**kwargs) as app:
        app(sys.argv[1 : ])
