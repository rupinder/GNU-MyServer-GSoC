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

import unittest
from lxml import etree
from config import MyServerConfig
from definition import DefinitionElement, DefinitionTree

class MyServerConfigTest(unittest.TestCase):
    def setUp(self):
        self.definitions = []
        self.definitions.append(DefinitionElement('a'))
        self.definitions.append(DefinitionElement('b'))
        self.definitions.append(DefinitionElement('c'))

    def test_creation(self):
        config = MyServerConfig()
        config = MyServerConfig(self.definitions)

    def test_definitions(self):
        config = MyServerConfig()
        self.assertEqual([], config.get_definitions())
        counter = 0
        for definition in self.definitions:
            config.add_definition(definition)
            counter += 1
            self.assertEqual(self.definitions[:counter],
                             config.get_definitions())
        for counter in xrange(len(self.definitions)):
            self.assertEqual(self.definitions[counter],
                             config.get_definition(counter))
        config.add_definition(DefinitionElement('test'), 1)
        self.assertEqual(DefinitionElement('test'), config.get_definition(1))
        config.remove_definition(1)
        self.assertEqual(self.definitions, config.get_definitions())
        for counter in xrange(len(self.definitions)):
            config.remove_definition(0)
            self.assertEqual(self.definitions[counter + 1:],
                             config.get_definitions())

    def test_equality(self):
        self.assertEqual(MyServerConfig(self.definitions),
                         MyServerConfig(self.definitions))
        self.assertNotEqual(MyServerConfig(), MyServerConfig(self.definitions))
        self.assertNotEqual(MyServerConfig(), 'other type')

    def test_from_string(self):
        text = '<MYSERVER />'
        config = MyServerConfig.from_string(text)
        right = MyServerConfig()
        self.assertEqual(config, right)

    def test_from_string_definitions(self):
        text = '''<MYSERVER>
  <DEFINE name="a" />
  <DEFINE>
    <DEFINE name="b" />
    <DEFINE name="c" />
  </DEFINE>
</MYSERVER>'''
        config = MyServerConfig.from_string(text)
        right = MyServerConfig([DefinitionElement('a'), DefinitionTree(
                    definitions = [DefinitionElement('b'),
                                   DefinitionElement('c')])])
        self.assertEqual(config, right)

    def test_from_lxml(self):
        text = '<MYSERVER />'
        config = MyServerConfig.from_lxml_element(etree.XML(text))
        right = MyServerConfig()
        self.assertEqual(config, right)

    def test_from_lxml_definitions(self):
        text = '''<MYSERVER>
  <DEFINE name="a" />
  <DEFINE>
    <DEFINE name="b" />
    <DEFINE name="c" />
  </DEFINE>
</MYSERVER>'''
        config = MyServerConfig.from_lxml_element(etree.XML(text))
        right = MyServerConfig([DefinitionElement('a'), DefinitionTree(
                    definitions = [DefinitionElement('b'),
                                   DefinitionElement('c')])])
        self.assertEqual(config, right)

    def test_to_string(self):
        config = MyServerConfig()
        copy = MyServerConfig.from_string(str(config))
        self.assertEqual(config, copy)

    def test_to_string_definitions(self):
        config = MyServerConfig([DefinitionElement('a'), DefinitionTree(
                    definitions = [DefinitionElement('b'),
                                   DefinitionElement('c')])])
        copy = MyServerConfig.from_string(str(config))
        self.assertEqual(config, copy)

    def test_to_string(self):
        config = MyServerConfig()
        copy = MyServerConfig.from_lxml_element(config.to_lxml_element())
        self.assertEqual(config, copy)

    def test_to_string_definitions(self):
        config = MyServerConfig([DefinitionElement('a'), DefinitionTree(
                    definitions = [DefinitionElement('b'),
                                   DefinitionElement('c')])])
        copy = MyServerConfig.from_lxml_element(config.to_lxml_element())
        self.assertEqual(config, copy)

    def test_bad_root_tag(self):
        text = '<ERROR />'
        self.assertRaises(AttributeError, MyServerConfig.from_string, text)
        self.assertRaises(AttributeError, MyServerConfig.from_lxml_element,
                          etree.XML(text))


if __name__ == '__main__':
    unittest.main()
