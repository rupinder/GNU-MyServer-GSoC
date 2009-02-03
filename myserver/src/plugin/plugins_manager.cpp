/*
MyServer
Copyright (C) 2007, 2008, 2009 Free Software Foundation, Inc.
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdafx.h"
#include <include/plugin/plugins_manager.h>
#include <include/base/xml/xml_parser.h>
#include <include/base/find_data/find_data.h>
#include <include/server/server.h>

#include <string>
using namespace std;

/*!
 *Default constructor.
 */
PluginsManager::PluginsManager()
{
	
}

/*!
 *Destroy the object.
 */
PluginsManager::~PluginsManager()
{
	
}

/*!
 *Get a plugin trough  its name plugin.
 *\param name The plugin name.
 */
Plugin* PluginsManager::getPlugin(string& name)
{
  return plugins.get(name);
}
  
/*!
 *Load the plugin soptions.
 *\param server The server object to use.
 *\param languageFile The language file to use to get errors and warnings 
 *messages.
 */
int PluginsManager::loadOptions(Server *server, XmlParser* languageFile)
{
  xmlDocPtr xmlDoc;
  int ret = 0;
  XmlParser* configuration;
  
  configuration = server->getConfiguration();
  
  xmlDoc = configuration->getDoc();
  

  for(xmlNode *root = xmlDoc->children; root; root = root->next)
    if(!xmlStrcmp(root->name, (const xmlChar *)"MYSERVER"))
      for(xmlNode *node = root->children; node; node = node->next)
        {
          string namespaceName;
          string pluginName;
		  bool global = false;
		  bool enabled = false;
          
          if(!xmlStrcmp(node->name, (const xmlChar *)"PLUGIN"))
          {
            xmlAttrPtr properties = node->properties;
            
            while(properties)
            {
              if(!xmlStrcmp(properties->name, (const xmlChar *)"name"))
              {
                if(properties->children && properties->children->content)
                  pluginName.assign((char*)properties->children->content);
              }
              properties = properties->next;
            }
            
            for(xmlNode *internal = node->children; internal; internal = internal->next)  
            {
              if(!xmlStrcmp(internal->name, (const xmlChar *)"ENABLED"))
                enabled = strcmpi("NO", (const char*)internal->children->content) ? true : false;
              else if(!xmlStrcmp(internal->name, (const xmlChar *)"GLOBAL"))
                global = strcmpi("YES", (const char*)internal->children->content) ? false : true;
            }

            if(!pluginName.length())
            {
              string error;
              error.assign("Warning: invalid plugin name in PLUGIN block");
              server->logWriteln(error.c_str(), MYSERVER_LOG_MSG_ERROR);
              ret = -1;  
            }
            else
            {
                addPluginInfo(pluginName, new PluginInfo(pluginName,enabled,global));
            }

          }
        }

  return ret;
  
}


/*!
 *Preload sequence, called when all the plugins are not yet loaded.
 *\param server The server object to use.
 *\param languageFile The language file to use to retrieve warnings/errors 
 *messages.
 *\param resource The resource location to use to load plugins, in this 
 *implementation it is a directory name.
 */
int PluginsManager::preLoad(Server* server, XmlParser* languageFile, 
                                     string& resource)
{
  FindData fdir;
  FindData flib;
  string filename;
  string completeFileName;  
  int ret;

  loadOptions(server,languageFile);
  
  filename.assign(resource);

  

  ret = fdir.findfirst(filename.c_str());  
  
  if(ret == -1)
  {
    return ret;  
  }

  ret = 0;

  do
  {  
  	string dirname(filename);
  	
  	dirname.append("/");
  	dirname.append(fdir.name);
    if(fdir.name[0]=='.')
      continue;
      
    
    ret = flib.findfirst(dirname.c_str());  
  
  	if(ret == -1)
  	  continue;
  	
  	do
  	{
  	  string name(flib.name);
      if(flib.name[0]=='.')
      	continue;

          /*!
     *Do not consider file other than dynamic libraries.
     */
#ifdef WIN32
      if(!strstr(flib.name,".dll") && !strstr(flib.name,"plugin.xml"))
#endif
#ifdef NOT_WIN
      if(!strstr(flib.name,".so") && !strstr(flib.name,"plugin.xml"))
#endif    
        continue;
      completeFileName.assign(filename);
      
      if((fdir.name[0] != '/') && (fdir.name[0] != '\\')) 
        completeFileName.append("/");
      completeFileName.append(fdir.name);
      
      if((flib.name[0] != '/') && (flib.name[0] != '\\')) 
        completeFileName.append("/");
      completeFileName.append(flib.name);

	  if(strstr(flib.name,"plugin.xml"))
	  {
	  	string pname(fdir.name);
        ret |= loadInfo(pname, completeFileName);
        continue;
	  }
#ifdef WIN32
      name = name.substr(0, name.length() - 4);
#endif
#ifdef NOT_WIN
      name = name.substr(0, name.length() - 3);
#endif
      PluginInfo* pinfo = getPluginInfo(name);
    	
       if(!pinfo || pinfo->isEnabled())
         ret |= addPlugin(completeFileName, server, languageFile, pinfo && pinfo->isGlobal());
	   
     }while(!flib.findnext());
   }while(!fdir.findnext());
  fdir.findclose();
  flib.findclose();
  return ret;
}

