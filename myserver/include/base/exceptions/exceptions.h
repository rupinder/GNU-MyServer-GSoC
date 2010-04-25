/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2008, 2009, 2010 Free Software
  Foundation, Inc.
  Copyright (C) 2010, Lisa Vitolo (shainer)
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
#ifndef EXCEPTIONS_H
# define EXCEPTIONS_H

# include "myserver.h"

# include <exception>
# include <errno.h>
# include <execinfo.h>
# include <stdlib.h>
# include <string.h>

# include <include/log/stream/log_stream.h>

using namespace std;

/*!
 * This class describes an abstract MyServer exception
 */
class AbstractServerException: public exception
{
  public:
    /*!
     * Constructor of the class
     */
    AbstractServerException ()
    {
      localErrno = errno;
      buffer = NULL;
      btString = NULL;
    }

    int getErrno ()
    {
      return localErrno;
    }

    /*!
     * Returns the process backtrace of the application
     * at the moment of the error
     */
    char **getBacktrace ()
    {
      if (localErrno == ENOMEM) /* it's not safe to allocate more */
        return NULL;

      if (buffer == NULL)
        {
# ifdef HAVE_BACKTRACE
          size = 20;
          buffer = (void **) gnulib::malloc (sizeof(void *) *size);
          int w = backtrace (buffer, size);
# endif

# ifdef HAVE_BACKTRACE_SYMBOLS
          btString = (char **) gnulib::malloc (sizeof(char *) *w);
          btString = backtrace_symbols (buffer, w);
# endif
        }

      return btString;
    }

    /*!
     * Returns a string representing the error
     */
    virtual const char *what () const throw ()
    {
      return gnulib::strerror (localErrno);
    }

    enum LoggingLevel getLogLevel()
    {
      return logLevel;
    }

    virtual ~AbstractServerException () throw ()
      {
        if (buffer)
          free (buffer);

        if (btString)
          free (btString);
      }

 protected:
    int localErrno;
    char **btString;

    void setLogLevel (enum LoggingLevel l)
    {
      logLevel = l;
    }

 private:
    void **buffer;
    int size;
    enum LoggingLevel logLevel;
};

/*
 * Generic categories to group together more exceptions of the same type
 */
class GenericFileException : public AbstractServerException
{
 public:
  GenericFileException () : AbstractServerException () {}
};

class GenericSocketException : public AbstractServerException
{
 public:
  GenericSocketException () : AbstractServerException () {}
};

class GenericMemoryException : public AbstractServerException
{
 public:
  GenericMemoryException () : AbstractServerException () {}
};

/*
 * Exceptions
 */
class AbortedConnectionException : public GenericSocketException
{
 public:
  AbortedConnectionException () : GenericSocketException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_INFO);
    }
};

class AddressException : public GenericSocketException
{
 public:
  AddressException () : GenericSocketException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_ERROR);
    }
};

class ArgumentListException : public GenericFileException
{
 public:
  ArgumentListException () : GenericFileException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_WARNING);
    }
};

class BadDescriptorException : public GenericSocketException
{
 public:
  BadDescriptorException () : GenericSocketException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_WARNING);
    }
};

class BlockingException : public GenericFileException
{
 public:
  BlockingException () : GenericFileException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_WARNING);
    }
};

class BufferOverflowException : public GenericMemoryException
{
 public:
  BufferOverflowException () : GenericMemoryException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_WARNING);
    }
};

class BusyResourceException : public GenericFileException
{
 public:
  BusyResourceException () : GenericFileException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_WARNING);
    }
};

class ByteException : public GenericMemoryException
{
 public:
  ByteException () : GenericMemoryException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_ERROR);
    }
};

class CancelException : public GenericSocketException
{
 public:
  CancelException () : GenericSocketException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_WARNING);
    }
};

class ChannelOutOfRangeException : public GenericSocketException
{
 public:
  ChannelOutOfRangeException () : GenericSocketException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_ERROR);
    }
};

class ConnectionInProgressException : public GenericSocketException
{
 public:
  ConnectionInProgressException () : GenericSocketException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_INFO);
    }
};

class DeadlockException : public GenericFileException
{
 public:
  DeadlockException () : GenericFileException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_ERROR);
    }
};

class DestAddressException : public GenericSocketException
{
 public:
  DestAddressException () : GenericSocketException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_INFO);
    }
};

class DeviceException : public GenericFileException
{
 public:
  DeviceException () : GenericFileException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_WARNING);
    }
};

class DirectoryException : public GenericFileException
{
 public:
  DirectoryException () : GenericFileException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_WARNING);
    }
};

class DiskQuotaException : public GenericMemoryException
{
 public:
  DiskQuotaException () : GenericMemoryException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_ERROR);
    }
};

class ExecFormatException : public GenericMemoryException
{
 public:
  ExecFormatException () : GenericMemoryException ()
    {
      setLogLevel(MYSERVER_LOG_MSG_ERROR);
    }
};

class FaultException : public GenericMemoryException
{
 public:
  FaultException () : GenericMemoryException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_ERROR);
    }
};

class FileExistsException : public GenericFileException
{
 public:
  FileExistsException () : GenericFileException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_WARNING);
    }
};

class FileNotFoundException : public GenericFileException
{
 public:
  FileNotFoundException () : GenericFileException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_WARNING);
    }
};

