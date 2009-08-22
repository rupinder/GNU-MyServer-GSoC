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
from MyServer.pycontrollib.security import SecurityList, User, Return, \
    Condition, Permission
from MyServer.pycontrollib.definition import DefinitionElement, DefinitionTree

class SecurityTree(gtk.TreeView):
    def __init__(self, security_table):
        gtk.TreeView.__init__(self, gtk.TreeStore(
                gobject.TYPE_STRING, # tag
                gobject.TYPE_PYOBJECT)) # object
        renderer = gtk.CellRendererText()
        column = gtk.TreeViewColumn('tag')
        column.pack_start(renderer)
        column.add_attribute(renderer, 'text', 0)
        self.append_column(column)

        self.security_table = security_table

        self.last_selected = None
        self.connect('cursor-changed', self.cursor_changed)

        self.scroll = gtk.ScrolledWindow()
        self.scroll.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        self.scroll.set_shadow_type(gtk.SHADOW_OUT)
        self.scroll.set_border_width(5)
        self.scroll.add(self)

    def clear(self):
        self.get_model().clear()
        self.last_selected = None
        self.get_model().append(None, ('SECURITY', SecurityList(), ))
        self.security_table.empty_table.security_check.set_active(False)
        self.security_table.switch_table('SECURITY')

    def set_up(self, security_list):
        def add_from_definition_tree(parent, model, definition):
            for element in definition.get_definitions():
                x = model.append(parent, (element.tag, element, ))
                if element.tag == 'DEFINE tree':
                    add_from_definition_tree(x, model, element)
        def add_from_condition(parent, model, condition):
            for element in condition.get_sub_elements():
                x = model.append(parent, (element.tag, element, ))
                if element.tag == 'CONDITION':
                    add_from_condition(x, model, element)
        self.security_table.empty_table.security_check.set_active(True)
        self.security_table.switch_table('SECURITY')
        model = self.get_model()
        self.last_selected = None
        model.clear()
        parent = model.append(None, ('SECURITY', security_list))
        for element in security_list.get_elements():
            x = model.append(parent, (element.tag, element, ))
            if element.tag == 'CONDITION':
                add_from_condition(x, model, element)
            elif element.tag == 'DEFINE tree':
                add_from_definition_tree(x, model, element)

    def save(self):
        if self.last_selected is None:
            return
        model = self.get_model()
        row = model[self.last_selected]
        tag = row[0]
        data = row[1]
        if tag == 'USER':
            table = self.security_table.user_table
            if table.name_check.get_active():
                data.set_name(table.name_entry.get_text())
            else:
                data.set_name(None)
            if table.password_check.get_active():
                data.set_password(table.password_entry.get_text())
            else:
                data.set_password(None)
            combo = table.read_combo
            value = combo.get_model()[combo.get_active()][0]
            data.set_read(value == 'Yes' if value != 'empty' else None)
            combo = table.execute_combo
            value = combo.get_model()[combo.get_active()][0]
            data.set_execute(value == 'Yes' if value != 'empty' else None)
            combo = table.browse_combo
            value = combo.get_model()[combo.get_active()][0]
            data.set_browse(value == 'Yes'if value != 'empty' else None)
            combo = table.delete_combo
            value = combo.get_model()[combo.get_active()][0]
            data.set_delete(value == 'Yes' if value != 'empty' else None)
            combo = table.write_combo
            value = combo.get_model()[combo.get_active()][0]
            data.set_write(value == 'Yes' if value != 'empty' else None)
        elif tag == 'CONDITION':
            table = self.security_table.condition_table
            if table.name_check.get_active():
                data.set_name(table.name_entry.get_text())
            else:
                data.set_name(None)
            if table.value_check.get_active():
                data.set_value(table.value_entry.get_text())
            else:
                data.set_value(None)
            combo = table.regex_combo
            value = combo.get_model()[combo.get_active()][0]
            data.set_regex(value == 'Yes' if value != 'empty' else None)
            combo = table.reverse_combo
            value = combo.get_model()[combo.get_active()][0]
            data.set_reverse(value == 'Yes' if value != 'empty' else None)
        elif tag == 'PERMISSION':
            table = self.security_table.permission_table
            combo = table.read_combo
            value = combo.get_model()[combo.get_active()][0]
            data.set_read(value == 'Yes' if value != 'empty' else None)
            combo = table.execute_combo
            value = combo.get_model()[combo.get_active()][0]
            data.set_execute(value == 'Yes' if value != 'empty' else None)
            combo = table.browse_combo
            value = combo.get_model()[combo.get_active()][0]
            data.set_browse(value == 'Yes'if value != 'empty' else None)
            combo = table.delete_combo
            value = combo.get_model()[combo.get_active()][0]
            data.set_delete(value == 'Yes' if value != 'empty' else None)
            combo = table.write_combo
            value = combo.get_model()[combo.get_active()][0]
            data.set_write(value == 'Yes' if value != 'empty' else None)
        elif tag == 'RETURN':
            table = self.security_table.return_table
            combo = table.value_combo
            value = combo.get_model()[combo.get_active()][0]
            data.set_value(value if value != 'empty' else None)
        elif tag == 'DEFINE element' or tag == 'DEFINE tree':
            table = self.security_table.definition_table
            if table.name_check.get_active():
                data.set_name(table.name_entry.get_text())
            else:
                data.set_name(None)
            variables = []
            for variable in data.get_attributes():
                variables.append(variable)
            for variable in variables:
                data.remove_attribute(variable)
            model = table.attribute_tree.get_model()
            i = model.iter_children(None)
            while i is not None: # iterate over attributes
                data.set_attribute(
                    model.get_value(i, 0),
                    model.get_value(i, 1))
                i = model.iter_next(i)
            if tag == 'DEFINE element':
                if table.value_check.get_active():
                    data.set_attribute('value', table.value_entry.get_text())

    def update_gui(self, tag, data):
        if tag == 'USER':
            table = self.security_table.user_table
            if data.get_name() is None:
                table.name_check.set_active(False)
                table.name_entry.set_text('')
            else:
                table.name_check.set_active(True)
                table.name_entry.set_text(data.get_name())
            if data.get_password() is None:
                table.password_check.set_active(False)
                table.password_entry.set_text('')
            else:
                table.password_check.set_active(True)
                table.password_entry.set_text(data.get_password())
            combo = table.read_combo
            value =  2 if data.get_read() is None else int(not data.get_read())
            combo.set_active(value)
            combo = table.execute_combo
            value =  2 if data.get_execute() is None else int(not data.get_execute())
            combo.set_active(value)
            combo = table.browse_combo
            value =  2 if data.get_browse() is None else int(not data.get_browse())
            combo.set_active(value)
            combo = table.delete_combo
            value =  2 if data.get_delete() is None else int(not data.get_delete())
            combo.set_active(value)
            combo = table.write_combo
            value =  2 if data.get_write() is None else int(not data.get_write())
            combo.set_active(value)
        elif tag == 'CONDITION':
            table = self.security_table.condition_table
            if data.get_name() is None:
                table.name_check.set_active(False)
                table.name_entry.set_text('')
            else:
                table.name_check.set_active(True)
                table.name_entry.set_text(data.get_name())
            if data.get_value() is None:
                table.value_check.set_active(False)
                table.value_entry.set_text('')
            else:
                table.value_check.set_active(True)
                table.value_entry.set_text(data.get_value())
            combo = table.regex_combo
            value =  2 if data.get_regex() is None else int(not data.get_regex())
            combo.set_active(value)
            combo = table.reverse_combo
            value =  2 if data.get_reverse() is None else int(not data.get_reverse())
            combo.set_active(value)
        elif tag == 'PERMISSION':
            table = self.security_table.permission_table
            combo = table.read_combo
            value =  2 if data.get_read() is None else int(not data.get_read())
            combo.set_active(value)
            combo = table.execute_combo
            value =  2 if data.get_execute() is None else int(not data.get_execute())
            combo.set_active(value)
            combo = table.browse_combo
            value =  2 if data.get_browse() is None else int(not data.get_browse())
            combo.set_active(value)
            combo = table.delete_combo
            value =  2 if data.get_delete() is None else int(not data.get_delete())
            combo.set_active(value)
            combo = table.write_combo
            value =  2 if data.get_write() is None else int(not data.get_write())
            combo.set_active(value)
        elif tag == 'RETURN':
            table = self.security_table.return_table
            combo = table.value_combo
            value =  2 if data.get_value() is None else int(data.get_value() == 'DENY')
            combo.set_active(value)
        elif tag == 'DEFINE element' or tag == 'DEFINE tree':
            table = self.security_table.definition_table
            model = table.attribute_tree.get_model()
            model.clear()
            for variable, value in data.get_attributes().iteritems():
                if variable != 'value':
                    model.append((variable, value, ))
            if data.get_name() is None:
                table.name_check.set_active(False)
                table.name_entry.set_text('')
            else:
                table.name_check.set_active(True)
                table.name_entry.set_text(data.get_name())
            if tag == 'DEFINE element':
                try:
                    table.value_check.set_active(True)
                    table.value_entry.set_text(data.get_attribute('value'))
                except KeyError:
                    table.value_check.set_active(False)
                    table.value_entry.set_text('')

    def cursor_changed(self, tree):
        self.save()

        model, selected = tree.get_selection().get_selected()
        self.last_selected = selected
        row = model[selected]
        self.security_table.switch_table(row[0])

        self.update_gui(row[0], row[1])

    def add_sub_element(self, tag):
        model, selected = self.get_selection().get_selected()
        if selected is None:
            return
        if tag == 'USER':
            add = User()
        elif tag == 'RETURN':
            add = Return()
        elif tag == 'CONDITION':
            add = Condition()
        elif tag == 'PERMISSION':
            add = Permission()
        elif tag == 'DEFINE element':
            add = DefinitionElement()
        elif tag == 'DEFINE tree':
            add = DefinitionTree()
        model.append(selected, (tag, add, ))

    def remove_element(self):
        model, selected = self.get_selection().get_selected()
        model.remove(selected)
        self.last_selected = None
        self.security_table.switch_table('SECURITY')

    def export(self):
        def add_definition_tree(parent, i, model):
            while len(parent.get_definitions()): # remove all children
                parent.remove_definition(0)
            i = model.iter_children(i)
            while i is not None:
                tag = model.get_value(i, 0)
                data = model.get_value(i, 1)
                parent.add_definition(data)
                if tag == 'DEFINE tree':
                    add_definition_tree(data, i, model)
                i = model.iter_next(i)
        def add_condition(parent, i, model):
            while len(parent.get_sub_elements()): # remove all children
                parent.remove_sub_element(0)
            i = model.iter_children(i)
            while i is not None:
                tag = model.get_value(i, 0)
                data = model.get_value(i, 1)
                parent.add_sub_element(data)
                if tag == 'DEFINE tree':
                    add_definition_tree(data, i, model)
                elif tag == 'CONDITION':
                    add_condition(data, i, model)
                i = model.iter_next(i)
        self.save()
        model = self.get_model()
        i = model.iter_children(None)
        security = model.get_value(i, 1)
        while len(security.get_elements()): # remove all children
            security.remove_element(0)
        i = model.iter_children(i)
        while i is not None:
            tag = model.get_value(i, 0)
            data = model.get_value(i, 1)
            security.add_element(data)
            if tag == 'DEFINE tree':
                add_definition_tree(data, i, model)
            elif tag == 'CONDITION':
                add_condition(data, i, model)
            i = model.iter_next(i)
        return security

