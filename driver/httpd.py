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

import concurrent.futures
import datetime
import errno
import http.server
import io
import logging
import os
import re
import shlex
import socketserver
import sys
import time
import urllib.parse

from .cli import *
from .configuration import *
from .constants import *
from .crash import *
from .errors import *
from .manager import *
from .resources import *
from .utility import *
from .www import *
from .www.badlog import serve as serve_badlog
from .www.graph import serve as serve_graph
from .www.layout import serve as serve_layout
from .www.nn import serve as serve_nn
from .www.overview import serve as serve_overview
from .www.perfstats import serve as serve_perfstats
from .www.property import serve as serve_property
from .www.property_plots import serve as serve_property_plot
from .www.select import serve as serve_select

PROGRAM_NAME = 'httpd'

PACKAGE_BUGREPORT = 'moritz.klammler@student.kit.edu'

_STARTUP_TIME = None

_XSLT_HEADERS = { 'Content-Type' : 'application/xml; charset="UTF-8"' }

_WELL_KNOWN_RESOURCES = {
    'index.html' : {
        'Content-Type' : 'text/html; charset="UTF-8"',
    },
    'nn/index.html' : {
        'Content-Type' : 'text/html; charset="UTF-8"',
    },
    'graphstudy.css' : {
        'Content-Type' : 'text/css; charset="UTF-8"',
    },
    'graphstudy.js' : {
        'Content-Type' : 'text/javascript; charset="UTF-8"',
    },
    'nn/testscore.js' : {
        'Content-Type' : 'text/javascript; charset="UTF-8"',
    },
    'schemata.sql' : {
        'Content-Type' : 'text/plain; charset="UTF-8"',
        'Content-Disposition' : 'inline; filename="schemata.sql"',
    },
    'graphstudy.png' : {
        'Content-Type' : 'image/png',
    },
    'xslt/about.xsl' : _XSLT_HEADERS,
    'xslt/all-graphs.xsl' : _XSLT_HEADERS,
    'xslt/all-layouts.xsl' : _XSLT_HEADERS,
    'xslt/badlog.xsl' : _XSLT_HEADERS,
    'xslt/graph-ids.xsl' : _XSLT_HEADERS,
    'xslt/graph-layouts.xsl' : _XSLT_HEADERS,
    'xslt/graph.xsl' : _XSLT_HEADERS,
    'xslt/layout-ids.xsl' : _XSLT_HEADERS,
    'xslt/layout.xsl' : _XSLT_HEADERS,
    'xslt/nn-demo.xsl' : _XSLT_HEADERS,
    'xslt/nn-features.xsl' : _XSLT_HEADERS,
    'xslt/nn-testscore-graphical.xsl' : _XSLT_HEADERS,
    'xslt/nn-testscore.xsl' : _XSLT_HEADERS,
    'xslt/perfstats.xsl' : _XSLT_HEADERS,
    'xslt/property.xsl' : _XSLT_HEADERS,
    'xslt/select.xsl' : _XSLT_HEADERS,
}

_WELL_KNOWN_REDIRECTS = {
    'favicon.ico' : '/graphstudy.png',
    'index.htm' : '/index',
    'index.html' : '/index',
    'index.php' : '/index',
}

_ERROR_PAGE_POINTERS = [
    ({ 'href' : '#', 'onclick' : "window.location.reload(true)" }, "Reload this page (via JavaScript)"),
    ({ 'href' : '?' }, "Reload this page dropping the query string"),
    ({ 'href' : '.' }, "Navigate to the current directory"),
    ({ 'href' : '..' }, "Navigate to the parent directory"),
    ({ 'href' : '/' }, "Navigate to the top-level directory"),
    ({ 'href' : 'mailto:' + PACKAGE_BUGREPORT }, "Contact the programmer"),
]

