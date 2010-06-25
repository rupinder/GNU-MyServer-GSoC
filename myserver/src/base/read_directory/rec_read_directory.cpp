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

#include "myserver.h"
#include <include/base/read_directory/rec_read_directory.h>

#include <string>

using namespace std;


void RecReadDirectory::generate (const char* path)
{
  FTSENT *e;
  char *argv[2] = {(char *) path, NULL};
    
  FTS* mytrav = fts_open (argv, FTS_LOGICAL, NULL);

  while (e = fts_read (mytrav))
    {
      if (e->fts_level == 6)
        continue;
      
    }
    
}
