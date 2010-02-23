/*
  MyServer
  Copyright (C) 2009 The Free Software Foundation Inc.
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
#include <myserver.h>

#include <libguile.h>

#ifdef WIN32
# define EXPORTABLE(x) x _declspec(dllexport)
#else
# define EXPORTABLE(x) extern "C" x
#endif

EXPORTABLE(char*) name (char* name, u_long len)
{
  char* str = (char*) "guile";
  if(name)
    strncpy(name, str, len);
  return str;
}

EXPORTABLE(int) eval (char *const string)
{
  scm_c_eval_string (string);
  return 0;
}

EXPORTABLE(int) load (void* server,void* parser)
{
  return 0;
}

EXPORTABLE(int) postLoad (void* server,void* parser)
{
  return 0;
}

EXPORTABLE(int) unLoad (void* parser)
{
  return 0;
}
