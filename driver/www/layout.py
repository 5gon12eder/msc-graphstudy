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

__all__ = [ 'serve' ]

import collections
import enum
import http
import logging
import math
import os
import secrets
import urllib.parse

from . import *
from ..constants import *
from ..errors import *
from ..tools import *
from ..utility import *
from .impl_common import *

_THUMBNAIL_SIZE = 200

_INTER_INFO_KEYS = [ 'id', 'method', 'parent1st', 'parent2nd', 'rate' ]
_WORSE_INFO_KEYS = [ 'id', 'method', 'parent', 'rate' ]

_InterInfo = collections.namedtuple('InterInfo', _INTER_INFO_KEYS)
_WorseInfo = collections.namedtuple('WorseInfo', _WORSE_INFO_KEYS)

def _serve_for_graph(this, graphid : Id, layout : Layouts, url=None):
    assert graphid is not None and layout is not None
    logging.warning("The URL scheme '/graphs/GRAPH-ID/LAYOUT/' is deprecated; refer to layouts by layout ID instead.")
    for row in this.server.graphstudy_manager.sql_select('Layouts', graph=graphid, layout=layout):
        return this.serve_redirect('/layouts/' + str(row['id']) + '/', url)
    raise HttpError(
        http.HTTPStatus.NOT_FOUND,
        "No {:s} layout for a graph with {!s} exists.".format(layout.name, graphid)
    )

def _serve_by_id(this, layoutid : Id, url=None):
    assert layoutid is not None
    with this.server.graphstudy_manager.sql_ctx as curs:
        layoutrow = get_one_or(this.server.graphstudy_manager.sql_select_curs(curs, 'Layouts', id=layoutid))
        if not layoutrow:
            raise HttpError(http.HTTPStatus.NOT_FOUND, "No layout with ID {!s} exists.".format(layoutid))
        graphid = layoutrow['graph']
        graphrow = get_one_or(this.server.graphstudy_manager.sql_select_curs(curs, 'Graphs', id=graphid))
        all_layouts = {
            r['layout'] : r['id']
            for r in this.server.graphstudy_manager.sql_select_curs(curs, 'Layouts', graph=graphid, layout=object)
        }
        inter_layouts = list()
        for row in this.server.graphstudy_manager.sql_select_curs(curs, 'InterLayouts', parent1st=layoutid):
            inter_layouts.append(_InterInfo(**{ k : row[k] for k in _INTER_INFO_KEYS }))
        worse_layouts = list()
        for row in this.server.graphstudy_manager.sql_select_curs(curs, 'WorseLayouts', parent=layoutid):
            worse_layouts.append(_WorseInfo(**{ k : row[k] for k in _WORSE_INFO_KEYS }))
        inter = None if layoutid in all_layouts else _get_inter_info(this.server.graphstudy_manager, curs, layoutid)
        worse = None if layoutid in all_layouts else _get_worse_info(this.server.graphstudy_manager, curs, layoutid)
    return _serve_generic(
        this, graphid, layoutid, graphrow,
        all_layouts=all_layouts, inter_layouts=inter_layouts, worse_layouts=worse_layouts,
        inter=inter, worse=worse,
    )

def _serve_generic(this, graphid, layoutid, graphrow, all_layouts, inter_layouts, worse_layouts, inter, worse):
    root = ET.Element('root')
    append_child(root, 'id').text = str(layoutid)
    append_common_all_constants(root, Layouts, LayInter, LayWorse, Properties, Binnings)
    append_common_graph(root, graphrow)
    if inter is not None:
        with Child(root, 'interpolation', method=inter.method.name, rate=fmtnum(inter.rate)) as node:
            append_child(node, 'parent').text = str(inter.parent1st)
            append_child(node, 'parent').text = str(inter.parent2nd)
    if worse is not None:
        with Child(root, 'worsening', method=worse.method.name, rate=fmtnum(worse.rate)) as node:
            append_child(node, 'parent').text = str(worse.parent)
    with this.server.graphstudy_manager.sql_ctx as curs:
        with Child(root, 'layouts') as node:
            for (layo, lid) in all_layouts.items():
                append_child(node, 'layout', _id=str(lid), layout=layo.name)
        with Child(root, 'inter-layouts') as node:
            for info in inter_layouts:
                attrs = {
                    'id' : str(info.id), 'method' : info.method.name, 'rate' : fmtnum(info.rate),
                    'parent1st' : str(info.parent1st), 'parent2nd' : str(info.parent2nd),
                }
                append_child(node, 'layout', **attrs)
        with Child(root, 'worse-layouts') as node:
            for info in worse_layouts:
                attrs = {
                    'id' : str(info.id), 'method' : info.method.name, 'rate' : fmtnum(info.rate),
                    '_parent' : str(info.parent),
                }
                append_child(node, 'layout', **attrs)
        with Child(root, 'properties') as node:
            for prop in Properties:
                _append_property(node, prop, this.server.graphstudy_manager, curs, layoutid=layoutid)
        _append_pca(root, prop, this.server.graphstudy_manager, curs, layoutid=layoutid)
    this.send_tree_xml(ET.ElementTree(root), transform='/xslt/layout.xsl')

