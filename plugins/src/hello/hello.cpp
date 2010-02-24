/*
  MyServer
  Copyright (C) 2010 The Free Software Foundation Inc.
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

#include <include/plugin/plugin.h>

PLUGIN_NAME ("hello");

EXPORTABLE(int) load (void* server, void* parser)
{
  printf ("load: hello world!\n");
  return 0;
}

EXPORTABLE(int) postLoad (void* server, void* parser)
{
  printf ("postLoad: hello world!\n");
  return 0;
}

EXPORTABLE(int) unLoad ()
{
  printf ("unLoad: bye world!\n");
  return 0;
}
