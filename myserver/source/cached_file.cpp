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
#include "../include/cached_file.h"

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

using namespace std;

/*!
 *Costructor of the class.
 */
CachedFile::CachedFile(CachedFileBuffer* cfb)
{
	File::File();
	buffer = cfb;
	fseek = 0;
	cfb->addRef();
}

/*!
 *Write data to a file is not supported by a CachedFile, return immediately -1.
 *Inherithed by File.
 *buffer is the pointer to the data to write
 *buffersize is the number of byte to write
 *nbw is a pointer to an unsigned long that receive the number of the
 *bytes written correctly.
 *\param buffer The buffer where write.
 *\param buffersize The length of the buffer in bytes.
 *\param nbw How many bytes were written to the file.
 */
int CachedFile::writeToFile(const char* buffer, u_long buffersize, u_long* nbw)
{
	return -1;
}

/*!
 *A CachedFile can't be opened directly, use a factory instead.
 *If the function have success the return value is nonzero.
 *\param nfilename Filename to open.
 *\agument opt Specify how open the file.
 *openFile returns 0 if the call was successfull, any other value on errors.
 */
int CachedFile::openFile(const char* nfilename,u_long opt)
{
	return -1;
}

/*!
 *Returns the file handle.
 */
FileHandle CachedFile::getHandle()
{
	return (FileHandle)-1;
}

/*!
 *Set the file handle.
 *Return a non null-value on errors.
 *\param hl The new file handle.
 */
int CachedFile::setHandle(FileHandle hl)
{
	return -1;
}

/*!
 *define the operator =.
 *\param f The file to copy.
 */
int CachedFile::operator =(CachedFile f)
{
  setHandle(f.getHandle());
  if(f.filename.length())
  {
    filename.assign(f.filename);
  }
  else
  {
    filename.clear();
    handle = 0;
  }
	fseek = f.fseek;
	return 0;
}

/*!
 *Read data from a file to a buffer.
 *Return 1 on errors.
 *Return 0 on success.
 *\param buffer The buffer where write.
 *\param buffersize The length of the buffer in bytes.
 *\param nbr How many bytes were read to the buffer.
 */
int CachedFile::readFromFile(char* buffer, u_long buffersize, u_long* nbr)
{
	u_long i;
	u_long toRead = std::min(buffersize, this->buffer->getFileSize() - fseek);
	const char* src = &(this->buffer->getBuffer()[fseek]);
	if(nbr)
		*nbr = toRead;

	i = toRead;

	while(i--)
		buffer[i] = src[i];

	fseek += toRead;

	return toRead == 0 ? 1 : 0;
}

/*!
 *A CachedFile can't be temporary.
 *Create a temporary file.
 *\param filename The new temporary file name.
 */
int CachedFile::createTemporaryFile(const char* filename)
{ 
	return -1;
}

/*!
 *Close an open file handle.
 */
int CachedFile::closeFile()
{
	buffer->decRef();
	return 0;
}

/*!
 *Returns the file size in bytes.
 *Returns -1 on errors.
 */
u_long CachedFile::getFileSize()
{
	return buffer->getFileSize();
}

/*!
 *Change the position of the pointer to the file.
 *\param initialByte The new file pointer position.
 */
int CachedFile::setFilePointer(u_long initialByte)
{
	if(initialByte <= buffer->getFileSize())
		fseek = initialByte;
	else
		return 0;
	return 0;
}

/*!
 *Inherited from Stream.
 */
int CachedFile::write(const char* buffer, u_long len, u_long *nbw)
{
	return -1;
}
