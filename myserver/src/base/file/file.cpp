/*
MyServer
Copyright (C) 2002, 2003, 2004, 2005, 2008, 2009 Free Software Foundation, Inc.
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

#ifdef SENDFILE
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif


}
#endif

#include <string>
#include <sstream>

using namespace std;

const u_long File::READ = (1<<0);
const u_long File::WRITE = (1<<1);
const u_long File::TEMPORARY = (1<<2);
const u_long File::HIDDEN = (1<<3);
const u_long File::FILE_OPEN_ALWAYS = (1<<4);
const u_long File::OPEN_IF_EXISTS = (1<<5);
const u_long File::APPEND = (1<<6);
const u_long File::FILE_CREATE_ALWAYS = (1<<7);
const u_long File::NO_INHERIT = (1<<8);


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
  if (buffersize == 0)
  {
    *nbw = 0;
    return 1;
  }
#ifdef WIN32
  int ret = WriteFile (handle, buffer, buffersize, nbw, NULL);
  return (!ret);
#else
  *nbw =  ::write (handle, buffer, buffersize);
  return (*nbw == buffersize) ? 0 : 1 ;
#endif
}

/*!
 *Constructor for the class.
 *\param nfilename Filename to open.
 *\param opt Specify how open the file.
 */
File::File (char *nfilename, int opt)
  : handle (0)
{
  openFile (nfilename, opt);
}

/*!
 * Truncate the file.
 *\param size Specify the new file size.
 */
int File::truncate (u_long size)
{
#ifdef WIN32
  if (seek (size))
    return 1;

  return !SetEndOfFile (handle);
#else
  int err = ftruncate (handle, size);
  if (err)
    return err;

  return seek (size);
#endif
}

/*!
 *Open (or create if not exists) a file, but must explicitly use read and/or
 *write flags and open flag.
 *\param nfilename Filename to open.
 *\param opt Specify how open the file.
 *openFile returns 0 if the call was successful, any other value on errors.
 */
int File::openFile (const char* nfilename,u_long opt)
{
  long ret = 0;

  filename.assign (nfilename);
#ifdef WIN32
  u_long creationFlag = 0;
  u_long openFlag = 0;
  u_long attributeFlag = 0;
  SECURITY_ATTRIBUTES sa = {0};
  sa.nLength = sizeof (sa);

  if (opt & File::NO_INHERIT)
    sa.bInheritHandle = FALSE;
  else
    sa.bInheritHandle = TRUE;

  sa.lpSecurityDescriptor = NULL;

  if (opt & File::FILE_OPEN_ALWAYS)
    creationFlag |= OPEN_ALWAYS;
  if (opt & File::OPEN_IF_EXISTS)
    creationFlag |= OPEN_EXISTING;
  if (opt & File::FILE_CREATE_ALWAYS)
    creationFlag |= CREATE_ALWAYS;

  if (opt & File::READ)
    openFlag |= GENERIC_READ;
  if (opt & File::WRITE)
    openFlag |= GENERIC_WRITE;

  if (opt & File::TEMPORARY)
  {
    openFlag |= FILE_ATTRIBUTE_TEMPORARY;
    attributeFlag |= FILE_FLAG_DELETE_ON_CLOSE;
  }
  if (opt & File::HIDDEN)
    openFlag|= FILE_ATTRIBUTE_HIDDEN;

  if (attributeFlag == 0)
    attributeFlag = FILE_ATTRIBUTE_NORMAL;

  handle = CreateFile(filename.c_str (), openFlag,
                      FILE_SHARE_READ|FILE_SHARE_WRITE,
                      &sa, creationFlag, attributeFlag, NULL);

  if (handle == INVALID_HANDLE_VALUE)
    {
      filename.clear();
      return 1;
    }
  else
    {
      if (opt & File::APPEND)
        ret = seek (getFileSize ());
      else
        ret = seek (0);

      if (ret)
        {
          close();
          filename.clear();
          return 1;
        }
    }
#else
  struct stat fStats;
  int flags;

  if ((opt & File::READ) && (opt & File::WRITE))
    flags = O_RDWR;
  else if (opt & File::READ)
    flags = O_RDONLY;
  else if (opt & File::WRITE)
    flags = O_WRONLY;

  /* FIXME: how avoid a stat?  */
  bool exists = stat (filename.c_str (), &fStats) == 0;

  if (opt & File::OPEN_IF_EXISTS && !exists)
    return 1;

  if (exists && (opt & File::APPEND))
    flags |= O_APPEND;

  if (exists)
    handle = open (filename.c_str (), O_APPEND | flags);
  else
    handle = open (filename.c_str (), O_CREAT | flags, S_IRUSR | S_IWUSR);

  if (opt & File::FILE_CREATE_ALWAYS)
    truncate ();

  if (opt & File::TEMPORARY)
    unlink (filename.c_str ()); /* It will be removed on close.  */

  return handle < 0;
#endif

  return 0;
}

