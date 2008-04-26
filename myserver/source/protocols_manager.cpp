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


#include "../include/protocols_manager.h"
#include "../include/xml_parser.h"
#include "../include/server.h"
#include "../include/lfind.h"
#include "../include/dynamic_protocol.h"
#include "../include/http.h"
#include "../include/ftp.h"
#include "../include/https.h"
#include "../include/control_protocol.h"
#include <string>

/*!
 *Create the appropriate object to keep a plugin.
 */
Plugin* ProtocolsManager::createPluginObject()
{
	return new DynamicProtocol();
}

/*!
 *Class constructor.
 */
ProtocolsManager::ProtocolsManager() : PluginsNamespaceManager("protocols")
{
  addStaticProtocols();
}

/*!
 *Class destructor.
 */
ProtocolsManager::~ProtocolsManager()
{

}


/*!
 *Return a protocol by its name.
 */
Protocol* ProtocolsManager::getProtocol(string& name)
{
  Protocol* staticProtocol = staticProtocols.get(name);

  if(staticProtocol)
    return staticProtocol;

  return getPlugin(name);
}

/*!
 *Populate the map with static builtin protocols.
 */
void ProtocolsManager::addStaticProtocols()
{
  string protocolName;

  protocolName.assign("http");
  staticProtocols.put(protocolName, new HttpProtocol());

  protocolName.assign("ftp");
  staticProtocols.put(protocolName, new Ftp());

  protocolName.assign("https");
  staticProtocols.put(protocolName, new HttpsProtocol());

  protocolName.assign("control");
  staticProtocols.put(protocolName, new ControlProtocol());

}
