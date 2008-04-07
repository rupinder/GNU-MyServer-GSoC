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
        self.protocol = "HTTP"

        self.headers_out = {}
        self.__headers_sent = False
        
    def send_headers(self):
        send_header()


    def write(self, str):
        if not self.__headers_sent:
            self.send_headers()
            self.__headers_sent = True
        return send_data(str)
