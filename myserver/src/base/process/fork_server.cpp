/*
  MyServer
  Copyright (C) 2008, 2009 Free Software Foundation, Inc.
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
#include <include/base/file/files_utility.h>
#include <include/base/utility.h>

#ifndef WIN32
extern "C"
{
#include <unistd.h>
#include <sys/wait.h>
}
#endif
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

  *out = new char[len + 1];
  (*out)[len] = '\0';

  if (len && (sock->read (*out, len, &nbr) || nbr < len))
    {
      delete [] *out;
      return -1;      
    }

  return 0;
}

/*!
 *Handle a request on the socket.
 */
int ForkServer::handleRequest (Socket *sock)
{
#ifndef WIN32
  int ret, flags, stdIn = -1, stdOut = -1, stdErr = -1;
  int gid, uid;
  int stdInPort = 0;
  char *exec;
  char *cwd;
  char *arg;
  char *env;

  readInt (sock, &flags);

  if (flags & FLAG_USE_IN)
    readFileHandle (sock->getHandle (), &stdIn);

  if (flags & FLAG_USE_OUT)
    readFileHandle (sock->getHandle (), &stdOut);
  
  if (flags & FLAG_USE_ERR)
    readFileHandle (sock->getHandle (), &stdErr);
  
  readInt (sock, &gid);
  readInt (sock, &uid);

  readString (sock, &exec);
  readString (sock, &cwd);

  readString (sock, &arg);
 
  string argS (arg);

  readString (sock, &env);

  Socket socketIn;

  if (flags & FLAG_STDIN_SOCKET)
    {
      u_short stdInPortS;
      if (generateListenerSocket (socketIn, &stdInPortS) ||
          socketIn.listen (SOMAXCONN))
        {
          delete [] exec;
          delete [] cwd;
          delete [] arg;
          writeInt (sock, -1);
          writeInt (sock, -1);
          return -1;
        }
      stdInPort = (int) stdInPortS;

      stdIn = socketIn.getHandle ();
      stdOut = stdErr = (FileHandle) -1;
    }

  StartProcInfo spi;

  spi.envString = (env && env[0]) ? env : NULL;

  spi.stdIn = stdIn;
  spi.stdOut = stdOut;
  spi.stdError = stdErr;

  spi.uid = uid;
  spi.gid = gid;

  spi.cmd.assign (exec);
  spi.arg.assign (arg);
  spi.cwd.assign (cwd);

  FileHandle handlesToClose[] = {0};
  spi.handlesToClose = handlesToClose;

  Process pi;
  int pid = pi.exec (&spi, false);

  writeInt (sock, pid);
  writeInt (sock, stdInPort);

  delete [] exec;
  delete [] cwd;
  delete [] arg;
  delete [] env;

  if (flags & FLAG_USE_IN)
    close (stdIn);

  if (flags & FLAG_USE_OUT)
    close (stdOut);
  
  if (flags & FLAG_USE_ERR)
    close (stdErr);

  return 0;
#endif
  return 0;
}

/*!
 *Entry point for the fork server.
 *Listen for new connections on the specified socket.
 *
 *\param serverSocket Socket where wait for new connections.
 *\return 0 on success.
 */
int ForkServer::forkServerLoop (UnixSocket *serverSocket)
{
#ifndef WIN32
  for (;;)
    {
      try
        {
          Socket socket = serverSocket->accept ();
 
          char command;
          u_long nbr;
          
          if (socket.read (&command, 1, &nbr))
            {
              socket.close ();
              continue;
            }
          switch (command)
            {
            case 'e': //exit process
              socket.close ();
              serverSocket->shutdown ();
              serverSocket->close ();
              exit (0);
              return 0;
            case 'r':
              if (handleRequest (&socket))
                {
                  socket.close ();
                  continue;
                }
            }
        }
      /* Don't let the fork server come back from this function 
         in _any_ case.  */
      catch(...)
        {
          serverSocket->close ();
          socket.close ();
          perror ("fork server died.");
          exit (1);
        }
    }
#endif
  return 0;
}

/*!
 *Execute a process using the fork server.
 *\param spi New process information.
 *\param flags Flags.
 *\param pid The new process ID.
 *\param port if FLAG_STDIN_SOCKET was specified.
 *\param waitEnd If true `executeProcess' will wait until
 *the process terminates.
 */
int ForkServer::executeProcess (StartProcInfo *spi, 
                                int flags, 
                                int *pid, 
                                int *port,
                                bool waitEnd)
{
#ifdef WIN32
  return 0;
#else
  u_long nbw;
  int len = 0;
  const char * env = (const char *) spi->envString;

  try
    {
      UnixSocket sock;
      sock.socket ();
      sock.connect (socketPath.c_str ());
      sock.write ("r", 1, &nbw);
      
      writeInt (&sock, flags);
      
      if (flags & FLAG_USE_IN)
        writeFileHandle (sock.getHandle (), spi->stdIn);
      
      if (flags & FLAG_USE_OUT)
        writeFileHandle (sock.getHandle (), spi->stdOut);
      
      if (flags & FLAG_USE_ERR)
        writeFileHandle (sock.getHandle (), spi->stdError);
      
      writeInt (&sock, spi->gid);
      writeInt (&sock, spi->uid);
      
      writeString (&sock, spi->cmd.c_str (), spi->cmd.length ());
      writeString (&sock, spi->cwd.c_str (), spi->cwd.length ());
      writeString (&sock, spi->arg.c_str (), spi->arg.length ());
      
      if (env)
        for (len = 0; env[len] != '\0' || env[len + 1] != '\0' ; len++);
      
      writeString (&sock, env, len);
      
      readInt (&sock, pid);
      readInt (&sock, port);
    }
  catch (exception &e)
    {
      throw e;
    }

  if (waitEnd)
    {
      return waitpid (*pid, NULL, 0);
    }
  
  return 0;
#endif

}

/*!
 *Terminate the fork server execution.
 */
void ForkServer::killServer ()
{
  u_long nbw;
  UnixSocket s;
  s.socket ();
  s.connect (socketPath.c_str ());
  s.write ("e", 1, &nbw);
  s.close ();
}
                  
/*!
 *Initialize the fork server.
 *
 *\return 0 on success.
 */
int ForkServer::startForkServer ()
{
#ifndef WIN32
  FilesUtility::temporaryFileName(0, socketPath);

  switch (fork ())
    {
    case -1:
      return -1;
    case 0:
      socket.socket ();
      socket.bind (socketPath.c_str ());
      socket.listen (SOMAXCONN);

      initialized = true;

      forkServerLoop (&socket);
      exit (1);
      break;

    default:
      initialized = true;
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
