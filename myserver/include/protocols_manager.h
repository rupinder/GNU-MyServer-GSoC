/*
MyServer
Copyright (C) 2002, 2003, 2004, 2007, 2008 The MyServer Team
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

#ifndef PROTOCOLS_MANAGER_H
#define PROTOCOLS_MANAGER_H
#include "../stdafx.h"
#include "../include/xml_parser.h"
#include "../include/protocol.h"
#include "../include/connection.h"
#include "../include/dynamiclib.h"
#include "../include/plugin.h"
#include "../include/dynamic_protocol.h"
#include "../include/hash_map.h"

#include <string>
using namespace std;

class ProtocolsManager : public PluginsNamespaceManager
{
public:
	ProtocolsManager();
	~ProtocolsManager();
  DynamicProtocol* getPlugin(string& name)
	{
		return (DynamicProtocol*)PluginsNamespaceManager::getPlugin(name);
	}

  Protocol* getProtocol(string& name);

protected:
	virtual Plugin* createPluginObject();
  HashMap<string, Protocol*> staticProtocols;
  void addStaticProtocols();
};

#endif
