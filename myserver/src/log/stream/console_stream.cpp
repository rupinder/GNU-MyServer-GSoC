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

/*!
 * Default color values to use when outputting log messages over the
 * console. Change them according to your tastes :)
 *
 * c[i][0] = Foreground color for the LoggingLevel `i'
 * c[i][1] = Background color for the LoggingLevel `i'
 */
MyServerColor defaultColors[][2] =
  {
    {
      MYSERVER_FG_COLOR_RESET, // } MYSERVER_LOG_MSG_PLAIN
      MYSERVER_BG_COLOR_RESET  // } Don't modify this
    },
    {
      MYSERVER_FG_COLOR_WHITE, // } MYSERVER_LOG_MSG_INFO
      MYSERVER_BG_COLOR_BLACK  // }
    },
    {
      MYSERVER_FG_COLOR_YELLOW, // } MYSERVER_LOG_MSG_WARNING
      MYSERVER_BG_COLOR_BLACK   // }
    },
    {
      MYSERVER_FG_COLOR_RED,   // } MYSERVER_LOG_MSG_ERROR
      MYSERVER_BG_COLOR_BLACK  // }
    }
  };

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
  int success = dynamic_cast<Console*>(out)->setColor (defaultColors[level]);
  mutex->unlock ();
  return success;
}
