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

#ifdef WIN32
WORD colors[] =
  {
    /* Foreground colors */
    0, // Black
    FOREGROUND_RED, // Red
    FOREGROUND_GREEN, // Green
    FOREGROUND_RED | FOREGROUND_GREEN, // Yellow
    FOREGROUND_BLUE, // Blue
    FOREGROUND_RED | FOREGROUND_BLUE // Magenta
    FOREGROUND_BLUE | FOREGROUND_GREEN // Cyan
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE, // White
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE // Reset
    /* Background colors */
    0, // Black
    BACKGROUND_RED, // Red
    BACKGROUND_GREEN, // Green
    BACKGROUND_RED | BACKGROUND_GREEN, // Yellow
    BACKGROUND_BLUE, // Blue
    BACKGROUND_RED | BACKGROUND_BLUE, // Magenta
    BACKGROUND_BLUE | BACKGROUND_GREEN, // Cyan
    BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE, // White
    0 // Reset
  };
#endif
#ifdef NOT_WIN
char const* colors[] =
  {
    /* Foreground colors */
    "\033[30m", // Black
    "\033[31m", // Red
    "\033[32m", // Green
    "\033[33m", // Yellow
    "\033[34m", // Blue
    "\033[35m", // Magenta
    "\033[36m", // Cyan
    "\033[37m", // White
    "\033[0m", // Reset
    /* Background colors */
    "\033[40m", // Black
    "\033[41m", // Red
    "\033[42m", // Green
    "\033[43m", // Yellow
    "\033[44m", // Blue
    "\033[45m", // Magenta
    "\033[46m", // Cyan
    "\033[47m", // White
    "\033[0m" // Reset
  };
#endif

Console::Console () : Stream ()
{
  fd = 0;
}

Console::~Console ()
{
  if (fd) 
    {
      reset ();
    }
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

/*!
 * Check that only allowed values are provided for background and 
 * foreground colors.
 * \param c the array to validate.
 * \return 0 if c is a valid array, 1 else.
 */
int
Console::checkColors (MyServerColor c[])
{
  return 
    c[0] < MYSERVER_FG_COLOR_BLACK || 
    c[0] > MYSERVER_FG_COLOR_RESET ||
    c[1] < MYSERVER_BG_COLOR_BLACK || 
    c[1] > MYSERVER_BG_COLOR_RESET;
}

/*!
 * Set the attributes for the console text.
 * \param c[0] holds the foreground color, c[1] the background one.
 * \return 0 on success, 1 on error.
 */
int
Console::setColor (MyServerColor c[])
{
  if (!checkColors (c))
    {
#ifdef WIN32
      SetConsoleTextAttribute (GetStdHandle ((fd == &cout) ? STD_OUTPUT_HANDLE : STD_ERROR_HANDLE),
                               colors[c[0]] | colors[c[1]]);
#endif
#ifdef NOT_WIN
      *fd << colors[c[0]] << colors[c[1]];
#endif
      return 0;
    }
  return 1;
}

/*!
 * Restore the original console colors (white text on black background on
 * WIN32).
 * \return 0 on success, 1 on error.
 */
int
Console::reset ()
{
  MyServerColor c[] = { MYSERVER_FG_COLOR_RESET, MYSERVER_BG_COLOR_RESET };
  return setColor (c);
}
