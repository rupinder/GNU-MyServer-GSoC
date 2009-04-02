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

#include <include/filter/console.h>

Console::Console () : Stream ()
{
  fd = 0;
#ifdef WIN32
  fg_colors["black"] = 0;
  fg_colors["red"] = FOREGROUND_RED;
  fg_colors["green"] = FOREGROUND_GREEN;
  fg_colors["yellow"] = FOREGROUND_RED | FOREGROUND_GREEN;
  fg_colors["blue"] = FOREGROUND_BLUE;
  fg_colors["magenta"] = FOREGROUND_RED | FOREGROUND_BLUE;
  fg_colors["cyan"] = FOREGROUND_BLUE | FOREGROUND_GREEN;
  fg_colors["white"] = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
  fg_colors["reset"] = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

  bg_colors["black"] = 0;
  bg_colors["red"] = BACKGROUND_RED;
  bg_colors["green"] = BACKGROUND_GREEN;
  bg_colors["yellow"] = BACKGROUND_RED | BACKGROUND_GREEN;
  bg_colors["blue"] = BACKGROUND_BLUE;
  bg_colors["magenta"] = BACKGROUND_RED | BACKGROUND_BLUE;
  bg_colors["cyan"] = BACKGROUND_BLUE | BACKGROUND_GREEN;
  bg_colors["white"] = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;
  bg_colors["reset"] = 0;
#endif
#ifndef WIN32
  fg_colors["black"] = "\033[30m";
  fg_colors["red"] = "\033[31m";
  fg_colors["green"] = "\033[32m";
  fg_colors["yellow"] = "\033[33m";
  fg_colors["blue"] = "\033[34m";
  fg_colors["magenta"] = "\033[35m";
  fg_colors["cyan"] = "\033[36m";
  fg_colors["white"] = "\033[37m";
  fg_colors["reset"] = "\033[0m";

  bg_colors["black"] = "\033[40m";
  bg_colors["red"] = "\033[41m";
  bg_colors["green"] = "\033[42m";
  bg_colors["yellow"] = "\033[43m";
  bg_colors["blue"] = "\033[44m";
  bg_colors["magenta"] = "\033[45m";
  bg_colors["cyan"] = "033[46m";
  bg_colors["white"] = "\033[47m";
  bg_colors["reset"] = "\033[0m";
#endif
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
 * Set the attributes for the console text. If a not valid color is provided,
 * the console attribute for both background and foreground text will be
 * reset.
 * \param fg_color the foreground text attribute.
 * \param bg_color the background text attribute.
 * \return 0 on success, 1 on error.
 */
int
Console::setColor (string fg_color, string bg_color)
{
#ifdef WIN32
  WORD attrs;
  DWORD nStdHandle;
  HANDLE h;
  if (fg_colors.count (fg_color) && bg_colors.count (bg_color))
    {
      attrs = fg_colors[fg_color] | bg_colors[bg_color];
    }
  else
    {
      attrs = fg_colors["reset"] | bg_colors["reset"];
    }
  if (fd == &cout)
    {
      nStdHandle = STD_OUTPUT_HANDLE;
    }
  else
    {
      nStdHandle = STD_ERROR_HANDLE;
    }
  h = GetStdHandle (nStdHandle);
  SetConsoleTextAttribute (attrs, h);
#else
  if (fg_colors.count (fg_color) && bg_colors.count (bg_color))
    {
      *fd << fg_colors[fg_color].c_str () << bg_colors[bg_color].c_str ();
    }
  else
    {
      *fd << fg_colors["reset"].c_str () << bg_colors["reset"].c_str ();
    }
#endif
      return 0;
}

/*!
 * Restore the original console colors (white text on black background on
 * WIN32).
 * \return 0 on success, 1 on error.
 */
int
Console::reset ()
{
  return setColor ("reset", "reset");
}
