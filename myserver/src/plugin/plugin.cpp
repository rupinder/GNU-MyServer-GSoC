/*
  MyServer
  Copyright (C) 2007, 2008, 2009, 2010 Free Software Foundation, Inc.
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
#include <include/plugin/plugin.h>

using namespace std;

typedef int (*loadPROC)(Server*);
typedef int (*postLoadPROC)(Server*);
typedef int (*unLoadPROC)();
typedef int (*versionPROC)();
typedef const char* (*getNamePROC)();

/*!
  Construct a plugin object.
 */
Plugin::Plugin ()
{

}

/*!
  Destroy the object.
 */
Plugin::~Plugin ()
{
  if (hinstLib.validHandle ())
    hinstLib.close ();
}

/*!
  Load the plugin.
  \param file The filename to load.
  \param server The server instance to use.
  \param languageFile The language file to use to retrieve warnings/errors
  messages.
 */
int Plugin::load (Server* server)
{
  loadPROC proc = (loadPROC)hinstLib.getProc ("load");
  if (proc)
    return proc (server);
  return 0;
}

/*!
  Preload the plugin.  This function doesn't ensure all other
  plugins are yet loaded.
  \param file The filename to load.
  messages.
  \param global Load the shared library globally.
 */
int Plugin::preLoad (string& file, bool global)
{
  return hinstLib.loadLibrary (file.c_str (), global ? 1 : 0);
}

/*!
  Post load initialization.  This is called once all the plugins are loaded.
  \param server The server instance to use.
  \param languageFile The language file to use to retrieve warnings/errors
  messages.
 */
int Plugin::postLoad (Server* server)
{
  if (hinstLib.validHandle ())
    {
      postLoadPROC proc = (postLoadPROC) hinstLib.getProc ("postLoad");
      if (proc)
        return proc (server);
    }
  return 0;

}

/*!
  Unload the plugin.
  \param languageFile The language file to use to retrieve warnings/errors
  messages.
 */
int Plugin::unLoad ()
{
  if (hinstLib.validHandle ())
    {
      unLoadPROC proc = (unLoadPROC) hinstLib.getProc ("unLoad");
      if (proc)
        return proc ();
    }
  return 0;
}

/*!
  Get the plugin name.
 */
const char* Plugin::getName ()
{
  getNamePROC proc;
  if (!hinstLib.validHandle ())
    return NULL;

  proc = (getNamePROC) hinstLib.getProc ("name");
  if (proc)
    return proc ();

  return NULL;
}

/*!
  Get directly access to a method.
  \param name The method name.
 */
void* Plugin::getDirectMethod (char* name)
{
  if (!hinstLib.validHandle ())
    return 0;
  return hinstLib.getProc (name);
}
