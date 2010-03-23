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

import gtk
from MyServer.pycontrollib.controller import Controller
from MyServer.GUI.AboutWindow import logo

class Connection():
    def __init__(self, parent):
        self.parent = parent
        self.window = gtk.Dialog('Connect',
                                 None,
                                 gtk.DIALOG_MODAL | \
                                     gtk.DIALOG_DESTROY_WITH_PARENT,
                                 (gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT,
                                  gtk.STOCK_OK, gtk.RESPONSE_ACCEPT))

        self.window.set_icon_list(logo)

        self.host_label = gtk.Label('Host:')
        self.host_entry = gtk.Entry()
        self.port_label = gtk.Label('Port:')
        self.port_entry = gtk.Entry()
        self.username_label = gtk.Label('Username:')
        self.username_entry = gtk.Entry()
        self.password_label = gtk.Label('Password:')
        self.password_entry = gtk.Entry()
        table = gtk.Table(4, 2)
        table.attach(self.host_label, 0, 1, 0, 1, gtk.FILL, gtk.FILL)
        table.attach(self.host_entry, 1, 2, 0, 1, yoptions = gtk.FILL)
        table.attach(self.port_label, 0, 1, 1, 2, gtk.FILL, gtk.FILL)
        table.attach(self.port_entry, 1, 2, 1, 2, yoptions = gtk.FILL)
        table.attach(self.username_label, 0, 1, 2, 3, gtk.FILL, gtk.FILL)
        table.attach(self.username_entry, 1, 2, 2, 3, yoptions = gtk.FILL)
        table.attach(self.password_label, 0, 1, 3, 4, gtk.FILL, gtk.FILL)
        table.attach(self.password_entry, 1, 2, 3, 4, yoptions = gtk.FILL)
        self.window.vbox.pack_start(table)
        self.window.connect('response', self.on_connectiondialog_response)
        self.window.show_all()
        self.window.run()

    def on_connectiondialog_response(self, widget, response):
        if response == gtk.RESPONSE_ACCEPT:
            self.parent.controller = Controller(
                self.get_host(), self.get_port(),
                self.get_username(), self.get_password())
            self.parent.controller.connect()
        self.window.destroy()

    def get_host(self):
        return self.host_entry.get_text()

    def get_port(self):
        return self.port_entry.get_text()

    def get_username(self):
        return self.username_entry.get_text()

    def get_password(self):
        return self.password_entry.get_text()
