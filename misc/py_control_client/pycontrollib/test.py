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
from mimetypes import MIMEType, MIMETypes

class MIMETypeTest(unittest.TestCase):
    text = '''<MIME mime="application/xhtml+xml" handler="CGI" param="/usr/bin/python" self="NO" >
  <EXTENSION value="xhtml" />
  <EXTENSION value="xml" />
  <EXTENSION value="py" />
  <FILTER value="gzip" />
  <PATH regex="^/cgi-bin/python/.*$" />
</MIME>'''
    
    def test_from_string(self):
        mime = MIMEType.from_string(MIMETypeTest.text)
        self.assertEqual(mime.mime, 'application/xhtml+xml')
        self.assertEqual(mime.handler, 'CGI')
        self.assertEqual(mime.param, '/usr/bin/python')
        self.assertEqual(mime.extension, set(['xhtml', 'xml', 'py']))
        self.assertEqual(mime.path, '^/cgi-bin/python/.*$')
        self.assertEqual(mime.filter, 'gzip')
        self.assertEqual(mime.self_executed, False)
        
    def test_to_string(self):
        mime = MIMEType.from_string(MIMETypeTest.text)
        copy = MIMEType.from_string(str(mime))
        self.assertEqual(mime, copy)
        
class MIMETypesTest(unittest.TestCase):
    mime_0 = '''<MIME mime="text/html" handler="FASTCGI" self="YES" param="">
  <EXTENSION value="fcgi"/>
</MIME>'''

    mime_1 = '''<MIME mime="text/plain" handler="SEND" param="">
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
    
    text = '<MIMES>{0}{1}</MIMES>'.format(mime_0, mime_1)
    
    def test_from_string(self):
        mimes = MIMETypes.from_string(MIMETypesTest.text)
        self.assertEqual(mimes.MIME_types[0],
                         MIMEType.from_string(MIMETypesTest.mime_0))
        self.assertEqual(mimes.MIME_types[1],
                         MIMEType.from_string(MIMETypesTest.mime_1))
        
    def test_to_string(self):
        mimes = MIMETypes.from_string(MIMETypesTest.text)
        copy = MIMETypes.from_string(str(mimes))
        self.assertEqual(mimes, copy)
        
if __name__ == '__main__':
    unittest.main()