/*!
 *Create the appropriate object to keep a plugin.
 */
Plugin* PluginsManager::createPluginObject()
{
  return new Plugin();
}

/*!
 *Loads the plugin info.
 *\param name The plugin name.
 *\param path the plugin xml descriptor path.
 */
int PluginsManager::loadInfo(string& name, string& path)
{
  PluginInfo* pinfo;
  
  pinfo = getPluginInfo(name);
  if (!pinfo)
    pinfo = new PluginInfo(name);
    
  XmlParser xml;
  
  if (xml.open(path,true))
    return 1;
  
  XmlXPathResult* xpathRes = xml.evaluateXpath("/PLUGIN");
  
  xmlNodeSetPtr nodes = xpathRes->getNodeSet();
  
  int size = (nodes) ? nodes->nodeNr : 0;
  
  if (size!=1)
    return 1;
  
  const char* mSMinVersion = (const char*)xmlGetProp(nodes->nodeTab[0],(const xmlChar*)"min-version");
  
  const char* mSMaxVersion = (const char*)xmlGetProp(nodes->nodeTab[0],(const xmlChar*)"max-version");
  
  pinfo->setMyServerMinVersion(PluginInfo::convertVersion(new string(mSMinVersion)));
  pinfo->setMyServerMaxVersion(PluginInfo::convertVersion(new string(mSMaxVersion)));
  
  delete xpathRes;
  
  xpathRes = xml.evaluateXpath("/PLUGIN/NAME/text()");
  nodes = xpathRes->getNodeSet();
  size = (nodes) ? nodes->nodeNr : 0;
  
  
  if (size!=1)
    return 1;
  const char* cname = (const char*)nodes->nodeTab[0]->content;

  
  if (strcmp(name.c_str(),cname))
    return 1;
  
  delete xpathRes;
  
  xpathRes = xml.evaluateXpath("/PLUGIN/VERSION/text()");
  nodes = xpathRes->getNodeSet();
  size = (nodes) ? nodes->nodeNr : 0;
  
  
  
  if (size!=1)
    return 1;
  const char* version = (const char*)nodes->nodeTab[0]->content;
  
  
  
  pinfo->setVersion(PluginInfo::convertVersion(new string(version)));
  
  
  delete xpathRes;
  
  xpathRes = xml.evaluateXpath("/PLUGIN/DEPENDS");
  nodes = xpathRes->getNodeSet();
  size = (nodes) ? nodes->nodeNr : 0;
  
  
  for (int i=0; i<size; i++)
  {
  	const char* depends = (const char*)nodes->nodeTab[i]->children->content;
  	
  	string nameDep(depends);
  	
  	const char* minVersion = (const char*)xmlGetProp(nodes->nodeTab[i],(const xmlChar*)"min-version");
    
    const char* maxVersion = (const char*)xmlGetProp(nodes->nodeTab[i],(const xmlChar*)"max-version");
    
    pinfo->addDependence(nameDep, PluginInfo::convertVersion(new string(minVersion)),PluginInfo::convertVersion(new string(maxVersion)));
  }

  delete xpathRes;
  
  addPluginInfo(name,pinfo);
  
  
  return 0;
}

/*!
 *Add a plugin.
 *\param file The plugin file name.
 *\param server The server object to use.
 *\param languageFile The language file to use to retrieve warnings/errors 
 *messages.
 *\param global Specify if the library should be loaded globally.
 */
int PluginsManager::addPlugin(string& file, Server* server, 
                                       XmlParser* languageFile, bool global)
{
  Plugin *plugin = createPluginObject();
  
  string name;
  const char* namePtr;

  if(plugin->preLoad(file, server, languageFile, global))
  {
    delete plugin;
    return 1;
  }
  namePtr = plugin->getName(0, 0);

  if(namePtr)
    name.assign(namePtr);
  else
  {
    delete plugin;
    return 1;
  }
    
  plugins.put(name, plugin);

  
  
  return 0;
}

