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

from security import SecurityElement, User, Condition, Permission, Return, SecurityList
from definition import Definition
from lxml import etree
import unittest

class SecurityElementTest(unittest.TestCase):
    def test_creation(self):
        element = SecurityElement()

    def test_from_string(self):
        text = '<USER />'
        element = SecurityElement.from_string(text)
        self.assertTrue(isinstance(element, User))
        text = '<CONDITION />'
        element = SecurityElement.from_string(text)
        self.assertTrue(isinstance(element, Condition))
        text = '<PERMISSION />'
        element = SecurityElement.from_string(text)
        self.assertTrue(isinstance(element, Permission))
        text = '<RETURN />'
        element = SecurityElement.from_string(text)
        self.assertTrue(isinstance(element, Return))
        text = '<DEFINE />'
        element = SecurityElement.from_string(text)
        self.assertTrue(isinstance(element, Definition))

class ConditionTest(unittest.TestCase):
    def test_creation(self):
        condition = Condition()
        condition = Condition('name')
        condition = Condition('name', 'value')
        condition = Condition('name', 'value', False)
        condition = Condition('name', 'value', False, False)
        condition = Condition('name', 'value', False, False, [])

    def test_name(self):
        condition = Condition('name1')
        self.assertEqual('name1', condition.get_name())
        condition.set_name('name2')
        self.assertEqual('name2', condition.get_name())
        condition.set_name(None)
        self.assertEqual(None, condition.get_name())
        condition = Condition(name = 'name1')
        self.assertEqual('name1', condition.get_name())

    def test_value(self):
        condition = Condition('name1', 'value1')
        self.assertEqual('value1', condition.get_value())
        condition.set_value('value2')
        self.assertEqual('value2', condition.get_value())
        condition.set_value(None)
        self.assertEqual(None, condition.get_value())
        condition = Condition(value = 'value1')
        self.assertEqual('value1', condition.get_value())

    def test_reverse(self):
        condition = Condition('name1', 'value1', False)
        self.assertFalse(condition.get_reverse())
        condition.set_reverse(True)
        self.assertTrue(condition.get_reverse())
        condition.set_reverse(None)
        self.assertEqual(None, condition.get_reverse())
        condition = Condition(reverse = True)
        self.assertTrue(condition.get_reverse())

    def test_regex(self):
        condition = Condition('name1', 'value1', False, False)
        self.assertFalse(condition.get_regex())
        condition.set_regex(True)
        self.assertTrue(condition.get_regex())
        condition.set_regex(None)
        self.assertEqual(None, condition.get_regex())
        condition = Condition(regex = True)
        self.assertTrue(condition.get_regex())

    def test_sub_elements(self):
        element_0 = Condition()
        element_1 = Return()
        condition = Condition()
        self.assertEqual(0, len(condition.get_sub_elements()))
        condition.add_sub_element(element_0)
        self.assertEqual(1, len(condition.get_sub_elements()))
        self.assertEqual(element_0, condition.get_sub_element(0))
        condition.add_sub_element(element_1)
        self.assertEqual(2, len(condition.get_sub_elements()))
        self.assertEqual(element_0, condition.get_sub_element(0))
        self.assertEqual(element_1, condition.get_sub_element(1))
        condition.remove_sub_element(0)
        self.assertEqual(1, len(condition.get_sub_elements()))
        self.assertEqual(element_1, condition.get_sub_element(0))
        condition.add_sub_element(element_0, 0)
        self.assertEqual(2, len(condition.get_sub_elements()))
        self.assertEqual(element_0, condition.get_sub_element(0))
        self.assertEqual(element_1, condition.get_sub_element(1))
        self.assertRaises(IndexError, condition.get_sub_element, 2)
        condition = Condition(sub_elements = [element_0, element_1])
        self.assertEqual(2, len(condition.get_sub_elements()))
        self.assertEqual(element_0, condition.get_sub_element(0))
        self.assertEqual(element_1, condition.get_sub_element(1))

    def test_equality(self):
        self.assertEqual(
            Condition('name1', 'value1', False, False, []),
            Condition('name1', 'value1', False, False, []))
        self.assertNotEqual(
            Condition('name1', 'value1', False, False, []),
            Condition('name2', 'value1', False, False, []))
        self.assertNotEqual(
            Condition('name1', 'value1', False, False, []),
            Condition('name1', 'value2', False, False, []))
        self.assertNotEqual(
            Condition('name1', 'value1', False, False, []),
            Condition('name1', 'value1', True, False, []))
        self.assertNotEqual(
            Condition('name1', 'value1', False, False, []),
            Condition('name1', 'value1', False, True, []))
        self.assertNotEqual(
            Condition('name1', 'value1', False, False, []),
            Condition('name1', 'value1', False, False, [Condition()]))
        self.assertNotEqual(Condition(), 'another type')

    def test_from_string(self):
        text = '<CONDITION />'
        condition = Condition.from_string(text)
        right = Condition()
        self.assertEqual(condition, right)

    def test_from_string_name(self):
        text = '<CONDITION name="test" />'
        condition = Condition.from_string(text)
        right = Condition(name = 'test')
        self.assertEqual(condition, right)

    def test_from_string_value(self):
        text = '<CONDITION value="test" />'
        condition = Condition.from_string(text)
        right = Condition(value = 'test')
        self.assertEqual(condition, right)

    def test_from_string_reverse(self):
        text = '<CONDITION not="yes" />'
        condition = Condition.from_string(text)
        right = Condition(reverse = True)
        self.assertEqual(condition, right)

    def test_from_string_regex(self):
        text = '<CONDITION regex="yes" />'
        condition = Condition.from_string(text)
        right = Condition(regex = True)
        self.assertEqual(condition, right)

    def test_from_string_sub_elements(self):
        text = '<CONDITION><PERMISSION /><RETURN /></CONDITION>'
        condition = Condition.from_string(text)
        right = Condition(sub_elements = [Permission(), Return()])
        self.assertEqual(condition, right)

    def test_from_string_full(self):
        text = '''
<CONDITION name="name" value="value" not="yes" regex="yes">
  <PERMISSION />
  <RETURN />
</CONDITION>'''
        condition = Condition.from_string(text)
        right = Condition('name', 'value', True, True,
                          [Permission(), Return()])
        self.assertEqual(condition, right)

    def test_from_lxml(self):
        text = '<CONDITION />'
        condition = Condition.from_lxml_element(etree.XML(text))
        right = Condition()
        self.assertEqual(condition, right)

    def test_from_lxml_name(self):
        text = '<CONDITION name="test" />'
        condition = Condition.from_lxml_element(etree.XML(text))
        right = Condition(name = 'test')
        self.assertEqual(condition, right)

    def test_from_lxml_value(self):
        text = '<CONDITION value="test" />'
        condition = Condition.from_lxml_element(etree.XML(text))
        right = Condition(value = 'test')
        self.assertEqual(condition, right)

    def test_from_lxml_reverse(self):
        text = '<CONDITION not="yes" />'
        condition = Condition.from_lxml_element(etree.XML(text))
        right = Condition(reverse = True)
        self.assertEqual(condition, right)

    def test_from_lxml_regex(self):
        text = '<CONDITION regex="yes" />'
        condition = Condition.from_lxml_element(etree.XML(text))
        right = Condition(regex = True)
        self.assertEqual(condition, right)

    def test_from_lxml_sub_elements(self):
        text = '<CONDITION><PERMISSION /><RETURN /></CONDITION>'
        condition = Condition.from_lxml_element(etree.XML(text))
        right = Condition(sub_elements = [Permission(), Return()])
        self.assertEqual(condition, right)

    def test_from_lxml_full(self):
        text = '''
<CONDITION name="name" value="value" not="yes" regex="yes">
  <PERMISSION />
  <RETURN />
</CONDITION>'''
        condition = Condition.from_lxml_element(etree.XML(text))
        right = Condition('name', 'value', True, True,
                          [Permission(), Return()])
        self.assertEqual(condition, right)

    def test_to_string(self):
        condition = Condition()
        copy = Condition.from_string(str(condition))
        self.assertEqual(condition, copy)

    def test_to_string_name(self):
        condition = Condition(name = 'name')
        copy = Condition.from_string(str(condition))
        self.assertEqual(condition, copy)

    def test_to_string_value(self):
        condition = Condition(value = 'value')
        copy = Condition.from_string(str(condition))
        self.assertEqual(condition, copy)

    def test_to_string_reverse(self):
        condition = Condition(reverse = True)
        copy = Condition.from_string(str(condition))
        self.assertEqual(condition, copy)

    def test_to_string_regex(self):
        condition = Condition(regex = True)
        copy = Condition.from_string(str(condition))
        self.assertEqual(condition, copy)

    def test_to_string_sub_elements(self):
        condition = Condition(sub_elements = [Permission(), Return()])
        copy = Condition.from_string(str(condition))
        self.assertEqual(condition, copy)

    def test_to_string_full(self):
        condition = Condition('name', 'value', True, True,
                              [Permission(), Return()])
        copy = Condition.from_string(str(condition))
        self.assertEqual(condition, copy)

    def test_to_lxml(self):
        condition = Condition()
        copy = Condition.from_lxml_element(condition.to_lxml_element())
        self.assertEqual(condition, copy)

    def test_to_lxml_name(self):
        condition = Condition(name = 'name')
        copy = Condition.from_lxml_element(condition.to_lxml_element())
        self.assertEqual(condition, copy)

    def test_to_lxml_value(self):
        condition = Condition(value = 'value')
        copy = Condition.from_lxml_element(condition.to_lxml_element())
        self.assertEqual(condition, copy)

    def test_to_lxml_reverse(self):
        condition = Condition(reverse = True)
        copy = Condition.from_lxml_element(condition.to_lxml_element())
        self.assertEqual(condition, copy)

    def test_to_lxml_regex(self):
        condition = Condition(regex = True)
        copy = Condition.from_lxml_element(condition.to_lxml_element())
        self.assertEqual(condition, copy)

    def test_to_lxml_sub_elements(self):
        condition = Condition(sub_elements = [Permission(), Return()])
        copy = Condition.from_lxml_element(condition.to_lxml_element())
        self.assertEqual(condition, copy)

    def test_to_lxml_full(self):
        condition = Condition('name', 'value', True, True,
                              [Permission(), Return()])
        copy = Condition.from_lxml_element(condition.to_lxml_element())
        self.assertEqual(condition, copy)

    def test_bad_root_tag(self):
        text = '<ERROR />'
        self.assertRaises(AttributeError, Condition.from_string, text)
        self.assertRaises(AttributeError, Condition.from_lxml_element,
                          etree.XML(text))

class BadMarkupTest(unittest.TestCase):
    def condition_test(self):
        text = '''
<CONDITION custom="unknown">
  <PERMISSION />
  <UNKNOWN>
    <CUSTOM />
  </UNKNOWN>
</CONDITION>'''
        condition = Condition.from_string(text)
        tree = condition.to_lxml_element()
        self.assertEqual('unknown', tree.get('custom'))
        unknown = tree.findall('UNKNOWN')
        self.assertEqual(1, len(unknown))
        unknown = unknown[0]
        custom = list(unknown)
        self.assertEqual(1, len(custom))
        custom = custom[0]
        self.assertEqual('CUSTOM', custom.tag)

if __name__ == '__main__':
    unittest.main()
