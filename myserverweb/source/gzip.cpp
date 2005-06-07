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


#include "../include/gzip.h"

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

#ifdef NOT_WIN
#include "../include/lfind.h"

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

// Bloodshed Dev-C++ Helper
#ifndef intptr_t
#define intptr_t int
#endif


char GZIP_HEADER[]={(char)0x1f,(char)0x8b,Z_DEFLATED,0,0,0,0,0,0,0x03};


/*!
*Initialize the gzip structure value.
*/
int Gzip::initialize()
{
#ifndef DO_NOT_USE_GZIP		
	long level = Z_DEFAULT_COMPRESSION;

	data.initialized=1;
	data.data_size=0;
	data.crc = crc32(0L, Z_NULL, 0);
	data.stream.zalloc = Z_NULL;
	data.stream.zfree = Z_NULL;
	data.stream.opaque = Z_NULL;
	data.stream.data_type=Z_BINARY;
	return deflateInit2(&(data.stream), level, Z_DEFLATED,-MAX_WBITS, MAX_MEM_LEVEL,0);
#else 
	return 0;
#endif

};

#ifdef GZIP_CHECK_BOUNDS	
int Gzip::compressBound(int size)
{
#ifdef compressBound 
	return compressBound(size);
#else
	return 0;
#endif	
}
#endif

/*!
*Compress the in buffer to the out buffer using the gzip compression.
*/
int Gzip::compress(char* in,u_long sizeIn,char *out,u_long sizeOut)
{
#ifndef DO_NOT_USE_GZIP	
	u_long old_total_out=data.stream.total_out;
	uLongf destLen=sizeOut;
	int ret;

#ifdef GZIP_CHECK_BOUNDS
	if(compressBound(sizeIn)>(u_long)sizeOut)
		return 0;
#endif
	data.stream.data_type=Z_BINARY;
	data.stream.next_in = (Bytef*) in;
	data.stream.avail_in = sizeIn;
	data.stream.next_out = (Bytef*) out;
	data.stream.avail_out = destLen;
	ret = deflate(&(data.stream), Z_FULL_FLUSH);

	data.data_size+=data.stream.total_out-old_total_out;
	data.crc = crc32(data.crc, (const Bytef *) in, sizeIn);
	return static_cast<int>(data.stream.total_out-old_total_out);
#else 
	/*
	*If is specified DO_NOT_USE_GZIP copy the input buffer to the output one as it is.
	*/
	memcpy(out, in, min(sizeIn,sizeOut));
	return min(static_cast<int>(sizeIn), static_cast<int>(sizeOut));
#endif
}

/*!
 *Close the gzip compression.
 */
int Gzip::free()
{
int ret=0;
#ifndef DO_NOT_USE_GZIP
	
	if(data.initialized==0)
		return 0;
	data.initialized=0;
	ret = deflateEnd(&(data.stream));
#endif
	return ret;
}
/*!
*Flush all the data
*/
int Gzip::flush(char *out,u_long sizeOUT)
{
#ifndef DO_NOT_USE_GZIP	
	int ret;
	u_long old_total_out=data.stream.total_out;
	uLongf destLen=sizeOUT;

	data.stream.data_type=Z_BINARY;
	data.stream.next_in = 0;
	data.stream.avail_in = 0;
	data.stream.next_out = (Bytef*) out;
	data.stream.avail_out = destLen;
	ret = deflate(&(data.stream), Z_FINISH);

	data.data_size+=data.stream.total_out-old_total_out;
	return static_cast<int>(data.stream.total_out-old_total_out);
#else 
	return 0;
#endif
}

/*!
 *Update the existent CRC.
 */
int Gzip::updateCRC(char* buffer, int size)
{
#ifndef DO_NOT_USE_GZIP		
	data.crc = crc32(data.crc, (const Bytef *) buffer,(u_long)size);
	return data.crc;
#else
	return 0;
#endif
}

/*!
 *Get the GZIP footer.
 */
int Gzip::getFOOTER(char *str,int /*size*/)
{
#ifndef DO_NOT_USE_GZIP		
	char *footer =  str;
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
 *Copy the GZIP header in the buffer.
 */
int Gzip::getHEADER(char *buffer,u_long buffersize)
{
	if(buffersize<GZIP_HEADER_LENGTH)
		return 0;
	memcpy(buffer, GZIP_HEADER, GZIP_HEADER_LENGTH);
	return GZIP_HEADER_LENGTH;
}