class UserTable(gtk.Table):
    def __init__(self, security_tree):
        gtk.Table.__init__(self, 8, 3)

        def remove_element(button, tree):
            tree.remove_element()
        button = gtk.Button('Remove this element')
        button.connect('clicked', remove_element, security_tree)
        self.attach(button, 0, 2, 0, 1, yoptions = gtk.FILL)

        self.attach(gtk.Label('name'), 0, 1, 1, 2, gtk.FILL, gtk.FILL)
        self.name_entry = gtk.Entry()
        self.attach(self.name_entry, 1, 2, 1, 2, yoptions = gtk.FILL)
        self.name_check = gtk.CheckButton()
        self.attach(self.name_check, 2, 3, 1, 2, gtk.FILL, gtk.FILL)

        self.attach(gtk.Label('password'), 0, 1, 2, 3, gtk.FILL, gtk.FILL)
        self.password_entry = gtk.Entry()
        self.attach(self.password_entry, 1, 2, 2, 3, yoptions = gtk.FILL)
        self.password_check = gtk.CheckButton()
        self.attach(self.password_check, 2, 3, 2, 3, gtk.FILL, gtk.FILL)

        def add_options(combo):
            combo.append_text('Yes')
            combo.append_text('No')
            combo.append_text('empty')

        self.attach(gtk.Label('READ'), 0, 1, 3, 4, gtk.FILL, gtk.FILL)
        self.read_combo = gtk.combo_box_new_text()
        add_options(self.read_combo)
        self.attach(self.read_combo, 1, 2, 3, 4, yoptions = gtk.FILL)

        self.attach(gtk.Label('EXECUTE'), 0, 1, 4, 5, gtk.FILL, gtk.FILL)
        self.execute_combo = gtk.combo_box_new_text()
        add_options(self.execute_combo)
        self.attach(self.execute_combo, 1, 2, 4, 5, yoptions = gtk.FILL)

        self.attach(gtk.Label('BROWSE'), 0, 1, 5, 6, gtk.FILL, gtk.FILL)
        self.browse_combo = gtk.combo_box_new_text()
        add_options(self.browse_combo)
        self.attach(self.browse_combo, 1, 2, 5, 6, yoptions = gtk.FILL)

        self.attach(gtk.Label('DELETE'), 0, 1, 6, 7, gtk.FILL, gtk.FILL)
        self.delete_combo = gtk.combo_box_new_text()
        add_options(self.delete_combo)
        self.attach(self.delete_combo, 1, 2, 6, 7, yoptions = gtk.FILL)

        self.attach(gtk.Label('WRITE'), 0, 1, 7, 8, gtk.FILL, gtk.FILL)
        self.write_combo = gtk.combo_box_new_text()
        add_options(self.write_combo)
        self.attach(self.write_combo, 1, 2, 7, 8, yoptions = gtk.FILL)

