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
        definition = Definition('no attributes')
        definition = Definition('with attributes', {'a': 'b', 'c': 'd'})

    def test_name(self):
        definition = Definition('name1')
        self.assertEqual('name1', definition.get_name())
        definition.set_name('name2')
        self.assertEqual('name2', definition.get_name())
        definition.set_name(None)
        self.assertEqual(None, definition.get_name())
        definition = Definition(name = 'name1')
        self.assertEqual('name1', definition.get_name())

    def test_attributes(self):
        definition = Definition('with attributes', {'a': 'b', 'c': 'd'})
        self.assertEqual('b', definition.get_attribute('a'))
        self.assertEqual('d', definition.get_attribute('c'))
        self.assertRaises(KeyError, definition.get_attribute, 'e')
        definition.set_attribute('e', 'f')
        self.assertEqual('f', definition.get_attribute('e'))
        definition.set_attribute('e', 'g')
        self.assertEqual('g', definition.get_attribute('e'))
        definition.remove_attribute('e')
        self.assertRaises(KeyError, definition.get_attribute, 'e')
        self.assertRaises(KeyError, definition.remove_attribute, 'e')
        definition = Definition(attributes = {'a': 'b'})
        self.assertEqual('b', definition.get_attribute('a'))
        self.assertRaises(KeyError, definition.get_attribute, 'e')

    def test_value(self):
        self.assertRaises(KeyError, Definition, 'name', {'value': 'x'})
        definition = Definition('name')
        self.assertRaises(KeyError, definition.set_attribute, 'value', 'x')

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

    def test_bad_root_tag(self):
        text = '<ERROR name="http.error.file.404" value="404.html" />'
        self.assertRaises(AttributeError, Definition.from_string, text)
        self.assertRaises(AttributeError, Definition.from_lxml_element,
                          etree.XML(text))

    def test_equality(self):
        self.assertNotEqual(Definition(), 'different type')
    
class DefinitionElementTest(unittest.TestCase):
    def test_creation(self):
        definition = DefinitionElement()
        self.assertTrue(isinstance(definition, Definition))
        definition = DefinitionElement('name')
        self.assertTrue(isinstance(definition, Definition))
        definition = DefinitionElement('name', {'value': 'value'})
        self.assertTrue(isinstance(definition, Definition))

    def test_value(self):
        definition = DefinitionElement('name', {'value': 'x'})
        self.assertEqual('x', definition.get_attribute('value'))
        definition.set_attribute('value', 'y')
        self.assertEqual('y', definition.get_attribute('value'))
        definition.set_attribute('value', None)
        self.assertEqual(None, definition.get_attribute('value'))
        definition.remove_attribute('value')
        self.assertRaises(KeyError, definition.get_attribute, 'value')
        definition = DefinitionElement()
        self.assertRaises(KeyError, definition.get_attribute, 'value')

    def test_from_string(self):
        text = '<DEFINE />'
        definition = DefinitionElement.from_string(text)
        right = DefinitionElement()
        self.assertEqual(definition, right)

    def test_from_string_name(self):
        text = '<DEFINE name="test" />'
        definition = DefinitionElement.from_string(text)
        right = DefinitionElement(name = "test")
        self.assertEqual(definition, right)

    def test_from_string_attributes(self):
        text = '<DEFINE a="b" b="c" value="x" />'
        definition = DefinitionElement.from_string(text)
        right = DefinitionElement(attributes = {'a': 'b', 'b': 'c', 'value': 'x'})
        self.assertEqual(definition, right)

    def test_from_string_full(self):
        text = '<DEFINE name="test" value="x" a="b" />'
        definition = DefinitionElement.from_string(text)
        right = DefinitionElement('test', {'value': 'x', 'a': 'b'})
        self.assertEqual(definition, right)
        
    def test_from_lxml(self):
        text = '<DEFINE />'
        definition = DefinitionElement.from_lxml_element(etree.XML(text))
        right = DefinitionElement()
        self.assertEqual(definition, right)

    def test_from_xml_name(self):
        text = '<DEFINE name="test" />'
        definition = DefinitionElement.from_lxml_element(etree.XML(text))
        right = DefinitionElement(name = "test")
        self.assertEqual(definition, right)

    def test_from_lxml_attributes(self):
        text = '<DEFINE a="b" b="c" value="x" />'
        definition = DefinitionElement.from_lxml_element(etree.XML(text))
        right = DefinitionElement(attributes = {'a': 'b', 'b': 'c', 'value': 'x'})
        self.assertEqual(definition, right)

    def test_from_lxml_full(self):
        text = '<DEFINE name="test" value="x" a="b" />'
        definition = DefinitionElement.from_lxml_element(etree.XML(text))
        right = DefinitionElement('test', {'value': 'x', 'a': 'b'})
        self.assertEqual(definition, right)

    def test_equality(self):
        self.assertEqual(DefinitionElement('name', {'some': 'attributes'}),
                         DefinitionElement('name', {'some': 'attributes'}))
        self.assertNotEqual(DefinitionElement('name1'),
                            DefinitionElement('name2'))
        self.assertNotEqual(DefinitionElement('name', {'attribute': '1'}),
                            DefinitionElement('name', {'attribute': '2'}))
        self.assertNotEqual(DefinitionElement(), 'different type')

    def test_to_string(self):
        definition = DefinitionElement()
        copy = DefinitionElement.from_string(str(definition))
        self.assertEqual(definition, copy)
    
    def test_to_string_name(self):
        definition = DefinitionElement(name = 'test')
        copy = DefinitionElement.from_string(str(definition))
        self.assertEqual(definition, copy)

    def test_to_string_attributes(self):
        definition = DefinitionElement(attributes = {'a': 'b', 'b': 'c', 'value': 'x'})
        copy = DefinitionElement.from_string(str(definition))
        self.assertEqual(definition, copy)

    def test_to_string_full(self):
        definition = DefinitionElement('test', {'value': 'x', 'a': 'b'})
        copy = DefinitionElement.from_string(str(definition))
        self.assertEqual(definition, copy)

    def test_to_lxml(self):
        definition = DefinitionElement()
        copy = DefinitionElement.from_lxml_element(definition.to_lxml_element())
        self.assertEqual(definition, copy)
    
    def test_to_lxml_name(self):
        definition = DefinitionElement(name = 'test')
        copy = DefinitionElement.from_lxml_element(definition.to_lxml_element())
        self.assertEqual(definition, copy)

    def test_to_lxml_attributes(self):
        definition = DefinitionElement(attributes = {'a': 'b', 'b': 'c', 'value': 'x'})
        copy = DefinitionElement.from_lxml_element(definition.to_lxml_element())
        self.assertEqual(definition, copy)

    def test_to_lxml_full(self):
        definition = DefinitionElement('test', {'value': 'x', 'a': 'b'})
        copy = DefinitionElement.from_lxml_element(definition.to_lxml_element())
        self.assertEqual(definition, copy)
    
    def test_bad_root_tag(self):
        text = '<ERROR name="http.error.file.404" value="404.html" />'
        self.assertRaises(AttributeError, Definition.from_string, text)
        self.assertRaises(AttributeError, Definition.from_lxml_element,
                          etree.XML(text))

