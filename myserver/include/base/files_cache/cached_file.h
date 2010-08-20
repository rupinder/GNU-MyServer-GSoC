/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2006, 2008, 2009, 2010 Free Software Foundation, Inc.
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

#ifndef CACHED_FILE_H
# define CACHED_FILE_H

# include "myserver.h"
# include <include/filter/stream.h>
# include <include/base/file/file.h>
# include <include/base/files_cache/cached_file_buffer.h>
# include <string>

using namespace std;

class CachedFile : public File
{
public:
  CachedFile (CachedFileBuffer *buffer);
  virtual Handle getHandle ();
  virtual int setHandle (Handle);
  virtual int read (char *, size_t, size_t *);
  virtual int writeToFile (const char *, size_t, size_t *);
  virtual int createTemporaryFile (const char *, bool unlink = true);

  virtual int openFile (const char *, u_long, mode_t mask = 00700);
  virtual int openFile (string const &file, u_long opt, mode_t mask = 00700)
  {return openFile (file.c_str (), opt, mask);}

  virtual off_t getFileSize ();
  virtual int seek (off_t);

  using File::operator =;
  virtual int operator =(CachedFile);
  virtual int close ();

  virtual int fastCopyToSocket (Socket *dest, off_t offset,
                                MemBuf *buf, size_t *nbw);

  virtual int write (const char *buffer, size_t len, size_t *nbw);

protected:
  off_t fseek;
  CachedFileBuffer *buffer;
};
#endif
