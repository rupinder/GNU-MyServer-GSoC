/*
  MyServer
  Copyright (C) 2009, 2010 The Free Software Foundation Inc.
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
#include <include/plugin/plugin.h>


PLUGIN_NAME ("guile");

EXPORTABLE(int) eval (char *const string)
{
  scm_c_eval_string (string);
  return 0;
}

EXPORTABLE(int) eval_file (char *const file)
{
  gh_eval_file (file);
  return 0;
}

EXPORTABLE(int) load (void* server)
{
  scm_init_guile ();
  return 0;
}

EXPORTABLE(int) postLoad (void* server)
{
  return 0;
}

EXPORTABLE(int) unLoad ()
{
  return 0;
}
