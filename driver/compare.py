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

import collections
import contextlib
import itertools
import logging
import sys

from .alternatives import *
from .cli import *
from .configuration import *
from .constants import *
from .errors import *
from .manager import *
from .model import *
from .utility import *

PROGRAM_NAME = 'compare'

DiscoRecord = collections.namedtuple('DiscoRecord', [ 'lhs', 'rhs', 'value', 'model' ])

def process(mngr, pairs, models):
    oracle = Oracle(mngr) if any(dm in models for dm in [ Tests.NN_FORWARD, Tests.NN_REVERSE ]) else None
    for dm in models:
        answers = None
        if dm.is_alternative():
            with mngr.sql_ctx as curs:
                altctx = get_alternative_context(mngr, curs, dm)
                for (lhs, rhs) in pairs:
                    answers = [ get_alternative_value(mngr, curs, altctx, lhs, rhs) for (lhs, rhs) in pairs ]
        elif dm is Tests.NN_FORWARD:
            answers = oracle(pairs)
        elif dm is Tests.NN_REVERSE:
            answers = oracle([ (rhs, lhs) for (lhs, rhs) in pairs ])
        elif dm is Tests.EXPECTED:
            raise NotImplementedError()
        else:
            raise ValueError(dm)
        for ((lhs, rhs), ans) in zip(pairs, answers):
            yield DiscoRecord(lhs=lhs, rhs=rhs, value=ans, model=dm)

class ValueErrorInDisguise(ValueError):

    pass

class Main(AbstractMain):

    def _run(self, ns):
        config = Configuration(ns.configdir)
        with Manager(absbindir=ns.bindir, absdatadir=ns.datadir, config=config) as mngr:
            with mngr.sql_ctx as curs:
                kwargs = { 'fingerprint' : ns.fingerprint, 'exception' : ValueErrorInDisguise }
                str2id = lambda token : mngr.idmatch_curs(curs, 'Layouts', token, **kwargs)
                try:
                    pairs = get_id_pairs(ns.ids, str2id=str2id)
                except ValueErrorInDisguise as e:
                    logging.critical(str(e))
                    raise FatalError("Invalid arguments")
            with smart_open(ns.output) as ostr:
                # Eagerly create a temporary list to avoid nested SQL transactions
                records = list(process(mngr, pairs, ns.model))
                with (mngr.sql_ctx if ns.fingerprint else contextlib.nullcontext()) as curs:
                    idproj = make_id_output_projection(mngr, curs, ns.fingerprint)
                    for record in records:
                        dmname = record.model.name
                        (lhs, rhs) = (idproj(record.lhs), idproj(record.rhs))
                        print("{!s} {!s} {:+.10f} {:s}".format(lhs, rhs, record.value, dmname), file=ostr)

    def _argparse_hook_before(self, ap):
        arguments = ap.add_argument_group("Arguments")
        arguments.add_argument('ids', metavar='ID', type=str, nargs='*', help="IDs of the layouts to compare")
        options = ap.add_argument_group("Options")
        options.add_argument(
            '--model', '-m', metavar='SPEC', action='append', type=parse_dmspec,
            help="use discriminator model SPEC (can be repeated)"
        )
        options.add_argument(
            '--output', '-o', metavar='FILE',
            help="write output to FILE (default: write to standard output)"
        )
        options.add_argument(
            '--fingerprint', '-f', action='store_true',
            help="select layouts by fingerprint, not by ID"
        )

    def _argparse_hook_after(self, ns):
        if ns.model is None:
            ns.model = [ Tests.NN_FORWARD, Tests.NN_REVERSE ]

def get_id_pairs(tokens=None, str2id=Id):
    pairs = list()
    if not tokens:
        pairs.extend(read_id_pairs(sys.stdin, str2id=str2id))
    elif all(token.startswith('@') for token in tokens):
        for filename in map(lambda token : token[1:], tokens):
            try:
                with open(filename, 'r') as istr:
                    pairs.extend(read_id_pairs(istr, str2id=str2id, filename=filename))
            except OSError as e:
                logging.error("{:s}: Cannot read file: {:s}".format(filename, e.strerror))
                raise FatalError("I/O error")
            except UnicodeDecodeError:
                logging.error("{:s}: Cannot decode file: {:s}".format(filename, e.strerror))
                raise FatalError("I/O error")
    elif any(token.startswith('@') for token in tokens):
        raise FatalError("You cannot mix specifying layout IDs on the command line and reading pairs from files")
    elif len(tokens) == 1:
        [ theid ] = map(str2id, tokens)
        pairs.append((theid, theid))
    else:
        pairs.extend(itertools.combinations(set(map(str2id, tokens)), 2))
    return pairs

def read_id_pairs(istr, str2id=Id, filename='/dev/stdin'):
    for (i, words) in enumerate(map(lambda s : s.partition('#')[0].split(), istr), start=1):
        if not words:
            continue
        if len(words) != 2:
            logging.error("{:s}:{:d}: Expected exactly 2 tokens".format(filename, i))
            raise FatalError("Syntax error")
        try:
            yield (str2id(words[0]), str2id(words[1]))
        except ValueError as e:
            logging.error("{:s}:{:d}: {!s}".format(filename, i, e))
            raise FatalError("Invalid arguments")

def make_id_output_projection(mngr, curs, fingerprint=False):
    id2fp = lambda id : Id(get_one(r['fingerprint'] for r in mngr.sql_select_curs(curs, 'Layouts', id=id)))
    id2id = lambda id : id
    return id2fp if fingerprint else id2id

def parse_dmspec(text):
    try:
        return enum_from_json(Tests, text)
    except ValueError as e:
        raise argparse.ArgumentTypeError(e)

def smart_open(filename=None):
    if not filename or filename == '-':
        return contextlib.nullcontext(sys.stdout)
    return open(filename, 'w')

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
