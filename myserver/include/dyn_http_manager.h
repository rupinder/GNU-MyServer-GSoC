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
#include "../include/plugin.h"
#include "../include/plugins_namespace_manager.h"
#include <string>
using namespace std;


class DynamicHttpManager : public Plugin
{
public:
	DynamicHttpManager();
	virtual ~DynamicHttpManager();
	int send(HttpThreadContext*, ConnectionPtr s, const char *filenamePath,
                   const char* cgi, int onlyHeader = 0);

};

#endif
