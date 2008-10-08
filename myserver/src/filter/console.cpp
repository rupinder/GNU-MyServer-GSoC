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

#include <include/filter/console.h>

Console::Console () : Stream ()
{
}

Console::~Console ()
{
  
}

int 
Console::flush (u_long* nbw)
{
  if (fd)
    {
      return *nbw = 0;
    }
  return 1;
}

int
Console::read (char* buffer, u_long len, u_long* nbr)
{
  if (fd)
    {
      return *nbr = 0;
    }
  return 1;
}

int
Console::write (const char* buffer, u_long len, u_long* nbw)
{
  if (fd)
    {
      *fd << buffer;
      *nbw = len;
      return 0;
    }
  return 1;
}

int 
Console::openConsole (string fd)
{
  int success = 1;
  if (!fd.compare ("stdout"))
    {
      this->fd = &cout;
      success = 0;
    }
  else if (!fd.compare ("stderr"))
    {
      this->fd = &cerr;
      success = 0;
    }
  return success;
}

int
Console::enterErrorMode ()
{
#ifdef WIN32

  int success = 
    SetConsoleTextAttribute (GetStdHandle ((fd == &cout) ? STD_OUTPUT_HANDLE : STD_ERROR_HANDLE),
                             FOREGROUND_RED |
                             FOREGROUND_INTENSITY);
  if (success)
    return 0;
#endif
#ifdef NOT_WIN
  *fd << "\033[31;1m";
  return 0;
#endif
}

int
Console::exitErrorMode ()
{
#ifdef WIN32
  int success = 
    SetConsoleTextAttribute (GetStdHandle ((fd == &cout) ? STD_OUTPUT_HANDLE : STD_ERROR_HANDLE),
                             FOREGROUND_RED |
                             FOREGROUND_GREEN |
                             FOREGROUND_BLUE);
  if (success)
    return 0;
#endif
#ifdef NOT_WIN
  *fd << "\033[0m";
  return 0;
#endif
}