/*!
 *Returns the base/file/file.handle.
 */
Handle File::getHandle()
{
  return (Handle) handle;
}

/*!
 *Set the base/file/file.handle.
 *Return a non null-value on errors.
 *\param hl The new base/file/file.handle.
 */
int File::setHandle(Handle hl)
{
  handle = (FileHandle) hl;
  return 0;
}

/*!
 *define the operator =.
 *\param f The file to copy.
 */
int File::operator =(File f)
{
  setHandle (f.getHandle ());
  filename.assign (f.filename);
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
  return filename.c_str ();
}

/*!
 *Create a temporary file.
 *\param filename The new temporary file name.
 */
int File::createTemporaryFile(const char* filename)
{
  if (FilesUtility::fileExists(filename))
    FilesUtility::deleteFile(filename);

  return openFile(filename, File::READ |
                  File::WRITE|
                  File::FILE_CREATE_ALWAYS |
                  File::TEMPORARY |
                  File::NO_INHERIT);
}

/*!
 *Close an open base/file/file.handle.
 */
int File::close()
{
  int ret = 0;
  if (handle)
    {
#ifdef WIN32
      ret = !FlushFileBuffers (handle);
      ret |= !CloseHandle (handle);
#else
      ret = fsync (handle);
      ret |= ::close (handle);
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
  ret = GetFileSize(handle, NULL);
  if (ret != INVALID_FILE_SIZE)
  {
    return ret;
  }
  else
    return (u_long)-1;
#else
  struct stat fStats;
  ret = fstat (handle, &fStats);

  if (ret)
    return (u_long)(-1);
  else
    return fStats.st_size;
#endif
}

/*!
 *Change the position of the pointer to the file.
 *\param initialByte The new file pointer position.
 */
int File::seek (u_long initialByte)
{
  u_long ret;
#ifdef WIN32
  ret = SetFilePointer (handle, initialByte, NULL, FILE_BEGIN);
  return (ret == INVALID_SET_FILE_POINTER) ? 1 : 0;
#else
  ret = lseek (handle, initialByte, SEEK_SET);
  return (ret != initialByte ) ? 1 : 0;
#endif
}

/*!
 * Get the current file pointer position.
 *
 *\return The current file pointer position.
 */
u_long File::getSeek ()
{
#ifdef WIN32
  return SetFilePointer (handle, 0, NULL, FILE_CURRENT);
#else
  return lseek (handle, 0, SEEK_CUR);
#endif
}

/*!
 *Get the time of the last modifify did to the file.
 */
time_t File::getLastModTime ()
{
  return FilesUtility::getLastModTime (filename);
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
time_t File::getLastAccTime ()
{
  return FilesUtility::getLastAccTime (filename);
}

/*!
 *Inherited from Stream.
 */
int File::write (const char* buffer, u_long len, u_long *nbw)
{
  int ret = writeToFile (buffer, len, nbw );
  if (ret != 0)
    return -1;
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
int File::read (char* buffer, u_long buffersize, u_long* nbr)
{
#ifdef WIN32
  int ret = ReadFile((HANDLE)handle, buffer, buffersize, nbr, NULL);
  return (!ret);
#else
  int ret  = ::read((long)handle, buffer, buffersize);
  *nbr = (u_long)ret;
  return (ret == -1) ;
#endif
}

/*!
 * Copy the file directly to the socket.
 * Return 0 on success.
 *
 * \param dest Destination socket.
 * \param firstByte File offset.
 * \param buf Temporary buffer that can be used by this function.
 * \param nbw Number of bytes sent.
 */
int File::fastCopyToSocket (Socket *dest, u_long firstByte, MemBuf *buf, u_long *nbw)
{
  *nbw = 0;
#ifdef SENDFILE
  off_t offset = firstByte;
  size_t fileSize = getFileSize ();
  while (1)
    {
      int ret = sendfile (dest->getHandle (), getHandle (), &offset,
                          fileSize - offset);

      if (ret < 0)
        {
          /* Rollback to read/write on EINVAL or ENOSYS.  */
          if (errno == EINVAL || errno == ENOSYS)
            break;

          return -1;
        }

      *nbw += ret;

      if (fileSize == offset)
        return 0;
    }

  firstByte = offset;
#else
  char *buffer = buf->getBuffer ();
  u_long size = buf->getRealLength ();

	if (seek (firstByte))
    return 0;

  for (;;)
  {
    u_long nbr;
    u_long tmpNbw;

    if (read (buffer, size, &nbr))
      return -1;

    if (nbr == 0)
      break;

    if (dest->write (buffer, nbr, &tmpNbw))
      return -1;

    *nbw += tmpNbw;
  }

  return 0;
#endif
}
