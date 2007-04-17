/*
MyServer
Copyright (C) 2007 The MyServer Team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "../stdafx.h"
#include "../include/plugins_manager.h"

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
int PluginsManager::unload(Server *server, XmlParser* languageFile)
{
	int ret = 0;
	HashMap<char*, PluginsNamespace*>::Iterator it = namespaces.begin();

	while(it != namespaces.end())
	{
		ret |= (*it)->unload(languageFile);
		delete (*it);
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
