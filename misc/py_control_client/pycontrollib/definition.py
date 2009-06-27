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

class Definition():
    '''Common interface for DefinitionElement and DefinitionTree. Objects of
    this type should not exist alone.'''

    def __init__(self, name = None, attributes = {}):
        '''Creates new definition with given name and attributes.'''
        self.name = name
        self.attributes = attributes

    def __eq__(self, other):
        return self.name == other.name and self.attributes == other.attributes

    def get_name(self):
        '''Definition name, None means no name.'''
        return self.name

    def set_name(self, name):
        '''Set definition name, None means no name.'''
        self.name = name

    def get_attribute(self, key):
        '''Get value of attribute key.'''
        return self.attributes.get(key)

    def set_attribute(self, key, value):
        '''Set attribute key to given value.'''
        self.attributes[key] = value

    def remove_attribute(self, key):
        '''Remove attribute key.'''
        self.attributes.pop(key)

    @staticmethod
    def from_lxml_element(root):
        '''Factory to produce definition element or tree from etree.Element
        object.'''
        if len(list(root)):
            return DefinitionTree.from_lxml_element(root)
        else:
            return DefinitionElement.from_lxml_element(root)

    @staticmethod
    def from_string(text):
        '''Factory to produce definition element or tree by parsing a string.'''
        return Definition.from_lxml_element(etree.XML(text))

class DefinitionElement(Definition):
    '''Single definition element.'''

    def __init__(self, name = None, attributes = {}):
        '''Creates new definition element with given name and attributes.'''
        Definition.__init__(self, name, attributes)

    def __eq__(self, other):
        return Definition.__eq__(self, other)

    def to_lxml_element(self):
        '''Convert definition element to etree.Element object.'''
        root = etree.Element('DEFINE')
        for key, value in self.attributes.iteritems():
            root.set(key, value)
        if self.name is not None:
            root.set('name', self.name)
        return root

    def __str__(self):
        return etree.tostring(self.to_lxml_element(), pretty_print = True)

    @staticmethod
    def from_lxml_element(root):
        '''Factory to produce definition element from etree.Element object.'''
        attributes = root.attrib
        name = attributes.pop('name', None)
        return DefinitionElement(name, attributes)

    @staticmethod
    def from_string(text):
        '''Factory to produce definition element by parsing a string.'''
        return DefinitionElement.from_lxml_element(etree.XML(text))

class DefinitionTree(Definition):
    '''Definition element containing other definitions.'''

    def __init__(self, name = None, values = [], attributes = {}):
        '''Creates new definition tree with given name, sub-definitions and
        attributes.'''
        Definition.__init__(self, name, attributes)
        self.values = values

    def __eq__(self, other):
        return Definition.__eq__(self, other) and self.values == other.values

    def get_values(self):
        '''Get all sub-definitions.'''
        return self.values

    def get_value(self, index):
        '''Get sub-definition with given index.'''
        return self.values[index]

    def add_value(self, value, index = None):
        '''Add value to sub-definitions, either at given position, or at the
        end.'''
        if index is None:
            self.values.append(value)
        else:
            self.values.insert(index, value)

    def remove_value(self, index):
        '''Remove sub-definition with given index.'''
        self.values.pop(index)

    @staticmethod
    def from_lxml_element(root):
        '''Factory to produce definition tree from etree.Element object.'''
        attributes = root.attrib
        name = attributes.pop('name', None)
        values = map(Definition.from_lxml_element, list(root))
        return DefinitionTree(name, values, attributes)

    @staticmethod
    def from_string(text):
        '''Factory to produce definition tree by parsing a string.'''
        return DefinitionTree.from_lxml_element(etree.XML(text))

    def to_lxml_element(self):
        '''Convert definition tree to etree.Element object.'''
        root = etree.Element('DEFINE')
        for key, value in self.attributes.iteritems():
            root.set(key, value)
        if self.name is not None:
            root.set('name', self.name)
        for value in self.values:
            root.append(value.to_lxml_element())
        return root

    def __str__(self):
        return etree.tostring(self.to_lxml_element(), pretty_print = True)

