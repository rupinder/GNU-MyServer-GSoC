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


#include "myserver.h"
#include <include/base/utility.h>
#include <include/base/string/stringutils.h>
#include <include/base/file/files_utility.h>
#include <include/base/files_cache/cached_file_factory.h>

#ifndef WIN32
# include <fcntl.h>
# include <unistd.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <errno.h>
# include <stdio.h>
# include <fcntl.h>
# include <stdlib.h>
# include <string.h>
# include <math.h>
# include <time.h>

#endif

/*!
  Default constructor.
 */
CachedFileFactory::CachedFileFactory ()
{
  initialize (1 << 23);
}

/*!
  Destructor.
 */
CachedFileFactory::~CachedFileFactory ()
{
  clean ();
}

/*!
  Class constructor.
  \param size Max size to use for the cache.
 */
CachedFileFactory::CachedFileFactory (u_long size)
{
  initialize (size);
}

/*!
  Clean the usedm resources.
 */
void CachedFileFactory::clean ()
{
  HashMap<char *, CachedFileFactoryRecord*>::Iterator it = buffers.begin ();
  for (; it != buffers.end (); it++)
    {
      delete (*it)->buffer;
      delete (*it);
    }
  buffers.clear ();
  mutex.destroy ();
}

/*!
  Set the max dimension for a file in the cache.
  \param maxSize The new value.
 */
void CachedFileFactory::setMaxSize (u_long maxSize)
{
  this->maxSize = maxSize;
}

/*!
  Set the min dimension for a file in the cache.
  \param minSize The new value.
 */
void CachedFileFactory::setMinSize (u_long minSize)
{
  this->minSize = minSize;
}

/*!
  Get the max dimension for a file in the cache.
  \return The current max value.
 */
u_long CachedFileFactory::getMaxSize ()
{
  return maxSize;
}

/*!
  Get the min dimension for a file in the cache.
  \return The current min value.
 */
u_long CachedFileFactory::getMinSize ()
{
  return minSize;
}

/*!
  Initialize the structure.
 */
void CachedFileFactory::initialize (u_long size)
{
  setSize (size);
  maxSize = 0;
  minSize = 0;
  usedSize = 0;
  created = getTicks ();
  mutex.init ();
}

/*!
  Open a new file in read-only mode, if the file is present in the cache then
  use the cache instead of a real file.
  \param filename The file name.
  \param flags Additional flags, actually only File::NO_FOLLOW_SYMLINK is
  supported.
*/
File* CachedFileFactory::open (const char* filename, int flags)
{
  CachedFileFactoryRecord *record;
  CachedFileBuffer *buffer;
  CachedFile* cachedFile;
  u_long ticks = getTicks ();

  mutex.lock ();
  try
    {
      record = buffers.get (filename);
      buffer = record ? record->buffer : NULL;

      if (record)
        {
          if (buffer->getExpireTime () <= ticks)
            {
              buffer->refreshExpireTime (ticks);
              record->invalidCache = ! buffer->revalidate (filename);
            }

          bool noSymlink = (! (flags & File::NO_FOLLOW_SYMLINK))
            && buffer->isSymlink ();

          if (record->invalidCache || noSymlink)
            {
              mutex.unlock ();

              File *file = new File ();
              flags = flags & File::NO_FOLLOW_SYMLINK;
              if (file->openFile (filename, File::OPEN_IF_EXISTS | flags
                                  | File::READ))
                {
                  delete file;
                  return NULL;
                }
              return file;
            }
        }

      if (buffer == NULL)
        {
          u_long fileSize;
          File *file = new File ();
          flags = flags & File::NO_FOLLOW_SYMLINK;
          if (file->openFile (filename, File::OPEN_IF_EXISTS | flags
                              | File::READ))
            {
              mutex.unlock ();
              delete file;
              return NULL;
            }

          fileSize = file->getFileSize ();
          purgeRecords ();
          if (minSize && fileSize < minSize
              || maxSize && fileSize > maxSize
              || fileSize > size - usedSize)
            {
              mutex.unlock ();
              return file;
            }
          else
            {
              record = new CachedFileFactoryRecord ();
              buffer = new CachedFileBuffer (file);
              file->close  ();
              delete file;

              buffer->setFactoryToNotify (this);
              record->created = ticks;
              record->buffer = buffer;
              buffer->refreshExpireTime (ticks);
              buffers.put ((char *) filename, record);
              usedSize += fileSize;
            }
        }
      record->used++;

      cachedFile = new CachedFile (buffer);

      mutex.unlock ();
    }
  catch (...)
    {
      mutex.unlock ();
      throw;
    }

  return cachedFile;
}

/*!
  Called by CachedFileBuffer when its counter reaches zero references.
  \param cfb A pointer to the source CachedFileBuffer object.
 */
void CachedFileFactory::nullReferences (CachedFileBuffer* cfb)
{
  CachedFileFactoryRecord* record;
  float bufferAverageUsage;
  float spaceUsage;
  u_long ticks = getTicks ();

  spaceUsage = (float) usedSize / size;
  if (spaceUsage < 0.65f)
    return;

  mutex.lock ();
  try
    {
      record = buffers.get (cfb->getFilename ());
      if (! record)
        {
          mutex.unlock ();
          return;
        }

      bufferAverageUsage = (float)record->used * 1000.0f /
        (ticks - record->created);

      if ((spaceUsage > 0.65f && (ticks - record->created) > 10000)
          || spaceUsage > 0.9f
          || record->invalidCache)
        {
          record = buffers.remove (cfb->getFilename ());
          if (record)
            buffersToRemove.push_back (record);
        }
      mutex.unlock ();
    }
  catch (...)
    {
      mutex.unlock ();
      throw;
    }
}

/*!
  Remove pending records from the list.
 */
u_long CachedFileFactory::purgeRecords ()
{
  list<CachedFileFactoryRecord *>::iterator it = buffersToRemove.begin ();
  u_long ret = 0;

  while (it != buffersToRemove.end ())
    {
      CachedFileFactoryRecord* rec = *it;
      if (rec->buffer->getReferenceCounter ())
        it++;
      else
        {
          ret += rec->buffer->getFileSize ();
          it = buffersToRemove.erase (it);
          delete rec->buffer;
          delete rec;
        }
    }
  usedSize -= ret;
  return ret;
}
