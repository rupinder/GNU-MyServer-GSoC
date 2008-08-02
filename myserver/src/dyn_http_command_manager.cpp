/*
MyServer
Copyright (C) 2005, 2006, 2007, 2008 Free Software Foundation, Inc.
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


#include "../include/dyn_http_command_manager.h"
#include "../include/dyn_http_command.h"

#include <string>

/*!
 *Create the plugin object.
 */
Plugin* DynHttpCommandManager::createPluginObject()
{
  return new DynamicHttpCommand();
}


/*!
 *Initialize the object.
 */
DynHttpCommandManager::DynHttpCommandManager() : 
  PluginsNamespaceManager(string("http_commands"))
{

}

/*!
 *Destroy the object.
 */
DynHttpCommandManager::~DynHttpCommandManager()
{

}
