/*
MyServer
Copyright (C) 2002, 2003, 2004 The MyServer Team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef MEMORY_STREAM_H
#define MEMORY_STREAM_H

#include "../stdafx.h"
#include "../include/stream.h"
#include "../include/mem_buff.h"

class MemoryStream : public Stream
{
private:
  int internalData;
  MemBuf *data;
  int readSeek;
public:
  virtual int read(char* buffer, u_long len, u_long*);
  virtual int write(const char* buffer, u_long len, u_long*);
  virtual int flush(u_long*);
  int refresh();
  int availableToRead();
  int read(Stream*, u_long len, u_long *nbw);
  MemoryStream();
  MemoryStream(MemBuf*);
  virtual ~MemoryStream();
};


#endif
