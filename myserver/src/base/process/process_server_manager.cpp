/*
MyServer
Copyright (C) 2007, 2008, 2009 Free Software Foundation, Inc.
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
#include <include/server/server.h>
#include <include/base/process/process.h>
#include <include/base/process/process_server_manager.h>
#include <string>
#include <sstream>
using namespace std;

/*!
 *Default constructor.
 */
ProcessServerManager::ProcessServerManager ()
{
  mutex.init ();
  nServers = 0;
  maxServers = 25;
}

/*!
 *Default destructor.
 */
ProcessServerManager::~ProcessServerManager ()
{
  mutex.destroy ();
}

/*!
 *Load the class.
 */
void ProcessServerManager::load ()
{

  string key ("server.process_servers");
  NodeTree<string> *node = ::Server::getInstance ()->getNodeTree (key);

  if (node == NULL)
    return;

  string serverKey ("server");
  string domainKey ("domain");
  string hostKey ("host");
  string portKey ("port");
  string localKey ("local");
  string uidKey ("uid");
  string gidKey ("gid");
  string chrootKey ("chroot");

  list<NodeTree<string>*> *children = node->getChildren ();

  for (list<NodeTree<string>*>::iterator it = children->begin ();
      it != children->end ();
      it++)
    {
      string domain;
      string host;
      string name;
      string port;
      const char *chroot = NULL;
      string local;
      bool localBool = true;
      int uid = 0;
      int gid = 0;

      NodeTree<string>* n = *it;

      if (n->getAttr (serverKey))
        name.assign (*n->getAttr (serverKey));

      if (n->getAttr (domainKey))
        domain.assign (*n->getAttr (domainKey));

      if (n->getAttr (hostKey))
        host.assign (*n->getAttr (hostKey));

      if (n->getAttr (portKey))
        port.assign (*n->getAttr (portKey));

      if (n->getAttr (localKey))
        local.assign (*n->getAttr (localKey));

      if (n->getAttr (uidKey))
        uid = atoi (n->getAttr (uidKey)->c_str ());

      if (n->getAttr (gidKey))
        gid = atoi (n->getAttr (gidKey)->c_str ());

      if (n->getAttr (chrootKey))
        chroot = n->getAttr (chrootKey)->c_str ();

      if (!local.compare ("YES") || !local.compare ("yes"))
        localBool = true;
      else
        localBool = false;

      if (name.size () && domain.size ())
        {
          u_short portN = 0;

          if (port.size ())
            portN = atoi (port.c_str ());

          if (localBool)
            runAndAddServer (domain.c_str (), name.c_str (), chroot, uid, gid, portN);
          else
            {
              if (portN)
                addRemoteServer (domain.c_str (), name.c_str (), host.c_str (), portN);
              else
                ::Server::getInstance ()->log (MYSERVER_LOG_MSG_ERROR,
           _("Error: incomplete remote PROCESS_SERVER block, %s:%s needs a port"),
                                               domain.c_str (), name.c_str ());
            }
        }
      else
        ::Server::getInstance ()->log (MYSERVER_LOG_MSG_ERROR,
                                   _("Error: incomplete PROCESS_SERVER block"));
    }

}

/*!
 *Get a servers process domain by its name.
 *\param name The domain name.
 */
ProcessServerManager::ServerDomain*
ProcessServerManager::getDomain (const char* name)
{
  ServerDomain* ret;
  mutex.lock ();
  ret = domains.get (name);
  mutex.unlock ();
  return ret;
}

/*!
 *Create a new servers process domain by its name and return it.
 *\param name The domain name.
 */
ProcessServerManager::ServerDomain*
ProcessServerManager::createDomain (const char* name)
{
  ServerDomain* ret;

  mutex.lock ();

  ret = domains.get (name);
  if (!ret)
  {
    string str (name);
    ret = new ServerDomain ();
    domains.put (str, ret);
  }

  mutex.unlock ();

  return ret;
}

/*!
 *Clear the used memory.
 */
void ProcessServerManager::clear ()
{
  HashMap<string, ServerDomain*>::Iterator it;

  mutex.lock ();

  it = domains.begin ();

  for (;it != domains.end (); it++)
    {
      ServerDomain *sd = *it;
      HashMap<string, vector<Server*>*>::Iterator server = sd->servers.begin ();

      for (; server != sd->servers.end (); server++)
        {
          for (vector<Server*>::iterator it = (*server)->begin ();
               it != (*server)->end ();
               it++)
            {
              Server *s = *it;

              if (s->isLocal)
                nServers--;

              if (sd->clear)
                sd->clear (s);

              s->terminate ();
              delete s;
            }
          delete (*server);
        }
      delete (*it);
  }
  domains.clear ();
  mutex.unlock ();
}

