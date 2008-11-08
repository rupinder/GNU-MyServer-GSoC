/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2006, 2008 Free Software Foundation, Inc.
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
#define CONSOLE_H

#include <iostream>

#include <include/filter/stream.h>

using namespace std;

enum MyServerColor
  {
    /* Foreground colors. */
    MYSERVER_FG_COLOR_BLACK,
    MYSERVER_FG_COLOR_RED,
    MYSERVER_FG_COLOR_GREEN,
    MYSERVER_FG_COLOR_YELLOW,
    MYSERVER_FG_COLOR_BLUE,
    MYSERVER_FG_COLOR_MAGENTA,
    MYSERVER_FG_COLOR_CYAN,
    MYSERVER_FG_COLOR_WHITE,
    MYSERVER_FG_COLOR_RESET,
    MYSERVER_FG_COLOR_NONE,
    /* Background colors */
    MYSERVER_BG_COLOR_BLACK,
    MYSERVER_BG_COLOR_RED,
    MYSERVER_BG_COLOR_GREEN,
    MYSERVER_BG_COLOR_YELLOW,
    MYSERVER_BG_COLOR_BLUE,
    MYSERVER_BG_COLOR_MAGENTA,
    MYSERVER_BG_COLOR_CYAN,
    MYSERVER_BG_COLOR_WHITE,
    MYSERVER_BG_COLOR_RESET,
    MYSERVER_BG_COLOR_NONE
  };

#ifdef WIN32
extern WORD colors[];
#endif
#ifdef NOT_WIN
extern char const* colors[];
#endif

class Console : public Stream
{
public:
  Console ();
  virtual ~Console ();
  virtual int flush (u_long* nbw);
  virtual int read (char* buffer, u_long len, u_long* nbr);
  virtual int write (const char* buffer, u_long len, u_long* nbw);
  virtual int openConsole (string fd);
  int setColor (MyServerColor color[]);
  int reset ();
protected:
  int checkColors (MyServerColor c[]);
  ostream* fd;
};

#endif
