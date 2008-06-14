# MyServer
# Copyright (C) 2002, 2003, 2004, 2007, 2008 The MyServer Team
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


import sys
try:
    import pygtk
    pygtk.require("2.0")
except:
    pass
try:
    import gtk
    import gtk.glade
except:
    sys.exit(1)

class ConfigGUIGTK:
    """ Configuration Tool GUI"""
    
    def __init__(self):
        
        #glade file
        self.gladefile = "XMLGui.glade"
        self.wTree = gtk.glade.XML(self.gladefile)
        
        # Bind main window
        self.window = self.wTree.get_widget("ConfigUI")
        # Bind "destroy" event
        if (self.window):
            self.window.connect("destroy", gtk.main_quit)

if __name__ == "__main__":
    ConfigUI = ConfigGUIGTK()
    gtk.main()
