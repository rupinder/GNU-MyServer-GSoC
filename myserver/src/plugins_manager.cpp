/*
MyServer
Copyright (C) 2007, 2008 Free Software Foundation, Inc.
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

#include "../stdafx.h"
#include "../include/plugins_manager.h"
#include "../include/xml_parser.h"
#include "../include/server.h"

#include <string>
using namespace std;

/*!
 *Get a plugin trough its namespace and its name.
 *\param namespacename The namespace name to use.
 *\param plugin The plugin name.
 */
Plugin* PluginsManager::getPlugin(string& namespacename, string& plugin)
{
  PluginsNamespace* pn = namespaces.get((char*)namespacename.c_str());

  if(pn)
    return pn->getPlugin(plugin);
  return 0;
}
/*!
 *Get a plugin trough its namespace and its name namespace-plugin.
 *\param fullname The plugin complete name.
 */
Plugin* PluginsManager::getPlugin(string& fullname)
{
  size_t sep = fullname.find('-', 0);
  if(sep != string::npos)
  {
    string namespacename(fullname.substr(0, sep - 1));
    string plugin(fullname.substr(sep + 1, fullname.length()));
    
    return getPlugin(namespacename, plugin);
  }
  return 0;
}
  
/*!
 *Preload the plugins.
 *\param server The server object to use.
 *\param languageFile The language file to use to get errors and warnings 
 *messages.
 *\param resource The resource to use to load plugins.
 */
int PluginsManager::preLoad(Server *server, XmlParser* languageFile, 
                            string& resource)
{
  xmlDocPtr xmlDoc;
  int ret = 0;
  XmlParser* configuration;
  HashMap<char*, PluginsNamespace*>::Iterator it = namespaces.begin();

  configuration = server->getConfiguration();
  
  xmlDoc = configuration->getDoc();
  

  for(xmlNode *root = xmlDoc->children; root; root = root->next)
    if(!xmlStrcmp(root->name, (const xmlChar *)"MYSERVER"))
      for(xmlNode *node = root->children; node; node = node->next)
        {
          string namespaceName;
          string pluginName;
          PluginsNamespace::PluginOption po;
          
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
        
              if(!xmlStrcmp(properties->name, (const xmlChar *)"namespace"))
              {
                if(properties->children && properties->children->content)
                  namespaceName.assign((char*)properties->children->content);
              }
              
              properties = properties->next;
            }
            
            for(xmlNode *internal = node->children; internal; internal = internal->next)  
            {
              if(!xmlStrcmp(internal->name, (const xmlChar *)"ENABLED"))
                po.enabled = strcmpi("NO", (const char*)internal->children->content) ? true : false;
              else if(!xmlStrcmp(internal->name, (const xmlChar *)"GLOBAL"))
                po.global = strcmpi("YES", (const char*)internal->children->content) ? false : true;
            }

            if(!namespaceName.length() || !pluginName.length())
            {
              string error;
              error.assign("Warning: invalid namespace or plugin name in PLUGIN block");
              server->logLockAccess();
              server->logPreparePrintError();
              server->logWriteln(error.c_str());     
              server->logEndPrintError();
              server->logUnlockAccess();
            }
            else
            {
              PluginsNamespace* ns = getNamespace(namespaceName);
              if(!ns)
              {
                string error;
                error.assign("Warning: invalid namespace name");
                server->logLockAccess();
                server->logPreparePrintError();
                server->logWriteln(error.c_str());     
                server->logEndPrintError();
                server->logUnlockAccess();
              }
              else
                ns->addPluginOption(pluginName, po);

            }

          }
        }

  while(it != namespaces.end())
  {
    ret |= (*it)->preLoad(server, languageFile, resource);
    it++;
  }
  return ret;
  
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
  int ret = 0;
  HashMap<char*, PluginsNamespace*>::Iterator it = namespaces.begin();

  while(it != namespaces.end())
  {
    ret |= (*it)->load(server, languageFile, resource);
    it++;
  }
  return ret;
  
}

/*!
 *PostLoad functions, called once all the plugins are loaded.
 *\param server The server object to use.
 *\param languageFile The language file to use to get errors and warnings 
 *messages.
 */
int PluginsManager::postLoad(Server *server, XmlParser* languageFile)
{
  int ret = 0;
  HashMap<char*, PluginsNamespace*>::Iterator it = namespaces.begin();

  while(it != namespaces.end())
  {
    ret |= (*it)->postLoad(server, languageFile);
    it++;
  }

  return ret;
}

/*!
 *Unload the plugins.
 *\param server The server object to use.
 *\param languageFile The language file to use to get errors and warnings 
 *messages.
 */
int PluginsManager::unLoad(Server *server, XmlParser* languageFile)
{
  int ret = 0;

  HashMap<char*, PluginsNamespace*>::Iterator it = namespaces.begin();

  while(it != namespaces.end())
  {
    ret |= (*it)->unLoad(languageFile);
    it++;
  }

  namespaces.clear();
  return ret;
}

/*!
 *Add a new namespace to the plugins system.
 *\param newnamespace The namespace to add.
 */
void PluginsManager::addNamespace(PluginsNamespace* newnamespace)
{
  removeNamespace(newnamespace->getName());
  namespaces.put((char*)newnamespace->getName().c_str(), newnamespace);
}

/*!
 *Get a namespace by its name.
 *\param name The namespace name.
 */
PluginsNamespace* PluginsManager::getNamespace(string &name)
{
  return namespaces.get((char*)name.c_str());
}

/*!
 *Remove a namespace by its name.
 *\param name The namespace name.
 */
PluginsNamespace* PluginsManager::removeNamespace(string& name)
{
  return namespaces.remove((char*)name.c_str());
}
