/*
MyServer
Copyright (C) 2002, 2003, 2004, 2007 The MyServer Team
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


#ifndef HTTP_ERRORS_H
#define HTTP_ERRORS_H

#include <string>
#include "../include/mutex.h"
#include "../include/hash_map.h"

using namespace std;

class HttpErrors
{
public:
	static void getErrorPage(int statusCode, string& out);
	static void getErrorMessage(int statusCode, string& out);
	static void load();
	static void unLoad();
private:
	static void putMessage(int, char*);
	static Mutex mutex;
	static HashMap<int, char*> messagesMap;
};
#endif
