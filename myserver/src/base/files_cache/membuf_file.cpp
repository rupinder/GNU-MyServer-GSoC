/*
  MyServer
  Copyright (C) 2009, 2010 Free Software Foundation, Inc.
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


#include "myserver.h"
#include <include/base/utility.h>
#include <include/base/string/stringutils.h>
#include <include/base/file/files_utility.h>
#include <include/base/files_cache/membuf_file.h>

#ifndef WIN32
# include <fcntl.h>
# include <unistd.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <errno.h>
# include <stdio.h>
# include <fcntl.h>
# include <stdlib.h>
# include <string.h>
# include <math.h>
# include <time.h>

#endif

#include <string>
#include <sstream>

using namespace std;

MemBufFile::MemBufFile (MemBuf* buffer)
{
  this->buffer = buffer;
  fseek = 0;
}

/*!
  \see File#getHandle.
*/
Handle MemBufFile::getHandle ()
{
  return (Handle) -1;
}

/*!
  \see File#setHandle.
*/
int MemBufFile::setHandle (Handle)
{
  return -1;
}

/*!
  \see File#read.
*/
int MemBufFile::read (char *data, u_long len, u_long *nbr)
{
  const char *buf = buffer->getBuffer ();

  *nbr = min (len, buffer->getLength () - fseek);
  if (*nbr)
    memcpy (data, buf, *nbr);

  fseek += *nbr;

  return 0;
}

/*!
  \see File#writeToFile.
*/
int MemBufFile::writeToFile (const char *data, u_long len, u_long *nbw)
{
  u_long initialSize = buffer->getLength ();
  buffer->addBuffer (data, len);

  fseek += *nbw = buffer->getLength () - initialSize;
  return 0;
}

/*!
  \see File#createTemporaryFile.
*/
int MemBufFile::createTemporaryFile (const char*)
{
  return -1;
}


/*!
  \see File#openFile.
*/
int MemBufFile::openFile (const char*, u_long)
{
  return -1;
}

/*!
  \see File#getFileSize.
*/
u_long MemBufFile::getFileSize ()
{
  return buffer->getLength ();
}

/*!
  \see File#seek.
*/
int MemBufFile::seek (u_long newSeek)
{
  fseek = newSeek;
  return 0;
}

/*!
  \see File#close.
*/
int MemBufFile::close ()
{
  return 0;
}

/*!
  \see File#fastCopyToSocket.
*/
int MemBufFile::fastCopyToSocket (Socket *dest, u_long offset,
                                  MemBuf *buf, u_long *nbw)
{
  const char *data = buffer->getBuffer ();
  u_long toWrite = buffer->getLength () - offset;

  return dest->write (data, toWrite, nbw);
}

/*!
  \see File#write.
*/
int MemBufFile::write (const char* buffer, u_long len, u_long *nbw)
{
  return writeToFile (buffer, len, nbw);
}

/*!
  \see File#getSeek.
*/
u_long MemBufFile::getSeek ()
{
  return fseek;
}
