/*
MyServer
Copyright (C) 2006 The MyServer Team
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

#ifndef HOME_DIR_H
#define HOME_DIR_H

#include "../stdafx.h"
#include "../include/file.h"
#include "../include/stringutils.h"
#include "../include/hash_map.h"
#include <string>

using namespace std;

class HomeDir
{
private:
#ifdef WIN32
  string data;
#else
	HashMap<string, string*> data;
#endif
	time_t timestamp;
	int loaded;
public:
	HomeDir();
	~HomeDir();

	void clear();
	int load();
	const string *getHomeDir(string& userName);

};
#endif
