/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2002, 2003, 2004, 2007, 2008, 2009 Free Software Foundation, Inc.
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

#ifndef MIME_MANAGER_H
#define MIME_MANAGER_H

#include <include/base/utility.h>
#include <include/base/hash_map/hash_map.h>
#include <include/base/sync/mutex.h>
#include <include/base/xml/xml_parser.h>
#include <include/base/regex/myserver_regex.h>


#ifdef WIN32
# include <windows.h>
#endif

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef WIN32
# include <tchar.h>
# include <io.h>
#endif
}

#include <string>
#include <map>
#include <vector>
#include <list>

using namespace std;

struct PathRegex
{
  Regex *regex;
  int record;
};

struct MimeRecord
{
	list<string> filters;
	list<string> extensions;
	string mimeType;
	string cmdName;
	string cgiManager;
  bool selfExecuted;
  list<Regex*> pathRegex;
  HashMap<string, string*> hashedData;

	MimeRecord ();
	MimeRecord (MimeRecord&);
	int addFilter (const char*, bool acceptDuplicate = true);
	~MimeRecord ();
	void clear ();
  const char* getHashedData(string &name);
};


class MimeManagerHandler
{
public:
	virtual MimeRecord* getMIME (const char *file){return NULL;}
  virtual MimeRecord* getMIME (string const &file)
  {return getMIME (file.c_str ());}
};

class MimeManager
{
public:
	MimeManager ();
  ~MimeManager ();
	u_long getNumMIMELoaded ();

  u_long loadXML (XmlParser* parser);
	u_long loadXML (const char *filename);
	u_long loadXML (string &filename) {return loadXML (filename.c_str ());}

	MimeRecord* getMIME (const char *file, const char *handler = NULL);
  MimeRecord* getMIME (string const &file, const char *handler = NULL);

  bool isLoaded ();
	void clean ();
	int addRecord (MimeRecord *record);

  static MimeRecord *readRecord (xmlNodePtr node);

  void registerHandler (string &name, MimeManagerHandler *handler);

protected:
	const char *getFilename ();
	void clearRecords ();

private:
  HashMap<string, MimeManagerHandler*> handlers;
  bool loaded;
  HashMap<string, int> extIndex;
  vector<MimeRecord*> records;
  list<PathRegex*> pathRegex;

	u_long numMimeTypesLoaded;
	string filename;
};

#endif
