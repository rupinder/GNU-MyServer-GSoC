/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010
  Free Software Foundation, Inc.
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

#include <include/protocol/http/http.h>
#include <include/protocol/http/http_headers.h>
#include <include/protocol/http/http_internal_exception.h>
#include <include/protocol/http/http_req_security_domain.h>
#include <include/server/server.h>
#include <include/conf/security/security_manager.h>
#include <include/conf/security/auth_domain.h>
#include <include/base/base64/mime_utils.h>
#include <include/base/file/file.h>
#include <include/base/file/files_utility.h>
#include <include/server/clients_thread.h>
#include <include/base/socket/socket.h>
#include <include/base/utility.h>
#include <include/base/crypt/md5.h>
#include <include/base/string/stringutils.h>
#include <include/base/string/securestr.h>

#include <include/http_handler/cgi/cgi.h>
#include <include/http_handler/wincgi/wincgi.h>
#include <include/http_handler/fastcgi/fastcgi.h>
#include <include/http_handler/scgi/scgi.h>
#include <include/http_handler/mscgi/mscgi.h>
#include <include/http_handler/isapi/isapi.h>
#include <include/http_handler/http_file/http_file.h>
#include <include/http_handler/proxy/proxy.h>
#include <include/http_handler/http_dir/http_dir.h>
#include <include/protocol/http/http_data_read.h>
#include <include/base/exceptions/checked.h>

#include <string>
#include <ostream>
#include <errno.h>

using namespace std;

int HttpProtocol::loadProtocol ()
{
  const char *data = NULL;

  HttpErrors::load ();

  timeout = MYSERVER_SEC (15);
  dynManagerList.addHttpManager ("SEND", new HttpFile ());
  dynManagerList.addHttpManager ("DIR", new HttpDir ());
  dynManagerList.addHttpManager ("CGI", new Cgi ());
  dynManagerList.addHttpManager ("MSCGI", new MsCgi ());
  dynManagerList.addHttpManager ("SCGI", new Scgi ());
  dynManagerList.addHttpManager ("WINGI", new WinCgi ());
  dynManagerList.addHttpManager ("FASTCGI", new FastCgi ());
  dynManagerList.addHttpManager ("ISAPI", new Isapi ());
  dynManagerList.addHttpManager ("PROXY", new Proxy ());

  data = Server::getInstance ()->getData ("vhost.allow_mime");
  if (data)
    {

      if (! strcasecmp (data, "YES"))
        allowVhostMime = 1;
      else
        allowVhostMime = 0;
    }

  data = Server::getInstance ()->getData ("cgi.timeout");
  if (data)
    timeout = MYSERVER_SEC (atoi (data));

  return 1;
}

int HttpProtocol::unLoadProtocol ()
{
  HttpErrors::unLoad ();
  clearMulticastRegistry ();
  dynManagerList.clear ();
  dynCmdManager.clear ();
  return 0;
}

/*!
  Build a response for an OPTIONS request.
 */
int Http::optionsHTTPRESOURCE (string& filename, bool yetmapped)
{
  int ret;
  string time;
  int permissions;

  try
    {
      HttpRequestHeader::Entry *connection = td->request.other.get ("connection");
      string methods ("OPTIONS, GET, POST, HEAD, DELETE, PUT, TRACE, MKCOL, PROPFIND, COPY, MOVE, LOCK, UNLOCK");

      HashMap<string, DynamicHttpCommand*>::Iterator it =
        staticData->getDynCmdManager ()->begin ();
      while (it != staticData->getDynCmdManager ()->end ())
        {
          methods.append (", ");
          methods.append ((*it)->getName ());
          it++;
        }

      ret = Http::preprocessHttpRequest (filename, yetmapped, &permissions, false);
      if (ret != 200)
        return raiseHTTPError (ret);

      getRFC822GMTTime (time, 32);
      td->auxiliaryBuffer->setLength (0);
      *td->auxiliaryBuffer << "HTTP/1.1 200 OK\r\n";
      *td->auxiliaryBuffer << "Date: " << time;
      *td->auxiliaryBuffer << "\r\nServer: GNU MyServer " << MYSERVER_VERSION;
      if (connection && connection->value.length ())
        *td->auxiliaryBuffer << "\r\nConnection:" << connection->value.c_str () << "\r\n";
      *td->auxiliaryBuffer << "Content-length: 0\r\nAccept-Ranges: bytes\r\n";
      *td->auxiliaryBuffer << "Allow: " << methods << "\r\n\r\n";
      td->connection->socket->send (td->auxiliaryBuffer->getBuffer (),
                                    td->auxiliaryBuffer->getLength (), 0);
      return 1;
    }
  catch (exception & e)
    {
      td->connection->host->warningsLogWrite (_E ("HTTP: internal error"), &e);
      return raiseHTTPError (500);
    };
}

/*!
  Handle the HTTP TRACE command.
 */
int Http::traceHTTPRESOURCE (string& filename, bool yetmapped)
{
  int ret;
  char tmpStr[12];
  int contentLength = (int) td->nHeaderChars;
  string time;
  int permissions;
  try
    {
      MemBuf tmp;
      HttpRequestHeader::Entry *connection;

      ret = Http::preprocessHttpRequest (filename, yetmapped, &permissions, false);
      if (ret != 200)
        return raiseHTTPError (ret);

      tmp.intToStr (contentLength, tmpStr, 12);
      getRFC822GMTTime (time, 32);

      td->auxiliaryBuffer->setLength (0);
      *td->auxiliaryBuffer << "HTTP/1.1 200 OK\r\n";
      *td->auxiliaryBuffer << "Date: " << time << "\r\n";
      *td->auxiliaryBuffer << "Server: GNU MyServer " << MYSERVER_VERSION << "\r\n";
      connection = td->request.other.get ("connection");
      if (connection && connection->value.length ())
        *td->auxiliaryBuffer << "Connection:" << connection->value.c_str () << "\r\n";

      *td->auxiliaryBuffer << "Content-length:" << tmp << "\r\n"
              << "Content-type: message/http\r\n"
              << "Accept-Ranges: bytes\r\n\r\n";

      /* Send our HTTP header.  */
      ret = td->connection->socket->send (td->auxiliaryBuffer->getBuffer (),
                                          (u_long) td->auxiliaryBuffer->getLength (), 0);
      if (ret < 0)
        {
          td->connection->host->warningsLogWrite (_("HTTP: socket error"));
          return 0;
        }

      /* Send the client request header as the HTTP payload.  */
      td->connection->socket->send (td->buffer->getBuffer (), contentLength, 0);
      return 1;
    }
  catch (exception & e)
    {
      td->connection->host->warningsLogWrite (_E ("HTTP: internal error"), &e);
      return raiseHTTPError (500);
    };
}

/*!
  Check if the method is allowed.
  \param method The HTTP method name.
  \return true if it is allowed.
 */
