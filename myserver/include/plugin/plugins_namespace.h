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

#ifndef PLUGINS_NAMESPACE_H
#define PLUGINS_NAMESPACE_H

#include "stdafx.h"
#include <include/connection/connection.h>
#include <include/plugin/plugin.h>
#include <include/base/hash_map/hash_map.h>
#include <string>
using namespace std;

class XmlParser;
class Server;

class PluginsNamespace
{
public:
	struct PluginOption
	{
		PluginOption(PluginOption& po){enabled = po.enabled; global = po.global;}
		PluginOption(){enabled = true; global = false;}
		bool enabled;
    bool global;
	};

	HashMap<string, Plugin*>::Iterator begin(){return plugins.begin();}
	HashMap<string, Plugin*>::Iterator end(){return plugins.end();}
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
	virtual int addPreloadedPlugin(Plugin*);
	virtual void removePlugin(string& name);
	bool isLoaded(){return loaded;}

	virtual int addPluginOption(string&, PluginOption&);
	virtual PluginOption* getPluginOption(string&);
protected:
	HashMap<string, Plugin*> plugins;
	HashMap<string, PluginOption*> pluginsOptions;
	void setName(string& name);
private:
	bool loaded;
	string name;
};
#endif
