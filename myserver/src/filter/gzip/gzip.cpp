/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2007, 2008, 2009, 2010 Free Software
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

#include <include/filter/gzip/gzip.h>
#include <include/base/string/securestr.h>

#ifdef WIN32
# include <direct.h>
# include <errno.h>
#endif
#ifndef WIN32
# include <string.h>
# include <errno.h>
#endif


#ifdef WIN32
# include <algorithm>
#endif

#ifdef HAVE_ZLIB
char GZIP_HEADER[] = {(char)0x1f, (char)0x8b, Z_DEFLATED,
                                 0, 0, 0, 0, 0, 0, 0x03};
#endif

/*!
  Initialize the gzip structure value.
 */
u_long Gzip::initialize ()
{
#ifdef HAVE_ZLIB
  long level = Z_DEFAULT_COMPRESSION;
  data.initialized = 1;
  data.data_size = 0;
  data.crc = crc32(0L, Z_NULL, 0);
  data.stream.zalloc = Z_NULL;
  data.stream.zfree = Z_NULL;
  data.stream.opaque = Z_NULL;
  data.stream.data_type = Z_BINARY;
  return deflateInit2 (&(data.stream), level, Z_DEFLATED, -MAX_WBITS, MAX_MEM_LEVEL, 0);
#else
  return 0;
#endif

}

u_long Gzip::compressBound (int size)
{
#ifdef GZIP_COMPRESS_BOUNDS
# ifdef GZIP_CHECK_BOUNDS
  return ::compressBound (size);
# endif
#else
  return -1;
#endif

  return 0;
}

/*!
  Compress the in buffer to the out buffer using the gzip compression.
  \param in Buffer to compress.
  \param sizeIn The dimension of the buffer to compress.
  \param out Buffer where compress.
  \param sizeOut The dimension of the buffer where compress.
 */
u_long Gzip::compress (const char* in, u_long sizeIn,
                       char *out, u_long sizeOut)
{
#ifdef HAVE_ZLIB
  u_long old_total_out = data.stream.total_out;
  u_long ret;

  if (compressBound (sizeIn) > (u_long)sizeOut)
    return 0;

  data.stream.data_type = Z_BINARY;
  data.stream.next_in = (Bytef*) in;
  data.stream.avail_in = sizeIn;
  data.stream.next_out = (Bytef*) out;
  data.stream.avail_out = sizeOut;
  ret = deflate (&(data.stream), Z_FULL_FLUSH);

  data.data_size += data.stream.total_out - old_total_out;
  data.crc = crc32(data.crc, (const Bytef *) in, sizeIn);
  return data.stream.total_out - old_total_out;
#else
  memcpy (out, in, std::min (sizeIn, sizeOut));
  return std::min (sizeIn, sizeOut);
#endif
}

/*!
  Close the gzip compression.
 */
u_long Gzip::free ()
{
  u_long ret = 0;
#ifdef HAVE_ZLIB
  if (!data.initialized)
    return 0;

  data.initialized = 0;
  ret = deflateEnd (&(data.stream));
#endif
  return ret;
}

/*!
  Inherited from Filter.
  \param buffer Buffer where write.
  \param len Buffer length.
  \param nbw Numbers of written bytes.
 */
int Gzip::getHeader (char* buffer, u_long len, u_long* nbw)
{
  *nbw = getHeader (buffer, len);
  return !(*nbw);
}

/*!
  Inherited from Filter.
  \param buffer Buffer where write.
  \param len Buffer length.
  \param nbw Numbers of written bytes.
 */
int Gzip::getFooter (char* buffer, u_long len, u_long* nbw)
{
  if (len < GZIP_FOOTER_LENGTH)
    return -1;
  getFooter (buffer, GZIP_FOOTER_LENGTH);
  *nbw = GZIP_FOOTER_LENGTH;
  return 0;

}

/*!
  The Gzip filter modifies the data.
 */
int Gzip::modifyData ()
{
  return 1;
}

/*!
  Flush all the remaining data.
  \param out Buffer where write.
  \param sizeOut Buffer length.
 */
u_long Gzip::flush (char *out, u_long sizeOut)
{
#ifdef HAVE_ZLIB
  u_long old_total_out = data.stream.total_out;
  uLongf destLen = sizeOut;

  data.stream.data_type = Z_BINARY;
  data.stream.next_in = 0;
  data.stream.avail_in = 0;
  data.stream.next_out = (Bytef*) out;
  data.stream.avail_out = destLen;
  deflate (&(data.stream), Z_FINISH);

  data.data_size += data.stream.total_out - old_total_out;
  return data.stream.total_out - old_total_out;
#else
  return 0;
#endif
}

/*!
  Constructor for the class.
 */
