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
from vhost import VHost, VHosts
from log import Log
from lxml import etree

class VHostTest(unittest.TestCase):
    def test_creation(self):
        vhost = VHost()
        vhost = VHost('test vhost')
        vhost = VHost('test vhost', 80)
        vhost = VHost('test vhost', 80, 'HTTP')
        vhost = VHost('test vhost', 80, 'HTTP', '/www')
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system')
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      [Log('ACCESSLOG'), Log('WARNINGLOG')])
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      [Log('ACCESSLOG'), Log('WARNINGLOG')],
                      ['127.0.0.0/8', '192.168.0.0/16'])
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      [Log('ACCESSLOG'), Log('WARNINGLOG')],
                      ['127.0.0.0/8', '192.168.0.0/16'],
                      {'host.domain': None})
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      [Log('ACCESSLOG'), Log('WARNINGLOG')],
                      ['127.0.0.0/8', '192.168.0.0/16'],
                      {'host.domain': None}, 'private_key')
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      [Log('ACCESSLOG'), Log('WARNINGLOG')],
                      ['127.0.0.0/8', '192.168.0.0/16'],
                      {'host.domain': None}, 'private_key', 'certificate')

    def test_name(self):
        vhost = VHost('test vhost')
        self.assertEqual('test vhost', vhost.get_name())
        vhost.set_name('vhost')
        self.assertEqual('vhost', vhost.get_name())
        vhost.set_name(None)
        self.assertEqual(None, vhost.get_name())
        vhost = VHost(name = 'test vhost')
        self.assertEqual('test vhost', vhost.get_name())

    def test_port(self):
        vhost = VHost('test vhost', 80)
        self.assertEqual(80, vhost.get_port())
        vhost.set_port(8080)
        self.assertEqual(8080, vhost.get_port())
        vhost.set_port(None)
        self.assertEqual(None, vhost.get_port())
        vhost = VHost(port = 80)
        self.assertEqual(80, vhost.get_port())

    def test_protocol(self):
        vhost = VHost('test vhost', 80, 'HTTP')
        self.assertEqual('HTTP', vhost.get_protocol())
        vhost.set_protocol('NEW PROTOCOL')
        self.assertEqual('NEW PROTOCOL', vhost.get_protocol())
        vhost.set_protocol(None)
        self.assertEqual(None, vhost.get_protocol())
        vhost = VHost(protocol = 'HTTP')
        self.assertEqual('HTTP', vhost.get_protocol())

    def test_doc_root(self):
        vhost = VHost('test vhost', 80, 'HTTP', '/www')
        self.assertEqual('/www', vhost.get_doc_root())
        vhost.set_doc_root('/var/www')
        self.assertEqual('/var/www', vhost.get_doc_root())
        vhost.set_doc_root(None)
        self.assertEqual(None, vhost.get_doc_root())
        vhost = VHost(doc_root = '/www')
        self.assertEqual('/www', vhost.get_doc_root())

    def test_sys_folder(self):
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system')
        self.assertEqual('/system', vhost.get_sys_folder())
        vhost.set_sys_folder('/var/system')
        self.assertEqual('/var/system', vhost.get_sys_folder())
        vhost.set_sys_folder(None)
        self.assertEqual(None, vhost.get_sys_folder())
        vhost = VHost(sys_folder = '/system')
        self.assertEqual('/system', vhost.get_sys_folder())

    def test_logs(self):
        a_log = Log('ACCESSLOG')
        w_log = Log('WARNINGLOG')
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system', [a_log, w_log])
        self.assertEqual([a_log, w_log], vhost.get_logs())
        vhost.add_log(a_log)
        self.assertEqual([a_log, w_log, a_log], vhost.get_logs())
        vhost.remove_log(0)
        self.assertEqual([w_log, a_log], vhost.get_logs())
        vhost.add_log(w_log, 0)
        self.assertEqual([w_log, w_log, a_log], vhost.get_logs())
        vhost = VHost(logs = [a_log, w_log])
        self.assertEqual([a_log, w_log], vhost.get_logs())

    def test_ip(self):
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      [Log('ACCESSLOG')])
        self.assertEqual(set(), vhost.get_ip())
        vhost.add_ip('10.0.0.0/8')
        vhost.add_ip('192.168.0.0/16')
        self.assertEqual(set(['10.0.0.0/8', '192.168.0.0/16']), vhost.get_ip())
        vhost.remove_ip('10.0.0.0/8')
        self.assertEqual(set(['192.168.0.0/16']), vhost.get_ip())
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      [Log('ACCESSLOG'), Log('WARNINGLOG')],
                      ['127.0.0.0/8', '192.168.0.0/16'])
        self.assertEqual(set(['127.0.0.0/8', '192.168.0.0/16']), vhost.get_ip())
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      [Log('ACCESSLOG'), Log('WARNINGLOG')],
                      ip = ['127.0.0.0/8', '192.168.0.0/16'])
        self.assertEqual(set(['127.0.0.0/8', '192.168.0.0/16']), vhost.get_ip())

    def test_host(self):
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      [Log('ACCESSLOG')])
        self.assertEqual({}, vhost.get_host())
        vhost.add_host('foo.bar.com', False)
        vhost.add_host('test.me.org')
        self.assertEqual({'foo.bar.com': False, 'test.me.org': None},
                         vhost.get_host())
        vhost.remove_host('foo.bar.com')
        self.assertEqual({'test.me.org': None}, vhost.get_host())
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      [Log('ACCESSLOG'), Log('WARNINGLOG')],
                      ['127.0.0.0/8', '192.168.0.0/16'],
                      {'test.me.org': None})
        self.assertEqual({'test.me.org': None}, vhost.get_host())
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      [Log('ACCESSLOG'), Log('WARNINGLOG')],
                      host = {'test.me.org': None})
        self.assertEqual({'test.me.org': None}, vhost.get_host())

    def test_private_key(self):
        vhost = VHost()
        self.assertEqual(None, vhost.get_private_key())
        vhost.set_private_key('path')
        self.assertEqual('path', vhost.get_private_key())
        vhost.set_private_key(None)
        self.assertEqual(None, vhost.get_private_key())
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      [Log('ACCESSLOG'), Log('WARNINGLOG')],
                      ['127.0.0.0/8', '192.168.0.0/16'],
                      {'host.domain': None}, 'path')
        self.assertEqual('path', vhost.get_private_key())
        vhost = VHost(private_key = 'path')
        self.assertEqual('path', vhost.get_private_key())

    def test_certificate(self):
        vhost = VHost()
        self.assertEqual(None, vhost.get_certificate())
        vhost.set_certificate('path')
        self.assertEqual('path', vhost.get_certificate())
        vhost.set_certificate(None)
        self.assertEqual(None, vhost.get_certificate())
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      [Log('ACCESSLOG'), Log('WARNINGLOG')],
                      ['127.0.0.0/8', '192.168.0.0/16'],
                      {'host.domain': None}, 'key', 'path')
        self.assertEqual('path', vhost.get_certificate())
        vhost = VHost(certificate = 'path')
        self.assertEqual('path', vhost.get_certificate())

    def test_equality(self):
        vhost_0 = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                        [Log('ACCESSLOG'), Log('WARNINGLOG')],
                        ['127.0.0.0/8', '192.168.0.0/16'],
                        {'host.domain': None}, 'private_key', 'certificate')
        vhost_1 = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                        [Log('ACCESSLOG'), Log('WARNINGLOG')],
                        ['127.0.0.0/8', '192.168.0.0/16'],
                        {'host.domain': None}, 'private_key', 'certificate')
        self.assertEqual(vhost_0, vhost_1)
        vhost_1 = VHost.from_string(str(vhost_0))
        vhost_1.set_name('different')
        self.assertNotEqual(vhost_0, vhost_1)
        vhost_1 = VHost.from_string(str(vhost_0))
        vhost_1.set_port(81)
        self.assertNotEqual(vhost_0, vhost_1)
        vhost_1 = VHost.from_string(str(vhost_0))
        vhost_1.set_protocol('HTTPS')
        self.assertNotEqual(vhost_0, vhost_1)
        vhost_1 = VHost.from_string(str(vhost_0))
        vhost_1.set_doc_root('/var/www')
        self.assertNotEqual(vhost_0, vhost_1)
        vhost_1 = VHost.from_string(str(vhost_0))
        vhost_1.set_sys_folder('/var/system')
        self.assertNotEqual(vhost_0, vhost_1)
        vhost_1 = VHost.from_string(str(vhost_0))
        vhost_1.add_log(Log('NEWLOG'))
        self.assertNotEqual(vhost_0, vhost_1)
        vhost_1 = VHost.from_string(str(vhost_0))
        vhost_1.add_ip('10.0.0.0/8')
        self.assertNotEqual(vhost_0, vhost_1)
        vhost_1 = VHost.from_string(str(vhost_0))
        vhost_1.add_host('newhost', None)
        self.assertNotEqual(vhost_0, vhost_1)
        vhost_1 = VHost.from_string(str(vhost_0))
        vhost_1.set_private_key('new_key')
        self.assertNotEqual(vhost_0, vhost_1)
        vhost_1 = VHost.from_string(str(vhost_0))
        vhost_1.set_certificate('new certificate')
        self.assertNotEqual(vhost_0, vhost_1)
        self.assertNotEqual(vhost_0, [])

    def test_from_string(self):
        text = '<VHOST />'
        vhost = VHost.from_string(text)
        right = VHost()
        self.assertEqual(vhost, right)

    def test_from_string_name(self):
        text = '<VHOST><NAME>test vhost</NAME></VHOST>'
        vhost = VHost.from_string(text)
        right = VHost('test vhost')
        self.assertEqual(vhost, right)

    def test_from_string_port(self):
        text = '<VHOST><PORT>80</PORT></VHOST>'
        vhost = VHost.from_string(text)
        right = VHost(port = 80)
        self.assertEqual(vhost, right)

    def test_from_string_protocol(self):
        text = '<VHOST><PROTOCOL>HTTP</PROTOCOL></VHOST>'
        vhost = VHost.from_string(text)
        right = VHost(protocol = 'HTTP')
        self.assertEqual(vhost, right)

    def test_from_string_doc_root(self):
        text = '<VHOST><DOCROOT>/www</DOCROOT></VHOST>'
        vhost = VHost.from_string(text)
        right = VHost(doc_root = '/www')
        self.assertEqual(vhost, right)

    def test_from_string_sys_folder(self):
        text = '<VHOST><SYSFOLDER>/system</SYSFOLDER></VHOST>'
        vhost = VHost.from_string(text)
        right = VHost(sys_folder = '/system')
        self.assertEqual(vhost, right)

    def test_from_string_logs(self):
        text = '<VHOST><ACCESSLOG /><SOMELOG /></VHOST>'
        vhost = VHost.from_string(text)
        right = VHost(logs = [Log('ACCESSLOG'), Log('SOMELOG')])
        self.assertEqual(vhost, right)

    def test_from_string_ip(self):
        text = '<VHOST><IP>127.0.0.0/8</IP></VHOST>'
        vhost = VHost.from_string(text)
        right = VHost(ip = ['127.0.0.0/8'])
        self.assertEqual(vhost, right)

    def test_from_string_host(self):
        text = '<VHOST><HOST useRegex="YES">.*</HOST><HOST>a.org</HOST></VHOST>'
        vhost = VHost.from_string(text)
        right = VHost(host = {'.*': True, 'a.org': None})
        self.assertEqual(vhost, right)

    def test_from_string_private_key(self):
        text = '<VHOST><SSL_PRIVATEKEY>private_key</SSL_PRIVATEKEY></VHOST>'
        vhost = VHost.from_string(text)
        right = VHost(private_key = 'private_key')
        self.assertEqual(vhost, right)

    def test_from_string_certificate(self):
        text = '<VHOST><SSL_CERTIFICATE>certificate</SSL_CERTIFICATE></VHOST>'
        vhost = VHost.from_string(text)
        right = VHost(certificate = 'certificate')
        self.assertEqual(vhost, right)

    def test_from_string_full(self):
        text = '''<VHOST>
  <NAME>test vhost</NAME>
  <PORT>80</PORT>
  <PROTOCOL>HTTP</PROTOCOL>
  <DOCROOT>/www</DOCROOT>
  <SYSFOLDER>/system</SYSFOLDER>
  <ACCESSLOG />
  <WARNINGLOG />
  <IP>127.0.0.0/8</IP>
  <IP>192.168.0.0/16</IP>
  <HOST useRegex="YES">host.domain</HOST>
  <HOST>test.domain</HOST>
  <SSL_PRIVATEKEY>private_key</SSL_PRIVATEKEY>
  <SSL_CERTIFICATE>certificate</SSL_CERTIFICATE>
</VHOST>'''
        vhost = VHost.from_string(text)
        right = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      [Log('ACCESSLOG'), Log('WARNINGLOG')],
                      ['127.0.0.0/8', '192.168.0.0/16'],
                      {'host.domain': True, 'test.domain': None},
                      'private_key', 'certificate')
        self.assertEqual(vhost, right)

    def test_from_lxml(self):
        text = '<VHOST />'
        vhost = VHost.from_lxml_element(etree.XML(text))
        right = VHost()
        self.assertEqual(vhost, right)

    def test_from_lxml_name(self):
        text = '<VHOST><NAME>test vhost</NAME></VHOST>'
        vhost = VHost.from_lxml_element(etree.XML(text))
        right = VHost('test vhost')
        self.assertEqual(vhost, right)

    def test_from_lxml_port(self):
        text = '<VHOST><PORT>80</PORT></VHOST>'
        vhost = VHost.from_lxml_element(etree.XML(text))
        right = VHost(port = 80)
        self.assertEqual(vhost, right)

    def test_from_lxml_protocol(self):
        text = '<VHOST><PROTOCOL>HTTP</PROTOCOL></VHOST>'
        vhost = VHost.from_lxml_element(etree.XML(text))
        right = VHost(protocol = 'HTTP')
        self.assertEqual(vhost, right)

    def test_from_lxml_doc_root(self):
        text = '<VHOST><DOCROOT>/www</DOCROOT></VHOST>'
        vhost = VHost.from_lxml_element(etree.XML(text))
        right = VHost(doc_root = '/www')
        self.assertEqual(vhost, right)

    def test_from_lxml_sys_folder(self):
        text = '<VHOST><SYSFOLDER>/system</SYSFOLDER></VHOST>'
        vhost = VHost.from_lxml_element(etree.XML(text))
        right = VHost(sys_folder = '/system')
        self.assertEqual(vhost, right)

    def test_from_lxml_logs(self):
        text = '<VHOST><ACCESSLOG /><SOMELOG /></VHOST>'
        vhost = VHost.from_lxml_element(etree.XML(text))
        right = VHost(logs = [Log('ACCESSLOG'), Log('SOMELOG')])
        self.assertEqual(vhost, right)

    def test_from_lxml_ip(self):
        text = '<VHOST><IP>127.0.0.0/8</IP></VHOST>'
        vhost = VHost.from_lxml_element(etree.XML(text))
        right = VHost(ip = ['127.0.0.0/8'])
        self.assertEqual(vhost, right)

    def test_from_lxml_host(self):
        text = '<VHOST><HOST useRegex="YES">.*</HOST><HOST>a.org</HOST></VHOST>'
        vhost = VHost.from_lxml_element(etree.XML(text))
        right = VHost(host = {'.*': True, 'a.org': None})
        self.assertEqual(vhost, right)

    def test_from_lxml_private_key(self):
        text = '<VHOST><SSL_PRIVATEKEY>private_key</SSL_PRIVATEKEY></VHOST>'
        vhost = VHost.from_lxml_element(etree.XML(text))
        right = VHost(private_key = 'private_key')
        self.assertEqual(vhost, right)

    def test_from_lxml_certificate(self):
        text = '<VHOST><SSL_CERTIFICATE>certificate</SSL_CERTIFICATE></VHOST>'
        vhost = VHost.from_lxml_element(etree.XML(text))
        right = VHost(certificate = 'certificate')
        self.assertEqual(vhost, right)

    def test_from_lxml_full(self):
        text = '''<VHOST>
  <NAME>test vhost</NAME>
  <PORT>80</PORT>
  <PROTOCOL>HTTP</PROTOCOL>
  <DOCROOT>/www</DOCROOT>
  <SYSFOLDER>/system</SYSFOLDER>
  <ACCESSLOG />
  <WARNINGLOG />
  <IP>127.0.0.0/8</IP>
  <IP>192.168.0.0/16</IP>
  <HOST useRegex="YES">host.domain</HOST>
  <HOST>test.domain</HOST>
  <SSL_PRIVATEKEY>private_key</SSL_PRIVATEKEY>
  <SSL_CERTIFICATE>certificate</SSL_CERTIFICATE>
</VHOST>'''
        vhost = VHost.from_lxml_element(etree.XML(text))
        right = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      [Log('ACCESSLOG'), Log('WARNINGLOG')],
                      ['127.0.0.0/8', '192.168.0.0/16'],
                      {'host.domain': True, 'test.domain': None},
                      'private_key', 'certificate')
        self.assertEqual(vhost, right)

    def test_bad_root_tag(self):
        text = '<ERROR />'
        self.assertRaises(AttributeError, VHost.from_string, text)
        self.assertRaises(AttributeError, VHost.from_lxml_element,
                          etree.XML(text))

    def test_to_string(self):
        vhost = VHost()
        copy = VHost.from_string(str(vhost))
        self.assertEqual(vhost, copy)

    def test_to_string_name(self):
        vhost = VHost('test vhost')
        copy = VHost.from_string(str(vhost))
        self.assertEqual(vhost, copy)

    def test_to_string_port(self):
        vhost = VHost(port = 80)
        copy = VHost.from_string(str(vhost))
        self.assertEqual(vhost, copy)

    def test_to_string_protocol(self):
        vhost = VHost(protocol = 'HTTP')
        copy = VHost.from_string(str(vhost))
        self.assertEqual(vhost, copy)

    def test_to_string_doc_root(self):
        vhost = VHost(doc_root = '/www')
        copy = VHost.from_string(str(vhost))
        self.assertEqual(vhost, copy)

    def test_to_string_sys_folder(self):
        vhost = VHost(doc_root = '/system')
        copy = VHost.from_string(str(vhost))
        self.assertEqual(vhost, copy)

    def test_to_string_logs(self):
        vhost = VHost(logs = [Log('ACCESSLOG'), Log('SOMELOG')])
        copy = VHost.from_string(str(vhost))
        self.assertEqual(vhost, copy)

    def test_to_string_ip(self):
        vhost = VHost(ip = ['127.0.0.0/8'])
        copy = VHost.from_string(str(vhost))
        self.assertEqual(vhost, copy)

    def test_to_string_host(self):
        vhost = VHost(host = {'.*': True, 'a.org': None})
        copy = VHost.from_string(str(vhost))
        self.assertEqual(vhost, copy)

    def test_to_string_private_key(self):
        vhost = VHost(private_key = 'private_key')
        copy = VHost.from_string(str(vhost))
        self.assertEqual(vhost, copy)

    def test_to_string_certificate(self):
        vhost = VHost(certificate = 'certificate')
        copy = VHost.from_string(str(vhost))
        self.assertEqual(vhost, copy)

    def test_to_string_full(self):
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      [Log('ACCESSLOG'), Log('WARNINGLOG')],
                      ['127.0.0.0/8', '192.168.0.0/16'],
                      {'host.domain': True, 'test.domain': None},
                      'private_key', 'certificate')
        copy = VHost.from_string(str(vhost))
        self.assertEqual(vhost, copy)

    def test_to_lxml(self):
        vhost = VHost()
        copy = VHost.from_lxml_element(vhost.to_lxml_element())
        self.assertEqual(vhost, copy)

    def test_to_lxml_name(self):
        vhost = VHost('test vhost')
        copy = VHost.from_lxml_element(vhost.to_lxml_element())
        self.assertEqual(vhost, copy)

    def test_to_lxml_port(self):
        vhost = VHost(port = 80)
        copy = VHost.from_lxml_element(vhost.to_lxml_element())
        self.assertEqual(vhost, copy)

    def test_to_lxml_protocol(self):
        vhost = VHost(protocol = 'HTTP')
        copy = VHost.from_lxml_element(vhost.to_lxml_element())
        self.assertEqual(vhost, copy)

    def test_to_lxml_doc_root(self):
        vhost = VHost(doc_root = '/www')
        copy = VHost.from_lxml_element(vhost.to_lxml_element())
        self.assertEqual(vhost, copy)

    def test_to_lxml_sys_folder(self):
        vhost = VHost(doc_root = '/system')
        copy = VHost.from_lxml_element(vhost.to_lxml_element())
        self.assertEqual(vhost, copy)

    def test_to_lxml_logs(self):
        vhost = VHost(logs = [Log('ACCESSLOG'), Log('SOMELOG')])
        copy = VHost.from_lxml_element(vhost.to_lxml_element())
        self.assertEqual(vhost, copy)

    def test_to_lxml_ip(self):
        vhost = VHost(ip = ['127.0.0.0/8'])
        copy = VHost.from_lxml_element(vhost.to_lxml_element())
        self.assertEqual(vhost, copy)

    def test_to_lxml_host(self):
        vhost = VHost(host = {'.*': True, 'a.org': None})
        copy = VHost.from_lxml_element(vhost.to_lxml_element())
        self.assertEqual(vhost, copy)
        
    def test_to_lxml_private_key(self):
        vhost = VHost(private_key = 'private_key')
        copy = VHost.from_lxml_element(vhost.to_lxml_element())
        self.assertEqual(vhost, copy)

    def test_to_lxml_certificate(self):
        vhost = VHost(certificate = 'certificate')
        copy = VHost.from_lxml_element(vhost.to_lxml_element())
        self.assertEqual(vhost, copy)

    def test_to_lxml_full(self):
        vhost = VHost('test vhost', 80, 'HTTP', '/www', '/system',
                      [Log('ACCESSLOG'), Log('WARNINGLOG')],
                      ['127.0.0.0/8', '192.168.0.0/16'],
                      {'host.domain': True, 'test.domain': None},
                      'private_key', 'certificate')
        copy = VHost.from_lxml_element(vhost.to_lxml_element())
        self.assertEqual(vhost, copy)

