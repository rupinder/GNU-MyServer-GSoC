/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2005, 2008, 2009, 2010 Free Software
  Foundation, Inc.
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
#include <include/base/exceptions/checked.h>

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

#ifdef HAVE_SYS_SENDFILE_H
# include <fcntl.h>
# include <stdlib.h>
# include <stdio.h>
# include <sys/sendfile.h>
# include <sys/stat.h>
# include <sys/types.h>
# include <unistd.h>
#endif

#include <string>
#include <sstream>
#include <memory>

using namespace std;

/*!
  Costructor of the class.
 */
File::File ()
{
  handle = (FileHandle) -1;
}

/*!
  D'tor.
 */
File::~File ()
{
  close ();
}

/*!
  Write data to a file.
  buffer is the pointer to the data to write
  buffersize is the number of byte to write
  nbw is a pointer to an unsigned long that receive the number of the
  bytes written correctly.
  \param buffer The buffer where write.
  \param buffersize The length of the buffer in bytes.
  \param nbw How many bytes were written to the file.
 */
int File::writeToFile (const char* buffer, size_t buffersize, size_t* nbw)
{
  int ret;
  if (buffersize == 0)
  {
    *nbw = 0;
    return -1;
  }

  ret = checked::write (handle, buffer, buffersize);
  if (ret < 0)
    return ret;

  *nbw = static_cast<size_t> (ret);
  return 0;
}

/*!
  Constructor for the class.
  \param nfilename Filename to open.
  \param opt Specify how open the file.
 */
File::File (char *nfilename, int opt)
  : handle ((FileHandle) -1)
{
  openFile (nfilename, opt);
}

/*!
  Truncate the file.
  \param size Specify the new file size.
 */
int File::truncate (u_long size)
{
  int err = ftruncate (handle, size);
  if (err)
    return err;

  return seek (size);
}

/*!
 Do a fstat on the file.
 \param fstat stat structure to fill.
*/
void File::fstat (struct stat *fstat)
{
  checked::fstat (handle, fstat);
}

/*!
  Open (or create if not exists) a file, but must explicitly use read and/or
  write flags and open flag.
  \param nfilename Filename to open.    If TEMPORARY or TEMPORARY_DELAYED is
  used, then this parameter specifies the mask to use for mkostemp(3).
  \param opt Specify how open the file.
  \param mask Creation mode when a new file is created.
  openFile returns 0 if the call was successful, any other value on errors.
 */
int File::openFile (const char* nfilename, u_long opt, mode_t mask)
{
  int flags;

  if ((opt & File::READ) && (opt & File::WRITE))
    flags = O_RDWR;
  else if (opt & File::READ)
    flags = O_RDONLY;
  else if (opt & File::WRITE)
    flags = O_WRONLY;

  if (opt & File::NO_FOLLOW_SYMLINK)
    flags |= O_NOFOLLOW;

  if (opt & File::APPEND)
    flags |= O_APPEND;


  if (opt & (File::TEMPORARY_DELAYED | File::TEMPORARY))
    {
      auto_ptr <char> templatefn (checked::strdup (nfilename));
      handle = gnulib::mkostemp (templatefn.get (), flags);
      if (handle < 0)
        checked::raiseException ();

      setFilename (templatefn.get ());

      if (opt & File::TEMPORARY)
        checked::unlink (getFilename ());
    }
  else
    {
      setFilename (nfilename);
      handle = gnulib::open (filename.c_str (), flags);
      if (handle < 0)
        {
          if (! ((errno == ENOENT) && (opt & File::FILE_OPEN_ALWAYS)))
            checked::raiseException ();

          flags |= O_CREAT;
          handle = checked::open (filename.c_str (), flags, S_IRUSR | S_IWUSR);
        }
    }

  this->opt = opt;
  return handle < 0;
}

/*!
  Returns the file handle.
 */
Handle File::getHandle ()
{
  return (Handle) handle;
}

/*!
  Set the base/file/file.handle.
  Return a non null-value on errors.
  \param hl The new base/file/file.handle.
 */
int File::setHandle (Handle hl)
{
  handle = (FileHandle) hl;
  return 0;
}