Gzip::Gzip ()
{
  active = 1;
  initialize ();
}

/*!
  Destructor for the class.
 */
Gzip::~Gzip ()
{
  free ();
}

/*!
  Update the existent CRC.
  \param buffer Buffer to look.
  \param size Number of bytes to look.
 */
u_long Gzip::updateCRC (char* buffer, int size)
{
#ifdef HAVE_ZLIB
  data.crc = crc32 (data.crc, (const Bytef *) buffer,
                    (u_long)size);
  return data.crc;
#else
  return 0;
#endif
}

/*!
  Get the GZIP footer.
  \param footer Buffer where write.
  \param size Buffer length.
 */
u_long Gzip::getFooter (char *footer, int /*size*/)
{
#ifdef HAVE_ZLIB
  footer[0] = (char) (data.crc) & 0xFF;
  footer[1] = (char) ((data.crc) >> 8) & 0xFF;
  footer[2] = (char) ((data.crc) >> 16) & 0xFF;
  footer[3] = (char) ((data.crc) >> 24) & 0xFF;
  footer[4] = (char) data.stream.total_in & 0xFF;
  footer[5] = (char) (data.stream.total_in >> 8) & 0xFF;
  footer[6] = (char) (data.stream.total_in >> 16) & 0xFF;
  footer[7] = (char) (data.stream.total_in >> 24) & 0xFF;
  footer[8] = '\0';
  return GZIP_FOOTER_LENGTH;
#else
  return 0;
#endif
}

/*!
  Copy the GZIP header in the buffer.
  \param buffer Buffer where write.
  \param buffersize Buffer length.
 */
u_long Gzip::getHeader (char *buffer, u_long buffersize)
{
#if HAVE_ZLIB
  if (buffersize < GZIP_HEADER_LENGTH)
    return 0;
  memcpy (buffer, GZIP_HEADER, GZIP_HEADER_LENGTH);
  return GZIP_HEADER_LENGTH;
#else
  return 0;
#endif
}

/*!
  Inherited from Filter.
  This function uses an internal buffer slowing it.
  It is better to use directly the Gzip::compress routine where possible.
  \param buffer Buffer where write.
  \param len Buffer length.
  \param nbr Number of read bytes.
 */
int Gzip::read (char* buffer, u_long len, u_long *nbr)
{
  char *tmp_buff;
  u_long nbr_parent;
  if (!parent)
    return -1;

  if (!active)
    return parent->read (buffer, len, nbr);

  tmp_buff = new char[len/2];

  try
    {
      parent->read (tmp_buff, len/2, &nbr_parent);
      *nbr = compress (tmp_buff, nbr_parent, buffer, len);
    }
  catch (...)
    {
      delete [] tmp_buff;
      throw;
    }

  delete [] tmp_buff;
  return 0;
}

/*!
  Inherited from Filter.
  \param buffer Buffer where write.
  \param len Buffer length.
  \param nbw Number of written bytes.
 */
int Gzip::write (const char* buffer, u_long len, u_long *nbw)
{
  char tmpBuffer[1024];
  u_long written = 0;
  *nbw = 0;

  /* No stream to write to.  */
  if (!parent)
    return -1;

  if (!active)
    return parent->write (buffer, len, nbw);

  while (len)
    {
      u_long nbw_parent;
      u_long size=std::min (len, 512UL);
      u_long ret = compress (buffer, size, tmpBuffer, 1024);

      if (ret)
        parent->write (tmpBuffer, ret, &nbw_parent);

      written += ret;
      buffer += size;
      len -= size;
      *nbw += nbw_parent;
    }
  return 0;
}

/*!
  Inherited from Filter.
  \param nbw Number of flushed bytes.
 */
int Gzip::flush (u_long *nbw)
{
  char buffer[512];

  if (!active)
    return 0;

  *nbw = flush (buffer, 512);
  if (*nbw)
    {
      u_long nbwParent;
      u_long nbwParentFlush;
      if (!parent)
        return -1;

      parent->write (buffer, *nbw, &nbwParent);
      parent->flush (&nbwParentFlush);
      *nbw = nbwParentFlush + nbwParent;
    }
  return 0;
}

/*!
  Returns a new Gzip object.
  \param name Filter name.
 */
Filter* Gzip::factory (const char* name)
{
  return new Gzip ();
}

/*!
  Return a string with the filter name.
 */
const char* Gzip::getName ()
{
  return "gzip";
}

/*!
  Get the GZIP header size.
 */
u_long Gzip::headerSize ()
{
  return GZIP_HEADER_LENGTH;
}

/*!
  Get the GZIP footer size.
 */
u_long Gzip::footerSize ()
{
  return GZIP_FOOTER_LENGTH;
}
