/*
  MyServer
  Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010 Free Software
  Foundation, Inc.
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

#include "myserver.h"

#include <include/protocol/http/dyn_http_command_manager.h>
#include <include/protocol/http/dyn_http_command.h>

#include <string>


/*!
  Initialize the object.
 */
DynHttpCommandManager::DynHttpCommandManager ()
{

}

/*!
  Destroy the object.
 */
DynHttpCommandManager::~DynHttpCommandManager ()
{

}

/*!
  Get the DynamicHttpCommand.
  \param name http command name.
 */
DynamicHttpCommand* DynHttpCommandManager::getHttpCommand (string& name)
{
  return dynamicHttpCommands.get (name);
}

/*!
  Add the DynamicHttpCommand.
  \param name http command name.
  \param httpCommand http command to add.
 */
void DynHttpCommandManager::addHttpCommand (string& name, DynamicHttpCommand* httpCommand)
{
  dynamicHttpCommands.put (name,httpCommand);
}

/*!
  Free the used memory.
 */
void DynHttpCommandManager::clear ()
{
  for (HashMap<string, DynamicHttpCommand*>::Iterator it = dynamicHttpCommands.begin ();
       it != dynamicHttpCommands.end ();
       it++)
    {
      delete *it;
    }

  dynamicHttpCommands.clear ();
}
