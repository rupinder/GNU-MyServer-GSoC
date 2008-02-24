/*
MyServer
Copyright (C) 2005, 2007, 2008 The MyServer Team
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

#ifndef HTTP_DIR_H
#define HTTP_DIR_H
#include "../stdafx.h"
#include "../include/protocol.h"
#include "../include/http_headers.h"
#include "../include/http_data_handler.h"
#include <list>

using namespace std;

class HttpDir : public HttpDataHandler
{
public:
	struct FileStruct
	{
		string name;
		time_t time_write;
		int attrib;
		off_t size;
	};

  static int load(XmlParser*);
  static int unLoad();
	int send(HttpThreadContext*, ConnectionPtr s, const char* directory, 
                        const char* cgi, int onlyHeader = 0);
  HttpDir();
  virtual ~HttpDir();
private:
  void getFormattedSize(int bytes, string& out);
};


#endif
