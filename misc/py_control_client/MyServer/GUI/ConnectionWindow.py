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
import gtk.glade

class Connection():
    def __init__(self):
        self.gladefile = 'PyGTKControl.glade'
        self.widgets = gtk.glade.XML(self.gladefile, 'connectiondialog')
        self.widgets.signal_autoconnect(self)

    def destroy(self):
        '''Destroys this widget.'''
        self.widgets.get_widget('connectiondialog').destroy()

    def on_cancel_button_clicked(self, widget):
        self.destroy()

    def get_host(self):
        return self.widgets.get_widget('host_entry').get_text()

    def get_port(self):
        return self.widgets.get_widget('port_entry').get_text()

    def get_username(self):
        return self.widgets.get_widget('username_entry').get_text()

    def get_password(self):
        return self.widgets.get_widget('password_entry').get_text()