class ConditionTable(gtk.Table):
    def __init__(self, security_tree):
        gtk.Table.__init__(self, 6, 3)

        def remove_element(button, tree):
            tree.remove_element()
        button = gtk.Button('Remove this element')
        button.connect('clicked', remove_element, security_tree)
        self.attach(button, 0, 2, 0, 1, yoptions = gtk.FILL)

        self.attach(gtk.Label('name'), 0, 1, 1, 2, gtk.FILL, gtk.FILL)
        self.name_entry = gtk.Entry()
        self.attach(self.name_entry, 1, 2, 1, 2, yoptions = gtk.FILL)
        self.name_check = gtk.CheckButton()
        self.attach(self.name_check, 2, 3, 1, 2, gtk.FILL, gtk.FILL)

        self.attach(gtk.Label('value'), 0, 1, 2, 3, gtk.FILL, gtk.FILL)
        self.value_entry = gtk.Entry()
        self.attach(self.value_entry, 1, 2, 2, 3, yoptions = gtk.FILL)
        self.value_check = gtk.CheckButton()
        self.attach(self.value_check, 2, 3, 2, 3, gtk.FILL, gtk.FILL)

        def add_options(combo):
            combo.append_text('Yes')
            combo.append_text('No')
            combo.append_text('empty')

        self.attach(gtk.Label('regex'), 0, 1, 3, 4, gtk.FILL, gtk.FILL)
        self.regex_combo = gtk.combo_box_new_text()
        add_options(self.regex_combo)
        self.attach(self.regex_combo, 1, 2, 3, 4, yoptions = gtk.FILL)

        self.attach(gtk.Label('reverse'), 0, 1, 4, 5, gtk.FILL, gtk.FILL)
        self.reverse_combo = gtk.combo_box_new_text()
        add_options(self.reverse_combo)
        self.attach(self.reverse_combo, 1, 2, 4, 5, yoptions = gtk.FILL)

        def add_sub_element(button, combo, tree):
            tag = combo.get_model()[combo.get_active()][0]
            tree.add_sub_element(tag)
        self.add_sub_element_button = gtk.Button('Add sub-element')
        self.attach(self.add_sub_element_button, 0, 1, 5, 6, gtk.FILL, gtk.FILL)
        self.add_sub_element_combo = gtk.combo_box_new_text()
        self.add_sub_element_combo.append_text('USER')
        self.add_sub_element_combo.append_text('CONDITION')
        self.add_sub_element_combo.append_text('PERMISSION')
        self.add_sub_element_combo.append_text('RETURN')
        self.add_sub_element_combo.append_text('DEFINE element')
        self.add_sub_element_combo.append_text('DEFINE tree')
        self.add_sub_element_combo.set_active(0)
        self.attach(self.add_sub_element_combo, 1, 2, 5, 6, gtk.FILL, gtk.FILL)
        self.add_sub_element_button.connect('clicked', add_sub_element,
                                            self.add_sub_element_combo,
                                            security_tree)

