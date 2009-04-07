# MyServer
# Copyright (C) 2009 The MyServer Team
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



CNFG_FILE = "myserver-get.xml"

REP_LIST_FILE = "repositories.xml"

PLUGIN_LIST_FILES_DIR = "list"

PLUGIN_LIST_PREFIX = "myserver-get-"

PLUGIN_LIST_SUFFIX = "-plugin-list.xml"

MYSERVER_PLUGIN_DIR = "../../myserver/binaries/plugins"

MYSERVER_VERSION = "0.9"

MSHEADERS = "/home/dany/srcprojects/myserver/myserver"

verbose = False

arch = ""

def loadConfig(file = CNFG_FILE):
    import xml.dom.minidom
    xmlCnfg = xml.dom.minidom.parse(file)
        
    elements = xmlCnfg.getElementsByTagName("REP_LIST_FILE")
    if len(elements)==1:
        REP_LIST_FILE = elements[0].firstChild.data
            
    elements = xmlCnfg.getElementsByTagName("PLUGIN_LIST_FILES_DIR")
    if len(elements)==1:
         PLUGIN_LIST_FILES_DIR = elements[0].firstChild.data
            
    elements = xmlCnfg.getElementsByTagName("PLUGIN_LIST_PREFIX")
    if len(elements)==1:
        PLUGIN_LIST_PREFIX = elements[0].firstChild.data
            
    elements = xmlCnfg.getElementsByTagName("PLUGIN_LIST_SUFFIX")
    if len(elements)==1:
        PLUGIN_LIST_SUFFIX = elements[0].firstChild.data
            
    elements = xmlCnfg.getElementsByTagName("MYSERVER_PLUGIN_DIR")
    if len(elements)==1:
        MYSERVER_PLUGIN_DIR = elements[0].firstChild.data
            
    elements = xmlCnfg.getElementsByTagName("MYSERVER_VERSION")
    if len(elements)==1:
        MYSERVER_VERSION = elements[0].firstChild.data
            
    elements = xmlCnfg.getElementsByTagName("MSHEADERS")
    if len(elements)==1:
        MSHEADERS = elements[0].firstChild.data