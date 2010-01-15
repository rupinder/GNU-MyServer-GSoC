/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2009, 2010 Free Software Foundation, Inc.
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

#ifndef XML_MIME_HANDLER_H
# define XML_MIME_HANDLER_H

# include "myserver.h"

# include <include/base/utility.h>
# include <include/base/hash_map/hash_map.h>
# include <include/base/sync/mutex.h>
# include <include/base/xml/xml_parser.h>
# include <include/base/regex/myserver_regex.h>
# include <include/conf/nodetree.h>
# include <include/conf/mime/mime_manager.h>

# ifdef WIN32
#  include <windows.h>
# endif

extern "C"
{
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <ctype.h>

# ifdef WIN32
#  include <tchar.h>
#  include <io.h>
# endif
}

# include <string>
# include <map>
# include <vector>
# include <list>

using namespace std;


class XmlMimeHandler : public MimeManagerHandler
{
public:
  XmlMimeHandler ();
  virtual ~XmlMimeHandler ();
  virtual void close (){}
  virtual MimeRecord* getMIME (const char *file);
  virtual MimeRecord* getMIME (string const &file)
  {return getMIME (file.c_str ());}
  u_long getNumMIMELoaded ();

  virtual u_long load (XmlParser* parser);
  virtual u_long load (const char *filename);
  virtual u_long load (string &filename) {return load (filename.c_str ());}

  virtual u_long reload ();
  bool isLoaded (){return loaded;}
  void clean ();
  int addRecord (MimeRecord *record);

  static MimeRecord *readRecord (xmlNodePtr node);
  static void registerBuilder (MimeManager& manager);
protected:
  const char *getFilename ();
  void clearRecords ();

private:
  struct PathRegex
  {
    Regex *regex;
    int record;
  };

  HashMap<string, int> extIndex;
  vector<MimeRecord*> records;
  list<PathRegex*> pathRegex;

  u_long numMimeTypesLoaded;
  string filename;
  bool loaded;
};

#endif
