/*
MyServer
Copyright (C) 2006, 2007 The MyServer Team
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

#ifndef CACHED_FILE_BUFFER_H
#define CACHED_FILE_BUFFER_H

#include "../stdafx.h"
#include "../include/stream.h"
#include "../include/file.h"
#include "../include/mutex.h"
#include <string>

using namespace std;

class CachedFileFactory;

class CachedFileBuffer
{
public:
	void addRef();
	void decRef();
	void setFactoryToNotify(CachedFileFactory *cff);
	u_long getReferenceCounter();
	u_long getFileSize(){return fileSize;}
	CachedFileBuffer(const char* filename);
	CachedFileBuffer(File* file);
	~CachedFileBuffer();
	const char* getFilename(){return filename.c_str();}
	const char* getBuffer(){return buffer;}
protected:
	Mutex mutex;
	char *buffer;
	u_long refCounter;
	u_long fileSize;
	CachedFileFactory *factoryToNotify;
	string filename;
};
#endif
