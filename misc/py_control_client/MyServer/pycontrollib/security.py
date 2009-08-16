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

from lxml import etree
from definition import Definition

class SecurityElement():
    @staticmethod
    def from_lxml_element(root):
        if root.tag == 'USER':
            return User.from_lxml_element(root)
        if root.tag == 'CONDITION':
            return Condition.from_lxml_element(root)
        elif root.tag == 'PERMISSION':
            return Permission.from_lxml_element(root)
        elif root.tag == 'RETURN':
            return Return.from_lxml_element(root)
        elif root.tag == 'DEFINE':
            return Definition.from_lxml_element(root)
        else:
            raise AttributeError(
                '{0} is not allowed in security files'.format(root.tag))

    @staticmethod
    def from_string(text):
        '''Factory to produce security element by parsing a string.'''
        return SecurityElement.from_lxml_element(etree.XML(text))

    def __str__(self):
        return etree.tostring(self.to_lxml_element(), pretty_print = True)

class Condition(SecurityElement):
    def __init__(self, name = None, value = None, reverse = None, regex = None,
                 sub_elements = []):
        self.set_name(name)
        self.set_value(value)
        self.set_reverse(reverse)
        self.set_regex(regex)
        self.custom_attrib = {}
        self.custom = []
        self.sub_elements = []
        for element in sub_elements:
            self.add_sub_element(element)

    def __eq__(self, other):
        return isinstance(other, Condition) and self.name == other.name and \
            self.value == other.value and self.reverse == other.reverse and \
            self.regex == other.regex and \
            self.sub_elements == other.sub_elements

    def set_name(self, name):
        self.name = name

    def get_name(self):
        return self.name

    def set_value(self, value):
        self.value = value

    def get_value(self):
        return self.value

    def set_reverse(self, reverse):
        self.reverse = reverse

    def get_reverse(self):
        return self.reverse

    def set_regex(self, regex):
        self.regex = regex

    def get_regex(self):
        return self.regex

    def add_sub_element(self, element, index = None):
        if isinstance(element, Definition) or \
                isinstance(element, SecurityElement):
            if index is None:
                self.sub_elements.append(element)
            else:
                self.sub_elements.insert(index, element)
        else:
            self.custom.append(element)

    def get_sub_elements(self):
        return self.sub_elements

    def get_sub_element(self, index):
        return self.sub_elements[index]

    def remove_sub_element(self, index):
        self.sub_elements.pop(index)

    def to_lxml_element(self):
        root = etree.Element('CONDITION')
        if self.name is not None:
            root.set('name', self.name)
        if self.value is not None:
            root.set('value', self.value)
        if self.reverse is not None:
            root.set('not', 'yes' if self.reverse else 'no')
        if self.regex is not None:
            root.set('regex', 'yes' if self.regex else 'no')
        for key, value in self.custom_attrib.iteritems():
            root.set(key, value)
        for element in self.sub_elements:
            root.append(element.to_lxml_element())
        for element in self.custom:
            root.append(element)
        return root

    @staticmethod
    def from_lxml_element(root):
        if root.tag != 'CONDITION':
            raise AttributeError('Expected CONDITION tag.')
        custom_attrib = root.attrib
        name = custom_attrib.pop('name', None)
        value = custom_attrib.pop('value', None)
        reverse = custom_attrib.pop('not', None)
        if reverse is not None:
            reverse = reverse.upper() == 'YES'
        regex = custom_attrib.pop('regex', None)
        if regex is not None:
            regex = regex.upper() == 'YES'
        custom = []
        sub_elements = []
        for child in list(root):
            try:
                sub_elements.append(SecurityElement.from_lxml_element(child))
            except:
                custom.append(child)
        condition = Condition(name, value, reverse, regex, sub_elements)
        condition.custom = custom
        condition.custom_attrib = custom_attrib
        return condition

    @staticmethod
    def from_string(text):
        '''Factory to produce condition element by parsing a string.'''
        return Condition.from_lxml_element(etree.XML(text))

