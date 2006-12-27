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
#include "../include/file.h"
#include "../include/files_utility.h"
#include "../include/cached_file_buffer.h"
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

#include <string>
#include <sstream>

/*!
 *Load a file in the buffer.
 *\param filename The name of the file.
 */
CachedFileBuffer::CachedFileBuffer(const char* filename)
{
	File file;
	u_long nbw;
	mutex.init();
	factoryToNotify = 0;
	refCounter = 0;
	this->filename.assign(filename);

	file.openFile(filename, File::MYSERVER_OPEN_IFEXISTS | 
								File::MYSERVER_OPEN_READ);

	fileSize = file.getFileSize();
	buffer = new char[fileSize];

	file.read(buffer, fileSize, &nbw);

	file.closeFile();
}

/*!
 *Increment the reference counter for this buffer.
 */
void CachedFileBuffer::addRef()
{
	mutex.lock();
	refCounter++;
	mutex.unlock();
}

/*!
 *Set the factory object to be notified when the object will have a reference
 *counter null.
 */
void CachedFileBuffer::setFactoryToNotify(CachedFileFactory *cff)
{
	factoryToNotify = cff;
}


/*!
 *Decrement the reference counter for this buffer.
 */
void CachedFileBuffer::decRef()
{
	bool isZero = false;
	mutex.lock();
	refCounter--;
	if(refCounter == 0)
		isZero = true;
	mutex.unlock();

	if(isZero && factoryToNotify)
		factoryToNotify->nullReferences(this);
}

/*!
 *Get the number of references to this object.
 */
u_long CachedFileBuffer::getReferenceCounter()
{
	u_long ret;
	mutex.lock();
	ret = refCounter;
	mutex.unlock();

	return ret;
}

/*!
 *Load a file in the buffer.
 *\param file The file object.
 */
CachedFileBuffer::CachedFileBuffer(File* file)
{
	u_long nbr;
	factoryToNotify = 0;
	refCounter = 0;
	fileSize = file->getFileSize();
	buffer = new char[fileSize];
	filename.assign(file->getFilename());
	file->setFilePointer(0);
	file->read(buffer, fileSize, &nbr);
}

/*!
 *Destroy the object.
 */
CachedFileBuffer::~CachedFileBuffer()
{
	mutex.destroy();
	delete [] buffer;
}
