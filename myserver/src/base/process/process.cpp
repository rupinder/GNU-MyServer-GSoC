/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2006, 2007, 2008, 2009, 2010 Free
  Software Foundation, Inc.
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

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <include/base/exceptions/checked.h>

#ifndef WIN32
# include <unistd.h>
# include <signal.h>
# include <sys/types.h>
# include <sys/wait.h>

# ifdef HAVE_GETPWNAM
#  include <pwd.h>
# endif

# ifdef HAVE_GRP_H
#  include <grp.h>
# endif

#endif


#ifndef UID_T_MAX
# define UID_T_MAX (1<<31)
#endif

#ifndef GID_T_MAX
# define GID_T_MAX (1<<31)
#endif

/* MAXUID may come from limits.h or sys/params.h.  */
#ifndef MAXUID
# define MAXUID UID_T_MAX
#endif
#ifndef MAXGID
# define MAXGID GID_T_MAX
#endif

#ifdef WIN32
# include <direct.h>
#endif

#ifdef HAVE_PTHREAD
Mutex Process::forkMutex;
#endif

ForkServer Process::forkServer;

/*!
  Generate the arguments vector for execve.
  \param args The output arguments vector to fill.
  \param size Size of the args vector.
  \param proc The executable.
  \param additionalArgs additional arguments.
 */
int Process::generateArgList (const char **args, size_t size, const char *proc,
                              string &additionalArgs)
{
  args[0] = proc;

  const char *arg = additionalArgs.c_str ();
  u_long count = 1;
  int len = additionalArgs.length ();
  int start = 0;

  while ((arg[start] == ' ') && (start < len))
    start++;

  for (int i = start; i < len; i++)
    {
      if (arg[i] == '"')
        {
          start = i++;
          while (additionalArgs[++i] != '"' && i < len);
          if (i == len)
            exit (1);

          if (count < size)
            {
              args[count++] = &(additionalArgs[start + 1]);
              start = i + 1;
              additionalArgs[i] = '\0';
            }
          else
            break;
        }
      else if (arg[i] == ' ')
        {
          if (i - start <= 1)
            continue;
          if (count < size)
            {
              args[count++] = &(additionalArgs[start]);
              additionalArgs[i] = '\0';

              while ((arg[i] == ' ') && (i < len))
                i++;

              start = i + 1;
            }
          else
            break;
        }
    }
  if (count < size && len != start)
    {
      args[count++] = &(additionalArgs[start]);
    }

  args[count] = NULL;

  return 0;
}

/*!
  Change the process root directory to the specified one.
  \param root The new root directory.
  On success 0 is returned.
 */
int Process::chroot (const char *root)
{
#ifdef WIN32
  return -1;
#else
  return chroot (root);
#endif
}


/*!
  Generate the env array for execve.
  \param envp Enviroment variables array.
  \param size Size of the args vector.
  \param envString Enviroment values separed by the NULL character.
 */
int Process::generateEnvString (const char **envp, size_t size, char *envString)
{
  int i = 0;
  size_t index = 0;

  if (envString != NULL)
    {
      while (*(envString + i) != '\0')
        {
          envp[index] = envString + i;
          index++;

          if (index == size)
            return 1;

          while (*(envString + i++) != '\0');
        }
      envp[index] = NULL;
    }
  else
    envp[0] = NULL;
  return 0;
}

/*!
  Spawn a new process.

  \param spi new process information.
  \param waitEnd Specify if wait until the process finishes.

  \return -1 on failure.
  \return the new process identifier on success.
 */
int Process::exec (StartProcInfo* spi, bool waitEnd)
{
  pid = 0;
#ifdef WIN32
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  const char *cwd;
  ZeroMemory ( &si, sizeof (si));
  si.cb = sizeof (si);
  si.hStdInput = (HANDLE)spi->stdIn;
  si.hStdOutput = (HANDLE)spi->stdOut;
  si.hStdError = (HANDLE)spi->stdError;
  si.dwFlags = STARTF_USESHOWWINDOW;
  if (si.hStdInput || si.hStdOutput ||si.hStdError)
    si.dwFlags |= STARTF_USESTDHANDLES;
  si.wShowWindow = SW_HIDE;

  cwd = spi->cwd.length () ? (char*)spi->cwd.c_str () : 0;
  ZeroMemory ( &pi, sizeof (pi) );

  BOOL ret = CreateProcess (NULL, (char*)spi->cmdLine.c_str (), NULL, NULL, TRUE,0,
                           spi->envString, cwd, &si, &pi);

  if (!ret)
    return (-1);

  pid = (u_long)pi.hProcess;

  if (waitEnd)
    {
      ret = WaitForSingleObject (pi.hProcess, INFINITE);

      if (ret == WAIT_FAILED)
        return (u_long)(-1);

      return 0;
    }
  else
    {
      pid = (*((int*)&pi.hProcess));

      return pid;
    }
#endif

#ifndef WIN32
  pid = fork ();

  if (pid < 0)
    return 0;

  if (pid == 0)
    {
      const size_t size = 100;
      const char *envp[size];
      const char *args[size];

      if (spi->chroot.length () && Process::chroot (spi->chroot.c_str ()))
        exit (1);

      if (spi->gid.length () && Process::setgid (spi->gid.c_str ()))
        exit (1);

      if (spi->uid.length () && Process::setuid (spi->uid.c_str ()))
        exit (1);

      if (generateArgList (args, size, spi->cmd.c_str (), spi->arg))
        exit (1);

      if (generateEnvString (envp, size, (char*) spi->envString))
        exit (1);

      if (spi->cwd.length ()
          && chdir (spi->cwd.c_str ()) == -1)
        exit (1);

      if ((long)spi->stdOut == -1)
        spi->stdOut = checked::open ("/dev/null", O_WRONLY);

      if ((long)spi->stdError == -1)
        spi->stdError = checked::open ("/dev/null", O_WRONLY);

      checked::close (0);

      if (spi->stdIn != -1)
        {
          if (checked::dup2 (spi->stdIn, 0) == -1)
            exit (1);
          checked::close (spi->stdIn);
        }

      checked::close (1);

      if (checked::dup2 (spi->stdOut, 1) == -1)
        exit (1);

      checked::close (2);

      if (checked::dup2 (spi->stdError, 2) == -1)
        exit (1);

      if (spi->handlesToClose)
        {
          FileHandle* h = spi->handlesToClose;
          while (*h)
            {
              checked::close (*h);
              h++;
            }
        }

      execve ((const char*) args[0],
              (char* const*) args, (char* const*) envp);

    exit (1);
  }

  if (waitEnd)
    return waitpid (pid, NULL, 0);
  else
    return pid;


#endif

}

