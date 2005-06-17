/*
*MyServer
*Copyright (C) 2005 The MyServer Team
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

#ifndef DYN_HTTP_COMMAND_H
#define DYN_HTTP_COMMAND_H

#include "../stdafx.h"
#include "../include/cXMLParser.h"
#include "../include/protocol.h"
#include "../include/connectionstruct.h"
#include "../include/dynamiclib.h"
#include "../include/http_headers.h"
#include "../include/hash_dictionary.h"
#include <string>
using namespace std;

class DynamicHttpCommand 
{
private:
  XmlParser *errorParser;
	string filename;
	DynamicLibrary hinstLib;
public:
	char *getCommandName(char* str,int len=0);
	DynamicHttpCommand();
	virtual ~DynamicHttpCommand();
	int loadCommand(const char*name, XmlParser*, Server*);
	int loadCommand(string &name, XmlParser* p, Server* s)
    {return loadCommand(name.c_str(), p, s);}
	int unloadCommand(XmlParser*);
	int acceptData();
  int send(HttpThreadContext* context, ConnectionPtr lpconnection, 
           string& Uri, int systemrequest=0,int OnlyHeader=0,int yetmapped=0);
};

class DynHttpCommandManager
{
private:
  HashDictionary data;
public:
  int addMethod(const char* name, XmlParser* p, Server* s);
  DynHttpCommandManager();
  virtual ~DynHttpCommandManager();
  int loadMethods(const char* dir, XmlParser* p, Server* s);
  int clean();
  DynamicHttpCommand* getMethodByName(const char* name);
  DynamicHttpCommand* getMethodByNumber(int n);
  int size();


};
#endif
