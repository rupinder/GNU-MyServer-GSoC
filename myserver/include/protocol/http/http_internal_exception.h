/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010
  Free Software Foundation, Inc.
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

#ifndef HTTP_INTERNAL_EXCEPTION_H
# define HTTP_INTERNAL_EXCEPTION_H

# include <exception>

using namespace std;

class HttpInternalException : public exception
{
public:
  virtual const char *what () const throw ()
  {
    return message;
  }

  HttpInternalException (string & str)
  {
    this->message = str.c_str ();
  }

private:
  const char *message;
};

#endif
