/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
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
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

 
#include "../include/isapi.h"
#include "../include/memory_stream.h"

#include <string>
#include <sstream>
using namespace std;

/*!
 *Inherited from Stream.
 */
int MemoryStream::read(char* buffer,u_long len, u_long *nbr)
{
  char *b;
  u_long towrite;
  towrite = *nbr = std::min(len, static_cast<u_long>(data->GetLength() - readSeek));
  b=data->GetBuffer()+readSeek;
  while(towrite--)
  {
    *buffer++ = *b++;
  }
  readSeek+=(*nbr);
  return 0;
}

/*!
 *Read directly on the stream.
 */
int MemoryStream::read(Stream* s, u_long len, u_long *nbr)
{
  u_long towrite = *nbr = std::min(len, static_cast<u_long>(data->GetLength() - readSeek));  
  int ret = s->write(data->GetBuffer()+readSeek, towrite, nbr);
  readSeek+=towrite;
  return ret;
}

/*!
 *Inherited from Stream.
 */
int MemoryStream::write(char* buffer, u_long len, u_long *nbw)
{
  data->AddBuffer(buffer, len);
  *nbw=len;
  return 0;
}

/*!
 *Inherited from Stream.
 */
int MemoryStream::flush(u_long* nbw)
{
  *nbw=0;
  return 0;
}

/*!
 *Use an external buffer to store data.
 */
MemoryStream::MemoryStream(CMemBuf* out)
{
  readSeek=0;
  internalData=0;
  data=out;
  data->SetLength(0);
}

/*!
 *Return how many bytes can be read.
 */
int MemoryStream::availableToRead()
{
  return data->GetLength()-readSeek;
}

/*!
 *Construct the object.
 */
MemoryStream::MemoryStream()
{
  internalData=1;
  readSeek=0;
  data = new CMemBuf();
  data->SetLength(0);
}

/*!
 *Inherited from Stream.
 */
MemoryStream::~MemoryStream()
{
  if(internalData)
    delete data;
}

/*!
 *Recycle the buffer.
 */
int MemoryStream::refresh()
{
  readSeek=0;
  data->SetLength(0);
}
