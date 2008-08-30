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

from xml.etree import ElementTree as ET

class MIMEtype:
    """This type wraps MIME types described in MIMEtypes.xml """

    def __init__(self, ext, MIME, CMD, MANAGER):
        self.ext = ext
        self.MIME = MIME
        self.CMD = CMD
        self.MANAGER = MANAGER
    
    #TODO: Override __str__ method
        
  
def ParseTypes():
    """ Parse types """
    sock = open("MIMEtypes.xml")
    tree = ET.parse(sock)
    sock.close()
    elem = tree.getroot()

    # Create nd populate a list with MIME types
    MTlist = []
    for MTypes in elem:
        ext = MTypes.findtext("EXT")
        mime = MTypes.findtext("MIME")
        cmd = MTypes.findtext("CMD")
        manager = MTypes.findtext("MANAGER")
        
        MT = MIMEtype(ext, mime, cmd, elem)
        
        MTlist.append(MT)
   
    return MTlist
                
#To test if it works:  
if __name__ == "__main__":
    LIST =  ParseTypes()
    for el in LIST:
        print "======="
        print el.ext
        print el.MIME
        print el.CMD
        #TODO: manager node is always None in the input XML - so it prints object type
        print el.MANAGER
    
