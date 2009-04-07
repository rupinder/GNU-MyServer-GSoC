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

import local
import error
import xml.dom.minidom
from remote import Repository
import console
import urllib
import config

class RepositoryGenericUrl(Repository):
    def __init__ (self,url):
        Repository.__init__(self,url)
        self.__pluginsXml = None

    def __getPluginsList(self):
        plugins = urllib.urlopen(self.url + "/list.xml")
        self.__pluginsXml = xml.dom.minidom.parse(plugins)
        
    
    def synchronizeListWithRepository(self, list):
        Repository.synchronizeListWithRepository(self,list)
        console.write("update: "+ list.repository + '\n')
        console.write("loading ")
        localRevision = int(list.getRevision())
        if self.__pluginsXml == None:
            self.__getPluginsList()
        pluginsElement = self.__pluginsXml.getElementsByTagName("PLUGINS")
        remoteRevision = int(pluginsElement[0].getAttribute("revision"))
        console.write(". ")
        if localRevision == remoteRevision:
            console.write("local list already updated.\n")
        if localRevision > remoteRevision:
            raise error.FatalError("Local plugins list corrupted!!")
        
        if localRevision < remoteRevision:
            list.resetToEmptyListFile(`remoteRevision`)
            console.write(". ")
            
            pluginElements = self.__pluginsXml.getElementsByTagName("PLUGIN")
            plugins = [plugin.firstChild.data for plugin in pluginElements]

            console.write(". ")
            for plugin in plugins:
                url = self.url + "/src/" + plugin + "/plugin.xml"
                pluginXml =  xml.dom.minidom.parse(urllib.urlopen(url))
                console.write(". ")
                element = pluginXml.getElementsByTagName("PLUGIN")
                pluginInfo = list.addPluginWithXml(element[0])
            list.synchronizeListWithFileSystem()
            console.write("DONE.\n")
    
    def __getRemoteFile(self, url,filename):
        import os
        (filepath, file) = os.path.split(filename)
        console.writeln("Downloading %s..\n" % (file))
        try:
            urllib.urlretrieve(url, filename)
        except Exception:
            console.writeln("Error while retriving remote file!")
            return False
        return True
    
    def getPluginBinary(self,list,plugin):
        Repository.getPluginBinary(self,list,plugin)
        url ="%s/pub/%s-%s-%s.tar.gz" % (list.repository,plugin["name"][0]["value"],plugin["version"][0]["value"],config.arch)
        filename = config.MYSERVER_PLUGIN_DIR + "/%s-%s-%s.tar.gz" % (plugin["name"][0]["value"],plugin["version"][0]["value"],config.arch)
        return self.__getRemoteFile(url, filename)
    
    def getPluginSource(self,list,plugin, dir):
        Repository.getPluginSource(self,list,plugin,dir)
        url ="%s/src/%s-%s-src.tar.gz" % (list.repository,plugin["name"][0]["value"],plugin["version"][0]["value"])
        filename = dir + "/%s-%s-src.tar.gz" % (plugin["name"][0]["value"],plugin["version"][0]["value"])
        return self.__getRemoteFile(url, filename)
        