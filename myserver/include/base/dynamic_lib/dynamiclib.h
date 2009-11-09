/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2005, 2007, 2009 Free Software Foundation, Inc.
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

#ifndef DYNAMICLIBRARY_H
# define DYNAMICLIBRARY_H
# include "stdafx.h"

# include <string>

extern "C"
{
# ifdef WIN32
#  include <direct.h>
# endif
# ifdef HAVE_DL
#  include <dlfcn.h>
# endif
}

using namespace std;

class DynamicLibrary
{
public:
  int validHandle ();
  DynamicLibrary ();
  ~DynamicLibrary ();
  int loadLibrary (const char* filename, int globally=0);
  void* getProc (const char*);
  int close ();
  const char* getFileName (){return fileName.c_str ();}
private:
  string fileName;
  void *handle;

};

#endif
