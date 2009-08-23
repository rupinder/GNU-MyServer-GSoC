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

#include <include/log/stream/socket_stream_creator.h>

LogStream*
SocketStreamCreator::create (FiltersFactory* ff, string location,
                             list<string>& filters, u_long cycle)
{
  Socket* out = new Socket ();
  char const* host = getHost (location).c_str ();
  u_short port = getPort (location);
  if (out && !out->connect (host, port))
    {
      u_long nbw;
      FiltersChain* fc = ff->chain (filters, out, &nbw);
      if (fc)
        {
          return new SocketStream (ff, cycle, out, fc);
        }
    }
  if (out)
    {
      delete out;
    }
  return 0;
}

u_short
SocketStreamCreator::getPort (string location)
{
  return static_cast<u_short>(atoi (location.substr (location.find (":") + 1).c_str ()));
}

string
SocketStreamCreator::getHost (string location)
{
  return location.substr (0, location.find (":"));
}
