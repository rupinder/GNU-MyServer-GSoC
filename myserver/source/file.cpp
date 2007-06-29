/*
MyServer
Copyright (C) 2002, 2003, 2004, 2005 The MyServer Team
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


#include "../stdafx.h"
#include "../include/utility.h"
#include "../include/stringutils.h"
#include "../include/files_utility.h"

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

const u_long File::MYSERVER_OPEN_READ = (1<<0);
const u_long File::MYSERVER_OPEN_WRITE = (1<<1);
const u_long File::MYSERVER_OPEN_TEMPORARY = (1<<2);
const u_long File::MYSERVER_OPEN_HIDDEN = (1<<3);
const u_long File::MYSERVER_OPEN_ALWAYS = (1<<4);
const u_long File::MYSERVER_OPEN_IFEXISTS = (1<<5);
const u_long File::MYSERVER_OPEN_APPEND = (1<<6);
const u_long File::MYSERVER_CREATE_ALWAYS = (1<<7);
const u_long File::MYSERVER_NO_INHERIT = (1<<8);


/*!
 *Costructor of the class.
 */
File::File()
{
	handle = 0;
}

/*!
 *Write data to a file.
 *buffer is the pointer to the data to write
 *buffersize is the number of byte to write
 *nbw is a pointer to an unsigned long that receive the number of the
 *bytes written correctly.
 *\param buffer The buffer where write.
 *\param buffersize The length of the buffer in bytes.
 *\param nbw How many bytes were written to the file.
 */
int File::writeToFile(const char* buffer, u_long buffersize, u_long* nbw)
{
	if(buffersize == 0)
	{
		*nbw = 0;
		return 1;
	}
#ifdef WIN32
	int ret = WriteFile((HANDLE)handle,buffer,buffersize,nbw,NULL);
	return (!ret);
#endif
#ifdef NOT_WIN
	*nbw =  ::write((long)handle, buffer, buffersize);
	return (*nbw == buffersize) ? 0 : 1 ;
#endif
}

/*!
 *Constructor for the class.
 *\param nfilename Filename to open.
 *\agument opt Specify how open the file.
 */
File::File(char *nfilename, int opt) 
  : handle(0)
{
  openFile(nfilename, opt);
}

/*!
 *Open(or create if not exists) a file.
 *If the function have success the return value is nonzero.
 *\param nfilename Filename to open.
 *\agument opt Specify how open the file.
 *openFile returns 0 if the call was successful, any other value on errors.
 */
