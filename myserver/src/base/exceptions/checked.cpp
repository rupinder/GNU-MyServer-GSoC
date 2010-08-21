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

#include <errno.h>

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
    return checkError (gnulib::pipe2 (pipefd, flags));
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

  int stat (const char *path, struct stat *buf)
  {
    return checkError (::stat (path, buf));
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
    Maps POSIX errors to exceptions
   */
  void raiseException ()
  {
    switch(errno)
      {
#ifdef E2BIG
      case E2BIG:
        throw ArgumentListException ();
        break;
#endif

#ifdef EACCES
      case EACCES:
        throw PermissionDeniedException ();
        break;
#endif

#ifdef EADDRINUSE
      case EADDRINUSE:
        throw AddressException ();
        break;
#endif

#ifdef EADDRNOTAVAIL
      case EADDRNOTAVAIL:
        throw AddressException ();
        break;
#endif

#ifdef EAFNOSUPPORT
      case EAFNOSUPPORT:
        throw ProtocolException ();
        break;
#endif

#ifdef EAGAIN
      case EAGAIN:
        throw InvalidResourceException ();
        break;
#endif

#ifdef EALREADY
      case EALREADY:
        throw ProgressException ();
        break;
#endif

#ifdef EBADE
      case EBADE:
        throw InvalidExchangeException ();
        break;
#endif

#ifdef EBADF
      case EBADF:
        throw BadDescriptorException ();
        break;
#endif

#ifdef EBADFD
      case EBADFD:
        throw BadDescriptorException ();
        break;
#endif

#ifdef EBADMSG
      case EBADMSG:
        throw MessageException ();
        break;
#endif

#ifdef EBADR
      case EBADR:
        throw RequestCodeException ();
        break;
#endif

#ifdef EBADRQC
      case EBADRQC:
        throw RequestCodeException ();
        break;
#endif

#ifdef EBADSLT
      case EBADSLT:
        throw SlotException ();
        break;
#endif

#ifdef EBUSY
      case EBUSY:
        throw BusyResourceException ();
        break;
#endif

#ifdef ECANCELED
      case ECANCELED:
        throw CancelException ();
        break;
#endif

#ifdef ECHILD
      case ECHILD:
        throw ProcessException ();
        break;
#endif

#ifdef ECHRNG
      case ECHRNG:
        throw ChannelOutOfRangeException ();
        break;
#endif

#ifdef ECOMM
      case ECOMM:
        throw SendException ();
        break;
#endif

#ifdef ECONNABORTED
      case ECONNABORTED:
        throw AbortedConnectionException ();
        break;
#endif

#ifdef ECONNREFUSED
      case ECONNREFUSED:
        throw RefusedConnectionException ();
        break;
#endif

#ifdef ECONNRESET
      case ECONNRESET:
        throw ResetException ();
        break;
#endif

#ifdef EDEADLK
      case EDEADLK:
        throw DeadlockException ();
        break;
#endif

#ifdef EDESTADDRREQ
      case EDESTADDRREQ:
        throw AddressException ();
        break;
#endif

#ifdef EDOM
      case EDOM:
        throw MathException ();
        break;
#endif

#ifdef EDQUOT
      case EDQUOT:
        throw DiskQuotaException ();
        break;
#endif

#ifdef EEXIST
      case EEXIST:
        throw FileExistsException ();
        break;
#endif

#ifdef EFAULT
      case EFAULT:
        throw FaultException ();
        break;
#endif

#ifdef EFBIG
      case EFBIG:
        throw FileTooLargeException ();
        break;
#endif

#ifdef EHOSTDOWN
      case EHOSTDOWN:
        throw HostDownException ();
        break;
#endif

#ifdef EHOSTUNREACH
      case EHOSTUNREACH:
        throw UnreachHostException ();
        break;
#endif

#ifdef EIDRM
      case EIDRM:
        throw IdentifierException ();
        break;
#endif

#ifdef EILSEQ
      case EILSEQ:
        throw ByteException ();
        break;
#endif

#ifdef EINPROGRESS
      case EINPROGRESS:
        throw ProgressException ();
        break;
#endif

#ifdef EINTR
      case EINTR:
        throw FunctionException ();
        break;
#endif

#ifdef EINVAL
      case EINVAL:
        throw ArgumentListException ();
        break;
#endif

#ifdef EIO
      case EIO:
        throw IOException ();
        break;
#endif

#ifdef EISCONN
      case EISCONN:
        throw SocketException ();
        break;
#endif

#ifdef EISDIR
      case EISDIR:
        throw DirectoryException ();
        break;
#endif

#ifdef EISNAM
      case EISNAM:
        throw NamedFileException ();
        break;
#endif

#ifdef EKEYEXPIRED
      case EKEYEXPIRED:
        throw KeyException ();
        break;
#endif

#ifdef EKEYREJECTED
      case EKEYREJECTED:
        throw KeyException ();
        break;
#endif

#ifdef EKEYREVOKED
      case EKEYREVOKED:
        throw KeyException ();
        break;
#endif

#ifdef EL2HLT
      case EL2HLT:
        throw HaltException ();
        break;
#endif

#ifdef EL2NSYNC
      case EL2NSYNC:
        throw HaltException ();
        break;
#endif

#ifdef EL3HLT
      case EL3HLT:
        throw HaltException ();
        break;
#endif

#ifdef ELIBACC
      case ELIBACC:
        throw LibraryException ();
        break;
#endif

#ifdef ELIBBAD
      case ELIBBAD:
        throw LibraryException ();
        break;
#endif

#ifdef ELIBMAX
      case ELIBMAX:
        throw LibraryException ();
        break;
#endif

#ifdef ELIBSCN
      case ELIBSCN:
        throw LibraryException ();
        break;
#endif

#ifdef ELIBEXEC
      case ELIBEXEC:
        throw LibraryException ();
        break;
#endif

#ifdef ELOOP
      case ELOOP:
        throw LinkException ();
        break;
#endif

#ifdef EMEDIUMTYPE
      case EMEDIUMTYPE:
        throw MediaException ();
        break;
#endif

#ifdef EMFILE
      case EMFILE:
        throw TooManyFileException ();
        break;
#endif

#ifdef EMLINK
      case EMLINK:
        throw LinkException ();
        break;
#endif

#ifdef EMSGSIZE
      case EMSGSIZE:
        throw MessageException ();
        break;
#endif

#ifdef EMULTIHOP
      case EMULTIHOP:
        throw MultihopException ();
        break;
#endif

#ifdef ENAMETOOLONG
      case ENAMETOOLONG:
        throw FileTooLongException ();
        break;
#endif

#ifdef ENETRESET
      case ENETRESET:
        throw NetworkException ();
        break;
#endif

#ifdef ENETUNREACH
      case ENETUNREACH:
        throw NetworkException ();
        break;
#endif

#ifdef ENFILE
      case ENFILE:
        throw FilesystemException ();
        break;
#endif

#ifdef ENOBUFS
      case ENOBUFS:
        throw BufferOverflowException ();
        break;
#endif

#ifdef ENODATA
      case ENODATA:
        throw StreamException ();
        break;
#endif

#ifdef ENODEV
      case ENODEV:
        throw DeviceException ();
        break;
#endif

#ifdef ENOENT
      case ENOENT:
        throw FileNotFoundException ();
        break;
#endif

#ifdef ENOEXEC
      case ENOEXEC:
        throw ExecFormatException ();
        break;
#endif

#ifdef ENOKEY
      case ENOKEY:
        throw KeyException ();
        break;
#endif

#ifdef ENOLINK
      case ENOLINK:
        throw LinkException ();
        break;
#endif

#ifdef ENOMEDIUM
      case ENOMEDIUM:
        throw MediaException ();
        break;
#endif

#ifdef ENOMEM
      case ENOMEM:
        throw MemoryOverflowException ();
        break;
#endif

#ifdef ENOMSG
      case ENOMSG:
        throw MessageException ();
        break;
#endif

#ifdef ENONET
      case ENONET:
        throw NetworkException ();
        break;
#endif

#ifdef ENOSPC
      case ENOSPC:
        throw OverflowException ();
        break;
#endif

#ifdef ENOSR
      case ENOSR:
        throw StreamException ();
        break;
#endif

#ifdef ENOSTR
      case ENOSTR:
        throw StreamException ();
        break;
#endif

#ifdef ENOSYS
      case ENOSYS:
        throw FunctionException ();
        break;
#endif

#ifdef ENOTBLK
      case ENOTBLK:
        throw BlockingException ();
        break;
#endif

#ifdef ENOTCONN
      case ENOTCONN:
        throw SocketException ();
        break;
#endif

#ifdef ENOTDIR
      case ENOTDIR:
        throw DirectoryException ();
        break;
#endif

#ifdef ENOTEMPTY
      case ENOTEMPTY:
        throw DirectoryException ();
        break;
#endif

#ifdef ENOTSOCK
      case ENOTSOCK:
        throw SocketException ();
        break;
#endif

#ifdef ENOTTY
      case ENOTTY:
        throw IOException ();
        break;
#endif

#ifdef ENOTUNIQ
      case ENOTUNIQ:
        throw NetworkException ();
        break;
#endif

#ifdef ENXIO
      case ENXIO:
        throw DeviceException ();
        break;
#endif

#ifdef EOVERFLOW
      case EOVERFLOW:
        throw OverflowException ();
        break;
#endif

#ifdef EPERM
      case EPERM:
        throw PermissionDeniedException ();
        break;
#endif

#ifdef EPFNOSUPPORT
      case EPFNOSUPPORT:
        throw ProtocolException ();
        break;
#endif

#ifdef EPIPE
      case EPIPE:
        throw PipeException ();
        break;
#endif

#ifdef EPROTO
      case EPROTO:
        throw ProtocolException ();
        break;
#endif

#ifdef EPROTONOSUPPORT
      case EPROTONOSUPPORT:
        throw ProtocolException ();
        break;
#endif

#ifdef EPROTOTYPE
      case EPROTOTYPE:
        throw ProtocolException ();
        break;
#endif

#ifdef ERANGE
      case ERANGE:
        throw OverflowException ();
        break;
#endif

#ifdef EREMCHG
      case EREMCHG:
        throw AddressException ();
        break;
#endif

#ifdef EREMOTEIO
      case EREMOTEIO:
        throw IOException ();
        break;
#endif

#ifdef ERESTART
      case ERESTART:
        throw FunctionException ();
        break;
#endif

#ifdef EROFS
      case EROFS:
        throw FilesystemException ();
        break;
#endif

#ifdef ESHUTDOWN
      case ESHUTDOWN:
        throw SendException ();
        break;
#endif

#ifdef ESPIPE
      case ESPIPE:
        throw PipeException ();
        break;
#endif

#ifdef ESOCKTNOSUPPORT
      case ESOCKTNOSUPPORT:
        throw SocketException ();
        break;
#endif

#ifdef ESRCH
      case ESRCH:
        throw ProcessException ();
        break;
#endif

#ifdef ESTALE
      case ESTALE:
        throw GenericFileException ();
        break;
#endif

#ifdef ESTRPIPE
      case ESTRPIPE:
        throw PipeException ();
        break;
#endif

#ifdef ETIME
      case ETIME:
        throw TimerException ();
        break;
#endif

#ifdef ETIMEDOUT
      case ETIMEDOUT:
        throw TimerException ();
        break;
#endif

#ifdef ETXTBSY
      case ETXTBSY:
        throw BusyResourceException ();
        break;
#endif

#ifdef EUNATCH
      case EUNATCH:
        throw ProtocolException ();
        break;
#endif

#ifdef EUSERS
      case EUSERS:
        throw UserException ();
        break;
#endif

#ifdef EXDEV
      case EXDEV:
        throw LinkException ();
        break;
#endif

#ifdef EXFULL
      case EXFULL:
        throw InvalidExchangeException ();
        break;
#endif

      default:
        throw UnknownException ();
        break;
      }
  }
}
