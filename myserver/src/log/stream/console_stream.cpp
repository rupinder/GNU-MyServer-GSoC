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

#include "stdafx.h"
#include <include/log/stream/console_stream.h>
#include <include/server/server.h>

ConsoleStream::ConsoleStream (FiltersFactory* ff, u_long cycle, Stream* out,
                              FiltersChain* fc) :
  LogStream (ff, cycle, out, fc)
{
}

/*!
 * Change the console text attributes according to the logging level.
 * \param level The logging level.
 * \return 0 on success, 1 on error.
 */
int
ConsoleStream::setMode (LoggingLevel level)
{
  mutex->lock ();
  int success = 1;
  Console* c = dynamic_cast<Console*>(out);
  if (level == MYSERVER_LOG_MSG_PLAIN)
    {
      success = c->setColor ("reset", "reset");
    }
  else
    {
      Server* server = Server::getInstance ();
      LogManager* lm = server->getLogManager ();
      map<string, string> userColors = server->getConsoleColors ();
      map<LoggingLevel, string> levels = lm->getLoggingLevels ();
      string fg_color = userColors[levels[level] + "_fg"];
      string bg_color = userColors[levels[level] + "_bg"];
      success = c->setColor (fg_color, bg_color);
    }
  mutex->unlock ();
  return success;
}
