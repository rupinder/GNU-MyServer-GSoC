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
import gtk.glade
import gobject
from MyServer.pycontrollib.config import MyServerConfig
from MyServer.pycontrollib.mimetypes import MIMETypes
from MyServer.pycontrollib.controller import Controller
from MyServer.pycontrollib.vhost import VHosts
from MyServer.GUI import GUIConfig
from MyServer.GUI.AboutWindow import About
from MyServer.GUI.ConnectionWindow import Connection
from MyServer.GUI.DefinitionWidgets import DefinitionTable, DefinitionTreeView
from MyServer.GUI.MIMEWidgets import MimeTable, MimeTreeView
from MyServer.GUI.VHostWidgets import VHostTable, VHostTreeView

class PyGTKControl():
    '''GNU MyServer Control main window.'''

    def __init__(self):
        self.gladefile = 'PyGTKControl.glade'
        self.widgets = gtk.glade.XML(self.gladefile, 'window')
        self.widgets.signal_autoconnect(self)
        self.construct_options()
        self.construct_mime()
        self.construct_vhosts()
        self.chooser = None # Active file chooser
        # path of currently edited files
        self.config_path = self.mime_path = self.vhost_path = None
        # remembered unhandled parts of configs
        self.config_custom = []
        self.mime_custom = []
        self.vhost_custom = []
        self.mime_custom_attrib = {}
        self.vhost_custom_attrib = {}
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
        '''Get server config from remote server.'''
        if self.controller is not None:
            self.set_up_config(self.controller.get_server_configuration())

    def on_get_mime_menu_item_activate(self, widget):
        '''Get MIME config from remote server.'''
        if self.controller is not None:
            self.set_up_mime(self.controller.get_MIME_type_configuration())

    def on_get_vhost_menu_item_activate(self, widget):
        '''Get VHost config from remote server.'''
        if self.controller is not None:
            self.set_up_vhost(self.controller.get_vhost_configuration())

    def on_put_config_menu_item_activate(self, widget):
        '''Put server config on remote server.'''
        if self.controller is not None:
            self.controller.put_server_configuration(self.get_current_config())

    def on_put_mime_menu_item_activate(self, widget):
        '''Put MIME config on remote server.'''
        if self.controller is not None:
            self.controller.put_MIME_type_configuration(self.get_current_mime())

    def on_put_vhost_menu_item_activate(self, widget):
        '''Put VHost config on remote server.'''
        if self.controller is not None:
            self.controller.put_vhost_configuration(self.get_vhost_config())

    def on_new_config_menu_item_activate(self, widget = None):
        '''Clears server configuration.'''
        if widget is not None:
            self.config_path = None
        self.config_custom = []
        table, tree = self.tabs['unknown']
        tree.get_model().clear()
        for tab in self.tabs:
            table, tree = self.tabs[tab]
            table.clear()
            tree.make_clear()

    def on_new_mime_menu_item_activate(self, widget = None):
        '''Clears MIME configuration.'''
        if widget is not None:
            self.mime_path = None
        self.mime_custom = []
        self.mime_custom_attrib = {}
        table, tree = self.mime_tab[0]
        table.clear()
        tree.get_model().clear()

    def on_new_vhost_menu_item_activate(self, widget = None):
        '''Clears VHost configuration.'''
        if widget is not None:
            self.vhost_path = None
        self.vhost_custom = []
        self.vhost_custom_attrib = {}
        table, tree = self.vhost_tab
        table.clear()
        tree.get_model().clear()

    def generic_open(self, dialog, path, config_type, set_up_method):
        '''Open configuration file.'''
        if self.chooser is not None:
            self.chooser.destroy()
        self.chooser = gtk.FileChooserDialog(
            dialog,
            buttons = (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                       gtk.STOCK_OPEN, gtk.RESPONSE_OK))
        def handle_response(widget, response):
            if response == gtk.RESPONSE_OK:
                setattr(self, path, self.chooser.get_filename())
                with open(getattr(self, path)) as f:
                    conf = config_type.from_string(f.read())
                set_up_method(conf)
            self.chooser.destroy()
        self.chooser.connect('response', handle_response)
        self.chooser.show()

    def on_open_config_menu_item_activate(self, widget):
        '''Open local server configuration file.'''
        self.generic_open(
            'Open server configuration file.', 'config_path',
            MyServerConfig, self.set_up_config)

    def on_open_mime_menu_item_activate(self, widget):
        '''Open local MIME configuration file.'''
        self.generic_open(
            'Open MIME configuration file.', 'mime_path',
            MIMETypes, self.set_up_mime)

    def on_open_vhost_menu_item_activate(self, widget):
        '''Open local VHost configuration file.'''
        self.generic_open(
            'Open VHost configuration file.', 'vhost_path',
            VHosts, self.set_up_vhost)

    def generic_save_as(self, dialog, widget, path, save_method):
        '''Save configuration as file.'''
        if self.chooser is not None:
            self.chooser.destroy()
        self.chooser = gtk.FileChooserDialog(
            dialog,
            action = gtk.FILE_CHOOSER_ACTION_SAVE,
            buttons = (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                       gtk.STOCK_SAVE_AS, gtk.RESPONSE_OK))
        def handle_response(widget, response):
            if response == gtk.RESPONSE_OK:
                setattr(self, path, self.chooser.get_filename())
                save_method(widget)
            self.chooser.destroy()
        self.chooser.connect('response', handle_response)
        self.chooser.show()

    def on_save_as_config_menu_item_activate(self, widget):
        '''Save server configuration as local file.'''
        self.generic_save_as(
            'Save server configuration file.',
            widget, 'config_path',
            self.on_save_config_menu_item_activate)

    def on_save_as_mime_menu_item_activate(self, widget):
        '''Save MIME configuration as local file.'''
        self.generic_save_as(
            'Save MIME configuration file.',
            widget, 'mime_path',
            self.on_save_mime_menu_item_activate)

    def on_save_as_vhost_menu_item_activate(self, widget):
        '''Save VHost configuration as local file.'''
        self.generic_save_as(
            'Save VHost configuration file.',
            widget, 'vhost_path',
            self.on_save_vhost_menu_item_activate)

    def generic_save(self, widget, path, save_as_method, config_method):
        '''Save configuration file.'''
        if getattr(self, path) is None:
            save_as_method(widget)
        else:
            config = config_method()
            with open(getattr(self, path), 'w') as f:
                f.write(str(config))

    def on_save_config_menu_item_activate(self, widget):
        '''Save server configuration to file.'''
        self.generic_save(
            widget, 'config_path',
            self.on_save_as_config_menu_item_activate,
            self.get_current_config)

    def on_save_mime_menu_item_activate(self, widget):
        '''Save MIME configuration to file.'''
        self.generic_save(
            widget, 'mime_path',
            self.on_save_as_mime_menu_item_activate,
            self.get_current_mime)

    def on_save_vhost_menu_item_activate(self, widget):
        '''Save VHost configuration to file.'''
        self.generic_save(
            widget, 'vhost_path',
            self.on_save_as_vhost_menu_item_activate,
            self.get_current_vhost)

    def get_current_config(self):
        '''Returns current server configuration as MyServerConfig instance.'''
        definitions = []
        for tab in self.tabs:
            table, tree = self.tabs[tab]
            definitions += table.make_def(tree)
        config = MyServerConfig(definitions)
        config.definitions.custom = self.config_custom
        return config

    def get_current_mime(self):
        '''Returns current MIME configuration as MIMETypes instance.'''
        table, tree = self.mime_tab[0]
        mimes = table.make_def(tree)
        config = MIMETypes(mimes)
        config.custom = self.mime_custom
        config.custom_attrib = self.mime_custom_attrib
        return config

    def get_current_vhost(self):
        '''Returns current VHost configuration as VHosts instance.'''
        table, tree = self.vhost_tab
        vhosts = table.make_def(tree)
        config = VHosts(vhosts)
        config.custom = self.vhost_custom
        config.custom_attrib = self.vhost_custom_attrib
        return config

    def on_add_unknown_definition_menu_item_activate(self, widget):
        '''Adds a new definition to unknown tab.'''
        table, tree = self.tabs['unknown']
        tree.get_model().append(None, ('', '', False, True, '', False, {}, ))

    def on_add_mime_type_menu_item_activate(self, widget):
        '''Adds a new MIME type.'''
        table, tree = self.mime_tab[0]
        mime_lists = {}
        for mime_list in GUIConfig.mime_lists:
            mime_lists[mime_list] = []
        tree.get_model().append(('', {}, mime_lists, [], [], {}, ))

    def on_add_vhost_menu_item_activate(self, widget):
        '''Adds a new VHost.'''
        table, tree = self.vhost_tab
        vhost_lists = {}
        for vhost_list in GUIConfig.vhost_lists:
            vhost_lists[vhost_list[0]] = []
        tree.get_model().append(('', {}, vhost_lists, [], [], {}, ))

    def on_add_definition_to_mime_menu_item_activate(self, widget):
        '''Adds a definition to currently selected MIME type.'''
        table, tree = self.mime_tab[1]
        tree.get_model().append(None, ('', '', False, True, '', False, {}, ))

    def on_add_log_to_vhost_menu_item_activate(self, widget):
        '''Adds a log to currently selected VHost.'''
        table, tree = self.vhost_tab
        table.add_log()

    def set_up_config(self, config):
        '''Reads server configuration from given config instance.'''
        self.on_new_config_menu_item_activate()
        self.config_custom = config.definitions.custom
        tabs = {}
        for tab in self.tabs:
            tabs[tab] = []
        for definition in config.get_definitions():
            name = definition.get_name()
            tab_name = self.options[name] if name in self.options else 'unknown'
            tabs[tab_name].append(definition)

        for tab in tabs:
            tree = self.tabs[tab][1]
            tree.set_up(tabs[tab], tab != 'unknown')

    def set_up_mime(self, config):
        '''Reads MIME configuration from given config instance.'''
        self.on_new_mime_menu_item_activate()
        self.mime_custom = config.custom
        self.mime_custom_attrib = config.custom_attrib
        tree = self.mime_tab[0][1]
        tree.set_up(config.MIME_types)

    def set_up_vhost(self, config):
        '''Reads VHost configuration from given config instance.'''
        self.on_new_vhost_menu_item_activate()
        self.vhost_custom = config.custom
        self.vhost_custom_attrib = config.custom_attrib
        tree = self.vhost_tab[1]
        tree.set_up(config.VHosts)

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

            tree = DefinitionTreeView()
            tree_model = tree.get_model()
            panels.pack1(tree.scroll, True, False)
            table = DefinitionTable(tree)
            panels.pack2(table, False, False)

            self.tabs[tab] = (table, tree, )

            for option in options:
                tooltip_text, var = GUIConfig.options[option]
                # all but first three columns will be set to defaults later by
                # on_new_menu_item_activate
                tree_model.append(None, (option, tooltip_text, True, False, '',
                                         False, {}, ))

            self.widgets.get_widget('notebook').append_page(
                panels, gtk.Label(tab))

        self.on_new_config_menu_item_activate()
        self.widgets.get_widget('notebook').show_all()

    def construct_mime(self):
        '''Reads mime options from file and prepares GUI.'''
        vpanels = gtk.VPaned()

        panels = gtk.HPaned()
        def_tree = DefinitionTreeView()
        panels.pack1(def_tree.scroll, True, False)
        def_table = DefinitionTable(def_tree)
        panels.pack2(def_table, False, False)

        vpanels.pack2(panels)

        panels = gtk.HPaned()
        tree = MimeTreeView()
        panels.pack1(tree.scroll, True, False)
        table = MimeTable(tree, def_tree, def_table)
        panels.pack2(table, False, False)

        vpanels.pack1(panels)

        self.mime_tab = ((table, tree, ), (def_table, def_tree), )
        self.widgets.get_widget('notebook').append_page(
            vpanels, gtk.Label('MIME Type'))

        self.widgets.get_widget('notebook').show_all()

    def construct_vhosts(self):
        '''Reads vhost options from file and prepares GUI.'''
        panels = gtk.HPaned()
        tree = VHostTreeView()
        panels.pack1(tree.scroll, False, False)
        table = VHostTable(tree)
        panels.pack2(table.scroll, True, False)

        self.vhost_tab = (table, tree, )
        self.widgets.get_widget('notebook').append_page(
            panels, gtk.Label('VHosts'))

        self.widgets.get_widget('notebook').show_all()

PyGTKControl()
gtk.main()