/*!
 *Get a server is running by its domain and name.
 *\param domain The domain name.
 *\param name The server name name.
 *\param seed Random seed to use for choosing a server.
 */
ProcessServerManager::Server*
ProcessServerManager::getServer (const char* domain, const char* name, int seed)
{
  ServerDomain* sd;
  Server* s = NULL;

  mutex.lock ();
  sd = domains.get (domain);

  if (sd)
    {
      vector<Server*> *slist = sd->servers.get (name);
      if (slist)
        s = slist->at (seed % slist->size ());
    }

  if (s && s->isLocal && !s->process.isProcessAlive ())
  {
    s->socket.close ();
    s->process.terminateProcess ();
    if (!s->path.length ())
      s->path.assign (name);

    s->port = 0;

    if (runServer (s, s->path.c_str (), s->port))
      {
        sd->servers.remove (name);
        delete s;
        s = 0;
      }
  }

  mutex.unlock ();
  return s;
}

/*!
 *Add a server to the manager.
 *\param server The server object.
 *\param domain The server's domain.
 *\param name The server's name.
 */
void ProcessServerManager::addServer (ProcessServerManager::Server* server,
                                     const char* domain, const char* name)
{
  ServerDomain *sd = createDomain (domain);
  Server *old;
  string strName (name);

  mutex.lock ();
  vector<Server*>* slist = sd->servers.get (strName);

  if (slist == NULL)
    {
      slist = new vector<Server*> ();
      sd->servers.put (strName, slist);
    }

  slist->push_back (server);

  mutex.unlock ();
}

/*!
 *Add a remote server.
 *\param domain The server's domain name.
 *\param name The server name.
 *\param host The host name to connect to.
 *\param port The port number to use for the connection.
 */
ProcessServerManager::Server*
ProcessServerManager::addRemoteServer (const char* domain, const char* name,
                                       const char* host, u_short port)
{
  Server* server = new Server;
  server->path.assign (name);
  server->host.assign (host);
  server->port = port;
  server->isLocal = false;

  addServer (server, domain, name);
  return server;
}

/*!
 *Remove a domain by its name.
 *\param domain The domain name.
 */
void ProcessServerManager::removeDomain (const char* domain)
{
  ServerDomain *sd;
  HashMap<string, Server*>::Iterator server;

  mutex.lock ();

  sd = getDomain (domain);

  if (sd)
    {
      HashMap<string, vector<Server*>*>::Iterator server = sd->servers.begin ();

      for (; server != sd->servers.end (); server++)
        {
          for (vector<Server*>::iterator it = (*server)->begin ();
               it != (*server)->end ();
               it++)
            {
              Server *s = *it;

              if (s->isLocal)
                nServers--;

              if (sd->clear)
                sd->clear (s);

              s->terminate ();
              delete s;
            }
          delete (*server);
        }
      delete sd;
    }

  mutex.unlock ();
}

/*!
 *Count how many servers are present in a domain.
 *\param domain The server domain.
 */
int ProcessServerManager::domainServers (const char* domain)
{
  ServerDomain* serverDomain = getDomain (domain);
  return serverDomain ? serverDomain->servers.size () : 0;
}

/*!
 *Run and add a server to the collection.
 *\param domain The server's domain.
 *\param path The path to the executable.
 *\param chroot The new process chroot.
 *\param uid User id to use for the new process.
 *\param gid Group id to use for the new process.
 *\param port Port to use for the server.
 */
ProcessServerManager::Server*
ProcessServerManager::runAndAddServer (const char *domain, const char *path,
                                       const char *chroot, int uid, int gid,
                                       u_short port)
{
  Server* server = new Server;

  if (runServer (server, path, port, chroot, uid, gid))
  {
    delete server;
    return 0;
  }
  addServer (server, domain, path);
  return server;
}

/*!
 *Run a new server.
 *\param server The server object.
 *\param path The path to the executable.
 *\param port The listening port.
 *\param chroot The new process chroot.
 *\param uid User id to use for the new process.
 *\param gid Group id to use for the new process.
 */
