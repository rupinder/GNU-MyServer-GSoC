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
from log import Stream
from lxml import etree

class StreamTest(unittest.TestCase):
    def test_creation(self):
        log = Stream('/logs/MyServer.log')
        log = Stream('/logs/MyServer.log', 1048576)
        log = Stream('/logs/MyServer.log', 1048576, False)
        log = Stream('/logs/MyServer.log', 1048576, False, ['gzip', 'bzip2'])

    def test_location(self):
        log = Stream('/logs/MyServer.log')
        self.assertEqual('/logs/MyServer.log', log.get_location())
        log.set_location('/logs/new.log')
        self.assertEqual('/logs/new.log', log.get_location())
        self.assertRaises(AttributeError, log.set_location, None)

    def test_cycle(self):
        log = Stream('path')
        self.assertEqual(None, log.get_cycle())
        log.set_cycle(123)
        self.assertEqual(123, log.get_cycle())
        log.set_cycle(None)
        self.assertEqual(None, log.get_cycle())
        log = Stream('path', 123)
        self.assertEqual(123, log.get_cycle())

    def test_cycle_gzip(self):
        log = Stream('path')
        self.assertEqual(None, log.get_cycle_gzip())
        log.set_cycle_gzip(True)
        self.assertEqual(True, log.get_cycle_gzip())
        log.set_cycle_gzip(None)
        self.assertEqual(None, log.get_cycle_gzip())
        log = Stream('path', 123, False)
        self.assertEqual(False, log.get_cycle_gzip())
        log = Stream('path', cycle_gzip = False)
        self.assertEqual(False, log.get_cycle_gzip())

    def test_filter(self):
        log = Stream('path')
        self.assertEqual([], log.get_filters())
        log.add_filter('gzip')
        self.assertEqual(['gzip'], log.get_filters())
        log.add_filter('bzip2')
        self.assertEqual(['gzip', 'bzip2'], log.get_filters())
        log.add_filter('bzip2', 0)
        self.assertEqual(['bzip2', 'gzip', 'bzip2'], log.get_filters())
        self.assertEqual('gzip', log.get_filter(1))
        log.remove_filter(1)
        self.assertEqual(['bzip2', 'bzip2'], log.get_filters())
        log = Stream('path', 123, False, ['gzip'])
        self.assertEqual(['gzip'], log.get_filters())
        log = Stream('path', filter = ['gzip'])
        self.assertEqual(['gzip'], log.get_filters())

    def test_from_string(self):
        text = '''<STREAM location="path" cycle="123" cycle_gzip="NO">
  <FILTER>gzip</FILTER>
  <FILTER>bzip2</FILTER>
</STREAM>'''
        log = Stream.from_string(text)
        self.assertEqual('path', log.get_location())
        self.assertEqual('123', log.get_cycle())
        self.assertFalse(log.get_cycle_gzip())
        self.assertEqual(['gzip', 'bzip2'], log.get_filters())
        text = '<STREAM location="path" />'
        log = Stream.from_string(text)
        self.assertEqual('path', log.get_location())
        self.assertEqual(None, log.get_cycle())
        self.assertEqual(None, log.get_cycle_gzip())
        self.assertEqual([], log.get_filters())

    def test_from_lxml(self):
        text = '''<STREAM location="path" cycle="123" cycle_gzip="NO">
  <FILTER>gzip</FILTER>
  <FILTER>bzip2</FILTER>
</STREAM>'''
        log = Stream.from_lxml_element(etree.XML(text))
        self.assertEqual('path', log.get_location())
        self.assertEqual('123', log.get_cycle())
        self.assertFalse(log.get_cycle_gzip())
        self.assertEqual(['gzip', 'bzip2'], log.get_filters())
        text = '<STREAM location="path" />'
        log = Stream.from_lxml_element(etree.XML(text))
        self.assertEqual('path', log.get_location())
        self.assertEqual(None, log.get_cycle())
        self.assertEqual(None, log.get_cycle_gzip())
        self.assertEqual([], log.get_filters())

    def test_bad_root_tag(self):
        text = '<ERROR location="path" />'
        self.assertRaises(AttributeError, Stream.from_string, text)
        self.assertRaises(AttributeError, Stream.from_lxml_element,
                          etree.XML(text))
    
    def test_equality(self):
        self.assertEqual(Stream('path', 123, False, ['gzip', 'bzip2']),
                         Stream('path', 123, False, ['gzip', 'bzip2']))
        self.assertNotEqual(Stream('path1', 123, False, ['gzip', 'bzip2']),
                            Stream('path2', 123, False, ['gzip', 'bzip2']))
        self.assertNotEqual(Stream('path', 1234, False, ['gzip', 'bzip2']),
                            Stream('path', 123, False, ['gzip', 'bzip2']))
        self.assertNotEqual(Stream('path', 123, True, ['gzip', 'bzip2']),
                            Stream('path', 123, False, ['gzip', 'bzip2']))
        self.assertNotEqual(Stream('path', 123, True, ['bzip2']),
                            Stream('path', 123, False, ['gzip', 'bzip2']))
        self.assertNotEqual([], Stream('path'))

    def test_to_string(self):
        text = '''<STREAM location="path" cycle="123" cycle_gzip="NO">
  <FILTER>gzip</FILTER>
  <FILTER>bzip2</FILTER>
</STREAM>'''
        log = Stream.from_string(text)
        copy = Stream.from_string(str(log))
        self.assertEqual(log, copy)

    def test_to_lxml(self):
        text = '''<STREAM location="path" cycle="123" cycle_gzip="NO">
  <FILTER>gzip</FILTER>
  <FILTER>bzip2</FILTER>
</STREAM>'''
        log = Stream.from_string(text)
        copy = Stream.from_lxml_element(log.to_lxml_element())
        self.assertEqual(log, copy)

if __name__ == '__main__':
    unittest.main()
