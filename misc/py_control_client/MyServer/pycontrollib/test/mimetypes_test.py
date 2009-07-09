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
from definition import DefinitionElement, DefinitionTree

class MIMETypeTest(unittest.TestCase):
    def setUp(self):
        self.definitions = []
        self.definitions.append(DefinitionElement('http.use_error_file',
                                                  {'value': 'YES'}))
        self.definitions.append(DefinitionElement('http.error.file.404',
                                                  {'value': '404.html'}))
        self.definitions.append(
            DefinitionTree(
                'http.default_file',
                [DefinitionElement(attributes = {'value': 'index.html'}),
                 DefinitionElement(attributes = {'value': 'index.htm'})]))
#        self.text = \
#             '''<MIME mime="application/xhtml+xml" handler="CGI" param="/usr/bin/python" self="NO" >
#   <EXTENSION value="xhtml" />
#   <EXTENSION value="xml" />
#   <EXTENSION value="py" />
#   <FILTER value="gzip" />
#   <FILTER value="bzip2" />
#   {0}
#   <PATH regex="^/cgi-bin/python/.*$" />
# </MIME>'''.format('\n'.join(map(lambda element: str(element),
#                                 self.definitions)))
    def test_creation(self):
        MIMEType('text/plain', 'CGI')
        MIMEType('text/plain', 'CGI', '/usr/bin/python')
        MIMEType('text/plain', 'CGI', '/usr/bin/python', set(['py']))
        MIMEType('text/plain', 'CGI', '/usr/bin/python', set(['py']),
                 '^/cgi-bin/.*$')
        MIMEType('text/plain', 'CGI', '/usr/bin/python', set(['py']),
                 '^/cgi-bin/.*$', ['gzip'])
        MIMEType('text/plain', 'CGI', '/usr/bin/python', set(['py']),
                 '^/cgi-bin/.*$', ['gzip'], False)
        MIMEType('text/plain', 'CGI', '/usr/bin/python', set(['py']),
                 '^/cgi-bin/.*$', ['gzip'], False, self.definitions)

    def test_mime(self):
        mime = MIMEType('text/plain', 'SEND')
        self.assertEqual('text/plain', mime.get_mime())
        mime.set_mime('application/xhtml+xml')
        self.assertEqual('application/xhtml+xml', mime.get_mime())
        self.assertRaises(AttributeError, mime.set_mime, None)
        self.assertRaises(AttributeError, MIMEType, None, 'SEND')
        
    def test_handler(self):
        mime = MIMEType('text/plain', 'SEND')
        self.assertEqual('SEND', mime.get_handler())
        mime.set_handler('SOME HANDLER')
        self.assertEqual('SOME HANDLER', mime.get_handler())
        self.assertRaises(AttributeError, mime.set_handler, None)
        self.assertRaises(AttributeError, MIMEType, 'text/plain', None)

    def test_param(self):
        mime = MIMEType('text/plain', 'SEND')
        self.assertEqual(None, mime.get_param())
        mime.set_param('/usr/bin/python')
        self.assertEqual('/usr/bin/python', mime.get_param())
        mime.set_param(None)
        self.assertEqual(None, mime.get_param())
        mime = MIMEType('text/plain', 'SEND', param = '/usr/bin/python')
        self.assertEqual('/usr/bin/python', mime.get_param())
    
    def test_extension(self):
        mime = MIMEType('text/plain', 'SEND')
        self.assertEqual(set(), mime.get_extensions())
        mime.add_extension('py')
        mime.add_extension('html')
        self.assertEqual(set(['py', 'html']), mime.get_extensions())
        mime.remove_extension('py')
        self.assertEqual(set(['html']), mime.get_extensions())
        mime = MIMEType('text/plain', 'SEND', extensions = set(['py', 'html']))
        self.assertEqual(set(['py', 'html']), mime.get_extensions())

    def test_path(self):
        mime = MIMEType('text/plain', 'SEND')
        self.assertEqual(None, mime.get_path())
        mime.set_path('^/www/.*$')
        self.assertEqual('^/www/.*$', mime.get_path())
        mime.set_path(None)
        self.assertEqual(None, mime.get_path())
        mime = MIMEType('text/plain', 'SEND', path = '^/www/.*$')
        self.assertEqual('^/www/.*$', mime.get_path())

    def test_filters(self):
        mime = MIMEType('text/plain', 'SEND')
        self.assertEqual([], mime.get_filters())
        mime.add_filter('gzip')
        mime.add_filter('bzip2')
        self.assertEqual(['gzip', 'bzip2'], mime.get_filters())
        mime.add_filter('bzip2', 0)
        self.assertEqual(['bzip2', 'gzip', 'bzip2'], mime.get_filters())
        mime.remove_filter(2)
        self.assertEqual(['bzip2', 'gzip'], mime.get_filters())
        self.assertEqual('gzip', mime.get_filter(1))
        mime = MIMEType('text/plain', 'SEND', filters = ['gzip', 'bzip2'])
        self.assertEqual(['gzip', 'bzip2'], mime.get_filters())

    def test_self_executed(self):
        mime = MIMEType('text/plain', 'SEND')
        self.assertEqual(None, mime.get_self_executed())
        mime.set_self_executed(True)
        self.assertEqual(True, mime.get_self_executed())
        mime.set_self_executed(None)
        self.assertEqual(None, mime.get_self_executed())
        mime = MIMEType('text/plain', 'SEND', self_executed = True)
        self.assertEqual(True, mime.get_self_executed())
        
    def test_definitions(self):
        mime = MIMEType('text/plain', 'SEND')
        self.assertEqual([], mime.get_definitions())
        for definition in self.definitions:
            mime.add_definition(definition)
        self.assertEqual(self.definitions, mime.get_definitions())
        for i in xrange(len(self.definitions)):
            self.assertEqual(self.definitions[i], mime.get_definition(i))
        mime.remove_definition(0)
        self.assertEqual(self.definitions[1:], mime.get_definitions())
        mime = MIMEType('text/plain', 'SEND', definitions = self.definitions)
        self.assertEqual(self.definitions, mime.get_definitions())
        
    def test_equality(self):
        self.assertEqual(MIMEType('text/plain', 'CGI', '/usr/bin/python',
                                  set(['py']), '^/.*$', ['gzip'], False,
                                  self.definitions),
                         MIMEType('text/plain', 'CGI', '/usr/bin/python',
                                  set(['py']), '^/.*$', ['gzip'], False,
                                  self.definitions))
        self.assertNotEqual(MIMEType('text/plain', 'CGI', '/usr/bin/python',
                                  set(['py']), '^/.*$', ['gzip'], False,
                                  self.definitions),
                            MIMEType('other', 'CGI', '/usr/bin/python',
                                  set(['py']), '^/.*$', ['gzip'], False,
                                     self.definitions))
        self.assertNotEqual(MIMEType('text/plain', 'CGI', '/usr/bin/python',
                                     set(['py']), '^/.*$', ['gzip'], False,
                                     self.definitions),
                            MIMEType('text/plain', 'SEND', '/usr/bin/python',
                                     set(['py']), '^/.*$', ['gzip'], False,
                                     self.definitions))
        self.assertNotEqual(MIMEType('text/plain', 'CGI', '/usr/bin/python',
                                     set(['py']), '^/.*$', ['gzip'], False,
                                     self.definitions),
                            MIMEType('text/plain', 'CGI', '/usr/bin/python26',
                                     set(['py']), '^/.*$', ['gzip'], False,
                                     self.definitions))
        self.assertNotEqual(MIMEType('text/plain', 'CGI', '/usr/bin/python',
                                     set(['py']), '^/.*$', ['gzip'], False,
                                     self.definitions),
                            MIMEType('text/plain', 'CGI', '/usr/bin/python',
                                     set(['pyc']), '^/.*$', ['gzip'], False,
                                     self.definitions))
        self.assertNotEqual(MIMEType('text/plain', 'CGI', '/usr/bin/python',
                                     set(['py']), '^/.*$', ['gzip'], False,
                                     self.definitions),
                            MIMEType('text/plain', 'CGI', '/usr/bin/python',
                                     set(['py']), '^/other/.*$', ['gzip'], False,
                                     self.definitions))
        self.assertNotEqual(MIMEType('text/plain', 'CGI', '/usr/bin/python',
                                     set(['py']), '^/.*$', ['gzip'], False,
                                     self.definitions),
                            MIMEType('text/plain', 'CGI', '/usr/bin/python',
                                     set(['py']), '^/.*$', ['bzip2'], False,
                                     self.definitions))
        self.assertNotEqual(MIMEType('text/plain', 'CGI', '/usr/bin/python',
                                     set(['py']), '^/.*$', ['gzip'], False,
                                     self.definitions),
                            MIMEType('text/plain', 'CGI', '/usr/bin/python',
                                     set(['py']), '^/.*$', ['gzip'], True,
                                     self.definitions))
        self.assertNotEqual(MIMEType('text/plain', 'CGI', '/usr/bin/python',
                                     set(['py']), '^/.*$', ['gzip'], False,
                                     self.definitions),
                            MIMEType('text/plain', 'CGI', '/usr/bin/python',
                                     set(['py']), '^/.*$', ['gzip'], False,
                                     []))
        self.assertNotEqual(MIMEType('text/plain', 'SEND'), 'other type')

    def test_from_string(self):
        text = '<MIME mime="text/plain" handler="SEND" />'
        mime = MIMEType.from_string(text)
        right = MIMEType('text/plain', 'SEND')
        self.assertEqual(mime, right)

    def test_from_string_param(self):
        text = '<MIME mime="text/plain" handler="SEND" param="/usr/bin/python" />'
        mime = MIMEType.from_string(text)
        right = MIMEType('text/plain', 'SEND', '/usr/bin/python')
        self.assertEqual(mime, right)

    def test_from_string_extension(self):
        text = '<MIME mime="text/plain" handler="SEND"><EXTENSION value="py" /></MIME>'
        mime = MIMEType.from_string(text)
        right = MIMEType('text/plain', 'SEND', extensions = ['py'])
        self.assertEqual(mime, right)

    def test_from_string_path(self):
        text = '<MIME mime="text/plain" handler="SEND"><PATH regex="^/.*$" /></MIME>'
        mime = MIMEType.from_string(text)
        right = MIMEType('text/plain', 'SEND', path = '^/.*$')
        self.assertEqual(mime, right)

    def test_from_string_filter(self):
        text = '''<MIME mime="text/plain" handler="SEND">
  <FILTER value="gzip" />
  <FILTER value="bzip2" />
</MIME>'''
        mime = MIMEType.from_string(text)
        right = MIMEType('text/plain', 'SEND', filters = ['gzip', 'bzip2'])
        self.assertEqual(mime, right)

    def test_from_string_self_executed(self):
        text = '<MIME mime="text/plain" handler="SEND" self="YES" />'
        mime = MIMEType.from_string(text)
        right = MIMEType('text/plain', 'SEND', self_executed = True)
        self.assertEqual(mime, right)

    def test_from_string_definitions(self):
        text = '<MIME mime="text/plain" handler="SEND">{0}</MIME>'.format(
            '\n'.join(map(str, self.definitions)))
        mime = MIMEType.from_string(text)
        right = MIMEType('text/plain', 'SEND', definitions = self.definitions)
        self.assertEqual(mime, right)
        
    def test_from_string_full(self):
        text = '''<MIME mime="text/plain" handler="SEND" self="YES" param="python">
  <EXTENSION value="py" />
  <FILTER value="gzip" />
  <FILTER value="bzip2" />
  <PATH regex="^/.*$" />
  {0}
</MIME>'''.format('\n'.join(map(str, self.definitions)))
        mime = MIMEType.from_string(text)
        right = MIMEType('text/plain', 'SEND', 'python', ['py'], '^/.*$',
                         ['gzip', 'bzip2'], True, self.definitions)
        self.assertEqual(mime, right)

    def test_from_lxml(self):
        text = '<MIME mime="text/plain" handler="SEND" />'
        mime = MIMEType.from_lxml_element(etree.XML(text))
        right = MIMEType('text/plain', 'SEND')
        self.assertEqual(mime, right)

    def test_from_lxml_param(self):
        text = '<MIME mime="text/plain" handler="SEND" param="/usr/bin/python" />'
        mime = MIMEType.from_lxml_element(etree.XML(text))
        right = MIMEType('text/plain', 'SEND', '/usr/bin/python')
        self.assertEqual(mime, right)

    def test_from_lxml_extension(self):
        text = '<MIME mime="text/plain" handler="SEND"><EXTENSION value="py" /></MIME>'
        mime = MIMEType.from_lxml_element(etree.XML(text))
        right = MIMEType('text/plain', 'SEND', extensions = ['py'])
        self.assertEqual(mime, right)

    def test_from_lxml_path(self):
        text = '<MIME mime="text/plain" handler="SEND"><PATH regex="^/.*$" /></MIME>'
        mime = MIMEType.from_lxml_element(etree.XML(text))
        right = MIMEType('text/plain', 'SEND', path = '^/.*$')
        self.assertEqual(mime, right)

    def test_from_lxml_filter(self):
        text = '''<MIME mime="text/plain" handler="SEND">
  <FILTER value="gzip" />
  <FILTER value="bzip2" />
</MIME>'''
        mime = MIMEType.from_lxml_element(etree.XML(text))
        right = MIMEType('text/plain', 'SEND', filters = ['gzip', 'bzip2'])
        self.assertEqual(mime, right)

    def test_from_lxml_self_executed(self):
        text = '<MIME mime="text/plain" handler="SEND" self="YES" />'
        mime = MIMEType.from_lxml_element(etree.XML(text))
        right = MIMEType('text/plain', 'SEND', self_executed = True)
        self.assertEqual(mime, right)

    def test_from_lxml_definitions(self):
        text = '<MIME mime="text/plain" handler="SEND">{0}</MIME>'.format(
            '\n'.join(map(str, self.definitions)))
        mime = MIMEType.from_lxml_element(etree.XML(text))
        right = MIMEType('text/plain', 'SEND', definitions = self.definitions)
        self.assertEqual(mime, right)

    def test_from_lxml_full(self):
        text = '''<MIME mime="text/plain" handler="SEND" self="YES" param="python">
  <EXTENSION value="py" />
  <FILTER value="gzip" />
  <FILTER value="bzip2" />
  <PATH regex="^/.*$" />
  {0}
</MIME>'''.format('\n'.join(map(str, self.definitions)))
        mime = MIMEType.from_lxml_element(etree.XML(text))
        right = MIMEType('text/plain', 'SEND', 'python', ['py'], '^/.*$',
                         ['gzip', 'bzip2'], True, self.definitions)
        self.assertEqual(mime, right)

    def test_bad_root_tag(self):
        text = '<ERROR mime="application/xhtml+xml" handler="CGI" />'
        self.assertRaises(AttributeError, MIMEType.from_string, text)
        self.assertRaises(AttributeError, MIMEType.from_lxml_element,
                          etree.XML(text))
        
    def test_to_string(self):
        mime = MIMEType('text/plain', 'SEND')
        copy = MIMEType.from_string(str(mime))
        self.assertEqual(mime, copy)

    def test_to_string_param(self):
        mime = MIMEType('text/plain', 'SEND', '/usr/bin/python')
        copy = MIMEType.from_string(str(mime))
        self.assertEqual(mime, copy)

    def test_to_string_extension(self):
        mime = MIMEType('text/plain', 'SEND', extensions = ['py'])
        copy = MIMEType.from_string(str(mime))
        self.assertEqual(mime, copy)

    def test_to_string_path(self):
        mime = MIMEType('text/plain', 'SEND', path = '^/.*$')
        copy = MIMEType.from_string(str(mime))
        self.assertEqual(mime, copy)

    def test_to_string_filter(self):
        mime = MIMEType('text/plain', 'SEND', filters = ['gzip', 'bzip2'])
        copy = MIMEType.from_string(str(mime))
        self.assertEqual(mime, copy)

    def test_to_string_self_executed(self):
        mime = MIMEType('text/plain', 'SEND', self_executed = True)
        copy = MIMEType.from_string(str(mime))
        self.assertEqual(mime, copy)

    def test_to_string_definitions(self):
        mime = MIMEType('text/plain', 'SEND', definitions = self.definitions)
        copy = MIMEType.from_string(str(mime))
        self.assertEqual(mime, copy)
        
    def test_to_string_full(self):
        mime = MIMEType('text/plain', 'SEND', 'python', ['py'], '^/.*$',
                        ['gzip', 'bzip2'], True, self.definitions)
        copy = MIMEType.from_string(str(mime))
        self.assertEqual(mime, copy)

    def test_to_lxml(self):
        mime = MIMEType('text/plain', 'SEND')
        copy = MIMEType.from_lxml_element(mime.to_lxml_element())
        self.assertEqual(mime, copy)

    def test_to_lxml_param(self):
        mime = MIMEType('text/plain', 'SEND', '/usr/bin/python')
        copy = MIMEType.from_lxml_element(mime.to_lxml_element())
        self.assertEqual(mime, copy)

    def test_to_lxml_extension(self):
        mime = MIMEType('text/plain', 'SEND', extensions = ['py'])
        copy = MIMEType.from_lxml_element(mime.to_lxml_element())
        self.assertEqual(mime, copy)

    def test_to_lxml_path(self):
        mime = MIMEType('text/plain', 'SEND', path = '^/.*$')
        copy = MIMEType.from_lxml_element(mime.to_lxml_element())
        self.assertEqual(mime, copy)

    def test_to_lxml_filter(self):
        mime = MIMEType('text/plain', 'SEND', filters = ['gzip', 'bzip2'])
        copy = MIMEType.from_lxml_element(mime.to_lxml_element())
        self.assertEqual(mime, copy)

    def test_to_lxml_self_executed(self):
        mime = MIMEType('text/plain', 'SEND', self_executed = True)
        copy = MIMEType.from_lxml_element(mime.to_lxml_element())
        self.assertEqual(mime, copy)

    def test_to_lxml_definitions(self):
        mime = MIMEType('text/plain', 'SEND', definitions = self.definitions)
        copy = MIMEType.from_lxml_element(mime.to_lxml_element())
        self.assertEqual(mime, copy)
        
    def test_to_lxml_full(self):
        mime = MIMEType('text/plain', 'SEND', 'python', ['py'], '^/.*$',
                        ['gzip', 'bzip2'], True, self.definitions)
        copy = MIMEType.from_lxml_element(mime.to_lxml_element())
        self.assertEqual(mime, copy)

