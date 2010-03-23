# -*- coding: utf-8 -*-
copyright_notice = '''
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

import os
import gtk

with open(os.path.join(os.path.dirname(__file__), 'COPYING')) as file:
    gpl_v3 = file.read()
with open(os.path.join(os.path.dirname(__file__), 'version')) as file:
    version = 'v' + file.read()

logo = gtk.gdk.pixbuf_new_from_file(os.path.join(os.path.dirname(__file__), 'myserverlogo.png'))


class About():
    '''GNU MyServer Control about window.'''

    def __init__(self):
        self.window = gtk.AboutDialog()
        self.window.connect('response', self.on_aboutdialog_response)
        self.window.set_program_name('GNU MyServer Control')
        self.window.set_icon_list(logo)
        self.window.set_version(version)
        self.window.set_copyright(copyright_notice)
        self.window.set_license(gpl_v3)
        self.window.set_website('http://www.gnu.org/software/myserver/')
        self.window.show_all()

    def on_aboutdialog_response(self, widget, response):
        widget.destroy()
