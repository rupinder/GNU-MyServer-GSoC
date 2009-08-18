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

        self.connect('cursor-changed', self.cursor_changed)

        self.scroll = gtk.ScrolledWindow()
        self.scroll.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        self.scroll.set_shadow_type(gtk.SHADOW_OUT)
        self.scroll.set_border_width(5)
        self.scroll.add(self)

        x = self.get_model().append(None, ('SECURITY', '', ))
        self.get_model().append(x, ('USER', '', ))
        self.get_model().append(x, ('RETURN', '', ))
        self.get_model().append(x, ('CONDITION', '', ))

    def cursor_changed(self, tree):
        model, selected = tree.get_selection().get_selected()
        row = model[selected]
        self.security_table.switch_table(row[0])

    def add_sub_element(self, tag):
        model, selected = self.get_selection().get_selected()
        model.append(selected, (tag, '', ))

class UserTable(gtk.Table):
    def __init__(self):
        gtk.Table.__init__(self, 7, 3)

        self.attach(gtk.Label('name'), 0, 1, 0, 1, gtk.FILL, gtk.FILL)
        self.name_entry = gtk.Entry()
        self.attach(self.name_entry, 1, 2, 0, 1, yoptions = gtk.FILL)
        self.name_check = gtk.CheckButton()
        self.attach(self.name_check, 2, 3, 0, 1, gtk.FILL, gtk.FILL)

        self.attach(gtk.Label('password'), 0, 1, 1, 2, gtk.FILL, gtk.FILL)
        self.password_entry = gtk.Entry()
        self.attach(self.password_entry, 1, 2, 1, 2, yoptions = gtk.FILL)
        self.password_check = gtk.CheckButton()
        self.attach(self.password_check, 2, 3, 1, 2, gtk.FILL, gtk.FILL)

        def add_options(combo):
            combo.append_text('YES')
            combo.append_text('NO')
            combo.append_text('empty')

        self.attach(gtk.Label('READ'), 0, 1, 2, 3, gtk.FILL, gtk.FILL)
        self.read_combo = gtk.combo_box_new_text()
        add_options(self.read_combo)
        self.attach(self.read_combo, 1, 2, 2, 3, yoptions = gtk.FILL)

        self.attach(gtk.Label('EXECUTE'), 0, 1, 3, 4, gtk.FILL, gtk.FILL)
        self.execute_combo = gtk.combo_box_new_text()
        add_options(self.execute_combo)
        self.attach(self.execute_combo, 1, 2, 3, 4, yoptions = gtk.FILL)

        self.attach(gtk.Label('BROWSE'), 0, 1, 4, 5, gtk.FILL, gtk.FILL)
        self.browse_combo = gtk.combo_box_new_text()
        add_options(self.browse_combo)
        self.attach(self.browse_combo, 1, 2, 4, 5, yoptions = gtk.FILL)

        self.attach(gtk.Label('DELETE'), 0, 1, 5, 6, gtk.FILL, gtk.FILL)
        self.delete_combo = gtk.combo_box_new_text()
        add_options(self.delete_combo)
        self.attach(self.delete_combo, 1, 2, 5, 6, yoptions = gtk.FILL)

        self.attach(gtk.Label('WRITE'), 0, 1, 6, 7, gtk.FILL, gtk.FILL)
        self.write_combo = gtk.combo_box_new_text()
        add_options(self.write_combo)
        self.attach(self.write_combo, 1, 2, 6, 7, yoptions = gtk.FILL)

class ConditionTable(gtk.Table):
    def __init__(self, security_tree):
        gtk.Table.__init__(self, 5, 3)

        self.attach(gtk.Label('name'), 0, 1, 0, 1, gtk.FILL, gtk.FILL)
        self.name_entry = gtk.Entry()
        self.attach(self.name_entry, 1, 2, 0, 1, yoptions = gtk.FILL)
        self.name_check = gtk.CheckButton()
        self.attach(self.name_check, 2, 3, 0, 1, gtk.FILL, gtk.FILL)

        self.attach(gtk.Label('value'), 0, 1, 1, 2, gtk.FILL, gtk.FILL)
        self.value_entry = gtk.Entry()
        self.attach(self.value_entry, 1, 2, 1, 2, yoptions = gtk.FILL)
        self.value_check = gtk.CheckButton()
        self.attach(self.value_check, 2, 3, 0, 1, gtk.FILL, gtk.FILL)

        def add_options(combo):
            combo.append_text('yes')
            combo.append_text('no')
            combo.append_text('empty')

        self.attach(gtk.Label('regex'), 0, 1, 2, 3, gtk.FILL, gtk.FILL)
        self.regex_combo = gtk.combo_box_new_text()
        add_options(self.regex_combo)
        self.attach(self.regex_combo, 1, 2, 2, 3, yoptions = gtk.FILL)

        self.attach(gtk.Label('reverse'), 0, 1, 3, 4, gtk.FILL, gtk.FILL)
        self.reverse_combo = gtk.combo_box_new_text()
        add_options(self.reverse_combo)
        self.attach(self.reverse_combo, 1, 2, 3, 4, yoptions = gtk.FILL)

        def add_sub_element(button, combo, tree):
            tag = combo.get_model()[combo.get_active()][0]
            tree.add_sub_element(tag)
        self.add_sub_element_button = gtk.Button('Add sub-element')
        self.attach(self.add_sub_element_button, 0, 1, 4, 5, gtk.FILL, gtk.FILL)
        self.add_sub_element_combo = gtk.combo_box_new_text()
        self.add_sub_element_combo.append_text('USER')
        self.add_sub_element_combo.append_text('CONDITION')
        self.add_sub_element_combo.append_text('PERMISSION')
        self.add_sub_element_combo.append_text('RETURN')
        self.add_sub_element_combo.append_text('DEFINE')
        self.add_sub_element_combo.set_active(0)
        self.attach(self.add_sub_element_combo, 1, 2, 4, 5, gtk.FILL, gtk.FILL)
        self.add_sub_element_button.connect('clicked', add_sub_element,
                                            self.add_sub_element_combo,
                                            security_tree)

