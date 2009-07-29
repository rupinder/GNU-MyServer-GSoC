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
from MyServer.pycontrollib.log import Log, Stream
from MyServer.pycontrollib.vhost import VHost

class FilterTreeView(gtk.TreeView):
    def __init__(self):
        gtk.TreeView.__init__(self, gtk.ListStore(
                gobject.TYPE_STRING))
        model = self.get_model()
        def edited_handler(cell, path, text, model):
            model[path][0] = text
        renderer = gtk.CellRendererText()
        renderer.set_property('editable', True)
        renderer.connect('edited', edited_handler, model)
        column = gtk.TreeViewColumn('Filter')
        column.pack_start(renderer)
        column.add_attribute(renderer, 'text', 0)
        self.append_column(column)
        self.scroll = gtk.ScrolledWindow()
        self.scroll.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        self.scroll.set_shadow_type(gtk.SHADOW_OUT)
        self.scroll.set_border_width(5)
        self.scroll.add(self)

class StreamTreeView(gtk.TreeView):
    def __init__(self, filter_tree):
        gtk.TreeView.__init__(self, gtk.ListStore(
                gobject.TYPE_STRING, # stream location
                gobject.TYPE_PYOBJECT)) # list of filters
        self.filter_model = filter_tree.get_model()
        model = self.get_model()
        def edited_handler(cell, path, text, model):
            model[path][0] = text
        renderer = gtk.CellRendererText()
        renderer.set_property('editable', True)
        renderer.connect('edited', edited_handler, model)
        column = gtk.TreeViewColumn('Stream')
        column.pack_start(renderer)
        column.add_attribute(renderer, 'text', 0)
        self.append_column(column)
        self.scroll = gtk.ScrolledWindow()
        self.scroll.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        self.scroll.set_shadow_type(gtk.SHADOW_OUT)
        self.scroll.set_border_width(5)
        self.scroll.add(self)

        self.last_selected = None
        self.connect('cursor-changed', self.cursor_changed)

    def clear(self):
        self.filter_model.clear()
        self.last_selected = None

    def save_changed(self):
        model = self.filter_model
        if self.last_selected is not None:
            filters = []
            i = model.iter_children(None)
            while i is not None: # iterate over filters
                filters.append(model[i][0])
                i = model.iter_next(i)
            row = self.get_model()[self.last_selected]
            row[1] = filters

    def cursor_changed(self, tree):
        self.save_changed()

        self.clear()
        model = self.filter_model

        self.last_selected = current = \
            tree.get_selection().get_selected()[1]
        row = tree.get_model()[current]
        for element in row[1]: # add all filters
            model.append((element, ))

class LogTreeView(gtk.TreeView):
    def __init__(self, stream_tree):
        gtk.TreeView.__init__(self, gtk.ListStore(
                gobject.TYPE_STRING, # log name
                gobject.TYPE_PYOBJECT)) # list of streams
        self.stream_tree = stream_tree
        model = self.get_model()
        def edited_handler(cell, path, text, model):
            model[path][0] = text
        renderer = gtk.CellRendererText()
        renderer.set_property('editable', True)
        renderer.connect('edited', edited_handler, model)
        column = gtk.TreeViewColumn('Log')
        column.pack_start(renderer)
        column.add_attribute(renderer, 'text', 0)
        self.append_column(column)
        self.scroll = gtk.ScrolledWindow()
        self.scroll.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        self.scroll.set_shadow_type(gtk.SHADOW_OUT)
        self.scroll.set_border_width(5)
        self.scroll.add(self)
        
        self.last_selected = None
        self.connect('cursor-changed', self.cursor_changed)

    def clear(self):
        self.stream_tree.clear()
        self.last_selected = None
        self.stream_tree.get_model().clear()
        
    def save_changed(self):
        self.stream_tree.save_changed()
        model = self.stream_tree.get_model()
        if self.last_selected is not None:
            streams = []
            i = model.iter_children(None)
            while i is not None: # iterate over streams
                row = model[i]
                streams.append((row[0], row[1], ))
                i = model.iter_next(i)
            row = self.get_model()[self.last_selected]
            row[1] = streams

    def cursor_changed(self, tree):
        self.save_changed()

        self.clear()
        model = self.stream_tree.get_model()

        self.last_selected = current = \
            tree.get_selection().get_selected()[1]
        row = tree.get_model()[current]
        for stream in row[1]:
            model.append(stream)

    def get_logs(self):
        self.save_changed()
        model = self.get_model()
        logs = []
        i = model.iter_children(None)
        while i is not None: # iterate over logs
            row = model[i]
            logs.append((row[0], row[1], ))
            i = model.iter_next(i)
        return logs

    def set_logs(self, logs):
        self.clear()
        self.last_selected = None
        model = self.get_model()
        model.clear()
        for log in logs:
            model.append((log[0], log[1], ))