def _append_property(node, prop, mngr, curs, layoutid=Ellipsis):
    with Child(node, 'property', name=prop.name, localized=format_bool(prop.localized)) as child:
        for (kernel, table1, table2, tag2pl, tag2sg) in [
                (Kernels.BOXED,    'PropertiesDisc', 'Histograms',      'histograms',       'histogram'),
                (Kernels.GAUSSIAN, 'PropertiesCont', 'SlidingAverages', 'sliding-averages', 'sliding-average'),
        ]:
            query = (
                "SELECT * FROM `{table:s}` WHERE `layout` = ? AND `property` = ? AND `vicinity` {null:s}" +
                " ORDER BY `vicinity`"
            ).format(table=table1, null=("NOTNULL" if prop.localized else "ISNULL"))
            for row in mngr.sql_exec_curs(curs, query, (layoutid, prop)):
                vicinity = row['vicinity']
                rowsinner = mngr.sql_select_curs(curs, table2, layout=layoutid, property=prop, vicinity=vicinity)
                if not rowsinner: continue
                with Child(child, 'property-data', kernel=kernel.name) as grandchild:
                    if vicinity is not None: grandchild.set('vicinity', fmtnum(vicinity))
                    _append_one_property_outer(grandchild, row)
                    with Child(grandchild, tag2pl) as grandgrandchild:
                        _append_one_property_inner(grandgrandchild, rowsinner, tagname=tag2sg)

def _append_pca(node, prop, mngr, curs, layoutid=Ellipsis):
        (major, minor) = _get_primary_axes_curs(mngr, curs, layoutid, oknone=True, scale=False)
        if major or minor:
            with Child(node, 'princomp') as child:
                if major is not None:
                    with Child(child, 'major') as grandchild:
                        append_child(grandchild, 'value').text = fmtnum(major[0])
                        append_child(grandchild, 'value').text = fmtnum(major[1])
                if minor is not None:
                    with Child(child, 'minor') as grandchild:
                        append_child(grandchild, 'value').text = fmtnum(minor[0])
                        append_child(grandchild, 'value').text = fmtnum(minor[1])

def _append_one_property_outer(node, row):
    keyspecs = [
        [ 'size' ], [ 'minimum' ], [ 'maximum' ], [ 'mean' ], [ 'rms' ],
        [ 'entropy', 'intercept' ], [ 'entropy', 'slope' ],
    ]
    for keyspec in keyspecs:
        try: value = row[_sqlkey(keyspec)]
        except IndexError: continue
        append_child(node, _xmlkey(keyspec)).text = fmtnum(value)

def _append_one_property_inner(node, rows, tagname='item'):
    keyspecs = [
        [ 'bincount' ], [ 'binwidth' ],
        [ 'sigma'    ], [ 'points'   ],
        [ 'entropy'  ],
    ]
    for row in rows:
        with Child(node, tagname) as child:
            try: binning = row['binning']
            except IndexError: pass
            else: child.set('binning', binning.name)
            for keyspec in keyspecs:
                try: value = row[_sqlkey(keyspec)]
                except IndexError: continue
                append_child(child, _xmlkey(keyspec)).text = fmtnum(value)

def _sqlkey(words):
    [ head, *tail ] = words
    return head.lower() + ''.join(w.capitalize() for w in tail)

def _xmlkey(words):
    return '-'.join(w.lower() for w in words)

def _get_inter_info(mngr, curs, layoutid):
    for row in mngr.sql_select_curs(curs, 'InterLayouts', id=layoutid):
        return _InterInfo(**{ k : row[k] for k in _INTER_INFO_KEYS })
    return None