class FileTooLargeException : public GenericFileException
{
 public:
  FileTooLargeException () : GenericFileException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_WARNING);
    }
};

class FileTooLongException : public GenericFileException
{
 public:
  FileTooLongException () : GenericFileException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_WARNING);
    }
};

class FilesystemException : public GenericFileException
{
 public:
  FilesystemException () : GenericFileException ()
    {
      setLogLevel(MYSERVER_LOG_MSG_ERROR);
    }
};

class FunctionException : public GenericMemoryException
{
 public:
  FunctionException () : GenericMemoryException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_ERROR);
    }
};

class HaltException : public GenericMemoryException
{
 public:
  HaltException () : GenericMemoryException ()
    {
      setLogLevel(MYSERVER_LOG_MSG_ERROR);
    }
};

class HostDownException : public GenericSocketException
{
 public:
  HostDownException () : GenericSocketException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_WARNING);
    }
};

class IOException : public GenericFileException
{
 public:
  IOException () : GenericFileException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_ERROR);
    }
};

class IdentifierException : public GenericFileException
{
 public:
  IdentifierException () : GenericFileException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_INFO);
    }
};

class InvalidExchangeException : public GenericSocketException
{
 public:
  InvalidExchangeException () : GenericSocketException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_INFO);
    }
};

class InvalidResourceException : public GenericFileException
{
 public:
  InvalidResourceException () : GenericFileException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_INFO);
    }
};

class KeyException : public GenericSocketException
{
 public:
  KeyException () : GenericSocketException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_WARNING);
    }
};

class LibraryException : public GenericFileException
{
 public:
  LibraryException () : GenericFileException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_WARNING);
    }
};

class LinkException : public GenericFileException
{
 public:
  LinkException () : GenericFileException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_ERROR);
    }
};

class MathException : public GenericMemoryException
{
 public:
  MathException () : GenericMemoryException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_INFO);
    }
};

class MediaException : public GenericFileException
{
 public:
  MediaException () : GenericFileException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_INFO);
    }
};

class MemoryOverflowException : public GenericMemoryException
{
 public:
  MemoryOverflowException () : GenericMemoryException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_INFO);
    }
};

class MessageException : public GenericSocketException
{
 public:
  MessageException () : GenericSocketException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_ERROR);
    }
};

class MultihopException : public GenericSocketException
{
 public:
  MultihopException () : GenericSocketException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_WARNING);
    }
};

class NamedFileException : public GenericFileException
{
 public:
  NamedFileException () : GenericFileException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_WARNING);
    }
};

class NetworkException : public GenericSocketException
{
 public:
  NetworkException () : GenericSocketException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_INFO);
    }
};

class OverflowException : public GenericMemoryException
{
 public:
  OverflowException () : GenericMemoryException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_WARNING);
    }
};

class PermissionDeniedException : public GenericFileException
{
 public:
  PermissionDeniedException () : GenericFileException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_WARNING);
    }
};

class PipeException : public GenericFileException
{
 public:
  PipeException () : GenericFileException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_ERROR);
    }
};

class ProcessException : public GenericFileException
{
 public:
  ProcessException () : GenericFileException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_WARNING);
    }
};

class ProgressException : public GenericSocketException
{
 public:
  ProgressException () : GenericSocketException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_WARNING);
    }
};

class ProtocolException : public GenericSocketException
{
 public:
  ProtocolException () : GenericSocketException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_WARNING);
    }
};

class ReadException : public GenericSocketException
{
 public:
  ReadException () : GenericSocketException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_ERROR);
    }
};

class RefusedConnectionException : public GenericSocketException
{
 public:
  RefusedConnectionException () : GenericSocketException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_ERROR);
    }
};

class RemoteIOException : public GenericSocketException
{
 public:
  RemoteIOException () : GenericSocketException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_WARNING);
    }
};

class RequestCodeException : public GenericSocketException
{
 public:
  RequestCodeException () : GenericSocketException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_INFO);
    }
};

class RequestDescriptorException : public GenericSocketException
{
 public:
  RequestDescriptorException () : GenericSocketException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_INFO);
    }
};

class ResetException : public GenericSocketException
{
 public:
  ResetException () : GenericSocketException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_WARNING);
    }
};

class SendException : public GenericSocketException
{
 public:
  SendException () : GenericSocketException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_WARNING);
    }
};

class SlotException : public GenericMemoryException
{
 public:
  SlotException () : GenericMemoryException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_ERROR);
    }
};

class SocketException : public GenericSocketException
{
 public:
  SocketException () : GenericSocketException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_INFO);
    }
};

class StreamException : public GenericSocketException
{
 public:
  StreamException () : GenericSocketException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_ERROR);
    }
};

class TimerException : public GenericMemoryException
{
 public:
  TimerException () : GenericMemoryException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_INFO);
    }
};

class TooManyFileException : public GenericMemoryException
{
 public:
  TooManyFileException () : GenericMemoryException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_ERROR);
    }
};

/*
 * Thrown if an unknown error occurs
 */
class UnknownException : public AbstractServerException
{
 public:
  UnknownException () : AbstractServerException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_INFO);
    }
};

class UnreachHostException : public GenericSocketException
{
 public:
  UnreachHostException () : GenericSocketException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_ERROR);
    }
};

class UserException : public GenericFileException
{
 public:
  UserException () : GenericFileException ()
    {
      setLogLevel (MYSERVER_LOG_MSG_WARNING);
    }
};

#endif
