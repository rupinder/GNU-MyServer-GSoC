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

class Stream():
    def __init__(self, location, cycle = None, cycle_gzip = None, filter = []):
        self.location = location
        self.cycle = cycle
        self.cycle_gzip = cycle_gzip
        self.filter = filter

    def __eq__(self, other):
        return isinstance(other, Stream) and \
            self.location == other.location and \
            self.cycle == other.cycle and \
            self.cycle_gzip == other.cycle_gzip and \
            self.filter == other.filter

    def get_location(self):
        '''Get stream location.'''
        return self.location

    def set_location(self, location):
        '''Set stream location.'''
        if location is None:
            raise AttributeError('location is required and can\'t be None')
        self.location = location

    def get_cycle(self):
        '''Get stream cycle value.'''
        return self.cycle

    def set_cycle(self, cycle):
        '''Set cycle value, None means not set.'''
        self.cycle = cycle

    def get_cycle_gzip(self):
        '''Get stream cycle_gzip value.'''
        return self.cycle_gzip

    def set_cycle_gzip(self, cycle_gzip):
        '''Set stream cycle_gzip value, None means not set'''
        self.cycle_gzip = cycle_gzip

    def get_filters(self):
        '''Get list of stream filters.'''
        return self.filter

    def get_filter(self, index):
        '''Get index-th filter.'''
        return self.filter[index]

    def add_filter(self, filter, index = None):
        '''Append a new filter, or insert it at index position.'''
        if index is None:
            self.filter.append(filter)
        else:
            self.filter.insert(index, filter)

    def remove_filter(self, index):
        '''Remove index-th filter.'''
        self.filter.pop(index)

    @staticmethod
    def from_string(text):
        '''Factory to produce stream by parsing a string.'''
        return Stream.from_lxml_element(etree.XML(text))

    @staticmethod
    def from_lxml_element(root):
        '''Factory to produce stream from etree.Element object.'''
        if root.tag != 'STREAM':
            raise AttributeError('Expected STREAM tag.')
        location = root.get('location')
        cycle = root.get('cycle', None)
        cycle_gzip = root.get('cycle_gzip', None)
        if cycle_gzip is not None:
            cycle_gzip = True if cycle_gzip == 'YES' else False
        filter = []
        for child in list(root):
            if child.tag == 'FILTER':
                filter.append(child.text)
        return Stream(location, cycle, cycle_gzip, filter)

    def __str__(self):
        return etree.tostring(self.to_lxml_element(), pretty_print = True)

    def to_lxml_element(self):
        root = etree.Element('STREAM')
        root.set('location', self.location)
        if self.cycle is not None:
            root.set('cycle', self.cycle)
        if self.cycle_gzip is not None:
            root.set('cycle_gzip', 'YES' if self.cycle_gzip else 'NO')
        for filter in self.filter:
            element = etree.Element('FILTER')
            element.text = filter
            root.append(element)
        return root
