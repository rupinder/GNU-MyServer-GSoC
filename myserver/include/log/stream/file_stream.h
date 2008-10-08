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

#ifndef FILE_STREAM_H
#define FILE_STREAM_H

#include <list>
#include <sstream>
#include <string>

#include <include/base/file/file.h>
#include <include/base/file/files_utility.h>
#include <include/base/string/stringutils.h>
#include <include/base/utility.h>
#include <include/log/stream/log_stream.h>

class FileStream : public LogStream
{
public:
  FileStream (FiltersFactory*, u_long, Stream*, FiltersChain*);
  string makeNewFileName (string oldFileName);
  virtual u_long streamSize ();
  virtual int chown (int uid, int gid);
  int const static defaultFileMask;
protected:
  virtual int streamCycle ();
};

#endif
