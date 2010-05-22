/*
  MyServer
  Copyright (C) 2007, 2008, 2009, 2010 Free Software Foundation, Inc.
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

#include "myserver.h"
#include <include/plugin/plugins_manager.h>
#include <include/base/xml/xml_parser.h>
#include <include/base/read_directory/read_directory.h>
#include <include/server/server.h>
#include <include/base/string/stringutils.h>
#include <list>
#include <string>
#include <memory>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>


using namespace std;

/*!
  Default constructor.
 */
PluginsManager::PluginsManager () { }

/*!
  Destroy the object.
 */
PluginsManager::~PluginsManager () { }

/*!
  Get a plugin trough  its name plugin.
  \param name The plugin name.
 */
Plugin*
PluginsManager::getPlugin (string &name)
{
  PluginInfo* info = pluginsInfos.get (name);
  if (info)
    return info->getPlugin ();
  return NULL;
}

/*!
  Load the plugin options.
  \param server The server object to use.
 */
int
PluginsManager::loadOptions (Server *server)
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

  for (list<NodeTree<string>*>::iterator it = children->begin ();
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

      if (plugin.length ())
        addPluginInfo (plugin, new PluginInfo (plugin, enabled, global));
      else
        {
          server->log (MYSERVER_LOG_MSG_WARNING,
                              _("Invalid plugin name in PLUGIN block."));
          ret = -1;
        }
    }

  return ret;
}

/*!
  Preload sequence, called when all the plugins are not yet loaded.
  \param server The server object to use.
  implementation it is a directory name.
 */
int
PluginsManager::preLoad (Server* server, string &resource)
{
  ReadDirectory fdir;
  ReadDirectory flib;
  string filename;
  string completeFileName;
  int ret;
  HashMap<string, bool> alreadyCkeched;

  loadOptions (server);
  filename.assign (resource);

  ret = fdir.findfirst (filename.c_str ());
  if (ret == -1)
    {
      server->log (MYSERVER_LOG_MSG_ERROR,
                   _("Invalid plugins source"));
      return ret;
    }

  ret = 0;
  do
    {
      string dirname (filename);

      dirname.append ("/");
      dirname.append (fdir.name);
      if (fdir.name[0] == '.' || !FilesUtility::isDirectory (dirname.c_str ()))
        continue;

      try
        {
          ret = flib.findfirst (dirname.c_str ());
        }
      catch (...)
        {
          continue;
        }

      do
        {
          string name (flib.name);
          if (flib.name[0] == '.')
            continue;

          if (!strstr (flib.name.c_str (), "plugin.xml"))
            continue;
          completeFileName.assign (filename);

          if ((fdir.name[0] != '/') && (fdir.name[0] != '\\'))
            completeFileName.append ("/");
          completeFileName.append (fdir.name);

          if ((flib.name[0] != '/') && (flib.name[0] != '\\'))
            completeFileName.append ("/");
          completeFileName.append (flib.name);

          string pname (fdir.name);

          if (alreadyCkeched.get (pname))
            continue;

          PluginInfo* pinfo = loadInfo (server, pname, completeFileName);
          alreadyCkeched.put (pname,true);
          if (!pinfo)
            {
              ret |= 1;
              continue;
            }
          string libname;
          libname.assign (dirname);

          libname.append ("/");
          libname.append (pinfo->getName ());
#ifdef WIN32
          libname.append (".dll");
#else
          libname.append (".so");
#endif

          ret |= loadFile (server, pname, libname, pinfo);
        }
      while (! flib.findnext ());
    }
  while (! fdir.findnext ());

  fdir.findclose ();
  flib.findclose ();
  return ret;
}

/*!
  Load the specified plugin file.
  \param server The server object to use.
  \param name Plugin name.
  \param file Plugin file.
  \param pinfo Plugin information.
 */
int
PluginsManager::loadFile (Server *server, string &name, string &file,
                          PluginInfo *pinfo)
{
  int ret = 0;
  if (pinfo->isEnabled ())
    {
      Plugin* plugin = preLoadPlugin (file, server,
                                      pinfo->isGlobal ());
      if (! plugin)
        ret = 1;
      else
        {
          pinfo->setPlugin (plugin);
          addPluginInfo (name, pinfo);
        }
    }

  return ret;
}

/*!
  Create the appropriate object to keep a plugin.
 */
Plugin*
PluginsManager::createPluginObject ()
{
  return new Plugin ();
}