/*!
 *Load the plugins.
 *\param server The server object to use.
 *\param languageFile The language file to use to get errors and warnings 
 *messages.
 *\param resource The resource to use to load plugins.
 */
int PluginsManager::load(Server *server, XmlParser* languageFile, 
                         string& resource)
{
	
  
  list<string*> toRemove;
  HashMap<string, Plugin*>::Iterator it = plugins.begin();
  while(it != plugins.end())
  {
  	string logBuf;
  	
  	string name(it.getKey());
  	
  	
  	PluginInfo* pinfo = getPluginInfo(name);
    
    
  	HashMap<string, pair<int,int>* >::Iterator depIt = pinfo->begin();
  	bool goodVersions = true;

  	string msversion(MYSERVER_VERSION);
  	int i = msversion.find("-",0);
  	if (i!=string::npos)
  	  msversion = msversion.substr(0,i); 
  	
  	int msVersion = PluginInfo::convertVersion(&msversion);
  	if (msVersion < pinfo->getMyServerMinVersion() || msVersion > pinfo->getMyServerMaxVersion())
  	{
  	  logBuf.append("myserver version not compatible --> ");
  	  goodVersions = false;
  	}
  	else
  	  for (;depIt != pinfo->end(); depIt++)
  	  {
  	  	string dname = depIt.getKey();
  	  	
  	  	PluginInfo* dep = getPluginInfo(dname);
  	  	
  	  	if (!dep)
  	  	{
  	  	  logBuf.append("missing dependence: ");
  	  	  logBuf.append(dname);
  	  	  logBuf.append(" --> ");
  	  	  logBuf.append(name);
  	  	  goodVersions = false;
  	  	  break;
  	  	}
  	  	  
  	  	if (dep->getVersion() < (*depIt)->first || dep->getVersion() > (*depIt)->second)
  	  	{
  	  	  logBuf.append("plugin version not compatible: ");
  	  	  logBuf.append(dname);
  	  	  logBuf.append(" --> ");
  	  	  logBuf.append(name);
  	  	  goodVersions = false;
  	  	  break;
  	  	}
  	  }
  	
  	
  	if (goodVersions)
  	{
  	  logBuf.assign(languageFile->getValue("MSG_LOADED"));
  	  logBuf.append(" -plugin- ");
      logBuf.append(name);
      (*it)->load(resource, server, languageFile);
  	} 
  	else
  	  toRemove.push_front(&name);
  	server->logWriteln( logBuf.c_str() );
    it++;
  }

  list<string*>::iterator tRIt = toRemove.begin();
  for (;tRIt != toRemove.end(); tRIt++)
    removePlugin(**tRIt);
  
  return 0;
  
}

/*!
 *PostLoad functions, called once all the plugins are loaded.
 *\param server The server object to use.
 *\param languageFile The language file to use to get errors and warnings 
 *messages.
 */
int PluginsManager::postLoad(Server *server, XmlParser* languageFile)
{
  HashMap<string, Plugin*>::Iterator it = plugins.begin();
  while(it != plugins.end())
  {
    (*it)->postLoad(server, languageFile);
    it++;
  }
  return 0;
}

/*!
 *Unload the plugins.
 *\param server The server object to use.
 *\param languageFile The language file to use to get errors and warnings 
 *messages.
 */
int PluginsManager::unLoad(Server *server, XmlParser* languageFile)
{
  HashMap<string, Plugin*>::Iterator it = plugins.begin();
  HashMap<string, PluginInfo*>::Iterator poit = pluginsInfos.begin();

  while(it != plugins.end())
  {
    (*it)->unLoad(languageFile);
    delete *it;
    it++;
  }

  while(poit != pluginsInfos.end())
  {
    delete *poit;
    poit++;
  }

  plugins.clear();
  pluginsInfos.clear();
  return 0;
}

/*!
 *Remove a plugin  without clean it.
 *\param name The plugin to remove.
 */
void PluginsManager::removePlugin(string& name)
{
  plugins.remove(name);
}

/*!
 *Add a plugin option structure.
 *\param plugin The plugin name.
 *\param pi The options for the plugin.
 */
int PluginsManager::addPluginInfo(string& plugin, PluginInfo* pi)
{
  PluginInfo* oldPi = pluginsInfos.put(plugin, pi);

  if(oldPi)
    delete oldPi;

  return 0;
}

/*!
 *Return a pluginOption.
 *\param plugin The plugin name.
 */
PluginInfo* PluginsManager::getPluginInfo(string& plugin)
{
  return pluginsInfos.get(plugin);
}
