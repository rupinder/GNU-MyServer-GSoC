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
        '''Create new Stream instance. filter is expected to be iterable.'''
        self.set_location(location)
        self.set_cycle(cycle)
        self.set_cycle_gzip(cycle_gzip)
        self.filter = []
        for single_filter in filter:
            self.add_filter(single_filter)

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
        '''Factory to produce stream from lxml.etree.Element object.'''
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
        '''Convert to lxml.etree.Element.'''
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

class Log():
    def __init__(self, log_type, stream = [], type = None):
        '''Create new Log instance. stream is expected to be iterable.'''
        self.set_log_type(log_type)
        self.set_type(type)
        self.stream = []
        for single_stream in stream:
            self.add_stream(single_stream)

    def __eq__(self, other):
        return isinstance(other, Log) and \
            self.log_type == other.log_type and \
            self.stream == other.stream and \
            self.type == other.type
    
    def get_type(self):
        '''Get log's type attribute.'''
        return self.type

    def set_type(self, type):
        '''Set log's type attribute, None means not set'''
        self.type = type

    def get_log_type(self):
        '''Get log's type.'''
        return self.log_type

    def set_log_type(self, log_type):
        '''Set log's type.'''
        if log_type is None:
            raise AttributeError('log_type is required and can\'t be None')
        self.log_type = log_type

    def get_streams(self):
        '''Get streams associated with this log.'''
        return self.stream

    def add_stream(self, stream, index = None):
        '''Append stream to current streams or if index is not None insert
        stream ad index-th position.'''
        if index is None:
            self.stream.append(stream)
        else:
            self.stream.insert(index, stream)

    def remove_stream(self, index):
        '''Remove stream from index-th position.'''
        self.stream.pop(index)
    
    def get_stream(self, index):
        '''Get index-th stream.'''
        return self.stream[index]

    @staticmethod
    def from_string(text):
        '''Factory to produce log by parsing a string.'''
        return Log.from_lxml_element(etree.XML(text))

    @staticmethod
    def from_lxml_element(root):
        '''Factory to produce log from lxml.etree.Element object.'''
        log_type = root.tag
        type = root.get('type', None)
        stream = []
        for child in list(root):
            if child.tag == 'STREAM':
                stream.append(Stream.from_lxml_element(child))
        return Log(log_type, stream, type)

    def __str__(self):
        return etree.tostring(self.to_lxml_element(), pretty_print = True)

    def to_lxml_element(self):
        '''Convert to lxml.etree.Element.'''
        root = etree.Element(self.log_type)
        if self.type is not None:
            root.set('type', self.type)
        for stream in self.stream:
            root.append(stream.to_lxml_element())
        return root