def _get_worse_info(mngr, curs, layoutid):
    for row in mngr.sql_select_curs(curs, 'WorseLayouts', id=layoutid):
        return _WorseInfo(**{ k : row[k] for k in _WORSE_INFO_KEYS })
    return None

def _serve_picture(this, layoutid : Id, format : str = None, preview : bool = False, princomp : bool = False, url=None):
    (mimetype, charset) = _check_graphics_format(format)
    row = get_one_or(this.server.graphstudy_manager.sql_select('Layouts', id=layoutid))
    if row is None:
        raise HttpError(http.HTTPStatus.NOT_FOUND, "No layout with ID {!s} exists.".format(layoutid))
    data = _get_picture_from_cache(layoutid, format=format, preview=preview, princomp=princomp)
    if data is None:
        if preview:
            colors = LAYOUTS_PREFERRED_COLORS.get(row['layout'], LAYOUTS_FALLBACK_COLORS)
        else:
            colors = LAYOUTS_FALLBACK_COLORS
        cmd = [ os.path.join(this.server.graphstudy_manager.abs_bindir, 'src', 'visualizations', 'picture') ]
        if princomp:
            colors = (lambda t : (t[2], t[3]))(TANGO_COLORS['Aluminium'])
            (major, minor) = _get_primary_axes(this.server.graphstudy_manager, layoutid, scale=True)
            cmd.append('--major=({0[0]:.20E}, {0[1]:.20E})'.format(major))
            cmd.append('--minor=({0[0]:.20E}, {0[1]:.20E})'.format(minor))
            cmd.append('--axis-color=#{:06X}'.format(SECONDARY_COLOR))
        cmd.append('--node-color=#{:06X}'.format(colors[0]))
        cmd.append('--edge-color=#{:06X}'.format(colors[1]))
        cmd.append(row['file'])
        try:
            data = this.server.graphstudy_manager.call_graphstudy_tool(cmd, stdout=True)
            _offer_picture_to_cache(data, layoutid, format='SVG', preview=preview, princomp=princomp)
        except RecoverableError as e:
            raise HttpError(http.HTTPStatus.INTERNAL_SERVER_ERROR, "Cannot visualize layout: {!s}".format(e))
        conversion = _get_conversion_command_from_svg(format, preview=preview)
        if conversion is not None:
            try:
                data = this.server.graphstudy_manager.call_image_magick(conversion, stdin=data, stdout=True)
            except RecoverableError as e:
                raise HttpError(http.HTTPStatus.INTERNAL_SERVER_ERROR,
                                "Cannot convert SVG to {:s}: {!s}".format(format, e))
            _offer_picture_to_cache(data, layoutid, format=format, preview=preview, princomp=princomp)
    this.send_response(http.HTTPStatus.OK)
    this.send_header('Content-Length', str(len(data)))
    this.send_header('Content-Type', '{:s}; charset="{:s}"'.format(mimetype, charset))
    this.end_headers()
    this.wfile.write(data)
    this.wfile.flush()

def _get_conversion_command_from_svg(fmt, preview=False):
    # https://www.imagemagick.org/Usage/thumbnails/#pad
    if fmt == 'SVG':
        return None
    argspec = '{:s}:-'
    src = argspec.format('svg')
    dst = argspec.format(fmt.lower())
    geometry = '{0:d}x{0:d}'.format(_THUMBNAIL_SIZE)
    return [
        src,
        '-thumbnail', geometry,
        '-gravity', 'center',
        '-background', 'white',
        '-extent', geometry,
        dst
    ] if preview else [ src, dst ]

def _check_graphics_format(fmt):
    if not isinstance(fmt, str):
        raise TypeError("Graphics format specification must be str type")
    elif fmt == 'SVG':
        return ('image/svg+xml', 'binary')
    elif fmt == 'PNG':
        return ('image/png', 'binary')
    else:
        raise HttpError(http.HTTPStatus.NOT_FOUND, "The graphics format {!r} is not supported.".format(fmt))

def _get_primary_axes(mngr, *args, **kwargs):
    with mngr.sql_ctx as curs:
        return _get_primary_axes_curs(mngr, curs, *args, **kwargs)

def _get_primary_axes_curs(mngr, curs, layoutid, oknone=False, scale=False):
    major = _get_primary_axis_curs(mngr, curs, layoutid, 1, oknone=oknone, scale=scale)
    minor = _get_primary_axis_curs(mngr, curs, layoutid, 2, oknone=oknone, scale=scale)
    return (major, minor)

