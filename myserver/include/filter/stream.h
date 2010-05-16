/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2008, 2009, 2010 Free Software
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

#ifndef STREAM_H
# define STREAM_H

# include "myserver.h"

# ifdef WIN32
typedef u_int Handle;
# else
typedef int Handle;
# endif

typedef int FileHandle;

/*!
  Abstract class to handle virtual data streams.
 */
class Stream
{
public:
  virtual int read (char* buffer, u_long len, u_long*);
  virtual int write (const char* buffer, u_long len, u_long*);
  virtual int flush (u_long*);
  virtual Handle getHandle ();
  virtual int close ();
  Stream ();
  /*! Avoid direct instances of this class. */
  virtual ~Stream () = 0;
};

#endif
