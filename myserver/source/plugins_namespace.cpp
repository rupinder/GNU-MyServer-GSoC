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
#include "../include/plugins_namespace.h"
#include <string>
using namespace std;

/*!
 *Constructor for the class.
 *\param name A name for the namespace.
 */
PluginsNamespace::PluginsNamespace(string name)
{
	this->name.assign(name);
}

/*!
 *Destroy the object.
 */
PluginsNamespace::~PluginsNamespace()
{
	unload(0);
}


/*!
 *Constructor for the class.
 *\param name A name for the namespace.
 *\param clone Another namespace to copy plugins from.  
 *A plugin can exist only in a namespace at once, this constructor will clean 
 *up automatically the clone namespace.
 */
PluginsNamespace::PluginsNamespace(string& name, PluginsNamespace& clone) 
{
	this->name.assign(name);
	HashMap<string, Plugin*>::Iterator it = clone.plugins.begin();
	while(it != clone.plugins.end())
	{
		string name((*it)->getName(0, 0));
		plugins.put(name, *it);

		it++;
	}

}

/*!
 *Get the namespace name.
 */
string& PluginsNamespace::getName()
{
	return name;
}

/*!
 *Get a plugin by its name.
 *\param name The plugin name.
 */
Plugin* PluginsNamespace::getPlugin(string &name)
{
	return plugins.get(name);
}

/*!
 *Post load sequence, called when all the plugins are loaded.
 *\param server The server object to use.
 *\param languageFile The language file to use to retrieve warnings/errors 
 *messages.
 */
int PluginsNamespace::postLoad(Server* server, XmlParser* languageFile)
{
	HashMap<string, Plugin*>::Iterator it = plugins.begin();
	while(it != plugins.end())
	{
		(*it)->postLoad(server, languageFile);
	}
	return 0;
}

/*!
 *Unload the namespace.
 *\param languageFile The language file to use to retrieve warnings/errors 
 *messages.
 */
int PluginsNamespace::unload(XmlParser* languageFile)
{
	HashMap<string, Plugin*>::Iterator it = plugins.begin();
	while(it != plugins.end())
	{
		(*it)->unload(languageFile);
		delete *it;
		it++;
	}
	plugins.clear();
	return 0;

}

/*!
 *Set a name for the namespace.
 *\param name The new name for the namespace.
 */
void PluginsNamespace::setName(string& name)
{
	this->name.assign(name);
}

