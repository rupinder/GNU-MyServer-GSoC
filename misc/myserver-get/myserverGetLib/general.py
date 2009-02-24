import xml.dom.minidom
import local
import config
import remote
import remoteSvn
import remoteGenericUrl
import console

class MyServerGet:
    def __init__(self, repositoryFile = config.REP_LIST_FILE, listDir = config.PLUGIN_LIST_FILES_DIR):
        self.__rep = repositoryFile
        self.__listDir = listDir
        self.__repManager = remote.RepositoryManager()
        self.__repManager.addSupportedRepository("svn",remoteSvn.RepositorySvn)
        self.__repManager.addSupportedRepository("http",remoteGenericUrl.RepositoryGenericUrl)
        self.__repManager.addSupportedRepository("ftp",remoteGenericUrl.RepositoryGenericUrl)
        self.__list = []
        self.loadRepositoryList()
    
    def loadRepositoryList(self):
        list = xml.dom.minidom.parse(self.__rep)
        repXmlList = list.getElementsByTagName("REPOSITORY")
        self.__list = [local.ListManager(repXml.firstChild.data,self.__listDir) for repXml in repXmlList]
        
    def update(self, arg):
        for list in self.__list:
            rep = self.__repManager.getRepository(list.repository)
            rep = rep(list.repository)
            rep.synchronizeListWithRepository(list)
    
  
    def __search(self,arg):
        import string
        results = []
        for list in self.__list:
            plugins = list.getPluginsList()
            for plugin in plugins:
                if string.find(plugin["name"][0]["value"],arg)!=-1:
                    results.append(plugin)
        return results
        
    def search(self,arg):
        arg = " ".join(arg)     
        if config.VERBOSE:
            results = ["- " + plugin["name"][0]["value"] + " (compatible with GNU MyServer version ["+plugin.getMyServerMinVersion() + "," + plugin.getMyServerMinVersion() + "])\n    " + plugin["description"][0]["value"] + "\n    Dependences: " + " ".join(dep["value"] + "[" + dep["min-version"] + "," + dep["max-version"] + "]"  for dep in plugin["depends"]) for plugin in self.__search(arg)]
        else: 
            results = [plugin["name"][0]["value"] + " - " + plugin["description"][0]["value"] for plugin in self.__search(arg)]
        console.writeln("\n".join(results))

            