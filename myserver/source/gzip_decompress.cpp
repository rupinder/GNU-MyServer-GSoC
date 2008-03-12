/*
MyServer
Copyright (C) 2002, 2003, 2004, 2007 The MyServer Team
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

#include "../include/gzip_decompress.h"
#include "../include/securestr.h"

extern "C" {
#ifdef WIN32
#include <direct.h>
#include <errno.h>
#pragma comment (lib,"libz.lib")
#endif
#ifdef NOT_WIN
#include <string.h>
#include <errno.h>
#endif
}
#ifdef WIN32
#include <algorithm>
#endif
#include "../include/lfind.h"

/*!
 *Initialize the gzip structure value.
 */
u_long GzipDecompress::initialize()
{
#ifndef DO_NOT_USE_GZIP		

	data.initialized = 1;
	data.data_size = 0;
	data.stream.zalloc = Z_NULL;
	data.stream.zfree = Z_NULL;
	data.stream.opaque = Z_NULL;
	data.stream.avail_in = 0;
	data.stream.next_in = Z_NULL;

	return inflateInit2(&(data.stream), -MAX_WBITS);
#else 
	return 0;
#endif

}

/*!
 *Decompress the in buffer to the out buffer using the gzip compression.
 *\param in Buffer to decompress.
 *\param sizeIn The dimension of the buffer to decompress.
 *\param out Buffer where decompress.
 *\param sizeOut The dimension of the buffer where decompress.
 */
u_long GzipDecompress::decompress(const char* in, u_long sizeIn, 
																	char *out, u_long sizeOut)
{
#ifndef DO_NOT_USE_GZIP
	u_long old_total_out = data.stream.total_out;
	u_long ret;
	data.stream.data_type = Z_BINARY;
	data.stream.next_in = (Bytef*) in;
	data.stream.avail_in = sizeIn;
	data.stream.next_out = (Bytef*) out;
	data.stream.avail_out = sizeOut;

	ret = inflate(&(data.stream), Z_FULL_FLUSH);

	data.data_size += data.stream.total_out - old_total_out;
	return data.stream.total_out - old_total_out;
#else 
	/*!
   *If is specified DO_NOT_USE_GZIP copy the input buffer 
	 *to the output one as it is.
   */
	memcpy(out, in, std::min(sizeIn, sizeOut));
	return std::min(sizeIn, sizeOut);
#endif
}

/*!
 *Close the gzip compression.
 */
u_long GzipDecompress::free()
{
  u_long ret = 0;
#ifndef DO_NOT_USE_GZIP
	
	if(data.initialized == 0)
		return 0;
	data.initialized = 0;
	ret = inflateEnd(&(data.stream));
#endif
	return ret;
}

/*! 
 *Inherited from Filter.
 *\param buffer Buffer where write.
 *\param len Buffer length.
 *\param nbw Numbers of written bytes.
 */
int GzipDecompress::getHeader(char* buffer, u_long len, u_long* nbw)
{
  *nbw = getHeader(buffer, len);
  return !(*nbw);
}

/*! 
 *Inherited from Filter.
 *\param buffer Buffer where write.
 *\param len Buffer length.
 *\param nbw Numbers of written bytes.
 */
int GzipDecompress::getFooter(char* buffer, u_long len, u_long* nbw)
{
  if(len < GZIP_FOOTER_LENGTH)
    return -1;
  getFooter(buffer, GZIP_FOOTER_LENGTH);
  *nbw = GZIP_FOOTER_LENGTH;
  return 0;

}

/*!
 *The GzipDecompress filter modifies the data. 
 */
int GzipDecompress::modifyData()
{
  return 1;
}

/*!
 *Flush all the remaining data.
 *\param out Buffer where write.
 *\param sizeOut Buffer length.
 */
u_long GzipDecompress::flush(char *out, u_long sizeOut)
{
#ifndef DO_NOT_USE_GZIP	
	u_long old_total_out = data.stream.total_out;
	uLongf destLen = sizeOut;

	data.stream.data_type = Z_BINARY;
	data.stream.next_in = 0;
	data.stream.avail_in = 0;
	data.stream.next_out = (Bytef*) out;
	data.stream.avail_out = destLen;
	inflate(&(data.stream), Z_FINISH);

	data.data_size += data.stream.total_out - old_total_out;
	return data.stream.total_out - old_total_out;
#else 
	return 0;
#endif
}

