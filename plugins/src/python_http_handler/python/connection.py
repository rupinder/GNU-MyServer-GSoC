'''
MyServer
Copyright (C) 2008 The Free Software Foundation Inc.
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


class Connection(object):

    def __init__(self):
        self.remote_ip = get_remote_addr()
        self.remote_logname = ""
        self.base_server = ""
        self.local_addr = ""
        self.remote_addr = ""
        self.remote_ip = ""
        self.remote_host = ""
        self.remote_logname = ""
        self.aborted = False
        self.keepalive = 0
        self.double_reverse = 0
        self.keepalives = 0
        self.local_ip = ""
        self.local_host = ""
        self.id = 0

    def log_error(self, message, level = 0):
        pass


    def read(self, length = -1):
        return None

    def readline(self, length = -1):
        return None

    def write(self, string):
        return send_data(string)


