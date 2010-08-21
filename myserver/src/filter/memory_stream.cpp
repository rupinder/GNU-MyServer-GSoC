/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2008, 2009, 2010 Free Software
  Foundation, Inc.
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

#include <include/filter/memory_stream.h>

#include <string>
#include <sstream>
using namespace std;

#include <string.h>

/*!
  Inherited from Stream.
 */
int MemoryStream::read (char* buffer, size_t len, size_t *nbr)
{
  char *b;
  *nbr = std::min (len, static_cast<size_t>(data->getLength () - readSeek));
  b = data->getBuffer () + readSeek;

  memcpy (buffer, b, *nbr);

  readSeek += (*nbr);

  return 0;
}

/*!
  Read directly on the stream.
 */
int MemoryStream::read (Stream* s, size_t len, size_t *nbr)
{
  size_t towrite = *nbr = std::min (len, static_cast<size_t>(data->getLength () - readSeek));
  int ret = s->write (data->getBuffer ()+readSeek, towrite, nbr);
  readSeek += towrite;
  return ret;
}

/*!
  Inherited from Stream.
 */
int MemoryStream::write (const char* buffer, size_t len, size_t *nbw)
{
  data->addBuffer (buffer, len);
  *nbw = len;
  return 0;
}

/*!
  Inherited from Stream.
 */
int MemoryStream::flush (size_t* nbw)
{
  *nbw = 0;
  return 0;
}

/*!
  Use an external buffer to store data.
 */
MemoryStream::MemoryStream (MemBuf* out)
{
  readSeek = 0;
  internalData = 0;
  data = out;
  data->setLength (0);
}

/*!
  Return how many bytes can be read.
 */
int MemoryStream::availableToRead ()
{
  return data->getLength ()-readSeek;
}

/*!
  Construct the object.
 */
MemoryStream::MemoryStream ()
{
  internalData = 1;
  readSeek = 0;
  data = new MemBuf ();
  data->setLength (0);
}

/*!
  Inherited from Stream.
 */
MemoryStream::~MemoryStream ()
{
  if (internalData)
    delete data;
}

/*!
  Recycle the buffer.
 */
int MemoryStream::refresh ()
{
  readSeek = 0;
  data->setLength (0);
  return 0;
}
