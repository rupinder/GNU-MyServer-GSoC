/*
  MyServer
  Copyright (C) 2008 Free Software Foundation, Inc.
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


#include <include/base/process/fork_server.h>
#include <include/base/process/process.h>
#include <include/base/pipe/pipe.h>

#ifdef NOT_WIN
#include <unistd.h>
#endif

/*!
 *Write an integer on the specified socket.
 *\param socket Socket to use.
 *\param num Integer to write.
 */
int ForkServer::writeInt (Socket *socket, int num)
{
  u_long nbw;

  if (socket->write ((const char*)&num, 4, &nbw))
    return 1;

  return 0;
}

/*!
 *Write a string to the socket.
 *The string length is sent before the content.
 *
 *\param socket Socket where write.
 *\param str string to write.
 *\param len string length.
 */
int ForkServer::writeString (Socket *socket, const char* str, int len)
{
  u_long nbw;

  if (str == NULL)
    len = 0;

  if (socket->write ((const char*)&len, 4, &nbw))
    return 1;

  if (len && socket->write (str, len, &nbw))
    return 1;

  return 0;
}

/*!
 *Read an integer from the socket.
 *
 *\param sock Socket where read.
 *\param dest integer where write
 *\return 0 on success.
 */
int ForkServer::readInt (Socket *sock, int *dest)
{
  u_long nbr;
  
  if (sock->read ((char*)dest, 4, &nbr) || nbr < 4)
    {
      return -1;
    }

  return 0;
}

/*!
 *Read a string from the socket.
 *The destination buffer is allocated here.
 *It must be freed by the caller.
 *
 *\param sock socket to use for read.
 *\param out destination buffer pointer.
 *\return 0 on success.
 */
int ForkServer::readString (Socket *sock, char **out)
{
  int len;
  u_long nbr;
  
  if (sock->read ((char*)&len, 4, &nbr) || nbr < 4)
    {
      return -1;
    }

  *out = new char[len];

  if (!len)
    {
      *out = NULL;
      return 0;
    }

  if (sock->read (*out, len, &nbr) || nbr < len)
    {
      delete [] *out;
      return -1;      
    }

  return 0;
}

/*!
 *Handle a request on the socket.
 */