class PermissionTable(gtk.Table):
    def __init__(self, security_tree):
        gtk.Table.__init__(self, 6, 2)

        def remove_element(button, tree):
            tree.remove_element()
        button = gtk.Button('Remove this element')
        button.connect('clicked', remove_element, security_tree)
        self.attach(button, 0, 2, 0, 1, yoptions = gtk.FILL)

        def add_options(combo):
            combo.append_text('Yes')
            combo.append_text('No')
            combo.append_text('empty')

        self.attach(gtk.Label('READ'), 0, 1, 1, 2, gtk.FILL, gtk.FILL)
        self.read_combo = gtk.combo_box_new_text()
        add_options(self.read_combo)
        self.attach(self.read_combo, 1, 2, 1, 2, yoptions = gtk.FILL)

        self.attach(gtk.Label('EXECUTE'), 0, 1, 2, 3, gtk.FILL, gtk.FILL)
        self.execute_combo = gtk.combo_box_new_text()
        add_options(self.execute_combo)
        self.attach(self.execute_combo, 1, 2, 2, 3, yoptions = gtk.FILL)

        self.attach(gtk.Label('BROWSE'), 0, 1, 3, 4, gtk.FILL, gtk.FILL)
        self.browse_combo = gtk.combo_box_new_text()
        add_options(self.browse_combo)
        self.attach(self.browse_combo, 1, 2, 3, 4, yoptions = gtk.FILL)

        self.attach(gtk.Label('DELETE'), 0, 1, 4, 5, gtk.FILL, gtk.FILL)
        self.delete_combo = gtk.combo_box_new_text()
        add_options(self.delete_combo)
        self.attach(self.delete_combo, 1, 2, 4, 5, yoptions = gtk.FILL)

        self.attach(gtk.Label('WRITE'), 0, 1, 5, 6, gtk.FILL, gtk.FILL)
        self.write_combo = gtk.combo_box_new_text()
        add_options(self.write_combo)
        self.attach(self.write_combo, 1, 2, 5, 6, yoptions = gtk.FILL)

