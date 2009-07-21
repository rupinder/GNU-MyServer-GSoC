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

from __future__ import print_function
import gtk
import gobject
import gtk.glade
import GUIConfig
from MyServer.pycontrollib.config import MyServerConfig
from MyServer.pycontrollib.controller import Controller
from MyServer.pycontrollib.definition import DefinitionElement, DefinitionTree

class About():
    '''GNU MyServer Control about window.'''

    def __init__(self):
        self.gladefile = 'PyGTKControl.glade'
        self.widgets = gtk.glade.XML(self.gladefile, 'aboutdialog')
        self.widgets.signal_autoconnect(self)

    def on_aboutdialog_response(self, widget, response):
        widget.destroy()

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

class EditionTable(gtk.Table):
    def __init__(self, tree):
        gtk.Table.__init__(self, 8, 3)
        
        tree.connect('cursor-changed', self.cursor_changed)
        self.last_selected = None

        enabled_label = gtk.Label('enabled:')
        self.enabled_field = enabled_checkbutton = gtk.CheckButton()
        enabled_checkbutton.set_tooltip_text('If not active, definition won\'t be included in saved configuration.')
        self.attach(enabled_label, 0, 1, 0, 1, gtk.FILL, gtk.FILL)
        self.attach(enabled_checkbutton, 1, 3, 0, 1, yoptions = gtk.FILL)

        value_label = gtk.Label('value:')
        self.value_field = value_entry = gtk.Entry()
        self.value_check_field = value_checkbutton = gtk.CheckButton()
        value_checkbutton.set_tooltip_text('If active value will be used.')
        value_checkbutton.set_active(True)
        value_checkbutton.connect(
            'toggled',
            lambda button: value_entry.set_editable(button.get_active()))
        self.attach(value_label, 0, 1, 1, 2, gtk.FILL, gtk.FILL)
        self.attach(value_entry, 1, 2, 1, 2, yoptions = gtk.FILL)
        self.attach(value_checkbutton, 2, 3, 1, 2, gtk.FILL, gtk.FILL)

        add_definition_button = gtk.Button('add sub-definition')
        self.attach(add_definition_button, 0, 3, 2, 3, yoptions = gtk.FILL)
        
        remove_definition_button = gtk.Button('remove this definition')
        self.attach(remove_definition_button, 0, 3, 3, 4, yoptions = gtk.FILL)

        add_attribute_button = gtk.Button('add attribute')
        remove_attribute_button = gtk.Button('remove attribute')
        add_attribute_button.connect(
            'clicked',
            lambda button: attributes_model.append(('', '', )))
        def remove_attribute(button):
            model, selected = attributes_list.get_selection().get_selected()
            if selected is not None:
                model.remove(selected)
        remove_attribute_button.connect('clicked', remove_attribute)
        self.attach(add_attribute_button, 0, 3, 4, 5, yoptions = gtk.FILL)
        self.attach(remove_attribute_button, 0, 3, 5, 6, yoptions = gtk.FILL)

        attributes_label = gtk.Label('attributes:')
        attributes_list = gtk.TreeView(gtk.ListStore(gobject.TYPE_STRING,
                                                     gobject.TYPE_STRING))
        attributes_scroll = gtk.ScrolledWindow()
        attributes_scroll.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        attributes_scroll.set_shadow_type(gtk.SHADOW_OUT)
        attributes_scroll.set_border_width(5)
        attributes_scroll.add(attributes_list)
        self.attributes_field = attributes_model = attributes_list.get_model()

        def edited_handler(cell, path, text, data):
            model, col = data
            model[path][col] = text
        variable_column = gtk.TreeViewColumn('variable')
        variable_renderer = gtk.CellRendererText()
        variable_renderer.set_property('editable', True)
        variable_renderer.connect('edited', edited_handler,
                                  (attributes_model, 0, ))
        variable_column.pack_start(variable_renderer)
        variable_column.add_attribute(variable_renderer, 'text', 0)
        
        value_column = gtk.TreeViewColumn('value')
        value_renderer = gtk.CellRendererText()
        value_renderer.set_property('editable', True)
        value_renderer.connect('edited', edited_handler,
                                  (attributes_model, 1, ))
        value_column.pack_start(value_renderer)
        value_column.add_attribute(value_renderer, 'text', 1)
        
        attributes_list.append_column(variable_column)
        attributes_list.append_column(value_column)
        self.attach(attributes_label, 0, 3, 6, 7, yoptions = gtk.FILL)
        self.attach(attributes_scroll, 0, 3, 7, 8)

    def clear(self):
        self.enabled_field.set_active(False)
        self.value_field.set_text('')
        self.value_check_field.set_active(False)
        self.attributes_field.clear()
        self.last_selected = None

    def save_changed(self, tree):
        if self.last_selected is not None:
            attributes = {}
            i = self.attributes_field.iter_children(None)
            while i is not None: # iterate over attributes
                attributes[self.attributes_field.get_value(i, 0)] = \
                    self.attributes_field.get_value(i, 1)
                i = self.attributes_field.iter_next(i)
            row = tree.get_model()[self.last_selected]
            row[2] = self.enabled_field.get_active()
            row[3] = self.value_field.get_text()
            row[4] = self.value_check_field.get_active()
            row[5] = attributes
            
    def cursor_changed(self, tree):
        self.save_changed(tree)
            
        self.clear()
        
        self.last_selected = current = self.get_selected(tree)
        row = tree.get_model()[current]
        self.enabled_field.set_active(row[2])
        self.value_field.set_text(row[3])
        self.value_check_field.set_active(row[4])
        for attribute in row[5]:
            self.attributes_field.append((attribute, row[5][attribute], ))

    def get_selected(self, tree):
        model, selected = tree.get_selection().get_selected()
        return selected
    
