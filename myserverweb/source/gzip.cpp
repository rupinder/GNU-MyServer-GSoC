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

#define GZIP_HEADER_LENGTH		10
#define GZIP_FOOTER_LENGTH		8

#ifndef DO_NOT_USE_GZIP	/*If is defined DO_NOT_USE_GZIP don't use the zlib library */
#include "zlib.h"		/*Include the ZLIB library header file*/
#endif

/*!
*Compress the in buffer to the out buffer using the gzip compression.
*/
u_long gzip_compress(char* in,u_long sizeIN,char *out,u_long sizeOUT)
{
#ifndef DO_NOT_USE_GZIP	

	uLongf destLen=sizeIN-GZIP_HEADER_LENGTH-GZIP_FOOTER_LENGTH;
	long level = Z_DEFAULT_COMPRESSION;
	z_stream stream;
	int ret;
	if(compressBound(sizeIN)>(u_long)sizeOUT)
		return 0;
	out[0] = (char)0x1f;/*GZIP HEADER MAGIC BYTE 1*/
	out[1] = (char)0x8b;/*GZIP HEADER MAGIC BYTE 2*/
	out[2] = Z_DEFLATED;
	out[3] = out[4] = out[5] = out[6] = out[7] = out[8] = 0;
	out[9] = 0x03;

	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;

	stream.next_in = (Bytef*) in;
	stream.avail_in = sizeIN;
	
	stream.avail_out = destLen;
	stream.next_out = (Bytef*) &out[GZIP_HEADER_LENGTH];
	ret = deflateInit2(&stream, level, Z_DEFLATED,	-MAX_WBITS, MAX_MEM_LEVEL,Z_DEFAULT_STRATEGY);
	if (ret != Z_OK)
		return 0;

	ret = deflate(&stream, Z_FINISH);
	if (ret != Z_STREAM_END) 
	{
		deflateEnd(&stream);
		if (ret == Z_OK) 
		{
			ret = Z_BUF_ERROR;
		}
	}
	else 
	{
		ret = deflateEnd(&stream);
	}

	if (ret==Z_OK) 
	{
		u_long crc = crc32(0L, Z_NULL, 0);
		crc = crc32(crc, (const Bytef *) in,(u_long)sizeIN);

		char *footer =  &out[stream.total_out+GZIP_HEADER_LENGTH];
		footer[0] = (char) crc & 0xFF;
		footer[1] = (char) (crc >> 8) & 0xFF;
		footer[2] = (char) (crc >> 16) & 0xFF;
		footer[3] = (char) (crc >> 24) & 0xFF;
		footer[4] = (char) stream.total_in & 0xFF;
		footer[5] = (char) (stream.total_in >> 8) & 0xFF;
		footer[6] = (char) (stream.total_in >> 16) & 0xFF;
		footer[7] = (char) (stream.total_in >> 24) & 0xFF;
		footer[8] = '\0';

		destLen=stream.total_out+GZIP_HEADER_LENGTH+GZIP_FOOTER_LENGTH;
	}
	/*compress((Bytef*)out,&destLen,(Bytef*)in,sizeIN);*/
	return destLen;
#else 
	/*
	*If is specified DO_NOT_USE_GZIP copy the input buffer to the output one as it is.
	*/
	memcpy(out,in,min(sizeIN,sizeOUT));
	return min(sizeIN,sizeOUT);
#endif
}
/*!
*Returns the size of the needed buffer that will contain the compressed data
*/
u_long gzip_bound(u_long size)
{
#ifndef DO_NOT_USE_GZIP	
	return compressBound(size);
#else
	return 0;
#endif
}
