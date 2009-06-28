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
from mimetypes import MIMEType, MIMETypes
from definition import Definition

class MIMETypeTest(unittest.TestCase):
    def setUp(self):
        self.definitions = []
        self.definitions.append(Definition.from_string(
                '<DEFINE name="http.use_error_file" value="YES" />'))
        self.definitions.append(Definition.from_string(
                '<DEFINE name="http.error.file.404" value="404.html" />'))
        self.definitions.append(Definition.from_string(
                '''<DEFINE name="http.default_file">
  <DEFINE value="index.html" />
  <DEFINE value="index.htm" />
</DEFINE>'''))
        self.text = \
            '''<MIME mime="application/xhtml+xml" handler="CGI" param="/usr/bin/python" self="NO" >
  <EXTENSION value="xhtml" />
  <EXTENSION value="xml" />
  <EXTENSION value="py" />
  <FILTER value="gzip" />
  <FILTER value="bzip2" />
  {0}
  <PATH regex="^/cgi-bin/python/.*$" />
</MIME>'''.format('\n'.join(map(lambda element: str(element),
                                self.definitions)))
    
    def test_from_string(self):
        mime = MIMEType.from_string(self.text)
        self.assertEqual(mime.get_mime(), 'application/xhtml+xml')
        self.assertEqual(mime.get_handler(), 'CGI')
        self.assertEqual(mime.get_param(), '/usr/bin/python')
        self.assertEqual(mime.get_extensions(), set(['xhtml', 'xml', 'py']))
        self.assertEqual(mime.get_path(), '^/cgi-bin/python/.*$')
        self.assertEqual(mime.get_filters(), ['gzip', 'bzip2'])
        self.assertEqual(mime.get_self_executed(), False)
        self.assertEqual(mime.get_definitions(), self.definitions)

    def test_from_lxml(self):
        mime = MIMEType.from_lxml_element(etree.XML(self.text))
        self.assertEqual(mime.get_mime(), 'application/xhtml+xml')
        self.assertEqual(mime.get_handler(), 'CGI')
        self.assertEqual(mime.get_param(), '/usr/bin/python')
        self.assertEqual(mime.get_extensions(), set(['xhtml', 'xml', 'py']))
        self.assertEqual(mime.get_path(), '^/cgi-bin/python/.*$')
        self.assertEqual(mime.get_filters(), ['gzip', 'bzip2'])
        self.assertEqual(mime.get_self_executed(), False)
        self.assertEqual(mime.get_definitions(), self.definitions)

    def test_handler(self):
        mime = MIMEType.from_string(self.text)
        for handler in MIMEType.valid_handlers:
            mime.set_handler(handler)
        self.assertRaises(AttributeError, mime.set_handler, 'ERROR')
        
    def test_attributes(self):
        mime = MIMEType.from_string(self.text)
        mime.set_mime('plain/text')
        self.assertEqual(mime.get_mime(), 'plain/text')
        mime.set_handler('SEND')
        self.assertEqual(mime.get_handler(), 'SEND')
        mime.set_param('')
        self.assertEqual(mime.get_param(), '')
        mime.remove_extension('py')
        self.assertEqual(mime.get_extensions(), set(['xhtml', 'xml']))
        mime.add_extension('pyc')
        self.assertEqual(mime.get_extensions(), set(['xhtml', 'xml', 'pyc']))
        mime.set_path('^/python/')
        self.assertEqual(mime.get_path(), '^/python/')
        mime.remove_filter(0)
        self.assertEqual(mime.get_filters(), ['bzip2'])
        mime.add_filter('gzip')
        self.assertEqual(mime.get_filters(), ['bzip2', 'gzip'])
        mime.add_filter('gzip', 0)
        self.assertEqual(mime.get_filters(), ['gzip', 'bzip2', 'gzip'])
        self.assertEqual(mime.get_filter(1), 'bzip2')
        mime.set_self_executed(True)
        self.assertEqual(mime.get_self_executed(), True)
        definition = Definition.from_string(
            '<DEFINE name="http.error.file.404" value="404.html" />')
        mime.add_definition(definition)
        self.definitions.append(definition)
        self.assertEqual(mime.get_definitions(), self.definitions)
        mime.add_definition(definition, 0)
        self.definitions.insert(0, definition)
        self.assertEqual(mime.get_definitions(), self.definitions)
        mime.remove_definition(1)
        self.definitions.pop(1)
        self.assertEqual(mime.get_definitions(), self.definitions)
        self.assertEqual(mime.get_definition(1), self.definitions[1])
        
    def test_equality(self):
        self.assertEqual(MIMEType.from_string(self.text),
                         MIMEType.from_string(self.text))
        self.assertNotEqual(MIMEType.from_string(self.text),
                            MIMEType('text/plain', 'SEND'))
        
    def test_to_string(self):
        mime = MIMEType.from_string(self.text)
        copy = MIMEType.from_string(str(mime))
        self.assertEqual(mime, copy)
        
    def test_to_lxml(self):
        mime = MIMEType.from_string(self.text)
        copy = MIMEType.from_lxml_element(mime.to_lxml_element())
        self.assertEqual(mime, copy)
        

    def test_bad_root_tag(self):
        text = '''<ERROR mime="application/xhtml+xml" handler="CGI" param="/usr/bin/python" self="NO" >
  <EXTENSION value="xhtml" />
</ERROR>'''
        self.assertRaises(AttributeError, MIMEType.from_string, text)
        self.assertRaises(AttributeError, MIMEType.from_lxml_element,
                          etree.XML(text))
        
