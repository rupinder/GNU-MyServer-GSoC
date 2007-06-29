/*
MyServer
Copyright (C) 2002, 2003, 2004, 2007 The MyServer Team
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

}

/*!
 *Class destructor.
 */
ProtocolsManager::~ProtocolsManager()
{

}