class RequestHandler(http.server.BaseHTTPRequestHandler):

    @Override()  # This is not actually defined in the base class, just documented
    def do_GET(self):
        url = urllib.parse.urlparse(urllib.parse.unquote_plus(self.path))
        parts, document = canonical_path(url.path)
        try:
            if len(parts) == 0 or document == 'index':
                self.__serve_index(url)
            elif document in _WELL_KNOWN_REDIRECTS:
                self.serve_redirect(_WELL_KNOWN_REDIRECTS[document], url, document=document)
            elif document in _WELL_KNOWN_RESOURCES:
                self.__serve_well_known_resource(url, document=document)
            elif parts[0] == 'graphs':
                self.__serve_graph(url, parts)
            elif parts[0] == 'layouts':
                self.__serve_layout(url, parts)
            elif parts[0] == 'properties':
                self.__serve_property(url, parts)
            elif parts[0] == 'property-plots':
                self.__serve_property_plots(url, parts)
            elif parts[0] == 'nn':
                self.__serve_nn(url, parts)
            elif document == 'perfstats':
                serve_perfstats(self, url)
            elif document == 'badlog':
                serve_badlog(self, url)
            elif document == 'select':
                serve_select(self, url)
            elif document == 'views.sql':
                raise HttpError(http.HTTPStatus.GONE, "There ain't no generated views no more")
            elif document == 'about':
                self.__serve_about(url)
            else:
                raise HttpError(http.HTTPStatus.NOT_FOUND)
        except HttpError as e:
            self.send_error(e.code, explain=e.what)

    def __serve_index(self, url):
        return self.__serve_well_known_resource(url, document='index.html')

    def __serve_well_known_resource(self, url, document=None):
        headers = _WELL_KNOWN_RESOURCES[document]
        assert 'Content-Length' not in headers
        assert 'Content-Encoding' not in headers
        assert 'Transfer-Encoding' not in headers
        try:
            logging.debug("Serving well-known resource {!r} verbatim ...".format(document))
            with get_resource_as_stream(document) as istr:
                self.send_response(http.HTTPStatus.OK)
                for key in sorted(headers.keys()):
                    self.send_header(key, headers[key])
                if self.server.well_known_resources_max_age is not None:
                    self.send_header('Cache-Control', 'max-age={:.0f}'.format(self.server.well_known_resources_max_age))
                self.send_header('Transfer-Encoding', 'chunked')
                self.end_headers()
                with _ChunkedIO(self.wfile) as ostr:
                    while True:
                        chunk = istr.read(0x2000)
                        ostr.write(chunk)
                        if not chunk: break
        except OSError as e:
            logging.error("Cannot serve document: {:s}: {!s}".format(document, e))
            raise HttpError(http.HTTPStatus.INTERNAL_SERVER_ERROR, str(e))

    def serve_redirect(self, location, url, document=None):
        scheme = url.scheme or 'http'
        (host, port) = self.server.server_address
        if host and port:
            location = '{:s}://{:s}:{:d}/{:s}'.format(scheme, host, port, location.lstrip('/'))
        logging.debug("Moved permanently: {!r} -> {!r}".format(document, location))
        info = "Please visit {!r} instead.\n".format(location).encode('utf-8')
        self.send_response(301)
        self.send_header('Location', location)
        self.send_header('Content-Length', len(info))
        self.send_header('Content-Type', 'text/plain; charset="UTF-8"')
        self.end_headers()
        self.wfile.write(info)
        self.wfile.flush()

    def __serve_graph(self, url, parts):
        assert parts[0] == 'graphs'
        if len(parts) == 1:
            serve_overview(self, url, focus='graphs')
        elif len(parts) == 2:
            graphid = validate_graph_id(parts[1])
            serve_graph(self, graphid, url)
        elif len(parts) == 3:
            graphid = validate_graph_id(parts[1])
            try:
                layout = enum_from_json(Layouts, parts[2])
            except ValueError:
                badname = translate_enum_name(parts[2], old='-', new='_', case='upper')
                msg = "The specified layout {!r} is unknown to mankind.".format(badname)
                raise HttpError(http.HTTPStatus.NOT_FOUND, msg)
            else:
                serve_layout.for_graph(self, graphid, layout, url)
        else:
            raise HttpError(http.HTTPStatus.NOT_FOUND)

    def __serve_layout(self, url, parts):
        assert parts[0] == 'layouts'
        if len(parts) == 1:
            serve_overview(self, url, focus='layouts')
        elif len(parts) == 2:
            layoutid = validate_layout_id(parts[1])
            serve_layout.by_id(self, layoutid, url)
        elif len(parts) == 3:
            layoutid = validate_layout_id(parts[1])
            (base, dot, ext) = parts[2].partition('.')
            if dot != '.':
                raise HttpError(http.HTTPStatus.NOT_FOUND, "Something is wrong; perhaps a missing file-name extension?")
            serve_layout.picture[base](self, layoutid, format=ext.upper(), url=url)
        else:
            raise HttpError(http.HTTPStatus.NOT_FOUND)

    def __serve_property(self, url, parts):
        assert parts[0] == 'properties'
        if not (3 <= len(parts) <= 4):
            raise HttpError(http.HTTPStatus.NOT_FOUND)
        graphid = validate_graph_id(parts[1])
        property = _validate_property(parts[2])
        vicinity = _validate_vicinity(parts[3] if len(parts) > 3 else None, property=property)
        serve_property(self, graphid, property, vicinity, url=url)

    def __serve_property_plots(self, url, parts):
        assert parts[0] == 'property-plots'
        if not (4 <= len(parts) <= 5):
            raise HttpError(http.HTTPStatus.NOT_FOUND)
        property = _validate_property(parts[1])
        vicinity = _validate_vicinity(parts[2] if len(parts) == 5 else '', property=property)
        kind = parts[-2]
        filename = parts[-1]
        (base, dot, ext) = filename.partition('.')
        if dot != '.':
            raise HttpError(http.HTTPStatus.NOT_FOUND, "Something is wrong; perhaps a missing file-name extension?")
        theid = validate_id(base)
        serve_property_plot[kind](self, theid, property, vicinity, url=url, format=ext.upper())

    def __serve_nn(self, url, parts):
        assert parts[0] == 'nn'
        if len(parts) == 1:
            self.__serve_well_known_resource(url, document='nn/index.html')
        elif len(parts) == 2:
            serve_nn[parts[1]](self, url)
        else:
            raise HttpError(http.HTTPStatus.NOT_FOUND)

    def __serve_sql_views(self, url):
        query = urllib.parse.parse_qs(url.query)
        try:
            force = parse_bool(''.join(query.get('force', [ 'false' ])))
        except ValueError:
            raise HttpError(http.HTTPStatus.BAD_REQUEST, "Parameter 'force' must be set to a boolean.")
        logging.debug("Serving SQL view definitions verbatim ...")
        data = get_sql_view_code(
            frozenset(self.server.graphstudy_manager.config.desired_properties.keys()),
            force=force
        ).encode('utf-8')
        self.send_response(http.HTTPStatus.OK)
        self.send_header('Content-Length', str(len(data)))
        self.send_header('Content-Type', 'text/plain; charset="UTF-8"')
        self.send_header('Content-Disposition', 'inline; filename="views.sql"')
        self.end_headers()
        self.wfile.write(data)
        self.wfile.flush()

    def __serve_about(self, url):
        uptime = datetime.timedelta(seconds=round(time.time() - _STARTUP_TIME))
        root = ET.Element('root')
        append_child(root, 'info', name="Host").text = str(self.server.server_address[0])
        append_child(root, 'info', name="Port").text = str(self.server.server_address[1])
        append_child(root, 'info', name="Process-ID").text = fmtnum(os.getpid())
        append_child(root, 'info', name="Uptime").text = str(uptime)
        append_child(root, 'info', name="Access-Log-File").text = shlex.quote(self.server.access_log_file_name)
        append_child(root, 'info', name="Error-Log-File").text = shlex.quote(self.server.error_log_file_name)
        append_child(root, 'info', name="PID-File").text = shlex.quote(value_or(self.server.pid_file_name, os.devnull))
        append_child(root, 'info', name="Command-Line").text = ' '.join(shlex.quote(w) for w in sys.argv)
        append_child(root, 'info', name="Working-Directory").text = shlex.quote(os.getcwd())
        with Child(root, 'environment') as env:
            for (envvar, envval) in os.environ.items():
                append_child(env, 'variable', name=envvar).text = shlex.quote(envval)
        self.send_tree_xml(ET.ElementTree(root), transform='/xslt/about.xsl')

    @Override(http.server.BaseHTTPRequestHandler)
    def send_error(self, code : int, message : str = None, explain : str = None):
        shortmsg, longmsg = self.responses.get(code, ("Unknown Error", None))
        assert isinstance(code, int)
        shortmsg = "{:d} {:s}".format(code, shortmsg)
        #longmsg = longmsg + '.' if longmsg and not longmsg.endswith('.') else longmsg
        if not message:
            message = shortmsg
        if not explain:
            explain = longmsg
        if explain and explain[-1] not in '.,:;?!':
            explain += '.'
        self.log_error('%s (%s)', shortmsg, explain or '???')
        tree, body = _make_html_tree(title=message)
        if explain:
            append_child(body, 'p', _class='alert').text = explain
        append_child(body, 'p').text = "You might want to try one of the following things:"
        with Child(body, 'ul') as ul:
            for (attrs, text) in _ERROR_PAGE_POINTERS:
                assert 'href' in attrs
                with Child(ul, 'li') as li:
                    append_child(li, 'a', **attrs).text = text
        self.send_tree_html(tree, status=code, extra_headers=[('Connection', 'close')])

    def send_tree_xml(self,
                      tree : ET.ElementTree,
                      status : int = http.HTTPStatus.OK,
                      filename : str = 'index.xml',
                      transform : str = None,
    ):
        # Without LXML we cannot apply transformations server-side but the client might be capable of doing so.  But
        # without LXML, we also cannot link the style sheets into the DOM.  However, it would be plain silly to fail
        # delivering a document if all that is missing is printing one trivial line of text.
        # TODO: Ideally, we would inspect the accepted formats the client offers and make decisions based on it ...
        monkey_patch = False
        if transform is not None:
            if self.server.use_server_side_xslt:
                try:
                    logging.debug("Applying XSLT style sheet {!r} ...".format(transform))
                    with get_resource_as_stream(transform) as istr:
                        xsldom = ET.parse(istr)
                        xslt = ET.XSLT(xsldom)
                        htmldom = xslt(tree)
                    return self.send_tree_html(htmldom, status)
                except (OSError, EtError) as e:
                    logging.error("Cannot apply XSLT style sheet: {:s}: {!s}".format(transform, e))
                    return self.send_tree_xml(tree, status=http.HTTPStatus.INTERNAL_SERVER_ERROR, transform=None)
                except NotImplementedError:
                    logging.error("Cannot apply XSLT style sheet {!r} (not implemented)".format(transform))
                    return self.send_tree_xml(tree, status=http.HTTPStatus.NOT_IMPLEMENTED, transform=None)
            else:
                try:
                    logging.debug("Linking XSLT style sheet {!r}".format(transform))
                    link_xslt(tree.getroot(), transform)
                except NotImplementedError:
                    monkey_patch = True
        self.send_response(status)
        self.send_header('Content-Type', 'application/xml; charset="UTF-8"')
        self.send_header('Content-Disposition', 'inline; filename="{:s}"'.format(filename))
        self.send_header('Transfer-Encoding', 'chunked')
        self.end_headers()
        with _ChunkedIO(self.wfile) as ostr:
            kwargs = { 'xml_declaration' : not monkey_patch, 'encoding' : 'UTF-8', 'method' : 'xml' }
            if ET_HAVE_LXML: kwargs['pretty_print'] = True
            if monkey_patch:
                logging.debug("Monky-patching XML processing instruction for XSLT style sheet {!r}".format(transform))
                preamble = '\n'.join([
                    '<?xml version="1.0" encoding="UTF-8" ?>',
                    '<?xml-stylesheet type="text/xsl" href="{:s}" ?>'.format(transform),
                ]) + '\n\n'
                ostr.write(preamble.encode('utf-8'))
            logging.debug("Serializing XML DOM ...")
            tree.write(ostr, **kwargs)

    def send_tree_html(self,
                       tree : ET.ElementTree,
                       status : int = http.HTTPStatus.OK,
                       filename : str = 'index.html',
                       extra_headers=[],
    ):
        self.send_response(status)
        for (key, val) in extra_headers:
            self.send_header(key, val)
        self.send_header('Content-Type', 'text/html; charset="UTF-8"')
        self.send_header('Content-Disposition', 'inline; filename="{:s}"'.format(filename))
        self.send_header('Transfer-Encoding', 'chunked')
        self.end_headers()
        with _ChunkedIO(self.wfile) as ostr:
            ostr.write(b'<!DOCTYPE html>\n\n')
            kwargs = { 'encoding' : 'UTF-8', 'method' : 'html' }
            if ET_HAVE_LXML: kwargs['pretty_print'] = True
            logging.debug("Serializing HTML DOM ...")
            tree.write(ostr, **kwargs)

    @Override(http.server.BaseHTTPRequestHandler)
    def log_request(self, code=None, size=None):
        if self.server.accesslog is not None:
            print("{host} {ident} {authuser} [{date}] {request} {status} {bytes}".format(
                host=self.address_string(),
                ident='-',
                authuser='-',
                date=self.log_date_time_string(),
                request=self.requestline,
                status=('-' if code is None else int(code)),
                bytes=('-' if size is None else int(size)),
            ), file=self.server.accesslog)

    @Override(http.server.BaseHTTPRequestHandler)
    def log_error(self, fmt, *args):
        if self.server.errorlog is not None:
            print((fmt % args), file=self.server.errorlog)

    @Override(http.server.BaseHTTPRequestHandler)
    def log_message(self, fmt, *args):
        logging.info(fmt % args)

