# -*- coding: utf-8 -*-
'''
MyServer
Copyright (C) 2009 Free Software Foundation, Inc.
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

import socket
from myserver.pycontrol import PyMyServerControl
from configWrapper import VHostWrapper, make_vhost_list

import sys

class Controller():
    '''High level library to control GNU MyServer. Every method returns True
    or requested data on success and False on error.'''
    class Parser():
        '''Parse command output.'''
        def connections(self, lines): # TODO: split lines to dicts
            '''Parse data returned by SHOWCONNECTIONS command.'''
            l = []
            for line in lines.splitlines():
                parts = line.split(' - ')
                l.append(dict(zip(['id', 'client_ip', 'client_port',
                    'server_ip', 'server_port', '?a', '?b'], parts)))
            return l

        def language_files(self, lines):
            '''Parse data returned by SHOWLANGUAGEFILES command.'''
            return lines.splitlines()

    def __init__(self, host, port, username, password):
        self.connection = None
        self.parser = self.Parser()
        self.host = host
        self.port = port
        self.username = username
        self.password = password
        self.response_code = 100

    def get_response_code(self):
        '''Get response code of last issued command.'''
        return self.response_code

    def connect(self):
        '''Connect to server.'''
        self.disconnect()
        try:
            self.connection = PyMyServerControl(self.host, self.port,
                self.username, self.password)
        except socket.error:
            return False
        return True

    def disconnect(self):
        '''Disconnect from server.'''
        if self.connection:
            self.connection.close()
            self.connection = None
        return True

    def reboot(self):
        '''Reboot server.'''
        if self.connection:
            self.connection.send_header('REBOOT', 0)
            self.connection.read_header()
            self.response_code = self.connection.response_code
            if self.response_code != '100':
                return False
            return True
        else:
            return False

    def version(self):
        '''Get version string.'''
        if self.connection:
            self.connection.send_header('VERSION', 0)
            self.connection.read_header()
            self.response_code = self.connection.response_code
            if self.response_code != '100':
                return False
            return self.connection.read()
        else:
            return False

    def enable_reboot(self):
        '''Enable server auto-reboot.'''
        if self.connection:
            self.connection.send_header('ENABLEREBOOT', 0)
            self.connection.read_header()
            self.response_code = self.connection.response_code
            if self.response_code != '100':
                return False
            return True
        else:
            return False

    def disable_reboot(self):
        '''Disable server auto-reboot.'''
        if self.connection:
            self.connection.send_header('DISABLEREBOOT', 0)
            self.connection.read_header()
            self.response_code = self.connection.response_code
            if self.response_code != '100':
                return False
            return True
        else:
            return False

    def show_connections(self):
        '''List active server connections.'''
        if self.connection:
            self.connection.send_header('SHOWCONNECTIONS', 0)
            self.connection.read_header()
            self.response_code = self.connection.response_code
            if self.response_code != '100':
                return False
            return self.parser.connections(self.connection.read())
        else:
            return False

    def kill_connection(self, num):
        '''Kill connection to server.'''
        if self.connection:
            self.connection.send_header('KILLCONNECTION', 0, str(num))
            self.connection.read_header()
            self.response_code = self.connection.response_code
            if self.response_code != '100':
                return False
            return True
        else:
            return False

    def show_language_files(self):
        '''List available language files.'''
        if self.connection:
            self.connection.send_header('SHOWLANGUAGEFILES', 0)
            self.connection.read_header()
            self.response_code = self.connection.response_code
            if self.response_code != '100':
                return False
            return self.parser.language_files(self.connection.read())
        else:
            return False

    def get_file(self, file_name):
        '''Get file from server.'''
        if self.connection:
            self.connection.send_header('GETFILE', 0, file_name)
            self.connection.read_header()
            self.response_code = self.connection.response_code
            if self.response_code != '100':
                return False
            return self.connection.read()
        else:
            return False

    def get_vhost_configuration(self):
        '''Get virtual hosts configuration wrapped in list of VHostWrapper
        objects.'''
        text = self.get_file('virtualhosts.xml')
        if text is not False:
            return make_vhost_list(text)
        return False

