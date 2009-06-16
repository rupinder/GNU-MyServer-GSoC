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

def make_vhost_list(data):
    '''Make a list of VHostWrapper objects from data in format as
    virtualhosts.xml configuration file.'''
    parser = etree.XMLParser(remove_blank_text = True)
    tree = etree.XML(data, parser)
    return [VHostWrapper(host, from_text = False) for host in \
        tree.findall('VHOST')]

class VHostWrapper():
    '''Class wrapping around vhost configuration.'''

    allowed_protocols = set(['CONTROL', 'FTP', 'HTTP', 'HTTPS'])
    
    class ProtocolError(ValueError):
        '''Exception raised when user tried to use invalid protocol.'''
        def __init__(self, protocol):
            '''protocol is the invalid protocol that user tried to use.'''
            ValueError.__init__(self)
            self.protocol = protocol
        
        def __str__(self):
            return self.protocol + ' is not a valid protocol'

    def __init__(self, data, from_text = True):
        '''Create new VHostWrapper by parsing data, if from_text is False, then
        it is assumed that data has already been parsed and is an lxml
        object.'''
        if from_text:
            parser = etree.XMLParser(remove_blank_text = True)
            self.tree = etree.XML(data, parser)
        else:
            self.tree = data
        
    def __str__(self):
        return self.to_xml()

    def to_xml(self):
        '''Return configuration representation in xml format.'''
        return etree.tostring(self.tree, encoding = 'utf-8',
            xml_declaration = True, pretty_print = True)

    def get_node(self, name, multiple = False):
        '''Get value stored in node of given name. If multiple is True, returns
        list of all values stored in nodes of given name'''
        if not multiple:
            node = self.tree.find(name)
            if node is not None:
                return node.text
            return None
        else:
            nodes = self.tree.findal(name)
            return map(lambda node: node.text, nodes)

    def set_node(self, name, data):
        '''Set value stored in node of given name to data. Don't use it when
        multiple nodes with given name are present, use add_node and del_node
        instead.'''
        node = self.tree.find(name)
        if data is None:
            if node is not None:
                self.tree.remove(node)
            return
        if node is not None:
            node.text = data
        else:
            node = etree.Element(name)
            node.text = data
            self.tree.append(node)
            
    def add_node(self, name, data):
        '''Add new node with given name and value to tree.'''
        node = self.tree.makeelement(name)
        node.text = data
        self.tree.append(node)
        
    def del_node(self, name, data):
        '''Remove node with given name and value from tree.'''
        for node in self.tree.findall(name):
            if node.text == data:
                self.tree.remove(node)
                break

    def get_name(self):
        '''Get vhost name.'''
        return self.get_node('NAME')

    def set_name(self, name):
        '''Set vhost name.'''
        self.set_node('NAME', name)

    def get_port(self):
        '''Get port used to accept connections.'''
        return self.get_node('PORT')

    def set_port(self, port):
        '''Set port used to accept connections.'''
        self.set_node('PORT', str(port))

    def get_ip(self):
        '''Get list of CIDR subnet masks for this vhost.'''
        return self.get_node('IP', multiple = True)

    def add_ip(self, ip):
        '''Add CIDR subnet mask for this vhost.'''
        self.add_node('IP', ip)
        
    def del_ip(self, ip):
        '''Remove CIDR subnet mast for this vhost.'''
        self.del_node('IP', ip)

    def get_protocol(self):
        '''Get used protocol.'''
        return self.get_node('PROTOCOL')

    def set_protocol(self, protocol):
        '''Set used protocol. Will raise ProtocolError if there is no such
        protocol.'''
        if protocol not in self.allowed_protocols:
            raise self.ProtocolError(protocol)
        self.set_node('PROTOCOL', protocol)

    def get_docroot(self):
        '''Get document root directory.'''
        return self.get_node('DOCROOT')

    def set_docroot(self, docroot):
        '''Set document root directory.'''
        self.set_node('DOCROOT', docroot)

    def get_sysfolder(self):
        '''Get system directory.'''
        return self.get_node('SYSFOLDER')

    def set_sysfolder(self, sysfolder):
        '''Set system directory.'''
        self.set_node('SYSFOLDER', sysfolder)

    def get_host(self): # TODO: useRegex option
        '''Get list of host names.'''
        return self.get_node('HOST', multiple = True)

    def add_host(self, host): # TODO: useRegex option
        '''Add host name.'''
        self.add_node('HOST', host)

    def del_host(self, host): # TODO: useRegex option
        '''Remove host name.'''
        self.del_node('HOST', host)

    def get_accesslog(self): # TODO: streams, etc.
        '''Get access log used by this vhost.'''
        return self.get_node('ACCESSLOG')

    def set_accesslog(self, accesslog): # TODO: streams, etc.
        '''Set access log used by this vhost.'''
        self.set_node('ACCESSLOG', accesslog)

    def get_warninglog(self): # TODO: streams, etc.
        '''Get errors log used by this vhost.'''
        return self.get_node('WARNINGLOG')

    def set_warninglog(self, warninglog): # TODO: streams, etc.
        '''Set errors log used by this vhost.'''
        self.set_node('WARNINGLOG', warninglog)