def _validate_property(param : str):
    try:
        return enum_from_json(Properties, param)
    except ValueError:
        badname = translate_enum_name(param, old='-', new='_', case='upper')
        msg = "The specified property {!r} is unknown to mankind.".format(badname)
        raise HttpError(http.HTTPStatus.NOT_FOUND, msg)

def _validate_vicinity(param : str, property : Properties = Ellipsis):
    if not param:
        vicinity = None
    else:
        try:
            vicinity = int(param)
        except ValueError:
            raise HttpError(http.HTTPStatus.NOT_FOUND, "Vicinities can only be positive integers")
        if vicinity <= 0:
            raise HttpError(http.HTTPStatus.NOT_FOUND, "Vicinities can only be positive integers")
    if vicinity is None and property.localized:
        msg = "Requested {name:s} but property {name:s}(d) is localized on vicinity d".format(name=property.name)
        raise HttpError(http.HTTPStatus.NOT_FOUND, msg)
    if vicinity is not None and not property.localized:
        msg = "Requested {name:s}({0:d}) but property {name:s} is not localized".format(vicinity, name=property.name)
        raise HttpError(http.HTTPStatus.NOT_FOUND, msg)
    return vicinity

def _run_httpd_forever(httpd, ns):
    config = Configuration(ns.configdir)
    with Manager(absbindir=ns.bindir, absdatadir=ns.datadir, config=config) as mngr:
        setattr(httpd, 'graphstudy_manager', mngr)
        setattr(httpd, 'use_server_side_xslt', ns.xslt)
        setattr(httpd, 'well_known_resources_max_age', ns.max_age)
        httpd.serve_forever()

