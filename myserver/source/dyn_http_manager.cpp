/*
MyServer
Copyright (C) 2005, 2007, 2008 The MyServer Team
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


#include "../include/dyn_http_manager.h"
#include "../include/xml_parser.h"
#include "../include/server.h"
#include "../include/find_data.h"

#include <string>

typedef int (*sendManagerPROC)(volatile void*, volatile void*, const char*, 
															 const char*, int); 


/*!
 *Default constructor.
 */
DynamicHttpManager::DynamicHttpManager() : Plugin()
{

}

/*!
 *Destroy the object.
 */
DynamicHttpManager::~DynamicHttpManager()
{

}

/*!
 *Control a request.
 */
int DynamicHttpManager::send(HttpThreadContext* context, ConnectionPtr s, 
                             const char *filenamePath, const char* cgi, 
                             int onlyHeader)
{
  sendManagerPROC control = (sendManagerPROC)hinstLib.getProc("sendManager");
  if(control)
    return control(context, s, filenamePath, cgi, onlyHeader);
  else
    return 0;
}