class DefinitionTreeTest(unittest.TestCase):
    def setUp(self):
        self.element_0 = DefinitionElement('test', {'value': 'x'})
        self.element_1 = DefinitionElement(attributes = {'a': 'b', 'b': 'c'})

    def test_creation(self):
        definition = DefinitionTree()
        self.assertTrue(isinstance(definition, Definition))
        definition = DefinitionTree('name')
        self.assertTrue(isinstance(definition, Definition))
        definition = DefinitionTree('name', [self.element_0])
        self.assertTrue(isinstance(definition, Definition))
        definition = DefinitionTree('name', [self.element_1], {'key': 'value'})
        self.assertTrue(isinstance(definition, Definition))

    def test_value(self):
        self.assertRaises(KeyError, DefinitionTree, 'name', [], {'value': 'x'})
        definition = Definition('name')
        self.assertRaises(KeyError, definition.set_attribute, 'value', 'x')

    def test_definitions(self):
        definition = DefinitionTree()
        self.assertEqual(0, len(definition.get_definitions()))
        definition.add_definition(self.element_0)
        self.assertEqual(1, len(definition.get_definitions()))
        self.assertEqual(self.element_0, definition.get_definition(0))
        definition.add_definition(self.element_1)
        self.assertEqual(2, len(definition.get_definitions()))
        self.assertEqual(self.element_0, definition.get_definition(0))
        self.assertEqual(self.element_1, definition.get_definition(1))
        definition.remove_definition(0)
        self.assertEqual(1, len(definition.get_definitions()))
        self.assertEqual(self.element_1, definition.get_definition(0))
        definition.add_definition(self.element_0, 0)
        self.assertEqual(2, len(definition.get_definitions()))
        self.assertEqual(self.element_0, definition.get_definition(0))
        self.assertEqual(self.element_1, definition.get_definition(1))
        self.assertRaises(IndexError, definition.get_definition, 2)
        definition = DefinitionTree(definitions = [self.element_0, self.element_1])
        self.assertEqual(2, len(definition.get_definitions()))
        self.assertEqual(self.element_0, definition.get_definition(0))
        self.assertEqual(self.element_1, definition.get_definition(1))

    def test_from_string(self):
        text = '<DEFINE />'
        definition = DefinitionTree.from_string(text)
        right = DefinitionTree()
        self.assertEqual(definition, right)

    def test_from_string_name(self):
        text = '<DEFINE name="test" />'
        definition = DefinitionTree.from_string(text)
        right = DefinitionTree(name = "test")
        self.assertEqual(definition, right)

    def test_from_string_definitions(self):
        text = '<DEFINE>{0}{1}</DEFINE>'.format(self.element_0, self.element_1)
        definition = DefinitionTree.from_string(text)
        right = DefinitionTree(definitions = [self.element_0, self.element_1])
        self.assertEqual(definition, right)

    def test_from_string_attributes(self):
        text = '<DEFINE a="b" b="c" />'
        definition = DefinitionTree.from_string(text)
        right = DefinitionTree(attributes = {'a': 'b', 'b': 'c'})
        self.assertEqual(definition, right)

    def test_from_string_full(self):
        text = '<DEFINE name="test" a="b">{0}{1}</DEFINE>'.format(
            self.element_0, self.element_1)
        definition = DefinitionTree.from_string(text)
        right = DefinitionTree('test', [self.element_0, self.element_1], {'a': 'b'})
        self.assertEqual(definition, right)

    def test_from_lxml(self):
        text = '<DEFINE />'
        definition = DefinitionTree.from_lxml_element(etree.XML(text))
        right = DefinitionTree()
        self.assertEqual(definition, right)

    def test_from_lxml_name(self):
        text = '<DEFINE name="test" />'
        definition = DefinitionTree.from_lxml_element(etree.XML(text))
        right = DefinitionTree(name = "test")
        self.assertEqual(definition, right)

    def test_from_lxml_definitions(self):
        text = '<DEFINE>{0}{1}</DEFINE>'.format(self.element_0, self.element_1)
        definition = DefinitionTree.from_lxml_element(etree.XML(text))
        right = DefinitionTree(definitions = [self.element_0, self.element_1])
        self.assertEqual(definition, right)

    def test_from_lxml_attributes(self):
        text = '<DEFINE a="b" b="c" />'
        definition = DefinitionTree.from_lxml_element(etree.XML(text))
        right = DefinitionTree(attributes = {'a': 'b', 'b': 'c'})
        self.assertEqual(definition, right)

    def test_from_lxml_full(self):
        text = '<DEFINE name="test" a="b">{0}{1}</DEFINE>'.format(
            self.element_0, self.element_1)
        definition = DefinitionTree.from_lxml_element(etree.XML(text))
        right = DefinitionTree('test', [self.element_0, self.element_1], {'a': 'b'})
        self.assertEqual(definition, right)

    def test_equality(self):
        definition_0 = DefinitionTree('test', [self.element_0], {'a': 'b'})
        definition_1 = DefinitionTree('test', [self.element_0], {'a': 'b'})
        self.assertEqual(definition_0, definition_1)
        self.assertNotEqual(definition_0, DefinitionTree('', [self.element_0], {'a': 'b'}))
        self.assertNotEqual(definition_0, DefinitionTree('name', [], {'a': 'b'}))
        self.assertNotEqual(definition_0, DefinitionTree('name', [self.element_0], {'b': 'b'}))
        self.assertNotEqual(DefinitionTree(), 'different type')

    def test_to_string(self):
        definition = DefinitionTree()
        copy = DefinitionTree.from_string(str(definition))
        self.assertEqual(definition, copy)

    def test_to_string_name(self):
        definition = DefinitionTree(name = "test")
        copy = DefinitionTree.from_string(str(definition))
        self.assertEqual(definition, copy)

    def test_top_string_definitions(self):
        definition = DefinitionTree(definitions = [self.element_0, self.element_1])
        copy = DefinitionTree.from_string(str(definition))
        self.assertEqual(definition, copy)

    def test_top_string_attributes(self):
        definition = DefinitionTree(attributes = {'a': 'b', 'b': 'c'})
        copy = DefinitionTree.from_string(str(definition))
        self.assertEqual(definition, copy)

    def test_to_string_full(self):
        definition = DefinitionTree('test', [self.element_0, self.element_1], {'a': 'b'})
        copy = DefinitionTree.from_string(str(definition))
        self.assertEqual(definition, copy)
        
    def test_to_lxml(self):
        definition = DefinitionTree()
        copy = DefinitionTree.from_lxml_element(definition.to_lxml_element())
        self.assertEqual(definition, copy)

    def test_to_lxml_name(self):
        definition = DefinitionTree(name = "test")
        copy = DefinitionTree.from_lxml_element(definition.to_lxml_element())
        self.assertEqual(definition, copy)

    def test_top_lxml_definitions(self):
        definition = DefinitionTree(definitions = [self.element_0, self.element_1])
        copy = DefinitionTree.from_lxml_element(definition.to_lxml_element())
        self.assertEqual(definition, copy)

    def test_top_lxml_attributes(self):
        definition = DefinitionTree(attributes = {'a': 'b', 'b': 'c'})
        copy = DefinitionTree.from_lxml_element(definition.to_lxml_element())
        self.assertEqual(definition, copy)

    def test_to_lxml_full(self):
        definition = DefinitionTree('test', [self.element_0, self.element_1], {'a': 'b'})
        copy = DefinitionTree.from_lxml_element(definition.to_lxml_element())
        self.assertEqual(definition, copy)

    def test_bad_root_tag(self):
        text = '<ERROR name="test" key="value">{0}{1}</ERROR>'.format(
            self.element_0, self.element_1)
        self.assertRaises(AttributeError, Definition.from_string, text)
        self.assertRaises(AttributeError, Definition.from_lxml_element,
                          etree.XML(text))

if __name__ == '__main__':
    unittest.main()