int ProcessServerManager::runServer (ProcessServerManager::Server* server,
                                     const char* path, u_short port,
                                     const char *chroot, int uid, int gid)
{
  StartProcInfo spi;
  MYSERVER_SOCKADDRIN serverSockAddrIn;
  int addrLen = sizeof (serverSockAddrIn);

  server->host.assign ("localhost");
  server->isLocal = true;

  if (nServers >= maxServers)
    {
      ::Server::getInstance ()->log (MYSERVER_LOG_MSG_ERROR,
                      _("Cannot run process %s: Reached max number of servers"),
                                 path);
      return 1;
    }

  if (port)
    server->port = port;
  else
    server->port = 0;

  string tmpCgiPath;
  string moreArg;
  int subString = path[0] == '"';
  int i;
  int len = strlen (path);

  for (i = 1; i < len; i++)
    {
      if (!subString && path[i] == ' ')
        break;

      if (path[i] == '"' && path[i - 1] != '\\')
        subString = !subString;
    }

  if (i < len)
    {
      string tmpString (path);
      int begin = tmpString[0] == '"' ? 1 : 0;
      int end   = tmpString[i] == '"' ? i + 1 : i ;
      tmpCgiPath.assign (tmpString.substr (begin, end));
      moreArg.assign (tmpString.substr (i, len));
    }
  else
    {
      int begin = (path[0] == '"') ? 1 : 0;
      int end   = (path[len] == '"') ? len - 1 : len;
      tmpCgiPath.assign (&path[begin], end - begin);
      moreArg.assign ("");
    }

  spi.envString = 0;
  spi.stdOut = spi.stdError = (FileHandle) -1;

  spi.cmd.assign (tmpCgiPath);
  spi.arg.assign (moreArg);
  spi.cmdLine.assign (path);
  server->path.assign (path);
  if (chroot)
    spi.chroot.assign (chroot);

  spi.uid = uid;
  spi.gid = gid;

  if (Process::getForkServer ()->isInitialized ())
    {
      int ret, port, pid;
      ret = Process::getForkServer ()->executeProcess (&spi,
                                                       ForkServer::FLAG_STDIN_SOCKET,
                                                       &pid,
                                                       &port);

      if (ret == 0)
        {
          server->port = port;
          server->process.setPid (pid);
          server->DESCRIPTOR.fileHandle = 0;
          return 0;
        }

    }

  server->socket.socket (AF_INET, SOCK_STREAM, 0);

  if (server->socket.getHandle () == (Handle)INVALID_SOCKET)
    return 1;

  ((sockaddr_in *)(&serverSockAddrIn))->sin_family = AF_INET;

  ((sockaddr_in *)(&serverSockAddrIn))->sin_addr.s_addr =
    htonl (INADDR_LOOPBACK);
  ((sockaddr_in *)(&serverSockAddrIn))->sin_port =
    htons (server->port);

  if ( server->socket.bind (&serverSockAddrIn,
                            sizeof (sockaddr_in)) ||
       server->socket.listen (SOMAXCONN) )
    {
      server->socket.close ();
      return 1;
    }

  server->DESCRIPTOR.fileHandle = (unsigned long) server->socket.getHandle ();
  spi.stdIn = (FileHandle)server->DESCRIPTOR.fileHandle;

  if (server->socket.getsockname (&serverSockAddrIn, &addrLen))
    return -1;

  server->port = ntohs (((sockaddr_in *)&serverSockAddrIn)->sin_port);

  server->process.exec (&spi);
  server->socket.close ();

  return 0;
}

/*!
 *Get a client socket in the fCGI context structure
 *\param sock The socket to connect.
 *\param server The server to connect to.
 */
int ProcessServerManager::connect (Socket* sock,
                                  ProcessServerManager::Server* server )
{
  MYSERVER_SOCKADDRIN serverSock = { 0 };
  socklen_t nLength = sizeof (MYSERVER_SOCKADDRIN);

  if (server->socket.getHandle ())
    server->socket.getsockname ((MYSERVER_SOCKADDR*)&serverSock, (int*)&nLength);

  if (!serverSock.ss_family || serverSock.ss_family == AF_INET || !server->isLocal)
  {
    /*! Try to create the socket.  */
    if (sock->socket (AF_INET, SOCK_STREAM, 0) == -1)
      return -1;

    /*! If the socket was created try to connect.  */
    if (sock->connect (server->host.c_str (), server->port) == -1)
    {
      sock->close ();
      return -1;
    }

  }
#if ( HAVE_IPV6 && false )/* IPv6 communication not implemented yet by php.  */
  else if ( serverSock.ss_family == AF_INET6 )
  {
    /*! Try to create the socket.  */
    if (sock->socket (AF_INET6, SOCK_STREAM, 0) == -1)
      return -1;
    /*! If the socket was created try to connect.  */
    if (sock->connect (server->host, server->port) == -1)
    {
      sock->close ();
      return -1;
    }
  }
#endif // HAVE_IPV6
  sock->setNonBlocking (1);

  return 0;
}
