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
import socket
import ssl
import time

error_codes = {'100': 'CONTROL_OK',
               '200': 'CONTROL_ERROR',
               '201': 'CONTROL_INTERNAL',
               '202': 'CONTROL_AUTH',     
               '203': 'CONTROL_MALFORMED',
               '204': 'CONTROL_CMD_NOT_FOUND',
               '205': 'CONTROL_BAD_LEN',
               '206': 'CONTROL_SERVER_BUSY',
               '207': 'CONTROL_BAD_VERSION',
               '208': 'CONTROL_FILE_NOT_FOUND'}

class PyMyServerControl(object):
    def __init__(self, host, port, login, password):
        '''Initialize a connection to server:port using login:password as
        credentials.'''

        self.connectionType = "Keep-Alive"

        self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.s.connect((host, port))

        self.host = host
        self.port = port
        self.login = login
        self.password = password
        self.sock = ssl.wrap_socket(self.s)

    def send_header(self, command, len, args = ""):
        '''Build the CONTROL header and send it.'''
        self.buffer = ""
        self.response_values = {}
        self.response_code = None
        req_header = "/" + command + " CONTROL/1.0 " +args + "\r\n"
        req_header = req_header + "/AUTH " + self.login +":" + self.password + "\r\n"
        req_header = req_header + "/CONNECTION " + self.connectionType + "\r\n"
        req_header = req_header + "\r\n"

        self.sock.write(req_header)

    def send(self, data):
        '''Send additional data after the header.'''
        self.sock.write(data)


    def __readline(self):
        '''Read a line from the socket.'''
        while True:
            self.buffer = self.buffer + self.sock.read(1024)
            ind = self.buffer.find("\r\n")

            if ind != 1:
                ret = self.buffer[0:ind]
                self.buffer = self.buffer[ind+2:len(self.buffer)]
                return ret

    def available_data(self):
        '''Returns the number of bytes to be read.'''
        return int(self.response_values['LEN']) - self.__bytes_read

    def read(self):
        '''Read data that follows the the response header.'''
        if len(self.buffer) > 0:
            self.__bytes_read = self.__bytes_read + len(self.buffer)
            return self.buffer

        data = self.sock.read(int(self.response_values['LEN']) - self.__bytes_read)
        self.__bytes_read = self.__bytes_read + len(data)

        return data

    def read_header(self):
        '''Read the response header.'''
        self.response_code = self.__readline()[1:4]
        self.__bytes_read = 0

        while True:
            line = self.__readline()
            if len(line) == 0:
                break
            ind = line.find(" ")

            if ind == -1:
                return

            header_name = line[1:ind]
            value = line[ind+1:len(line)]

            self.response_values[header_name] = value


    def close(self):
        '''Close the socket.'''
        del self.sock
        self.s.close()
