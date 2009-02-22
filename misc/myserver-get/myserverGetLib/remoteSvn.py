import pysvn
import local
import error
import xml.dom.minidom
from remote import Repository
import console

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
        console.write("update: "+ list.repository + '\n')
        console.write("loading ")
        localRevision = int(list.getRevision())
        info = self.client.info2(self.url, recurse=False)
        console.write(". ")
        remoteRevision = int(info[0][1]["rev"].number)
        if localRevision == remoteRevision:
            console.write("local list already updated.\n")
        if localRevision > remoteRevision:
            raise error.FatalError("Local plugins list corrupted!!")
        
        if localRevision < remoteRevision:
            list.resetToEmptyListFile(`remoteRevision`)
            console.write(". ")
            plugins = self.__getPluginsList()
            console.write(". ")
            for plugin in plugins:
                url = self.url + "/src/" + plugin + "/plugin.xml"
                pluginXml =  xml.dom.minidom.parseString(self.client.cat(url))
                console.write(". ")
                element = pluginXml.getElementsByTagName("PLUGIN")
                pluginInfo = list.addPluginWithXml(element[0])
            list.synchronizeListWithFileSystem()
            console.write("DONE.\n")
