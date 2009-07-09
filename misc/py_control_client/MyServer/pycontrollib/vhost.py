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
from log import Log

class VHost():
    def __init__(self, name = None, port = None, protocol = None,
                 doc_root = None, sys_folder = None, logs = [],
                 ip = [], host = {}):
        '''Create new instance of VHost. logs and ip are expected to be a
        collection and host is expected to be a dict {name: useRegex} where None
        means not set.'''
        self.set_name(name)
        self.set_port(port)
        self.set_protocol(protocol)
        self.set_doc_root(doc_root)
        self.set_sys_folder(sys_folder)
        self.logs = []
        for log in logs:
            self.add_log(log)
        self.ip = set()
        for ip_address in ip:
            self.add_ip(ip_address)
        self.host = {}
        for single_host in host.iteritems():
            self.add_host(single_host[0], single_host[1])

    def __eq__(self, other):
        return isinstance(other, VHost) and self.name == other.name and \
            self.port == other.port and self.protocol == other.protocol and \
            self.doc_root == other.doc_root and \
            self.sys_folder == other.sys_folder and \
            self.logs == other.logs and self.ip == other.ip and \
            self.host == other.host

    def get_name(self):
        '''Get VHost name.'''
        return self.name

    def set_name(self, name):
        '''Set VHost name.'''
        self.name = name

    def get_doc_root(self):
        '''Get VHost doc root.'''
        return self.doc_root

    def set_doc_root(self, doc_root):
        '''Set VHost doc root.'''
        self.doc_root = doc_root

    def get_sys_folder(self):
        '''Get VHost sys folder.'''
        return self.sys_folder

    def set_sys_folder(self, sys_folder):
        '''Set VHost sys folder.'''
        self.sys_folder = sys_folder

    def get_protocol(self):
        '''Get VHost protocol.'''
        return self.protocol

    def set_protocol(self, protocol):
        '''Set VHost protocol.'''
        self.protocol = protocol

    def get_port(self):
        '''Get VHost port.'''
        return self.port

    def set_port(self, port):
        '''Set VHost port.'''
        if port is None:
            self.port = None
        else:
            self.port = int(port)

    def get_logs(self):
        '''Get list of logs.'''
        return self.logs

    def get_log(self, index):
        '''Get index-th log.'''
        return self.logs[index]

    def remove_log(self, index):
        '''Remove index-th log.'''
        self.logs.pop(index)

    def add_log(self, log, index = None):
        '''Add log to VHost's logs, if index is None append it, otherwise insert
        at index position.'''
        if index is None:
            self.logs.append(log)
        else:
            self.logs.insert(index, log)

    def get_ip(self):
        '''Get VHost ip set.'''
        return self.ip

    def add_ip(self, ip):
        '''Add ip to VHost ip set.'''
        self.ip.add(ip)

    def remove_ip(self, ip):
        '''Remove ip from VHost ip set.'''
        self.ip.remove(ip)

    def get_host(self):
        '''Get VHost host dict.'''
        return self.host

    def add_host(self, host, use_regex = None):
        '''Add host to VHost host dict.'''
        self.host[host] = use_regex

    def remove_host(self, host):
        '''Remove host from VHost host dict.'''
        self.host.pop(host)

    def __str__(self):
        return etree.tostring(self.to_lxml_element(), pretty_print = True)

    def to_lxml_element(self):
        '''Convert to instance of lxml.etree.Element.'''
        def make_element(tag, text):
            element = etree.Element(tag)
            element.text = text
            return element
        root = etree.Element('VHOST')
        if self.name is not None:
            root.append(make_element('NAME', self.name))
        if self.port is not None:
            root.append(make_element('PORT', str(self.port)))
        if self.protocol is not None:
            root.append(make_element('PROTOCOL', self.protocol))
        if self.doc_root is not None:
            root.append(make_element('DOCROOT', self.doc_root))
        if self.sys_folder is not None:
            root.append(make_element('SYSFOLDER', self.sys_folder))
        for ip_address in self.ip:
            root.append(make_element('IP', ip_address))
        for host, use_regex in self.host.iteritems():
            element = make_element('HOST', host)
            if use_regex is not None:
                element.set('useRegex', 'YES' if use_regex else 'NO')
            root.append(element)
        for log in self.logs:
            root.append(log.to_lxml_element())
        return root

    @staticmethod
    def from_lxml_element(root):
        '''Factory to produce VHost from lxml.etree.Element object.'''
        if root.tag != 'VHOST':
            raise AttributeError('Expected VHOST tag.')
        name = None
        port = None
        protocol = None
        doc_root = None
        sys_folder = None
        ip = []
        host = {}
        logs = []
        for child in list(root):
            if child.tag == 'NAME':
                name = child.text
            elif child.tag == 'PORT':
                port = child.text
            elif child.tag == 'PROTOCOL':
                protocol = child.text
            elif child.tag == 'DOCROOT':
                doc_root = child.text
            elif child.tag == 'SYSFOLDER':
                sys_folder = child.text
            elif child.tag == 'IP':
                ip.append(child.text)
            elif child.tag == 'HOST':
                use_regex = child.get('useRegex', None)
                if use_regex is not None:
                    use_regex = use_regex == 'YES'
                host[child.text] = use_regex
            else:
                logs.append(Log.from_lxml_element(child))
        return VHost(name, port, protocol, doc_root, sys_folder, logs, ip, host)
    
    @staticmethod
    def from_string(text):
        '''Factory to produce VHost by parsing a string.'''
        return VHost.from_lxml_element(etree.XML(text))

class VHosts():
    def __init__(self, VHosts):
        '''Create a new instance of VHosts. VHosts attribute is expected to be a
        list.'''
        self.VHosts = VHosts

    def __eq__(self, other):
        return isinstance(other, VHosts) and self.VHosts == other.VHosts

    def to_lxml_element(self):
        '''Convert to lxml.etree.Element.'''
        root = etree.Element('VHOSTS')
        for vhost in self.VHosts:
            root.append(vhost.to_lxml_element())
        return root

    def __str__(self):
        return etree.tostring(self.to_lxml_element(), pretty_print = True)

    @staticmethod
    def from_lxml_element(root):
        '''Factory to produce VHosts from lxml.etree.Element object.'''
        if root.tag != 'VHOSTS':
            raise AttributeError('Expected VHOSTS tag.')
        return VHosts(map(VHost.from_lxml_element, list(root)))

    @staticmethod
    def from_string(text):
        '''Factory to produce VHosts from parsing a string.'''
        return VHosts.from_lxml_element(etree.XML(text))