class MIMETypesTest(unittest.TestCase):
    def setUp(self):
        self.mime_0 = MIMEType('text/html', 'FASTCGI', '', self_executed = True,
                               extensions = ['fcgi'])
        self.mime_1 = MIMEType('text/plain', 'SEND', '',
                               extensions = ['asc', 'c', 'cc', 'f', 'f90', 'h', 'hh'])
        self.text = '<MIMES>{0}{1}</MIMES>'.format(self.mime_0, self.mime_1)
    
    def test_creation(self):
        mimes = MIMETypes([self.mime_0, self.mime_1])
        self.assertEqual(mimes.MIME_types[0], self.mime_0)
        self.assertEqual(mimes.MIME_types[1], self.mime_1)

    def test_from_string(self):
        mimes = MIMETypes.from_string(self.text)
        self.assertEqual(mimes.MIME_types[0], self.mime_0)
        self.assertEqual(mimes.MIME_types[1], self.mime_1)

    def test_from_lxml(self):
        mimes = MIMETypes.from_lxml_element(etree.XML(self.text))
        self.assertEqual(mimes.MIME_types[0], self.mime_0)
        self.assertEqual(mimes.MIME_types[1], self.mime_1)
        
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
        self.assertNotEqual(MIMETypes.from_string(self.text), [])

    def test_bad_root_tag(self):
        text = '<ERROR>{0}{1}</ERROR>'.format(
            self.mime_0, self.mime_1)
        self.assertRaises(AttributeError, MIMETypes.from_string, text)
        self.assertRaises(AttributeError, MIMETypes.from_lxml_element,
                          etree.XML(text))
        
if __name__ == '__main__':
    unittest.main()
