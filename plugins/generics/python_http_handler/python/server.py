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


class Server(object):

    def __init__(self):
        self.server_hostname = get_request_header("Host")
        self.port = get_local_port()
        self.defn_name = "myserver.xml"
        self.defn_line_number = ""
        self.server_admin = "admin"
        self.server_hostname = ""
        self.names = ""
        self.wild_names = ""
        self.port = 80
        self.error_fname = None
        self.loglevel = 0
        self.is_virtual = True
        self.timeout = 0
        self.keep_alive_timeout = 0
        self.keep_alive_max = -1
        self.keep_alive = True
        self.path = ""
        self.pathlen = 0
        self.limit_req_line = 0
        self.limit_req_fieldsize = 0
        self.limit_req_fields = 25
        self.__cleanups = ()

    def get_cleanups(self):
        return self.__cleanups


    def get_config(self):
        return {}

    def get_options(self):
        return {}


    def log_error(self, message, level = 0):
        log_server_error(message)
        pass

    def register_cleanup(request, callable, data = None):
        self.__cleanups.append([callable, data])

