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

#ifndef PLUGINS_NAMESPACE_H
#define PLUGINS_NAMESPACE_H

#include "../stdafx.h"
#include "../include/connection.h"
#include "../include/plugin.h"
#include "../include/hash_map.h"
#include <string>
using namespace std;

class XmlParser;
class Server;

class PluginsNamespace
{
private:
	string name;
protected:
	HashMap<string, Plugin*> plugins;
	void setName(string& name);
public:
	HashMap<string, Plugin*>::Iterator begin(){return plugins.begin();}
	HashMap<string, Plugin*>::Iterator end(){return plugins.end();}
	string& getName();
	PluginsNamespace(string name);
	PluginsNamespace(string& name, PluginsNamespace& clone);
	Plugin* getPlugin(string& name);
	virtual int load(Server* server, XmlParser* languageFile, 
									 string& resource) = 0;
	virtual int postLoad(Server* server, XmlParser* languageFile);
	virtual int unload(XmlParser* languageFile);
	virtual ~PluginsNamespace();
};

#endif
