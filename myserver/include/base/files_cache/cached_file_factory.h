/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2006, 2007, 2008, 2009, 2010 Free Software Foundation,
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

#ifndef CACHED_FILE_FACTORY_H
# define CACHED_FILE_FACTORY_H

# include "stdafx.h"
# include <include/filter/stream.h>
# include <include/base/hash_map/hash_map.h>
# include <include/base/file/file.h>
# include <include/base/sync/mutex.h>
# include <include/base/files_cache/cached_file.h>
# include <include/base/files_cache/cached_file_buffer.h>
# include <string>
# include <list>

using namespace std;

class CachedFileFactory
{
public:
  CachedFileFactory ();
  ~CachedFileFactory ();
  CachedFileFactory (u_long m);
  void initialize (u_long size);
  void clean ();
  void setSize (u_long m){size = m;}
  u_long getSize (){return size;}
  u_long getUsedSize (){return usedSize;}
  u_long getUsed (){return used;}

  File *open (const char* file);
  void nullReferences (CachedFileBuffer* cfb);

  void setMaxSize (u_long maxSize);
  void setMinSize (u_long minSize);
  u_long getMaxSize ();
  u_long getMinSize ();

protected:
  u_long purgeRecords ();

  Mutex mutex;

  /*! Max elements count for this cache.  */
  u_long size;

  /*! Size currently used.  */
  u_long usedSize;

  /*! Number of times the cache was used.  */
  u_long used;

  /*! Cache creation time.  */
  u_long created;

  /*! Max size for single file.  */
  u_long maxSize;

  /*! Min size for single file.  */
  u_long minSize;

  struct CachedFileFactoryRecord
  {
    CachedFileBuffer* buffer;
    /*! Number of times the cache record was used.  */
    u_long used;

    /*! Cache record creation time.  */
    u_long created;

    /*! Last mtime for this file.  */
    time_t mtime;

    /*! Last time we checked for the mtime of the buffered file.  */
    u_long lastModTimeCheck;

    /*! This entry is not valid and will be removed when refCount = 0.  */
    bool invalidCache;
  };

  list<CachedFileFactoryRecord*> buffersToRemove;
  HashMap<char*, CachedFileFactoryRecord*> buffers;
};

#endif
