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

from definition import Definition, DefinitionElement, DefinitionTree
from lxml import etree
import unittest

class DefinitionTest(unittest.TestCase):
    def test_creation(self):
        definition = Definition()
        self.assertEqual(None, definition.get_name())
        self.assertEqual({}, definition.attributes)
        definition = Definition('no attributes')
        self.assertEqual('no attributes', definition.get_name())
        self.assertEqual({}, definition.attributes)
        definition = Definition('with attributes', {'a': 'b', 'c': 'd'})
        self.assertEqual('with attributes', definition.get_name())
        self.assertEqual({'a': 'b', 'c': 'd'}, definition.attributes)

    def test_name(self):
        definition = Definition('name1')
        self.assertEqual('name1', definition.get_name())
        definition.set_name('name2')
        self.assertEqual('name2', definition.get_name())

    def test_attributes(self):
        definition = Definition('with attributes', {'a': 'b', 'c': 'd'})
        self.assertEqual('b', definition.get_attribute('a'))
        self.assertEqual('d', definition.get_attribute('c'))
        self.assertEqual(None, definition.get_attribute('e'))
        definition.set_attribute('e', 'f')
        self.assertEqual('f', definition.get_attribute('e'))
        definition.set_attribute('e', 'g')
        self.assertEqual('g', definition.get_attribute('e'))
        definition.remove_attribute('e')
        self.assertEqual(None, definition.get_attribute('e'))
        self.assertRaises(KeyError, definition.remove_attribute, 'e')

    def test_from_string(self):
        text = '<DEFINE name="http.error.file.404" value="404.html" />'
        definition = Definition.from_string(text)
        self.assertTrue(isinstance(definition, DefinitionElement))
        text = '<DEFINE name="x"><DEFINE value="y" /><DEFINE value="z" /></DEFINE>'
        definition = Definition.from_string(text)
        self.assertTrue(isinstance(definition, DefinitionTree))

    def test_from_lxml(self):
        text = '<DEFINE name="http.error.file.404" value="404.html" />'
        definition = Definition.from_lxml_element(etree.XML(text))
        self.assertTrue(isinstance(definition, DefinitionElement))
        text = '<DEFINE name="x"><DEFINE value="y" /><DEFINE value="z" /></DEFINE>'
        definition = Definition.from_lxml_element(etree.XML(text))
        self.assertTrue(isinstance(definition, DefinitionTree))

class DefinitionElementTest(unittest.TestCase):
    def test_creation(self):
        definition = DefinitionElement()
        self.assertTrue(isinstance(definition, Definition))
        definition = DefinitionElement('name')
        self.assertTrue(isinstance(definition, Definition))
        definition = DefinitionElement('name', {'value': 'value'})
        self.assertTrue(isinstance(definition, Definition))

    def test_from_string(self):
        text = '<DEFINE name="http.error.file.404" value="404.html" />'
        definition = DefinitionElement.from_string(text)
        self.assertEqual('http.error.file.404', definition.get_name())
        self.assertEqual('404.html', definition.get_attribute('value'))
        text = '''<DEFINE server="/opt/bin/fastcgi_server" domain="fastcgi"
host="localhost" port="2010" local="yes" uid="1000" gid="1000" />'''
        definition = DefinitionElement.from_string(text)
        self.assertEqual(None, definition.get_attribute('value'))
        self.assertEqual(None, definition.get_name())
        self.assertEqual('/opt/bin/fastcgi_server',
                         definition.get_attribute('server'))
        self.assertEqual('fastcgi', definition.get_attribute('domain'))
        self.assertEqual('localhost', definition.get_attribute('host'))
        self.assertEqual('2010', definition.get_attribute('port'))
        self.assertEqual('yes', definition.get_attribute('local'))
        self.assertEqual('1000', definition.get_attribute('uid'))
        self.assertEqual('1000', definition.get_attribute('gid'))

    def test_from_lxml(self):
        root = etree.Element('DEFINE')
        root.set('server', '/opt/bin/fastcgi_server')
        root.set('domain', 'fastcgi')
        root.set('host', 'localhost')
        root.set('port', '2010')
        root.set('local', 'yes')
        root.set('uid', '1000')
        root.set('gid', '1000')
        definition = DefinitionElement.from_lxml_element(root)
        self.assertEqual(None, definition.get_attribute('value'))
        self.assertEqual(None, definition.get_name())
        self.assertEqual('/opt/bin/fastcgi_server',
                         definition.get_attribute('server'))
        self.assertEqual('fastcgi', definition.get_attribute('domain'))
        self.assertEqual('localhost', definition.get_attribute('host'))
        self.assertEqual('2010', definition.get_attribute('port'))
        self.assertEqual('yes', definition.get_attribute('local'))
        self.assertEqual('1000', definition.get_attribute('uid'))
        self.assertEqual('1000', definition.get_attribute('gid'))

    def test_equality(self):
        self.assertEqual(DefinitionElement('name', {'some': 'attributes'}),
                         DefinitionElement('name', {'some': 'attributes'}))
        self.assertNotEqual(DefinitionElement('name1'),
                            DefinitionElement('name2'))
        self.assertNotEqual(DefinitionElement('name', {'attribute': '1'}),
                            DefinitionElement('name', {'attribute': '2'}))

    def test_to_string(self):
        text = '''<DEFINE server="/opt/bin/fastcgi_server" domain="fastcgi"
host="localhost" port="2010" local="yes" uid="1000" gid="1000" />'''
        definition = DefinitionElement.from_string(text)
        copy = DefinitionElement.from_string(str(definition))
        self.assertEqual(definition, copy)

    def test_to_lxml(self):
        text = '''<DEFINE name="test" server="/opt/bin/fastcgi_server"
domain="fastcgi" host="localhost" port="2010" local="yes" uid="1000" gid="1000"
/>'''
        definition = DefinitionElement.from_string(text)
        copy = DefinitionElement.from_lxml_element(definition.to_lxml_element())
        self.assertEqual(definition, copy)

