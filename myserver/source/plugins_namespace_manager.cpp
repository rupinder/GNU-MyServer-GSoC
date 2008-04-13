/*
MyServer
Copyright (C) 2007 The MyServer Team
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
#include "../include/plugins_namespace_manager.h"
#include "../include/lfind.h"
#include "../include/xml_parser.h"
#include "../include/server.h"
#include <string>
using namespace std;

/*!
 *Constructor for the class.
 *\param name The name for this namespace.
 */
PluginsNamespaceManager::PluginsNamespaceManager(string name) : 
	PluginsNamespace(name)
{

}

/*!
 *Create the appropriate object to keep a plugin.
 */
Plugin* PluginsNamespaceManager::createPluginObject()
{
	return new Plugin();
}

/*!
 *Add a plugin to the namespace.
 *\param file The plugin file name.
 *\param server The server object to use.
 *\param languageFile The language file to use to retrieve warnings/errors 
 *messages.
 *\param global Specify if the library should be loaded globally.
 */
int PluginsNamespaceManager::addPlugin(string& file, Server* server, 
																			 XmlParser* languageFile, bool global)
{
	Plugin *plugin = createPluginObject();
	string logBuf;
	string name;
	const char* namePtr;

  printf ("%s %i\n", file.c_str(), global);


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

	logBuf.assign(languageFile->getValue("MSG_LOADED"));
	logBuf.append(" ");
	logBuf.append(file);
	logBuf.append(" --> ");
	logBuf.append(name);
	server->logWriteln( logBuf.c_str() );
	return 0;
}


/*!
 *Preload sequence, called when all the plugins are not yet loaded.
 *\param server The server object to use.
 *\param languageFile The language file to use to retrieve warnings/errors 
 *messages.
 *\param resource The resource location to use to load plugins, in this 
 *implementation it is a directory name.
 */
int PluginsNamespaceManager::preLoad(Server* server, XmlParser* languageFile, 
																		 string& resource)
{
	FindData fd;
	string filename;
  string completeFileName;	
	int ret;

  filename.assign(resource);
  filename.append("/");
  filename.append(getName());

	ret = fd.findfirst(filename.c_str());	
	
  if(ret == -1)
  {
		return ret;	
  }

	ret = 0;

	do
	{	
		string name(fd.name);
		PluginsNamespace::PluginOption *po;

		if(fd.name[0]=='.')
			continue;
		/*!
     *Do not consider file other than dynamic libraries.
     */
#ifdef WIN32
		if(!strstr(fd.name,".dll"))
#endif
#ifdef NOT_WIN
		if(!strstr(fd.name,".so"))
#endif		
			continue;

    completeFileName.assign(filename);
		if((fd.name[0] != '/') && (fd.name[0] != '\\')) 
			completeFileName.append("/");
    completeFileName.append(fd.name);

#ifdef WIN32
		name = name.substr(0, name.length() - 4);
#endif
#ifdef NOT_WIN
		name = name.substr(0, name.length() - 3);
#endif

		po = getPluginOption(name);
		
		if(!po || po->enabled)
			ret |= addPlugin(completeFileName, server, languageFile, po && po->global);
	}while(!fd.findnext());
	fd.findclose();

	return ret;
}

/*!
 *Load sequence, called when all the plugins are loaded.
 *\param server The server object to use.
 *\param languageFile The language file to use to retrieve warnings/errors 
 *messages.
 *\param resource The resource location to use to load plugins, in this 
 *implementation it is a directory name.
 */
int PluginsNamespaceManager::load(Server* server, XmlParser* languageFile, 
																		 string& resource)
{
	HashMap<string, Plugin*>::Iterator it = plugins.begin();
	while(it != plugins.end())
	{
		(*it)->load(resource, server, languageFile);
		it++;
	}
	return 0;


}