/*!
  define the operator =.
  \param f The file to copy.
 */
int File::operator =(File f)
{
  setHandle (f.getHandle ());
  filename.assign (f.filename);
  return 0;
}

/*!
  Set the name of the file
  Return Non-zero on errors.
  \param nfilename The new file name.
 */
int File::setFilename (const char* nfilename)
{
  filename.assign (nfilename);
  return 0;
}

/*!
  Returns the file path.
 */
const char *File::getFilename ()
{
  return filename.c_str ();
}

/*!
  Create a temporary file.
  \param filename The new temporary file name.
  \param unlink Unlink the inode immediately, not before close.
 */
int File::createTemporaryFile (const char* filename, bool unlink)
{
  u_long temporaryOpt = unlink ? File::TEMPORARY : File::TEMPORARY_DELAYED;

  return openFile (filename, File::READ | File::WRITE
                   | File::FILE_OPEN_ALWAYS | temporaryOpt);
}

/*!
  Close the file.
 */
int File::close ()
{
  int ret = 0;
  if (handle != -1)
    {
      if (opt & File::TEMPORARY_DELAYED)
        gnulib::unlink (filename.c_str ());

      ret |= checked::close (handle);
    }

  filename.clear ();
  handle = -1;

  return ret;
}

/*!
  Returns the file size in bytes.
  Returns -1 on errors.
 */
off_t File::getFileSize ()
{
  struct stat fStats;

  checked::fstat (handle, &fStats);

  return fStats.st_size;
}

/*!
  Change the position of the pointer to the file.
  \param initialByte The new file pointer position.
 */
int File::seek (off_t initialByte)
{
  u_long ret;
  ret = checked::checkError (gnulib::lseek (handle, initialByte, SEEK_SET));
  return (ret != initialByte ) ? 1 : 0;
}

/*!
  Get the current file pointer position.

  \return The current file pointer position.
 */
off_t File::getSeek ()
{
  off_t ret = gnulib::lseek (handle, 0, SEEK_CUR);
  if (ret < 0)
    checked::raiseException ();

  return ret;
}

/*!
  Get the time of the last modifify did to the file.
 */
time_t File::getLastModTime ()
{
  return FilesUtility::getLastModTime (filename);
}

/*!
  This function returns the creation time of the file.
 */
time_t File::getCreationTime ()
{
  return FilesUtility::getCreationTime (filename);
}

/*!
  Returns the time of the last access to the file.
 */
time_t File::getLastAccTime ()
{
  return FilesUtility::getLastAccTime (filename);
}

/*!
  Inherited from Stream.
 */
int File::write (const char* buffer, size_t len, size_t *nbw)
{
  return writeToFile (buffer, len, nbw );
}

/*!
  Read data from a file to a buffer.
  Return a negative value on errors.
  Return 0 on success.
  \param buffer The buffer where write.
  \param buffersize The length of the buffer in bytes.
  \param nbr How many bytes were read to the buffer.
 */
int File::read (char* buffer, size_t buffersize, size_t* nbr)
{
  int ret = ::read (handle, buffer, buffersize);
  if (ret < 0)
    return ret;

  *nbr = static_cast<size_t> (ret);
  return 0;
}

/*!
  Copy the file directly to the socket.
  Return 0 on success.

  \param dest Destination socket.
  \param firstByte File offset.
  \param buf Temporary buffer that can be used by this function.
  \param nbw Number of bytes sent.
 */
int File::fastCopyToSocket (Socket *dest, off_t firstByte, MemBuf *buf, size_t *nbw)
{
  *nbw = 0;
#ifdef HAVE_SYS_SENDFILE_H
  off_t offset = firstByte;
  off_t fileSize = getFileSize ();
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

      if (fileSize == (size_t) offset)
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
      size_t nbr;
      u_long tmpNbw;

      if (read (buffer, size, &nbr))
        return -1;

      if (nbr == 0)
        break;

      if (dest->write (buffer, nbr, &tmpNbw))
        return -1;

      *nbw += tmpNbw;
    }
#endif

  return 0;
}
