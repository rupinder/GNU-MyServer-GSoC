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

#ifndef PLUGINS_NAMESPACE_MANAGER_H
#define PLUGINS_NAMESPACE_MANAGER_H

#include "../stdafx.h"
#include "../include/plugins_namespace.h"

using namespace std;

class PluginsNamespaceManager : public PluginsNamespace
{
public:
	PluginsNamespaceManager(string name);

	virtual int preload(Server* server, XmlParser* languageFile, 
											string& resource);
	virtual int load(Server* server, XmlParser* languageFile, 
									 string& resource);
private:
	int addPlugin(string& file, Server* server, XmlParser* languageFile);

protected:
	virtual Plugin* createPluginObject();
};

#endif