class MIMETypesTest(unittest.TestCase):
    def setUp(self):
        self.mime_0 = '''<MIME mime="text/html" handler="FASTCGI" self="YES" param="">
  <EXTENSION value="fcgi"/>
</MIME>'''
        self.mime_1 = '''<MIME mime="text/plain" handler="SEND" param="">
  <EXTENSION value="asc"/>
  <EXTENSION value="c"/>
  <EXTENSION value="cc"/>
  <EXTENSION value="f"/>
  <EXTENSION value="f90"/>
  <EXTENSION value="h"/>
  <EXTENSION value="hh"/>
  <EXTENSION value="m"/>
  <EXTENSION value="txt"/>
</MIME>'''
        self.text = '<MIMES>{0}{1}</MIMES>'.format(self.mime_0, self.mime_1)
    
    def test_from_string(self):
        mimes = MIMETypes.from_string(self.text)
        self.assertEqual(mimes.MIME_types[0], MIMEType.from_string(self.mime_0))
        self.assertEqual(mimes.MIME_types[1], MIMEType.from_string(self.mime_1))

    def test_from_lxml(self):
        mimes = MIMETypes.from_lxml_element(etree.XML(self.text))
        self.assertEqual(mimes.MIME_types[0], MIMEType.from_string(self.mime_0))
        self.assertEqual(mimes.MIME_types[1], MIMEType.from_string(self.mime_1))
        
    def test_to_string(self):
        mimes = MIMETypes.from_string(self.text)
        copy = MIMETypes.from_string(str(mimes))
        self.assertEqual(mimes, copy)
        
    def test_to_lxml(self):
        mimes = MIMETypes.from_string(self.text)
        copy = MIMETypes.from_lxml_element(mimes.to_lxml_element())
        self.assertEqual(mimes, copy)

    def test_equality(self):
        self.assertEqual(MIMETypes.from_string(self.text),
                         MIMETypes.from_string(self.text))
        self.assertNotEqual(MIMETypes.from_string(self.text),
                            MIMETypes.from_string(
                '<MIMES>{0}</MIMES>'.format(self.mime_0)))

    def test_bad_root_tag(self):
        text = '<ERROR>{0}{1}</ERROR>'.format(
            self.mime_0, self.mime_1)
        self.assertRaises(AttributeError, MIMETypes.from_string, text)
        self.assertRaises(AttributeError, MIMETypes.from_lxml_element,
                          etree.XML(text))
        
if __name__ == '__main__':
    unittest.main()