int ForkServer::handleRequest (Socket sin, Socket sout, Socket *serverSock)
{
#ifdef NOT_WIN
  int ret, flags, stdIn, stdOut, stdErr;
  char *exec;
  char *cwd;
  char *arg;
  char *env;

  if (readInt (&sin, &flags) ||
      readInt (&sin, &stdIn) ||
      readInt (&sin, &stdOut) ||
      readInt (&sin, &stdErr))
    {
      return -1;     
    }
   
  if (readString (&sin, &exec))
    {
      return -1;
    }

  if (readString (&sin, &cwd))
    {
      delete [] exec;
      return -1;
    }

  if (readString (&sin, &arg))
    {
      delete [] exec;
      delete [] cwd;
      return -1;
    }
  string argS (arg);
  

  if (readString (&sin, &env))
    {
      delete [] exec;
      delete [] cwd;
      delete [] arg;
      return -1;
    }
  
  FileHandle stdHandles[3] = {flags & FLAG_USE_IN ? sin.getHandle () : -1,
                              flags & FLAG_USE_OUT ? sout.getHandle () : -1,
                              -1};

  Socket socketIn;
  int stdInPort = 0;

  if (flags & FLAG_STDIN_SOCKET)
    {
      u_short stdInPortS;
      if (generateListenerSocket (socketIn, &stdInPortS) ||
          socketIn.listen (SOMAXCONN))
        {
          delete [] exec;
          delete [] cwd;
          delete [] arg;
          return -1;
        }
      stdInPort = (int) stdInPortS;

      stdHandles[0] = socketIn.getHandle ();
      stdHandles[1] = stdHandles[2] = (FileHandle) -1;
    }

  StartProcInfo spi;

  spi.envString = (env && env[0]) ? env : NULL;

  spi.stdIn = stdHandles[0];
  spi.stdOut = stdHandles[1];
  spi.stdError = stdHandles[2];

  spi.cmd.assign (exec);
  spi.arg.assign (arg);
  spi.cwd.assign (cwd);

  Pipe syncPipe;
  syncPipe.create ();
  /* spi.cmdLine is used only under Windows and 
   * the fork server doesn't work there.  */
  ret = fork ();

  if (ret)
    {
      u_long nbw;
      writeInt (&sout, ret);
      writeInt (&sout, stdInPort);

      /* Synchronize with the child process.  It avoids that the
       * child process starts to write on `sout' before the process
       * information are sent back.  */
      syncPipe.write ("1", 1, &nbw);
      syncPipe.close ();
    }

  /* Code already present in process.cpp, refactoring needed.  */
  if (ret == 0) // child
    {
      u_long nbr;
      char syncB;
      const char *envp[100];
      const char *args[100];

      /* The parent process sent an ack when the child can start 
       * its execution.  */
      syncPipe.read (&syncB, 1, &nbr);
      syncPipe.close ();

      /* Close the fork server descriptor in the child process.  */
      serverSock->close ();
     
      if (Process::generateArgList (args, spi.cmd.c_str (), spi.arg))
        exit (1);

      if (Process::generateEnvString (envp, (char*) spi.envString))
        exit (1);
      
      if (spi.cwd.length ())
        {
          ret = chdir ((const char*)(spi.cwd.c_str()));
          if (ret == -1)
            exit (1);
        }
      if ((long)spi.stdIn == -1)
      {
        spi.stdIn = (FileHandle)open ("/dev/null", O_RDONLY);
      }

      if ((long)spi.stdIn == -1 || flags & FLAG_STDIN_SOCKET)
        sin.close ();

      if ((long)spi.stdOut == -1)
      {
        spi.stdOut = (FileHandle)open ("/dev/null", O_WRONLY);
        sout.close ();
      }

      ret = close(0);
      if (ret == -1)
        exit (1);
      ret = dup2(spi.stdIn, 0);
      if (ret == -1)
        exit (1);
      ret = close(spi.stdIn);
      if (ret == -1)
        exit (1);
      ret = close (1);
      if (ret == -1)
        exit (1);
    ret = dup2(spi.stdOut, 1);
    if (ret == -1)
      exit (1);
    ret = close (spi.stdOut);
    if (ret == -1)
      exit (1);

    execve ((const char*)args[0], 
            (char* const*)args, (char* const*) envp);
    exit (0);

  }

  delete [] exec;
  delete [] cwd;
  delete [] arg;
  delete [] env;

  return ret == -1 ? -1 : 0;
#endif
  return 0;
}

/*!
 *Entry point for the fork server.
 *Listen for new connections on the specified socket.
 *
 *\param socket Socket where wait for new connections.
 *\return 0 on success.
 */
int ForkServer::forkServerLoop (Socket *socket)
{
#ifdef NOT_WIN
  for (;;)
    {
      try
        {
          char command;
          u_long nbr;
          MYSERVER_SOCKADDR_STORAGE sockaddr;
          int len = sizeof (sockaddr);
          
          if (!socket->dataOnRead(5, 0))
            continue;
          
          Socket sin = socket->accept (&sockaddr, &len);
          Socket sout = socket->accept (&sockaddr, &len);
          
          //if (sin.getHandle () == -1 || sout.getHandle ())
          //  {
          //    continue;
          //  }
          
          if (sin.read (&command, 1, &nbr))
            {
              sin.shutdown (2);
              sin.close ();
              sout.shutdown (2);
              sout.close ();
              continue;
            }
        
          switch (command)
            {
            case 'e': //exit process
              exit (0);
              return 0;
            case 'r':
              if (handleRequest (sin, sout, socket))
                {
                  sin.shutdown (2);
                  sin.close ();
                  sout.shutdown (2);
                  sout.close ();
                }
            }
        }
      /* Don't let the fork server come back from this function 
         in _any_ case.  */
      catch(...)
        {
          perror ("fork server died.");
          exit (1);
        }
    }
#endif
  return 0;
}

/*!
 *Get a connection to the fork server on the specified sockets.
 *
 *\param sin Socket where obtain the input connection.
 *\param sout Socket where obtain the output connection.
 *\return 0 on success.
 */
