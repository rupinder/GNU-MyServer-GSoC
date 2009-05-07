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
#include <include/base/string/stringutils.h>
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
  PluginInfo* info = pluginsInfos.get(name);
  if (info)
  	return info->getPlugin();
  return NULL;
}
  
/*!
 *Load the plugin soptions.
 *\param server The server object to use.
 *\param languageFile The language file to use to get errors and warnings 
 *messages.
 */
int PluginsManager::loadOptions (Server *server, XmlParser* languageFile)
{
  int ret = 0;
  string key ("server.plugins");

  NodeTree<string>* node = server->getNodeTree (key);

  if (node == NULL)
    return 0;

  list<NodeTree<string>*> *children = node->getChildren ();

  if (children == NULL)
    return 0;

  string namespaceKey ("namespace");
  string pluginKey ("plugin");
  string globalKey ("global");
  string enabledKey ("enabled");

  for(list<NodeTree<string>*>::iterator it = children->begin ();
      it != children->end ();
      it++)
    {
      NodeTree<string>* node = *it;

      string plugin;
      string namespaceName;
		  bool global = false;
		  bool enabled = false;

      if (node->getAttr (namespaceKey))
        namespaceName.assign (*node->getAttr (namespaceKey));

      if (node->getAttr (pluginKey))
        plugin.assign (*node->getAttr (pluginKey));

      if (node->getAttr (globalKey))
        global = stringcmpi (*node->getAttr (globalKey), "YES") == 0;

      if (node->getAttr (enabledKey))
        enabled = stringcmpi (*node->getAttr (enabledKey), "YES") == 0;

      if (!plugin.length ())
        {
          string error;
          error.assign ("PLUGINS MANAGER: Warning: invalid plugin name in PLUGIN block");
          server->logWriteln (error.c_str (), MYSERVER_LOG_MSG_ERROR);
          ret = -1;  
        }
      else
        {
          addPluginInfo (plugin, new PluginInfo (plugin, enabled, global));
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

          
      if(!strstr(flib.name,"plugin.xml"))
        continue;
      completeFileName.assign(filename);
      
      if((fdir.name[0] != '/') && (fdir.name[0] != '\\')) 
        completeFileName.append("/");
      completeFileName.append(fdir.name);
      
      if((flib.name[0] != '/') && (flib.name[0] != '\\')) 
        completeFileName.append("/");
      completeFileName.append(flib.name);

	  string pname(fdir.name);
      PluginInfo* pinfo = loadInfo(server,pname, completeFileName);
      if(!pinfo)
      {
        ret|=1;
        continue;
      }
      string libname;
      libname.assign(dirname);
         
      libname.append("/");
      libname.append(pinfo->getName());
#ifdef WIN32
      libname.append(".dll");
#else
      libname.append(".so");
#endif
       if(pinfo->isEnabled())
       {
         Plugin* plugin = preLoadPlugin(libname, server, languageFile, pinfo->isGlobal());
         if (!plugin)
         {
           ret|=1;
           string logBuf;
  	       logBuf.append("PLUGINS MANAGER: Error!! Unable to enable plugin ");
  	       logBuf.append(name);
  	       server->logWriteln( logBuf.c_str() );
         }else
           pinfo->setPlugin(plugin);
       }
       addPluginInfo(pname,pinfo);
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
PluginInfo* PluginsManager::loadInfo(Server* server, string& name, string& path)
{
  PluginInfo* pinfo;
  
  pinfo = getPluginInfo(name);
  if (!pinfo)
    pinfo = new PluginInfo(name);
  else
  	if (pinfo->getVersion()!=0)
  	{
  	  string logBuf;
  	  logBuf.append("PLUGINS MANAGER: Error!!! a version of plugin ");
  	  logBuf.append(name);
  	  logBuf.append(" is already loading!");
  	  server->logWriteln( logBuf.c_str() );
  	  return NULL;
  	}  
  XmlParser xml;
  
  if (xml.open(path,true))
    return NULL;
  
  XmlXPathResult* xpathRes = xml.evaluateXpath("/PLUGIN");
  
  xmlNodeSetPtr nodes = xpathRes->getNodeSet();
  
  int size = (nodes) ? nodes->nodeNr : 0;
  
  if (size!=1)
    return NULL;
  
  const char* mSMinVersion = (const char*)xmlGetProp(nodes->nodeTab[0],(const xmlChar*)"min-version");
  
  const char* mSMaxVersion = (const char*)xmlGetProp(nodes->nodeTab[0],(const xmlChar*)"max-version");
  
  pinfo->setMyServerMinVersion(PluginInfo::convertVersion(new string(mSMinVersion)));
  pinfo->setMyServerMaxVersion(PluginInfo::convertVersion(new string(mSMaxVersion)));
  
  delete xpathRes;
  
  xpathRes = xml.evaluateXpath("/PLUGIN/NAME/text()");
  nodes = xpathRes->getNodeSet();
  size = (nodes) ? nodes->nodeNr : 0;
  
  
  if (size!=1)
    return NULL;
  const char* cname = (const char*)nodes->nodeTab[0]->content;

  
  if (strcmp(name.c_str(),cname))
    return NULL;
  
  delete xpathRes;
  
  xpathRes = xml.evaluateXpath("/PLUGIN/VERSION/text()");
  nodes = xpathRes->getNodeSet();
  size = (nodes) ? nodes->nodeNr : 0;
  
  
  
  if (size!=1)
    return NULL;
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

  return pinfo;
}

/*!
 *Add a plugin.
 *\param file The plugin file name.
 *\param server The server object to use.
 *\param languageFile The language file to use to retrieve warnings/errors 
 *messages.
 *\param global Specify if the library should be loaded globally.
 */
Plugin* PluginsManager::preLoadPlugin(string& file, Server* server, 
                                       XmlParser* languageFile, bool global)
{
  Plugin *plugin = createPluginObject();
  
  string name;
  const char* namePtr;

  if(plugin->preLoad(file, server, languageFile, global))
  {
    delete plugin;
    return NULL;
  }
  namePtr = plugin->getName(0, 0);

  if(namePtr)
    name.assign(namePtr);
  else
  {
    delete plugin;
    return NULL;
  }
 
  return plugin;
}

void PluginsManager::recursiveDependencesFallDown(Server* server, string name, HashMap<string,bool> remove,  HashMap<string,list<string>*> dependsOn)
{
  remove.put(name,true);
  list<string>* dependsList = dependsOn.get(name);
  if (!dependsList || dependsList->empty())
    return;
  
  list<string>::iterator lit = dependsList->begin();

  for (;lit!=dependsList->end();lit++)
  {
  	
  	string logBuf;
  	logBuf.append("PLUGINS MANAGER: missing dependence: ");
  	logBuf.append(*lit);
  	logBuf.append(" --> ");
  	logBuf.append(name);
  	recursiveDependencesFallDown(server,*lit,remove,dependsOn);
  	server->logWriteln( logBuf.c_str() );
  }
  	
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
  HashMap<string,list<string>*> dependsOn;
  HashMap<string, PluginInfo*>::Iterator it = pluginsInfos.begin();
  HashMap<string,bool> remove;
  while(it != pluginsInfos.end())
  {
  	
  	
  	string name(it.getKey());
  	
  	
  	PluginInfo* pinfo = *it;
    
    
  	HashMap<string, pair<int,int>* >::Iterator depIt = pinfo->begin();

  	string msversion(MYSERVER_VERSION);
  	int i = msversion.find("-",0);
  	if (i!=string::npos)
  	  msversion = msversion.substr(0,i); 
  	
  	int msVersion = PluginInfo::convertVersion(&msversion);
  	if (msVersion < pinfo->getMyServerMinVersion() || msVersion > pinfo->getMyServerMaxVersion())
  	{
	  string logBuf;
  	  logBuf.append("PLUGINS MANAGER: ");
  	  logBuf.append(name);
  	  logBuf.append(" not compatible with this myserver version! ");
  	  server->logWriteln( logBuf.c_str() );
  	  toRemove.push_front(&name);
  	}
  	else
  	  remove.put(name,false);
  	  for (;depIt != pinfo->end(); depIt++)
  	  {
  	  	string dname = depIt.getKey();
  	  	
  	  	list<string>* deps = dependsOn.get(dname);
  	  	if (!deps)
  	  	{
  	  	  deps = new list<string>();
  	  	  dependsOn.put(dname,deps);
  	  	}
  	  	
  	  	deps->push_front(name);
  	  }
  	  
  	
    it++;
  }

  list<string*>::iterator tRIt = toRemove.begin();
  for (;tRIt != toRemove.end(); tRIt++)
    removePlugin(**tRIt);
  toRemove.clear();
  
  
  
  HashMap<string,list<string>*>::Iterator dIt = dependsOn.begin();
  for(;dIt!=dependsOn.end();dIt++)
  {
  	string logBuf;
  	string dname = dIt.getKey();
  	
  	PluginInfo* pinfo = getPluginInfo(dname);
  	
  	if (!pinfo || pinfo->getVersion()==0)
  	  	remove.put(dname,true);
  	  	
  	list<string>* dependsList = (*dIt);
  	if (!dependsList)
  	  continue;
  	if(dependsList->empty())
  	  continue;
  	
  	bool rem = remove.get(dname);
  	if (rem)
  	{
  	  recursiveDependencesFallDown(server,dname,remove,dependsOn);
  	  continue;
  	}
  	
  	HashMap<string, pair<int,int>* >::Iterator lit = pinfo->begin();
  	for (;lit!=pinfo->end();lit++)
  	{
  		string depN = lit.getKey();
  		PluginInfo* dep = getPluginInfo(depN);
  		if (!dep || remove.get(depN))
  	  	{
  	  	  logBuf.append("PLUGINS MANAGER: missing dependence: ");
  	  	  logBuf.append(dname);
  	  	  logBuf.append(" --> ");
  	  	  logBuf.append(depN);
  	  	  recursiveDependencesFallDown(server,dname,remove,dependsOn);
  	  	  server->logWriteln( logBuf.c_str() );
  	  	  break;
  	  	}
  	  	

  	  	pair<int,int>* pdep = *lit;
  	  	if (dep->getVersion() < pdep->first || dep->getVersion() > pdep->second)
  	  	{
  	  	  logBuf.append("PLUGINS MANAGER: plugin version not compatible: ");
  	  	  logBuf.append(dname);
  	  	  logBuf.append(" --> ");
  	  	  logBuf.append(depN);
  	  	  recursiveDependencesFallDown(server,dname,remove,dependsOn);
  	  	  server->logWriteln( logBuf.c_str() );
  	  	  break;
  	  	}
  	}
  }
  
  
  HashMap<string,bool>::Iterator rIt = remove.begin();
  for(;rIt!=remove.end();rIt++)
  {
  	string name(rIt.getKey());
  	if (*rIt)
  	  removePlugin(name);
  }
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
  HashMap<string, PluginInfo*>::Iterator it = pluginsInfos.begin();
  while(it != pluginsInfos.end())
  {
    Plugin* plugin = (*it)->getPlugin();
    if (plugin)
    {
      plugin->postLoad(server, languageFile);
      string logBuf;
      logBuf.append("PLUGINS MANAGER: plugin ");
  	  logBuf.append((*it)->getName());
  	  logBuf.append(" loaded! ");
  	  server->logWriteln( logBuf.c_str() );
    }
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

  HashMap<string, PluginInfo*>::Iterator poit = pluginsInfos.begin();

  while(poit != pluginsInfos.end())
  {
    delete *poit;
    poit++;
  }

  pluginsInfos.clear();
  return 0;
}

/*!
 *Remove a plugin  without clean it.
 *\param name The plugin to remove.
 */
void PluginsManager::removePlugin(string& name)
{
  PluginInfo* info = pluginsInfos.remove(name);
  
  delete info;
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
