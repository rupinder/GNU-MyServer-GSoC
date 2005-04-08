/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
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


#include "../include/protocol.h"
#include "../include/cXMLParser.h"


/*!
 *Load the protocol. Called once at runtime.
 */
int Protocol::loadProtocol(XmlParser* /*languageParser*/)
{
	return 1;
}

/*!
 *Unload the protocol. Called once.
 */
int Protocol::unloadProtocol(XmlParser* /*languageParser*/)
{
	return 1;
}

/*!
 *Control the connection.
 */
int Protocol::controlConnection(ConnectionPtr /*a*/,char* /*b1*/,
                                char* /*b2*/,int /*bs1*/,int /*bs2*/,
                                u_long /*nbtr*/,u_long /*id*/)
{
	/*!
   *Returns value are:
   *0 to delete the connection from the active connections list
   *1 to keep the connection active and clear the connectionBuffer
   *2 if the header is incomplete and to save it in a temporary buffer
   *3 if the header is incomplete without save it in a temporary buffer
   */
	return 0;
}

/*!
 *Returns the name of the protocol. If an out buffer is 
 *defined fullfill it with the name too.
 */
char* Protocol::registerName(char* /*out*/,int /*len*/)
{
	return 0;
}

/*!
 *Constructor for the class protocol.
 */
Protocol::Protocol()
{
	PROTOCOL_OPTIONS=0;
}
