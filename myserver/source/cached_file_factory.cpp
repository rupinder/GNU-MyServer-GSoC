/*
MyServer
Copyright (C) 2006 The MyServer Team
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "../stdafx.h"
#include "../include/utility.h"
#include "../include/stringutils.h"
#include "../include/files_utility.h"
#include "../include/cached_file_factory.h"

#ifdef NOT_WIN
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
 *\param m Max size to use for the cache.
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
	created = getTicks();
	mutex.init();
}
/*!
 *Open a new file in read-only mode, if the file is present in the cache then
 *use the cache instead of a real file.
 *\param file The file name.
 */
File* CachedFileFactory::open(const char* filename)
{
	CachedFileFactoryRecord *record;
	CachedFileBuffer *buffer;

	mutex.lock();
	
	record = buffers.get(filename);
	buffer = record ? record->buffer : 0;

	used++;

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

		if((fileSize > size - usedSize) || 
			 (minSize != 0 && fileSize < minSize) ||
			 (maxSize != 0 && fileSize > maxSize) )
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
				file->closeFile();
				delete file;
				mutex.unlock();
				return 0;
			}

			buffer = new CachedFileBuffer(file);
			
			file->closeFile();
			delete file;

			if(buffer == 0)
			{
				delete record;
				mutex.unlock();
				return 0;
			}
			record->created = getTicks();
			record->buffer = buffer;
			record->mtime = file->getLastModTime();

			buffers.put((char *)filename, record);
			usedSize -= fileSize;
		}
	}
	record->used++;

	mutex.unlock();

	return new CachedFile(buffer);
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

	mutex.lock();

	record = buffers.get(cfb->getFilename());
	if(record == 0)
	{
		mutex.unlock();
		return;
	}
	
	spaceUsage = (float)usedSize / size;
	averageUsage = (float)used * 1000.0f / (getTicks() - created);
	bufferAverageUsage = (float)record->used * 1000.0f / 
		(getTicks() - record->created);

	if(((spaceUsage > 0.65f) && (bufferAverageUsage < averageUsage) 
			 && ((getTicks() - record->created) > 10000)) 
		 || (spaceUsage > 0.9f)
		 || (FilesUtility::getLastModTime(cfb->getFilename()) != record->mtime))
  {
		record = buffers.remove(cfb->getFilename());
		if(record)
		{
			delete record->buffer;
			delete record;
		}
	}
		
	mutex.unlock();
}
