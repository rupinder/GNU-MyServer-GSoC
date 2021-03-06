/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2009, 2010 Free Software Foundation,
  Inc.
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

#ifndef GZIP_H
# define GZIP_H
# include "myserver.h"
# include <include/filter/filter.h>

# ifdef HAVE_ZLIB
#  include <zlib.h>
# endif

# define GZIP_HEADER_LENGTH             10
# define GZIP_FOOTER_LENGTH             8
extern char GZIP_HEADER[];

class Gzip : public Filter
{

public:
  struct GzipData
  {
# ifdef HAVE_ZLIB
    z_stream stream;
    u_long crc;
    u_long data_size;
    u_long initialized;
# endif
  };
  static u_long compressBound (int size);

  Gzip ();
  ~Gzip ();

  static u_long headerSize ();
  static u_long footerSize ();
  u_long updateCRC (char* buffer, size_t size);
  u_long getFooter (char *str, size_t size);
  u_long initialize ();
  u_long compress (const char* in, size_t sizeIn,
                   char *out, size_t sizeOut);
  u_long free ();
  u_long flush (char *out, size_t sizeOut);
  u_long getHeader (char *buffer, size_t buffersize);

  static Filter* factory (const char *name);

  /*! From Filter.  */
  virtual int getHeader (char *buffer, size_t len, size_t *nbw);
  virtual int getFooter (char *buffer, size_t len, size_t *nbw);
  virtual int read (char* buffer, size_t len, size_t *);
  virtual int write (const char* buffer, size_t len, size_t *);
  virtual int flush (size_t *);
  virtual int modifyData ();
  virtual const char* getName ();
private:
  int active;
  GzipData data;
};

#endif
