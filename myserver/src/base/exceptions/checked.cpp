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
#include <include/base/exceptions/checked.h>

/*!
 * Check if the last operation was successful
 * \param x The function's return value
 */
static inline int checkError (int x)
{
  if (x < 0)
    checked::raiseException ();

  return x;
}

/*!
 * \see checkError
 */
static inline const void *checkErrorNull (const void *x)
{
  if (x == NULL)
    checked::raiseException ();

  return x;
}

namespace checked
{
  int accept (int fd, struct sockaddr *addr, socklen_t *addrlen)
  {
    return checkError (gnulib::accept (fd, addr, addrlen));
  }

  int bind (int fd, const struct sockaddr *addr, socklen_t addrlen)
  {
    return checkError (gnulib::bind (fd, addr, addrlen));
  }

  int chown (const char *file, uid_t uid, gid_t gid)
  {
    return checkError (gnulib::chown (file, uid, gid));
  }

  int close (int fd)
  {
    return checkError (gnulib::close (fd));
  }

  int closedir (DIR *dirp)
  {
    return checkError (gnulib::closedir (dirp));
  }

  int connect (int fd, const struct sockaddr *addr, socklen_t addrlen)
  {
    return checkError (gnulib::connect (fd, addr, addrlen));
  }

  int dup (int oldfd)
  {
    return checkError (gnulib::dup (oldfd));
  }

  int dup2 (int oldfd, int newfd)
  {
    return checkError (gnulib::dup2 (oldfd, newfd));
  }

  int pipe2 (int pipefd[2], int flags)
  {
    return checkError (pipe2 (pipefd, flags));
  }

  int fstat (int fd, struct stat *buf)
  {
    return checkError (gnulib::fstat (fd, buf));
  }

  int fstatat (int fd, char const *name, struct stat *st, int flags)
  {
    return checkError (gnulib::fstatat (fd, name, st, flags));
  }

  char *getcwd (char *buf, size_t size)
  {
    return (char *) checkErrorNull (gnulib::getcwd (buf, size));
  }

  int gethostname (char *name, size_t len)
  {
    return checkError (gnulib::gethostname (name, len));
  }

  int getsockname (int fd, struct sockaddr *addr, socklen_t *addrlen)
  {
    return checkError (gnulib::getsockname (fd, addr, addrlen));
  }

  int gettimeofday (struct timeval *tv, struct timezone *tz)
  {
    return checkError (gnulib::gettimeofday (tv, tz));
  }

  struct tm *gmtime_r (time_t const *__timer, struct tm *__result)
  {
    return (struct tm *) checkErrorNull (gnulib::gmtime_r (__timer, __result));
  }

  int listen (int fd, int backlog)
  {
    return checkError (gnulib::listen (fd, backlog));
  }

  struct tm *localtime_r (time_t const *__timer, struct tm *__result)
  {
    return (struct tm *) checkErrorNull (gnulib::localtime_r (__timer, __result));
  }

  int lstat (const char *name, struct stat *buf)
  {
    return checkError (gnulib::lstat (name, buf));
  }

  void *memchr (void const *__s, int __c, size_t __n)
  {
    return (void *) checkErrorNull (gnulib::memchr (__s, __c, __n));
  }

  int mkdir (char const *name, mode_t mode)
  {
    return checkError (gnulib::mkdir (name, mode));
  }

  time_t mktime (struct tm *__tp)
  {
    return checkError (gnulib::mktime (__tp));
  }

  int open (const char *filename, int flags, int mask)
  {
    return checkError (gnulib::open (filename, flags, mask));
  }

  DIR *opendir (const char *name)
  {
    return (DIR *) checkErrorNull (gnulib::opendir (name));
  }

  ssize_t read (int fd, void *buf, size_t count)
  {
    return checkError (::read (fd, buf, count));
  }

  int fsync (int fd)
  {
    return checkError (gnulib::fsync (fd));
  }


  void *realloc (void *ptr, size_t size)
  {
    return (void *) checkErrorNull (gnulib::realloc (ptr, size));
  }

  ssize_t recv (int fd, void *buf, size_t len, int flags)
  {
    return checkError (gnulib::recv (fd, buf, len, flags));
  }

  int remove (const char *name)
  {
    return checkError (gnulib::remove (name));
  }

  int rename (const char *old_filename, const char *new_filename)
  {
    return checkError (gnulib::rename (old_filename, new_filename));
  }

  int rmdir (char const *name)
  {
    return checkError (gnulib::rmdir (name));
  }

