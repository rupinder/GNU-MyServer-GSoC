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

from lxml import etree

class Vhost():
    def __init__(self, name, port, protocol, docroot, sysfolder, access_log,
                 warning_log, ip = set(), host = set(), host_use_regex = None):
        self.name = name
        self.port = port
        self.protocol = protocol
        self.docroot = docroot
        self.sysfolder = sysfolder
        self.access_log = access_log
        self.warning_log = warning_log
        self.ip = ip
        self.host = host
        self.host_use_regex = host_use_regex

    def get_name(self):
        '''Get VHost name.'''
        return self.name

    def set_name(self, name):
        '''Set VHost name.'''
        if name is None:
            raise AttributeError('name is required and can\'t be None')
        self.name = name

    def get_sysfolder(self):
        '''Get VHost sysfolder.'''
        return self.sysfolder

    def set_sysfolder(self, sysfolder):
        '''Set VHost sysfolder.'''
        if sysfolder is None:
            raise AttributeError('sysfolder is required and can\'t be None')
        self.sysfolder = sysfolder

    def get_protocol(self):
        '''Get VHost protocol.'''
        return self.protocol

    def set_protocol(self, protocol):
        '''Set VHost protocol.'''
        if protocol is None:
            raise AttributeError('protocol is required and can\'t be None')
        self.protocol = protocol

    def get_port(self):
        '''Get VHost port.'''
        return self.port

    def set_port(self, port):
        '''Set VHost port.'''
        if port is None:
            raise AttributeError('port is required and can\'t be None')
        self.port = port

    def get_access_log(self):
        '''Get VHost access log.'''
        return self.access_log

    def set_access_log(self, access_log):
        '''Set VHost access log.'''
        if access_log is None:
            raise AttributeError('access_log is required and can\'t be None')
        self.access_log = access_log

    def get_warning_log(self):
        '''Get VHost warning_log.'''
        return self.warning_log

    def set_warning_log(self, warning_log):
        '''Set VHost warning_log.'''
        if warning_log is None:
            raise AttributeError('warning_log is required and can\'t be None')
        self.warning_log = warning_log

    def get_ip(self):
        '''Get VHost ip set.'''
        return self.ip

    def add_ip(self, ip):
        '''Add ip to VHost ip set.'''
        self.ip.add(ip)

    def remove_ip(self, ip):
        '''Remove ip from VHost ip set.'''
        self.ip.remove(ip)
