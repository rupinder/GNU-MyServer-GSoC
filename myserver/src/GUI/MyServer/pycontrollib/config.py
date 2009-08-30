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
from definition import Definition, DefinitionList

class MyServerConfig():
    def __init__(self, definitions = []):
        self.definitions = DefinitionList(definitions)

    def __eq__(self, other):
        return isinstance(other, MyServerConfig) and \
            self.definitions == other.definitions

    def get_definitions(self):
        '''Get current definitions.'''
        return self.definitions.get_definitions()
    
    def add_definition(self, definition, index = None):
        '''Append definition, if index is not None insert it at index-th
        position.'''
        self.definitions.add_definition(definition, index)

    def get_definition(self, index):
        '''Get index-th definition.'''
        return self.definitions.get_definition(index)

    def remove_definition(self, index):
        '''Remove index-th definition.'''
        self.definitions.remove_definition(index)

    @staticmethod
    def from_string(text):
        '''Factory to produce MyServerConfig by parsing a string.'''
        return MyServerConfig.from_lxml_element(etree.XML(
                text, parser = etree.XMLParser(remove_comments = True)))

    @staticmethod
    def from_lxml_element(root):
        '''Factory to produce MyServerConfig from lxml.etree.Element object.'''
        if root.tag != 'MYSERVER':
            raise AttributeError('Expected MYSERVER tag.')
        return MyServerConfig(map(Definition.from_lxml_element, list(root)))

    def __str__(self):
        return etree.tostring(self.to_lxml_element(), pretty_print = True)

    def to_lxml_element(self):
        '''Convert MyServerConfig to lxml.etree.Element object.'''
        root = etree.Element('MYSERVER')
        for definition in self.definitions.get_definitions():
            root.append(definition.to_lxml_element())
        for element in self.definitions.custom:
            root.append(element)
        return root
