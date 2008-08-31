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

#include <include/plugin/plugin.h>

using namespace std;


typedef int (*loadPROC)(Server*, XmlParser*);
typedef int (*postLoadPROC)(Server*, XmlParser*);
typedef int (*unLoadPROC)();
typedef int (*versionPROC)();
typedef const char* (*getNamePROC)(char*, u_long);

/*!
 *Construct a plugin object.
 */
Plugin::Plugin()
{
  version = 1;
}

/*!
 *Destroy the object.
*/
Plugin::~Plugin()
{
  if(hinstLib.validHandle())
    hinstLib.close();
}

/*!
 *Load the plugin.
 *\param file The filename to load.
 *\param server The server instance to use.
 *\param languageFile The language file to use to retrieve warnings/errors 
 *messages.
 */
int Plugin::load(string& file, Server* server, XmlParser* languageFile)
{
  loadPROC proc = (loadPROC)hinstLib.getProc("load"); 
  if(proc)
    return proc(server, languageFile);
  return 0;
}

/*!
 *Preload the plugin.  This function doesn't ensure all other
 *plugins are yet loaded.
 *\param file The filename to load.
 *\param server The server instance to use.
 *\param languageFile The language file to use to retrieve warnings/errors 
 *messages.
 *\param global Load the shared library globally.
 */
int Plugin::preLoad(string& file, Server* server, XmlParser* languageFile, 
                    bool global)
{
  
  return hinstLib.loadLibrary(file.c_str(), global ? 1 : 0);
}

/*!
 *Post load initialization.  This is called once all the plugins are loaded.
 *\param file The filename to load.
 *\param server The server instance to use.
 *\param languageFile The language file to use to retrieve warnings/errors 
 *messages.
 */
int Plugin::postLoad(Server* server, XmlParser* languageFile)
{
  if(hinstLib.validHandle())
  {
    postLoadPROC proc = (postLoadPROC)hinstLib.getProc("postLoad"); 
    if(proc)
      return proc(server, languageFile);
  }
  return 0;

}

/*!
 *Unload the plugin.
 *\param languageFile The language file to use to retrieve warnings/errors 
 *messages.
 */
int Plugin::unLoad(XmlParser* languageFile)
{
  if(hinstLib.validHandle())
  {
    unLoadPROC proc = (unLoadPROC)hinstLib.getProc("unLoad"); 
    if(proc)
      return proc();
  }
  return 0;
}

/*!
 *Get the version number for this plugin.
 */
int Plugin::getVersion()
{
  if(hinstLib.validHandle())
  {
    versionPROC proc = (versionPROC)hinstLib.getProc("version"); 
    if(proc)
      return proc();
    return 1;
  }
  return 0;
}

/*!
 *Get the plugin name.
 *\param buffer The buffer where write the plugin name.
 *\param len The buffer length in bytes.
 */
const char* Plugin::getName(char* buffer, u_long len)
{
  getNamePROC proc;
  if(!hinstLib.validHandle())
    return 0;
  proc = (getNamePROC)hinstLib.getProc("name");
  if(proc)
    return proc(buffer, len);

  return 0;
}

/*!
 *Get directly access to a method.
 *\param name The method name.
 */
void* Plugin::getDirectMethod(char* name)
{
  if(!hinstLib.validHandle())
    return 0;
  return hinstLib.getProc(name);
}
