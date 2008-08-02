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


#include "../include/dyn_executor_manager.h"
#include "../include/dynamic_executor.h"

#include <string>

/*!
 *Create the appropriate object to keep a plugin.
 */
Plugin* DynExecutorManager::createPluginObject()
{
  return new DynamicExecutor();
}

/*!
 *Initialize the object.
 */
DynExecutorManager::DynExecutorManager() : 
  PluginsNamespaceManager(string("executors"))
{

}

/*!
 *Destroy the object.
 */
DynExecutorManager::~DynExecutorManager()
{

}