class Httpd(http.server.HTTPServer):

    def __init__(self, *args, accesslog : str = None, errorlog : str = None, pidfile : str = None, **kwargs):
        super().__init__(*args, **kwargs)
        def typecheck1(s):
            assert s is None or isinstance(s, str) or isinstance(s, int)
            return s
        def typecheck2(s):
            assert s is not None and isinstance(s, str) or isinstance(s, int)
            return s
        self.accesslog = None
        self.errorlog = None
        self.__access_logfile = typecheck2(accesslog)
        self.__error_logfile = typecheck2(errorlog)
        self.__pidfile = typecheck1(pidfile)

    @property
    def access_log_file_name(self):
        return _convenient_name(self.__access_logfile, mode='w')

    @property
    def error_log_file_name(self):
        return _convenient_name(self.__error_logfile, mode='w')

    @property
    def pid_file_name(self):
        return _convenient_name(self.__pidfile)

    def __enter__(self, *args, **kwargs):
        if self.__pidfile:
            pid = os.getpid()
            logging.info("Writing server PID ({:d}) to file {!r} ...".format(pid, self.__pidfile))
            with _open_log_file(self.__pidfile, truncate=True) as istr:
                print(pid, file=istr)
        if self.__access_logfile:
            logging.info("Opening access log file {!r} ...".format(self.__access_logfile))
            self.accesslog = _open_log_file(self.__access_logfile, dash=sys.stdout)
        if self.__error_logfile:
            logging.info("Opening error log file {!r} ...".format(self.__error_logfile))
            self.errorlog = _open_log_file(self.__error_logfile, dash=sys.stderr)
        return super().__enter__(*args, **kwargs)

    def __exit__(self, *args, **kwargs):
        status = super().__exit__(*args, **kwargs)
        self.accesslog = _close_log_file(self.accesslog)
        self.errorlog = _close_log_file(self.errorlog)
        if self.__pidfile:
            try: os.unlink(self.__pidfile)
            except FileNotFoundError: pass
        return status

    @Override(socketserver.TCPServer)
    def handle_error(self, *args, **kwargs):
        try:
            raise
        except BrokenPipeError:
            logging.notice("Could not send complete reply; client closed connection prematurely")
        except Exception as e:
            logging.critical("Uncaught exception of type {:s} in main loop of HTTP server".format(type(e).__name__))
            dump_current_exception_trace()
            raise SystemExit(True)