/*!
  Quick load a plugin, doing all phases {pre,,post}Load.
  \param server The server object to use.
  \param plugins comma separed list of plugins to load as:
  "name1:pluginfile1.so,name2:pluginfile2.so"
*/
int
PluginsManager::quickLoad (Server *server, const string &plugins)
{
  size_t start = 0;
  int ret = 0;
  while (1)
    {
      size_t commaPos = plugins.find (",", start);

      size_t sep = plugins.find (":", start);
      if (sep > commaPos || sep == string::npos)
        {
          server->log (MYSERVER_LOG_MSG_ERROR,
                       _("Invalid plugins data specified"));
          return -1;
        }

      string name = plugins.substr (start, sep - start);
      string file = plugins.substr (sep + 1, commaPos == string::npos
                                    ? string::npos
                                    : commaPos - sep - 1);

      string dir;
      string filename;
      FilesUtility::splitPath (file, dir, filename);

      string xmlDescriptor = dir + "/plugin.xml";
      PluginInfo *pinfo = loadInfo (server, name, xmlDescriptor);

      if (! pinfo)
        {
          server->log (MYSERVER_LOG_MSG_ERROR,
                       _("Cannot find the plugin data, please check "
                         "that the specified name and path are correct"));
          return -1;
        }

      auto_ptr<PluginInfo> pinfoAutoPtr (pinfo);

      ret |= loadFile (server, name, file, pinfo);

      pinfoAutoPtr.release ();

      Plugin *plugin = pinfo->getPlugin ();
      if (! plugin)
        {
          server->log (MYSERVER_LOG_MSG_ERROR,
                       _("Cannot load plugin %s"), file.c_str ());
          return -1;
        }

      ret |= plugin->load (server);
      ret |= plugin->postLoad (server);

      if (commaPos == string::npos)
        break;

      start = commaPos + 1;
    }

  return ret;
}

/*!
  Loads the plugin info.
  \param name The plugin name.
  \param path the plugin xml descriptor path.
 */
PluginInfo*
PluginsManager::loadInfo (Server* server, string &name, string &path)
{
  PluginInfo* pinfo = getPluginInfo (name);
  auto_ptr<PluginInfo> pinfoAutoPtr (NULL);
  if (!pinfo)
    pinfoAutoPtr.reset (pinfo = new PluginInfo (name));
  else if (pinfo->getVersion () != 0)
    return NULL;

  XmlParser xml;

  if (xml.open (path, true))
    {
      server->log (MYSERVER_LOG_MSG_ERROR,
                          _("Error loading plugin `%s'"), name.c_str ());
      return NULL;
    }

  auto_ptr<XmlXPathResult> xpathResPlugin = auto_ptr<XmlXPathResult>
    (xml.evaluateXpath ("/PLUGIN"));
  xmlNodeSetPtr nodes = xpathResPlugin->getNodeSet ();

  int size = (nodes) ? nodes->nodeNr : 0;
  if (size != 1)
    {
      server->log (MYSERVER_LOG_MSG_ERROR,
                          _("Error loading plugin `%s': invalid plugin.xml"),
                          name.c_str ());
      return NULL;
    }

  if (xmlHasProp (nodes->nodeTab[0], (const xmlChar*) "min-version"))
    {
      xmlChar *minVersion = xmlGetProp (nodes->nodeTab[0],
                                        (const xmlChar*) "min-version");

      string sMinVer ((char*) minVersion);
      pinfo->setMyServerMinVersion (PluginInfo::convertVersion (sMinVer));
    }
  else
    {
      server->log (MYSERVER_LOG_MSG_ERROR,
                          _("Error loading plugin `%s': invalid plugin.xml"),
                          name.c_str ());
      return NULL;
    }

  if (xmlHasProp (nodes->nodeTab[0], (const xmlChar*) "max-version"))
    {
      xmlChar* maxVersion = xmlGetProp (nodes->nodeTab[0],
                                        (const xmlChar*) "max-version");

      string sMaxVer ((char*)maxVersion);
      pinfo->setMyServerMaxVersion (PluginInfo::convertVersion (sMaxVer));
    }
  else
    {
      server->log (MYSERVER_LOG_MSG_ERROR,
                          _("Error loading plugin `%s': invalid plugin.xml"),
                          name.c_str ());
      return NULL;
    }

  auto_ptr<XmlXPathResult> xpathResPluginName = auto_ptr<XmlXPathResult>
    (xml.evaluateXpath ("/PLUGIN/NAME/text ()"));
  nodes = xpathResPluginName->getNodeSet ();
  size = (nodes) ? nodes->nodeNr : 0;

  if (size != 1)
    {
      server->log (MYSERVER_LOG_MSG_ERROR,
                          _("Error loading plugin `%s': invalid plugin.xml"),
                          name.c_str ());
      return NULL;
    }

  const char* cname = (const char*) nodes->nodeTab[0]->content;
  if (strcmp (name.c_str (), cname))
    return NULL;

  auto_ptr<XmlXPathResult> xpathResPluginVersion = auto_ptr<XmlXPathResult>
    (xml.evaluateXpath ("/PLUGIN/VERSION/text ()"));
  nodes = xpathResPluginVersion->getNodeSet ();
  size = (nodes) ? nodes->nodeNr : 0;

  if (size != 1)
    {
      server->log (MYSERVER_LOG_MSG_ERROR,
                          _("Error loading plugin `%s': invalid plugin.xml"),
                          name.c_str ());
      return NULL;
    }

  string verStr ((char*) nodes->nodeTab[0]->content);
  int version = PluginInfo::convertVersion (verStr);

  if (version != -1)
    pinfo->setVersion (version);
  else
    {
      server->log (MYSERVER_LOG_MSG_ERROR,
                          _("Error loading plugin `%s': invalid plugin.xml"),
                          name.c_str ());
      return NULL;
    }

  auto_ptr<XmlXPathResult> xpathResDeps = auto_ptr<XmlXPathResult>
    (xml.evaluateXpath ("/PLUGIN/DEPENDS"));
  nodes = xpathResDeps->getNodeSet ();
  size = (nodes) ? nodes->nodeNr : 0;

  for (int i = 0; i < size; i++)
    {
      const char* depends = (const char*) nodes->nodeTab[i]->children->content;

      string nameDep (depends);

      if (!xmlHasProp (nodes->nodeTab[i], (const xmlChar*) "min-version") ||
          !xmlHasProp (nodes->nodeTab[i], (const xmlChar*) "max-version"))
        {
          server->log (MYSERVER_LOG_MSG_ERROR,
                              _("Error loading plugin `%s': invalid plugin.xml"),
                              name.c_str ());
          return NULL;
        }

      string minVerStr = ((char*) xmlGetProp (nodes->nodeTab[i],
                                              (const xmlChar*) "min-version"));
      string maxVerStr = ((char*) xmlGetProp (nodes->nodeTab[i],
                                              (const xmlChar*) "max-version"));

      int minVersion = PluginInfo::convertVersion (minVerStr);
      int maxVersion = PluginInfo::convertVersion (maxVerStr);

      if (minVersion == -1 || maxVersion == -1)
        {
          server->log (MYSERVER_LOG_MSG_ERROR,
                              _("Error loading plugin `%s': invalid plugin.xml"),
                              name.c_str ());
          return NULL;
        }

      pinfo->addDependence (nameDep, minVersion, maxVersion);
    }

  pinfoAutoPtr.release ();
  return pinfo;
}