class DefinitionTreeTest(unittest.TestCase):
    element_1 = DefinitionElement('http.error.file.404', {'value': '404.html'})
    element_2 = DefinitionElement(attributes = {'server': '/opt/bin/fastcgi_server',
                                                'domain': 'fastcgi', 'host': 'localhost',
                                                'port': '2010', 'local': 'yes',
                                                'uid': '1000', 'gid': '1000'})
    def test_creation(self):
        definition = DefinitionTree()
        self.assertTrue(isinstance(definition, Definition))
        definition = DefinitionTree('name')
        self.assertTrue(isinstance(definition, Definition))
        definition = DefinitionTree('name', [DefinitionTreeTest.element_1])
        self.assertTrue(isinstance(definition, Definition))
        definition = DefinitionTree('name', [DefinitionTreeTest.element_2], {'value': '1'})
        self.assertTrue(isinstance(definition, Definition))

    def test_values(self):
        definition = DefinitionTree()
        self.assertEqual(0, len(definition.get_values()))
        definition.add_value(DefinitionTreeTest.element_1)
        self.assertEqual(1, len(definition.get_values()))
        self.assertEqual(DefinitionTreeTest.element_1, definition.get_value(0))
        definition.add_value(DefinitionTreeTest.element_2)
        self.assertEqual(2, len(definition.get_values()))
        self.assertEqual(DefinitionTreeTest.element_1, definition.get_value(0))
        self.assertEqual(DefinitionTreeTest.element_2, definition.get_value(1))
        definition.remove_value(0)
        self.assertEqual(1, len(definition.get_values()))
        self.assertEqual(DefinitionTreeTest.element_2, definition.get_value(0))
        definition.add_value(DefinitionTreeTest.element_1, 0)
        self.assertEqual(2, len(definition.get_values()))
        self.assertEqual(DefinitionTreeTest.element_1, definition.get_value(0))
        self.assertEqual(DefinitionTreeTest.element_2, definition.get_value(1))

    def test_from_string(self):
        text = '<DEFINE name="test" key="value">{0}{1}</DEFINE>'.format(
            DefinitionTreeTest.element_1, DefinitionTreeTest.element_2)
        definition = DefinitionTree.from_string(text)
        self.assertEqual('test', definition.get_name())
        self.assertEqual('value', definition.get_attribute('key'))
        definitions = definition.get_values()
        self.assertEqual(2, len(definitions))
        self.assertEqual(DefinitionTreeTest.element_1, definitions[0])
        self.assertEqual(DefinitionTreeTest.element_2, definitions[1])

    def test_from_lxml(self):
        root = etree.Element('DEFINE')
        root.set('name', 'test')
        root.set('key', 'value')
        root.append(DefinitionTreeTest.element_1.to_lxml_element())
        root.append(DefinitionTreeTest.element_2.to_lxml_element())
        definition = DefinitionTree.from_lxml_element(root)
        self.assertEqual('test', definition.get_name())
        self.assertEqual('value', definition.get_attribute('key'))
        definitions = definition.get_values()
        self.assertEqual(2, len(definitions))
        self.assertEqual(DefinitionTreeTest.element_1, definitions[0])
        self.assertEqual(DefinitionTreeTest.element_2, definitions[1])

    def test_equality(self):
        text = '<DEFINE name="test" key="value">{0}{1}</DEFINE>'.format(
            DefinitionTreeTest.element_1, DefinitionTreeTest.element_2)
        self.assertEqual(DefinitionTree.from_string(text), DefinitionTree.from_string(text))
        self.assertNotEqual(DefinitionTree.from_string(text), DefinitionTree('test'))

    def test_to_string(self):
        text = '<DEFINE name="test" key="value">{0}{1}</DEFINE>'.format(
            DefinitionTreeTest.element_1, DefinitionTreeTest.element_2)
        definition = DefinitionTree.from_string(text)
        copy = DefinitionTree.from_string(str(definition))
        self.assertEqual(definition, copy)

    def test_to_lxml(self):
        text = '<DEFINE name="test" key="value">{0}{1}</DEFINE>'.format(
            DefinitionTreeTest.element_1, DefinitionTreeTest.element_2)
        definition = DefinitionTree.from_string(text)
        copy = DefinitionTree.from_lxml_element(definition.to_lxml_element())
        self.assertEqual(definition, copy)

if __name__ == '__main__':
    unittest.main()
