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


class DynamicProtocol : public Protocol
{
private:
  XmlParser *errorParser;
	char *filename;
	HMODULE hinstLib;
	char protocolName[16];
public:
	char *getProtocolName();
	int setFilename(char *filename);
	DynamicProtocol();
	virtual ~DynamicProtocol();
	char* registerName(char*,int len);
	virtual int controlConnection(ConnectionPtr a,char *b1,char *b2,int bs1,
                                int bs2,u_long nbtr,u_long id);
	int loadProtocol(XmlParser*, char*, Server*);
	int unloadProtocol(XmlParser*);	
	int getOptions();
};

/*! Structure used to create a linked list. */
struct DynamicProtocolListElement
{
	DynamicProtocol data;
	DynamicProtocolListElement* next;
	
};

class ProtocolsManager
{
private:
	DynamicProtocolListElement* list;
public:
	ProtocolsManager();
  DynamicProtocol* getDynProtocolByOrder(int order);
	DynamicProtocol* getDynProtocol(char *protocolName);
	int	addProtocol(char*, XmlParser*, char*, Server* lserver);
	int unloadProtocols(XmlParser*);
	int loadProtocols(char* directory, XmlParser*, char*, Server* lserver);
};
#endif
