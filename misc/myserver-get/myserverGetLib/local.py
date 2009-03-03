import xml.dom.minidom
import error
import config
import string

class PluginInfoElement:
    def __init__(self,name,element, document):
        self.name = name
        self.element = element
        self.document = document
    
    def __getitem__(self, key):
        key = string.lower(key)
        if key == "value":
            return string.replace(self.element.firstChild.data,'\n','')
        att = self.element.attributes[key]
        if att!=None:
            return att.value
    
    def __setitem__(self, key, item):
        key = string.lower(key)
        if key == "value":
            self.document.createTextNode(item)
            oldChild = self.__element.firstChild
            self.element.replaceChild(newChild,oldChild)
        self.element.setAttribute(key,item)


class PluginInfo:
    def __init__(self,xmlElement, manager):
        self.__xmlElement = xmlElement
        self.__manager = manager
    
    def __getitem__(self, key):
        key = string.upper(key)
        elementList = self.__xmlElement.getElementsByTagName(key)
        
        return [PluginInfoElement(key, element, self.__manager) for element in elementList]

    def __setitem__(self, key, item):
        key = string.upper(key)
        elementList = self.__xmlElement.getElementsByTagName(key) 
        for element in elementList:
            self.__xmlElement.removeChild(element)
        
        for element in item:
            self.__xmlElement.appendChild(element.element)
            element.document = self.__manager.listDOM
    
    def getMyServerMinVersion(self):
        return self.__xmlElement.getAttribute("min-version")
    
    def getMyServerMaxVersion(self):
        return self.__xmlElement.getAttribute("max-version")
    
    def finalize(self):
        self.__manager.synchronizeListWithFileSystem()


class ListManager:   
    def __init__ (self,repository, listFilePath, listFile = None):
        self.repository = repository
        
        if listFile != None:
            self.FileName = listFilePath + "/" + listFile;
        else:
            self.FileName = listFilePath + "/" + config.PLUGIN_LIST_PREFIX + "-".join(self.repository.split("/")) + config.PLUGIN_LIST_SUFFIX;
            
        try:
            self.listDOM = xml.dom.minidom.parse(self.FileName)
        except Exception:
            self.resetToEmptyListFile("-1")
            self.synchronizeListWithFileSystem()
                
            


    def resetToEmptyListFile(self,revision):
        self.listDOM = xml.dom.minidom.Document()
        plugins = self.listDOM.createElement("PLUGINS")
        plugins.setAttribute("repository",self.repository)
        plugins.setAttribute("revision",revision)
        self.listDOM.appendChild(plugins)
    
    
    def synchronizeListWithFileSystem (self):
        file_object = open(self.FileName, "w")
        self.listDOM.writexml(file_object,"","", "\n")
        file_object.close()

    
    def getPlugin(self,name):
        plugins = self.listDOM.getElementsByTagName("plugin")
        
        plugin = [elem for elem in plugins if elem.attributes["name"].value == name]
        
        if len(plugin)==1:
            pluginDOM = plugin[0]
            plugin = PluginInfo(pluginDOM,self)
            return plugin
        
        raise error.Error("Plugin "+ name + "doesn't exists! in " + self.repository)
        
    
    def addPlugin(self,name, minVersion, maxVersion):
        plugin = self.listDOM.createElement("PLUGIN")
        
        name = self.listDOM.createElement("NAME")
        name.appendChild(self.listDOM.createTextNode(name))
        
        plugin.appendChild(name)
        plugin.setAttribute("min-version",`minVersion`)
        plugin.setAttribute("max-version",`maxVersion`)
        
        plugins = self.listDOM.getElementsByTagName("PLUGINS")
        if plugins!=None:
            plugins[0].appendChild(plugin)
            pluginInfo = PluginInfo(plugin,self)
            return pluginInfo
        
        raise error.FatalError(self.FileName + "corrupted!")
        
        
    def addPluginWithXml(self,pluginXml):
        plugins = self.listDOM.getElementsByTagName("PLUGINS")
        if plugins!=None:
            plugins[0].appendChild(pluginXml)
            pluginInfo = PluginInfo(pluginXml,self)
            return pluginInfo
        
        raise error.FatalError(self.FileName + "corrupted!")
        
        
    def getPluginsList(self):
        plugins = self.listDOM.getElementsByTagName("PLUGIN")
        
        return [PluginInfo(plugin,self) for plugin in plugins]
    

    def getRevision(self):
        pluginsList = self.listDOM.getElementsByTagName("PLUGINS")
        plugins = pluginsList[0]
        return plugins.getAttribute("revision")

class DatabaseManager:
    def __init__ (self,path = config.MYSERVER_PLUGIN_DIR):
        import os
        self.pluginsDir = path
        self.plugins = [f for f in os.listdir(self.pluginsDir) if not os.path.isfile(os.path.join(self.pluginsDir, f))]
        
    def isPluginInstalled(self,plugin):
        return plugin["name"][0]["value"] in self.plugins   
    
            

if __name__ == "__main__":
    manager = ListManager("localhost.it",".","localhost.it-plugins.list")
    plugin = manager.getPlugin("prova")
    print plugin["description"] 
    plugin.finalize()