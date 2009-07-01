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
from vhost import Vhost
from log import Log
from lxml import etree

class VhostTest(unittest.TestCase):
    def test_creation(self):
        vhost = Vhost('test vhost', 80, 'HTTP', '/www', '/system',
                      Log('ACCESSLOG'), Log('WARNINGLOG'))
        vhost = Vhost('test vhost', 80, 'HTTP', '/www', '/system',
                      Log('ACCESSLOG'), Log('WARNINGLOG'),
                      set(['127.0.0.0/8', '192.168.0.0/16']))
        vhost = Vhost('test vhost', 80, 'HTTP', '/www', '/system',
                      Log('ACCESSLOG'), Log('WARNINGLOG'),
                      set(['127.0.0.0/8', '192.168.0.0/16']),
                      set(['host.domain']))
        vhost = Vhost('test vhost', 80, 'HTTP', '/www', '/system',
                      Log('ACCESSLOG'), Log('WARNINGLOG'),
                      set(['127.0.0.0/8', '192.168.0.0/16']),
                      set(['host.domain']), True)

    def test_name(self):
        vhost = Vhost('test vhost', 80, 'HTTP', '/www', '/system',
                      Log('ACCESSLOG'), Log('WARNINGLOG'))
        self.assertEqual('test vhost', vhost.get_name())
        vhost.set_name('vhost')
        self.assertEqual('vhost', vhost.get_name())
        self.assertRaises(AttributeError, vhost.set_name, None)

    def test_port(self):
        vhost = Vhost('test vhost', 80, 'HTTP', '/www', '/system',
                      Log('ACCESSLOG'), Log('WARNINGLOG'))
        self.assertEqual(80, vhost.get_port())
        vhost.set_port(8080)
        self.assertEqual(8080, vhost.get_port())
        self.assertRaises(AttributeError, vhost.set_port, None)

    def test_protocol(self):
        vhost = Vhost('test vhost', 80, 'HTTP', '/www', '/system',
                      Log('ACCESSLOG'), Log('WARNINGLOG'))
        self.assertEqual('HTTP', vhost.get_protocol())
        vhost.set_port('FTP')
        self.assertEqual('FTP', vhost.get_protocol())
        self.assertRaises(AttributeError, vhost.set_protocol, None)

    def test_log(self):
        vhost = Vhost('test vhost', 80, 'HTTP', '/www', '/system',
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

    def test_ip(self):
        vhost = Vhost('test vhost', 80, 'HTTP', '/www', '/system',
                      Log('ACCESSLOG'), Log('WARNINGLOG'))
        self.assertEqual(set(), vhost.get_ip())
        vhost.add_ip('10.0.0.0/8')
        vhost.add_ip('192.168.0.0/16')
        self.assertEqual(set(['10.0.0.0/8', '192.168.0.0/16']), vhost.get_ip())
        vhost.remove_ip('10.0.0.0/8')
        self.assertEqual(set(['192.168.0.0/16']), vhost.get_ip())
        vhost = Vhost('test vhost', 80, 'HTTP', '/www', '/system',
                      Log('ACCESSLOG'), Log('WARNINGLOG'),
                      set(['127.0.0.0/8', '192.168.0.0/16']))
        self.assertEqual(set(['127.0.0.0/8', '192.168.0.0/16']), vhost.get_ip())

    def test_host(self):
        vhost = Vhost('test vhost', 80, 'HTTP', '/www', '/system',
                      Log('ACCESSLOG'), Log('WARNINGLOG'))h
        self.assertEqual(set(), vhost.get_host())
        vhost.add_host('foo.bar.com')
        vhost.add_host('test.me.org')
        self.assertEqual(set(['foo.bar.com', 'test.me.org']), vhost.get_host())
        vhost.remove_host('foo.bar.com')
        self.assertEqual(set(['test.me.org']), vhost.get_host())
        vhost = Vhost('test vhost', 80, 'HTTP', '/www', '/system',
                      Log('ACCESSLOG'), Log('WARNINGLOG'),
                      set(['127.0.0.0/8', '192.168.0.0/16']),
                      set(['test.me.org']))
        self.assertEqual(set(['test.me.org']), vhost.get_host())

    def test_host_use_regex(self):
        vhost = Vhost('test vhost', 80, 'HTTP', '/www', '/system',
                      Log('ACCESSLOG'), Log('WARNINGLOG'))h
        self.assertEqual(None, vhost.get_host_use_regex())
        vhost.set_host_use_regex(True)
        self.assertEqual(True, vhost.get_use_regex())
        vhost.set_host_use_regex(None)
        self.assertEqual(None, vhost.get_host_use_regex())
        vhost = Vhost('test vhost', 80, 'HTTP', '/www', '/system',
                      Log('ACCESSLOG'), Log('WARNINGLOG'),
                      set(['127.0.0.0/8', '192.168.0.0/16']),
                      set(['host.domain']), True)
        self.assertEqual(True, vhost.get_use_regex())
