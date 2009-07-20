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
        self.set_name(name)
        self.attributes = {}
        for key, value in attributes.iteritems():
            self.set_attribute(key, value)

    def __eq__(self, other):
        return isinstance(other, Definition) and \
            self.name == other.name and \
            self.attributes == other.attributes

    def get_name(self):
        '''Definition name, None means no name.'''
        return self.name

    def set_name(self, name):
        '''Set definition name, None means no name.'''
        self.name = name

    def get_attributes(self):
        '''Get dict of all attributes.'''
        return self.attributes

    def get_attribute(self, key):
        '''Get value of attribute key.'''
        return self.attributes[key]

    def set_attribute(self, key, value):
        '''Set attribute key to given value.'''
        if key == 'value':
            raise KeyError('value is not an allowed key')
        self.attributes[key] = value

    def remove_attribute(self, key):
        '''Remove attribute key.'''
        self.attributes.pop(key)

    @staticmethod
    def from_lxml_element(root):
        '''Factory to produce definition element or tree from lxml.etree.Element
        object.'''
        if root.tag != 'DEFINE':
            raise AttributeError('Expected DEFINE tag.')
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
        Definition.__init__(self, name)
        for key, value in attributes.iteritems():
            self.set_attribute(key, value)

    def __eq__(self, other):
        return isinstance(other, DefinitionElement) and \
            Definition.__eq__(self, other)

    def set_attribute(self, key, value):
        '''Set attribute key to given value.'''
        if key == 'value':
            self.attributes['value'] = value
        else:
            Definition.set_attribute(self, key, value)

    def to_lxml_element(self):
        '''Convert definition element to lxml.etree.Element object.'''
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
        '''Factory to produce definition element from lxml.etree.Element
        object.'''
        if root.tag != 'DEFINE':
            raise AttributeError('Expected DEFINE tag.')
        attributes = root.attrib
        name = attributes.pop('name', None)
        return DefinitionElement(name, attributes)

    @staticmethod
    def from_string(text):
        '''Factory to produce definition element by parsing a string.'''
        return DefinitionElement.from_lxml_element(etree.XML(text))
    
    def search_by_name(self, name):
        return None if name != self.name else self

class DefinitionTree(Definition):
    '''Definition element containing other definitions.'''

    def __init__(self, name = None, definitions = [], attributes = {}):
        '''Creates new definition tree with given name, sub-definitions and
        attributes. values is expected to be iterable.'''
        Definition.__init__(self, name, attributes)
        self.definitions = []
        for definition in definitions:
            self.add_definition(definition)

    def __eq__(self, other):
        return isinstance(other, DefinitionTree) and \
            Definition.__eq__(self, other) and \
            self.definitions == other.definitions

    def get_definitions(self):
        '''Get all sub-definitions.'''
        return self.definitions

    def get_definition(self, index):
        '''Get sub-definition with given index.'''
        return self.definitions[index]

    def add_definition(self, value, index = None):
        '''Add value to sub-definitions, either at given position, or at the
        end.'''
        if index is None:
            self.definitions.append(value)
        else:
            self.definitions.insert(index, value)

    def remove_definition(self, index):
        '''Remove sub-definition with given index.'''
        self.definitions.pop(index)

    @staticmethod
    def from_lxml_element(root):
        '''Factory to produce definition tree from lxml.etree.Element object.'''
        if root.tag != 'DEFINE':
            raise AttributeError('Expected DEFINE tag.')
        attributes = root.attrib
        name = attributes.pop('name', None)
        definitions = map(Definition.from_lxml_element, list(root))
        return DefinitionTree(name, definitions, attributes)

    @staticmethod
    def from_string(text):
        '''Factory to produce definition tree by parsing a string.'''
        return DefinitionTree.from_lxml_element(etree.XML(text))

    def to_lxml_element(self):
        '''Convert definition tree to lxml.etree.Element object.'''
        root = etree.Element('DEFINE')
        for key, value in self.attributes.iteritems():
            root.set(key, value)
        if self.name is not None:
            root.set('name', self.name)
        for definition in self.definitions:
            root.append(definition.to_lxml_element())
        return root

    def __str__(self):
        return etree.tostring(self.to_lxml_element(), pretty_print = True)
    
    def search_by_name(self, name):
        for definition in reversed(self.definitions):
            ret = definition.search_by_name(name)
            if ret != None:
                return ret
        return None if name != self.name else self

class DefinitionList():
    def __init__(self, definitions = []):
        '''Construct new DefinitionList object with given definitions.'''
        self.definitions = []
        for definition in definitions:
            self.add_definition(definition)

    def add_definition(self, definition, index = None):
        '''Append definition to current list of definitions, if index is not
        None insert at index-th position.'''
        if index is None:
            self.definitions.append(definition)
        else:
            self.definitions.insert(index, definition)

    def get_definitions(self):
        '''Get current list of definitions.'''
        return self.definitions

    def get_definition(self, index):
        '''Get index-th definition.'''
        return self.definitions[index]

    def remove_definition(self, index):
        '''Remove index-th definition.'''
        self.definitions.pop(index)

    def __eq__(self, other):
        return isinstance(other, DefinitionList) and \
            self.definitions == other.definitions

    def __str__(self):
        return '\n'.join(map(str, self.definitions))

    def search_by_name(self, name):
        for definition in reversed(self.definitions):
            ret = definition.search_by_name(name)
            if ret != None:
                return ret
        return None
