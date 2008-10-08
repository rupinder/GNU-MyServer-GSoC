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

#include <include/log/stream/console_stream.h>

ConsoleStream::ConsoleStream (FiltersFactory* ff, u_long cycle, Stream* out,
                              FiltersChain* fc) :  
  LogStream (ff, cycle, out, fc)
{
}

int
ConsoleStream::enterErrorMode ()
{
  mutex->lock ();
  int success = dynamic_cast<Console*>(out)->enterErrorMode ();
  mutex->unlock ();
  return success;
}

int
ConsoleStream::exitErrorMode ()
{
  mutex->lock ();
  int success = dynamic_cast<Console*>(out)->exitErrorMode ();
  mutex->unlock ();
  return success;
}
