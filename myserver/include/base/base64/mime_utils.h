/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2007, 2009, 2010 Free Software
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


#ifndef MIMEUTILS_H
# define MIMEUTILS_H

# include "myserver.h"

class CBase64Utils
{
private:
  int ErrorCode;
public:
  int GetLastError () {return ErrorCode;};
  CBase64Utils ();
  ~CBase64Utils ();
  char* decode (const char *in, int *bufsize);
  char* encode (const char *in, int bufsize);
};

class CQPUtils
{
private:
  char* expandBuffer (char *buffer, int UsedSize, int *BufSize, int SingleChar = 1);
  int ErrorCode;
public:
  int getLastError () {return ErrorCode;};
  char* encode (char*in);
  char* decode (char*in);
  CQPUtils ();
  ~CQPUtils ();
};

#endif
