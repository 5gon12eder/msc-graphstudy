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

import http
import logging
import math
import os.path
import statistics
import textwrap
import urllib.parse

from . import *
from ..constants import *
from ..errors import *
from ..tools import *
from ..utility import *
from .impl_common import *

class _GN(float):

    def __init__(self, value=None):
        self.__value = math.nan if value is None else float(value)

    @property
    def value(self):
        return self.__value

    def __format__(self, fmtspec=None):
        textual = 'NaN' if not math.isfinite(self.__value) else format(self.__value, '.10E')
        if not fmtspec or fmtspec == 'G':
            return textual
        elif fmtspec.endswith('G'):
            width = int(fmtspec[:-1])
            return ''.join(' ' for i in range(max(0, width - len(textual)))) + textual
        else:
            raise ValueError(fmtspec)

def _serve_histogram(this, layoutid, prop, vicinity, url=Ellipsis, format=Ellipsis):
    assert (vicinity is None) != (prop.localized)
    (terminal, mimetype, charset) = _check_graphics_format(this, format)
    query = urllib.parse.parse_qs(url.query)
    serve_source = get_bool_from_query(query, 'source')
    bincount = _get_bincount_from_query(query)
    infoname = prop.name if not prop.localized else '{:s}({:d})'.format(prop.name, vicinity)
    if bincount is None:
        row = get_first_or(this.server.graphstudy_manager.sql_select(
            'Histograms', layout=layoutid, property=prop, vicinity=vicinity, binning=Binnings.SCOTT_NORMAL_REFERENCE)
        )
        if not row:
            msg = "No histogram for property {:s} with automatic binning available.".format(infoname)
            raise HttpError(http.HTTPStatus.NOT_FOUND, msg)
    else:
        fc = Binnings.FIXED_COUNT
        row = get_first_or(this.server.graphstudy_manager.sql_select(
            'Histograms', property=prop, layout=layoutid, binning=fc, bincount=bincount)
        )
        if not row:
            msg = "No histogram for property {:s} of layout {!s} with {:d} bins ({:s}) available.".format(
                infoname, layoutid, bincount, fc.name)
            raise HttpError(http.HTTPStatus.NOT_FOUND, msg)
    assert row is not None
    xlabel = "{0[1]:s} / {0[0]:s}".format(PROPERTY_QUANTITIES[prop])
    ylabel = "relative frequency"
    title = "{bc:,d} bins (width = {bw:,.3g} {xunit:s})".format(
        bc=row['bincount'], bw=row['binwidth'], xunit=PROPERTY_QUANTITIES[prop][0]
    )
    color = '#{:06X}'.format(PRIMARY_COLOR)
    gnuplot = find_tool_lazily(GNUPLOT)
    script = textwrap.dedent('''\
    #! {gp:s}
    #! -*- coding:utf-8; mode:gnuplot; -*-"

    set terminal {fmt:s} noenhanced size 600,450
    set output
    set xrange [0 : *]
    set yrange [0 : *]
    set title {plottitle!r}
    set xlabel {xlabel!r}
    set ylabel {ylabel!r}
    set format y '%.2E'
    set style fill solid 1.0
    plot {df!r} with boxes linecolor rgb {lc!r} title {title!r}
    ''').format(
        gp=gnuplot, fmt=terminal, plottitle=infoname, df=_gpltin(row['file']), xlabel=xlabel, ylabel=ylabel,
        title=title, lc=color
    )
    _serve_plot(this, script, mimetype, charset, source=serve_source)

def _serve_sliding_average(this, layoutid, prop, vicinity, url=Ellipsis, format=Ellipsis):
    assert (vicinity is None) != (prop.localized)
    (terminal, mimetype, charset) = _check_graphics_format(this, format)
    query = urllib.parse.parse_qs(url.query)
    serve_source = get_bool_from_query(query, 'source')
    sigma = _get_sigma_from_query(query)
    infoname = prop.name if not prop.localized else '{:s}({:d})'.format(prop.name, vicinity)
    entries = {
        row['sigma'] : row for row in
        this.server.graphstudy_manager.sql_select('SlidingAverages', layout=layoutid, property=prop, vicinity=vicinity)
    }
    if not entries:
        raise HttpError(http.HTTPStatus.NOT_FOUND, "No sliding averages for property {:s} available".format(infoname))
    if sigma is None:
        sigma = statistics.median(entries.keys())
    row = entries[min(entries.keys(), key=(lambda s : abs(s - sigma)))]
    (sigma, points, filename) = (row[key] for key in [ 'sigma', 'points', 'file' ])
    xlabel = "{0[1]:s} / {0[0]:s}".format(PROPERTY_QUANTITIES[prop])
    ylabel = "relative density"
    title = "\u03C3 = {:.3g} {:s} ({:,d} points)".format(sigma, PROPERTY_QUANTITIES[prop][0], points)
    color = '#{:06X}'.format(PRIMARY_COLOR)
    gnuplot = find_tool_lazily(GNUPLOT)
    script = textwrap.dedent('''\
    #! {gp:s}
    #! -*- coding:utf-8; mode:gnuplot; -*-"

    set terminal {fmt:s} noenhanced size 600,450
    set output
    set xrange [0 : *]
    set yrange [0 : *]
    set title {plottitle!r}
    set xlabel {xlabel!r}
    set ylabel {ylabel!r}
    set format y '%.2E'
    set arrow from graph 0.05,0.90 rto first {s2:G},0 heads linecolor rgb {lc2!r}
    plot {df!r} with linespoints linecolor rgb {lc!r} title {title!r}
    ''').format(
        gp=gnuplot, fmt=terminal, plottitle=infoname, df=_gpltin(filename), xlabel=xlabel, ylabel=ylabel,
        s2=_GN(2.0 * sigma), title=title, lc=color, lc2='#{:06X}'.format(SECONDARY_COLOR)
    )
    _serve_plot(this, script, mimetype, charset, source=serve_source)

