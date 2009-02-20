import pysvn
import local
import error
import xml.dom.minidom
from remote import Repository
import sys

class RepositorySvn(Repository):
    def __init__ (self,url):
        Repository.__init__(self,url)
        self.client = pysvn.Client()
    
    def __getPluginsList(self):
        plugins = self.client.list(self.url + "/src",recurse=False)
        import string
        plugins = [string.split(elem[0]["repos_path"],"/")[-1] for elem in plugins]
        return plugins[1:]
    
    def synchronizeListWithRepository(self, list):
        sys.stdout.write("update: "+ list.repository + '\n')
        sys.stdout.write("loading ")
        sys.stdout.flush()
        localRevision = int(list.getRevision())
        info = self.client.info2(self.url, recurse=False)
        sys.stdout.write(". ")
        sys.stdout.flush()
        remoteRevision = int(info[0][1]["rev"].number)
        if localRevision == remoteRevision:
            sys.stdout.write("local list already updated.\n")
            sys.stdout.flush()
        if localRevision > remoteRevision:
            raise error.FatalError("Local plugins list corrupted!!")
        
        if localRevision < remoteRevision:
            list.resetToEmptyListFile(`remoteRevision`)
            sys.stdout.write(". ")
            sys.stdout.flush()
            plugins = self.__getPluginsList()
            sys.stdout.write(". ")
            sys.stdout.flush()
            for plugin in plugins:
                url = self.url + "/src/" + plugin + "/plugin.xml"
                pluginXml =  xml.dom.minidom.parseString(self.client.cat(url))
                sys.stdout.write(". ")
                sys.stdout.flush()
                element = pluginXml.getElementsByTagName("PLUGIN")
                pluginInfo = list.addPluginWithXml(element[0])
            list.synchronizeListWithFileSystem()
            sys.stdout.write("DONE.\n")
            sys.stdout.flush()