bool Http::allowMethod (const char *method)
{
  char name[64];
  sprintf (name, "http.%s.allow", method);
  const char *allow = td->securityToken.getData (name,
                                                 MYSERVER_VHOST_CONF |
                                                 MYSERVER_SERVER_CONF, "YES");

  return strcasecmp (allow, "NO");
}

/*!
  Get the timeout for the cgi.
 */
u_long Http::getTimeout ()
{
  return staticData->getTimeout ();
}

/*!
  Main function to handle the HTTP PUT command.
 */
int Http::putHTTPRESOURCE (string& filename, bool sysReq, bool onlyHeader,
                           bool yetmapped)
{
  return sendHTTPResource (filename, sysReq, onlyHeader, yetmapped);
}

/*!
  Get the file permissions mask.
  \param resource Resource to access.
  \param directory Directory where the resource is.
  \param file The file specified by the resource.
  \param filenamePath Complete path to the file.
  \param yetmapped Is the resource mapped to the localfilesystem?
  \param permissions Permission mask for this resource.
  \return Return 200 on success.
  \return Any other value is the HTTP error code.
 */
int Http::getFilePermissions (string& resource, string& directory, string& file,
                              string &filenamePath, bool yetmapped, int* permissions)
{
  try
    {
      const string *sysdir = &td->connection->host->getSystemRoot ();
      td->securityToken.setServer (Server::getInstance ());
      td->securityToken.setSysDirectory (sysdir);

      td->securityToken.setVhost (td->connection->host);

      FilesUtility::splitPath (resource, directory, file);
      FilesUtility::completePath (directory);

      td->securityToken.setResource (&filenamePath);
      td->securityToken.setDirectory (&directory);

      /*
        td->filenamePath is the file system mapped path while filename
        is the uri requested.
        systemrequest is 1 if the file is in the system directory.
        If filename is already mapped on the file system don't map it again.
       */
      if (yetmapped)
        filenamePath.assign (resource);
      else
        {
          int ret;
          /*
            If the client tries to access files that aren't in the web directory
            send a HTTP 401 error page.
           */
          translateEscapeString (resource);
          if ((resource[0] != '\0')
              && (FilesUtility::getPathRecursionLevel (resource) < 1))
            return 401;

          ret = getPath (filenamePath, resource, 0);
          if (ret != 200)
            return ret;
        }

      bool isDirectory = false;
      try
        {
          isDirectory = FilesUtility::isDirectory (filenamePath.c_str ());
        }
      catch (FileNotFoundException & e)
        {}

      if (isDirectory)
        directory.assign (filenamePath);
      else
        FilesUtility::splitPath (filenamePath, directory, file);

      if (td->connection->protocolBuffer == NULL)
        {
          td->connection->protocolBuffer = new HttpUserData;
          if (!td->connection->protocolBuffer)
            {
              td->connection->host->warningsLogWrite (_("HTTP: internal error"));
              return 500;
            }
          static_cast<HttpUserData*> (td->connection->protocolBuffer)->reset ();
        }

      string user;
      string password;

      if (td->request.auth.length ())
        {
          user.assign (td->connection->getLogin ());
          password.assign (td->connection->getPassword ());
        }
      else
        {
          /* The default user is Guest with a null password.  */
          user.assign ("Guest");
          password.assign ("");
        }
      if (*permissions == -1)
        {
          td->connection->host->warningsLogWrite (
                                _("HTTP: error accessing the security file"));
          return 500;
        }

      td->securityToken.setUser (user);
      td->securityToken.setPassword (password);

      AuthDomain auth (&(td->securityToken));
      HttpReqSecurityDomain httpReqSecDom (&(td->request));

      string validator (td->securityToken.getData ("sec.validator", MYSERVER_VHOST_CONF
                                                         | MYSERVER_SERVER_CONF, "xml"));
      string authMethod (td->securityToken.getData ("sec.auth_method", MYSERVER_VHOST_CONF
                                                         | MYSERVER_SERVER_CONF, "xml"));
      SecurityDomain * domains[] = {&auth, &httpReqSecDom, NULL};

      Server::getInstance ()->getSecurityManager ()->getPermissionMask (&(td->securityToken),
                                                         domains, validator, authMethod);

      const char *authType = td->securityToken.getData ("http.auth",
           MYSERVER_SECURITY_CONF | MYSERVER_VHOST_CONF | MYSERVER_SERVER_CONF);
      *permissions = td->securityToken.getMask ();

      /* Check if we have to use digest for the current directory.  */
      if (authType && ! strcasecmp (authType, "Digest"))
        {
          HttpUserData* hud = static_cast<HttpUserData*>
                                               (td->connection->protocolBuffer);

          if (!td->request.auth.compare ("Digest"))
            {
              if (!hud->digestChecked)
                {
                  try
                    {
                      hud->digest = checkDigest ();
                    }
                  catch (exception & e)
                    {
                      td->connection->host->warningsLogWrite (e.what ());
                      return raiseHTTPError (500);
                    }
                }

              hud->digestChecked = 1;
              if (hud->digest == 1)
                {
                  string &pwd = td->securityToken.getNeededPassword ();
                  td->connection->setPassword (pwd.c_str ());
                  *permissions = td->securityToken.getProvidedMask ();
                }
            }
          else
            *permissions = 0;

          td->authScheme = HTTP_AUTH_SCHEME_DIGEST;
        }
      /* By default use the Basic authentication scheme. */
      else
        td->authScheme = HTTP_AUTH_SCHEME_BASIC;
    }
  catch (FileNotFoundException & e)
    {
      return 404;
    }
  catch (exception & e)
    {
      td->connection->host->warningsLogWrite (
                                 _E ("HTTP: cannot get permissions for %s"),
                                 resource.c_str (), &e);
      return 500;
    }

  const char *tr = td->securityToken.getData ("connection.throttling",
                                              MYSERVER_SECURITY_CONF
                                              | MYSERVER_VHOST_CONF
                                              | MYSERVER_SERVER_CONF);

  /* If a throttling rate was specifed use it.  */
  if (tr)
    td->connection->socket->setThrottling (atoi (tr));

  return 200;
}

/*!
  Preprocess a HTTP request.
  \param resource Resource to access.
  \param yetmapped Is the resource mapped to the localfilesystem?
  \param permissions Permission mask for this resource.
  \param system Is it a system request?
  \return Return 200 on success.
  \return Any other value is the HTTP error code.
 */
