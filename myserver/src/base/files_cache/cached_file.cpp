/*
  MyServer
  Copyright (C) 2006, 2008, 2009, 2010 Free Software Foundation, Inc.
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
#include <include/base/files_cache/cached_file.h>

#include <include/base/exceptions/checked.h>

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

#include <string>
#include <sstream>

using namespace std;

/*!
  Costructor of the class.
 */
CachedFile::CachedFile (CachedFileBuffer* cfb)
{
  handle = -1;
  buffer = cfb;
  fseek = 0;
  cfb->addRef ();
}

/*!
  Write data to a file is not supported by a CachedFile, return immediately -1.
  Inherithed by File.
  \see File#writeToFile.
 */
int CachedFile::writeToFile (const char* buffer, size_t buffersize, size_t* nbw)
{
  errno = EBADF;
  checked::raiseException ();
  return -1;
}

/*!
  A CachedFile can't be opened directly, use a factory instead.
  If the function have success the return value is nonzero.
  \see File#openFile
 */
int CachedFile::openFile (const char* nfilename, u_long opt, mode_t mask)
{
  errno = ENOENT;
  checked::raiseException ();
  return -1;
}

/*!
  Returns the base/file/file.handle.
 */
Handle CachedFile::getHandle ()
{
  return (Handle) -1;
}

/*!
  Set the base/file/file.handle.
  Return a non null-value on errors.
  \param hl The new base/file/file.handle.
 */
int CachedFile::setHandle (Handle hl)
{
  return -1;
}

/*!
  define the operator =.
  \param f The file to copy.
 */
int CachedFile::operator =(CachedFile f)
{
  setHandle (f.getHandle ());
  if (f.filename.length ())
    filename.assign (f.filename);
  else
    {
      filename.clear ();
      handle = 0;
    }
  fseek = f.fseek;
  return 0;
}

/*!
  Read data from a file to a buffer.
  Return 1 on errors.
  Return 0 on success.
  \param buffer The buffer where write.
  \param buffersize The length of the buffer in bytes.
  \param nbr How many bytes were read to the buffer.
 */
int CachedFile::read (char* buffer, size_t buffersize, size_t* nbr)
{
  size_t toRead = std::min (buffersize, this->buffer->getFileSize () - fseek);
  const char* src = &(this->buffer->getBuffer ()[fseek]);
  if (nbr)
    *nbr = toRead;

  memcpy (buffer, src, toRead);

  fseek += toRead;

  return toRead == 0 ? 1 : 0;
}

/*!
  A CachedFile can't be temporary.
  Create a temporary file.
  \see File#createTemporaryFile.
 */
int CachedFile::createTemporaryFile (const char *, bool)
{
  errno = EINVAL;
  checked::raiseException ();
  return -1;
}

/*!
  Close an open base/file/file.handle.
 */
int CachedFile::close ()
{
  buffer->decRef ();
  return 0;
}

/*!
  Returns the file size in bytes.
  Returns -1 on errors.
 */
size_t CachedFile::getFileSize ()
{
  return buffer->getFileSize ();
}

/*!
  Change the position of the pointer to the file.
  \param initialByte The new file pointer position.
 */
int CachedFile::seek (size_t initialByte)
{
  if (initialByte > buffer->getFileSize ())
    return -1;

  fseek = initialByte;
  return 0;
}

/*!
  Inherited from Stream.
 */
int CachedFile::write (const char* buffer, size_t len, size_t *nbw)
{
  errno = EBADF;
  checked::raiseException ();
  return -1;
}

/*!
  Copy the file directly to the socket.
  \param dest Destination socket.
  \param firstByte File offset.
  \param buf Temporary buffer that can be used by this function.
  \param nbw Number of bytes sent.
 */
int CachedFile::fastCopyToSocket (Socket *dest, size_t firstByte,
                                  MemBuf *buf, size_t *nbw)
{
  return dest->write (&(this->buffer->getBuffer ()[firstByte]),
                      buffer->getFileSize () - firstByte, nbw);
}