def _serve_entropy(this, graphid, prop, vicinity, url=Ellipsis, format=Ellipsis):
    mngr = this.server.graphstudy_manager
    assert (vicinity is None) != (prop.localized)
    (terminal, mimetype, charset) = _check_graphics_format(this, format)
    query = urllib.parse.parse_qs(url.query)
    serve_source = get_bool_from_query(query, 'source')
    query = urllib.parse.parse_qs(url.query)
    infoname = prop.name if not prop.localized else '{:s}({:d})'.format(prop.name, vicinity)
    try:             selected_layouts = frozenset(validate_layout_id(token) for token in query['layouts'])
    except KeyError: selected_layouts = None
    entropies = dict()
    regressions = dict()
    layouts = { l : None for l in Layouts }
    with mngr.sql_ctx as curs:
        for layorow in mngr.sql_select_curs(curs, 'Layouts', graph=graphid, layout=object):
            layoutid = layorow['id']
            if selected_layouts is not None and layoutid not in selected_layouts:
                continue
            entropy = dict()
            kwargs = { 'layout' : layoutid, 'property' : prop, 'vicinity' : vicinity }
            for proprow in mngr.sql_select_curs(curs, 'Histograms', **kwargs, binning=Binnings.FIXED_COUNT):
                entropy[proprow['bincount']] = value_or(proprow['entropy'], math.nan)
            if not entropy:
                raise HttpError(
                    http.HTTPStatus.NOT_FOUND,
                    "No entropy for property {:s} of layout {!s}".format(infoname, layoutid)
                )
            regrow = get_one_or(mngr.sql_select_curs(curs, 'PropertiesDisc', **kwargs))
            if regrow is None:
                raise HttpError(
                    http.HTTPStatus.NOT_FOUND,
                    "No entropy regression for property {:s} of layout {!s}".format(infoname, layoutid)
                )
            regression = (regrow['entropyIntercept'], regrow['entropySlope'])
            layouts[layorow['layout']] = layoutid
            entropies[layoutid] = entropy
            regressions[layoutid] = regression
    if selected_layouts is not None:
        for lid in selected_layouts - set(layouts.values()):
            raise HttpError(http.HTTPStatus.NOT_FOUND, "No layout with ID {!s} exists.".format(lid))
    script = [
        "#! {:s}".format(find_tool_lazily(GNUPLOT)),
        "#! -*- coding:utf-8; mode:gnuplot; -*-",
        "",
        "set terminal {:s} size 800,600".format(terminal),
        "set output",
        "set title {!r} noenhanced".format(infoname),
        "set xrange [0 : *]",
        "set yrange [0 : *]",
        "set xlabel {!r}".format("log_2({:s})".format("bin count / 1")),
        "set ylabel {!r}".format("entropy / bit"),
        "set format x '%.1f'",
        "set format y '%.2E'",
        "set key left top Left reverse noenhanced",
        "",
    ]
    plotexprs = list()
    for (lo, lid) in sorted(filter(get_second, layouts.items()), key=get_first):
        (d, k) = regressions[lid]
        script.append("{:20s} = {d:20G} + {k:20G} * x".format('f_' + lo.name.lower() + '(x)', d=_GN(d), k=_GN(k)))
        (color1st, color2nd) = ('#{:06X}'.format(c) for c in LAYOUTS_PREFERRED_COLORS[lo])
        title = '{:s} (d = {:,.3g}, k = {:,.3g})'.format(lo.name, d, k)
        plotexprs.append("{!r} using 1:2 linecolor rgb {!r} title {!r}".format('-', color1st, title))
        plotexprs.append("{:s}(x) linecolor rgb {!r} notitle".format('f_' + lo.name.lower(), color2nd))
    script.append("")
    script.append("plot " + ', \\\n     '.join(plotexprs))
    script.append("")
    for (lo, lid) in sorted(filter(get_second, layouts.items()), key=get_first):
        script.append("")
        script.append("# {!s} ({:s})".format(lid, lo.name))
        script.append("# {:>18s} {:>20s}".format("key", "entropy / bit"))
        for kv in sorted(entropies[lid].items(), key=get_first):
            script.append("{x:20G} {y:20G}".format(x=_GN(math.log2(kv[0])), y=_GN(kv[1])))
        script.append('e')
    script.append("")  # ensure trailing newline at end of file
    _serve_plot(this, '\n'.join(script), mimetype, charset, source=serve_source)