class VHostTreeView(gtk.TreeView):
    def __init__(self):
        gtk.TreeView.__init__(self, gtk.ListStore(
                gobject.TYPE_STRING, # vhost name
                gobject.TYPE_PYOBJECT, # vhost single attributes
                gobject.TYPE_PYOBJECT, # vhost attribute lists
                gobject.TYPE_PYOBJECT)) # vhost logs
        model = self.get_model()
        def vhost_edited_handler(cell, path, text, data):
            model = data
            model[path][0] = text
        vhost_renderer = gtk.CellRendererText()
        vhost_renderer.set_property('editable', True)
        vhost_renderer.connect('edited', vhost_edited_handler, model)
        vhost_column = gtk.TreeViewColumn('VHost')
        vhost_column.pack_start(vhost_renderer)
        vhost_column.add_attribute(vhost_renderer, 'text', 0)
        self.append_column(vhost_column)

        self.scroll = gtk.ScrolledWindow()
        self.scroll.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        self.scroll.set_shadow_type(gtk.SHADOW_OUT)
        self.scroll.set_border_width(5)
        self.scroll.add(self)

    def set_up(self, vhosts):
        '''Fill model with data provided as list of Vhosts.'''
        model = self.get_model()
        for vhost in vhosts:
            attributes = {}
            for attribute in GUIConfig.vhost_attributes:
                a = getattr(vhost, 'get_' + attribute)()
                if a is not None:
                    attributes[attribute] = (str(a), True, )
            vhost_lists = {}
            for vhost_list in GUIConfig.vhost_lists:
                vhost_list = vhost_list[0]
                l = getattr(vhost, 'get_' + vhost_list)()
                m = []
                for element in l:
                    if isinstance(l, dict):
                        m.append((element, l[element], ))
                    else:
                        m.append((element, ))
                vhost_lists[vhost_list] = m
            logs = []
            for log in vhost.get_logs():
                streams = []
                for stream in log.get_streams():
                    filters = stream.get_filters()
                    streams.append((stream.get_location(), filters, ))
                logs.append((log.get_log_type(), streams, ))
            model.append((getattr(vhost, 'get_' + GUIConfig.vhost_name)(),
                          attributes,
                          vhost_lists,
                          logs, ))

