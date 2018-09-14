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

import argparse
import datetime
import itertools
import logging
import sys

from .cli import *
from .configuration import *
from .constants import *
from .errors import *
from .graphs import *
from .layinter import *
from .layouts import *
from .layworse import *
from .manager import *
from .metrics import *
from .model import *
from .properties import *
from .utility import *

PROGRAM_NAME = 'deploy'

def _make_badlog(ns, logfile):
    if ns.known_bad:
        return BadLog(logfile)
    else:
        return BadLog(None)

class Main(AbstractMain):

    def _run(self, ns):
        config = Configuration(ns.configdir)
        config.use_badlog = ns.known_bad
        if not is_sorted(ns.actions):
            logging.warning("Actions will be performed in the order of the respective command-line arguments")
            logging.notice("Action order as requested: " + ', '.join(a.name for a in ns.actions))
            logging.notice("Suggested natural order of actions: " + ', '.join(a.name for a in sorted(set(ns.actions))))
        for (action, constants) in itertools.groupby(sorted(ns.actions)):
            count = sum(1 for c in constants)
            if count != 1:
                logging.warning("Redundant action {:s} will be performed {:d} times".format(action.name, count))
        with Manager(ns.bindir, ns.datadir, timeout=ns.timeout, config=config) as mngr:
            with _make_badlog(ns, logfile=mngr.badlog) as badlog:
                if ns.clean == 0:
                    if not ns.actions:
                        logging.warning("Nothing will be done")
                    for action in ns.actions:
                        _perform_action(mngr, action, badlog)
                elif ns.clean == 1:
                    if not ns.actions:
                        logging.warning("Nothing will be cleaned")
                    for action in ns.actions:
                        _perform_cleaning(mngr, action)
                elif ns.clean == 2:
                    if ns.actions:
                        raise FatalError("No action must be specified in combination with {!r}".format('-cc'))
                    if ns.datadir == '.':
                        raise FatalError("Cowardly refusing to recursively delete the current working directory")
                    mngr.clean_all()
                else:
                    raise AssertionError(ns.clean)

    def _argparse_hook_before(self, ap):
        actions = ap.add_argument_group("Actions")
        actions.add_argument('-g', '--graphs', dest='actions', action='append_const', const=Actions.GRAPHS,
                             help="generate desired graphs")
        actions.add_argument('-l', '--layouts', dest='actions', action='append_const', const=Actions.LAYOUTS,
                             help="layout all graphs")
        actions.add_argument('-i', '--lay-inter', dest='actions', action='append_const', const=Actions.LAY_INTER,
                             help="compute interpolated layouts")
        actions.add_argument('-w', '--lay-worse', dest='actions', action='append_const', const=Actions.LAY_WORSE,
                             help="compute worse layouts")
        actions.add_argument('-p', '--properties', dest='actions', action='append_const', const=Actions.PROPERTIES,
                             help="compute all properties for all layouts")
        actions.add_argument('-r', '--metrics', dest='actions', action='append_const', const=Actions.METRICS,
                             help="compute all metrics for all layouts")
        actions.add_argument('-m', '--model', dest='actions', action='append_const', const=Actions.MODEL,
                             help="build, train, evaluate and save a neural network (model)")
        actions.add_argument('-a', '--all', action='store_true',
                             help="perform all actions in order (no additional explict actions are allowed)")
        operational = ap.add_argument_group("Modus Operandi")
        operational.add_argument('-c', '--clean', action='count', default=0,
                                 help="discard previous work; pass twice to discard everything")
        operational.add_argument('-t', '--timeout', metavar='X', type=parse_positive_float,
                                 help="use timeout of X seconds when executing tools (default: no limit)")
        operational.add_argument('-k', '--known-bad', action='store_true',
                                 help="Skip over computations that have already failed in the past")

    def _argparse_hook_after(self, ns):
        if ns.all:
            if ns.actions:
                raise FatalError("No explicit actions must be specified when using the {!r} option".format('--all'))
            ns.actions = list(Actions)
        if ns.actions is None:
            ns.actions = list()
        if ns.clean > 2:
            raise FatalError("Don't overdo it with the cleaning  ;-)")

def _perform_action(mngr : Manager, act : Actions, badlog : BadLog):
    logging.notice("{:s} ==> Performing action {:s} ...".format(rfc5322(datetime.datetime.now()), act.name))
    if act is None:
        return
    elif act is Actions.GRAPHS:
        do_graphs(mngr, badlog)
    elif act is Actions.LAYOUTS:
        do_layouts(mngr, badlog)
    elif act is Actions.LAY_INTER:
        do_lay_inter(mngr, badlog)
    elif act is Actions.LAY_WORSE:
        do_lay_worse(mngr, badlog)
    elif act is Actions.PROPERTIES:
        do_properties(mngr, badlog)
    elif act is Actions.METRICS:
        do_metrics(mngr, badlog)
    elif act is Actions.MODEL:
        do_model(mngr, badlog)
    else:
        raise ValueError(act)

def _perform_cleaning(mngr : Manager, act : Actions):
    logging.notice("{:s} ==> Cleaning action {:s} ...".format(rfc5322(datetime.datetime.now()), act.name))
    if act is None:
        return
    elif act is Actions.GRAPHS:
        mngr.clean_graphs()
    elif act is Actions.LAYOUTS:
        mngr.clean_layouts()
    elif act is Actions.LAY_INTER:
        mngr.clean_inter()
    elif act is Actions.LAY_WORSE:
        mngr.clean_worse()
    elif act is Actions.PROPERTIES:
        mngr.clean_properties()
    elif act is Actions.METRICS:
        mngr.clean_metrics()
    elif act is Actions.MODEL:
        mngr.clean_model()
    else:
        raise ValueError(act)

def parse_positive_float(string):
    try:
        value = float(string)
        if value > 0.0:
            return value
    except ValueError:
        pass
    raise argparse.ArgumentTypeError("Not a positive floating-point number: {!r}".format(string))

if __name__ == '__main__':
    kwargs = {
        'prog' : PROGRAM_NAME,
        'usage' : "%(prog)s [-g] [-l] [-i] [-w] [-p] [-m]",
        'description' : "Orchestrates the big picture (TM).",
    }
    with Main(**kwargs) as app:
        app(sys.argv[1 : ])
