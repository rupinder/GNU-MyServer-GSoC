/*
  MyServer
  Copyright (C) 2006, 2008, 2009 Free Software Foundation, Inc.
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

#include "stdafx.h"
#include <include/log/stream/log_stream_factory.h>

LogStreamFactory::LogStreamFactory ()
{
  creators["file://"] = new FileStreamCreator ();
  creators["console://"] = new ConsoleStreamCreator ();
  creators["socket://"] = new SocketStreamCreator ();
}

LogStreamFactory::~LogStreamFactory ()
{
  map<string, LogStreamCreator*>::iterator it;
  for (it = creators.begin (); it != creators.end (); it++)
    {
      delete it->second;
    }
  creators.clear ();
}

LogStream*
LogStreamFactory::create (FiltersFactory* ff, string location,
                          list<string>& filters, u_long cycle)
{
  string protocol (getProtocol (location));
  string path (getPath (location));
  if (protocolCheck (protocol))
    {
      return creators[protocol]->create (ff, path, filters, cycle);
    }
  return 0;
}

string
LogStreamFactory::getProtocol (string location)
{
  if (location.find ("://") != string::npos)
    return (location.substr (0, location.find ("://"))).append ("://");
  return string ("");
}

string
LogStreamFactory::getPath (string location)
{
  if (protocolCheck (getProtocol (location)))
    return location.substr (getProtocol (location).size (), location.size ());
  return string ("");
}

bool
LogStreamFactory::protocolCheck (string protocol)
{
  return creators.count (protocol) != 0;
}
