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

#ifndef PLUGINS_MANAGER_H
#define PLUGINS_MANAGER_H

#include "../stdafx.h"
#include "../include/plugin.h"
#include "../include/plugins_namespace.h"
#include "../include/dynamiclib.h"
#include "../include/hash_map.h"
#include <string>
using namespace std;

class Server;
class XmlParser;

class PluginsManager
{
private:
	HashMap<string, PluginsNamespace*> namespaces;
public:
	Plugin* getPlugin(string& namespacename, string& plugin);
	Plugin* getPlugin(string& fullname);
	
	int load(Server *server, XmlParser* languageFile, string& resource);
	int postLoad(Server *server, XmlParser* languageFile);
	int unload(Server *server, XmlParser* languageFile);
	void addNamespace(PluginsNamespace* namespacename);
	PluginsNamespace* getNamespace(string &name);
	PluginsNamespace* removeNamespace(string& name);
};

#endif
