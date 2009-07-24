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
from MyServer.pycontrollib.definition import DefinitionElement, DefinitionTree

class DefinitionTable(gtk.Table):
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
        def toggle_value(button, tree):
            # don't allow value for definition with children
            model, selected = tree.get_selection().get_selected()
            if selected is not None and model.iter_children(selected) is not None:
                button.set_active(False)
                return
            active = button.get_active()
            self.value_field.set_editable(active)
        value_checkbutton.connect('toggled', toggle_value, tree)
        self.attach(value_label, 0, 1, 1, 2, gtk.FILL, gtk.FILL)
        self.attach(value_entry, 1, 2, 1, 2, yoptions = gtk.FILL)
        self.attach(value_checkbutton, 2, 3, 1, 2, gtk.FILL, gtk.FILL)

        add_definition_button = gtk.Button('add sub-definition')
        def add_sub_definition(button):
            model, selected = tree.get_selection().get_selected()
            if selected is not None:
                model.append(selected, ('', '', False, True, '', False, {}, ))
                self.value_check_field.set_active(False) # auto disable value
                self.value_field.set_editable(False)
        add_definition_button.connect('clicked', add_sub_definition)
        self.attach(add_definition_button, 0, 3, 2, 3, yoptions = gtk.FILL)

        remove_definition_button = gtk.Button('remove this definition')
        def remove_definition(button):
            model, selected = tree.get_selection().get_selected()
            if selected is not None and not model[selected][2]:
                model.remove(selected)
                self.clear()
        remove_definition_button.connect('clicked', remove_definition)
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
        '''Clear input widgets.'''
        self.enabled_field.set_active(False)
        self.value_field.set_text('')
        self.value_check_field.set_active(False)
        self.attributes_field.clear()
        self.last_selected = None

    def save_changed(self, tree):
        '''Save data from input widgets to tree.'''
        if self.last_selected is not None:
            attributes = {}
            i = self.attributes_field.iter_children(None)
            while i is not None: # iterate over attributes
                attributes[self.attributes_field.get_value(i, 0)] = \
                    self.attributes_field.get_value(i, 1)
                i = self.attributes_field.iter_next(i)
            row = tree.get_model()[self.last_selected]
            row[3] = self.enabled_field.get_active()
            row[4] = self.value_field.get_text()
            row[5] = self.value_check_field.get_active()
            row[6] = attributes

    def cursor_changed(self, tree):
        '''Save data, and display data of current selected row.'''
        self.save_changed(tree)

        self.clear()

        self.last_selected = current = self.get_selected(tree)
        row = tree.get_model()[current]
        self.enabled_field.set_active(row[3])
        self.value_field.set_text(row[4])
        self.value_check_field.set_active(row[5])
        for attribute in row[6]:
            self.attributes_field.append((attribute, row[5][attribute], ))

    def get_selected(self, tree):
        '''Get iterator of currently selected row.'''
        model, selected = tree.get_selection().get_selected()
        return selected

    def make_def(self, tree):
        '''Export all data as list of definitions.'''
        self.save_changed(tree)
        model = tree.get_model()
        definitions = []
        i = model.iter_children(None)
        while i is not None: # iterate over options
            definition = tree.make_def(i)
            if definition is not None:
                definitions.append(definition)
            i = model.iter_next(i)
        return definitions

class DefinitionTreeView(gtk.TreeView):
    def __init__(self):
        gtk.TreeView.__init__(self, gtk.TreeStore(
                gobject.TYPE_STRING, # option name
                gobject.TYPE_STRING, # option tooltip
                gobject.TYPE_BOOLEAN, # True if option is known
                gobject.TYPE_BOOLEAN, # enabled
                gobject.TYPE_STRING, # value
                gobject.TYPE_BOOLEAN, # value_check
                gobject.TYPE_PYOBJECT)) # attributes dict
        model = self.get_model()
        def name_edited_handler(cell, path, text, data):
            model = data
            row = model[path]
            if not row[2]: # don't edit names of known options
                row[0] = text
        name_renderer = gtk.CellRendererText()
        name_renderer.set_property('editable', True)
        name_renderer.connect('edited', name_edited_handler, model)
        name_column = gtk.TreeViewColumn('name')
        name_column.pack_start(name_renderer)
        name_column.add_attribute(name_renderer, 'text', 0)
        self.append_column(name_column)
        value_renderer = gtk.CellRendererText()
        value_column = gtk.TreeViewColumn('value')
        value_column.pack_start(value_renderer)
        value_column.add_attribute(value_renderer, 'text', 4)
        self.append_column(value_column)

        self.scroll = gtk.ScrolledWindow()
        self.scroll.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        self.scroll.set_shadow_type(gtk.SHADOW_OUT)
        self.scroll.set_border_width(5)
        self.scroll.add(self)

        def show_tooltip(widget, x, y, keyboard_tip, tooltip):
            if not widget.get_tooltip_context(x, y, keyboard_tip):
                return False
            else:
                model, path, it = widget.get_tooltip_context(x, y, keyboard_tip)
                tooltip.set_text(model[it][1])
                widget.set_tooltip_row(tooltip, path)
                return True
        self.props.has_tooltip = True
        self.connect("query-tooltip", show_tooltip)
        
    def make_def(self, current):
        '''Return row pointed by current exported as definition.'''
        model = self.get_model()
        row = model[current]
        name = row[0]
        enabled = row[3]
        value = row[4]
        value_check = row[5]
        attributes = row[6]
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
                definition = self.make_def(i)
                if definition is not None:
                    definitions.append(definition)
                i = model.iter_next(i)
            return DefinitionTree(name, definitions, attributes)

    def make_clear(self):
        '''Remove all sub-definitions, reset values of level-1 definitions.'''
        model = self.get_model()
        self.get_selection().unselect_all()
        i = model.iter_children(None)
        while i is not None: # iterate over options
            row = model[i]
            row[3] = False
            row[4] = ''
            row[5] = False
            row[6] = {}
            child = model.iter_children(i)
            if child is not None: # remove node children
                while model.remove(child):
                    pass
            i = model.iter_next(i)

    def set_up(self, definitions, search):
        '''Sets up model reading from given config instance. If search in True
        will substitute alreaty present model data, if it is false will append
        all the definitions.'''
        
        def get_value_and_attributes(definition):
            '''Get column values from definition instance.'''
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

        def add_children(parent, definition):
            '''Insert sub-definitions of given definition as children of given
            parent tree iterator.'''
            if isinstance(definition, DefinitionTree):
                for d in definition.get_definitions():
                    name, enabled, value, value_check, attributes = \
                        get_value_and_attributes(d)
                    i = self.get_model().append(
                        parent,
                        (name, '', False, enabled, value, value_check,
                         attributes, ))
                    add_children(i, d)

        model = self.get_model()
        for definition in definitions:
            name, enabled, value, value_check, attributes = \
                get_value_and_attributes(definition)
            if search:
                i = model.iter_children(None) # find this option
                while model[i][0] != name:
                    i = model.iter_next(i)
            else:
                i = model.append(None,
                                 (name, '', False, False, '', False, {}, ))
            row = model[i]
            row[3] = enabled
            row[4] = value
            row[5] = value_check
            row[6] = attributes
            add_children(i, definition)
