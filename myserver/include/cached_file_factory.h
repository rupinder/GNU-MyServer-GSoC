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

#ifndef CACHED_FILE_FACTORY_H
#define CACHED_FILE_FACTORY_H

#include "../stdafx.h"
#include "../include/stream.h"
#include "../include/hash_map.h"
#include "../include/file.h"
#include "../include/mutex.h"
#include "../include/cached_file.h"
#include "../include/cached_file_buffer.h"
#include <string>

using namespace std;

class CachedFileFactory
{
protected:
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
	};

	HashMap<char*, CachedFileFactoryRecord*> buffers;
public:
	CachedFileFactory();
	~CachedFileFactory();
	CachedFileFactory(u_long m);
	void initialize(u_long size);
	void clean();
	void setSize(u_long m){size = m;}
	File *open(const char* file);
	void nullReferences(CachedFileBuffer* cfb);

	void setMaxSize(u_long maxSize);
	void setMinSize(u_long minSize);
	u_long getMaxSize();
	u_long getMinSize();

};

#endif
