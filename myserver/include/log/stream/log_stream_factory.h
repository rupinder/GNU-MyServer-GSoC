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

#ifndef LOG_STREAM_FACTORY_H
# define LOG_STREAM_FACTORY_H

# include "stdafx.h"

# include <list>
# include <map>
# include <string>

# include <include/log/stream/log_stream.h>
# include <include/log/stream/console_stream_creator.h>
# include <include/log/stream/file_stream_creator.h>
# include <include/log/stream/socket_stream_creator.h>
# include <include/log/stream/log_stream_creator.h>

using namespace std;

class LogStreamFactory
{
public:
  LogStreamFactory ();
  ~LogStreamFactory ();
  LogStream* create (FiltersFactory*, string, list<string>&, u_long);
  string getProtocol (string location);
  string getPath (string location);
  bool protocolCheck (string protocol);
private:
  map<string, LogStreamCreator*> creators;;
};

#endif