class Permission(SecurityElement):
    def __init__(self, read = None, execute = None, browse = None,
                 delete = None, write = None):
        self.set_read(read)
        self.set_execute(execute)
        self.set_browse(browse)
        self.set_delete(delete)
        self.set_write(write)
        self.custom_attrib = {}
        self.custom = []

    def __eq__(self, other):
        return isinstance(other, Permission) and self.read == other.read and \
            self.execute == other.execute and self.browse == other.browse and \
            self.delete == other.delete and self.write == other.write

    def set_read(self, read):
        self.read = read

    def get_read(self):
        return self.read

    def set_execute(self, execute):
        self.execute = execute

    def get_execute(self):
        return self.execute

    def set_browse(self, browse):
        self.browse = browse

    def get_browse(self):
        return self.browse

    def set_delete(self, delete):
        self.delete = delete

    def get_delete(self):
        return self.delete

    def set_write(self, write):
        self.write = write

    def get_write(self):
        return self.write

    def to_lxml_element(self):
        root = etree.Element('PERMISSION')
        if self.read is not None:
            root.set('READ', 'YES' if self.read else 'NO')
        if self.execute is not None:
            root.set('EXECUTE', 'YES' if self.execute else 'NO')
        if self.browse is not None:
            root.set('BROWSE', 'YES' if self.browse else 'NO')
        if self.delete is not None:
            root.set('DELETE', 'YES' if self.delete else 'NO')
        if self.write is not None:
            root.set('WRITE', 'YES' if self.write else 'NO')
        for key, value in self.custom_attrib.iteritems():
            root.set(key, value)
        for element in self.custom:
            root.append(element)
        return root

    @staticmethod
    def from_lxml_element(root):
        if root.tag != 'PERMISSION':
            raise AttributeError('Expected PERMISSION tag.')
        custom_attrib = root.attrib
        read = custom_attrib.pop('READ', None)
        if read is not None:
            read = read.upper() == 'YES'
        execute = custom_attrib.pop('EXECUTE', None)
        if execute is not None:
            execute = execute.upper() == 'YES'
        browse = custom_attrib.pop('BROWSE', None)
        if browse is not None:
            browse = browse.upper() == 'YES'
        delete = custom_attrib.pop('DELETE', None)
        if delete is not None:
            delete = delete.upper() == 'YES'
        write = custom_attrib.pop('WRITE', None)
        if write is not None:
            write = write.upper() == 'YES'
        custom = list(root)
        permission = Permission(read, execute, browse, delete, write)
        permission.custom_attrib = custom_attrib
        permission.custom = custom
        return permission

    @staticmethod
    def from_string(text):
        '''Factory to produce permission element by parsing a string.'''
        return Permission.from_lxml_element(etree.XML(text))

