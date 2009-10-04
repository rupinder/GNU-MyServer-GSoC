/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2009 Free Software Foundation, Inc.
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

#ifndef MAIN_CONFIGURATION_H
# define MAIN_CONFIGURATION_H

# include "stdafx.h"
# include <string>

using namespace std;

/*! Define the interface to read from the main configuration file.  */
class MainConfiguration
{
public:
  MainConfiguration ();
  virtual ~MainConfiguration ();
  virtual const char *getValue (const char* field) = 0;
  virtual const char *getValue (string const &field);
  virtual int close ()
  {
    return 0;
  }
private:

};

#endif