  int select (int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
              struct timeval *timeout)
  {
    return checkError (gnulib::select (nfds, readfds, writefds, exceptfds, timeout));
  }

  ssize_t send (int fd, const void *buf, size_t len, int flags)
  {
    return checkError (gnulib::send (fd, buf, len, flags));
  }

  int setsockopt (int fd, int level, int optname, const void *optval, socklen_t optlen)
  {
    return checkError (gnulib::setsockopt (fd, level, optname, optval, optlen));
  }

  int shutdown (int fd, int how)
  {
    return checkError (gnulib::shutdown (fd, how));
  }

  int sigaction (int signum, const struct sigaction *act, struct sigaction *oldact)
  {
    return checkError (gnulib::sigaction (signum, act, oldact));
  }

  int sigaddset (sigset_t *set, int signum)
  {
    return checkError (gnulib::sigaddset (set, signum));
  }

  int sigemptyset (sigset_t *set)
  {
    return checkError (gnulib::sigemptyset (set));
  }

  int sigprocmask (int how, const sigset_t *set, sigset_t *oldset)
  {
    return checkError (gnulib::sigprocmask (how, set, oldset));
  }

  int socket (int domain, int type, int protocol)
  {
    return checkError (gnulib::socket (domain, type, protocol));
  }

  char *strdup (char const *__s)
  {
    return (char *) checkErrorNull (gnulib::strdup (__s));
  }

  int unlink (char const *file)
  {
    return checkError (gnulib::unlink (file));
  }

  ssize_t write (int fd, const void *buf, size_t count)
  {
    return checkError (gnulib::write (fd, buf, count));
  }

