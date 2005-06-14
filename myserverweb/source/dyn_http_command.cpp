/*
*MyServer
*Copyright (C) 2005 The MyServer Team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "../include/dyn_http_command.h"
#include "../include/cXMLParser.h"
#include "../include/cserver.h"
#include "../include/lfind.h"

#include <string>

#ifdef NOT_WIN
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

// Bloodshed Dev-C++ Helper
#ifndef intptr_t
#define intptr_t int
#endif

typedef int (*loadMethodPROC)(void*, void*); 
typedef int (*acceptDataPROC)();
typedef int (*unloadMethodPROC)(void* languageParser); 
typedef int (*controlMethodPROC)(void*, volatile void*, const char*, int, int, int); 
typedef char* (*registerNamePROC)(char*, int); 

/*!
 *Get the command name.
 */
char *DynamicHttpCommand::getCommandName(char* out, int len)
{
  registerNamePROC name = (registerNamePROC)hinstLib.getProc("registerName");
  if(name)
    return name(out, len);
}

/*!
 *Default constructor.
 */
DynamicHttpCommand::DynamicHttpCommand()
{

}

/*!
 *Destroy the object.
 */
DynamicHttpCommand::~DynamicHttpCommand()
{
  hinstLib.close();
}

/*!
 *Load the plugin. Returns 0 on success.
 */
int DynamicHttpCommand::loadCommand(char* name, XmlParser* parser, Server* server)
{
  if(hinstLib.loadLibrary(name))
    return 1;
  loadMethodPROC load =(loadMethodPROC)hinstLib.getProc("loadMethod");
  errorParser=parser;
  if(load)
    return load(parser, server);
}

/*!
 *Unload the plugin.
 */
int DynamicHttpCommand::unloadCommand(XmlParser*)
{
  unloadMethodPROC unload =(unloadMethodPROC) hinstLib.getProc("unloadMethod");
  if(unload)
    return unload(errorParser);
}

/*!
 *Does the method accept POST data? 
 */
int DynamicHttpCommand::acceptData()
{
  acceptDataPROC accept = (acceptDataPROC) hinstLib.getProc("acceptData");
  if(accept)
    return accept();
  else
    /*! By default assume that POST data is not used. */
    return 0;
}

/*!
 *Control a request.
 */
int DynamicHttpCommand::send(HttpThreadContext* context, ConnectionPtr* lpconnection, 
                             string& Uri, int systemrequest, int OnlyHeader, int yetmapped)
{
  controlMethodPROC control = (controlMethodPROC)hinstLib.getProc("controlMethod");
  if(control)
    return control(context, lpconnection, Uri.c_str(), systemrequest, OnlyHeader, yetmapped);
  else
    return 0;
}