class ReturnTable(gtk.Table):
    def __init__(self, security_tree):
        gtk.Table.__init__(self, 2, 2)

        def remove_element(button, tree):
            tree.remove_element()
        button = gtk.Button('Remove this element')
        button.connect('clicked', remove_element, security_tree)
        self.attach(button, 0, 2, 0, 1, yoptions = gtk.FILL)

        self.attach(gtk.Label('value'), 0, 1, 1, 2, gtk.FILL, gtk.FILL)
        self.value_combo = gtk.combo_box_new_text()
        self.value_combo.append_text('ALLOW')
        self.value_combo.append_text('DENY')
        self.value_combo.append_text('empty')
        self.attach(self.value_combo, 1, 2, 1, 2, yoptions = gtk.FILL)

class DefinitionTable(gtk.Table):
    def __init__(self, security_tree):
        gtk.Table.__init__(self, 7, 3)

        def remove_element(button, tree):
            tree.remove_element()
        button = gtk.Button('Remove this element')
        button.connect('clicked', remove_element, security_tree)
        self.attach(button, 0, 2, 0, 1, yoptions = gtk.FILL)

        def add_sub_element(button, combo, tree):
            tag = combo.get_model()[combo.get_active()][0]
            tree.add_sub_element(tag)
        self.add_sub_element_button = gtk.Button('Add sub-element')
        self.attach(self.add_sub_element_button, 0, 1, 1, 2, gtk.FILL, gtk.FILL)
        self.add_sub_element_combo = gtk.combo_box_new_text()
        self.add_sub_element_combo.append_text('DEFINE element')
        self.add_sub_element_combo.append_text('DEFINE tree')
        self.add_sub_element_combo.set_active(0)
        self.attach(self.add_sub_element_combo, 1, 2, 1, 2, gtk.FILL, gtk.FILL)
        self.add_sub_element_button.connect('clicked', add_sub_element,
                                            self.add_sub_element_combo,
                                            security_tree)

        self.attach(gtk.Label('name'), 0, 1, 2, 3, gtk.FILL, gtk.FILL)
        self.name_entry = gtk.Entry()
        self.attach(self.name_entry, 1, 2, 2, 3, yoptions = gtk.FILL)
        self.name_check = gtk.CheckButton()
        self.attach(self.name_check, 2, 3, 2, 3, gtk.FILL, gtk.FILL)

        self.value_label = gtk.Label('value')
        self.attach(self.value_label, 0, 1, 3, 4, gtk.FILL, gtk.FILL)
        self.value_entry = gtk.Entry()
        self.attach(self.value_entry, 1, 2, 3, 4, yoptions = gtk.FILL)
        self.value_check = gtk.CheckButton()
        self.attach(self.value_check, 2, 3, 3, 4, gtk.FILL, gtk.FILL)

        self.attribute_tree = gtk.TreeView(gtk.ListStore(
                gobject.TYPE_STRING, # variable
                gobject.TYPE_STRING)) # value
        model = self.attribute_tree.get_model()
        def edited_handler(cell, path, text, data):
            model, col = data
            model[path][col] = text
        renderer = gtk.CellRendererText()
        renderer.set_property('editable', True)
        renderer.connect('edited', edited_handler, (model, 0, ))
        column = gtk.TreeViewColumn('variable')
        column.pack_start(renderer)
        column.add_attribute(renderer, 'text', 0)
        self.attribute_tree.append_column(column)
        renderer = gtk.CellRendererText()
        renderer.set_property('editable', True)
        renderer.connect('edited', edited_handler, (model, 1, ))
        column = gtk.TreeViewColumn('value')
        column.pack_start(renderer)
        column.add_attribute(renderer, 'text', 1)
        self.attribute_tree.append_column(column)
        scroll = gtk.ScrolledWindow()
        scroll.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        scroll.set_shadow_type(gtk.SHADOW_OUT)
        scroll.set_border_width(5)
        scroll.add(self.attribute_tree)
        self.attach(scroll, 0, 2, 6, 7)

        def add_attribute(button, model):
            model.append(('', '', ))
        add_button = gtk.Button('add')
        add_button.connect('clicked', add_attribute, model)
        self.attach(add_button, 0, 2, 4, 5, gtk.FILL, gtk.FILL)

        def remove_attribute(button, tree):
            model, selected = tree.get_selection().get_selected()
            if selected is not None:
                model.remove(selected)
        remove_button = gtk.Button('remove')
        remove_button.connect('clicked', remove_attribute, self.attribute_tree)
        self.attach(remove_button, 0, 2, 5, 6, gtk.FILL, gtk.FILL)

    def set_tree(self, tree):
        if tree:
            self.add_sub_element_button.show()
            self.add_sub_element_combo.show()
            self.value_label.hide()
            self.value_entry.hide()
            self.value_check.hide()
        else:
            self.add_sub_element_button.hide()
            self.add_sub_element_combo.hide()
            self.value_label.show()
            self.value_entry.show()
            self.value_check.show()