int Http::preprocessHttpRequest (string &resource, bool yetmapped,
                                 int* permissions, bool systemrequest)
{
  string directory;
  string file;
  size_t filenamePathLen;
  string dirscan;
  int ret;
  size_t splitPoint = string::npos;

  try
    {
      if (td->request.isKeepAlive ())
        td->response.setValue ("connection", "keep-alive");

      ret = getFilePermissions (resource, directory, file,
                                td->filenamePath, yetmapped, permissions);
      if (ret != 200)
        return ret;

      /*
        Get the PATH_INFO value.
        Use dirscan as a buffer for put temporary directory scan.
        When an '/' character is present check if the path up to '/' character
        is a file. If it is a file send the rest of the uri as PATH_INFO.
       */
      td->pathInfo.assign ("");
      td->pathTranslated.assign ("");
      filenamePathLen = td->filenamePath.length ();
      dirscan.assign ("");

      MimeRecord* mimeLoc = NULL;
      size_t resOffset = systemrequest ?
        td->connection->host->getSystemRoot ().length ()
        : td->connection->host->getDocumentRoot ().length ();

      td->pathInfo = "";
      for (size_t i = resOffset;;)
        {
          /*
            http://host/path/to/file/file.txt/PATH_INFO_VALUE?QUERY_INFO_VALUE
            When a request has this form send the file file.txt with the
            environment string PATH_INFO equals to PATH_INFO_VALUE and QUERY_INFO
            to QUERY_INFO_VALUE.
           *
            If there is the '/' character check if dirscan is a file.
           */

          size_t next = td->filenamePath.find ('/', i + 1);
          string curr = td->filenamePath.substr (0, next);
          const string &resource = curr.substr (resOffset);
          mimeLoc = td->connection->host
            ? td->connection->host->getLocationMime (resource) : NULL;
          if (mimeLoc)
            {
              if (next != string::npos)
                {
                  td->pathInfo.assign (&(td->filenamePath.c_str ()[next]));
                  td->filenamePath.erase (next);
                }

              break;
            }

          if (next >= filenamePathLen)
            break;

          /* Don't exit immediately if we find a non directory element, a
             location handler can be registered after it.  */
          if (splitPoint && FilesUtility::notDirectory (curr.c_str ()))
            splitPoint = next;

          i = next;
        }

      if (mimeLoc == NULL)
        {
          if (splitPoint == string::npos)
            {
              if (! FilesUtility::nodeExists (td->filenamePath.c_str ()))
                return 404;
            }
          else
            {
              td->pathInfo.assign (&(td->filenamePath.c_str ()[splitPoint]));
              td->filenamePath.erase (splitPoint);
            }
        }

      /*
        PATH_TRANSLATED is the local filesystem mapped version of PATH_INFO.
       */
      if (td->pathInfo.length () <= 1)
        td->pathTranslated.assign ("");
      else
        {
          int ret;
          /* Omit the first slash character.  */
          ret = getPath (td->pathTranslated, &((td->pathInfo.c_str ())[1]), 0);
          if (ret != 200)
            td->pathTranslated.assign ("");
          else
            FilesUtility::completePath (td->pathTranslated);
        }

      FilesUtility::completePath (td->filenamePath);

      td->mime = mimeLoc ? mimeLoc : getMIME (td->filenamePath);
    }
  catch (exception & e)
    {
      td->connection->host->warningsLogWrite (_E ("HTTP: internal error"), &e);
      return 500;
    }
  catch (...)
    {
      td->connection->host->warningsLogWrite (_("HTTP: internal error"));
      return 500;
    }

  return 200;
}

/*!
  Check the Digest authorization

  \return 1 if the client credentials are OK.
  \return 0 if the credentials are wrong.
 */
u_long Http::checkDigest ()
{
  Md5 md5;
  char A1[48];
  char A2[48];
  char response[48];
  const char *uri;
  u_long digestCount;
  HttpUserData *hud =
    static_cast<HttpUserData*>(td->connection->protocolBuffer);

  if (td->request.digestOpaque[0]
      && strcmp (td->request.digestOpaque, hud->opaque))
    return 0;

  if (strcmp (td->request.digestRealm, hud->realm))
    return 0;

  digestCount = hexToInt (td->request.digestNc);
  if (digestCount != hud->nc + 1)
    return 0;
  else
    hud->nc++;

  string &algorithm = td->securityToken.getAlgorithm ();

  if (algorithm.length () == 0)
    {
      md5.init ();
      td->auxiliaryBuffer->setLength (0);
      *td->auxiliaryBuffer << td->request.digestUsername << ":"
                           << td->request.digestRealm
                           << ":" << td->securityToken.getNeededPassword ();

      md5.update (*td->auxiliaryBuffer);
      md5.end (A1);
    }
  else if (algorithm.compare ("a1") == 0)
    {
      strcpy (A1, td->securityToken.getNeededPassword ().c_str ());
    }
  else
    {
      string err (_("HTTP: internal error, when using digest auth only "\
                    "a1 and cleartext passwords can be used"));

      throw HttpInternalException (err);
    }

  md5.init ();

  if (td->request.digestUri[0])
    uri = td->request.digestUri;
  else
    uri = td->request.uriOpts.c_str ();

  td->auxiliaryBuffer->setLength (0);
  *td->auxiliaryBuffer << td->request.cmd.c_str () << ":" << uri;
  md5.update (*td->auxiliaryBuffer);
  md5.end (A2);

  md5.init ();
  td->auxiliaryBuffer->setLength (0);
  *td->auxiliaryBuffer << A1 << ":"
                       << hud->nonce << ":"
                       << td->request.digestNc << ":"
                       << td->request.digestCnonce << ":"
                       << td->request.digestQop << ":" << A2;
  md5.update (*td->auxiliaryBuffer);
  md5.end (response);

  return strcmp (response, td->request.digestResponse) ? 0 : 1;
}

/*!
  Create the buffer.
 */
HttpUserData::HttpUserData ()
{
  reset ();
}

/*!
  Destroy the buffer.
 */
HttpUserData::~HttpUserData () { }

/*!
  Reset the structure.
 */
void
HttpUserData::reset ()
{
  realm[0] = '\0';
  opaque[0] = '\0';
  nonce[0] = '\0';
  cnonce[0] = '\0';
  digestChecked = 0;
  nc = 0;
  digest = 0;
}

/*!
  Main function to send a resource to a client.
 */
