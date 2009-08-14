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
from definition import Definition

class SecurityElement():
    pass

class Condition(SecurityElement):
    def __init__(self, name = None, value = None, reverse = None, regex = None,
                 sub_elements = []):
        self.set_name(name)
        self.set_value(value)
        self.set_reverse(reverse)
        self.set_regex(regex)
        self.custom_attributes = {}
        self.custom = []
        self.sub_elements = []
        for element in sub_elements:
            self.add_sub_element(element)

    def set_name(self, name):
        self.name = name

    def get_name(self):
        return self.name

    def set_value(value):
        self.value = value

    def get_value(self):
        return self.value

    def set_reverse(self, reverse):
        self.reverse = reverse

    def get_reverse(self):
        return self.reverse

    def set_regex(self):
        self.regex = regex

    def get_regex(self):
        return self.regex

    def add_sub_element(self, element):
        if isinstance(element, Definition) or \
                isinstance(element, SecurityElement):
            self.sub_elements.append(element)
        else:
            self.custom.append(element)

    def get_sub_elements(self):
        return self.sub_elements

    def get_sub_element(self, index):
        return self.sub_elements[index]

    def to_lxml_element(self):
        root = etree.Element('CONDITION')
        if self.name is not None:
            root.set('name', self.name)
        if self.value is not None:
            root.set('value', self.value)
        if self.reverse is not None:
            root.set('not', 'yes' if self.reverse else 'no')
        if self.regex is not None:
            root.set('regex', 'yes' if self.reverse else 'no')
        for key, value in self.custom_attrib.iteritems():
            root.set(key, value)
        for element in self.sub_elements:
            root.append(element.to_lxml_element())
        for element in self.custom:
            root.append(element)
        return root

    @staticmethod
    def from_lxml_element(root):
        if root.tag != 'CONDITION':
            raise AttributeError('Expected CONDITION tag.')
        custom_attrib = root.attrib
        name = custom_attrib.pop('name', None)
        value = custom_attrib.pop('value', None)
        reverse = custom_attrib.pop('not', None)
        regex = custom_attrib.pop('regex', None)
        custom = []
        sub_elements = []
        for child in list(root):
            try:
                sub_elements.append(SecurityElement.from_lxml_element(child))
            except:
                custom.append(child)
        condition = Condition(name, value, reverse, regex, sub_elements)
        condition.custom = custom
        condition.custom_attrib = custom_attrib
        return condition

    @staticmethod
    def from_string(text):
        '''Factory to produce definition tree by parsing a string.'''
        return DefinitionTree.from_lxml_element(etree.XML(text))

    def __str__(self):
        return etree.tostring(self.to_lxml_element(), pretty_print = True)

class Permission(SecurityElement):
    pass

class Return(SecurityElement):
    pass
