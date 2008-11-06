/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2007, 2008 Free Software Foundation, Inc.
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

#ifndef PLUGIN_H
#define PLUGIN_H

#include "stdafx.h"
#include <include/base/dynamic_lib/dynamiclib.h>
#include <include/base/hash_map/hash_map.h>
#include <string>

using namespace std;

class Server;
class XmlParser;

class Plugin
{
public:
	Plugin();
	virtual ~Plugin();
	virtual int load(string& file, Server* server, XmlParser* languageFile);
	virtual int preLoad(string& file, Server* server, XmlParser* languageFile, bool global);
	virtual int postLoad(Server* server, XmlParser* languageFile);
	virtual int unLoad(XmlParser* languageFile);
	virtual const char* getName(char* buffer, u_long len);
	virtual void* getDirectMethod(char* name);
protected:
	DynamicLibrary hinstLib;

};

#endif
