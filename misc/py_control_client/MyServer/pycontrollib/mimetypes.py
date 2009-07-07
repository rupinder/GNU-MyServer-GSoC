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

from definition import Definition
from lxml import etree

class MIMEType():
    def __init__(self, mime, handler, param = None, extension = [], path = None,
                 filter = [], self_executed = None, definitions = []):
        '''Creates new MIMEType with specified attributes. extension, filter and
        definitions are expected to be iterable.'''
        self.set_mime(mime)
        self.set_handler(handler)
        self.set_param(param)
        self.extension = set()
        for single_extension in extension:
            self.add_extension(single_extension)
        self.set_path(path)
        self.filter = []
        for single_filter in filter:
            self.add_filter(single_filter)
        self.set_self_executed(self_executed)
        self.definitions = []
        for definition in definitions:
            self.add_definition(definition)

    def get_mime(self):
        '''Get associated mime type.'''
        return self.mime

    def set_mime(self, mime):
        '''Set associated mime type.'''
        if mime is None:
            raise AttributeError('mime is required and can\'t be None')
        self.mime = mime

    def get_handler(self):
        '''Get associated handler.'''
        return self.handler

    def set_handler(self, handler):
        '''Set associated handler.'''
        if handler is None:
            raise AttributeError('handler is required and can\'t be None')
        self.handler = handler

    def get_param(self):
        '''Get associated param.'''
        return self.param

    def set_param(self, param):
        '''Set associated param. None means no param.'''
        self.param = param

    def get_extensions(self):
        '''Get associated extensions.'''
        return self.extension

    def remove_extension(self, extension):
        '''Remove extension from associated extensions.'''
        self.extension.remove(extension)

    def add_extension(self, extension):
        '''Add extension to associated extensions.'''
        self.extension.add(extension)

    def get_path(self):
        '''Get associated path.'''
        return self.path

    def set_path(self, path):
        '''Set associated path. None means no path.'''
        self.path = path

    def get_filters(self):
        '''Get associated filters.'''
        return self.filter

    def get_filter(self, index):
        '''Get filter with given index.'''
        return self.filter[index]

    def remove_filter(self, index):
        '''Remove filter with given index.'''
        self.filter.pop(index)

    def add_filter(self, filter, index = None):
        '''Append filter after all other filters, or insert it at index.'''
        if index is None:
            self.filter.append(filter)
        else:
            self.filter.insert(index, filter)

    def get_self_executed(self):
        '''Get self_executed setting.'''
        return self.self_executed

    def set_self_executed(self, self_executed):
        '''Set self_executed setting.'''
        self.self_executed = self_executed

    def get_definitions(self):
        '''Get all definitions.'''
        return self.definitions
    
    def get_definition(self, index):
        '''Get definition with given index.'''
        return self.definitions[index]

    def add_definition(self, definition, index = None):
        '''Append definition after all other definitions, or insert it at
        index.'''
        if index is None:
            self.definitions.append(definition)
        else:
            self.definitions.insert(index, definition)

    def remove_definition(self, index):
        '''Remove definition with given index.'''
        self.definitions.pop(index)

    def __eq__(self, other):
        return isinstance(other, MIMEType) and \
            self.mime == other.mime and \
            self.handler == other.handler and \
            self.param == other.param and \
            self.extension == other.extension and \
            self.path == other.path and \
            self.filter == other.filter and \
            self.self_executed == other.self_executed and \
            self.definitions == other.definitions

    def to_lxml_element(self):
        '''Convert to lxml.etree.Element.'''
        def make_element(tag, attribute, value):
            element = etree.Element(tag)
            element.set(attribute, value)
            return element
        def make_extension_element(extension):
            return make_element('EXTENSION', 'value', extension)
        def make_filter_element(filter):
            return make_element('FILTER', 'value', filter)
        root = etree.Element('MIME')
        root.set('mime', self.mime)
        root.set('handler', self.handler)
        if self.param is not None:
            root.set('param', self.param)
        if self.self_executed is not None:
            root.set('self', 'YES' if self.self_executed else 'NO')
        if self.path is not None:
            root.append(make_element('PATH', 'regex', self.path))
        for element in map(make_extension_element, self.extension):
            root.append(element)
        for element in map(make_filter_element, self.filter):
            root.append(element)
        for definition in self.definitions:
            root.append(definition.to_lxml_element())
        return root

    def __str__(self):
        return etree.tostring(self.to_lxml_element(), pretty_print = True)

    @staticmethod
    def from_lxml_element(root):
        '''Factory to produce MIMEType from lxml.etree.Element object.'''
        if root.tag != 'MIME':
            raise AttributeError('Expected MIME tag.')
        mime = root.get('mime', None)
        handler = root.get('handler', None)
        param = root.get('param', None)
        self_executed = root.get('self', None)
        if self_executed is not None:
            self_executed = self_executed == 'YES'
        path = None
        extension = set()
        filter = []
        definitions = []
        for child in list(root):
            if child.tag == 'PATH':
                path = child.get('regex')
            elif child.tag == 'FILTER':
                filter.append(child.get('value'))
            elif child.tag == 'EXTENSION':
                extension.add(child.get('value'))
            elif child.tag == 'DEFINE':
                definitions.append(Definition.from_lxml_element(child))
        return MIMEType(mime, handler, param, extension, path, filter,
                        self_executed, definitions)

    @staticmethod
    def from_string(text):
        '''Factory to produce MIMEType by parsing a string.'''
        return MIMEType.from_lxml_element(etree.XML(text))

class MIMETypes():
    def __init__(self, MIME_types):
        self.MIME_types = MIME_types
        
    def __eq__(self, other):
        return isinstance(other, MIMETypes) and \
            self.MIME_types == other.MIME_types

    def to_lxml_element(self):
        '''Convert to lxml.etree.Element.'''
        root = etree.Element('MIMES')
        for mime in self.MIME_types:
            root.append(mime.to_lxml_element())
        return root

    def __str__(self):
        return etree.tostring(self.to_lxml_element(), pretty_print = True)
        
    @staticmethod
    def from_lxml_element(root):
        '''Factory to produce MIMETypes from lxml.etree.Element object.'''
        if root.tag != 'MIMES':
            raise AttributeError('Expected MIMES tag.')
        return MIMETypes(map(MIMEType.from_lxml_element, list(root)))
    
    @staticmethod
    def from_string(text):
        '''Factory to produce MIMETypes from parsing a string.'''
        return MIMETypes.from_lxml_element(etree.XML(text))
