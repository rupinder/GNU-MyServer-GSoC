/*
MyServer
Copyright (C) 2005, 2007 Free Software Foundation, Inc.
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DYN_HTTP_COMMAND_H
#define DYN_HTTP_COMMAND_H

#include "../stdafx.h"
#include "../include/xml_parser.h"
#include "../include/protocol.h"
#include "../include/connection.h"
#include "../include/dynamiclib.h"
#include "../include/http_headers.h"
#include "../include/hash_map.h"
#include "../include/plugin.h"
#include "../include/plugins_namespace_manager.h"
#include <string>
using namespace std;

class DynamicHttpCommand : public Plugin
{
public:
	DynamicHttpCommand();
	virtual ~DynamicHttpCommand();
	int acceptData();
  virtual int send(HttpThreadContext* context, ConnectionPtr lpconnection, 
									 string& Uri, int systemrequest = 0, 
									 int OnlyHeader = 0, int yetmapped = 0);
private:
  XmlParser *errorParser;
	string filename;
	DynamicLibrary hinstLib;
};

#endif
