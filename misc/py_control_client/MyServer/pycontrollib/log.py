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
    def __init__(self, location = None, cycle = None, cycle_gzip = None, filters = []):
        '''Create new Stream instance. filter is expected to be iterable.'''
        self.set_location(location)
        self.set_cycle(cycle)
        self.set_cycle_gzip(cycle_gzip)
        self.filters = []
        for filter in filters:
            self.add_filter(filter)

    def __eq__(self, other):
        return isinstance(other, Stream) and \
            self.location == other.location and \
            self.cycle == other.cycle and \
            self.cycle_gzip == other.cycle_gzip and \
            self.filters == other.filters

    def get_location(self):
        '''Get stream location.'''
        return self.location

    def set_location(self, location):
        '''Set stream location.'''
        self.location = location

    def get_cycle(self):
        '''Get stream cycle value.'''
        return self.cycle

    def set_cycle(self, cycle):
        '''Set cycle value, None means not set.'''
        if cycle is not None:
            self.cycle = int(cycle)
        else:
            self.cycle = cycle

    def get_cycle_gzip(self):
        '''Get stream cycle_gzip value.'''
        return self.cycle_gzip

    def set_cycle_gzip(self, cycle_gzip):
        '''Set stream cycle_gzip value, None means not set'''
        self.cycle_gzip = cycle_gzip

    def get_filters(self):
        '''Get list of stream filters.'''
        return self.filters

    def get_filter(self, index):
        '''Get index-th filter.'''
        return self.filters[index]

    def add_filter(self, filter, index = None):
        '''Append a new filter, or insert it at index position.'''
        if index is None:
            self.filters.append(filter)
        else:
            self.filters.insert(index, filter)

    def remove_filter(self, index):
        '''Remove index-th filter.'''
        self.filters.pop(index)

    @staticmethod
    def from_string(text):
        '''Factory to produce stream by parsing a string.'''
        return Stream.from_lxml_element(etree.XML(
                text, parser = etree.XMLParser(remove_comments = True)))

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
        filters = []
        for child in list(root):
            if child.tag == 'FILTER':
                filters.append(child.text)
        return Stream(location, cycle, cycle_gzip, filters)

    def __str__(self):
        return etree.tostring(self.to_lxml_element(), pretty_print = True)

    def to_lxml_element(self):
        '''Convert to lxml.etree.Element.'''
        root = etree.Element('STREAM')
        if self.location is not None:
            root.set('location', self.location)
        if self.cycle is not None:
            root.set('cycle', str(self.cycle))
        if self.cycle_gzip is not None:
            root.set('cycle_gzip', 'YES' if self.cycle_gzip else 'NO')
        for filter in self.filters:
            element = etree.Element('FILTER')
            element.text = filter
            root.append(element)
        return root

class Log():
    def __init__(self, log_type, streams = [], type = None):
        '''Create new Log instance. stream is expected to be iterable.'''
        self.set_log_type(log_type)
        self.set_type(type)
        self.streams = []
        for stream in streams:
            self.add_stream(stream)

    def __eq__(self, other):
        return isinstance(other, Log) and \
            self.log_type == other.log_type and \
            self.streams == other.streams and \
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
        return self.streams

    def add_stream(self, stream, index = None):
        '''Append stream to current streams or if index is not None insert
        stream ad index-th position.'''
        if index is None:
            self.streams.append(stream)
        else:
            self.streams.insert(index, stream)

    def remove_stream(self, index):
        '''Remove stream from index-th position.'''
        self.streams.pop(index)
    
    def get_stream(self, index):
        '''Get index-th stream.'''
        return self.streams[index]

    @staticmethod
    def from_string(text):
        '''Factory to produce log by parsing a string.'''
        return Log.from_lxml_element(etree.XML(
                text, parser = etree.XMLParser(remove_comments = True)))

    @staticmethod
    def from_lxml_element(root):
        '''Factory to produce log from lxml.etree.Element object.'''
        log_type = root.tag
        type = root.get('type', None)
        streams = []
        for child in list(root):
            if child.tag == 'STREAM':
                streams.append(Stream.from_lxml_element(child))
        return Log(log_type, streams, type)

    def __str__(self):
        return etree.tostring(self.to_lxml_element(), pretty_print = True)

    def to_lxml_element(self):
        '''Convert to lxml.etree.Element.'''
        root = etree.Element(self.log_type)
        if self.type is not None:
            root.set('type', self.type)
        for stream in self.streams:
            root.append(stream.to_lxml_element())
        return root