def _convenient_name(fileinfo, mode=None):
    if fileinfo is None:
        return os.devnull
    if isinstance(fileinfo, int):
        if os.name == 'posix':
            return os.path.realpath('/proc/self/fd/' + str(fileinfo))
        else:
            return str(fileinfo)
    if fileinfo == '-' and os.name == 'posix':
        if mode == 'r': return '/dev/stdin'
        if mode == 'w': return '/dev/stdout'
    return fileinfo

def _open_log_file(filename, dash=None, truncate=False):
    if filename is None:
        return None
    if filename == '-':
        return dash
    try:
        fileno = int(filename)
    except ValueError:
        pass
    else:
        return os.fdopen(os.dup(fileno), 'w')
    return open(filename, 'w' if truncate else 'a')

def _close_log_file(logfile):
    if logfile is not None:
        logfile.close()

class Main(AbstractMain):

    def _run(self, ns):
        if ns.fork and os.fork() != 0:
            logging.notice("Successfully forked child process; exiting")
            sys.exit(0)
        if not ET_HAVE_LXML:
            logging.warning("The LXML package is not available, the HTTP server might serve less useful documents")
            logging.notice("Please visit {:s} for more information about LXML and Python".format('http://lxml.de/'))
        logging.notice("Binding HTTP server to port {!r} on host {!r} ...".format(ns.port, ns.host))
        kwargs = { 'accesslog' : ns.log_access, 'errorlog' : ns.log_error, 'pidfile' : ns.pid_file }
        try:
            with Httpd((ns.host, ns.port), RequestHandler, **kwargs) as httpd:
                logging.notice("Web interface is availabe at http://{h:s}:{p:d}/".format(h=ns.host, p=ns.port))
                with concurrent.futures.ThreadPoolExecutor(max_workers=1) as executor:
                    logging.notice("Running HTTP server with PID {:d} (send SIGINT to shut down) ..."
                                   .format(os.getpid()))
                    try:
                        executor.submit(_run_httpd_forever, httpd, ns).result(timeout=ns.shutdown)
                    except concurrent.futures.TimeoutError:
                        logging.notice("{:.0f} seconds passed by; shutting down ...".format(ns.shutdown))
                        httpd.shutdown()
                    except KeyboardInterrupt:
                        logging.notice("Received shutdown signal; shutting down ...")
                        httpd.shutdown()
        except OSError as e:
            if e.errno == errno.EADDRINUSE:
                logging.error("Cannot connect to port {:d} which is already in use by another service".format(ns.port))
                raise FatalError("Cannot start HTTP server")
            raise

    def _argparse_hook_before(self, ap):
        ag = ap.add_argument_group("Webserver")
        ag.add_argument(
            '-h', '--host', metavar='HOST', type=str, default='localhost',
            help="bind to the specified interfaces (default: %(default)s)"
        )
        ag.add_argument(
            '-p', '--port', metavar='PORT', type=int, default=8000,
            help="bind to the specified port number (default: %(default)d)"
        )
        ag.add_argument(
            '-s', '--shutdown', metavar='SECS', type=float, default=None,
            help="stop running after the specified number of seconds (default: run forever)"
        )
        ag.add_argument(
            '-f', '--fork', action='store_true',
            help="fork and run as background process (default: don't fork)"
        )
        ag.add_argument(
            '--pid-file', metavar='FILE', type=_filename, default=None,
            help="write to FILE the PID to send SIGINT to in order to shut down (default: don't write PID file)"
        )
        ag.add_argument(
            '-a', '--log-access', metavar='FILE', type=_filename, default=os.devnull,
            help="write access log to FILE (default: don't log anything)"
        )
        ag.add_argument(
            '-e', '--log-error', metavar='FILE', type=_filename, default=os.devnull,
            help="write error log to FILE (default: don't log anything)"
        )
        ag.add_argument(
            '--xslt', action='store_true', dest='xslt', default=ET_HAVE_LXML,
            help="try processing XSLT style sheets on the server side"
        )
        ag.add_argument(
            '--no-xslt', action='store_false', dest='xslt', default=ET_HAVE_LXML,
            help="never process XSLT style sheets on the server side"
        )
        ag.add_argument(
            '--max-age', metavar='SECS', type=float, default=None,
            help="allow content caching for static resources (default: expire immediately)"
        )

