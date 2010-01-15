/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2006, 2008, 2009, 2010 Free Software Foundation, Inc.
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

#ifndef HOME_DIR_H
# define HOME_DIR_H

# include "myserver.h"
# include <include/base/file/file.h>
# include <include/base/string/stringutils.h>
# include <include/base/hash_map/hash_map.h>
# include <include/base/sync/mutex.h>
# include <string>

using namespace std;

class HomeDir
{
public:
  HomeDir ();
  ~HomeDir ();

  void clear ();
  int load ();

  int getHomeDir (string& userName, string& out);

  int isLoaded (){return loaded;}

private:
  Mutex loadMutex;
  int loadImpl ();

# ifdef WIN32
  string data;
# elif !GETPWNAM
  HashMap<string, string*> data;
# endif
  time_t timestamp;
  int loaded;
};
#endif
