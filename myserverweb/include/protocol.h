/*
*MyServer
*Copyright (C) 2002 The MyServer Team
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
#include "../include/cXMLParser.h"
#include "../include/connectionstruct.h"
/*!
*This is the base class to derive from other protocols implementations for the server.
*/
class protocol 
{
private:

public:
	virtual int controlConnection(LPCONNECTION a,char *b1,char *b2,int bs1,int bs2,u_long nbtr,u_long id);
	static int loadProtocol(cXMLParser*);
	static int unloadProtocol(cXMLParser*);

};
#endif
