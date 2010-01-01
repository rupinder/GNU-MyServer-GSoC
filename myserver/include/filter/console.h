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

#ifndef CONSOLE_H
# define CONSOLE_H

# include "stdafx.h"

# include <iostream>
# include <map>

# include <include/filter/stream.h>

using namespace std;

class Console : public Stream
{
public:
  Console ();
  virtual ~Console ();
  virtual int flush (u_long* nbw);
  virtual int read (char* buffer, u_long len, u_long* nbr);
  virtual int write (const char* buffer, u_long len, u_long* nbw);
  virtual int openConsole (string fd);
  int setColor (string fg_color, string bg_color);
  int reset ();
protected:
  ostream* fd;
# ifdef WIN32
  map<string, WORD> fg_colors;
  map<string, WORD> bg_colors;
# else
  map<string, string> fg_colors;
  map<string, string> bg_colors;
# endif
};

#endif
