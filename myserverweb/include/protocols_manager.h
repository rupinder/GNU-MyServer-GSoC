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

#ifndef PROTOCOLS_MANAGER_H
#define PROTOCOLS_MANAGER_H
#include "../stdafx.h"
#include "../include/cXMLParser.h"
#include "../include/protocol.h"
#include "../include/connectionstruct.h"

extern "C" {
#ifdef WIN32
#include <direct.h>
#elif HAVE_DL
#include <dlfcn.h>
#define HMODULE void *
#else
#define HMODULE void *
#endif
}


class dynamic_protocol : public protocol
{
private:
  cXMLParser *errorParser;
	char *filename;
	HMODULE hinstLib;
	char protocolName[16];
public:
	char *getProtocolName();
	int setFilename(char *filename);
	dynamic_protocol();
	~dynamic_protocol();
	char* registerName(char*,int len);
	virtual int controlConnection(LPCONNECTION a,char *b1,char *b2,int bs1,
                                int bs2,u_long nbtr,u_long id);
	int loadProtocol(cXMLParser*, char*, cserver*);
	int unloadProtocol(cXMLParser*);	
	int getOptions();
};

struct dynamic_protocol_list_element
{
	dynamic_protocol data;
	dynamic_protocol_list_element* next;
	
};
class protocols_manager
{
private:
	dynamic_protocol_list_element* list;
public:
	protocols_manager();
  dynamic_protocol* getDynProtocolByOrder(int order);
	dynamic_protocol* getDynProtocol(char *protocolName);
	int	addProtocol(char*,cXMLParser*,char*,cserver* lserver);
	int unloadProtocols(cXMLParser*);
	int loadProtocols(char* folder,cXMLParser*,char*,cserver* lserver);
};
#endif