int File::openFile(const char* nfilename,u_long opt)
{
	long ret = 0;

	filename.assign(nfilename);
#ifdef WIN32
	u_long creationFlag = 0;
	u_long openFlag = 0;
	u_long attributeFlag = 0;
	SECURITY_ATTRIBUTES sa = {0};
	sa.nLength = sizeof(sa);
	if(opt & File::MYSERVER_NO_INHERIT)
		sa.bInheritHandle = FALSE;
	else
		sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	if(opt & File::MYSERVER_OPEN_ALWAYS)
		creationFlag|=OPEN_ALWAYS;
	if(opt & File::MYSERVER_OPEN_IFEXISTS)
		creationFlag|=OPEN_EXISTING;
	if(opt & File::MYSERVER_CREATE_ALWAYS)
		creationFlag|=CREATE_ALWAYS;

	if(opt & File::MYSERVER_OPEN_READ)
		openFlag|=GENERIC_READ;
	if(opt & File::MYSERVER_OPEN_WRITE)
		openFlag|=GENERIC_WRITE;

	if(opt & File::MYSERVER_OPEN_TEMPORARY)
	{
		openFlag |= FILE_ATTRIBUTE_TEMPORARY; 
		attributeFlag |= FILE_FLAG_DELETE_ON_CLOSE;
	}
	if(opt & File::MYSERVER_OPEN_HIDDEN)
		openFlag|= FILE_ATTRIBUTE_HIDDEN;

	if(attributeFlag == 0)
		attributeFlag = FILE_ATTRIBUTE_NORMAL;

	handle = (FileHandle)CreateFile(filename.c_str(), openFlag, 
																	FILE_SHARE_READ|FILE_SHARE_WRITE, 
																	&sa, creationFlag, attributeFlag, NULL);

	/*! Return 1 if an error happens.  */
  if(handle == INVALID_HANDLE_VALUE)
  {
    filename.clear();
		return 1;
  }
	else/*! Open the file. */
	{
		if(opt & File::MYSERVER_OPEN_APPEND)
			ret = setFilePointer(getFileSize());
		else
			ret = setFilePointer(0);
  		if(ret)
      {
        closeFile();
        filename.clear();
        return 1;
      }
	}

#endif
#ifdef NOT_WIN
	struct stat F_Stats;
	int F_Flags;
	if(opt & File::MYSERVER_OPEN_READ && opt & File::MYSERVER_OPEN_WRITE)
		F_Flags = O_RDWR;
	else if(opt & File::MYSERVER_OPEN_READ)
		F_Flags = O_RDONLY;
	else if(opt & File::MYSERVER_OPEN_WRITE)
		F_Flags = O_WRONLY;
		
		
	if(opt & File::MYSERVER_OPEN_IFEXISTS)
	{
		ret = stat(filename.c_str(), &F_Stats);
		if(ret  < 0)
		{
      filename.clear();
			return 1;
		}
		ret = open(filename.c_str(), F_Flags);
		if(ret == -1)
    {
      filename.clear();
			return 1;
    }
		handle= (FileHandle)ret;
	}
	else if(opt & File::MYSERVER_OPEN_APPEND)
	{
		ret = stat(filename.c_str(), &F_Stats);
		if(ret < 0)
			ret = open(filename.c_str(),O_CREAT | F_Flags, S_IRUSR | S_IWUSR);
		else
			ret = open(filename.c_str(),O_APPEND | F_Flags);
		if(ret == -1)
     {
      filename.c_str();
			return 1;
    }
		else
			handle = (FileHandle)ret;
	}
	else if(opt & File::MYSERVER_CREATE_ALWAYS)
	{
		stat(filename.c_str(), &F_Stats);
		if(ret)
			remove(filename.c_str());

		ret = open(filename.c_str(),O_CREAT | F_Flags, S_IRUSR | S_IWUSR);
		if(ret == -1)
    {
      filename.clear();
			return 1;
    }
		else
			handle=(FileHandle)ret;
	}
	else if(opt & File::MYSERVER_OPEN_ALWAYS)
	{
		ret = stat(filename.c_str(), &F_Stats);

		if(ret < 0)
			ret = open(filename.c_str(), O_CREAT | F_Flags, S_IRUSR | S_IWUSR);
		else
			ret = open(filename.c_str(), F_Flags);

		if(ret == -1)
    {
      filename.clear();
			return 1;
    }
		else
			 handle = (FileHandle)ret;
	}
	
	if(opt & File::MYSERVER_OPEN_TEMPORARY)
		unlink(filename.c_str()); // Remove File on close
	
	if((long)handle < 0)
  {
		handle = (FileHandle)0;
    filename.clear();
  }
#endif
	
	return 0;
}

/*!
 *Returns the file handle.
 */
FileHandle File::getHandle()
{
	return handle;
}

/*!
 *Set the file handle.
 *Return a non null-value on errors.
 *\param hl The new file handle.
 */
int File::setHandle(FileHandle hl)
{
	handle = hl;
	return 0;
}

/*!
 *define the operator =.
 *\param f The file to copy.
 */
int File::operator =(File f)
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
	return 0;
}

/*!
 *Set the name of the file
 *Return Non-zero on errors.
 *\param nfilename The new file name.
 */
int File::setFilename(const char* nfilename)
{
	filename.assign(nfilename);
  return 0;
}

