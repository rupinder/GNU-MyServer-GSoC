/* -*- mode: c++ -*- */
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

#ifndef LOG_STREAM_CREATOR_H
#define LOG_STREAM_CREATOR_H

#include <list>
#include <string>

#include <include/log/stream/log_stream.h>
#include <include/filter/filters_factory.h>

class LogStreamCreator
{
 public:
  

  /*!
   * Create a new instance of a concrete LogStream. If something
   * of wrong happens, it must return 0.
   */
  virtual LogStream* create (FiltersFactory* filtersFactory, 
                             string location, 
                             list<string>& filters,
                             u_long cycleLog) = 0;
};

#endif
