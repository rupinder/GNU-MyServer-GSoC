/*
MyServer
Copyright (C) 2007 Free Software Foundation, Inc.
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

#ifndef PLUGINS_MANAGER_H
#define PLUGINS_MANAGER_H

#include "stdafx.h"
#include <include/plugin/plugin.h>
#include <include/plugin/plugins_namespace.h>
#include <include/base/dynamic_lib/dynamiclib.h>
#include <include/base/hash_map/hash_map.h>
#include <string>
using namespace std;

class Server;
class XmlParser;

class PluginsManager
{
public:
	Plugin* getPlugin(string& namespacename, string& plugin);
	Plugin* getPlugin(string& fullname);
	
	int preLoad(Server *server, XmlParser* languageFile, string& resource);
	int load(Server *server, XmlParser* languageFile, string& resource);
	int postLoad(Server *server, XmlParser* languageFile);
	int unLoad(Server *server, XmlParser* languageFile);
	void addNamespace(PluginsNamespace* namespacename);
	PluginsNamespace* getNamespace(string &name);
	PluginsNamespace* removeNamespace(string& name);
private:
	HashMap<char*, PluginsNamespace*> namespaces;
};

#endif
