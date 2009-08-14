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

import os

class FileBrowser():
    'Abstract file browser class.'
    def list_dir(self):
        raise NotImplementedError()

    def list_files(self):
        raise NotImplementedError()
    
    def get_file(self, path):
        raise NotImplementedError()
    
    def put_file(self, path, text):
        raise NotImplementedError()
    
    def get_path(self):
        raise NotImplementedError()

    def change_dir(self, path):
        raise NotImplementedError()

    def show_hidden(self, show):
        raise NotImplementedError()

class LocalFileBrowser(FileBrowser):
    'Local file browser class.'
    def __init__(self, path = os.path.expanduser('~')):
        self.path = path
        self.show = False

    def list_dir(self):
        return ['..'] + sorted(
            [path for path in os.listdir(self.path) if \
                 not os.path.isfile(os.path.join(self.path, path)) \
                and (self.show or not path.startswith('.'))])

    def list_files(self):
        return ['..'] + sorted(
            [path for path in os.listdir(self.path) if \
                 os.path.isfile(os.path.join(self.path, path)) \
                and (self.show or not path.startswith('.'))])

    def get_file(self, path):
        with open(os.path.join(self.path, path)) as f:
            return f.read()

    def put_file(self, path, text):
        with open(os.path.join(self.path, path), 'w') as f:
            f.write(text) 

    def get_path(self):
        return self.path

    def change_dir(self, path):
        if path == '..':
            self.path = os.path.split(self.path)[0]
        else:
            self.path = os.path.join(self.path, path)

    def show_hidden(self, show):
        self.show = show