class VHostTable(gtk.Table):
    def __init__(self, tree):
        gtk.Table.__init__(self, len(GUIConfig.vhost_attributes) +
                           3 * len(GUIConfig.vhost_lists) + 3, 4)

        tree.connect('cursor-changed', self.cursor_changed)
        self.last_selected = None

        self.attributes = {}
        i = 0
        for attribute in GUIConfig.vhost_attributes: # Add single attributes
            label = gtk.Label(attribute)
            entry = gtk.Entry()
            check = gtk.CheckButton()
            self.attributes[attribute] = (entry, check, )
            self.attach(label, 0, 1, i, i + 1, yoptions = gtk.FILL)
            self.attach(entry, 1, 3, i, i + 1, yoptions = gtk.FILL)
            self.attach(check, 3, 4, i, i + 1, gtk.FILL, gtk.FILL)
            i += 1
            
        self.vhost_lists = {}
        for vhost_list in GUIConfig.vhost_lists: # Add attribute lists
            name = vhost_list[0]
            column_names = []
            columns = []
            defaults = []
            for element in vhost_list[1]:
                column_names.append(element[0])
                columns.append(element[1])
                defaults.append(element[2])
            tree = gtk.TreeView(gtk.ListStore(*(columns)))
            tree_model = tree.get_model()
            def tree_edited_handler(cell, path, text, data):
                model, col_index = data
                model[path][col_index] = text
            col_index = 0
            for column in column_names:
                tree_renderer = gtk.CellRendererText()
                tree_renderer.set_property('editable', True)
                tree_renderer.connect('edited', tree_edited_handler,
                                      (tree_model, col_index, ))
                tree_column = gtk.TreeViewColumn(column)
                tree_column.pack_start(tree_renderer)
                tree_column.add_attribute(tree_renderer, 'text', col_index)
                tree.append_column(tree_column)
                col_index += 1
            tree_scroll = gtk.ScrolledWindow()
            tree_scroll.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
            tree_scroll.set_shadow_type(gtk.SHADOW_OUT)
            tree_scroll.set_border_width(5)
            tree_scroll.add(tree)

            def add_to_tree(button, data):
                model, defaults = data
                model.append(defaults)
            add_button = gtk.Button('Add')
            add_button.connect('clicked', add_to_tree, (tree_model, defaults, ))
            def remove_from_tree(button, tree):
                model, selected = tree.get_selection().get_selected()
                if selected is not None:
                    model.remove(selected)
            remove_button = gtk.Button('Remove')
            remove_button.connect('clicked', remove_from_tree, tree)

            self.vhost_lists[name] = tree_model
            self.attach(tree_scroll, 0, 2, i, i + 3)
            self.attach(add_button, 2, 3, i, i + 1, yoptions = gtk.FILL)
            self.attach(remove_button, 2, 3, i + 1, i + 2, yoptions = gtk.FILL)
            self.attach(gtk.Label(), 2, 3, i + 2, i + 3)
            i += 3

        # Add logs
        filter_tree = FilterTreeView()
        stream_tree = StreamTreeView(filter_tree)
        self.log_tree = LogTreeView(stream_tree)
        self.log_model = self.log_tree.get_model()

        def add_stream(button, model):
            model.append(('', [], ))
        add_stream_button = gtk.Button('Add stream')
        add_stream_button.connect(
            'clicked', add_stream, stream_tree.get_model())
        def add_filter(button, model):
            model.append(('', ))
        add_filter_button = gtk.Button('Add filter')
        add_filter_button.connect(
            'clicked', add_filter, filter_tree.get_model())

        def remove_selected(button, tree):
            model, selected = tree.get_selection().get_selected()
            if selected is not None:
                model.remove(selected)
        remove_stream_button = gtk.Button('Remove stream')
        remove_stream_button.connect('clicked', remove_selected, stream_tree)
        remove_filter_button = gtk.Button('Remove filter')
        remove_filter_button.connect('clicked', remove_selected, filter_tree)

        button_grid = gtk.Table(2, 2)
        button_grid.attach(add_stream_button, 0, 1, 0, 1)
        button_grid.attach(add_filter_button, 1, 2, 0, 1)
        button_grid.attach(remove_stream_button, 0, 1, 1, 2)
        button_grid.attach(remove_filter_button, 1, 2, 1, 2)

        self.attach(self.log_tree.scroll, 0, 2, i, i + 3)
        self.attach(button_grid, 2, 3, i, i + 1, yoptions = gtk.FILL)
        self.attach(stream_tree.scroll, 2, 3, i + 1, i + 2)
        self.attach(filter_tree.scroll, 2, 3, i + 2, i + 3)

    def add_log(self):
        '''Add a log to currently selected VHost.'''
        self.log_model.append(('', [], ))
            
    def clear(self):
        '''Clear input widgets.'''
        for entry, check in self.attributes.itervalues():
            entry.set_text('')
            check.set_active(False)
        for model in self.vhost_lists.itervalues():
            model.clear()
        self.log_tree.clear()
        self.log_tree.get_model().clear()
        self.last_selected = None

    def save_changed(self, tree):
        '''Save data from input widgets to model.'''
        if self.last_selected is not None:
            attributes = {}
            for attribute in self.attributes:
                entry, check = self.attributes[attribute]
                attributes[attribute] = (entry.get_text(), check.get_active(), )
            vhost_lists = {}
            for vhost_list in GUIConfig.vhost_lists:
                container = vhost_lists[vhost_list[0]] = []
                model = self.vhost_lists[vhost_list[0]]
                i = model.iter_children(None)
                while i is not None: # iterate over list elements
                    row = []
                    for counter in xrange(len(vhost_list[1])):
                        row.append(model[i][counter])
                    container.append(tuple(row))
                    i = model.iter_next(i)
            row = tree.get_model()[self.last_selected]
            row[1] = attributes
            row[2] = vhost_lists
            row[3] = self.log_tree.get_logs()

    def cursor_changed(self, tree):
        self.save_changed(tree)

        self.clear()

        self.last_selected = current = tree.get_selection().get_selected()[1]
        row = tree.get_model()[current]
        for attribute in row[1]:
            entry, check = self.attributes[attribute]
            entry.set_text(row[1][attribute][0])
            check.set_active(row[1][attribute][1])
        for vhost_list in row[2]:
            model = self.vhost_lists[vhost_list]
            for element in row[2][vhost_list]:
                model.append(element)
        self.log_tree.set_logs(row[3])

    def make_def(self, tree):
        '''Export all data as list of VHosts.'''
        self.save_changed(tree)
        model = tree.get_model()
        vhosts = []
        i = model.iter_children(None)
        while i is not None: # iterate over VHosts
            row = model[i]
            logs = []
            for log in row[3]:
                streams = []
                for stream in log[1]:
                    streams.append(Stream(stream[0], filters = stream[1]))
                logs.append(Log(log[0], streams = streams))
            vhost = VHost(row[0], logs = logs)
            for attribute in row[1]:
                text, enabled = row[1][attribute]
                if enabled:
                    getattr(vhost, 'set_' + attribute)(text)
            for vhost_list in row[2]:
                for entry in row[2][vhost_list]:
                    getattr(vhost, 'add_' + vhost_list)(*entry)
            vhosts.append(vhost)
            i = model.iter_next(i)
        return vhosts