class PermissionTable(gtk.Table):
    def __init__(self):
        gtk.Table.__init__(self, 5, 2)

        def add_options(combo):
            combo.append_text('YES')
            combo.append_text('NO')
            combo.append_text('empty')

        self.attach(gtk.Label('READ'), 0, 1, 0, 1, gtk.FILL, gtk.FILL)
        self.read_combo = gtk.combo_box_new_text()
        add_options(self.read_combo)
        self.attach(self.read_combo, 1, 2, 0, 1, yoptions = gtk.FILL)

        self.attach(gtk.Label('EXECUTE'), 0, 1, 1, 2, gtk.FILL, gtk.FILL)
        self.execute_combo = gtk.combo_box_new_text()
        add_options(self.execute_combo)
        self.attach(self.execute_combo, 1, 2, 1, 2, yoptions = gtk.FILL)

        self.attach(gtk.Label('BROWSE'), 0, 1, 2, 3, gtk.FILL, gtk.FILL)
        self.browse_combo = gtk.combo_box_new_text()
        add_options(self.browse_combo)
        self.attach(self.browse_combo, 1, 2, 2, 3, yoptions = gtk.FILL)

        self.attach(gtk.Label('DELETE'), 0, 1, 3, 4, gtk.FILL, gtk.FILL)
        self.delete_combo = gtk.combo_box_new_text()
        add_options(self.delete_combo)
        self.attach(self.delete_combo, 1, 2, 3, 4, yoptions = gtk.FILL)

        self.attach(gtk.Label('WRITE'), 0, 1, 4, 5, gtk.FILL, gtk.FILL)
        self.write_combo = gtk.combo_box_new_text()
        add_options(self.write_combo)
        self.attach(self.write_combo, 1, 2, 4, 5, yoptions = gtk.FILL)

class ReturnTable(gtk.Table):
    def __init__(self):
        gtk.Table.__init__(self, 1, 2)

        self.attach(gtk.Label('value'), 0, 1, 0, 1, gtk.FILL, gtk.FILL)
        self.value_combo = gtk.combo_box_new_text()
        self.value_combo.append_text('ALLOW')
        self.value_combo.append_text('DENY')
        self.value_combo.append_text('empty')
        self.attach(self.value_combo, 1, 2, 0, 1, yoptions = gtk.FILL)

class DefinitionTable(gtk.Table):
    def __init__(self):
        gtk.Table.__init__(self, 1, 2)

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
        self.add_sub_element_combo.append_text('DEFINE')
        self.add_sub_element_combo.set_active(0)
        self.attach(self.add_sub_element_combo, 1, 2, 1, 2, gtk.FILL, gtk.FILL)
        self.add_sub_element_button.connect('clicked', add_sub_element,
                                            self.add_sub_element_combo,
                                            security_tree)

class SecurityTable(gtk.Table):
    def __init__(self):
        gtk.Table.__init__(self, 1, 2)

        security_tree = SecurityTree(self)

        self.user_table = UserTable()
        self.condition_table = ConditionTable(security_tree)
        self.permission_table = PermissionTable()
        self.return_table = ReturnTable()
        self.definition_table = DefinitionTable()
        self.empty_table = EmptyTable(security_tree)
        self.current_table = self.empty_table

        self.attach(security_tree.scroll, 0, 1, 0, 1)
        self.attach(self.current_table, 1, 2, 0, 1, gtk.FILL)

    def switch_table(self, tag): # switch doesn't work
        self.remove(self.current_table)
        if tag == 'USER':
            self.current_table = self.user_table
        elif tag == 'CONDITION':
            self.current_table = self.condition_table
        elif tag == 'PERMISSION':
            self.current_table = self.permission_table
        elif tag == 'RETURN':
            self.current_table = self.return_table
        elif tag == 'DEFINITION':
            self.current_table = self.definition_table
        elif tag == 'SECURITY':
            self.current_table = self.empty_table
        self.attach(self.current_table, 1, 2, 0, 1, gtk.FILL)
