/* -*- mode: cpp-mode */
/*
MyServer
Copyright (C) 2002, 2003, 2004, 2007, 2008 Free Software Foundation, Inc.
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
#include "stdafx.h"
#include <include/base/xml/xml_parser.h>
#include <include/protocol/protocol.h>
#include <include/connection/connection.h>
#include <include/base/dynamic_lib/dynamiclib.h>
#include <include/plugin/plugin.h>
#include <include/plugin/protocol/dynamic_protocol.h>
#include <include/base/hash_map/hash_map.h>
#include <list>
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

  void addProtocol(string& name, Protocol* protocol);

  void addProtocol(char* name, Protocol* protocol)
  {
    string strName(name);
    addProtocol(strName, protocol);
  }

	virtual int unLoad(XmlParser* languageFile);
protected:
	virtual Plugin* createPluginObject();
  list<Protocol*> staticProtocolsList;
  HashMap<string, Protocol*> staticProtocols;
};

#endif
