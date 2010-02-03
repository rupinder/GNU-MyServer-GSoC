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


#ifndef HTTP_ERRORS_H
# define HTTP_ERRORS_H

# include "myserver.h"

# include <string>
# include <include/base/hash_map/hash_map.h>

using namespace std;

class HttpErrors
{
public:
  static void getErrorPage (int statusCode, string& out);
  static void getErrorMessage (int statusCode, string& out);
  static void load ();
  static void unLoad ();
private:
  static bool loaded;
  static void putMessage (int, const char*);
  static HashMap<int, const char*> messagesMap;
};
#endif