class User(SecurityElement):
    def __init__(self, name = None, password = None, read = None,
                 execute = None, browse = None, delete = None, write = None):
        self.set_name(name)
        self.set_password(password)
        self.set_read(read)
        self.set_execute(execute)
        self.set_browse(browse)
        self.set_delete(delete)
        self.set_write(write)
        self.custom_attrib = {}
        self.custom = []

    def __eq__(self, other):
        return isinstance(other, User) and self.name == other.name and \
            self.password == other.password and self.read == other.read and \
            self.execute == other.execute and self.browse == other.browse and \
            self.delete == other.delete and self.write == other.write

    def set_name(self, name):
        self.name = name

    def get_name(self):
        return self.name

    def set_password(self, password):
        self.password = password

    def get_password(self):
        return self.password

    def set_read(self, read):
        self.read = read

    def get_read(self):
        return self.read

    def set_execute(self, execute):
        self.execute = execute

    def get_execute(self):
        return self.execute

    def set_browse(self, browse):
        self.browse = browse

    def get_browse(self):
        return self.browse

    def set_delete(self, delete):
        self.delete = delete

    def get_delete(self):
        return self.delete

    def set_write(self, write):
        self.write = write

    def get_write(self):
        return self.write

    def to_lxml_element(self):
        root = etree.Element('USER')
        if self.name is not None:
            root.set('name', self.name)
        if self.password is not None:
            root.set('password', self.password)
        if self.read is not None:
            root.set('READ', 'YES' if self.read else 'NO')
        if self.execute is not None:
            root.set('EXECUTE', 'YES' if self.execute else 'NO')
        if self.browse is not None:
            root.set('BROWSE', 'YES' if self.browse else 'NO')
        if self.delete is not None:
            root.set('DELETE', 'YES' if self.delete else 'NO')
        if self.write is not None:
            root.set('WRITE', 'YES' if self.write else 'NO')
        for key, value in self.custom_attrib.iteritems():
            root.set(key, value)
        for element in self.custom:
            root.append(element)
        return root

    @staticmethod
    def from_lxml_element(root):
        if root.tag != 'USER':
            raise AttributeError('Expected USER tag.')
        custom_attrib = root.attrib
        name = custom_attrib.pop('name', None)
        password = custom_attrib.pop('password', None)
        read = custom_attrib.pop('READ', None)
        if read is not None:
            read = read.upper() == 'YES'
        execute = custom_attrib.pop('EXECUTE', None)
        if execute is not None:
            execute = execute.upper() == 'YES'
        browse = custom_attrib.pop('BROWSE', None)
        if browse is not None:
            browse = browse.upper() == 'YES'
        delete = custom_attrib.pop('DELETE', None)
        if delete is not None:
            delete = delete.upper() == 'YES'
        write = custom_attrib.pop('WRITE', None)
        if write is not None:
            write = write.upper() == 'YES'
        custom = list(root)
        user = User(name, password, read, execute, browse,
                    delete, write)
        user.custom_attrib = custom_attrib
        user.custom = custom
        return user

    @staticmethod
    def from_string(text):
        '''Factory to produce user element by parsing a string.'''
        return User.from_lxml_element(etree.XML(text))

class Return(SecurityElement):
    def __init__(self, value = None):
        self.set_value(value)
        self.custom_attrib = {}
        self.custom = []

    def __eq__(self, other):
        return isinstance(other, Return) and self.value == other.value

    def set_value(self, value):
        self.value = value

    def get_value(self):
        return self.value

    def to_lxml_element(self):
        root = etree.Element('RETURN')
        if self.value is not None:
            root.set('value', self.value)
        for key, value in self.custom_attrib.iteritems():
            root.set(key, value)
        for element in self.custom:
            root.append(element)
        return root

    @staticmethod
    def from_lxml_element(root):
        if root.tag != 'RETURN':
            raise AttributeError('Expected RETURN tag.')
        custom_attrib = root.attrib
        value = custom_attrib.pop('value', None)
        custom = list(root)
        ret = Return(value)
        ret.custom_attrib = custom_attrib
        ret.custom = custom
        return ret

    @staticmethod
    def from_string(text):
        '''Factory to produce return element by parsing a string.'''
        return Return.from_lxml_element(etree.XML(text))

class SecurityList():
    def __init__(self, elements = []):
        for elements in elements:
            self.add_element(element)
        self.custom = []
        self.custom_attrib = {}

    def get_elements(self):
        return self.elements

    def add_element(self, element, index = None):
        if index is None:
            self.elements.append(element)
        else:
            self.elements.insert(index, element)

    def remove_element(self, index):
        self.elements.pop(index)

    @staticmethod
    def from_lxml_element(root):
        if root.tag != 'SECURITY':
            raise AttributeError('Expected SECURITY tag.')
        elements = []
        custom = []
        for element in list(root):
            try:
                elements.append(SecurityElement.from_lxml_element(element))
            except:
                custom.append(element)
        security_list = SecurityList(elements)
        security_list.custom = custom
        security_list.custom_attrib = root.attrib
        return security_list

    @staticmethod
    def from_string(text):
        '''Factory to produce security list by parsing a string.'''
        return SecurityList.from_lxml_element(etree.XML(text))

    def to_lxml_element(self):
        root = etree.Element('SECURITY')
        for element in self.elements:
            root.append(element.to_lxml_element())
        for key, value in self.custom_attrib.iteritems():
            root.set(key, value)
        for element in self.custom:
            root.append(element)
        return root

    def __str__(self):
        return etree.tostring(self.to_lxml_element(), pretty_print = True)