/*!
  Add a plugin.
  \param file The plugin file name.
  \param server The server object to use.
  messages.
  \param global Specify if the library should be loaded globally.
 */
Plugin*
PluginsManager::preLoadPlugin (string &file, Server* server, bool global)
{
  Plugin *plugin = createPluginObject ();

  string name;
  const char* namePtr;

  if (plugin->preLoad (file, global))
    {
      server->log (MYSERVER_LOG_MSG_ERROR,
                          _("Error pre-loading plugin `%s'"),
                          file.c_str ());
      delete plugin;
      return NULL;
    }

  namePtr = plugin->getName ();
  if (namePtr)
    name.assign (namePtr);
  else
    {
      server->log (MYSERVER_LOG_MSG_ERROR,
                          _("Error pre-loading plugin `%s'"),
                          file.c_str ());
      delete plugin;
      return NULL;
    }

  if (plugin->findSymbol ("__gpl_license") == NULL)
    {
      server->log (MYSERVER_LOG_MSG_ERROR,
                   _("Error loading plugin `%s', the license is not GPL"),
                   file.c_str ());
      delete plugin;
      return NULL;
    }

  return plugin;
}

void
PluginsManager::recursiveDependencesFallDown (Server* server, string &name,
                                              HashMap<string, bool> &remove,
                                  HashMap<string, list<string>*> &dependsOn)
{
  remove.put (name, true);
  list<string>* dependsList = dependsOn.get (name);
  if (!dependsList || dependsList->empty ())
    return;

  list<string>::iterator lit = dependsList->begin ();

  for (; lit != dependsList->end (); lit++)
    {
      recursiveDependencesFallDown (server, *lit, remove, dependsOn);

      server->log (MYSERVER_LOG_MSG_WARNING,
                          _("Missing plugin dependence `%s' --> `%s'"),
                          name.c_str (), (*lit).c_str ());
    }

}

/*!
  Load the plugins.
  \param server The server object to use.
 */
