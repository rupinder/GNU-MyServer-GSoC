/*
*MyServer
*Copyright (C) 2002 The MyServer Team
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
#ifdef __linux__
#include <string.h>
#include <errno.h>
#endif
}

#ifndef WIN32
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

u_long gzip::gzip_initialize(char* ,u_long ,char *,u_long ,gzip::gzip_data* data)
{
	if(data==0)
		data=&(this->data);
	long level = Z_DEFAULT_COMPRESSION;

	data->initialized=1;
	data->data_size=0;
	data->crc = crc32(0L, Z_NULL, 0);
	data->stream.zalloc = Z_NULL;
	data->stream.zfree = Z_NULL;
	data->stream.opaque = Z_NULL;
	data->stream.data_type=Z_BINARY;
	return deflateInit2(&(data->stream), level, Z_DEFLATED,-MAX_WBITS, MAX_MEM_LEVEL,Z_DEFAULT_STRATEGY);

};

/*!
*Compress the in buffer to the out buffer using the gzip compression.
*/
u_long gzip::gzip_compress(char* in,u_long sizeIN,char *out,u_long sizeOUT,gzip::gzip_data* data)
{
	if(data==0)
		data=&(this->data);
#ifndef DO_NOT_USE_GZIP	
	u_long old_total_out=data->stream.total_out;
	uLongf destLen=sizeIN;
	int ret;

#ifdef GZIP_CHECK_BOUNDS
	if(compressBound(sizeIN)>(u_long)sizeOUT)
		return 0;
#endif
	data->stream.data_type=Z_BINARY;
	data->stream.next_in = (Bytef*) in;
	data->stream.avail_in = sizeIN;
	data->stream.next_out = (Bytef*) out;
	data->stream.avail_out = destLen;
	ret = deflate(&(data->stream), Z_FULL_FLUSH);
	data->data_size+=data->stream.total_out-old_total_out;
	data->crc = crc32(data->crc, (const Bytef *) in, sizeIN);
	return data->stream.total_out-old_total_out;
#else 
	/*
	*If is specified DO_NOT_USE_GZIP copy the input buffer to the output one as it is.
	*/
	memcpy(out,in,min(sizeIN,sizeOUT));
	return min(sizeIN,sizeOUT);
#endif
}

/*!
*Close the gzip compression
*/
u_long gzip::gzip_free(char* ,u_long ,char *,u_long ,gzip::gzip_data* data)
{
	if(data==0)
		data=&(this->data);
#ifndef DO_NOT_USE_GZIP
	if(data->initialized=0)
		return 0;
	data->initialized=0;
	int ret;
	ret = deflateEnd(&(data->stream));
#endif
	return 0;
}
/*!
*Update the existent CRC
*/
u_long gzip::gzip_updateCRC(char* buffer,int size,gzip::gzip_data* data)
{
	if(data==0)
		data=&(this->data);
	data->crc = crc32(data->crc, (const Bytef *) buffer,(u_long)size);
	return data->crc;
}
/*!
*Get the GZIP footer.
*/
u_long gzip::gzip_getFOOTER(char *str,int size,gzip::gzip_data* data)
{
	if(data==0)
		data=&(this->data);
	char *footer =  str;
	footer[0] = (char) (data->crc) & 0xFF;
	footer[1] = (char) ((data->crc) >> 8) & 0xFF;
	footer[2] = (char) ((data->crc) >> 16) & 0xFF;
	footer[3] = (char) ((data->crc) >> 24) & 0xFF;
	footer[4] = (char) data->stream.total_in & 0xFF;
	footer[5] = (char) (data->stream.total_in >> 8) & 0xFF;
	footer[6] = (char) (data->stream.total_in >> 16) & 0xFF;
	footer[7] = (char) (data->stream.total_in >> 24) & 0xFF;
	footer[8] = '\0';
	return GZIP_FOOTER_LENGTH;
}