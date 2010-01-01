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

#ifndef SOCKET_STREAM_H
# define SOCKET_STREAM_H

# include "stdafx.h"

# include <list>
# include <string>

# include <include/base/socket/socket.h>
# include <include/log/stream/log_stream.h>

class SocketStream : public LogStream
{
public:
  SocketStream (FiltersFactory* filtersFactory,
                u_long cycleLog,
                Stream* outStream,
                FiltersChain* filtersChain);
};

#endif