  /*
   * Maps POSIX errors to exceptions
   */
  void raiseException ()
  {
    switch(errno)
      {
      case E2BIG:
        throw ArgumentListException ();
        break;

      case EACCES:
        throw PermissionDeniedException ();
        break;

      case EADDRINUSE:
        throw AddressException ();
        break;

      case EADDRNOTAVAIL:
        throw AddressException ();
        break;

      case EAFNOSUPPORT:
        throw ProtocolException ();
        break;

      case EAGAIN:
        throw InvalidResourceException ();
        break;

      case EALREADY:
        throw ProgressException ();
        break;

      case EBADE:
        throw InvalidExchangeException ();
        break;

      case EBADF:
        throw BadDescriptorException ();
        break;

      case EBADFD:
        throw BadDescriptorException ();
        break;

      case EBADMSG:
        throw MessageException ();
        break;

      case EBADR:
        throw RequestCodeException ();
        break;

      case EBADRQC:
        throw RequestCodeException ();
        break;

      case EBADSLT:
        throw SlotException ();
        break;

      case EBUSY:
        throw BusyResourceException ();
        break;

      case ECANCELED:
        throw CancelException ();
        break;

      case ECHILD:
        throw ProcessException ();
        break;

      case ECHRNG:
        throw ChannelOutOfRangeException ();
        break;

      case ECOMM:
        throw SendException ();
        break;

      case ECONNABORTED:
        throw AbortedConnectionException ();
        break;

      case ECONNREFUSED:
        throw RefusedConnectionException ();
        break;

      case ECONNRESET:
        throw ResetException ();
        break;

      case EDEADLK:
        throw DeadlockException ();
        break;

      case EDESTADDRREQ:
        throw AddressException ();
        break;

      case EDOM:
        throw MathException ();
        break;

      case EDQUOT:
        throw DiskQuotaException ();
        break;

      case EEXIST:
        throw FileExistsException ();
        break;

      case EFAULT:
        throw FaultException ();
        break;

      case EFBIG:
        throw FileTooLargeException ();
        break;

      case EHOSTDOWN:
        throw HostDownException ();
        break;

      case EHOSTUNREACH:
        throw UnreachHostException ();
        break;

      case EIDRM:
        throw IdentifierException ();
        break;

      case EILSEQ:
        throw ByteException ();
        break;

      case EINPROGRESS:
        throw ProgressException ();
        break;

      case EINTR:
        throw FunctionException ();
        break;

      case EINVAL:
        throw ArgumentListException ();
        break;

      case EIO:
        throw IOException ();
        break;

      case EISCONN:
        throw SocketException ();
        break;

      case EISDIR:
        throw DirectoryException ();
        break;

      case EISNAM:
        throw NamedFileException ();
        break;

      case EKEYEXPIRED:
        throw KeyException ();
        break;

      case EKEYREJECTED:
        throw KeyException ();
        break;

      case EKEYREVOKED:
        throw KeyException ();
        break;

      case EL2HLT:
        throw HaltException ();
        break;

      case EL2NSYNC:
        throw HaltException ();
        break;

      case EL3HLT:
        throw HaltException ();
        break;

      case ELIBACC:
        throw LibraryException ();
        break;

      case ELIBBAD:
        throw LibraryException ();
        break;

      case ELIBMAX:
        throw LibraryException ();
        break;

      case ELIBSCN:
        throw LibraryException ();
        break;

      case ELIBEXEC:
        throw LibraryException ();
        break;

      case ELOOP:
        throw LinkException ();
        break;

      case EMEDIUMTYPE:
        throw MediaException ();
        break;

      case EMFILE:
        throw TooManyFileException ();
        break;

      case EMLINK:
        throw LinkException ();
        break;

      case EMSGSIZE:
        throw MessageException ();
        break;

      case EMULTIHOP:
        throw MultihopException ();
        break;

      case ENAMETOOLONG:
        throw FileTooLongException ();
        break;

      case ENETRESET:
        throw NetworkException ();
        break;

      case ENETUNREACH:
        throw NetworkException ();
        break;

      case ENFILE:
        throw FilesystemException ();
        break;

      case ENOBUFS:
        throw BufferOverflowException ();
        break;

      case ENODATA:
        throw StreamException ();
        break;

      case ENODEV:
        throw DeviceException ();
        break;

      case ENOENT:
        throw FileNotFoundException ();
        break;

      case ENOEXEC:
        throw ExecFormatException ();
        break;

      case ENOKEY:
        throw KeyException ();
        break;

      case ENOLINK:
        throw LinkException ();
        break;

      case ENOMEDIUM:
        throw MediaException ();
        break;

      case ENOMEM:
        throw MemoryOverflowException ();
        break;

      case ENOMSG:
        throw MessageException ();
        break;

      case ENONET:
        throw NetworkException ();
        break;

      case ENOSPC:
        throw OverflowException ();
        break;

      case ENOSR:
        throw StreamException ();
        break;

      case ENOSTR:
        throw StreamException ();
        break;

      case ENOSYS:
        throw FunctionException ();
        break;

      case ENOTBLK:
        throw BlockingException ();
        break;

      case ENOTCONN:
        throw SocketException ();
        break;

      case ENOTDIR:
        throw DirectoryException ();
        break;

      case ENOTEMPTY:
        throw DirectoryException ();
        break;

      case ENOTSOCK:
        throw SocketException ();
        break;

      case ENOTTY:
        throw IOException ();
        break;

      case ENOTUNIQ:
        throw NetworkException ();
        break;

      case ENXIO:
        throw DeviceException ();
        break;

      case EOVERFLOW:
        throw OverflowException ();
        break;

      case EPERM:
        throw PermissionDeniedException ();
        break;

      case EPFNOSUPPORT:
        throw ProtocolException ();
        break;

      case EPIPE:
        throw PipeException ();
        break;

      case EPROTO:
        throw ProtocolException ();
        break;

      case EPROTONOSUPPORT:
        throw ProtocolException ();
        break;

      case EPROTOTYPE:
        throw ProtocolException ();
        break;

      case ERANGE:
        throw OverflowException ();
        break;

      case EREMCHG:
        throw AddressException ();
        break;

      case EREMOTEIO:
        throw IOException ();
        break;

      case ERESTART:
        throw FunctionException ();
        break;

      case EROFS:
        throw FilesystemException ();
        break;

      case ESHUTDOWN:
        throw SendException ();
        break;

      case ESPIPE:
        throw PipeException ();
        break;

      case ESOCKTNOSUPPORT:
        throw SocketException ();
        break;

      case ESRCH:
        throw ProcessException ();
        break;

      case ESTALE:
        throw GenericFileException ();
        break;

      case ESTRPIPE:
        throw PipeException ();
        break;

      case ETIME:
        throw TimerException ();
        break;

      case ETIMEDOUT:
        throw TimerException ();
        break;

      case ETXTBSY:
        throw BusyResourceException ();
        break;

      case EUNATCH:
        throw ProtocolException ();
        break;

      case EUSERS:
        throw UserException ();
        break;

      case EXDEV:
        throw LinkException ();
        break;

      case EXFULL:
        throw InvalidExchangeException ();
        break;

      default:
        throw UnknownException ();
        break;
      }
  }
}
