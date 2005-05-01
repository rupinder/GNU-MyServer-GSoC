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

#ifndef PROTOCOL_H
#define PROTOCOL_H
#include "../stdafx.h"
#include "cXMLParser.h"
#include "connectionstruct.h"
/*! Various options that can be use for the protocol.  */
#define PROTOCOL_USES_SSL 1

/*!
 *This is the base class to derive other protocols implementations for the server.
 */
class Protocol 
{
private:

public:
	int protocolOptions;
	Protocol();
	char* registerName(char*,int len);
	virtual int controlConnection(ConnectionPtr a, char *b1, char *b2,
                                int bs1, int bs2, u_long nbtr, u_long id);
	static int loadProtocol(XmlParser*);
	static int unloadProtocol(XmlParser*);
};
#endif
