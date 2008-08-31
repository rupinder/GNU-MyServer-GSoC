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

#ifndef PLUGINS_NAMESPACE_MANAGER_H
#define PLUGINS_NAMESPACE_MANAGER_H

#include "stdafx.h"
#include <include/plugin/plugins_namespace.h>

using namespace std;

class PluginsNamespaceManager : public PluginsNamespace
{
public:
	PluginsNamespaceManager(string name);

	virtual int preLoad(Server* server, XmlParser* languageFile, 
											string& resource);
	virtual int load(Server* server, XmlParser* languageFile, 
									 string& resource);

protected:
	virtual Plugin* createPluginObject();
private:
	int addPlugin(string& file, Server* server, 
                XmlParser* languageFile, bool global);

};

#endif
