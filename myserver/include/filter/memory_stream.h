/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2009, 2010 Free Software Foundation,
  Inc.
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

#ifndef MEMORY_STREAM_H
# define MEMORY_STREAM_H

# include "myserver.h"
# include <include/filter/stream.h>
# include <include/base/mem_buff/mem_buff.h>

class MemoryStream : public Stream
{
public:
  virtual int read (char *buffer, size_t len, size_t *);
  virtual int write (const char *buffer, size_t len, size_t *);
  virtual int flush (size_t *);
  int refresh ();
  int availableToRead ();
  int read (Stream *, size_t len, size_t *nbw);
  MemoryStream ();
  MemoryStream (MemBuf *);
  virtual ~MemoryStream ();
private:
  int internalData;
  MemBuf *data;
  int readSeek;
};


#endif
