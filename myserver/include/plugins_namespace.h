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
	HashMap<char*, Plugin*> plugins;
	void setName(string& name);
public:
	HashMap<char*, Plugin*>::Iterator begin(){return plugins.begin();}
	HashMap<char*, Plugin*>::Iterator end(){return plugins.end();}
	string& getName();
	PluginsNamespace(string name);
	PluginsNamespace(string& name, PluginsNamespace& clone);
	Plugin* getPlugin(string& name);
	virtual int preLoad(Server* server, XmlParser* languageFile, 
											string& resource) = 0;
	virtual int load(Server* server, XmlParser* languageFile, 
									 string& resource) = 0;
	virtual int postLoad(Server* server, XmlParser* languageFile);
	virtual int unLoad(XmlParser* languageFile);
	virtual ~PluginsNamespace();
};

#endif