def _get_primary_axis_curs(mngr, curs, layoutid, comp, oknone=False, scale=False):
    (prop, table) = { 1 : (Properties.PRINCOMP1ST, 'MajorAxes'), 2 : (Properties.PRINCOMP2ND, 'MinorAxes') }[comp]
    row = get_one_or(mngr.sql_select_curs(curs, table, layout=layoutid))
    if row is None:
        if oknone: return None
        raise HttpError(
            http.HTTPStatus.NOT_FOUND,
            "No {:d}. primary axis available for layout {!s}.".format(comp, layoutid)
        )
    (x, y) = (row['x'], row['y'])
    if not scale: return (x, y)
    for table in [ 'PropertiesDisc', 'PropertiesCont' ]:
        for row in mngr.sql_select_curs(curs, table, layout=layoutid, property=prop, vicinity=None):
            n, mean, rms = (row['size'], row['mean'], row['rms'])
            try:
                stdev = math.sqrt(n / (n - 1) * (rms**2 - mean**2))
                return (stdev * x, stdev * y)
            except ArithmeticError as e:
                raise HttpError(http.HTTPStatus.INTERNAL_SERVER_ERROR, str(e))
    if oknone: return None
    raise HttpError(
        http.HTTPStatus.NOT_FOUND,
        "No PCA for {:d}. primary axis available for layout {!s}.".format(layoutid)
    )

def _offer_picture_to_cache(data : bytes, layoutid : Id, format : str, preview : bool, princomp : bool):
    if not _caching_policy(format=format, preview=preview, princomp=princomp):
        return False
    filename = _get_cache_file_name(layoutid, format=format, preview=preview, princomp=princomp)
    if not os.path.isfile(filename):
        directory = os.path.dirname(filename)
        tempfilename = filename + '.' + secrets.token_hex(8)
        try:
            os.makedirs(directory, exist_ok=True)
            with open(tempfilename, 'wb') as ostr:
                ostr.write(data)
            os.rename(tempfilename, filename)
            logging.debug("Added file to disk-cache: {!r}".format(filename))
            return True
        except OSError as e:
            logging.error("Cannot put file {!r} into cache: {!s}".format(filename, e))
            return False

def _get_picture_from_cache(layoutid : Id, format : str, preview : bool, princomp : bool):
    if not _caching_policy(format=format, preview=preview, princomp=princomp):
        # We could always just go ahead and see if the file exists but this quick-exit saves us one round to the
        # file-system in cases where we know for sure that the file won't be cached anyway.
        return None
    filename = _get_cache_file_name(layoutid, format=format, preview=preview, princomp=princomp)
    if filename is None:
        return None
    try:
        with open(filename, 'rb') as istr:
            data = istr.read()
        logging.debug("Found file in disk-cache: {!r}".format(filename))
        return data
    except FileNotFoundError:
        return None
    except OSError as e:
        logging.error("Cannot get file {!r} from cache: {!s}".format(filename, e))
        return None

def _get_cache_file_name(layoutid : Id, format : str, preview : bool, princomp : bool):
    cachedir = get_cache_directory()
    if cachedir is None:
        return None
    basename = {
        (False, False) : 'picture',
        (False, True ) : 'princomp',
        (True,  False) : 'thumbnail-{:d}'.format(_THUMBNAIL_SIZE),
        (True,  True ) : 'princomp-{:d}'.format(_THUMBNAIL_SIZE),
    }[(preview, princomp)]
    return os.path.join(cachedir, 'layout-pictures', str(layoutid), basename + '.' + format.lower())

def _caching_policy(format : str, preview : bool, princomp : bool):
    return (format == 'PNG')

class _NotFound(object):

    def __call__(self, *args, **kwargs):
        raise HttpError(http.HTTPStatus.NOT_FOUND)

_picture_server = collections.defaultdict(_NotFound)
_picture_server['picture']   = lambda *args, **kwargs : _serve_picture(*args, **kwargs, preview=False, princomp=False)
_picture_server['thumbnail'] = lambda *args, **kwargs : _serve_picture(*args, **kwargs, preview=True,  princomp=False)
_picture_server['princomp']  = lambda *args, **kwargs : _serve_picture(*args, **kwargs, preview=False, princomp=True)

class serve(Static):

    for_graph = _serve_for_graph
    by_id = _serve_by_id
    picture = _picture_server