class EmptyTable(gtk.Table):
    def __init__(self, security_tree):
        gtk.Table.__init__(self, 2, 2)

        self.attach(gtk.Label('Use security file'), 0, 1, 0, 1, gtk.FILL, gtk.FILL)
        self.security_check = gtk.CheckButton()
        self.attach(self.security_check, 1, 2, 0, 1, yoptions = gtk.FILL)

        def add_sub_element(button, combo, tree):
            tag = combo.get_model()[combo.get_active()][0]
            tree.add_sub_element(tag)
        self.add_sub_element_button = gtk.Button('Add sub-element')
        self.attach(self.add_sub_element_button, 0, 1, 1, 2, gtk.FILL, gtk.FILL)
        self.add_sub_element_combo = gtk.combo_box_new_text()
        self.add_sub_element_combo.append_text('USER')
        self.add_sub_element_combo.append_text('CONDITION')
        self.add_sub_element_combo.append_text('PERMISSION')
        self.add_sub_element_combo.append_text('RETURN')
        self.add_sub_element_combo.append_text('DEFINE element')
        self.add_sub_element_combo.append_text('DEFINE tree')
        self.add_sub_element_combo.set_active(0)
        self.attach(self.add_sub_element_combo, 1, 2, 1, 2, gtk.FILL, gtk.FILL)
        self.add_sub_element_button.connect('clicked', add_sub_element,
                                            self.add_sub_element_combo,
                                            security_tree)

