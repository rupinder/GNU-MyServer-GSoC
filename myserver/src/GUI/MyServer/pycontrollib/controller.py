# -*- coding: utf-8 -*-
'''
MyServer
Copyright (C) 2009, 2010 Free Software Foundation, Inc.
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

from MyServer.pycontrol.pycontrol import PyMyServerControl, error_codes
from mimetypes import MIMETypes
from vhost import VHosts
from config import MyServerConfig

class ServerError(Exception):
    '''Raised when MyServer return code doesn't mean success.'''
    pass

class BasicController():
    def __init__(self, host, port, username, password):
        self.connection = None
        self.host = host
        self.port = int(port)
        self.username = username
        self.password = password

    def __check_return_code(self):
        return_code = self.connection.response_code
        if error_codes.get(return_code, '') != 'CONTROL_OK':
            raise ServerError('MyServer returned {0}: {1}'.format(
                    return_code, error_codes.get(return_code, '')))

    def connect(self):
        '''Connect to server.'''
        self.disconnect()
        self.connection = PyMyServerControl(self.host, self.port,
                                            self.username, self.password)

    def disconnect(self):
        '''Disconnect from server.'''
        if self.connection is not None:
            self.connection.close()
            self.connection = None

    def reboot(self):
        '''Reboot server.'''
        if self.connection:
            self.connection.send_header('REBOOT', 0)
            self.connection.read_header()
            self.__check_return_code()

    def version(self):
        '''Get version string.'''
        if self.connection:
            self.connection.send_header('VERSION', 0)
            self.connection.read_header()
            self.__check_return_code()
            return self.connection.read()

    def enable_reboot(self):
        '''Enable server auto-reboot.'''
        if self.connection:
            self.connection.send_header('ENABLEREBOOT', 0)
            self.connection.read_header()
            self.__check_return_code()

    def disable_reboot(self):
        '''Disable server auto-reboot.'''
        if self.connection:
            self.connection.send_header('DISABLEREBOOT', 0)
            self.connection.read_header()
            self.__check_return_code()

    def show_connections(self):
        '''List active server connections.'''
        if self.connection:
            self.connection.send_header('SHOWCONNECTIONS', 0)
            self.connection.read_header()
            self.__check_return_code()
            return self.connection.read()

    def kill_connection(self, num):
        '''Kill connection to server.'''
        if self.connection:
            self.connection.send_header('KILLCONNECTION', 0, str(num))
            self.connection.read_header()
            self.__check_return_code()

    def show_language_files(self):
        '''List available language files.'''
        if self.connection:
            self.connection.send_header('SHOWLANGUAGEFILES', 0)
            self.connection.read_header()
            self.__check_return_code()
            return self.connection.read()

    def get_file(self, path):
        '''Get file from server.'''
        if self.connection:
            self.connection.send_header('GETFILE', 0, path)
            self.connection.read_header()
            self.__check_return_code()
            return self.connection.read()

    def put_file(self, text, path):
        '''Put file to server.'''
        if self.connection:
            self.connection.send_header('PUTFILE', len(text), path)
            self.connection.send(text)
            self.connection.read_header()
            self.__check_return_code()

class Controller(BasicController):
    def get_MIME_type_configuration(self):
        '''Get MIME types settings.'''
        return MIMETypes.from_string(self.get_file('&&&mime'))

    def get_vhost_configuration(self):
        '''Get VHosts settings.'''
        return VHosts.from_string(self.get_file('&&&vhost'))

    def get_server_configuration(self):
        '''Get server settings.'''
        return MyServerConfig.from_string(self.get_file('&&&server'))

    def put_MIME_type_configuration(self, config):
        '''Put MIME types settings.'''
        self.put_file(str(config), '&&&mime')

    def put_vhost_configuration(self, config):
        '''Put VHost settings.'''
        self.put_file(str(config), '&&&vhost')

    def put_server_configuration(self, config):
        '''Put server settings.'''
        self.put_file(str(config), '&&&server')
