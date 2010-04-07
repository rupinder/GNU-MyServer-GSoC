/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2007, 2008, 2009, 2010 Free Software
  Foundation, Inc.
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
# define MIME_MANAGER_H

# include "myserver.h"

# include <include/base/utility.h>
# include <include/base/hash_map/hash_map.h>
# include <include/base/sync/mutex.h>
# include <include/base/xml/xml_parser.h>
# include <include/base/regex/myserver_regex.h>
# include <include/conf/nodetree.h>

# ifdef WIN32
#  include <windows.h>
# endif

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <ctype.h>

# ifdef WIN32
#  include <tchar.h>
#  include <io.h>
# endif


# include <string>
# include <map>
# include <vector>
# include <list>

using namespace std;

struct MimeRecord
{
  list<string> filters;
  list<string> extensions;
  string mimeType;
  string cmdName;
  string cgiManager;
  bool selfExecuted;
  list<Regex*> pathRegex;
  HashMap<string, NodeTree<string>*> hashedData;

  MimeRecord ();
  MimeRecord (MimeRecord&);
  int addFilter (const char*, bool acceptDuplicate = true);
  ~MimeRecord ();
  void clear ();
  const char* getData (string &name);
  NodeTree<string>* getNodeTree (string &name);
};


class MimeManagerHandler
{
public:
  virtual u_long load (const char *resource){return 0;}
  virtual void close (){}
  virtual MimeRecord* getMIME (const char *file){return NULL;}
  virtual MimeRecord* getMIME (string const &file)
  {return getMIME (file.c_str ());}
  virtual u_long reload (){return 0;}
};


class MimeManager
{
public:
  typedef MimeManagerHandler* (*MAKE_HANDLER)();

  MimeManager ();
  ~MimeManager ();
  u_long reload ();
  MimeRecord* getMIME (const char *file, const char *handler = NULL);
  MimeRecord* getMIME (string const &file, const char *handler = NULL)
  {
    return getMIME (file.c_str (), handler);
  }
  void registerHandler (string &name, MimeManagerHandler *handler);
  void setDefaultHandler (string &name);
  void clean ();
  MimeManagerHandler *getDefaultHandler () {return defHandler;}


  void registerBuilder (string &name, MAKE_HANDLER builder);
  MimeManagerHandler *buildHandler (string &name);

private:
  HashMap<string, MAKE_HANDLER> builders;
  MimeManagerHandler *defHandler;
  HashMap<string, MimeManagerHandler*> handlers;
};

#endif
