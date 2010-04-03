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
from MyServer.GUI import GUIConfig

class DefinitionTable(gtk.Table):
    def __init__(self, tree):

        # 7 rows and 3 columns
        gtk.Table.__init__(self, 7, 3)

        tree.connect('cursor-changed', self.cursor_changed)
        self.last_selected = None

        # Enabled checkbutton
        enabled_label = gtk.Label('Enabled:')
        self.enabled_field = enabled_checkbutton = gtk.CheckButton()
        enabled_checkbutton.set_tooltip_text('If not checked, Definition won\'t be included in Saved Configuration.')
        self.attach(enabled_label, 0, 1, 0, 1, gtk.FILL, gtk.FILL)
        self.attach(enabled_checkbutton, 1, 3, 0, 1, yoptions = gtk.FILL)

        #Value Field
        value_label = gtk.Label('Value:')
        self.string_value_field = gtk.Entry()
        self.int_value_field = gtk.SpinButton()
        self.int_value_field.set_adjustment(
            gtk.Adjustment(0, 0, 2 ** 32 - 1, 1))
        self.int_value_field.hide()
        self.bool_value_field = gtk.combo_box_new_text()
        self.bool_value_field.append_text('YES')
        self.bool_value_field.append_text('NO')
        self.bool_value_field.hide()
        value_box = gtk.VBox()
        value_box.add(self.string_value_field)
        value_box.add(self.int_value_field)
        value_box.add(self.bool_value_field)
        self.value_check_field = value_checkbutton = gtk.CheckButton()
        value_checkbutton.set_tooltip_text('If not checked, Value won\'t be included in Saved Configuration.')
        value_checkbutton.set_active(True)

        self.attach(value_label, 0, 1, 1, 2, gtk.FILL, gtk.FILL)
        self.attach(value_box, 1, 2, 1, 2, gtk.FILL, gtk.FILL)
        self.attach(value_checkbutton, 2, 3, 1, 2, gtk.FILL, gtk.FILL)

        # Add Sub-Definition button
        add_definition_button = gtk.Button('Add Sub-Definition')
        def add_sub_definition(button):
            model, selected = tree.get_selection().get_selected()
            if selected is not None:
                model.append(selected, ('', '', False, True, '', False, {}, ))
                self.value_check_field.set_active(False) # auto disable value
        add_definition_button.connect('clicked', add_sub_definition)
        self.attach(add_definition_button, 0, 3, 2, 3, yoptions = gtk.FILL)

        # Remove Sub-Definition button
        remove_definition_button = gtk.Button('Remove Definition')
        def remove_definition(button):
            model, selected = tree.get_selection().get_selected()
            if selected is not None and not model[selected][2]:
                model.remove(selected)
                self.clear()
        remove_definition_button.connect('clicked', remove_definition)
        self.attach(remove_definition_button, 0, 3, 3, 4, yoptions = gtk.FILL)

        # Add/Remove Attribute Buttons
        add_attribute_button = gtk.Button('Add Attribute')
        remove_attribute_button = gtk.Button('Remove Attribute')
        button_table = gtk.Table(1, 3)
        add_attribute_button.connect(
            'clicked',
            lambda button: attributes_model.append(('var', '0', )))
        def remove_attribute(button):
            model, selected = attributes_list.get_selection().get_selected()
            if selected is not None:
                model.remove(selected)
        remove_attribute_button.connect('clicked', remove_attribute)
        button_table.attach(add_attribute_button, 0, 2, 0, 1)
        button_table.attach(remove_attribute_button, 2, 3, 0, 1)
        self.attach(button_table, 0, 3, 4, 5, yoptions = gtk.FILL)

        # Attributes TreeView
        attributes_label = gtk.Label('Attributes:')
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
        variable_column = gtk.TreeViewColumn('Variable')
        variable_column.set_min_width(gtk.gdk.Screen().get_width() / 11)
        variable_renderer = gtk.CellRendererText()
        variable_renderer.set_property('editable', True)
        variable_renderer.connect('edited', edited_handler,
                                  (attributes_model, 0, ))
        variable_column.pack_start(variable_renderer)
        variable_column.add_attribute(variable_renderer, 'text', 0)

        value_column = gtk.TreeViewColumn('Value')
        value_renderer = gtk.CellRendererText()
        value_renderer.set_property('editable', True)
        value_renderer.connect('edited', edited_handler,
                                  (attributes_model, 1, ))
        value_column.pack_start(value_renderer)
        value_column.add_attribute(value_renderer, 'text', 1)

        attributes_list.append_column(variable_column)
        attributes_list.append_column(value_column)
        self.attach(attributes_label, 0, 3, 5, 6, yoptions = gtk.FILL)
        self.attach(attributes_scroll, 0, 3, 6, 7)

    def clear(self):
        '''Clear input widgets.'''
        self.enabled_field.set_active(False)
        self.string_value_field.set_text('')
        self.int_value_field.set_value(0)
        self.bool_value_field.set_active(0)
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
            var_type = GUIConfig.options.get(row[0], ('', 'string', ))[1]
            if var_type == 'string':
                row[4] = self.string_value_field.get_text()
            elif var_type == 'integer':
                row[4] = str(int(self.int_value_field.get_value()))
            elif var_type == 'bool':
                row[4] = 'NO' if self.bool_value_field.get_active() else 'YES'
            row[5] = self.value_check_field.get_active()
            row[6] = attributes

    def cursor_changed(self, tree):
        '''Save data, and display data of current selected row.'''
        self.save_changed(tree)

        self.clear()

        self.last_selected = current = self.get_selected(tree)
        row = tree.get_model()[current]
        self.enabled_field.set_active(row[3])
        var_type = GUIConfig.options.get(row[0], ('', 'string', ))[1]
        self.string_value_field.hide()
        self.int_value_field.hide()
        self.bool_value_field.hide()

        if var_type == 'string':
            self.string_value_field.set_text(row[4])
            self.string_value_field.show()
            self.set_col_spacing(1,0)
            self.set_row_spacing(1,4)

        elif var_type == 'integer':
            self.int_value_field.set_value(int(row[4]))
            self.int_value_field.show()
            self.set_col_spacing(1,66)
            self.set_row_spacing(1,4)

        elif var_type == 'bool':
            self.bool_value_field.set_active(int(row[4] == 'NO'))
            self.bool_value_field.show()
            self.set_col_spacing(1,103)
            self.set_row_spacing(1,0)

        self.value_check_field.set_active(row[5])
        for attribute in row[6]:
            self.attributes_field.append((attribute, row[6][attribute], ))

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
        name_column = gtk.TreeViewColumn('Name')
        name_column.set_min_width(gtk.gdk.Screen().get_width() / 5)
        name_column.pack_start(name_renderer)
        name_column.add_attribute(name_renderer, 'text', 0)
        self.append_column(name_column)
        value_renderer = gtk.CellRendererText()
        value_column = gtk.TreeViewColumn('Value')
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
            var_type = GUIConfig.options.get(row[0], ('', 'string', ))[1]
            if var_type == 'string':
                row[4] = ''
            elif var_type == 'integer':
                row[4] = '0'
            elif var_type == 'bool':
                row[4] = 'YES'
            row[5] = False
            row[6] = {}
            child = model.iter_children(i)
            if child is not None: # remove node children
                while model.remove(child):
                    pass
            i = model.iter_next(i)

    def set_up(self, definitions, search):
        '''Sets up model reading from given definition list. If search is True
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