int
Http::sendHTTPResource (string& uri, bool systemrequest, bool onlyHeader,
                        bool yetmapped)
{
  /*
    With this code we manage a request of a file or a directory or anything
    that we must send over the HTTP.
   */
  string filename;
  const char *cgiManager;
  int ret;
  HttpDataHandler *manager;

  /* By default allows only few actions.  */
  td->permissions = MYSERVER_PERMISSION_READ | MYSERVER_PERMISSION_BROWSE;

  try
    {
      filename.assign (uri);
      td->buffer->setLength (0);

      ret = Http::preprocessHttpRequest (filename, yetmapped, &td->permissions,
                                         systemrequest);
      if (ret != 200)
        return raiseHTTPError (ret);

      if (systemrequest)
        td->filenamePath.assign (uri);

      bool isDirectory = false;

      try
        {
          isDirectory = FilesUtility::isDirectory (td->filenamePath.c_str ());
        }
      catch (FileNotFoundException & e)
        {}

      if (!td->mime && isDirectory)
        return processDefaultFile (uri, td->permissions, onlyHeader);

      /* If not specified differently, set the default content type to text/html.  */
      if (td->mime)
        {
          td->response.setValue ("content-type", td->mime->mimeType.c_str ());
          cgiManager = td->mime->cgiManager.c_str ();
        }
      else
        {
          td->response.setValue ("content-type", "text/html");
          cgiManager = "";
        }

      if (td->mime && (manager =
          staticData->getDynManagerList ()->getHttpManager (td->mime->cmdName)))
        return manager->send (td, td->filenamePath.c_str (), cgiManager,
                              td->mime->selfExecuted, onlyHeader);

      if (!(td->permissions & MYSERVER_PERMISSION_READ))
        return sendAuth ();

      manager = staticData->getDynManagerList ()->getHttpManager ("SEND");
      if (!manager)
        {
          td->connection->host->warningsLogWrite (_("HTTP: internal error"));
          return raiseHTTPError (500);
        }

      return manager->send (td, td->filenamePath.c_str (), NULL, false, onlyHeader);
    }
  catch (exception & e)
    {
      td->connection->host->warningsLogWrite (_E ("HTTP: internal error"), &e);
      return raiseHTTPError (500);
    }

  return HttpDataHandler::RET_OK;
}

/*!
  Log the access using the Common Log Format or the Combined one.
 */
int Http::logHTTPaccess ()
{
  char tmpStrInt[12];

  string time;

  try
    {
      td->auxiliaryBuffer->setLength (0);
      *td->auxiliaryBuffer << td->connection->getIpAddr ();
      *td->auxiliaryBuffer << " ";

      if (td->connection->getLogin ()[0])
        *td->auxiliaryBuffer << td->connection->getLogin ();
      else
        *td->auxiliaryBuffer << "-";

      *td->auxiliaryBuffer << " ";

      if (td->connection->getLogin ()[0])
        *td->auxiliaryBuffer << td->connection->getLogin ();
      else
        *td->auxiliaryBuffer << "-";

      *td->auxiliaryBuffer << " [";

      getLocalLogFormatDate (time, 32);
      *td->auxiliaryBuffer << time << "] \"";

      if (td->request.cmd.length ())
        *td->auxiliaryBuffer << td->request.cmd.c_str () << "";

      if (td->request.cmd.length () || td->request.uri.length ())
        *td->auxiliaryBuffer << " ";

      if (td->request.uri.length () == '\0')
        *td->auxiliaryBuffer << "/";
      else
        *td->auxiliaryBuffer << td->request.uri.c_str ();

      if (td->request.uriOpts.length ())
        *td->auxiliaryBuffer << "?" << td->request.uriOpts.c_str ();

      sprintf (tmpStrInt, "%u ", td->response.httpStatus);

      if (td->request.ver.length ())
        *td->auxiliaryBuffer << " " << td->request.ver.c_str ();

      *td->auxiliaryBuffer << "\" " << tmpStrInt << " ";

      sprintf (tmpStrInt, "%u", td->sentData);
      *td->auxiliaryBuffer << tmpStrInt;

      if (td->connection->host)
        {
          HttpRequestHeader::Entry *userAgent = td->request.other.get ("user-agent");
          HttpRequestHeader::Entry *referer = td->request.other.get ("refer");

          if (strstr ((td->connection->host)->getAccessLogOpt (), "type=combined"))
            *td->auxiliaryBuffer << " " << (referer ? referer->value.c_str () : "")
            << " " << (userAgent ? userAgent->value.c_str () : "");
        }

      *td->auxiliaryBuffer << end_str;

      /*
        Request the access to the log file then append the message.
       */
      if (td->connection->host)
        {
          const char *msg = td->auxiliaryBuffer->getBuffer ();
          td->connection->host->accessesLogWrite ("%s", msg);
        }

      td->auxiliaryBuffer->setLength (0);
    }
  catch (...)
    {
      return HttpDataHandler::RET_FAILURE;
    }
  return 0;
}

/*!
  This is the HTTP protocol main procedure to parse a request over HTTP.
 */
