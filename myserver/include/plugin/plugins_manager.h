/* -*- mode: c++ -*- */
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
#include <include/base/dynamic_lib/dynamiclib.h>
#include <include/base/hash_map/hash_map.h>
#include <string>
using namespace std;

class Server;
class XmlParser;

class PluginsManager
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
	
	Plugin* getPlugin(string& name);
	
	int preLoad(Server *server, XmlParser* languageFile, string& resource);
	int load(Server *server, XmlParser* languageFile, string& resource);
	int postLoad(Server *server, XmlParser* languageFile);
	int unLoad(Server *server, XmlParser* languageFile);

	virtual void removePlugin(string& name);

	virtual int addPluginOption(string&, PluginOption&);
	virtual PluginOption* getPluginOption(string&);
	
	virtual Plugin* createPluginObject();
	
	PluginsManager();
	~PluginsManager();
	
private:
	HashMap<string, PluginOption*> pluginsOptions;
	HashMap<string, Plugin*> plugins;
	int loadOptions(Server *server, XmlParser* languageFile);
	int addPlugin(string& file, Server* server, 
                XmlParser* languageFile, bool global);
};

#endif
