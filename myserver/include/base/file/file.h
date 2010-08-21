/* -*- mode: c++ -*- */
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

#ifndef FILE_H
# define FILE_H

# include "myserver.h"
# include <include/filter/stream.h>
# include <string>
# include <include/base/socket/socket.h>
# include <include/base/mem_buff/mem_buff.h>
# include <sys/stat.h>

using namespace std;

class File : public Stream
{
public:
  enum
    {
      READ = (1 << 0),
      WRITE = (1 << 1),
      TEMPORARY = (1 << 2),
      TEMPORARY_DELAYED = (1 << 3),
      HIDDEN = (1 << 4),
      FILE_OPEN_ALWAYS = (1 << 5),
      OPEN_IF_EXISTS = (1 << 6),
      APPEND = (1 << 7),
      NO_INHERIT = (1 << 8),
      NO_FOLLOW_SYMLINK = (1 << 9),
      NO_CACHE_STAT = (1 << 10)
    };

  File ();
  File (char *,int);
  virtual ~File ();

  virtual Handle getHandle ();
  virtual int setHandle (Handle);
  virtual int writeToFile (const char *, size_t, size_t *);
  virtual int createTemporaryFile (const char *, bool unlink = true);

  virtual int openFile (const char *, u_long, mode_t mask = 00700);
  virtual int openFile (string const &file, u_long opt, mode_t mask = 00700)
  {return openFile (file.c_str (), opt, mask);}

  virtual off_t getFileSize ();

  virtual int seek (off_t);
  virtual off_t getSeek ();

  virtual time_t getLastModTime ();
  virtual time_t getCreationTime ();
  virtual time_t getLastAccTime ();
  virtual const char *getFilename ();
  virtual int setFilename (const char*);
  virtual int setFilename (string const &name)
  {return setFilename (name.c_str ());}

  virtual int operator =(File);
  virtual int close ();

  /* Inherithed from Stream.  */
  virtual int read (char* buffer, size_t len, size_t *nbr);
  virtual int write (const char* buffer, size_t len, size_t *nbw);

  virtual int fastCopyToSocket (Socket *dest, off_t offset,
                                MemBuf *buf, size_t *nbw);

  int truncate (u_long size = 0);

  void fstat (struct stat *fstat);

  /*! Get the options mask used with openFile.  */
  u_long getOpenOptions (){return opt;}

protected:
  /* Options used by open.  */
  u_long opt;
  FileHandle handle;
  string filename;

  struct stat statS;
};
#endif
