'''
MyServer
Copyright (C) 2008 The MyServer Team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
'''

from python_http_handler_internal import *
from connection import Connection
from server import Server
import os
from time import time
from time import gmtime

class Request(object):

    def __init__(self):
        self.subprocess_env = []
        self.uri = get_request_header("uri")
        self.headers_in = {}
        self.ap_auth_type = None
        self.clength = 0
        self.content_type = get_request_header("Content-Type")
        self.path_info = self.uri
        self.args = get_request_header("opts")
        self.connection = Connection()
        self.user = ""
        self.method = get_request_header("cmd")
        self.server = Server()
        self.headers_out = {}
        self.next = None
        self.prev = None
        self.main = None
        self.the_request = self.method + " " + self.uri + " HTTP/1.1"
        self.protocol = "HTTP"
        self.__headers_sent = False
        self.assbackwards = False
        self.proxyreq = False
        self.header_only = False
        self.proto_num = 1001
        self.hostname = ""
        self.request_time = 0
        self.status_line = "200 OK"
        self.status = ""
        self.method_number = 0
        self.allowed = 0
        self.allowed_xmethods = 0
        self.allowed_methods = 0
        self.sent_bodyct = 0
        self.bytes_sent = 0
        self.mtime = 0
        self.chunked = False
        self.range = ""
        self.remaining = 0
        self.read_length = 0
        self.read_body = 1
        self.read_chunked = False
        self.expecting_100 = False
        self.err_headers_out = {}
        self.notes = None
        self.phase = 0
        self.interpreter = "python"
        self.content_languages = []
        self.handler = ""
        self.content_encoding = ""
        self.vlist_validator = 0
        self.no_cache = True
        self.no_local_copy = True
        self.unparsed_uri = ""
        self.filename = ""
        self.canonical_filename = ""
        self.finfo = None
        self.parsed_uri = {}
        self.used_path_info = ""
        self.eos_sent = False
        self.__cleanups = ()
        self.__allowed = ()

    def get_cleanups(self):
        return self.__cleanups

    def send_headers(self):
        if len(self.__allowed) > 0:
            set_response_header("Allowed", self.__allowed)

        send_header()


    def write(self, str, flush = False):
        if not self.__headers_sent:
            self.send_headers()
            self.__headers_sent = True
        ret = self.connection.write(str)
        
        if flush:
            self.flush()

        return ret



    def add_common_vars(self):
        self.subprocess_env = []


    def add_handler(self, type, handler, *dir):
        pass

    def add_input_filter(self, filter_name):
        pass

    def add_output_filter(self, filter_name):
        pass

    def allow_methods(self, methods, reset = False):
        if reset:
            self.__allowed = methods
        else:
            self.__allowed = self.__allowed + ", " + methods


    def auth_name(self):
        pass

    def auth_type(self):
        pass

    def construct_url(self, uri):
        pass

    def discard_request_body(self):
        pass

    def document_root(self):
        return get_document_root()

    def get_basic_auth_pw(self):
        pass

    def get_config(self):
        pass

    def get_remote_host(self, type = 'REMOTE_NAME', str_is_ip = True):
        return get_remote_addr()

    def get_options(self):
        pass
    
    def internal_redirect(self, uri):
        #This is not an internal redirect.
        return send_redirect(uri)

    def is_https(self):
        return is_ssl()

    def log_error(self, message, level = 0):
        pass

    def meets_conditions(self):
        return True

    def requires(self):
        return []

    def read(self, len = -1):
        return self.connection.read(len)

    def readline(self, len = -1):
        return self.connection.readline(len)

    def readlines(self, sizehint = 1):
        pass

    def register_cleanup(self, callable, data = None):
        self.__cleanups.append([callable, data])

    def register_input_filter(self, filter_name, filter, dir =""):
        pass

    def register_output_filter(self, filter_name, filter, dir = ""):
        pass

    def sendfile(self, path, offset = 0, len = -1):
        file = open(path, "r")

        size = os.path.getsize(path) - offset
        tosend = size

        while tosend > 0:
            data = file.read(4096)
            send(data)
            tosend = tosend - len(data)

        return size

    def set_etag(self):
        return set_response_header("ETag", str(floor(time() * 100) % 100000))

    def set_last_modified(self):
        time_str = strftime("%a, %d %b %Y %H:%M:%S GMT", gmtime())
        set_response_header("Last-Modified", time_str)

    def ssl_var_lookup(self, var_name):
        return None

    def update_mtime(self, dependency_mtime):
        if dependency_mtime > mtime:
            mtime = dependency_mtime

    def flush(self):
        pass

    def set_content_length(self, len):
        set_response_header("Content-Length", str(len))