def _make_html_tree(title=None, h1=True):
    iconurl = '/graphstudy.png'
    scripturl = '/graphstudy.js'
    styleurl = '/graphstudy.css'
    html = ET.Element('html')
    with Child(html, 'head') as head:
        Child(head, 'meta', charset='UTF-8')
        if title is not None:
            append_child(head, 'title').text = title
        append_child(head, 'link', rel='stylesheet', type='text/css', href=styleurl)
        append_child(head, 'link', rel='shortcut icon', type='image/png', href=iconurl)
        append_child(head, 'script', type='text/javascript', src=scripturl)
    body = append_child(html, 'body', onload='init()')
    if not h1:
        pass
    elif h1 is True and title is not None:
        append_child(body, 'h1').text = title
    elif isinstance(h1, str):
        append_child(body, 'h1').text = h1
    else:
        assert title is None
    return (ET.ElementTree(html), body)

def canonical_path(path):
    parts = list(filter(None, path.split('/')))
    return (parts, '/'.join(parts))

class _ChunkedIO(io.IOBase):

    def __init__(self, ostr):
        self.__out = ostr

    @Override(io.IOBase)
    def __enter__(self, *args):
        return self

    @Override(io.IOBase)
    def __exit__(self, *args):
        self.__out.write(b'0\r\n\r\n')
        self.__out.flush()

    def write(self, data):
        if data:
            self.__out.write('{:x}\r\n'.format(len(data)).encode('ascii'))
            self.__out.write(data)
            self.__out.write(b'\r\n')

    @Override(io.IOBase)
    def flush(self):
        return self.__out.flush()

    @Override(io.IOBase)
    def fileno(self, *args, **kwargs):
        return self.__out.fileno(*args, **kwargs)

    @Override(io.IOBase)
    def seek(self, *args, **kwargs):
        return self.__out.seek(*args, **kwargs)

    @Override(io.IOBase)
    def truncate(self, *args, **kwargs):
        return self.__out.truncate(*args, **kwargs)

def _filename(token):
    try:
        return int(token)
    except ValueError:
        return str(token)

if __name__ == '__main__':
    _STARTUP_TIME = time.time()
    kwargs = {
        'prog' : PROGRAM_NAME,
        'usage' : "%(prog)s [-h HOST] [-p PORT]",
        'description' : "Runs a web server that allows inspecting the results.",
    }
    with Main(**kwargs) as app:
        app(sys.argv[1 : ])