int Http::controlConnection (ConnectionPtr a, char*, char*, u_long, u_long,
                             u_long nbtr, u_long id)
{
  int ret = -1;
  int validRequest;

  /* Dimension of the POST data. */
  int contentLength = -1;
  DynamicHttpCommand *dynamicCommand;
  bool keepalive = false;
  bool pipelineData = false;

  try
    {
      td->buffer = a->getActiveThread ()->getBuffer ();
      td->auxiliaryBuffer = a->getActiveThread ()->getAuxiliaryBuffer ();
      td->buffersize = a->getActiveThread ()->getBufferSize ();
      td->nBytesToRead = nbtr;
      td->connection = a;
      td->id = id;
      td->lastError = 0;
      td->http = this;
      td->appendOutputs = false;
      td->onlyHeader = false;
      td->filenamePath.assign ("");
      td->mime = NULL;
      td->headerSent = false;
      td->sentData = 0;
      td->vhostDir.assign ("");
      td->vhostSys.assign ("");
      HashMap<string, string*>::Iterator it = td->other.begin ();
      while (it != td->other.end ())
        delete (*it);
      td->other.clear ();

      HttpHeaders::resetHTTPRequest (&td->request);
      HttpHeaders::resetHTTPResponse (&td->response);

      HttpHeaders::buildDefaultHTTPResponseHeader (&(td->response));

      /* Reset the HTTP status once per request. */
      td->response.httpStatus = 200;

      /* The connection must be removed in a soft-way.  The connections
         scheduler sets this flag.  */
      if (td->connection->getToRemove ())
        {
          switch (td->connection->getToRemove ())
            {
              /* Remove the connection from the list.  */
            case Connection::REMOVE_OVERLOAD:
              ret = raiseHTTPError (503);
              logHTTPaccess ();
              return ClientsThread::DELETE_CONNECTION;

            default:
              logHTTPaccess ();
              return ClientsThread::DELETE_CONNECTION;
            }
        }

      validRequest =
        HttpHeaders::buildHTTPRequestHeaderStruct (td->buffer->getBuffer (),
                                                   td->buffer->getRealLength (),
                                                   &(td->nHeaderChars),
                                                   &(td->request),
                                                   td->connection);

      /* -1 means the request is not complete yet.  */
      if (validRequest == -1)
        return ClientsThread::INCOMPLETE_REQUEST;

      if (a->protocolBuffer)
        ((HttpUserData *) a->protocolBuffer)->digestChecked = 0;

      /* If the validRequest cointains an error code send it to the user.  */
      if (validRequest != 200)
        {
          ret = raiseHTTPError (validRequest);
          logHTTPaccess ();
          return ClientsThread::DELETE_CONNECTION;
        }

      /* Be sure that we can handle the HTTP version.  */
      if ((td->request.ver.compare ("HTTP/1.1")) &&
          (td->request.ver.compare ("HTTP/1.0")) &&
          (td->request.ver.compare ("HTTP/0.9")))
        {
          raiseHTTPError (505);
          logHTTPaccess ();
          /* Remove the connection from the list.  */
          return ClientsThread::DELETE_CONNECTION;
        }

      td->response.ver.assign (td->request.ver.c_str ());

      dynamicCommand =
        staticData->getDynCmdManager ()->getHttpCommand (td->request.cmd);

      /* If the used method supports POST data, read it.  */
      if ((!td->request.cmd.compare ("POST")) ||
          (!td->request.cmd.compare ("PUT")) ||
          (!td->request.cmd.compare ("LOCK")) ||
          (!td->request.cmd.compare ("PROPFIND")) ||
          (dynamicCommand && dynamicCommand->acceptData ()))
        {
          int httpErrorCode;
          int readPostRet;
          /* Be sure that the client can handle the 100 status code.  */
          if (nbtr == td->nHeaderChars && td->request.contentLength.compare ("0")
              && td->request.ver.compare ("HTTP/1.0"))
            {
              const char* msg = "HTTP/1.1 100 Continue\r\n\r\n";
              if (a->socket->bytesToRead () == 0)
                a->socket->send (msg, (int) strlen (msg), 0);

              return ClientsThread::INCOMPLETE_REQUEST;
            }

          readPostRet = HttpDataRead::readPostData (td, &httpErrorCode);
          if (readPostRet < 0)
            {
              logHTTPaccess ();
              return ClientsThread::DELETE_CONNECTION;
            }
          else if (readPostRet)
            ret = raiseHTTPError (httpErrorCode);
        }
      else
        {
          contentLength = 0;
          td->request.uriOptsPtr = 0;
        }

      /* If return value is not configured properly.  */
      if (ret < 0)
        {
          /*
            How is expressly said in the RFC2616 a client that sends an
            HTTP/1.1 request MUST sends a Host header.
            Servers MUST reports a 400 (Bad request) error if an HTTP/1.1
            request does not include a Host request-header.
           */
          HttpRequestHeader::Entry *host = td->request.other.get ("host");
          HttpRequestHeader::Entry *connection
            = td->request.other.get ("connection");
          if (connection)
            keepalive = !stringcmpi (connection->value.c_str (), "keep-alive")
              && !td->request.ver.compare ("HTTP/1.1");

          if (! td->request.ver.compare ("HTTP/1.1")
              && (host == NULL || host->value.length () == 0))
            {
              int ret = raiseHTTPError (400);
              logHTTPaccess ();
              if (ret == HttpDataHandler::RET_OK && keepalive)
                return ClientsThread::KEEP_CONNECTION;
              else
                return ClientsThread::DELETE_CONNECTION;
            }

          /* Find the virtual host to check both host name and IP value.  */
          Vhost* newHost = Server::getInstance ()->getVhosts ()->getVHost (host ?
                                                       host->value.c_str () : "",
                                         a->getLocalIpAddr (), a->getLocalPort ());
          if (a->host)
            a->host->removeRef ();

          a->host = newHost;
          if (a->host == NULL)
            {
              int ret = raiseHTTPError (400);
              logHTTPaccess ();
              if (ret == HttpDataHandler::RET_OK && keepalive)
                return ClientsThread::KEEP_CONNECTION;
              else
                return ClientsThread::DELETE_CONNECTION;
            }

          if (td->request.uri.length () > 2 && td->request.uri[1] == '~')
            {
              string documentRoot;
              u_long pos = 2;
              string user;
              while (pos < td->request.uri.length ())
                if (td->request.uri[++pos] == '/')
                  break;

              user.assign (td->request.uri.substr (2, pos - 2));
              Server::getInstance ()->getHomeDir ()->getHomeDir (user,
                                                                 documentRoot);

              if (documentRoot.length ())
                {
                  const char *useHomeDir = td->securityToken.getData
                    ("http.use_home_directory", MYSERVER_VHOST_CONF
                     | MYSERVER_SERVER_CONF, "YES");

                  const char *homeDir = td->securityToken.getData
                    ("http.home_directory", MYSERVER_VHOST_CONF
                     | MYSERVER_SERVER_CONF, "public_html");

                  if (strcasecmp (useHomeDir, "YES"))
                    return raiseHTTPError (404);

                  td->vhostDir.assign (documentRoot);
                  td->vhostDir.append ("/");
                  td->vhostDir.append (homeDir);

                  if (!td->request.uriEndsWithSlash
                      && !(td->request.uri.length () - pos))
                    {
                      td->request.uri.append ("/");
                      return sendHTTPRedirect (td->request.uri.c_str ());
                    }

                  if (td->request.uri.length () - pos)
                    td->request.uri.assign (td->request.uri.substr (pos,
                                                  td->request.uri.length ()));
                  else
                    td->request.uri.assign ("");
                }
            }

          /* Support for HTTP pipelining.  */
          if (contentLength == 0)
            {
              /*  connectionBuffer is 8 KB, so don't copy more bytes.  */
              u_long bufferStrLen = strlen (td->buffer->getBuffer ());
              u_long remainingData = 0;

              if (bufferStrLen - td->nHeaderChars >= MYSERVER_KB (8))
                remainingData = MYSERVER_KB (8);
              else
                remainingData = bufferStrLen - td->nHeaderChars;

              if (remainingData)
                {
                  const char *data = (td->buffer->getBuffer ()
                                      + td->nHeaderChars);
                  u_long toCopy = nbtr - td->nHeaderChars;

                  a->getConnectionBuffer ()->setBuffer (data, toCopy);
                  pipelineData = true;
                }
            }

          /*
            Set the throttling rate for the socket. This setting can be
            changed later.
           */
          if (a->host->getThrottlingRate () == (u_long) - 1)
            a->socket->setThrottling (Server::getInstance ()->getThrottlingRate ());
          else
            a->socket->setThrottling (a->host->getThrottlingRate ());

          {
            string msg ("new-http-request");
            vector<Multicast<string, void*, int>*>* handlers =
                    staticData->getHandlers (msg);
            if (handlers)
              {
                for (size_t i = 0; i < handlers->size (); i++)
                  {
                    int handlerRet = (*handlers)[i]->updateMulticast (getStaticData (),
                                                                      msg, td);
                    if (handlerRet == ClientsThread::DELETE_CONNECTION)
                      {
                        ret = HttpDataHandler::RET_FAILURE;
                        break;
                      }
                  }
              }
          }

          if (ret < 0)
            {
              if (!allowMethod (td->request.cmd.c_str ()))
                return raiseHTTPError (401);

              if (!td->request.cmd.compare ("GET"))
                ret = sendHTTPResource (td->request.uri);
              else if (!td->request.cmd.compare ("POST"))
                ret = sendHTTPResource (td->request.uri);
              else if (!td->request.cmd.compare ("HEAD"))
                {
                  td->onlyHeader = true;
                  ret = sendHTTPResource (td->request.uri, false, true);
                }
              else if (!td->request.cmd.compare ("DELETE"))
                ret = dav.davdelete (td);
              else if (!td->request.cmd.compare ("PUT"))
                ret = putHTTPRESOURCE (td->request.uri, false, true);
              else if (!td->request.cmd.compare ("OPTIONS"))
                ret = optionsHTTPRESOURCE (td->request.uri, false);
              else if (!td->request.cmd.compare ("TRACE"))
                ret = traceHTTPRESOURCE (td->request.uri, false);
              else if (!td->request.cmd.compare ("MKCOL"))
                ret = dav.mkcol (td);
              else if (!td->request.cmd.compare ("PROPFIND"))
                ret = dav.propfind (td);
              else if (!td->request.cmd.compare ("COPY"))
                ret = dav.copy (td);
              else if (!td->request.cmd.compare ("MOVE"))
                ret = dav.move (td);
              else if (!td->request.cmd.compare ("LOCK"))
                ret = dav.lock (td);
              else if (!td->request.cmd.compare ("UNLOCK"))
                ret = dav.unlock (td);


              else
                {
                  /*
                    Return Method not implemented (501) if there
                    is not a dynamic methods manager.
                   */
                  if (!dynamicCommand)
                    ret = raiseHTTPError (501);
                  else
                    ret = dynamicCommand->send (td, a, td->request.uri, 0,
                                                false, false);
                }
            }
        }

      try
        {
          td->inputData.close ();
          td->outputData.close ();
        }
      catch (GenericFileException & e)
        {
        }

      logHTTPaccess ();

      /* Map the HttpDataHandler return value to codes understood by
         ClientsThread.  */
      if (ret == HttpDataHandler::RET_OK && keepalive)
        {
          if (pipelineData)
            return ClientsThread::INCOMPLETE_REQUEST_NO_WAIT;

          return ClientsThread::KEEP_CONNECTION;
        }
      else
        return ClientsThread::DELETE_CONNECTION;

    }
  catch (...)
    {
      td->inputData.close ();
      td->outputData.close ();

      td->connection->host->warningsLogWrite (_("HTTP: internal error"));
      raiseHTTPError (500);
      logHTTPaccess ();
      return ClientsThread::DELETE_CONNECTION;
    }

  return ClientsThread::KEEP_CONNECTION;
}