/*! 
 *Constructor for the class. 
 */
GzipDecompress::GzipDecompress()
{
  active = 1;
  initialize();
}

/*!
 *Destructor for the class. 
 */
GzipDecompress::~GzipDecompress()
{
  free();
}

/*!
 *Get the GZIP footer.
 *\param str Buffer where write.
 *\param size Buffer length.
 */
u_long GzipDecompress::getFooter(char *str,int /*size*/)
{
#ifndef DO_NOT_USE_GZIP		
	return GZIP_FOOTER_LENGTH;
#else
	return 0;
#endif
}

/*!
 *Copy the GZIP header in the buffer.
 *\param buffer Buffer where write.
 *\param buffersize Buffer length.
 */
u_long GzipDecompress::getHeader(char *buffer,u_long buffersize)
{
	if(buffersize < GZIP_HEADER_LENGTH)
		return 0;
	memcpy(buffer, GZIP_HEADER, GZIP_HEADER_LENGTH);
	return GZIP_HEADER_LENGTH;
}

/*! 
 *Inherited from Filter.
 *This function uses an internal buffer slowing it. 
 *It is better to use directly the GzipDecompress::compress routine where possible.
 *\param buffer Buffer where write.
 *\param len Buffer length.
 *\param nbr Number of read bytes.
 */
int GzipDecompress::read(char* buffer, u_long len, u_long *nbr)
{
  char *tmp_buff;
  int ret;
  u_long nbr_parent;
  if(!parent)
    return -1;
  tmp_buff = new char[len/2];
  if(!tmp_buff)
    return -1; 
  
	if(!active)
    return parent->read(buffer, len, nbr);

  ret = parent->read(tmp_buff, len/2, &nbr_parent);

  if(ret == -1)
  {
    delete [] tmp_buff;
    return -1;
  }

  *nbr = decompress(tmp_buff, nbr_parent, buffer, len);

  delete [] tmp_buff;
  return 0;
}

/*! 
 *Inherited from Filter.
 *\param buffer Buffer where write.
 *\param len Buffer length.
 *\param nbw Number of written bytes.
 */
int GzipDecompress::write(const char* buffer, u_long len, u_long *nbw)
{
  char tmpBuffer[1024];
  u_long written = 0;
  *nbw = 0;

  /*! No stream to write to.  */
  if(!parent)
    return -1;

  if(!active)
    return parent->write(buffer, len, nbw);

  while(len)
  {
    u_long nbw_parent;
    u_long size = std::min(len, 512UL);
    u_long ret = decompress(buffer, size, tmpBuffer, 1024);

		if(ret)
      if(parent->write(tmpBuffer, ret, &nbw_parent) == -1 )
        return -1;

    written += ret;
    buffer += size;
    len -= size;
    *nbw += nbw_parent;
  }
  return 0;
}

/*! 
 *Inherited from Filter.
 *\param nbw Number of flushed bytes.
 */
int GzipDecompress::flush(u_long *nbw)
{
  char buffer[512];

  if(!active)
    return 0;

  *nbw = flush(buffer, 512);
  if(*nbw)
  {
    u_long nbwParent;
    u_long nbwParentFlush;
    if(!parent)
      return -1;
    if(parent->write(buffer, *nbw, &nbwParent) != 0)
      return -1;
    if(parent->flush(&nbwParentFlush) != 0)
      return -1;  
    *nbw = nbwParentFlush + nbwParent;
   }
  return 0;
}

/*!
 *Returns a new GzipDecompress object.
 *\param name Filter name.
 */
Filter* GzipDecompress::factory(const char* name)
{
  return new GzipDecompress();
}

/*!
 *Return a string with the filter name. 
 *If an external buffer is provided write the name there too.
 *\param name Buffer where write the filter name.
 *\param len Buffer size.
*/
const char* GzipDecompress::getName(char* name, u_long len)
{
  /*! No name by default. */
  if(name)
  {
    myserver_strlcpy(name, "gzip", len);
  }
  return "gzip";
}

/*!
 *Get the GZIP header size.
 */
u_long GzipDecompress::headerSize()
{
	return GZIP_HEADER_LENGTH;
}

/*!
 *Get the GZIP footer size.
 */
u_long GzipDecompress::footerSize()
{
	return GZIP_FOOTER_LENGTH;
}