int ForkServer::getConnection (Socket *sin, Socket *sout)
{
  int len = sizeof(sockaddr_in);
  MYSERVER_SOCKADDR_STORAGE sockaddr = { 0 };
  ((sockaddr_in*)(&sockaddr))->sin_family = AF_INET;
  ((sockaddr_in*)(&sockaddr))->sin_addr.s_addr = htonl (INADDR_LOOPBACK);
  ((sockaddr_in*)(&sockaddr))->sin_port = htons (port);

  sin->socket (AF_INET, SOCK_STREAM, 0);
  sout->socket (AF_INET, SOCK_STREAM, 0);
  
  serverLock.lock ();

  int ret = sin->connect ((MYSERVER_SOCKADDR*)&sockaddr, len);
  ret |= sout->connect ((MYSERVER_SOCKADDR*)&sockaddr, len) ;

  serverLock.unlock ();

  return ret;
}


/*!
 *Execute a process using the fork server.
 *\param spi New process information.
 *\param sin Socket to use for the process stdin.
 *\param sout Socket to use for the process stdout.
 *\param flags Flags.
 *\param pid The new process ID.
 *\param port if FLAG_STDIN_SOCKET was specified.
 */
int ForkServer::executeProcess (StartProcInfo *spi, Socket *sin, Socket *sout, 
                                int flags, int *pid, int *port)
{
  u_long nbw;
  int len = 0;
  const char * env = (const char *) spi->envString;

  if (getConnection (sin, sout))
    {
      return 1;
    }

  sin->write ("r", 1, &nbw);

  writeInt (sin, flags);
  writeInt (sin, spi->stdIn);
  writeInt (sin, spi->stdOut);
  writeInt (sin, spi->stdError);

  writeString (sin, spi->cmd.c_str (), spi->cmd.length () + 1);
  writeString (sin, spi->cwd.c_str (), spi->cwd.length () + 1);
  writeString (sin, spi->arg.c_str (), spi->arg.length () + 1);

  if (env)
    {
      for (len = 0; env[len] != '\0' || env[len + 1] != '\0' ; len++);
    }

  writeString (sin, env, len + 1);

  readInt (sout, pid);
  readInt (sout, port);

  return 0;
}

/*!
 *Terminate the fork server execution.
 */
void ForkServer::killServer ()
{
  Socket sin;
  Socket sout;
  u_long nbw;

  if (getConnection (&sin, &sout))
    {
      return;
    }

  sin.write ("e", 1, &nbw);
  sin.shutdown (2);
  sout.shutdown (2);
  sin.close ();
  sout.close ();
}
                  
/*!
 *Initialize the fork server.
 *
 *\return 0 on success.
 */
int ForkServer::startForkServer ()
{
#ifdef NOT_WIN
  Socket socket;

  if (generateListenerSocket (socket, &port))
    return 1;

  if (socket.listen (2))
    return -1;

  switch (fork ())
    {
    case -1:
      return -1;
    case 0:
      initialized = true;
      forkServerLoop (&socket);
      break;

    default:
      initialized = true;
      socket.close ();
      break;
    }  
#endif
  return 0;
}

/*!
 *Create a listener without specify a port.
 *
 *\param socket The socket to generate.
 *\param port the obtained port.
 */
int ForkServer::generateListenerSocket (Socket &socket, u_short *port)
{
  int optvalReuseAddr = 1;
  int len = sizeof(sockaddr_in);

  socket.socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

  if (socket.getHandle() == (FileHandle)INVALID_SOCKET)
    return -1;

  MYSERVER_SOCKADDR_STORAGE sockaddr = { 0 };
  ((sockaddr_in*)(&sockaddr))->sin_family = AF_INET;
  ((sockaddr_in*)(&sockaddr))->sin_addr.s_addr = htonl (INADDR_LOOPBACK);
  ((sockaddr_in*)(&sockaddr))->sin_port = 0;

  if (socket.bind (&sockaddr, sizeof (sockaddr_in)) != 0)
    return -1;

  if (socket.getsockname (&sockaddr, &len))
    return -1;

  *port = ntohs (((sockaddr_in*)(&sockaddr))->sin_port);

  if(socket.setsockopt (SOL_SOCKET, SO_REUSEADDR, (const char *)&optvalReuseAddr,
                        sizeof (optvalReuseAddr)) < 0)
    return -1;

  return 0;
}
