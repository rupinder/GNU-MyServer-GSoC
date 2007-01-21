/*
MyServer
Copyright (C) 2002, 2003, 2004, 2007 The MyServer Team
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


#include "../include/protocols_manager.h"
#include "../include/xml_parser.h"
#include "../include/server.h"
#include "../include/lfind.h"

#include <string>


typedef int (*controlConnectionPROC)(void*, char*, char*, int, 
																		 int, u_long, u_long); 

/*!
 *Load the protocol. Called once at runtime.
 */
int DynamicProtocol::loadProtocol(XmlParser* languageFile)
{
	return load(filename, Server::getInstance(), languageFile);
}

/*!
 *Unload the protocol. Called once.
 */
int DynamicProtocol::unloadProtocol(XmlParser* languageFile)
{
	return unload(languageFile);
}

/*!
 *Return the protocol name.
 */
char *DynamicProtocol::getProtocolName()
{
	return (char*) getName(0, 0);	
}

/*!
 *Get the options for the protocol.
 */
int DynamicProtocol::getOptions()
{
	return  protocolOptions;
}

/*!
 *Control the connection.
 */
int DynamicProtocol::controlConnection(ConnectionPtr a, char *b1, char *b2,
                                       int bs1, int bs2, 
																			 u_long nbtr, u_long id)
{
	controlConnectionPROC proc;
	proc = (controlConnectionPROC)hinstLib.getProc("controlConnection"); 

	if(proc)
		return proc((void*)a, b1, b2, bs1, bs2, nbtr, id);
	else
		return 0;
}

/*!
 *Returns the name of the protocol. If an out buffer is defined 
 *fullfill it with the name too.
 */
char* DynamicProtocol::registerName(char* out, int len)
{
	return (char*)getName(out, len);
}

/*!
 *Constructor for the class protocol.
 */
DynamicProtocol::DynamicProtocol(string filename)
{
	protocolOptions = 0;
  this->filename.assign(filename);
}

/*!
 *Destroy the protocol object.
 */
DynamicProtocol::~DynamicProtocol()
{
  unloadProtocol(0);
	protocolOptions = 0;
  filename.assign("");
}
