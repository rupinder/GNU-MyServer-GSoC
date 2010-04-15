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

#ifndef CHECKED_H
# define CHECKED_H

# include "myserver.h"
# include <errno.h>

# include <alloca.h>
# include <dirent.h>
# include <fcntl.h>
# include <regex.h>
# include <signal.h>
# include <stdio.h>
# include <stdlib.h>
# include <sys/ioctl.h>
# include <sys/select.h>
# include <sys/socket.h>
# include <sys/stat.h>
# include <sys/time.h>
# include <sys/wait.h>
# include <time.h>
# include <unistd.h>
# include <wchar.h>
# include <wctype.h>

# include <include/base/exceptions/exceptions.h>

/*!
 * Group of checked functions that throws an exception in case of error,
 * instead of just setting errno
 */
namespace checked
{
  int accept (int fd, struct sockaddr *addr, socklen_t *addrlen);
  int bind (int fd, const struct sockaddr *addr, socklen_t addrlen);
  int chown (const char *file, uid_t uid, gid_t gid);
  int close (int fd);
  int closedir (DIR *);
  int connect (int fd, const struct sockaddr *addr, socklen_t addrlen);
  int dup (int oldfd);
  int dup2 (int oldfd, int newfd);
  int pipe2 (int pipefd[2], int flags);
  int fstat (int fd, struct stat *buf);
  int fstatat (int fd, char const *name, struct stat *st, int flags);
  char *getcwd (char *buf, size_t size);
  int gethostname (char *name, size_t len);
  int getsockname (int fd, struct sockaddr *addr, socklen_t *addrlen);
  int gettimeofday (struct timeval *, void *);
  struct tm *gmtime_r (time_t const *__timer, struct tm *__result);
  int listen (int fd, int backlog);
  struct tm *localtime_r (time_t const *__timer, struct tm *__result);
  int lstat (const char *name, struct stat *buf);
  void *memchr (void const *__s, int __c, size_t __n);
  int mkdir (char const *name, mode_t mode);
  time_t mktime (struct tm *__tp);
  int open (const char *filename, int flags, int mask = 0);
  DIR *opendir (const char *);
  int fsync (int fd);
  ssize_t read (int fd, void *buf, size_t count);
  void *realloc (void *ptr, size_t size);
  ssize_t recv (int fd, void *buf, size_t len, int flags);
  int remove (const char *name);
  int rename (const char *old_filename, const char *new_filename);
  int rmdir (char const *name);
  int select (int, fd_set *, fd_set *, fd_set *, struct timeval *);
  ssize_t send (int fd, const void *buf, size_t len, int flags);
  int setsockopt (int fd, int level, int optname, const void *optval, socklen_t optlen);
  int shutdown (int fd, int how);
  int sigaction (int signum, const struct sigaction *act, struct sigaction *oldact);
  int sigaddset (sigset_t *set, int signum);
  int sigemptyset (sigset_t *set);
  int sigprocmask (int how, const sigset_t *set, sigset_t *oldset);
  int socket (int domain, int type, int protocol);
  char *strdup (char const *__s);
  int unlink (char const *file);
  ssize_t write (int fd, const void *buf, size_t count);

  void raiseException ();
}

#endif
