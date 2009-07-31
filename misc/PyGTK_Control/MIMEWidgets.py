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
import gobject
import GUIConfig
from MyServer.pycontrollib.mimetypes import MIMEType

class MimeTreeView(gtk.TreeView):
    def __init__(self):
        gtk.TreeView.__init__(self, gtk.ListStore(
                gobject.TYPE_STRING, # mime name
                gobject.TYPE_PYOBJECT, # mime single attributes
                gobject.TYPE_PYOBJECT, # mime attribute lists
                gobject.TYPE_PYOBJECT)) # mime definitions
        model = self.get_model()
        def mime_edited_handler(cell, path, text, data):
            model = data
            model[path][0] = text
        mime_renderer = gtk.CellRendererText()
        mime_renderer.set_property('editable', True)
        mime_renderer.connect('edited', mime_edited_handler, model)
        mime_column = gtk.TreeViewColumn('MIME Type')
        mime_column.pack_start(mime_renderer)
        mime_column.add_attribute(mime_renderer, 'text', 0)
        self.append_column(mime_column)

        self.scroll = gtk.ScrolledWindow()
        self.scroll.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        self.scroll.set_shadow_type(gtk.SHADOW_OUT)
        self.scroll.set_border_width(5)
        self.scroll.add(self)

    def set_up(self, MIME_types):
        '''Fill model with data provided as list of MIME types.'''
        model = self.get_model()
        for mime in MIME_types:
            attributes = {}
            for attribute in GUIConfig.mime_attributes:
                a = getattr(mime, 'get_' + attribute)()
                if a is not None:
                    attributes[attribute] = (a, True, )
            mime_lists = {}
            for mime_list in GUIConfig.mime_lists:
                mime_lists[mime_list] = getattr(mime, 'get_' + mime_list)()
            model.append((getattr(mime, 'get_' + GUIConfig.mime_name)(),
                          attributes,
                          mime_lists,
                          mime.get_definitions(), ))

class MimeTable(gtk.Table):
    def __init__(self, tree, def_tree, def_table):
        gtk.Table.__init__(self, len(GUIConfig.mime_attributes) +
                           3 * len(GUIConfig.mime_lists), 3)

        tree.connect('cursor-changed', self.cursor_changed)
        self.last_selected = None
        self.def_tree = def_tree
        self.def_table = def_table

        self.attributes = {}
        i = 0
        for attribute in GUIConfig.mime_attributes:
            label = gtk.Label(attribute)
            entry = gtk.Entry()
            check = gtk.CheckButton()
            self.attributes[attribute] = (entry, check, )
            self.attach(label, 0, 1, i, i + 1, yoptions = gtk.FILL)
            self.attach(entry, 1, 2, i, i + 1, yoptions = gtk.FILL)
            self.attach(check, 2, 3, i, i + 1, gtk.FILL, gtk.FILL)
            i += 1
        self.mime_lists = {}
        for mime_list in GUIConfig.mime_lists:
            tree = gtk.TreeView(gtk.ListStore(gobject.TYPE_STRING))
            tree_model = tree.get_model()
            def tree_edited_handler(cell, path, text, data):
                model = data
                model[path][0] = text
            tree_renderer = gtk.CellRendererText()
            tree_renderer.set_property('editable', True)
            tree_renderer.connect('edited', tree_edited_handler, tree_model)
            tree_column = gtk.TreeViewColumn(mime_list)
            tree_column.pack_start(tree_renderer)
            tree_column.add_attribute(tree_renderer, 'text', 0)
            tree.append_column(tree_column)
            tree_scroll = gtk.ScrolledWindow()
            tree_scroll.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
            tree_scroll.set_shadow_type(gtk.SHADOW_OUT)
            tree_scroll.set_border_width(5)
            tree_scroll.add(tree)

            def add_to_tree(button, model):
                model.append(('', ))
            add_button = gtk.Button('Add')
            add_button.connect('clicked', add_to_tree, tree_model)
            def remove_from_tree(button, tree):
                model, selected = tree.get_selection().get_selected()
                if selected is not None:
                    model.remove(selected)
            remove_button = gtk.Button('Remove')
            remove_button.connect('clicked', remove_from_tree, tree)

            self.mime_lists[mime_list] = tree_model
            self.attach(tree_scroll, 1, 2, i, i + 3)
            self.attach(add_button, 0, 1, i, i + 1, yoptions = gtk.FILL)
            self.attach(remove_button, 0, 1, i + 1, i + 2, yoptions = gtk.FILL)
            self.attach(gtk.Label(), 0, 1, i + 2, i + 3)
            i += 3
            
    def clear(self):
        '''Clear input widgets (including connected definition widgets).'''
        for entry, check in self.attributes.itervalues():
            entry.set_text('')
            check.set_active(False)
        for model in self.mime_lists.itervalues():
            model.clear()
        self.def_table.clear()
        self.def_tree.get_model().clear()
        self.last_selected = None

    def save_changed(self, tree):
        '''Save data from input widgets to model.'''
        if self.last_selected is not None:
            attributes = {}
            for attribute in self.attributes:
                entry, check = self.attributes[attribute]
                attributes[attribute] = (entry.get_text(), check.get_active(), )
            mime_lists = {}
            for mime_list in GUIConfig.mime_lists:
                container = mime_lists[mime_list] = []
                model = self.mime_lists[mime_list]
                i = model.iter_children(None)
                while i is not None: # iterate over list elements
                    container.append(model.get_value(i, 0))
                    i = model.iter_next(i)
            row = tree.get_model()[self.last_selected]
            row[1] = attributes
            row[2] = mime_lists
            row[3] = self.def_table.make_def(self.def_tree)

    def cursor_changed(self, tree):
        self.save_changed(tree)

        self.clear()

        self.last_selected = current = tree.get_selection().get_selected()[1]
        row = tree.get_model()[current]
        for attribute in row[1]:
            entry, check = self.attributes[attribute]
            entry.set_text(row[1][attribute][0])
            check.set_active(row[1][attribute][1])
        for mime_list in row[2]:
            model = self.mime_lists[mime_list]
            for element in row[2][mime_list]:
                model.append((element, ))
        self.def_tree.set_up(row[3], False)

    def make_def(self, tree):
        '''Export all data as list of MIME types.'''
        self.save_changed(tree)
        model = tree.get_model()
        mimes = []
        i = model.iter_children(None)
        while i is not None: # iterate over MIME types
            row = model[i]
            mime = MIMEType(row[0], definitions = row[3])
            for attribute in row[1]:
                text, enabled = row[1][attribute]
                if enabled:
                    getattr(mime, 'set_' + attribute)(text)
            for mime_list in row[2]:
                for entry in row[2][mime_list]:
                    getattr(mime, 'add_' + mime_list[:-1])(entry)
            mimes.append(mime)
            i = model.iter_next(i)
        return mimes