int
PluginsManager::load (Server *server)
{
  list<string*> toRemove;
  HashMap<string, list<string>*> dependsOn;
  HashMap<string, PluginInfo*>::Iterator it = pluginsInfos.begin ();
  HashMap<string, bool> remove;
  while (it != pluginsInfos.end ())
    {
      string name (it.getKey ());
      PluginInfo* pinfo = *it;
      HashMap<string, pair<int, int>* >::Iterator depIt = pinfo->begin ();

      string msversion (MYSERVER_VERSION);
      size_t i = msversion.find ("-", 0);
      if (i != string::npos)
        msversion = msversion.substr (0, i);

      int msVersion = PluginInfo::convertVersion (msversion);
      if (msVersion < pinfo->getMyServerMinVersion ()
          || msVersion > pinfo->getMyServerMaxVersion ())
        server->log (MYSERVER_LOG_MSG_WARNING,
                            _("Plugin `%s' not compatible with this version"),
                            name.c_str ());
      else
        remove.put (name, false);

      for (; depIt != pinfo->end (); depIt++)
        {
          string dname = depIt.getKey ();

          list<string>* deps = dependsOn.get (dname);
          if (!deps)
            {
              deps = new list<string > ();
              dependsOn.put (dname, deps);
            }

          deps->push_front (name);
        }

      it++;
    }

  list<string*>::iterator tRIt = toRemove.begin ();
  for (; tRIt != toRemove.end (); tRIt++)
    removePlugin (**tRIt);
  toRemove.clear ();

  HashMap<string, list<string>*>::Iterator dIt = dependsOn.begin ();
  for (; dIt != dependsOn.end (); dIt++)
    {
      string logBuf;
      string dname = dIt.getKey ();

      PluginInfo* pinfo = getPluginInfo (dname);

      if (!pinfo || pinfo->getVersion () == 0)
        remove.put (dname, true);

      list<string>* dependsList = (*dIt);
      if (!dependsList)
        continue;
      if (dependsList->empty ())
        continue;

      bool rem = remove.get (dname);
      if (rem)
        {
          recursiveDependencesFallDown (server, dname, remove, dependsOn);
          continue;
        }

      HashMap<string, pair<int, int>* >::Iterator lit = pinfo->begin ();
      for (; lit != pinfo->end (); lit++)
        {
          string depN = lit.getKey ();
          PluginInfo* dep = getPluginInfo (depN);
          if (!dep || remove.get (depN))
            {
              server->log (MYSERVER_LOG_MSG_WARNING,
                                  _("Missing plugin dependence `%s' --> `%s'"),
                                  dname.c_str (), depN.c_str ());
              recursiveDependencesFallDown (server, dname, remove, dependsOn);
              break;
            }


          pair<int, int>* pdep = *lit;
          if (dep->getVersion () < pdep->first
              || dep->getVersion () > pdep->second)
            {
              recursiveDependencesFallDown (server, dname, remove, dependsOn);
              server->log (MYSERVER_LOG_MSG_WARNING,
                            _("Plugin `%s' not compatible with this version"),
                            dname.c_str ());
              break;
            }
        }
    }


  HashMap<string, bool>::Iterator rIt = remove.begin ();
  for (; rIt != remove.end (); rIt++)
    {
      string name (rIt.getKey ());
      if (*rIt)
        removePlugin (name);
    }

  for (it = pluginsInfos.begin (); it != pluginsInfos.end (); it++)
    (*it)->getPlugin ()->load (server);

  return 0;
}

/*!
  PostLoad functions, called once all the plugins are loaded.
  \param server The server object to use.
 */
int
PluginsManager::postLoad (Server *server)
{
  HashMap<string, PluginInfo*>::Iterator it = pluginsInfos.begin ();
  while (it != pluginsInfos.end ())
    {
      Plugin* plugin = (*it)->getPlugin ();
      if (plugin)
        {
          plugin->postLoad (server);
          server->log (MYSERVER_LOG_MSG_INFO, _("Loaded plugin `%s'"),
		       (*it)->getName ().c_str ());
        }
      it++;
    }
  return 0;
}

/*!
  Unload the plugins.
  messages.
 */
int
PluginsManager::unLoad ()
{

  HashMap<string, PluginInfo*>::Iterator poit = pluginsInfos.begin ();

  while (poit != pluginsInfos.end ())
    {
      (*poit)->getPlugin ()->unLoad ();
      delete *poit;
      poit++;
    }

  pluginsInfos.clear ();
  return 0;
}

/*!
  Remove a plugin  without clean it.
  \param name The plugin to remove.
 */
void
PluginsManager::removePlugin (string &name)
{
  PluginInfo* info = pluginsInfos.remove (name);

  delete info;
}

/*!
  Add a plugin option structure.
  \param plugin The plugin name.
  \param pi The options for the plugin.
 */
int
PluginsManager::addPluginInfo (string &plugin, PluginInfo* pi)
{
  PluginInfo* oldPi = pluginsInfos.put (plugin, pi);

  if (oldPi)
    delete oldPi;

  return 0;
}

/*!
  Return a pluginOption.
  \param plugin The plugin name.
 */
PluginInfo*
PluginsManager::getPluginInfo (string &plugin)
{
  return pluginsInfos.get (plugin);
}