class SecurityTable(gtk.Table):
    def __init__(self):
        gtk.Table.__init__(self, 1, 2)

        self.security_tree = SecurityTree(self)

        self.empty_table = EmptyTable(self.security_tree)
        self.user_table = UserTable(self.security_tree)
        self.condition_table = ConditionTable(self.security_tree)
        self.permission_table = PermissionTable(self.security_tree)
        self.return_table = ReturnTable(self.security_tree)
        self.definition_table = DefinitionTable(self.security_tree)
        self.table_notebook = gtk.Notebook()
        self.table_notebook.append_page(self.empty_table)
        self.table_notebook.append_page(self.user_table)
        self.table_notebook.append_page(self.condition_table)
        self.table_notebook.append_page(self.permission_table)
        self.table_notebook.append_page(self.return_table)
        self.table_notebook.append_page(self.definition_table)
        self.table_notebook.set_show_tabs(False)

        self.attach(self.security_tree.scroll, 0, 1, 0, 1)
        self.attach(self.table_notebook, 1, 2, 0, 1, gtk.FILL)

    def read_from_file(self, browser):
        self.switch_table('SECURITY')
        self.security_tree.clear()
        try:
            security = SecurityList.from_string(
                browser.get_file('security.xml'))
            self.security_tree.set_up(security)
        except IOError:
            self.security_tree.clear()

    def write_to_file(self, browser):
        if self.empty_table.security_check.get_active():
            security = self.security_tree.export()
            browser.put_file('security.xml', str(security))
        else:
            try:
                browser.remove_file('security.xml')
            except OSError: # file does not exist
                pass

    def switch_table(self, tag):
        if tag == 'SECURITY':
            self.current_table = self.empty_table
            self.table_notebook.set_current_page(0)
        elif tag == 'USER':
            self.current_table = self.user_table
            self.table_notebook.set_current_page(1)
        elif tag == 'CONDITION':
            self.current_table = self.condition_table
            self.table_notebook.set_current_page(2)
        elif tag == 'PERMISSION':
            self.current_table = self.permission_table
            self.table_notebook.set_current_page(3)
        elif tag == 'RETURN':
            self.current_table = self.return_table
            self.table_notebook.set_current_page(4)
        elif tag == 'DEFINE element':
            self.current_table = self.definition_table
            self.definition_table.set_tree(False)
            self.table_notebook.set_current_page(5)
        elif tag == 'DEFINE tree':
            self.current_table = self.definition_table
            self.definition_table.set_tree(True)
            self.table_notebook.set_current_page(5)
