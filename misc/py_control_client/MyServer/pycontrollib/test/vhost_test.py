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
from vhost import VHost
from log import Log
from lxml import etree

class VHostTest(unittest.TestCase):
    def test_creation(self):
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      Log('ACCESSLOG'), Log('WARNINGLOG'))
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      Log('ACCESSLOG'), Log('WARNINGLOG'),
                      ['127.0.0.0/8', '192.168.0.0/16'])
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      Log('ACCESSLOG'), Log('WARNINGLOG'),
                      ['127.0.0.0/8', '192.168.0.0/16'],
                      {'host.domain': None})

    def test_name(self):
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      Log('ACCESSLOG'), Log('WARNINGLOG'))
        self.assertEqual('test vhost', vhost.get_name())
        vhost.set_name('vhost')
        self.assertEqual('vhost', vhost.get_name())
        self.assertRaises(AttributeError, vhost.set_name, None)
        self.assertRaises(AttributeError, VHost, None, 80, 'HTTP',
                          '/www', '/system', Log('ACCESSLOG'), Log('WARNINGLOG'))

    def test_port(self):
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      Log('ACCESSLOG'), Log('WARNINGLOG'))
        self.assertEqual(80, vhost.get_port())
        vhost.set_port(8080)
        self.assertEqual(8080, vhost.get_port())
        self.assertRaises(AttributeError, vhost.set_port, None)
        self.assertRaises(AttributeError, VHost, 'test vhost', None, 'HTTP',
                          '/www', '/system', Log('ACCESSLOG'), Log('WARNINGLOG'))

    def test_protocol(self):
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      Log('ACCESSLOG'), Log('WARNINGLOG'))
        self.assertEqual('HTTP', vhost.get_protocol())
        for protocol in VHost.valid_protocols:
            vhost.set_protocol(protocol)
            self.assertEqual(protocol, vhost.get_protocol())
        self.assertRaises(AttributeError, vhost.set_protocol, None)
        self.assertRaises(AttributeError, vhost.set_protocol, 'ERROR')
        self.assertRaises(AttributeError, VHost, 'test vhost', 80, None, '/www',
                          '/system', Log('ACCESSLOG'), Log('WARNINGLOG'))
        self.assertRaises(AttributeError, VHost, 'test vhost', 80, 'ERROR',
                          '/www', '/system', Log('ACCESSLOG'), Log('WARNINGLOG'))

    def test_doc_root(self):
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      Log('ACCESSLOG'), Log('WARNINGLOG'))
        self.assertEqual('/www', vhost.get_doc_root())
        vhost.set_doc_root('/var/www')
        self.assertEqual('/var/www', vhost.get_doc_root())
        self.assertRaises(AttributeError, vhost.set_doc_root, None)
        self.assertRaises(AttributeError, VHost, 'test vhost', 80, 'HTTP',
                          None, '/system', Log('ACCESSLOG'), Log('WARNINGLOG'))

    def test_sys_folder(self):
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      Log('ACCESSLOG'), Log('WARNINGLOG'))
        self.assertEqual('/system', vhost.get_sys_folder())
        vhost.set_sys_folder('/var/system')
        self.assertEqual('/var/system', vhost.get_sys_folder())
        self.assertRaises(AttributeError, vhost.set_sys_folder, None)
        self.assertRaises(AttributeError, VHost, 'test vhost', 80, 'HTTP',
                          '/www', None, Log('ACCESSLOG'), Log('WARNINGLOG'))

    def test_log(self):
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      Log('ACCESSLOG'), Log('WARNINGLOG'))
        self.assertEqual(Log('ACCESSLOG'), vhost.get_access_log())
        self.assertEqual(Log('WARNINGLOG'), vhost.get_warning_log())
        vhost.set_access_log(Log('ACCESSLOG', type = 'combined'))
        vhost.set_warning_log(Log('WARNINGLOG', type = 'combined'))
        self.assertEqual(Log('ACCESSLOG', type = 'combined'),
                         vhost.get_access_log())
        self.assertEqual(Log('WARNINGLOG', type = 'combined'),
                         vhost.get_warning_log())
        self.assertRaises(AttributeError, vhost.set_access_log, None)
        self.assertRaises(AttributeError, vhost.set_warning_log, None)
        self.assertRaises(AttributeError, vhost.set_access_log, Log('ERROR'))
        self.assertRaises(AttributeError, vhost.set_warning_log, Log('ERROR'))
        self.assertRaises(AttributeError, VHost, 'test vhost', 80, 'HTTP',
                          '/www', '/system', None, Log('WARNINGLOG'))
        self.assertRaises(AttributeError, VHost, 'test vhost', 80, 'HTTP',
                          '/www', '/system', Log('ACCESSLOG'), None)
        self.assertRaises(AttributeError, VHost, 'test vhost', 80, 'HTTP',
                          '/www', '/system', Log('ERROR'), Log('WARNINGLOG'))
        self.assertRaises(AttributeError, VHost, 'test vhost', 80, 'HTTP',
                          '/www', '/system', Log('ACCESSLOG'), Log('ERROR'))

    def test_ip(self):
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      Log('ACCESSLOG'), Log('WARNINGLOG'))
        self.assertEqual(set(), vhost.get_ip())
        vhost.add_ip('10.0.0.0/8')
        vhost.add_ip('192.168.0.0/16')
        self.assertEqual(set(['10.0.0.0/8', '192.168.0.0/16']), vhost.get_ip())
        vhost.remove_ip('10.0.0.0/8')
        self.assertEqual(set(['192.168.0.0/16']), vhost.get_ip())
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      Log('ACCESSLOG'), Log('WARNINGLOG'),
                      ['127.0.0.0/8', '192.168.0.0/16'])
        self.assertEqual(set(['127.0.0.0/8', '192.168.0.0/16']), vhost.get_ip())
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      Log('ACCESSLOG'), Log('WARNINGLOG'),
                      ip = ['127.0.0.0/8', '192.168.0.0/16'])
        self.assertEqual(set(['127.0.0.0/8', '192.168.0.0/16']), vhost.get_ip())

    def test_host(self):
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      Log('ACCESSLOG'), Log('WARNINGLOG'))
        self.assertEqual({}, vhost.get_host())
        vhost.add_host('foo.bar.com', False)
        vhost.add_host('test.me.org')
        self.assertEqual({'foo.bar.com': False, 'test.me.org': None},
                         vhost.get_host())
        vhost.remove_host('foo.bar.com')
        self.assertEqual({'test.me.org': None}, vhost.get_host())
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      Log('ACCESSLOG'), Log('WARNINGLOG'),
                      ['127.0.0.0/8', '192.168.0.0/16'],
                      {'test.me.org': None})
        self.assertEqual({'test.me.org': None}, vhost.get_host())
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      Log('ACCESSLOG'), Log('WARNINGLOG'),
                      host = {'test.me.org': None})
        self.assertEqual({'test.me.org': None}, vhost.get_host())

    def test_equality(self):
        vhost_0 = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                        Log('ACCESSLOG'), Log('WARNINGLOG'),
                        ['127.0.0.0/8', '192.168.0.0/16'],
                        {'host.domain': None})
        vhost_1 = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                        Log('ACCESSLOG'), Log('WARNINGLOG'),
                        ['127.0.0.0/8', '192.168.0.0/16'],
                        {'host.domain': None})
        self.assertEqual(vhost_0, vhost_1)
        vhost_1 = VHost('different', 80, 'HTTP', '/www', '/system',
                        Log('ACCESSLOG'), Log('WARNINGLOG'),
                        ['127.0.0.0/8', '192.168.0.0/16'],
                        {'host.domain': None})
        self.assertNotEqual(vhost_0, vhost_1)
        vhost_1 = VHost('test vhost', 81, 'HTTP', '/www', '/system',
                        Log('ACCESSLOG'), Log('WARNINGLOG'),
                        ['127.0.0.0/8', '192.168.0.0/16'],
                        {'host.domain': None})
        self.assertNotEqual(vhost_0, vhost_1)
        vhost_1 = VHost('test_vhost', 80, 'HTTPS', '/www', '/system',
                        Log('ACCESSLOG'), Log('WARNINGLOG'),
                        ['127.0.0.0/8', '192.168.0.0/16'],
                        {'host.domain': None})
        self.assertNotEqual(vhost_0, vhost_1)
        vhost_1 = VHost('test vhost', 80, 'HTTP', '/var/www', '/system',
                        Log('ACCESSLOG'), Log('WARNINGLOG'),
                        ['127.0.0.0/8', '192.168.0.0/16'],
                        {'host.domain': None})
        self.assertNotEqual(vhost_0, vhost_1)
        vhost_1 = VHost('test vhost', 80, 'HTTP', '/www', '/var/system',
                        Log('ACCESSLOG'), Log('WARNINGLOG'),
                        ['127.0.0.0/8', '192.168.0.0/16'],
                        {'host.domain': None})
        self.assertNotEqual(vhost_0, vhost_1)
        vhost_1 = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                        Log('ACCESSLOG', type = 'combined'), Log('WARNINGLOG'),
                        ['127.0.0.0/8', '192.168.0.0/16'],
                        {'host.domain': None})
        self.assertNotEqual(vhost_0, vhost_1)
        vhost_1 = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                        Log('ACCESSLOG'), Log('WARNINGLOG', type = 'combined'),
                        ['127.0.0.0/8', '192.168.0.0/16'],
                        {'host.domain': None})
        self.assertNotEqual(vhost_0, vhost_1)
        vhost_1 = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                        Log('ACCESSLOG'), Log('WARNINGLOG'),
                        ['127.0.0.0/24', '192.168.0.0/16'],
                        {'host.domain': None})
        self.assertNotEqual(vhost_0, vhost_1)
        vhost_1 = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                        Log('ACCESSLOG'), Log('WARNINGLOG'),
                        ['127.0.0.0/8', '192.168.0.0/16'],
                        {'host.domain': True})
        self.assertNotEqual(vhost_0, vhost_1)
        self.assertNotEqual(vhost_0, [])

    def test_from_string(self):
        text = '''<VHOST>
  <NAME>test vhost</NAME>
  <PORT>80</PORT>
  <IP>127.0.0.0/8</IP>
  <IP>192.168.0.0/16</IP>
  <PROTOCOL>HTTP</PROTOCOL>
  <DOCROOT>/www</DOCROOT>
  <SYSFOLDER>/system</SYSFOLDER>
  <HOST useRegex="YES">host.domain</HOST>
  <HOST>test.domain</HOST>
  <ACCESSLOG />
  <WARNINGLOG />
</VHOST>'''
        vhost = VHost.from_string(text)
        self.assertEqual(vhost, VHost('test vhost', 80, 'HTTP', '/www', '/system',
                                      Log('ACCESSLOG'), Log('WARNINGLOG'),
                                      ['127.0.0.0/8', '192.168.0.0/16'],
                                      {'host.domain': True, 'test.domain': None}))

    def test_from_lxml(self):
        text = '''<VHOST>
  <NAME>test vhost</NAME>
  <PORT>80</PORT>
  <IP>127.0.0.0/8</IP>
  <IP>192.168.0.0/16</IP>
  <PROTOCOL>HTTP</PROTOCOL>
  <DOCROOT>/www</DOCROOT>
  <SYSFOLDER>/system</SYSFOLDER>
  <HOST useRegex="YES">host.domain</HOST>
  <HOST>test.domain</HOST>
  <ACCESSLOG />
  <WARNINGLOG />
</VHOST>'''
        vhost = VHost.from_lxml_element(etree.XML(text))
        self.assertEqual(vhost, VHost('test vhost', 80, 'HTTP', '/www', '/system',
                                 Log('ACCESSLOG'), Log('WARNINGLOG'),
                                 ['127.0.0.0/8', '192.168.0.0/16'],
                                 {'host.domain': True, 'test.domain': None}))

    def test_to_string(self):
        text = '''<VHOST>
  <NAME>test vhost</NAME>
  <PORT>80</PORT>
  <IP>127.0.0.0/8</IP>
  <IP>192.168.0.0/16</IP>
  <PROTOCOL>HTTP</PROTOCOL>
  <DOCROOT>/www</DOCROOT>
  <SYSFOLDER>/system</SYSFOLDER>
  <HOST useRegex="YES">host.domain</HOST>
  <HOST>test.domain</HOST>
  <ACCESSLOG />
  <WARNINGLOG />
</VHOST>'''
        vhost = VHost.from_string(text)
        copy = VHost.from_string(str(vhost))
        self.assertEqual(vhost, copy)

    def test_to_lxml(self):
        text = '''<VHOST>
  <NAME>test vhost</NAME>
  <PORT>80</PORT>
  <IP>127.0.0.0/8</IP>
  <IP>192.168.0.0/16</IP>
  <PROTOCOL>HTTP</PROTOCOL>
  <DOCROOT>/www</DOCROOT>
  <SYSFOLDER>/system</SYSFOLDER>
  <HOST useRegex="YES">host.domain</HOST>
  <HOST>test.domain</HOST>
  <ACCESSLOG />
  <WARNINGLOG />
</VHOST>'''
        vhost = VHost.from_string(text)
        copy = VHost.from_lxml_element(vhost.to_lxml_element())
        self.assertEqual(vhost, copy)

    def test_bad_root_tag(self):
        text = '''<ERROR>
  <NAME>test vhost</NAME>
  <PORT>80</PORT>
  <PROTOCOL>HTTP</PROTOCOL>
  <DOCROOT>/www</DOCROOT>
  <SYSFOLDER>/system</SYSFOLDER>
  <ACCESSLOG />
  <WARNINGLOG />
</ERROR>'''
        self.assertRaises(AttributeError, VHost.from_string, text)
        self.assertRaises(AttributeError, VHost.from_lxml_element,
                          etree.XML(text))

if __name__ == '__main__':
    unittest.main()
