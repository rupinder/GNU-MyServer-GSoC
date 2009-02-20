import xml.dom.minidom
import local
import config
import remote
import remoteSvn

class MyServerGet:
    def __init__(self, repositoryFile = config.REP_LIST_FILE, listDir = config.PLUGIN_LIST_FILES_DIR):
        self.__rep = repositoryFile
        self.__listDir = listDir
        self.__repManager = remote.RepositoryManager()
        self.__repManager.addSupportedRepository("svn",remoteSvn.RepositorySvn)
        self.__list = []
    
    def loadRepositoryList(self):
        list = xml.dom.minidom.parse(self.__rep)
        repXmlList = list.getElementsByTagName("REPOSITORY")
        self.__list = [local.ListManager(repXml.firstChild.data,self.__listDir) for repXml in repXmlList]
        
    def update(self, arg):
        self.loadRepositoryList()
        for list in self.__list:
            rep = self.__repManager.getRepository(list.repository)
            rep = rep(list.repository)
            rep.synchronizeListWithRepository(list)