/*!
  Get an uid given a username.
  \param the user name to convert.
 */
uid_t Process::getUid (const char *user)
{
#ifndef WIN32

  if (isdigit (user[0]))
    {
      uid_t uid = atol (user);

      if (uid >= 0L && uid <= (uid_t) MAXUID)
        return uid;
    }

# ifdef HAVE_GETPWNAM
  struct passwd *u = getpwnam (user);
  if (u != NULL)
    return u->pw_uid;
# endif

#endif

  return (gid_t)-1;
}

/*!
  Get an uid given a username.
  \param the user name to convert.
 */
gid_t Process::getGid (const char *grp)
{
#ifndef WIN32
  if (isdigit (grp[0]))
    {
      gid_t gid = atol (grp);

      if (gid >= 0L && gid <= (gid_t) MAXGID)
        return gid;
    }

# ifdef HAVE_GRP_H
  struct group *g = getgrnam (grp);
  if (g != NULL)
    return g->gr_gid;
# endif

#endif

  return (gid_t)-1;
}

/*!
  Return a nonzero value if the process is still alive. A return value of zero
  means the process is a zombie.
 */
int Process::isProcessAlive ()
{
  if (pid == 0)
    return 0;
#ifdef WIN32
  u_long ec;
  int ret = GetExitCodeProcess (*((HANDLE*)&pid), &ec);
  if (ret == 0)
    return 0;
  if (ec == STILL_ACTIVE)
    return 1;
  return 0;
#endif

#ifndef WIN32
  int status = 0;
# ifdef PROCESS_ALIVE_USEWAITPID
  int ret;
  do
  {
    ret = waitpid (pid, &status, WNOHANG);
  }
  while (!ret && errno == EINTR);
  if (!ret)
    return 1;
  return 0;
# else
  /* Send the pseudo-signal 0 to check if the process is alive.  */
  int ret;
  do
  {
    ret = kill (pid, 0);
  }
  while (!ret && errno == EINTR);
  if (ret == 0)
    return 1;

  /* Waitpid it to free the resource.  */
  waitpid (pid, &status, WNOHANG | WUNTRACED);

  return 0;
# endif

#endif
  return 0;
}

#ifdef HAVE_PTHREAD

/*!
  Called in the parent before do a fork.
 */
void Process::forkPrepare ()
{
  forkMutex.lock ();
}

/*!
  Called in the parent after the fork.
 */
void Process::forkParent ()
{
  forkMutex.unlock ();
}

/*!
  Called in the child process after the fork.
 */
void Process::forkChild ()
{
  forkMutex.unlock ();
}
#endif

/*!
  Initialize the static process data.
 */
void Process::initialize ()
{
#ifdef HAVE_PTHREAD
  forkMutex.init ();
  pthread_atfork (forkPrepare, forkParent, forkChild);
#endif
}

/*!
  Create the object.
 */
Process::Process ()
{
  pid = 0;
}

/*!
  Destroy the object.
 */
Process::~Process ()
{

}

/*!
  Terminate a process.
  Return 0 on success.
  Return nonzero on fails.
 */
int Process::terminateProcess ()
{
  int ret;
  if (pid == 0)
    return 0;
#ifdef WIN32
  ret = TerminateProcess (*((HANDLE*)&pid),0);
  pid = 0;
  return (!ret);
#endif
#ifndef WIN32
  ret = kill ((pid_t)pid, SIGKILL);
  pid = 0;
  return ret;
#endif
}

/*!
  Set the user identity for the process. Returns 0 on success.
 */
int Process::setuid (const char *uid)
{
#ifndef WIN32
  if (uid && uid[0])
    return ::setuid (getUid (uid));
#endif
  return 0;
}

/*!
  Set the group identity for the process. Returns 0 on success.
 */
int Process::setgid (const char *gid)
{
#ifndef WIN32
  if (gid && gid[0])
    return ::setgid (getGid (gid));
#endif
  return 0;
}

/*!
  Set the additional groups list for the process.
 */
int Process::setAdditionalGroups (u_long len, u_long *groups)
{
#ifndef WIN32

# if HAVE_SETGROUPS
  u_long i;
  int ret;
  gid_t *gids = new gid_t[len];

  for (i = 0; i < len; i++)
    if (groups)
      gids[i] = (gid_t) groups[i];
    else
      gids[i] = (gid_t) 0;

  ret = setgroups ((size_t) 0, gids) == -1;

  delete [] gids;

  if (errno == EPERM && len == 0)
    return 0;
  return ret;

# else
  return 0;
# endif

#else
  return 0;
#endif
}
