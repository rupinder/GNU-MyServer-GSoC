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

#include "../include/protocol.h"
#include "../include/https.h"
#include "../include/cXMLParser.h"
extern "C" 
{
#ifdef WIN32
#include <direct.h>
#include <errno.h>
#endif
#ifdef __linux__
#include <string.h>
#include <errno.h>
#endif
}

#ifndef WIN32
#include "../include/lfind.h"

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

// Bloodshed Dev-C++ Helper
#ifndef intptr_t
#define intptr_t int
#endif

/*!
*Returns the name of the protocol. If an out buffer is defined fullfill it with the name too.
*/
char* https::registerName(char* out,int len)
{
	if(out)
	{
		strncpy(out,"HTTPS",len);
	}
	return "HTTPS";
}
/*!
*https class constructor.
*/
https::https() 
{
	strcpy(protocolPrefix,"https://");
	PROTOCOL_OPTIONS=PROTOCOL_USES_SSL;
}
