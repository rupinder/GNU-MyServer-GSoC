/*
MyServer
Copyright (C) 2002, 2003, 2004 The MyServer Team
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

#ifndef PROTOCOLS_MANAGER_H
#define PROTOCOLS_MANAGER_H
#include "../stdafx.h"
#include "../include/xml_parser.h"
#include "../include/protocol.h"
#include "../include/connectionstruct.h"
#include "../include/dynamiclib.h"

#include <string>
using namespace std;

class DynamicProtocol : public Protocol
{
private:
  XmlParser *errorParser;
	string filename;
	DynamicLibrary hinstLib;
	char protocolName[16];
public:
	char *getProtocolName();
	int setFilename(const char *filename);
	DynamicProtocol();
	virtual ~DynamicProtocol();
	char* registerName(char*,int len);
	virtual int controlConnection(ConnectionPtr a,char *b1,char *b2,int bs1,
                                int bs2,u_long nbtr,u_long id);
	int loadProtocol(XmlParser*, Server*);
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
	DynamicProtocol* getDynProtocol(const char *protocolName);
	DynamicProtocol* getDynProtocol(string& protocolName)
    {return getDynProtocol(protocolName.c_str()); };
	int	addProtocol(const char*, XmlParser*, char*, Server* lserver);
	int unloadProtocols(XmlParser*);
	int loadProtocols(const char* directory, XmlParser*, char*, Server* lserver);
};
#endif
