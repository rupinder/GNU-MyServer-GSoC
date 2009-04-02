/*
MyServer
Copyright (C) 2006, 2007, 2008, 2009 Free Software Foundation, Inc.
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


#include "stdafx.h"
#include <include/base/utility.h>
#include <include/base/string/stringutils.h>
#include <include/base/file/files_utility.h>
#include <include/base/files_cache/cached_file_factory.h>

#ifndef WIN32
extern "C" {
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
}
#endif

/*!
 *Default constructor.
 */
CachedFileFactory::CachedFileFactory()
{
  initialize(1 << 23);
}

/*!
 *Destructor.
 */
CachedFileFactory::~CachedFileFactory()
{
  clean();
}

/*!
 *Class constructor.
 *\param size Max size to use for the cache.
 */
CachedFileFactory::CachedFileFactory(u_long size)
{
  initialize(size);
}

/*!
 *Clean the used resources.
 */
void CachedFileFactory::clean()
{
  mutex.lock();

  HashMap<char*, CachedFileFactoryRecord*>::Iterator it = buffers.begin();
  for(; it != buffers.end(); it++)
  {
    delete (*it)->buffer;
    delete (*it);
  }
  buffers.clear();
  
  mutex.unlock();
   mutex.destroy();
}

/*!
 *Set the max dimension for a file in the cache.
 *\param maxSize The new value.
 */
void CachedFileFactory::setMaxSize(u_long maxSize)
{
  this->maxSize = maxSize;
}

/*!
 *Set the min dimension for a file in the cache.
 *\param minSize The new value.
 */
void CachedFileFactory::setMinSize(u_long minSize)
{
  this->minSize = minSize;
}

/*!
 *Get the max dimension for a file in the cache.
 *\return The current max value.
 */
u_long CachedFileFactory::getMaxSize()
{
  return maxSize;
}

/*!
 *Get the min dimension for a file in the cache.
 *\return The current min value.
 */
u_long CachedFileFactory::getMinSize()
{
  return minSize;
}

/*!
 *Initialize the structure.
 */
void CachedFileFactory::initialize(u_long size)
{
  setSize(size);
  used = 0;
  maxSize = 0;
  minSize = 0;
  usedSize = 0;
  created = getTicks();
  mutex.init();
}

/*!
 *Open a new file in read-only mode, if the file is present in the cache then
 *use the cache instead of a real file.
 *\param filename The file name.
 */
File* CachedFileFactory::open(const char* filename)
{
  CachedFileFactoryRecord *record;
  CachedFileBuffer *buffer;
  CachedFile* cachedFile;
  u_long ticks;

  mutex.lock();
  
  ticks = getTicks();
  record = buffers.get(filename);
  buffer = record ? record->buffer : 0;

  used++;

  /*!
   *If the file on the file system has a different mtime then don't use
   *the cache, in this way when opened instance of this file will be closed
   *the null reference callback can be called and the file reloaded.
   */
  if(record)
  {
    
    if(ticks - record->lastModTimeCheck > MYSERVER_SEC(5))
    {
      record->invalidCache = (FilesUtility::getLastModTime(filename) != 
                              record->mtime);
      record->lastModTimeCheck = ticks;
    }

    if(record->invalidCache)
    {
      File *file;
      mutex.unlock();
      
      file = new File();
      if(file->openFile(filename, File::MYSERVER_OPEN_IFEXISTS | 
                        File::MYSERVER_OPEN_READ))
      {
        delete file;
        return 0;
      }
      return file;
    }
  }

  if(buffer == 0)
  {
    u_long fileSize;
    File *file = new File();
    if(file->openFile(filename, File::MYSERVER_OPEN_IFEXISTS | 
                      File::MYSERVER_OPEN_READ))
    {
      mutex.unlock();
      delete file;
      return 0;
    }
    fileSize = file->getFileSize();

    if((minSize != 0 && fileSize < minSize) ||
       (maxSize != 0 && fileSize > maxSize)  ||
       (fileSize > size - usedSize + purgeRecords()))
    {
      mutex.unlock();
      return file;
    }
    else
    {
      record = new CachedFileFactoryRecord();

      if(record == 0)
      {
        delete record;
        file->close();
        delete file;
        mutex.unlock();
        return 0;
      }

      buffer = new CachedFileBuffer(file);
      record->mtime = file->getLastModTime();

      
      file->close();
      delete file;

      if(buffer == 0)
      {
        delete record;
        mutex.unlock();
        return 0;
      }
      buffer->setFactoryToNotify(this);
      record->created = ticks;
      record->buffer = buffer;

      buffers.put((char *)filename, record);
      usedSize += fileSize;
    }
  }
  record->used++;

  cachedFile = new CachedFile(buffer);

  mutex.unlock();
  
  return cachedFile;
}

/*!
 *Called by CachedFileBuffer when its counter reaches zero references.
 *\param cfb A pointer to the source CachedFileBuffer object.
 */
void CachedFileFactory::nullReferences(CachedFileBuffer* cfb)
{
  CachedFileFactoryRecord* record;
  float averageUsage;
  float bufferAverageUsage;
  float spaceUsage;
  u_long ticks;
  mutex.lock();

  ticks = getTicks();
  record = buffers.get(cfb->getFilename());
  if(record == 0)
  {
    mutex.unlock();
    return;
  }
  
  spaceUsage = (float)usedSize / size;
  averageUsage = (float)used * 1000.0f / (ticks - created);
  bufferAverageUsage = (float)record->used * 1000.0f / 
    (getTicks() - record->created);

  if(((spaceUsage > 0.65f) && (bufferAverageUsage < averageUsage) 
       && ((ticks - record->created) > 10000)) 
     || (spaceUsage > 0.9f)
     || record->invalidCache)
  {
    record = buffers.remove(cfb->getFilename());
    if(record)
      buffersToRemove.push_back(record);
  }
    
  mutex.unlock();
}

/*!
 *Remove pending records from the list.
 */
u_long CachedFileFactory::purgeRecords()
{
  list<CachedFileFactoryRecord*>::iterator it = buffersToRemove.begin();
  u_long ret = 0;

  while(it != buffersToRemove.end())
  {
    CachedFileFactoryRecord* rec = *it;
    if(!rec->buffer->getReferenceCounter())
    {
      ret += rec->buffer->getFileSize();
      it = buffersToRemove.erase(it);
      delete rec->buffer;
      delete rec;
    }
    else
      it++;
  }
  usedSize -= ret;
  return ret;
}
