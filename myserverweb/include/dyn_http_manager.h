/*
MyServer
Copyright (C) 2005 The MyServer Team
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

#ifndef DYN_HTTP_MANAGER_H
#define DYN_HTTP_MANAGER_H

#include "../stdafx.h"
#include "../include/xml_parser.h"
#include "../include/protocol.h"
#include "../include/connection.h"
#include "../include/dynamiclib.h"
#include "../include/http_headers.h"
#include "../include/hash_map.h"
#include <string>
using namespace std;

class DynamicHttpManager
{
private:
  XmlParser *errorParser;
	string filename;
	DynamicLibrary hinstLib;
public:
	char *getManagerName(char* str,int len=0);
	DynamicHttpManager();
	virtual ~DynamicHttpManager();
	int loadManager(const char*name, XmlParser*, Server*);
	int loadManager(string &name, XmlParser* p, Server* s)
    {return loadManager(name.c_str(), p, s);}
	int unloadManager(XmlParser*);
	int send(HttpThreadContext*, ConnectionPtr s, const char *filenamePath,
                   const char* cgi, int onlyHeader=0);
};

class DynHttpManagerList
{
private:
  HashMap <string, DynamicHttpManager*> data;
public:
  int addManager(const char* name, XmlParser* p, Server* s);
  DynHttpManagerList();
  virtual ~DynHttpManagerList();
  int loadManagers(const char* dir, XmlParser* p, Server* s);
  int clean();
  DynamicHttpManager* getManagerByName(const char* name);
  int size();


};
#endif
