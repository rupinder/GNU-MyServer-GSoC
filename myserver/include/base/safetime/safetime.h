/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2007, 2009, 2010 Free Software Foundation, Inc.
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

#ifndef SAFETIME_H
# define SAFETIME_H

# include "myserver.h"

# ifdef GETTIMEOFDAY
#  include <sys/time.h>
# endif

# ifdef WIN32
#  include <direct.h>
#  include <time.h>
# endif

void myserver_safetime_init ();
void myserver_safetime_destroy ();
struct tm *myserver_localtime (const time_t *timep, tm* res);
struct tm *myserver_gmtime (const time_t *timep, tm* res);

#endif

