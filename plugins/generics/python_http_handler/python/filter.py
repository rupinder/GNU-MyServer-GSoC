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

class Filter(object):

    def __init__(self, req):
        self.closed = False
        self.name = ""
        self.req = req
        self.is_input = False
        self.handler = None

    def pass_on(self):
        pass

    def read(self, length = -1):
        pass

    def readline(length = -1):
        pass

    def write(self, data):
        pass

    def flush(self):
        pass
    
    def close(self):
        pass

    def disable(self):
        pass

