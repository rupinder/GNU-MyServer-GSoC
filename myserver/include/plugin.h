/*
MyServer
Copyright (C) 2007 The MyServer Team
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

#include "../stdafx.h"
#include "../include/dynamiclib.h"
#include "../include/hash_map.h"
#include <string>

using namespace std;

class Server;
class XmlParser;

class Plugin
{
public:
	static int getVersionNumber(int version)
	{
		return (version >> 24) & 0xFF;
	}

	static int getVersionMinor(int version)
	{
		return (version >> 16) & 0xFF;
	}

	static int getVersionRevision(int version)
	{
		return (version >> 8) & 0xFF;
	}

	static int getVersionStatus(int version)
	{
		return version & 0xFF;
	}

	static int createVersion(int v = 1, int x = 0, int y = 0, int z = 0)
	{
		return ((v & 0xFF) << 24) | ((x & 0xFF) << 16) | 
			((y & 0xFF) << 8) | z & 0xFF;
	}

	Plugin();
	virtual ~Plugin();
	virtual int load(string& file, Server* server, XmlParser* languageFile);
	virtual int preLoad(string& file, Server* server, XmlParser* languageFile);
	virtual int postLoad(Server* server, XmlParser* languageFile);
	virtual int unLoad(XmlParser* languageFile);
	virtual const char* getName(char* buffer, u_long len);
	virtual int getVersion();
	virtual void* getDirectMethod(char* name);
protected:
	DynamicLibrary hinstLib;
private:
	/*! A number in the form v.x.y.z where:
	 * v is the version major number.
	 * x is the version minor number.
	 * y is the version revision number.
	 * z is the version status number.
	 */
	int version;
};

#endif
