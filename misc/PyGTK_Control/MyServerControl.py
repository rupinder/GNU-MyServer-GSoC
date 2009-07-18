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
        '''Returns current configuration as list of definitions.'''
        definitions = []
        for option in self.options:
            check, field, var = self.options[option]
            if not check.get_active():
                continue
            definition = self.definitions.get(option)
            if var == 'string':
                definition.set_attribute('value', field.get_text())
            elif var == 'integer':
                definition.set_attribute('value', str(int(field.get_value())))
            elif var == 'bool':
                value = 'NO' if field.get_active() else 'YES'
                definition.set_attribute('value', value)
            elif var == 'list':
                values = []
                model = field.get_model()
                i = model.iter_children(None)
                while i is not None:
                    values.append(model.get_value(i, 0))
                    i = model.iter_next(i)
                values = map(
                    lambda v: DefinitionElement(attributes = {'value': v}),
                    values)
                for i in xrange(len(definition.get_definitions())):
                    definition.remove_definition(0)
                for value in values:
                    definition.add_definition(value)
            definitions.append(definition)
        return MyServerConfig(definitions + self.unknown)

    def on_new_menu_item_activate(self, widget = None):
        '''Clears configuration.'''
        if widget is not None:
            self.path = None
        self.definitions = {} # option name => corresponding Definition
        self.unknown = [] # list of unknown definitions
        for option in self.options:
            check, field, var = self.options.get(option)
            if var != 'list':
                self.definitions[option] = DefinitionElement(option)
            else:
                self.definitions[option] = DefinitionTree(option)
            check.set_active(False)
            if var == 'string':
                field.set_text('')
            elif var == 'integer':
                field.set_value(0)
            elif var == 'bool':
                field.set_active(0)
            elif var == 'list':
                field.get_model().clear()

    def set_up(self, config):
        '''Reads configuration from given config instance.'''
        def put_to_unknown(definition):
            self.unknown.append(definition)
            print('Unknown option:', definition, sep = '\n')
        self.on_new_menu_item_activate()
        for definition in config.get_definitions():
            name = definition.get_name()
            if name not in self.options:
                put_to_unknown(definition)
                continue
            check, field, var = self.options[name]
            if (isinstance(definition, DefinitionElement) and var == 'list') or \
                    (isinstance(definition, DefinitionTree) and var != 'list'):
                put_to_unknown(definition)
                continue
            self.definitions[name] = definition
            check.set_active(True)
            if var == 'string':
                field.set_text(definition.get_attribute('value'))
            elif var == 'integer':
                field.set_value(int(definition.get_attribute('value')))
            elif var == 'bool':
                if definition.get_attribute('value').upper() != 'YES':
                    field.set_active(1)
            elif var == 'list':
                for definition in definition.get_definitions():
                    field.get_model().append(
                        (definition.get_attribute('value'), ))

    def construct_options(self):
        '''Reads known options from file and prepares GUI.'''
        def make_tab_name(text):
            return text.capitalize().replace('.', ' ').replace('_', ' ')
        def make_name(text, tab):
            if tab == 'other':
                return text
            return text[len(tab):].replace('.', ' ').replace('_', ' ').strip()

        # segregate options by tab
        segregated_options = {}
        for option in GUIConfig.options:
            tab = 'other'
            for prefix in GUIConfig.tabs:
                if option.startswith(prefix):
                    tab = prefix
                    break
            if not segregated_options.has_key(tab):
                segregated_options[tab] = []
            segregated_options[tab].append(option)

        tabs = {}
        self.options = {} # option name => (CheckButton, field, type, )
        for tab in GUIConfig.tabs + ['other', 'unknown']:
            options = segregated_options.get(tab, [])
            tabs[tab] = gtk.Table(max(1, len(options)), 3)
            for i in xrange(len(options)):
                option = options[i]
                text, var = GUIConfig.options[option]
                label = gtk.Label(make_name(option, tab))
                label.set_tooltip_text(text)
                check = gtk.CheckButton()
                if var == 'string':
                    field = gtk.Entry()
                    self.options[option] = (check, field, 'string', )
                elif var == 'integer':
                    field = gtk.SpinButton(gtk.Adjustment(
                            0, 0, 2147483647, 1))
                    self.options[option] = (check, field, 'integer', )
                elif var == 'bool':
                    field = gtk.combo_box_new_text()
                    field.append_text('yes')
                    field.append_text('no')
                    field.set_active(0)
                    self.options[option] = (check, field, 'bool', )
                elif var == 'list':
                    tree  = gtk.TreeView(gtk.ListStore(gobject.TYPE_STRING))
                    tree.set_headers_visible(False)
                    tree_column = gtk.TreeViewColumn()
                    tree_renderer = gtk.CellRendererText()
                    tree_column.pack_start(tree_renderer)
                    tree_column.add_attribute(tree_renderer, 'text', 0)
                    tree.append_column(tree_column)
                    new_name = gtk.Entry()
                    add_button = gtk.Button('add')
                    remove_button = gtk.Button('remove')
                    field = gtk.Table(3, 2)
                    field.attach(new_name, 0, 2, 0, 1)
                    field.attach(add_button, 0, 1, 1, 2)
                    field.attach(remove_button, 1, 2, 1, 2)
                    field.attach(tree, 0, 2, 2, 3)
                    def add_to_list(button):
                        tree.get_model().append((new_name.get_text(), ))
                        new_name.set_text('')
                    add_button.connect('clicked', add_to_list)
                    def remove_from_list(button):
                        model, selected = tree.get_selection().get_selected()
                        if selected is not None:
                            model.remove(selected)
                    remove_button.connect('clicked', remove_from_list)
                    self.options[option] = (check, tree, 'list', )
                tabs[tab].attach(check, 0, 1, i, i + 1, gtk.FILL, gtk.FILL)
                tabs[tab].attach(label, 1, 2, i, i + 1, yoptions = gtk.FILL)
                tabs[tab].attach(field, 2, 3, i, i + 1, yoptions = gtk.FILL)
            self.widgets.get_widget('notebook').append_page(
                tabs[tab], gtk.Label(make_tab_name(tab)))

        self.on_new_menu_item_activate()
        self.widgets.get_widget('notebook').show_all()

PyGTKControl()
gtk.main()
