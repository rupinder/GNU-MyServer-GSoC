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

class MIMEType():
    valid_handlers = set(['SEND', 'CGI', 'FASTCGI', 'SCGI', 'MSCGI', 'ISAPI',
                          'WINCGI', 'PROxY'])

    class WrongArguments(Exception):
        '''Exception raised when MIMEType is constructed with invalid
        arguments.'''
        pass

    def __init__(self, mime, handler, param, extension, path = None,
                 filter = None, self_executed = None, definitions = None):
        '''Creates new MIMEType with specified attributes.'''
        self.mime = mime
        if handler not in MIMEType.valid_handlers:
            raise MIMEType.WrongArguments(
                '{0} is not a valid handler'.format(handler))
        self.handler = handler
        self.param = param
        self.extension = extension
        self.path = path
        self.filter = filter
        self.self_executed = self_executed
        self.definitions = definitions
        
    def __eq__(self, other):
        return \
            self.mime == other.mime and \
            self.handler == other.handler and \
            self.param == other.param and \
            self.extension == other.extension and \
            self.path == other.path and \
            self.filter == other.filter and \
            self.self_executed == other.self_executed and \
            self.definitions == other.definitions

    def to_lxml_element(self): 
        def make_element(tag, attribute, value):
            element = etree.Element(tag)
            element.set(attribute, value)
            return element
        def make_extension_element(extension):
            return make_element('EXTENSION', 'value', extension)
        root = etree.Element('MIME')
        root.set('mime', self.mime)
        root.set('handler', self.handler)
        root.set('param', self.param)
        if self.self_executed is not None:
            root.set('self', 'YES' if self.self_executed else 'NO')
        for element in map(make_extension_element, self.extension):
            root.append(element)
        if self.path is not None:
            root.append(make_element('PATH', 'regex', self.path))
        if self.filter is not None:
            root.append(make_element('FILTER', 'value', self.filter))
        return root

    def __str__(self): # TODO: definitions
        return etree.tostring(self.to_lxml_element(), pretty_print = True)

    @staticmethod
    def from_lxml_element(root): # TODO: definitions
        '''Factory to produce MIMEType from etree.Element object.'''
        mime = root.get('mime', None)
        handler = root.get('handler', None)
        param = root.get('param', None)
        self_executed = root.get('self', None)
        if self_executed is not None:
            self_executed = False if self_executed == 'NO' else True
        extension = set(map(lambda element: element.get('value'),
                            root.findall('EXTENSION')))
        path = root.find('PATH')
        if path is not None:
            path = path.get('regex')
        filter = root.find('FILTER')
        if filter is not None:
            filter = filter.get('value')
        return MIMEType(mime, handler, param, extension, path, filter,
                        self_executed, None)

    @staticmethod
    def from_string(text):
        '''Factory to produce MIMEType by parsing a string.'''
        return MIMEType.from_lxml_element(etree.XML(text))

class MIMETypes():
    def __init__(self, MIME_types):
        self.MIME_types = MIME_types
        
    def __eq__(self, other):
        return self.MIME_types == other.MIME_types

    def to_lxml_element(self):
        root = etree.Element('MIMES')
        for mime in self.MIME_types:
            root.append(mime.to_lxml_element())
        return root

    def __str__(self):
        return etree.tostring(self.to_lxml_element(), pretty_print = True)
        
    @staticmethod
    def from_lxml_element(root):
        '''Factory to produce MIMETypes from etree.Element object.'''
        return MIMETypes(map(MIMEType.from_lxml_element, root.findall('MIME')))
    
    @staticmethod
    def from_string(text):
        '''Factory to produce MIMETypes from parsing a string.'''
        return MIMETypes.from_lxml_element(etree.XML(text))
