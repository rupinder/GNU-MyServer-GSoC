/*
MyServer
Copyright (C) 2005, 2007, 2008 Free Software Foundation, Inc.
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


#include <include/protocol/http/dyn_http_manager_list.h>
#include <include/protocol/http/dyn_http_manager.h>
#include <include/server/server.h>

#include <string>


/*!
 *Initialize the object.
 */
DynHttpManagerList::DynHttpManagerList() 
{

}

/*!
 *Destroy the object.
 */
DynHttpManagerList::~DynHttpManagerList()
{

}

/*!
 *Get the DynamicHttpManagers.
 *\param name http manager name.
 */
DynamicHttpManager* DynHttpManagerList::getHttpManager(string& name)
{
  return dynamicHttpManagers.get(name);
}

/*!
 *Add the DynamicHttpManagers.
 *\param name http manager name.
 *\param httpManager http manager to add.
 */
void DynHttpManagerList::addHttpManager(string& name, DynamicHttpManager* httpManager)
{
  dynamicHttpManagers.put(name,httpManager);
}