class VHostsTest(unittest.TestCase):
    def setUp(self):
        self.vhost_0 = VHost('vhost 0', 80, 'HTTP', '/www', '/system',
                             [Log('ACCESSLOG'), Log('WARNINGLOG')],
                             ['127.0.0.0/8', '192.168.0.0/16'],
                             {'host.domain': True, 'test.domain': None})
        self.vhost_1 = VHost('vhost 1', 443, 'HTTPS', '/www', '/system',
                             [Log('ACCESSLOG'), Log('WARNINGLOG')])
        self.text = '<VHOSTS>{0}{1}</VHOSTS>'.format(str(self.vhost_0),
                                                     str(self.vhost_1))

    def test_creation(self):
        vhosts = VHosts([self.vhost_0, self.vhost_1])
        self.assertEqual(vhosts.VHosts[0], self.vhost_0)
        self.assertEqual(vhosts.VHosts[1], self.vhost_1)

    def test_from_string(self):
        vhosts = VHosts.from_string(self.text)
        self.assertEqual(vhosts.VHosts[0], self.vhost_0)
        self.assertEqual(vhosts.VHosts[1], self.vhost_1)

    def test_from_string(self):
        vhosts = VHosts.from_lxml_element(etree.XML(self.text))
        self.assertEqual(vhosts.VHosts[0], self.vhost_0)
        self.assertEqual(vhosts.VHosts[1], self.vhost_1)

    def test_to_string(self):
        vhosts = VHosts.from_string(self.text)
        copy = VHosts.from_string(str(vhosts))
        self.assertEqual(vhosts, copy)

    def test_to_lxml(self):
        vhosts = VHosts.from_string(self.text)
        copy = VHosts.from_lxml_element(vhosts.to_lxml_element())
        self.assertEqual(vhosts, copy)

    def test_equality(self):
        self.assertEqual(VHosts.from_string(self.text),
                         VHosts.from_string(self.text))
        self.assertNotEqual(VHosts.from_string(self.text),
                            VHosts.from_string(
                '<VHOSTS>{0}</VHOSTS>'.format(str(self.vhost_0))))
        self.assertNotEqual(VHosts.from_string(self.text), [])

if __name__ == '__main__':
    unittest.main()