/*!
 *Returns the file path.
 */
const char *File::getFilename()
{
	return filename.c_str();
}

/*!
 *Read data from a file to a buffer.
 *Return 1 on errors.
 *Return 0 on success.
 *\param buffer The buffer where write.
 *\param buffersize The length of the buffer in bytes.
 *\param nbr How many bytes were read to the buffer.
 */
int File::readFromFile(char* buffer,u_long buffersize,u_long* nbr)
{
#ifdef WIN32
	int ret = ReadFile((HANDLE)handle, buffer, buffersize, nbr, NULL);
	return (!ret);
#endif
#ifdef NOT_WIN
	int ret  = ::read((long)handle, buffer, buffersize);
  *nbr = (u_long)ret;
	return (ret == -1) ;
#endif
}

/*!
 *Create a temporary file.
 *\param filename The new temporary file name.
 */
int File::createTemporaryFile(const char* filename)
{ 
  if(FilesUtility::fileExists(filename))
    FilesUtility::deleteFile(filename);

	return openFile(filename, File::MYSERVER_OPEN_READ | 
									File::MYSERVER_OPEN_WRITE| 
									File::MYSERVER_CREATE_ALWAYS | 
									File::MYSERVER_OPEN_TEMPORARY |
									File::MYSERVER_NO_INHERIT);
}

/*!
 *Close an open file handle.
 */
int File::closeFile()
{
	int ret = 0;
	if(handle)
  {
#ifdef WIN32
    ret = !FlushFileBuffers((HANDLE)handle);
    ret |= CloseHandle((HANDLE)handle);
#endif
#ifdef NOT_WIN
    ret = fsync((long)handle);
    ret |= close((long)handle);
#endif
	}
	filename.clear();
	handle = 0;
	return ret;
}

/*!
 *Returns the file size in bytes.
 *Returns -1 on errors.
 */
u_long File::getFileSize()
{
	u_long ret;
#ifdef WIN32
	ret = GetFileSize((HANDLE)handle,NULL);
	if(ret != INVALID_FILE_SIZE)
	{
		return ret;
	}
	else
		return (u_long)-1;
#endif
#ifdef NOT_WIN
	struct stat F_Stats;
	ret = fstat((long)handle, &F_Stats);
	if(ret)
		return (u_long)(-1);
	else
		return F_Stats.st_size;
#endif
}

/*!
 *Change the position of the pointer to the file.
 *\param initialByte The new file pointer position.
 */
int File::setFilePointer(u_long initialByte)
{
	u_long ret;
#ifdef WIN32
	ret=SetFilePointer((HANDLE)handle,initialByte,NULL,FILE_BEGIN);
  /*! SetFilePointer returns INVALID_SET_FILE_POINTER on an error.  */
	return (ret == INVALID_SET_FILE_POINTER) ? 1 : 0;
#endif
#ifdef NOT_WIN
	ret = lseek((long)handle, initialByte, SEEK_SET);
	return (ret!=initialByte)?1:0;
#endif
}

/*!
 *Get the time of the last modifify did to the file.
 */
time_t File::getLastModTime()
{
	return FilesUtility::getLastModTime(filename);
}

/*!
 *This function returns the creation time of the file.
 */
time_t File::getCreationTime()
{
	return FilesUtility::getCreationTime(filename);
}

/*!
 *Returns the time of the last access to the file.
 */
time_t File::getLastAccTime()
{
	return FilesUtility::getLastAccTime(filename);
}

/*!
 *Inherited from Stream.
 */
int File::read(char* buffer, u_long len, u_long *nbr)
{
	int ret = readFromFile(buffer, len, nbr );
  if(ret != 0)
    return -1;
  return 0;
}

/*!
 *Inherited from Stream.
 */
int File::write(const char* buffer, u_long len, u_long *nbw)
{
	int ret = writeToFile(buffer, len, nbw );
  if(ret != 0)
    return -1;
  return 0;
}
