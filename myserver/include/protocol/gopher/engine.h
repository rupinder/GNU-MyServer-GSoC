/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010
  Free Software Foundation, Inc.
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

#ifndef GOPHER_ENGINE_H
# define GOPHER_ENGINE_H

# include "myserver.h"
# include <include/base/hash_map/hash_map.h>
# include <include/conf/vhost/vhost.h>
# include <include/base/base64/mime_utils.h>

using namespace std;

class GopherEngine;

typedef void (GopherEngine::* MyMethodPtr)(const string&, const string&, GopherMenu&);

class GopherEngine
{
public:
  GopherEngine ();
  GopherContent &incoming (GopherRequest, Vhost *tmp);

protected:
  void dirManagement (const string &fname, const string &path, GopherMenu &tmp);
  void fileManagement (const string &fname, const string &path ,GopherMenu &tmp);

  void infoFile (const string &fname, const string &path, GopherMenu &tmp);
  void textFile (const string &fname, const string &path, GopherMenu &tmp);
  void csoFile (const string &fname, const string &path, GopherMenu &tmp);
  void binhexFile (const string &fname, const string &path, GopherMenu &tmp);
  void dosbinFile (const string &fname, const string &path, GopherMenu &tmp);
  void UUencodeFile (const string &fname, const string &path, GopherMenu &tmp);
  void telnetFile (const string &fname, const string &path, GopherMenu &tmp);
  void binFile (const string &fname, const string &path, GopherMenu &tmp);
  void imageFile (const string &fname, const string &path, GopherMenu &tmp);
  void gifFile (const string &fname, const string &path, GopherMenu &tmp);

  MimeRecord* getMIME (string &filename);

  string abs_path;
  string hostname;
  string port;
  Vhost *Host;
  HashMap<string,MyMethodPtr> handlers;

  // Class Const
};
#endif