/*!
  Compute the Digest and write it to out.
 */
void Http::computeDigest (char* out)
{
  Md5 md5;
  char buffer[64];
  sprintf (buffer, "%i-%u-%s", (int) clock (), (u_int) td->id,
           td->connection->getIpAddr ());

  md5.init ();
  md5.update ((char const*) buffer, (unsigned int) strlen (buffer));
  md5.end (out);
}

/*!
  Send to the client an authorization request.
 */
int Http::requestAuthorization ()
{
  Md5 md5;
  string time;
  HttpRequestHeader::Entry *connection = td->request.other.get ("connection");
  HttpRequestHeader::Entry *host = td->request.other.get ("host");
  td->response.httpStatus = 401;
  td->auxiliaryBuffer->setLength (0);
  *td->auxiliaryBuffer << "HTTP/1.1 401 Unauthorized\r\n"
          << "Accept-Ranges: bytes\r\n";
  *td->auxiliaryBuffer << "Server: GNU MyServer " << MYSERVER_VERSION << "\r\n";
  *td->auxiliaryBuffer << "Content-type: text/html\r\n"
          << "Connection: ";
  *td->auxiliaryBuffer << (connection ? connection->value.c_str () : "");
  *td->auxiliaryBuffer << "\r\nContent-length: 0\r\n";

  if (td->authScheme == HTTP_AUTH_SCHEME_BASIC)
    {
      *td->auxiliaryBuffer << "WWW-Authenticate: Basic realm=\""
                           << (host ? host->value.c_str () : "")
                           << "\"\r\n";
    }
  else if (td->authScheme == HTTP_AUTH_SCHEME_DIGEST)
    {
      char md5Str[256];
      HttpUserData *hud = (HttpUserData*) td->connection->protocolBuffer;
      if (td->connection->protocolBuffer == 0)
        {
          td->connection->protocolBuffer = new HttpUserData;
          if (!td->connection->protocolBuffer)
            {
              sendHTTPhardError500 ();
              return HttpDataHandler::RET_FAILURE;
            }
          hud->reset ();
        }

      myserver_strlcpy (hud->realm, host ? host->value.c_str () : "", 48);

      /* Just a random string.  */
      md5Str[0] = (char) td->id;
      md5Str[1] = (char) ((clock () >> 24) & 0xFF);
      md5Str[2] = (char) ((clock () >> 16) & 0xFF);
      md5Str[3] = (char) ((clock () >> 8) & 0xFF);
      md5Str[4] = (char) (clock () & 0xFF);
      strncpy (&(md5Str[5]), td->request.uri.c_str (), 256 - 5);
      md5.init ();
      md5.update (md5Str, strlen (md5Str));
      md5.end (hud->opaque);

      if (td->connection->protocolBuffer &&
          ((!(hud->digest)) || (hud->nonce[0] == '\0')))
        {
          computeDigest (hud->nonce);
          hud->nc = 0;
        }

      *td->auxiliaryBuffer << "WWW-Authenticate: digest "
                           << " qop=\"auth\", algorithm=\"MD5\", realm=\""
                           << hud->realm << "\", opaque=\"" << hud->opaque
                           << "\",  nonce=\"" << hud->nonce << "\" ";

      if (hud->cnonce[0])
        *td->auxiliaryBuffer << ", cnonce=\"" << hud->cnonce << "\" ";

      *td->auxiliaryBuffer << "\r\n";
    }
  else
    {
      /* Send a non implemented error page if the auth scheme is not known.  */
      return raiseHTTPError (501);
    }

  *td->auxiliaryBuffer << "Date: ";
  getRFC822GMTTime (time, 32);
  *td->auxiliaryBuffer << time << "\r\n\r\n";

  if (td->connection->socket->send (td->auxiliaryBuffer->getBuffer (),
                                    td->auxiliaryBuffer->getLength (), 0) < 0)
    {
      td->connection->host->warningsLogWrite (_("HTTP: socket error"));
      return HttpDataHandler::RET_FAILURE;
    }

  return HttpDataHandler::RET_OK;
}

/*!
  Sends an error page to the client.
  Nonzero to keep the connection.
 */
