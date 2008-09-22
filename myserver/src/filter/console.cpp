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
  return *nbw = 0;
}

int
Console::read (char* buffer, u_long len, u_long* nbr)
{
  return *nbr = 0;
}

int
Console::write (const char* buffer, u_long len, u_long* nbw)
{
  cout << buffer;
  *nbw = len;
  return 0;
}
