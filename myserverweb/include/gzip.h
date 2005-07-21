/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef GZIP_H
#define GZIP_H
#include "../stdafx.h"
#include "../include/filter.h"

/*! If is defined DO_NOT_USE_GZIP don't use the zlib library. */
#ifndef DO_NOT_USE_GZIP	
/*! Include the ZLIB library header file. */
#include "zlib.h"		
#endif

#ifdef DO_NOT_USE_GZIP	
#define z_stream (void*)
#endif

#define GZIP_HEADER_LENGTH		10
#define GZIP_FOOTER_LENGTH		8
extern char GZIP_HEADER[];

class Gzip : public Filter
{
public:
	struct GzipData
	{
		z_stream stream;
		u_long crc;
		u_long data_size;
		u_long initialized;
	};

#ifdef GZIP_CHECK_BOUNDS	
	static u_long compressBound(int size);
#endif

  Gzip();
  ~Gzip();

	u_long updateCRC(char* buffer,int size);
	u_long getFooter(char *str,int size);
	u_long initialize();
	u_long compress(const char* in, u_long sizeIn, 
                  char *out,u_long sizeOut);
	u_long free();
	u_long flush(char *out,u_long sizeOut);
	u_long getHeader(char *buffer,u_long buffersize);

  static Filter* factory(const char* name);
 
  /*! From Filter*/
  virtual int getHeader(char* buffer, u_long len, u_long* nbw);
  virtual int getFooter(char* buffer, u_long len, u_long* nbw);
  virtual int read(char* buffer, u_long len, u_long*);
  virtual int write(const char* buffer, u_long len, u_long*);
	virtual int flush(u_long*);
	virtual int modifyData();
  virtual const char* getName(char* name, u_long len);
private:
	GzipData data;
};
#endif
