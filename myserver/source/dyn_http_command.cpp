/*
MyServer
Copyright (C) 2005, 2006 The MyServer Team
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "../include/dyn_http_command.h"
#include "../include/xml_parser.h"
#include "../include/server.h"
#include "../include/lfind.h"

#include <string>

typedef int (*acceptDataPROC)();
typedef int (*controlMethodPROC)(void*, volatile void*, const char*, 
																 int, int, int); 

/*!
 *Default constructor.
 */
DynamicHttpCommand::DynamicHttpCommand() : Plugin()
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
int DynamicHttpCommand::send(HttpThreadContext* context, 
														 ConnectionPtr lpconnection, 
                             string& Uri, int systemrequest, 
														 int OnlyHeader, int yetmapped)
{
  controlMethodPROC control = 
		(controlMethodPROC)hinstLib.getProc("controlMethod");
  if(control)
    return control(context, lpconnection, Uri.c_str(), 
									 systemrequest, OnlyHeader, yetmapped);
  else
    return 0;
}