class PyGTKControl():
    '''GNU MyServer Control main window.'''

    def __init__(self):
        self.gladefile = 'PyGTKControl.glade'
        self.widgets = gtk.glade.XML(self.gladefile, 'window')
        self.widgets.signal_autoconnect(self)
        self.construct_options()
        self.chooser = None # Active file chooser
        self.path = None # path of currently edited file
        self.controller = None

    def on_window_destroy(self, widget):
        '''Exits program.'''
        gtk.main_quit()

    def on_quit_menu_item_activate(self, widget):
        '''Exits program.'''
        gtk.main_quit()

    def on_about_menu_item_activate(self, widget):
        '''Shows about window.'''
        About()

    def on_connect_menu_item_activate(self, widget):
        '''Show connection dialog.'''
        dialog = Connection()
        def connect(widget):
            self.controller = Controller(
                dialog.get_host(), dialog.get_port(),
                dialog.get_username(), dialog.get_password())
            self.controller.connect()
            dialog.destroy()
        dialog.widgets.get_widget('ok_button').connect('clicked', connect)
        dialog.widgets.get_widget('connectiondialog').show()

    def on_disconnect_menu_item_activate(self, widget):
        '''Disconnect from server.'''
        if self.controller is not None:
            self.controller.disconnect()
        self.controller = None

    def on_get_config_menu_item_activate(self, widget):
        '''Get config from remote server.'''
        if self.controller is not None:
            self.set_up(self.controller.get_server_configuration())

    def on_put_config_menu_item_activate(self, widget):
        '''Put config on remote server.'''
        if self.controller is not None:
            self.controller.put_server_configuration(self.get_current_config())

    def on_open_menu_item_activate(self, widget):
        '''Open local configuration file.'''
        if self.chooser is not None:
            self.chooser.destroy()
        self.chooser = gtk.FileChooserDialog(
            'Open configuration file.',
            buttons = (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                       gtk.STOCK_OPEN, gtk.RESPONSE_OK))
        def handle_response(widget, response):
            if response == gtk.RESPONSE_OK:
                self.path = self.chooser.get_filename()
                with open(self.path) as f:
                    conf = MyServerConfig.from_string(f.read())
                self.set_up(conf)
            self.chooser.destroy()
        self.chooser.connect('response', handle_response)
        self.chooser.show()

    def on_save_as_menu_item_activate(self, widget):
        '''Save configuration as local file.'''
        if self.chooser is not None:
            self.chooser.destroy()
        self.chooser = gtk.FileChooserDialog(
            'Save configuration file.',
            action = gtk.FILE_CHOOSER_ACTION_SAVE,
            buttons = (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                       gtk.STOCK_SAVE_AS, gtk.RESPONSE_OK))
        def handle_response(widget, response):
            if response == gtk.RESPONSE_OK:
                self.path = self.chooser.get_filename()
                self.on_save_menu_item_activate(widget)
            self.chooser.destroy()
        self.chooser.connect('response', handle_response)
        self.chooser.show()

    def on_save_menu_item_activate(self, widget):
        '''Save configuration to file.'''
        if self.path is None:
            self.on_save_as_menu_item_activate(widget)
        else:
            config = self.get_current_config()
            with open(self.path, 'w') as f:
                f.write(str(config))
    
    def get_current_config(self):
        '''Returns current configuration as MyServerConfig instance.'''

        def make_def(current, model):
            row = model[current]
            name = row[0]
            enabled = row[2]
            value = row[3]
            value_check = row[4]
            attributes = row[5]
            if value_check:
                attributes['value'] = value
            if not enabled:
                return None
            i = model.iter_children(current)
            if i is None:
                return DefinitionElement(name, attributes)
            else:
                definitions = []
                while i is not None: # iterate over children
                    definition = make_def(i, model)
                    if definition is not None:
                        definitions.append(definition)
                    i = model.iter_next(i)
                return DefinitionTree(name, definitions, attributes)
            
        definitions = []
        for tab in self.tabs:
            table, tree = self.tabs[tab]
            model = tree.get_model()
            table.save_changed(tree)
            i = model.iter_children(None)
            while i is not None: # iterate over options
                definition = make_def(i, model)
                if definition is not None:
                    definitions.append(definition)
                i = model.iter_next(i)
        return MyServerConfig(definitions)

    def on_new_menu_item_activate(self, widget = None):
        '''Clears configuration.'''
        if widget is not None:
            self.path = None
        for tab in self.tabs:
            table, tree = self.tabs[tab]
            model = tree.get_model()
            table.clear()
            tree.get_selection().unselect_all()
            i = model.iter_children(None)
            while i is not None: # iterate over options
                row = model[i]
                row[2] = False
                row[3] = ''
                row[4] = False
                row[5] = {}
                child = model.iter_children(i)
                if child is not None: # remove node children
                    while model.remove(child):
                        pass
                i = model.iter_next(i)

    def set_up(self, config):
        '''Reads configuration from given config instance.'''
        
        def get_value_and_attributes(definition):
            name = definition.get_name()
            enabled = True
            try:
                value = definition.get_attribute('value')
                value_check = True
            except KeyError:
                value = ''
                value_check = False
            attributes = definition.get_attributes()
            attributes.pop('value', None)
            return (name, enabled, value, value_check, attributes, )

        def add_children(parent, definition, model):
            if isinstance(definition, DefinitionTree):
                for d in definition.get_definitions():
                    name, enabled, value, value_check, attributes = \
                        get_value_and_attributes(d)
                    i = model.append(
                        parent,
                        (name, '', enabled, value, value_check, attributes, ))
                    add_children(i, d, model)
        
        self.on_new_menu_item_activate()
        for definition in config.get_definitions():
            name, enabled, value, value_check, attributes = \
                get_value_and_attributes(definition)
            
            if name not in self.options:
                tab_name = 'unknown'
            else:
                tab_name = self.options[name]
            
            tree = self.tabs[tab_name][1]
            model = tree.get_model()
            if tab_name != 'unknown':
                i = model.iter_children(None) # find this option
                while model[i][0] != name:
                    i = model.iter_next(i)
            else:
                i = model.append(None, (name, '', False, '', False, {}, ))
            row = model[i]
            row[2] = enabled
            row[3] = value
            row[4] = value_check
            row[5] = attributes
            add_children(i, definition, model)

    def construct_options(self):
        '''Reads known options from file and prepares GUI.'''

        # segregate options by tab
        segregated_options = {} # tab name => option names
        self.options = {} # option name => tab name
        for option in GUIConfig.options:
            tab = 'other'
            for prefix in GUIConfig.tabs:
                if option.startswith(prefix):
                    tab = prefix
                    break
            if not segregated_options.has_key(tab):
                segregated_options[tab] = []
            segregated_options[tab].append(option)
            self.options[option] = tab

        self.tabs = {} # tab name => (table, tree, )
        for tab in GUIConfig.tabs + ['other', 'unknown']:
            options = segregated_options.get(tab, [])
            panels = gtk.HPaned()
            
            tree = gtk.TreeView(gtk.TreeStore(
                    gobject.TYPE_STRING, # option name
                    gobject.TYPE_STRING, # option tooltip
                    gobject.TYPE_BOOLEAN, # enabled
                    gobject.TYPE_STRING, # value
                    gobject.TYPE_BOOLEAN, # value_check
                    gobject.TYPE_PYOBJECT)) # attributes dict
            tree_model = tree.get_model()
            tree.set_headers_visible(False)
            tree_column = gtk.TreeViewColumn()
            tree_renderer = gtk.CellRendererText()
            tree_column.pack_start(tree_renderer)
            tree_column.add_attribute(tree_renderer, 'text', 0)
            tree.append_column(tree_column)
            
            tree_scroll = gtk.ScrolledWindow()
            tree_scroll.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
            tree_scroll.set_shadow_type(gtk.SHADOW_OUT)
            tree_scroll.set_border_width(5)
            tree_scroll.add(tree)
            panels.pack1(tree_scroll, True, False)
            table = EditionTable(tree)
            panels.pack2(table, False, False)

            self.tabs[tab] = (table, tree, )
            
            for option in options:
                tooltip_text, var = GUIConfig.options[option]
                # all but first two columns will be set to defaults later by
                # on_new_menu_item_activate
                tree_model.append(None, (option, tooltip_text, False, '',
                                         False, {}, ))
             
            self.widgets.get_widget('notebook').append_page(
                panels, gtk.Label(tab))

        self.on_new_menu_item_activate()
        self.widgets.get_widget('notebook').show_all()

PyGTKControl()
gtk.main()