int Http::raiseHTTPError (int ID)
{
  try
    {
      string time;
      ostringstream errorFile;
      string errorMessage;
      ostringstream errorBodyMessage;
      int errorBodyLength = 0;
      int useMessagesFiles = 1;
      HttpRequestHeader::Entry *host = td->request.other.get ("host");
      HttpRequestHeader::Entry *connection
        = td->request.other.get ("connection");
      const char *useMessagesVal =
        td->securityToken.getData ("http.use_error_file", MYSERVER_VHOST_CONF
                                   | MYSERVER_SERVER_CONF,
                                   NULL);
      if (useMessagesVal)
        {
          if (! strcasecmp (useMessagesVal, "YES"))
            useMessagesFiles = 1;
          else
            useMessagesFiles = 0;
        }

      if (td->lastError)
        {
          td->connection->host->warningsLogWrite (_("HTTP: recursive error"));
          return sendHTTPhardError500 ();
        }

      td->lastError = ID;

      if (connection && !stringcmpi (connection->value.c_str (), "keep-alive"))
        td->response.setValue ("connection", "keep-alive");

      td->response.httpStatus = ID;

      char errorName [32];
      sprintf (errorName, "http.error.file.%i", ID);
      const char *defErrorFile = td->securityToken.getData (errorName,
                                                            MYSERVER_SECURITY_CONF
                                                            | MYSERVER_VHOST_CONF
                                                            | MYSERVER_SERVER_CONF);
      if (defErrorFile)
        {
          ostringstream nURL;
          int isPortSpecified = 0;
          const char* hostStr = host ? host->value.c_str () : "";
          /* Change the URI to reflect the default file name.  */
          nURL << protocolPrefix << hostStr;
          for (int i = 0; hostStr[i]; i++)
            {
              if (hostStr[i] == ':')
                {
                  isPortSpecified = 1;
                  break;
                }
            }
          if (!isPortSpecified)
            nURL << ":" << td->connection->host->getPort ();

          if (nURL.str ()[nURL.str ().length () - 1] != '/')
            nURL << "/";

          nURL << defErrorFile;

          if (td->pathInfo.length ())
            nURL << "/" << td->pathInfo;

          if (td->request.uriOpts.length ())
            nURL << "?" << td->request.uriOpts;

          return sendHTTPRedirect (nURL.str ().c_str ());
        }

      if (useMessagesFiles)
        {
          string page;
          HttpErrors::getErrorMessage (ID, td->response.errorType);
          HttpErrors::getErrorPage (ID, page);
          errorFile << td->getVhostSys () << "/" << page;

          if (FilesUtility::nodeExists (errorFile.str ().c_str ()))
            {
              string errorFileStr = errorFile.str ();
              return sendHTTPResource (errorFileStr, 1, td->onlyHeader, 1);
            }
          else
            td->connection->host->warningsLogWrite (
               _("HTTP: The specified error page: %s does not exist"),
               errorFile.str ().c_str ());
        }

      HttpErrors::getErrorMessage (ID, errorMessage);

      const char* value
        = td->securityToken.getData ("http.error_body", MYSERVER_VHOST_CONF
                                     | MYSERVER_SERVER_CONF, NULL);

      if (value && ! strcasecmp (value, "NO"))
        {
          errorBodyLength = 0;
          td->response.contentLength.assign ("0");
        }
      else
        {
          ostringstream size;
          errorBodyMessage << ID << " - " << errorMessage << "\r\n";
          errorBodyLength = errorBodyMessage.str ().length ();
          size << errorBodyLength;
          td->response.contentLength.assign (size.str ());
        }

      HttpHeaders::sendHeader (td->response, *td->connection->socket,
                               *td->buffer, td);

      if (errorBodyLength)
          td->connection->socket->send (errorBodyMessage.str ().c_str (),
                                        errorBodyLength, 0);
    }
  catch (exception &e)
    {
      td->connection->host->warningsLogWrite (_E ("HTTP: internal error"), &e);
      return HttpDataHandler::RET_FAILURE;
    }
  return HttpDataHandler::RET_OK;
}

/*!
  Send a hard wired 500 error when we have a system error
 */
