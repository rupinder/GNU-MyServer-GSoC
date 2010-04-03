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
from gtk import gdk
import gobject
from MyServer.pycontrollib.browser import LocalFileBrowser

class BrowserTreeView(gtk.TreeView):
    def __init__(self, callback):
        gtk.TreeView.__init__(self, gtk.ListStore(
                gdk.Pixbuf,
                gobject.TYPE_STRING))
        self.callback = callback
        self.model = self.get_model()
        renderer = gtk.CellRendererPixbuf()
        column = gtk.TreeViewColumn()
        column.pack_start(renderer)
        column.add_attribute(renderer, 'pixbuf', 0)
        self.append_column(column)
        renderer = gtk.CellRendererText()
        column = gtk.TreeViewColumn('File')
        column.pack_start(renderer)
        column.add_attribute(renderer, 'text', 1)
        self.append_column(column)

        self.browser = LocalFileBrowser()
        self.browser.show_hidden(False)
        icon_theme = gtk.icon_theme_get_default()
        self.dir_icon = icon_theme.load_icon('gtk-directory', 16, 0)
        self.update()
        self.connect('row-activated', self.change_dir)
        callback(self.browser)

        self.scroll = gtk.ScrolledWindow()
        self.scroll.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        self.scroll.set_shadow_type(gtk.SHADOW_OUT)
        self.scroll.set_border_width(5)
        self.scroll.add(self)

    def update(self):
        '''Show contents of current directory.'''
        self.model.clear()
        for d in self.browser.list_dir():
            self.model.append((self.dir_icon, d, ))

    def change_dir(self, widget, path, view_column):
        self.browser.change_dir(self.model[path][1])
        self.update()
        self.callback(self.browser)

class BrowserTable(gtk.Table):
    def __init__(self, callback):
        gtk.Table.__init__(self, 2, 1)

        self.browser_tree = BrowserTreeView(callback)

        def set_show_hidden(button):
            self.browser_tree.browser.show_hidden(button.get_active())
            self.browser_tree.update()
        hide_check = gtk.CheckButton ('Show Hidden Directories')
        hide_check.connect('toggled', set_show_hidden)

        self.attach(hide_check, 0, 1, 0, 1, yoptions = gtk.FILL)
        self.attach(self.browser_tree.scroll, 0, 1, 1, 2)
