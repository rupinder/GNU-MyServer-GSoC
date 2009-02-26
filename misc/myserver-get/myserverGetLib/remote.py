import string

class Repository:
    def __init__(self,url):
        self.url = url
        

    def synchronizeListWithRepository(self, list):
        pass
    
    def getPluginBinary(self,list,plugin):
        pass
    
class RepositoryManager:
    def __init__(self):
        self.__reps = {}
    
    def addSupportedRepository(self, protocol, repository):
        self.__reps[protocol] = repository
    
    def getRepository(self, url):
        url = string.lower(url)
        if string.find(url,"svn")==0:
            return self.__reps["svn"]
        elif string.find(url,"http")==0:
            return self.__reps["http"]
        elif string.find(url,"ftp")==0:
            return self.__reps["ftp"]
        return None
            
if __name__ == "__main__":
    list = local.ListManager("localhost.it",".")
    rep = Repository("svn://svn.savannah.gnu.org/myserver/trunk/plugins")
    rep.synchronizeListWithRepository(list)