int Http::sendHTTPhardError500 ()
{
  MemBuf tmp;
  char tmpStr[12];
  string time;
  string errorMsg;
  const char hardHTML[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\"\n\
\"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n\
<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\">\n\
<head>\n\
<title>500 Internal Server Error</title>\n\
<meta http-equiv=\"content-type\" content=\"text/html;charset=UTF-8\" />\n\
</head>\n\
<body style=\"color: #666699;\">\n\
<br />\n\
<h1 style=\"text-align: center;\">\n\
Error 500\n\
<br /><br />\n\
Internal Server Error\n\
</h1>\n\
</body>\n\
</html>\r\n";
  HttpErrors::getErrorMessage (500, errorMsg);

  td->response.httpStatus = 500;
  td->buffer->setLength (0);
  *td->buffer << errorMsg;
  *td->buffer << " from: ";
  *td->buffer << td->connection->getIpAddr ();
  *td->buffer << "\r\n";
  td->auxiliaryBuffer->setLength (0);
  *td->auxiliaryBuffer << "HTTP/1.1 500 System Error\r\n";
  *td->auxiliaryBuffer << "Server: GNU MyServer " << MYSERVER_VERSION << "\r\n";
  *td->auxiliaryBuffer << " Content-type: text/html\r\nContent-length: ";
  tmp.intToStr ((int) strlen (hardHTML), tmpStr, 12);
  *td->auxiliaryBuffer << tmp;
  *td->auxiliaryBuffer << "\r\n";
  *td->auxiliaryBuffer << "Date: ";
  getRFC822GMTTime (time, 32);
  *td->auxiliaryBuffer << time;
  *td->auxiliaryBuffer << "\r\n\r\n";

  td->connection->socket->send (td->auxiliaryBuffer->getBuffer (),
                                (u_long) td->auxiliaryBuffer->getLength (),
                                0);

  if (!td->onlyHeader)
    td->connection->socket->send (hardHTML, (u_long) strlen (hardHTML), 0);

  return HttpDataHandler::RET_FAILURE;
}

/*!
  Returns the MIME type passing its extension.
  Returns zero if the file is registered.
 */
MimeRecord* Http::getMIME (string &filename)
{
  const char *handler = td->securityToken.getData ("mime.handler",
                      MYSERVER_VHOST_CONF | MYSERVER_SERVER_CONF, NULL);

  if (staticData->getAllowVhostMime () && td->connection->host->isMIME ())
    return td->connection->host->getMIME ()->getMIME (filename);

  return Server::getInstance ()->getMimeManager ()->getMIME (filename, handler);
}

/*!
  Map an URL to the machine file system. Return 200 on success.
  Any other return value is the HTTP error.
 */
int Http::getPath (HttpThreadContext* td, string& filenamePath,
                   const char *filename, bool systemrequest)
{
  if (systemrequest)
    {
      if ((td->getVhostSys ()[0] == '\0')
          || FilesUtility::getPathRecursionLevel (filename) < 2)
        return 401;

      filenamePath.assign (td->getVhostSys ());
      if (filename[0] != '/')
        filenamePath.append ("/");
      filenamePath.append (filename);
    }
  else
    {
      if (! filename[0])
        filenamePath.append (td->getVhostDir ());
      else
        {
          const char *root;
          /*
            uri starting with a /sys/ will use the system directory as
            the root path. Be sure to don't allow access to the system root
            but only to subdirectories.
           */
          if (filename[0] == '/' && filename[1] == 's' && filename[2] == 'y'
              && filename[3] == 's' && filename[4] == '/')
            {
              root = td->getVhostSys ();
              /*
                Do not allow access to the system directory root but only
                to subdirectories.
               */
              if (FilesUtility::getPathRecursionLevel (filename) < 2)
                {
                  return 401;
                }
              filename = filename + 5;
            }
          else
            root = td->getVhostDir ();

          filenamePath.assign (root);

          if (filename[0] != '/')
            filenamePath.append ("/");

          filenamePath.append (filename);
        }

    }
  return 200;
}

/*!
  If a directory is accessed try in order:

  1) The default files in order.
  2) The directory content.
  3) An error.

  \param uri The accessed URI.
  \param permissions The permission mask for the client.
  \param onlyHeader specify if the client requested only the header.
 */
int Http::processDefaultFile (string& uri, int permissions, bool onlyHeader)
{
  string key ("http.default_file");
  NodeTree<string> *node = td->securityToken.getNodeTree (key,
                     MYSERVER_VHOST_CONF | MYSERVER_SERVER_CONF, NULL);


  if (node)
    {
      list<NodeTree<string>*> *children = node->getChildren ();

      for (list<NodeTree<string>*>::iterator it = children->begin ();
           it != children->end (); it++)
        {
          ostringstream defaultFileName;
          const string *file = (*it)->getValue ();
          defaultFileName.clear ();
          defaultFileName << td->filenamePath << "/" << *file;

          if (FilesUtility::nodeExists (defaultFileName.str ().c_str ()))
            {
              ostringstream nUrl;

              if (td->request.uriEndsWithSlash)
                nUrl << *file;
              else
                {
                  u_long lastSlashOffset = uri.length ();
                  while (lastSlashOffset && uri[lastSlashOffset] != '/')
                    --lastSlashOffset;

                  nUrl << &(uri.c_str ()[lastSlashOffset < uri.length () ?
                            lastSlashOffset + 1 : 0])
                          << "/" << *file;
                }

              if (td->pathInfo.length ())
                nUrl << "/" << td->pathInfo;

              if (td->request.uriOpts.length ())
                nUrl << "?" << td->request.uriOpts;

              /* Send a redirect to the new location.  */
              return sendHTTPRedirect (nUrl.str ().c_str ());
            }
        }
    }

  HttpDataHandler *handler =
    staticData->getDynManagerList ()->getHttpManager ("DIR");
  if (!handler)
    {
      td->connection->host->warningsLogWrite (_("HTTP: internal error"));
      return raiseHTTPError (500);
    }

  return handler->send (td, td->filenamePath.c_str (), NULL, 0, onlyHeader);
}

/*!
  Send a redirect message to the client.
 */
int Http::sendHTTPRedirect (const char *newURL)
{
  string time;
  HttpRequestHeader::Entry *connection = td->request.other.get ("connection");

  td->response.httpStatus = 302;
  td->auxiliaryBuffer->setLength (0);
  *td->auxiliaryBuffer << "HTTP/1.1 302 Moved\r\nAccept-Ranges: bytes\r\n"
          << "Server: GNU MyServer " << MYSERVER_VERSION << "\r\n"
          << "Content-type: text/html\r\n"
          << "Location: " << newURL << "\r\n"
          << "Content-length: 0\r\n";

  if (connection && !stringcmpi (connection->value.c_str (), "keep-alive"))
    *td->auxiliaryBuffer << "Connection: keep-alive\r\n";
  else
    *td->auxiliaryBuffer << "Connection: close\r\n";

  *td->auxiliaryBuffer << "Date: ";
  getRFC822GMTTime (time, 32);
  *td->auxiliaryBuffer << time
          << "\r\n\r\n";
  td->connection->socket->send (td->auxiliaryBuffer->getBuffer (),
                                (int) td->auxiliaryBuffer->getLength (), 0);
  return HttpDataHandler::RET_OK;
}

/*!
  Send a non-modified message to the client.
 */
int Http::sendHTTPNonModified ()
{
  string time;
  HttpRequestHeader::Entry *connection = td->request.other.get ("connection");

  td->response.httpStatus = 304;
  td->auxiliaryBuffer->setLength (0);
  *td->auxiliaryBuffer << "HTTP/1.1 304 Not Modified\r\nAccept-Ranges: bytes\r\n"
          << "Server: GNU MyServer " << MYSERVER_VERSION << "\r\n";

  if (connection && !stringcmpi (connection->value.c_str (), "keep-alive"))
    *td->auxiliaryBuffer << "Connection: keep-alive\r\n";
  else
    *td->auxiliaryBuffer << "Connection: close\r\n";

  getRFC822GMTTime (time, 32);

  *td->auxiliaryBuffer << "Date: " << time << "\r\n\r\n";

  td->connection->socket->send (td->auxiliaryBuffer->getBuffer (),
                                (int) td->auxiliaryBuffer->getLength (), 0);

  return HttpDataHandler::RET_OK;
}

/*!
  Send a 401 error.
 */
int Http::sendAuth ()
{
  if (td->connection->getnTries () > 2)
    return raiseHTTPError (401);
  else
    {
      td->connection->incnTries ();
      return requestAuthorization ();
    }
}

/*!
  Load the HTTP protocol.
 */
int Http::loadProtocolStatic ()
{
  return 0;
}

/*!
  Returns the name of the protocol. If an out buffer
  is defined fullfill it with the name too.
 */
const char* Http::getNameImpl ()
{
  return "HTTP";
}

/*!
  Constructor for the class http.
 */
Http::Http (HttpProtocol *staticData)
{
  td = new HttpThreadContext ();
  this->staticData = staticData;
  protocolPrefix.assign ("http://");
  protocolOptions = 0;
  td->filenamePath.assign ("");
  td->pathInfo.assign ("");
  td->pathTranslated.assign ("");
  td->cgiRoot.assign ("");
  td->cgiFile.assign ("");
  td->scriptPath.assign ("");
  td->scriptDir.assign ("");
  td->scriptFile.assign ("");
}

/*!
  Destructor for the http class.
 */
Http::~Http ()
{
  clean ();
}

/*!
  Clean the used memory.
 */
void Http::clean ()
{
  td->filenamePath.assign ("");
  td->pathInfo.assign ("");
  td->pathTranslated.assign ("");
  td->cgiRoot.assign ("");
  td->cgiFile.assign ("");
  td->scriptPath.assign ("");
  td->scriptDir.assign ("");
  td->scriptFile.assign ("");
  delete td;
}
