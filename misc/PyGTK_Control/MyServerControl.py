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
import gtk.glade
import GUIConfig
from MyServer.pycontrollib.config import MyServerConfig
from MyServer.pycontrollib.definition import DefinitionElement, DefinitionTree

class About():
    def __init__(self):
        self.gladefile = 'PyGTKControl.glade'
        self.widgets = gtk.glade.XML(self.gladefile, 'aboutdialog')
        self.widgets.signal_autoconnect(self)

    def on_aboutdialog_response(self, widget, response):
        widget.destroy()

class PyGTKControl():
    def __init__(self):
        self.gladefile = 'PyGTKControl.glade'
        self.widgets = gtk.glade.XML(self.gladefile, 'window')
        self.widgets.signal_autoconnect(self)
        self.construct_options()
        self.chooser = None
        self.path = None

    def on_window_destroy(self, widget):
        gtk.main_quit()
        
    def on_quit_menu_item_activate(self, widget):
        gtk.main_quit()

    def on_about_menu_item_activate(self, widget):
        About()

    def on_open_menu_item_activate(self, widget):
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
                self.set_up(conf.get_definitions())
            self.chooser.destroy()
        self.chooser.connect('response', handle_response)
        self.chooser.show()

    def on_save_as_menu_item_activate(self, widget):
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
        if self.path is None:
            self.on_save_as_menu_item_activate(widget)
        else:
            definitions = []
            for option in self.options:
                check, field, var = self.options[option]
                if not check.get_active():
                    continue
                if var == 'string':
                    definitions.append(
                        DefinitionElement(option, {'value': field.get_text()}))
                elif var == 'integer':
                    value = str(int(field.get_value()))
                    definitions.append(
                        DefinitionElement(option, {'value': value}))
                elif var == 'bool':
                    value = 'YES' if field.get_active() else 'NO'
                    definitions.append(
                        DefinitionElement(option, {'value': value}))
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
                    definitions.append(
                        DefinitionTree(option, values))
            config = MyServerConfig(definitions)
            with open(self.path, 'w') as f:
                f.write(str(config))

    def on_new_menu_item_activate(self, widget = None):
        if widget is not None:
            self.path = None
        for check, field, var in self.options.itervalues():
            check.set_active(False)
            if var == 'string':
                field.set_text('')
            elif var == 'integer':
                field.set_value(0)
            elif var == 'bool':
                field.set_active(0)
            elif var == 'list':
                field.get_model().clear()

    def set_up(self, definitions):
        self.on_new_menu_item_activate()
        for definition in definitions:
            name = definition.get_name()
            if name not in self.options:
                pass # goes to unknown
            check, field, var = self.options[name]
            check.set_active(True)
            if var == 'string':
                field.set_text(definition.get_attribute('value'))
            elif var == 'integer':
                field.set_value(int(definition.get_attribute('value')))
            elif var == 'bool':
                if definition.get_attribute('value').upper() != 'YES':
                    field.set_active(1)
            elif var == 'list':
                if isinstance(definition, DefinitionElement):
                    field.get_model().append(
                        (definition.get_attribute('value'), ))
                else:
                    for definition in definition.get_definitions():
                        field.get_model().append(
                            (definition.get_attribute('value'), ))

    def construct_options(self):
        def make_tab_name(text):
            return text.capitalize().replace('.', ' ').replace('_', ' ')
        def make_name(text, tab):
            if tab == 'other':
                return text
            return text[len(tab):].replace('.', ' ').replace('_', ' ').strip()

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
        self.options = {}
        for tab in GUIConfig.tabs + ['other', 'unknown']:
            options = segregated_options.get(tab, [])
            tabs[tab] = gtk.Table(max(1, len(options)), 3)
            for i in xrange(len(options)):
                option = options[i]
                label = gtk.Label(make_name(option, tab))
                label.set_tooltip_text(GUIConfig.options[option][0])
                check = gtk.CheckButton()
                if GUIConfig.options[option][1] == 'string':
                    field = gtk.Entry()
                    self.options[option] = (check, field, 'string', )
                elif GUIConfig.options[option][1] == 'integer':
                    field = gtk.SpinButton(gtk.Adjustment(
                            0, 0, 2147483647, 1))
                    self.options[option] = (check, field, 'integer', )
                elif GUIConfig.options[option][1] == 'bool':
                    field = gtk.combo_box_new_text()
                    field.append_text('yes')
                    field.append_text('no')
                    field.set_active(0)
                    self.options[option] = (check, field, 'bool', )
                elif GUIConfig.options[option][1] == 'list':
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
                        selected = tree.get_selection().get_selected()
                        if selected[1] is not None:
                            selected[0].remove(selected[1])
                    remove_button.connect('clicked', remove_from_list)
                    self.options[option] = (check, tree, 'list', )
                tabs[tab].attach(check, 0, 1, i, i + 1, gtk.FILL, gtk.FILL)
                tabs[tab].attach(label, 1, 2, i, i + 1, yoptions = gtk.FILL)
                tabs[tab].attach(field, 2, 3, i, i + 1, yoptions = gtk.FILL)
            self.widgets.get_widget('notebook').append_page(
                tabs[tab], gtk.Label(make_tab_name(tab)))

        self.widgets.get_widget('notebook').show_all()

PyGTKControl()
gtk.main()