def _serve_plot(this, script, mimetype, charset, source=False):
    if source:
        data = script.encode('utf-8')
        this.send_response(http.HTTPStatus.OK)
        this.send_header('Content-Length', str(len(data)))
        this.send_header('Content-Type', 'text/plain; charset="UTF-8"')
        this.send_header('Content-Disposition', 'inline; filename="gnuplot.txt"')
        this.end_headers()
        this.wfile.write(data)
        this.wfile.flush()
    else:
        try:
            data = this.server.graphstudy_manager.call_gnuplot(script, stdout=True)
        except RecoverableError as e:
            raise HttpError(http.HTTPStatus.INTERNAL_SERVER_ERROR, "Cannot create plot: {!s}".format(e))
        else:
            this.send_response(http.HTTPStatus.OK)
            this.send_header('Content-Length', str(len(data)))
            this.send_header('Content-Type', '{:s}; charset="{:s}"'.format(mimetype, charset))
            this.end_headers()
            this.wfile.write(data)
            this.wfile.flush()

def _get_bincount_from_query(query):
    try:
        [ text ] = query['bincount']
    except KeyError:
        return None
    except ValueError:
        raise HttpError(http.HTTPStatus.BAD_REQUEST, "Zero or one 'bincount' parameters are allowed.")
    try:
        bincount = int(text)
        if bincount >= 0: return bincount
    except ValueError:
        pass
    raise HttpError(http.HTTPStatus.BAD_REQUEST, "Parameter 'bincount' must be set to a non-negative integer.")

def _get_sigma_from_query(query):
    try:
        [ text ] = query['sigma']
    except KeyError:
        return None
    except ValueError:
        raise HttpError(http.HTTPStatus.BAD_REQUEST, "Zero or one 'sigma' parameters are allowed.")
    try:
        sigma = float(text)
        if math.isfinite(sigma) and sigma > 0.0: return sigma
    except ValueError:
        pass
    raise HttpError(http.HTTPStatus.BAD_REQUEST, "Parameter 'sigma' must be set to a positive real value.")

def _get_vicinities_from_query(query):
    vic = query.get('vicinity')
    if not vic or '*' in vic:
        return None
    try:
        vics = list(map(int, vic))
        if all(l >= 0 for l in vics): return vics
    except ValueError:
        pass
    raise HttpError(http.HTTPStatus.BAD_REQUEST, "Parameter 'vicinity' must be set to a non-negative integer.")

def _check_graphics_format(this, fmt):
    if not isinstance(fmt, str):
        raise TypeError()
    elif fmt == 'SVG':
        return ('svg', 'image/svg+xml', 'binary')
    elif fmt == 'PNG':
        return ('png', 'image/png', 'binary')
    else:
        raise HttpError(http.HTTPStatus.NOT_FOUND, "The graphics format {!r} is not supported.".format(fmt))

def _gpltin(filename):
    ext = os.path.splitext(filename)[1].upper().lstrip('.')
    if ext in [ 'TXT', 'DAT' ]:
        return filename
    elif ext == 'GZ':
        return '<{:s} {:s}'.format(find_tool_lazily(GZIP_FILTER), filename)
    elif ext == 'BZ2':
        return '<{:s} {:s}'.format(find_tool_lazily(BZ2_FILTER), filename)
    else:
        raise HttpError(http.HTTPStatus.INTERNAL_SERVER_ERROR, "Cannot infer type of data file {!r}".format(filename))

class _UnknownHandler(object):

    def __init__(self, kind):
        self.__kind = kind

    def __call__(self, *args, **kwargs):
        msg = "The specified plot type {!r} is unknown to mankind.".format(self.__kind)
        raise HttpError(http.HTTPStatus.NOT_FOUND, msg)

class _Handler(object):

    def __init__(self):
        self.__handlers = {
            'ENTROPY' : _serve_entropy,
            Kernels.BOXED.name : _serve_histogram,
            Kernels.GAUSSIAN.name : _serve_sliding_average,
        }

    def __getitem__(self, key):
        key = translate_enum_name(key, old='-', new='_', case='upper')
        try:
            return self.__handlers[key]
        except KeyError:
            return _UnknownHandler(key)

serve = _Handler()
