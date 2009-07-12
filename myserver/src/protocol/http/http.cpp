/*
MyServer
Copyright (C) 2002-2009 Free Software Foundation, Inc.
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


#include <include/protocol/http/http.h>
#include <include/protocol/http/http_headers.h>
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
#include <include/base/md5/md5.h>
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

#include <string>
#include <ostream>

using namespace std;

extern "C"
{
#ifdef WIN32
#include <direct.h>
#include <errno.h>
#endif

#ifndef WIN32
#include <string.h>
#include <errno.h>
#endif
}

static HttpStaticData staticHttp;


/*!
 *Get a pointer to a structure shared among all the instances.
 */
HttpStaticData* Http::getStaticData ()
{
  return &staticHttp;
}


HttpStaticData::HttpStaticData ()
{

}

HttpStaticData::~HttpStaticData ()
{
}


/*!
 *Build a response for an OPTIONS request.
 */
int Http::optionsHTTPRESOURCE(string& filename, int yetmapped)
{
  int ret;
  string time;
  int permissions;

  try
  {
    HttpRequestHeader::Entry *connection = td->request.other.get("Connection");
    string methods("OPTIONS, GET, POST, HEAD, DELETE, PUT, TRACE");

    HashMap<string, DynamicHttpCommand*>::Iterator it = staticHttp.dynCmdManager.begin();
    while(it != staticHttp.dynCmdManager.end())
    {
      methods.append(", ");
      methods.append((*it)->getName());
      it++;
    }

    ret = Http::preprocessHttpRequest(filename, yetmapped, &permissions);

    if (ret != 200)
      return raiseHTTPError (ret);
   
    getRFC822GMTTime(time, HTTP_RESPONSE_DATE_DIM);
    td->secondaryBuffer->setLength(0);
    *td->secondaryBuffer <<  "HTTP/1.1 200 OK\r\n";
    *td->secondaryBuffer << "Date: " << time ;
    *td->secondaryBuffer <<  "\r\nServer: GNU MyServer "<< MYSERVER_VERSION;
    if(connection && connection->value->length())
      *td->secondaryBuffer << "\r\nConnection:" << connection->value->c_str() << "\r\n";
    *td->secondaryBuffer <<"Content-Length: 0\r\nAccept-Ranges: bytes\r\n";
    *td->secondaryBuffer << "Allow: " << methods << "\r\n\r\n";

    /* Send the HTTP header. */
    ret = td->connection->socket->send(td->secondaryBuffer->getBuffer(),
                                      (u_long)td->secondaryBuffer->getLength(), 0);
    if( ret == SOCKET_ERROR )
    {
      return 0;
    }
    return 1;
  }
  catch(...)
  {
    return raiseHTTPError(500);
  };
}

/*!
 *Handle the HTTP TRACE command.
 */
int Http::traceHTTPRESOURCE(string& filename, int yetmapped)
{
  int ret;
  char tmpStr[12];
  int contentLength = (int)td->nHeaderChars;
  string time;
  int permissions;
  try
  {
    MemBuf tmp;
    HttpRequestHeader::Entry *connection;

    ret = Http::preprocessHttpRequest(filename, yetmapped, &permissions);

    if(ret != 200)
      return raiseHTTPError(ret);

    tmp.intToStr(contentLength, tmpStr, 12);
    getRFC822GMTTime(time, HTTP_RESPONSE_DATE_DIM);

    td->secondaryBuffer->setLength(0);
    *td->secondaryBuffer << "HTTP/1.1 200 OK\r\n";
    *td->secondaryBuffer << "Date: " << time << "\r\n";
    *td->secondaryBuffer << "Server: GNU MyServer " << MYSERVER_VERSION  << "\r\n";
    connection = td->request.other.get("Connection");
    if(connection && connection->value->length())
      *td->secondaryBuffer << "Connection:" << connection->value->c_str() << "\r\n";
    *td->secondaryBuffer <<"Content-Length:" << tmp << "\r\n"
                 << "Content-Type: message/http\r\n"
                 << "Accept-Ranges: bytes\r\n\r\n";

    /*! Send our HTTP header.  */
    ret = td->connection->socket->send(td->secondaryBuffer->getBuffer(),
                                      (u_long)td->secondaryBuffer->getLength(), 0);
    if( ret == SOCKET_ERROR )
    {
      return 0;
    }

    /*! Send the client request header as the HTTP body.  */
    ret = td->connection->socket->send(td->buffer->getBuffer(),
                                      contentLength, 0);
    if(ret == SOCKET_ERROR)
    {
      return 0;
    }
    return 1;
  }
  catch(...)
  {
    return raiseHTTPError(500);
  };
}

/*!
 *Check if the method is allowed.
 *\param method The HTTP method name.
 *\return true if it is allowed.
 */
bool Http::allowMethod(const char *method)
{
  char name[64];
  sprintf (name, "http.%s.allow", method);
  const char *allow = td->securityToken.getHashedData (name, 
                                                       MYSERVER_VHOST_CONF |
                                                       MYSERVER_SERVER_CONF, "YES");

  if (!strcmpi (allow, "NO"))
    return false;
  else
    return true;
}

/*!
 *Get the timeout for the cgi.
 */
u_long Http::getTimeout ()
{
  return staticHttp.timeout;
}

/*!
 *Main function to handle the HTTP PUT command.
 */
int Http::putHTTPRESOURCE(string& filename,
                          int sysReq,
                          int onlyHeader,
                          int yetmapped)
{
  return sendHTTPResource (filename, sysReq, onlyHeader, yetmapped);
}

/*!
 *Get the file permissions mask.
 *\param filename Resource to access.
 *\param directory Directory where the resource is.
 *\param file The file specified by the resource.
 *\param filenamePath Complete path to the file.
 *\param yetmapped Is the resource mapped to the localfilesystem?
 *\param permissions Permission mask for this resource.
 *\return Return 200 on success.
 *\return Any other value is the HTTP error code.
 */
int Http::getFilePermissions(string& filename, string& directory, string& file, 
                             string &filenamePath, int yetmapped, int* permissions)
{
  try
  {
    td->securityToken.setServer (Server::getInstance ());
    td->securityToken.setSysDirectory ((string*)&(td->connection->host->getSystemRoot ()));
    
    td->securityToken.setVhost (td->connection->host);

    FilesUtility::splitPath (filename, directory, file);
    FilesUtility::completePath (directory);

    td->securityToken.setResource (&filenamePath);
    td->securityToken.setDirectory (&directory);

    /*!
     *td->filenamePath is the file system mapped path while filename
     *is the uri requested.
     *systemrequest is 1 if the file is in the system directory.
     *If filename is already mapped on the file system don't map it again.
     */
    if (yetmapped)
    {
      filenamePath.assign (filename);
    }
    else
    {
      int ret;
      /*!
       *If the client tries to access files that aren't in the web directory
       *send a HTTP 401 error page.
       */
      translateEscapeString (filename);
      if ((filename[0] != '\0') &&
          (FilesUtility::getPathRecursionLevel(filename) < 1))
      {
        return 401;
      }

      ret = getPath (filenamePath, filename, 0);

      if (ret != 200)
        return ret;
    }

    if (FilesUtility::isLink (td->filenamePath.c_str()))
    {
      const char *perm = td->securityToken.getHashedData ("symlinks.follow", MYSERVER_VHOST_CONF |
                                                          MYSERVER_SERVER_CONF, "NO");

      if(!perm || strcmpi(perm, "YES"))
        return raiseHTTPError(401);
    }

    if (FilesUtility::isDirectory (filenamePath.c_str()))
    {
      directory.assign(filenamePath);
    }
    else
    {
      FilesUtility::splitPath (filenamePath, directory, file);
    }

    if (td->connection->protocolBuffer == 0)
    {
      td->connection->protocolBuffer = new HttpUserData;
      if (!td->connection->protocolBuffer)
      {
        return 500;
      }
      ((HttpUserData*)(td->connection->protocolBuffer))->reset();
    }

    string user;
    string password;

    if(td->request.auth.length())
    {
      user.assign (td->connection->getLogin());
      password.assign (td->connection->getPassword ());
    }
    else
    {
      /* The default user is Guest with a null password. */
      user.assign ("Guest");
      password.assign ("");
    }
    if(*permissions == -1)
    {
      td->connection->host->warningsLogWrite(
                                     "Http: Error reading security file");
      return 500;
    }

    td->securityToken.setUser (user);
    td->securityToken.setPassword (password);

    AuthDomain auth (&(td->securityToken));
    HttpReqSecurityDomain httpReqSecDom (&(td->request));

    string validator (td->securityToken.getHashedData ("sec.validator", MYSERVER_VHOST_CONF |
                                                   MYSERVER_SERVER_CONF, "xml"));
    string authMethod (td->securityToken.getHashedData ("sec.auth_method", MYSERVER_VHOST_CONF |
                                                    MYSERVER_SERVER_CONF, "xml"));


    SecurityDomain* domains[] = {&auth, &httpReqSecDom, NULL};

    Server::getInstance()->getSecurityManager ()->getPermissionMask (&(td->securityToken), 
                                                                     domains, 
                                                                     validator, authMethod);

    const char *authType = td->securityToken.getHashedData ("http.auth", MYSERVER_SECURITY_CONF |
                                                        MYSERVER_VHOST_CONF |
                                                        MYSERVER_SERVER_CONF);
    *permissions = td->securityToken.getMask ();

    /*! Check if we have to use digest for the current directory. */
    if(authType && !strcmpi(authType, "Digest"))
    {
      HttpUserData* hud = (HttpUserData*)td->connection->protocolBuffer;

      if(!td->request.auth.compare("Digest"))
      {
        if(!hud->digestChecked)
          hud->digest = checkDigest();

        hud->digestChecked = 1;

        if(hud->digest == 1)
        {
          td->connection->setPassword (td->securityToken.getNeededPassword ().c_str ());
          *permissions = td->securityToken.getProvidedMask ();
        }
      }
      td->authScheme = HTTP_AUTH_SCHEME_DIGEST;
    }
    /*! By default use the Basic authentication scheme. */
    else
    {
      td->authScheme = HTTP_AUTH_SCHEME_BASIC;
    }
  }
  catch(...)
  {
    return 500;
  }

  const char *tr = td->securityToken.getHashedData ("connection.throttling", 
                                                    MYSERVER_SECURITY_CONF |
                                                    MYSERVER_VHOST_CONF |
                                                    MYSERVER_SERVER_CONF);

  /*! If a throttling rate was specifed use it.  */
  if(tr)
    td->connection->socket->setThrottling( atoi (tr));

  return 200;
}


/*!
 *Preprocess a HTTP request.
 *\param filename Resource to access.
 *\param yetmapped Is the resource mapped to the localfilesystem?
 *\param permissions Permission mask for this resource.
 *\return Return 200 on success.
 *\return Any other value is the HTTP error code.
 */
int Http::preprocessHttpRequest(string& filename, int yetmapped, int* permissions)
{
  string directory;
  string file;
  int filenamePathLen;
  string dirscan;
  int ret;

  try
  {
    if(td->request.isKeepAlive())
    {
      td->response.connection.assign( "keep-alive");
    }


    ret = getFilePermissions (filename, directory, file, 
                              td->filenamePath, yetmapped, permissions);

    if (ret != 200)
      return ret;

    /*
     *Get the PATH_INFO value.
     *Use dirscan as a buffer for put temporary directory scan.
     *When an '/' character is present check if the path up to '/' character
     *is a file. If it is a file send the rest of the uri as PATH_INFO.
     */
    td->pathInfo.assign("");
    td->pathTranslated.assign("");
    filenamePathLen = (int)td->filenamePath.length();
    dirscan.assign("");

    MimeRecord* mimeLoc = NULL;

    for(u_long i = 0;; )
    {
      /*
       *http://host/path/to/file/file.txt/PATH_INFO_VALUE?QUERY_INFO_VALUE
       *When a request has this form send the file file.txt with the
       *environment string PATH_INFO equals to PATH_INFO_VALUE and QUERY_INFO
       *to QUERY_INFO_VALUE.
       *
       *If there is the '/' character check if dirscan is a file.
       */

      u_long next = td->filenamePath.find ('/', i + 1);

      string curr = td->filenamePath.substr (0, next);

      mimeLoc = td->connection->host ? td->connection->host->getLocationMime (curr) : NULL;

      if(mimeLoc)
      {
        if (next != string::npos)
        {
          td->pathInfo.assign (&(td->filenamePath.c_str ()[next]));
          td->filenamePath.erase (next);
        }
        else
        {
          td->pathInfo.assign ("");
        }

        break;
      }

      if (next >= filenamePathLen)
        break;

      if(mimeLoc ||
         !FilesUtility::isDirectory (curr.c_str ()))
      {
        td->pathInfo.assign (&(td->filenamePath.c_str ()[next]));
        td->filenamePath.erase (next);
        break;
      }

      i = next;
    }

    /*
     *PATH_TRANSLATED is the local filesystem mapped version of PATH_INFO.
     */
    if(td->pathInfo.length () > 1)
    {
      int ret;
      /* Omit the first slash character.  */
      ret = getPath (td->pathTranslated, &((td->pathInfo.c_str())[1]), 0);

      if (ret != 200)
        td->pathTranslated.assign ("");
      else
        FilesUtility::completePath (td->pathTranslated);
    }
    else
    {
      td->pathTranslated.assign ("");
    }
    FilesUtility::completePath (td->filenamePath);

    td->mime = mimeLoc ? mimeLoc : getMIME (td->filenamePath);
  }
  catch(...)
  {
    return 500;
  }

  return 200;
}

/*!
 *Delete the resource identified by filename.
 */
int Http::deleteHTTPRESOURCE(string& filename,
                             int sysReq,
                             int onlyHeader,
                             int yetmapped)
{
  return sendHTTPResource (filename, sysReq, onlyHeader, yetmapped);
}

/*!
 *Check the Digest authorization
 */
u_long Http::checkDigest()
{
  Md5 md5;
  char A1[48];
  char A2[48];
  char response[48];
  char *uri;
  u_long digestCount;
  /*! Return 0 if the password is different.  */
  if(td->request.digestOpaque[0] && strcmp(td->request.digestOpaque,
                      ((HttpUserData*)td->connection->protocolBuffer)->opaque))
    return 0;
  /*! If is not equal return 0.  */
  if(strcmp(td->request.digestRealm,
             ((HttpUserData*)td->connection->protocolBuffer)->realm))
    return 0;

  digestCount = hexToInt(td->request.digestNc);

  if(digestCount != ((HttpUserData*)td->connection->protocolBuffer)->nc + 1)
    return 0;
  else
    ((HttpUserData*)td->connection->protocolBuffer)->nc++;

  md5.init();
  td->secondaryBuffer->setLength(0);
  *td->secondaryBuffer << td->request.digestUsername << ":" << td->request.digestRealm
               << ":" << td->securityToken.getNeededPassword();

  md5.update((unsigned char const*)td->secondaryBuffer->getBuffer(),
             (unsigned int)td->secondaryBuffer->getLength());
  md5.end(A1);

  md5.init();

  if(td->request.digestUri[0])
    uri = td->request.digestUri;
  else
    uri = (char*)td->request.uriOpts.c_str();

  td->secondaryBuffer->setLength(0);
  *td->secondaryBuffer <<  td->request.cmd.c_str() <<  ":" << uri;
  md5.update((unsigned char const*)td->secondaryBuffer->getBuffer(),
             (unsigned int)td->secondaryBuffer->getLength());
  md5.end( A2);

  md5.init();
  td->secondaryBuffer->setLength(0);
  *td->secondaryBuffer << A1 << ":"
              << ((HttpUserData*)td->connection->protocolBuffer)->nonce << ":"
              << td->request.digestNc << ":"  << td->request.digestCnonce << ":"
              << td->request.digestQop  << ":" << A2;
  md5.update((unsigned char const*)td->secondaryBuffer->getBuffer(),
             (unsigned int)td->secondaryBuffer->getLength());
  md5.end(response);

  if(!strcmp(response, td->request.digestResponse))
    return 1;
  return 0;
}

/*!
 *Create the buffer.
 */
HttpUserData::HttpUserData()
{
  reset();
}

/*!
 *Destroy the buffer.
 */
HttpUserData::~HttpUserData()
{

}

/*!
 *Reset the structure.
 */
void HttpUserData::reset()
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
 *Main function to send a resource to a client.
 */
int Http::sendHTTPResource(string& uri, int systemrequest, int onlyHeader,
                           int yetmapped)
{
  /*!
   *With this code we manage a request of a file or a directory or anything
   *that we must send over the HTTP.
   */
  string filename;
  const char *cgiManager;
  int ret;
  string directory;
  string file;
  HttpDataHandler *manager;

  /*! By default allows only few actions. */
  td->permissions = MYSERVER_PERMISSION_READ | MYSERVER_PERMISSION_BROWSE ;

  try
  {
    filename.assign(uri);
    td->buffer->setLength(0);

    ret = Http::preprocessHttpRequest(filename, yetmapped, &td->permissions);

    if(systemrequest)
      td->filenamePath.assign(uri);

    if(ret != 200)
      return raiseHTTPError(ret);

    if(!td->mime && FilesUtility::isDirectory(td->filenamePath.c_str()))
    {
      return processDefaultFile (uri, td->permissions, onlyHeader);
    }

    td->response.contentType[0] = '\0';

    /* If not specified differently, set the default content type to text/html.  */
    if(td->mime)
    {
      td->response.contentType.assign (td->mime->mimeType);
      cgiManager = td->mime->cgiManager.c_str ();
    }
    else
    {
      td->response.contentType.assign("text/html");
      cgiManager = "";
    }


    if (td->mime && (manager = staticHttp.dynManagerList.getHttpManager (td->mime->cmdName)))
      return manager->send (td, td->filenamePath.c_str(), cgiManager,
                            td->mime->selfExecuted, onlyHeader);

    if (!(td->permissions & MYSERVER_PERMISSION_READ))
      return sendAuth ();

    manager = staticHttp.dynManagerList.getHttpManager ("SEND");

    if (!manager)
      return raiseHTTPError (500);

    return manager->send (td, td->filenamePath.c_str(), 0, onlyHeader);
  }
  catch (...)
  {
    return raiseHTTPError (500);
  };

  return ret;
}
/*!
 *Log the access using the Common Log Format or the Combined one.
 */
int Http::logHTTPaccess()
{
  char tmpStrInt[12];

  string time;

  try
  {
    td->secondaryBuffer->setLength(0);
    *td->secondaryBuffer << td->connection->getIpAddr();
    *td->secondaryBuffer<< " ";

    if(td->connection->getLogin()[0])
      *td->secondaryBuffer << td->connection->getLogin();
    else
      *td->secondaryBuffer << "-";

    *td->secondaryBuffer<< " ";

    if(td->connection->getLogin()[0])
      *td->secondaryBuffer << td->connection->getLogin();
    else
      *td->secondaryBuffer << "-";

    *td->secondaryBuffer << " [";

    getLocalLogFormatDate(time, HTTP_RESPONSE_DATE_DIM);
    *td->secondaryBuffer <<  time  << "] \"";

    if(td->request.cmd.length())
      *td->secondaryBuffer << td->request.cmd.c_str() << "";

    if(td->request.cmd.length() || td->request.uri.length())
      *td->secondaryBuffer << " ";

    if(td->request.uri.length() == '\0')
      *td->secondaryBuffer <<  "/";
    else
      *td->secondaryBuffer << td->request.uri.c_str();


    if(td->request.uriOpts.length())
      *td->secondaryBuffer << "?" << td->request.uriOpts.c_str();

    sprintf(tmpStrInt, "%u ", td->response.httpStatus);

    if(td->request.ver.length())
      *td->secondaryBuffer << " " << td->request.ver.c_str()  ;

    *td->secondaryBuffer<< "\" " << tmpStrInt  << " ";


    sprintf(tmpStrInt, "%u", td->sentData);
    *td->secondaryBuffer << tmpStrInt;

    if(td->connection->host)
    {
      HttpRequestHeader::Entry *userAgent = td->request.other.get("User-Agent");
      HttpRequestHeader::Entry *referer = td->request.other.get("Refer");

      if(strstr((td->connection->host)->getAccessLogOpt(), "type=combined"))
        *td->secondaryBuffer << " "  << (referer   ? referer->value->c_str() : "")
                    << " "  << (userAgent ? userAgent->value->c_str() : "");
    }
#ifdef WIN32
    *td->secondaryBuffer  << "\r\n" << end_str;
#else
    *td->secondaryBuffer  << "\n" << end_str;
#endif
    /*!
     *Request the access to the log file then append the message.
     */
     if(td->connection->host)
     {
       td->connection->host->accessesLogWrite(td->secondaryBuffer->getBuffer());
     }
    td->secondaryBuffer->setLength(0);
  }
  catch(...)
  {
    return 1;
  };
  return 0;
}

/*!
 *This is the HTTP protocol main procedure to parse a request
 *over HTTP.
 */
int Http::controlConnection(ConnectionPtr a, char* /*b1*/, char* /*b2*/,
                            int bs1, int bs2, u_long nbtr, u_long id)
{
  int retvalue = -1;
  int ret = 0;
  int validRequest;

  /* Dimension of the POST data. */
  int contentLength = -1;
  DynamicHttpCommand *dynamicCommand;

  try
  {
    td->buffer = a->getActiveThread ()->getBuffer ();
    td->secondaryBuffer = a->getActiveThread ()->getSecondaryBuffer ();
    td->buffersize = bs1;
    td->secondaryBufferSize = bs2;
    td->nBytesToRead = nbtr;
    td->connection = a;
    td->id = id;
    td->lastError = 0;
    td->http = this;
    td->appendOutputs = 0;
    td->onlyHeader = 0;
    td->inputData.setHandle ((Handle)0);
    td->outputData.setHandle ((Handle)0);
    td->filenamePath.assign ("");
    td->outputDataPath.assign ("");
    td->inputDataPath.assign ("");
    td->mime = 0;
    td->sentData = 0;
    td->vhostDir.assign ("");
    td->vhostSys.assign ("");
    {
      HashMap<string,string*>::Iterator it = td->other.begin();
      while(it != td->other.end())
        delete (*it);
    }
    td->other.clear();

    /*!
     *Reset the request and response structures.
     */
    HttpHeaders::resetHTTPRequest(&td->request);
    HttpHeaders::resetHTTPResponse(&td->response);

    /*! Reset the HTTP status once per request. */
    td->response.httpStatus = 200;

    /*!
     *If the connection must be removed, remove it.
     */
    if(td->connection->getToRemove())
    {
      switch(td->connection->getToRemove())
      {
        /*! Remove the connection from the list.  */
        case CONNECTION_REMOVE_OVERLOAD:
          retvalue = raiseHTTPError(503);
          logHTTPaccess();
          return ClientsThread::DELETE_CONNECTION;
        default:
          return ClientsThread::DELETE_CONNECTION;
      }
    }

    validRequest = HttpHeaders::buildHTTPRequestHeaderStruct(td->buffer->getBuffer(),
                                                             td->buffer->getRealLength(),
                                                             &(td->nHeaderChars),
                                                             &(td->request), 
                                                             td->connection);

    /*! -1 means the request is not complete yet. */
    if(validRequest == -1)
    {
      return ClientsThread::INCOMPLETE_REQUEST;
    }

    if(a->protocolBuffer)
      ((HttpUserData*)a->protocolBuffer)->digestChecked = 0;

    /*!
     *If the validRequest cointains an error code send it to the user.
     */
    if(validRequest != 200)
    {
      retvalue = raiseHTTPError(validRequest);
      logHTTPaccess();
      return ClientsThread::DELETE_CONNECTION;
    }
    /*! Be sure that we can handle the HTTP version.  */
    if((td->request.ver.compare("HTTP/1.1")) &&
       (td->request.ver.compare("HTTP/1.0")) &&
       (td->request.ver.compare("HTTP/0.9")))
    {
      raiseHTTPError(505);
      logHTTPaccess();
      /*! Remove the connection from the list.  */
      return ClientsThread::DELETE_CONNECTION;
    }

    td->response.ver.assign(td->request.ver.c_str());

    /*! Do not use Keep-Alive with HTTP version older than 1.1.  */
    if(td->request.ver.compare("HTTP/1.1") )
    {
      HttpRequestHeader::Entry *connection =
        td->request.other.get("Connection");

      if(connection && connection->value->length())
      {
        connection->value->assign("close");
      }
    }

    /*!
     *For methods that accept data after the HTTP header set the correct
     *pointer and create a file containing the informations after the header.
     */
    FilesUtility::temporaryFileName(td->id, td->inputDataPath);
    FilesUtility::temporaryFileName(td->id, td->outputDataPath);

    dynamicCommand = staticHttp.dynCmdManager.getHttpCommand(td->request.cmd);

    /* If the used method supports POST data, read it.  */
    if((!td->request.cmd.compare("POST")) ||
       (!td->request.cmd.compare("PUT")) ||
       (dynamicCommand && dynamicCommand->acceptData() ))
    {
      int ret;
      int httpErrorCode;

      /*! Be sure that the client can handle the 100 status code.  */
      if(nbtr == td->nHeaderChars && td->request.contentLength.compare("0") &&
         td->request.ver.compare("HTTP/1.0"))
      {
        const char* msg = "HTTP/1.1 100 Continue\r\n\r\n";
        if(a->socket->bytesToRead() == 0)
        {
          if(a->socket->send(msg, (int)strlen(msg), 0)==-1)
            return ClientsThread::DELETE_CONNECTION;
        }
        return ClientsThread::INCOMPLETE_REQUEST;
      }

      ret = HttpDataRead::readPostData(td, &httpErrorCode);

      if(ret == -1)
      {
        logHTTPaccess();

        return ClientsThread::DELETE_CONNECTION;
      }
      else if(ret)
      {
        int retvalue = raiseHTTPError(httpErrorCode);

        logHTTPaccess();

        return retvalue ? ClientsThread::KEEP_CONNECTION
                        : ClientsThread::DELETE_CONNECTION;
      }
    }
    else
    {
       contentLength = 0;
       td->request.uriOptsPtr = 0;
    }

    /*! If return value is not configured propertly.  */
    if(retvalue == -1)
    {
      /*!
       *How is expressly said in the RFC2616 a client that sends an
       *HTTP/1.1 request MUST sends a Host header.
       *Servers MUST reports a 400 (Bad request) error if an HTTP/1.1
       *request does not include a Host request-header.
       */
      HttpRequestHeader::Entry *host = td->request.other.get("Host");

      if((!td->request.ver.compare("HTTP/1.1")) &&
         ((host && host->value->length() == 0) || (host == 0)) )
      {
        raiseHTTPError(400);
        /*!
         *If the inputData file was not closed close it.
         */
        if(td->inputData.getHandle())
        {
          td->inputData.close();
          FilesUtility::deleteFile(td->inputDataPath);
        }

        /*!
         *If the outputData file was not closed close it.
         */
        if(td->outputData.getHandle())
        {
          td->outputData.close();
          FilesUtility::deleteFile(td->outputDataPath);
        }
        logHTTPaccess();
        return ClientsThread::DELETE_CONNECTION;
      }
      else
      {
        /*!
         *Find the virtual host to check both host name and IP value.
         */
        Vhost* newHost = Server::getInstance()->getVhosts()->getVHost(host ?
                                    host->value->c_str() : "",
                                     a->getLocalIpAddr(), a->getLocalPort());
        if(a->host)
          a->host->removeRef();
        a->host = newHost;
        if(a->host == 0)
        {
          string errMsg;
          errMsg.assign("Invalid virtual host requested from ");
          errMsg.append(a->getIpAddr());

          Server::getInstance()->logWriteln(errMsg.c_str(), MYSERVER_LOG_MSG_ERROR);

          raiseHTTPError(400);
          /*!
           *If the inputData file was not closed close it.
           */
          if(td->inputData.getHandle())
          {
            td->inputData.close();
            FilesUtility::deleteFile(td->inputDataPath);
          }
          /*!
           *If the outputData file was not closed close it.
           */
          if(td->outputData.getHandle())
          {
            td->outputData.close();
            FilesUtility::deleteFile(td->outputDataPath);
          }
          logHTTPaccess();
          return ClientsThread::DELETE_CONNECTION;
        }
      }

      if(td->request.uri.length() > 2 && td->request.uri[1] == '~')
      {
        string documentRoot;
        u_long pos = 2;
        string user;
        while(pos < td->request.uri.length())
          if(td->request.uri[++pos] == '/')
            break;
        user.assign(td->request.uri.substr(2, pos - 2));
        Server::getInstance()->getHomeDir()->getHomeDir(user,
                                                        documentRoot);

        if(documentRoot.length())
        {
          const char *useHomeDir = td->securityToken.getHashedData ("http.use_home_directory", 
                                                                    MYSERVER_VHOST_CONF |
                                                                    MYSERVER_SERVER_CONF, "YES");


          const char *homeDir = td->securityToken.getHashedData ("http.home_directory", 
                                                                 MYSERVER_VHOST_CONF |
                                                                 MYSERVER_SERVER_CONF, 
                                                                 "public_html");

          if (strcmpi (useHomeDir, "YES"))
            return raiseHTTPError (404);

          td->vhostDir.assign (documentRoot);
          td->vhostDir.append ("/");
          td->vhostDir.append (homeDir);

          if(!td->request.uriEndsWithSlash && !(td->request.uri.length() - pos))
          {
            td->request.uri.append("/");
            return sendHTTPRedirect(td->request.uri.c_str());
          }
    
          if(td->request.uri.length() - pos)
            td->request.uri.assign(td->request.uri.substr(pos,
                                                          td->request.uri.length()));
          else
            td->request.uri.assign("");
        }
      }
      
      /*!
       *Check if there is a limit for the number of connections in the
       *virtual host A value of zero means no limit.
       */
      {
        const char* val = td->securityToken.getHashedData ("MAX_CONNECTIONS", 
                                                           MYSERVER_VHOST_CONF |
                                                           MYSERVER_SERVER_CONF, NULL);
        
        if(val)
          {
            u_long limit = (u_long)atoi(val);
            if(limit && (u_long)a->host->getRef() >= limit)
              {
                retvalue = raiseHTTPError(500);
                logHTTPaccess();
                return retvalue ? ClientsThread::KEEP_CONNECTION
                  : ClientsThread::DELETE_CONNECTION;
              }
          }
      }
      
      if(td->request.isKeepAlive())
        {
          /*!
           *Support for HTTP pipelining.
           */
          if(contentLength == 0)
            {
              /*!
               *connectionBuffer is 8 KB, so don't copy more bytes.
               */
              u_long bufferStrLen = strlen(td->buffer->getBuffer());
              u_long remainingData = 0;
              
              if(bufferStrLen - td->nHeaderChars >= MYSERVER_KB(8))
                remainingData = MYSERVER_KB(8);
              else 
                remainingData = bufferStrLen - td->nHeaderChars;
              
              if(remainingData)
                {
                  u_long toCopy = nbtr - td->nHeaderChars;
                  a->connectionBuffer.setBuffer((td->buffer->getBuffer() + td->nHeaderChars), 
                                                toCopy);
                  retvalue = ClientsThread::INCOMPLETE_REQUEST_NO_WAIT;
                }
              else
                retvalue = ClientsThread::KEEP_CONNECTION;
            }
          else
            retvalue = ClientsThread::KEEP_CONNECTION;
        }
      else
        {
          retvalue = ClientsThread::DELETE_CONNECTION;
        }

      /*!
       *Set the throttling rate for the socket. This setting can be
       *changed later.
       */
      if(a->host->getThrottlingRate() == (u_long) -1)
        a->socket->setThrottling(Server::getInstance()->getThrottlingRate());
      else
        a->socket->setThrottling(a->host->getThrottlingRate());

      {
        string msg("new-http-request");
        vector<Multicast<string, void*, int>*>* handlers = 
                                 staticHttp.getHandlers(msg);

        if(handlers)
        {
          for(size_t i = 0; i < handlers->size(); i++)
          {
            int handlerRet = (*handlers)[i]->updateMulticast(getStaticData(), 
                                                             msg, td);
            if(handlerRet == 1)
            {
              ret = 1;
              retvalue = ClientsThread::DELETE_CONNECTION;
              break;
            }
            else if(handlerRet == 2)
            {
              ret = 1;
              retvalue = ClientsThread::KEEP_CONNECTION;
              break;

            }

          }
        }
      }

      if(!ret)
      {
        if (!allowMethod (td->request.cmd.c_str ()))
          return raiseHTTPError (401);

        /* GET REQUEST.  */
        if(!td->request.cmd.compare("GET"))
          ret = sendHTTPResource(td->request.uri);
        /* POST REQUEST.  */
        else if(!td->request.cmd.compare("POST"))
          ret = sendHTTPResource(td->request.uri);
        /* HEAD REQUEST.  */
        else if(!td->request.cmd.compare("HEAD"))
        {
          td->onlyHeader = 1;
          ret = sendHTTPResource(td->request.uri, 0, 1);
        }
        /* DELETE REQUEST.  */
        else if(!td->request.cmd.compare("DELETE"))
          ret = deleteHTTPRESOURCE(td->request.uri, 0, 1);
        /* PUT REQUEST.  */
        else if(!td->request.cmd.compare("PUT"))
          ret = putHTTPRESOURCE(td->request.uri, 0, 1);
        /* OPTIONS REQUEST.  */
        else if(!td->request.cmd.compare("OPTIONS"))
          ret = optionsHTTPRESOURCE(td->request.uri, 0);
        /* TRACE REQUEST.  */
        else if(!td->request.cmd.compare("TRACE"))
          ret = traceHTTPRESOURCE(td->request.uri, 0);
        else
        {
          /*
           *Return Method not implemented(501) if there
           *is not a dynamic methods manager.
           */
          if(!dynamicCommand)
            ret = raiseHTTPError(501);
          else
            retvalue = dynamicCommand->send(td, a, td->request.uri, 0, 0, 0)
              ? ClientsThread::KEEP_CONNECTION
              : ClientsThread::DELETE_CONNECTION;
        }
    }
      logHTTPaccess();
    }

    /*
     *If the inputData file was not closed close it.
     */
    if(td->inputData.getHandle())
    {
      td->inputData.close();
      FilesUtility::deleteFile(td->inputDataPath);
    }
    /*
     *If the outputData file was not closed close it.
     */
    if(td->outputData.getHandle())
    {
      td->outputData.close();
      FilesUtility::deleteFile(td->outputDataPath);
    }

    {
      HttpRequestHeader::Entry *connection =
                                    td->request.other.get("Connection");
      if(connection)
        ret &= !stringcmpi(connection->value->c_str(), "keep-alive");
      else
        ret = 0;
    }
    return (ret && retvalue != ClientsThread::DELETE_CONNECTION)
             ? retvalue
             : ClientsThread::DELETE_CONNECTION;
  }
  catch(...)
  {
    logHTTPaccess();
    return ClientsThread::DELETE_CONNECTION;
  }
}

/*!
 *Compute the Digest outputting it to a buffer.
 */
void Http::computeDigest(char* out, char* buffer)
{
  Md5 md5;
  if(!out)
    return;
  sprintf(buffer, "%i-%u-%s", (int)clock(), (u_int)td->id,
          td->connection->getIpAddr());
  md5.init();
  md5.update((unsigned char const*)buffer, (unsigned int)strlen(buffer));
  md5.end(out);
}

/*!
 *Send to the client an authorization request.
 */
int Http::requestAuthorization()
{
  Md5 md5;
  string time;
  HttpRequestHeader::Entry *connection = td->request.other.get("Connection");
  HttpRequestHeader::Entry *host = td->request.other.get("Host");
  td->response.httpStatus = 401;
  td->secondaryBuffer->setLength(0);
  *td->secondaryBuffer << "HTTP/1.1 401 Unauthorized\r\n"
               << "Accept-Ranges: bytes\r\n";
  *td->secondaryBuffer << "Server: GNU MyServer " << MYSERVER_VERSION << "\r\n";
  *td->secondaryBuffer << "Content-Type: text/html\r\n"
               << "Connection: ";
  *td->secondaryBuffer << (connection ? connection->value->c_str() : "");
  *td->secondaryBuffer << "\r\nContent-Length: 0\r\n";

  if(td->authScheme == HTTP_AUTH_SCHEME_BASIC)
  {
    *td->secondaryBuffer <<  "WWW-Authenticate: Basic realm=\""
                << (host ? host->value->c_str() : "") <<  "\"\r\n";
  }
  else if(td->authScheme == HTTP_AUTH_SCHEME_DIGEST)
  {
    char md5Str[256];

    if(td->connection->protocolBuffer == 0)
    {
      td->connection->protocolBuffer = new HttpUserData;
      if(!td->connection->protocolBuffer)
      {
        sendHTTPhardError500();
        return 0;
      }
      ((HttpUserData*)(td->connection->protocolBuffer))->reset();
    }
    myserver_strlcpy(((HttpUserData*)td->connection->protocolBuffer)->realm,
                     host ? host->value->c_str() : "", 48);

    /*! Just a random string.  */
    md5Str[0] = (char)td->id;
    md5Str[1] = (char)((clock() >> 24) & 0xFF);
    md5Str[2] = (char)((clock() >> 16) & 0xFF);
    md5Str[3] = (char)((clock() >>  8)   & 0xFF);
    md5Str[4] = (char) (clock() & 0xFF);
    strncpy(&(md5Str[5]), td->request.uri.c_str(), 256 - 5);
    md5.init();
    md5.update((unsigned char const*)md5Str,
               (unsigned int)strlen(md5Str));
    md5.end(((HttpUserData*)td->connection->protocolBuffer)->opaque);

    if(td->connection->protocolBuffer &&
       ((!(((HttpUserData*)td->connection->protocolBuffer)->digest)) ||
        (((HttpUserData*)td->connection->protocolBuffer)->nonce[0]=='\0')))
    {
      computeDigest(((HttpUserData*)td->connection->protocolBuffer)->nonce,
                    md5Str);
      ((HttpUserData*)td->connection->protocolBuffer)->nc = 0;
    }

    *td->secondaryBuffer << "WWW-Authenticate: digest "
                << " qop=\"auth\", algorithm =\"MD5\", realm =\""
                << ((HttpUserData*)td->connection->protocolBuffer)->realm
                << "\",  opaque =\""
                << ((HttpUserData*)td->connection->protocolBuffer)->opaque
                << "\",  nonce =\""
                << ((HttpUserData*)td->connection->protocolBuffer)->nonce
                <<"\" ";

    if(((HttpUserData*)td->connection->protocolBuffer)->cnonce[0])
    {
      *td->secondaryBuffer << ", cnonce =\""
                  <<((HttpUserData*)td->connection->protocolBuffer)->cnonce
                  <<"\" ";
    }
    *td->secondaryBuffer << "\r\n";
  }
  else
  {
    /*!
     *Send a non implemented error page if the auth scheme is not known.
     */
    return raiseHTTPError(501);
  }
  *td->secondaryBuffer << "Date: ";
  getRFC822GMTTime(time, HTTP_RESPONSE_DATE_DIM);
  *td->secondaryBuffer  << time
               << "\r\n\r\n";
  if(td->connection->socket->send(td->secondaryBuffer->getBuffer(),
                                 td->secondaryBuffer->getLength(), 0) == -1)
  {
    return 0;
  }
  return 1;
}

/*!
 *Sends an error page to the client.
 *Nonzero to keep the connection.
 */
int Http::raiseHTTPError(int ID)
{
  try
  {
    string time;
    ostringstream errorFile;
    string errorMessage;
    ostringstream errorBodyMessage;
    int errorBodyLength = 0;
    int useMessagesFiles = 1;
    HttpRequestHeader::Entry *host = td->request.other.get("Host");
    HttpRequestHeader::Entry *connection = td->request.other.get("Connection");
    const char *useMessagesVal = td->securityToken.getHashedData ("http.use_error_file", 
                                                                  MYSERVER_VHOST_CONF |
                                                                 MYSERVER_SERVER_CONF, NULL);

    if(useMessagesVal)
    {
      if(!strcmpi(useMessagesVal, "YES"))
         useMessagesFiles = 1;
       else
        useMessagesFiles = 0;
     }

    if(td->lastError)
    {
      td->connection->host->warningsLogWrite("Http: recursive error ");
      return sendHTTPhardError500();
    }

    td->lastError = ID;

    HttpHeaders::buildDefaultHTTPResponseHeader(&(td->response));
    if(connection && !stringcmpi(connection->value->c_str(), "keep-alive"))
    {
      td->response.connection.assign("keep-alive");
    }

    td->response.httpStatus = ID;


    char errorName [32];
    sprintf (errorName, "http.error.file.%i", ID);

    const char *defErrorFile = td->securityToken.getHashedData (errorName, 
                                                                MYSERVER_SECURITY_CONF |
                                                                MYSERVER_VHOST_CONF |
                                                                MYSERVER_SERVER_CONF);

    if (defErrorFile)
    {
      ostringstream nURL;
      int isPortSpecified = 0;
      const char* hostStr = host ? host->value->c_str() : "";
      /*!
       *Change the URI to reflect the default file name.
       */
      nURL << protocolPrefix << hostStr;
      for(int i = 0; hostStr[i]; i++)
      {
        if(hostStr[i]==':')
        {
          isPortSpecified = 1;
          break;
        }
      }
      if(!isPortSpecified)
        nURL << ":" << td->connection->host->getPort();

      if(nURL.str()[nURL.str().length()-1] != '/')
        nURL << "/";

      nURL << defErrorFile;

      if(td->pathInfo.length())
        nURL << "/" << td->pathInfo;

      if(td->request.uriOpts.length())
        nURL << "?" << td->request.uriOpts;

        return sendHTTPRedirect(nURL.str().c_str());
    }

    getRFC822GMTTime(time, HTTP_RESPONSE_DATE_EXPIRES_DIM);
    td->response.dateExp.assign(time);

    if(useMessagesFiles)
    {
      string page;
      HttpErrors::getErrorMessage(ID, td->response.errorType);
      HttpErrors::getErrorPage(ID, page);
      errorFile << td->getVhostSys() << "/" << page;

      if(FilesUtility::fileExists(errorFile.str().c_str()))
      {
        string errorFileStr = errorFile.str();
        return sendHTTPResource(errorFileStr, 1, td->onlyHeader);
      }
      else
      {
        string error = "Http: The specified error page " + errorFile.str() + " does not exist";
        td->connection->host->warningsLogWrite(error.c_str());
      }
    }

    HttpErrors::getErrorMessage(ID, errorMessage);

    /*! Send only the header (and the body if specified). */
    {
      const char* value = td->securityToken.getHashedData ("http.error_body", 
                                                           MYSERVER_VHOST_CONF |
							   MYSERVER_SERVER_CONF, NULL);

      if(value && !strcmpi(value, "NO"))
      {
        errorBodyLength = 0;
        td->response.contentLength.assign("0");
      }
      else
      {
        ostringstream size;
        errorBodyMessage << ID << " - " << errorMessage << "\r\n";
        errorBodyLength = errorBodyMessage.str().length();
        size << errorBodyLength;
        td->response.contentLength.assign(size.str());
      }
    }
    u_long hdrLen = HttpHeaders::buildHTTPResponseHeader(td->buffer->getBuffer(),
                                                         &td->response);

    if(td->connection->socket->send(td->buffer->getBuffer(), hdrLen, 0) == -1)
      return 0;

    if(errorBodyLength && (td->connection->socket->send(errorBodyMessage.str().c_str(),
                                                       errorBodyLength, 0)
                           == -1))
      return 0;

    return 1;
  }
  catch(bad_alloc &ba)
  {
    return 0;
  }
  catch(...)
  {
    return 0;
  };
}

/*!
 *Send a hard wired 500 error when we have a system error
 */
int Http::sendHTTPhardError500()
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
  HttpErrors::getErrorMessage(500, errorMsg);

  td->response.httpStatus = 500;
  td->buffer->setLength(0);
  *td->buffer <<  errorMsg;
  *td->buffer << " from: " ;
  *td->buffer << td->connection->getIpAddr() ;
  *td->buffer << "\r\n";
  td->secondaryBuffer->setLength(0);
  *td->secondaryBuffer << "HTTP/1.1 500 System Error\r\n";
  *td->secondaryBuffer << "Server: GNU MyServer " << MYSERVER_VERSION << "\r\n";
  *td->secondaryBuffer <<" Content-Type: text/html\r\nContent-Length: ";
  tmp.intToStr((int)strlen(hardHTML), tmpStr, 12);
  *td->secondaryBuffer << tmp;
  *td->secondaryBuffer << "\r\n";
  *td->secondaryBuffer <<"Date: ";
  getRFC822GMTTime(time, HTTP_RESPONSE_DATE_DIM);
  *td->secondaryBuffer << time;
  *td->secondaryBuffer << "\r\n\r\n";
  /*! Send the header.  */
  if(td->connection->socket->send(td->secondaryBuffer->getBuffer(),
                                 (u_long)td->secondaryBuffer->getLength(), 0) != -1)
  {
    /*! Send the body.  */
    if(!td->onlyHeader)
       td->connection->socket->send(hardHTML, (u_long)strlen(hardHTML), 0);
  }
  return 0;
}

/*!
 *Returns the MIME type passing its extension.
 *Returns zero if the file is registered.
 */
MimeRecord* Http::getMIME (string &filename)
{
  if(staticHttp.allowVhostMime && td->connection->host->isMIME () )
  {
    return td->connection->host->getMIME ()->getMIME (filename);
  }

  return Server::getInstance ()->getMimeManager ()->getMIME (filename);
}

/*!
 *Map an URL to the machine file system. Return 200 on success.
 *Any other return value is the HTTP error.
 */
int Http::getPath(HttpThreadContext* td, string& filenamePath, const char *filename,
                  int systemrequest)
{
  /*
   *If it is a system request, search the file in the system directory.
   */
  if(systemrequest)
  {
    if(!strlen(td->getVhostSys())
       || FilesUtility::getPathRecursionLevel(filename)< 2 )
    {
      return 401;
    }
    filenamePath.assign(td->getVhostSys());
    if(filename[0] != '/')
      filenamePath.append("/");
    filenamePath.append(filename);
  }
  /*
   *Else the file is in the web directory.
   */
  else
  {
    if(filename[0])
    {
      const char *root;
      /*
       *uri starting with a /sys/ will use the system directory as
       *the root path. Be sure to don't allow access to the system root
       *but only to subdirectories.
       */
      if(filename[0] == '/' && filename[1] == 's' && filename[2] == 'y'
         && filename[3] == 's' && filename[4] == '/')
      {
        root = td->getVhostSys();
        /*
         *Do not allow access to the system directory root but only
         *to subdirectories.
         */
        if(FilesUtility::getPathRecursionLevel(filename)< 2)
        {
          return 401;
        }
        filename = filename + 5;
      }
      else
      {
        root = td->getVhostDir();
      }
      filenamePath.assign(root);

      if(filename[0] != '/')
        filenamePath.append("/");

      filenamePath.append(filename);
    }
    else
    {
      filenamePath.append(td->getVhostDir());
    }

  }
  return 200;
}



/*!
 *If a directory is accessed try in order:
 *
 *1) The default files in order.
 *2) The directory content.
 *3) An error.
 *
 *\param uri The accessed URI.
 *\param permissions The permission mask for the client.
 *\param onlyHeader specify if the client requested only the header.
 */
int Http::processDefaultFile (string& uri, int permissions, int onlyHeader)
{
  int i;
  int ret;
  string key ("http.default_file");
  NodeTree<string> *node = td->securityToken.getNodeTree (key,
                                                          MYSERVER_VHOST_CONF |
                                                          MYSERVER_SERVER_CONF, NULL);


  if (node)
    {
      list<NodeTree<string>*> *children = node->getChildren ();

      for(list<NodeTree<string>*>::iterator it = children->begin ();
          it != children->end ();
          it++)
        {
          ostringstream defaultFileName;
          const string *file = (*it)->getValue ();
          defaultFileName.clear();
          defaultFileName << td->filenamePath << "/" << *file;

          if(FilesUtility::fileExists (defaultFileName.str ().c_str ()))
            {
              ostringstream nUrl;

              if (td->request.uriEndsWithSlash)
                nUrl << *file;
              else
                {
                  u_long lastSlashOffset = uri.length();
                  while (lastSlashOffset && uri[lastSlashOffset] != '/')
                    --lastSlashOffset;

                  nUrl << &(uri.c_str ()[lastSlashOffset < uri.length() ?
                                         lastSlashOffset + 1 : 0])
                       << "/" << *file;
                }

              if (td->pathInfo.length())
                nUrl << "/" << td->pathInfo;

              if (td->request.uriOpts.length())
                nUrl << "?" << td->request.uriOpts;

              /*! Send a redirect to the new location.  */
              if (sendHTTPRedirect(nUrl.str().c_str()))
                ret = 1;
              else
                ret = 0;

              return ret;
            }
        }
    }

  HttpDataHandler *handler = staticHttp.dynManagerList.getHttpManager ("DIR");

  if (!handler)
    {
      td->connection->host->warningsLogWrite("Http: cannot find a valid handler");
      return raiseHTTPError (500);
    }

  return handler->send (td, td->filenamePath.c_str(), 0, onlyHeader);
}

/*!
 *Send a redirect message to the client.
 */
int Http::sendHTTPRedirect(const char *newURL)
{
  string time;
  HttpRequestHeader::Entry *connection = td->request.other.get("Connection");

  td->response.httpStatus = 302;
  td->secondaryBuffer->setLength(0);
  *td->secondaryBuffer << "HTTP/1.1 302 Moved\r\nAccept-Ranges: bytes\r\n"
              << "Server: GNU MyServer "  << MYSERVER_VERSION << "\r\n"
              << "Content-Type: text/html\r\n"
              << "Location: " << newURL << "\r\n"
              << "Content-Length: 0\r\n";

  if(connection && !stringcmpi(connection->value->c_str(), "keep-alive"))
    *td->secondaryBuffer << "Connection: keep-alive\r\n";
  else
    *td->secondaryBuffer << "Connection: close\r\n";

  *td->secondaryBuffer<< "Date: ";
  getRFC822GMTTime(time, HTTP_RESPONSE_DATE_DIM);
  *td->secondaryBuffer << time
              << "\r\n\r\n";
  if(td->connection->socket->send(td->secondaryBuffer->getBuffer(),
                                 (int)td->secondaryBuffer->getLength(), 0) == -1)
    return 0;

  return 1;
}

/*!
 *Send a non-modified message to the client.
 */
int Http::sendHTTPNonModified()
{
  string time;
  HttpRequestHeader::Entry *connection = td->request.other.get("Connection");

  td->response.httpStatus = 304;
  td->secondaryBuffer->setLength(0);
  *td->secondaryBuffer << "HTTP/1.1 304 Not Modified\r\nAccept-Ranges: bytes\r\n"
              << "Server: GNU MyServer "  << MYSERVER_VERSION << "\r\n";

  if(connection && !stringcmpi(connection->value->c_str(), "keep-alive"))
    *td->secondaryBuffer << "Connection: keep-alive\r\n";
  else
    *td->secondaryBuffer << "Connection: close\r\n";

  getRFC822GMTTime(time, HTTP_RESPONSE_DATE_DIM);

  *td->secondaryBuffer << "Date: " << time << "\r\n\r\n";

  if(td->connection->socket->send(td->secondaryBuffer->getBuffer(),
                                 (int)td->secondaryBuffer->getLength(), 0) == -1)
    return 0;
  return 1;
}

/*!
 *Send a 401 error.
 */
int Http::sendAuth()
{
  if(td->connection->getnTries() > 2)
  {
    return raiseHTTPError(401);
  }
  else
  {
    td->connection->incnTries();
    return requestAuthorization();
  }
}

/*!
 *Load the HTTP protocol.
 */
int Http::loadProtocolStatic(XmlParser* languageParser)
{
  const char *data = NULL;
  string pluginsResource(Server::getInstance()->getExternalPath());

  /*
   *Store defaults value.
   *By default use GZIP with files bigger than a MB.
   */
  staticHttp.timeout = MYSERVER_SEC(15);

  Server::getInstance()->setGlobalData("http-static", getStaticData());

  staticHttp.dynManagerList.addHttpManager ("SEND", new HttpFile ());
  staticHttp.dynManagerList.addHttpManager ("DIR", new HttpDir ());
  staticHttp.dynManagerList.addHttpManager ("CGI", new Cgi ());
  staticHttp.dynManagerList.addHttpManager ("MSCGI", new MsCgi ());
  staticHttp.dynManagerList.addHttpManager ("SCGI", new Scgi ());
  staticHttp.dynManagerList.addHttpManager ("WINGI", new WinCgi ());
  staticHttp.dynManagerList.addHttpManager ("FASTCGI", new FastCgi ());
  staticHttp.dynManagerList.addHttpManager ("ISAPI", new Isapi ());
  staticHttp.dynManagerList.addHttpManager ("PROXY", new Proxy ());

  data = Server::getInstance ()->getHashedData ("vhost.allow_mime");
  if(data)
  {

    if(!strcmpi(data, "YES"))
      staticHttp.allowVhostMime = 1;
    else
      staticHttp.allowVhostMime = 0;
  }
  data = Server::getInstance ()->getHashedData ("cgi.timeout");
  if(data)
  {
    staticHttp.timeout = MYSERVER_SEC (atoi (data));
  }

  return 1;
}

/*!
 *Unload the HTTP protocol.
 */
int Http::unLoadProtocolStatic(XmlParser* languageParser)
{
  /* Unload the errors.  */
  HttpErrors::unLoad();

  staticHttp.clear();

  return 1;
}

/*!
 *Returns the name of the protocol. If an out buffer
 *is defined fullfill it with the name too.
 */
char* Http::registerNameImpl(char* out, int len)
{
  if(out)
  {
    myserver_strlcpy(out, "HTTP", len);
  }
  return (char*) "HTTP";
}

/*!
 *Constructor for the class http.
 */
Http::Http()
{
  td = new HttpThreadContext();

  protocolPrefix.assign("http://");
  protocolOptions = 0;
  td->filenamePath.assign("");
  td->pathInfo.assign("");
  td->pathTranslated.assign("");
  td->cgiRoot.assign("");
  td->cgiFile.assign("");
  td->scriptPath.assign("");
  td->scriptDir.assign("");
  td->scriptFile.assign("");
  td->inputDataPath.assign("");
  td->outputDataPath.assign("");
}

/*!
 *Destructor for the http class.
 */
Http::~Http()
{
  clean();
}

/*!
 *Clean the used memory.
 */
void Http::clean()
{
  td->filenamePath.assign("");
  td->pathInfo.assign("");
  td->pathTranslated.assign("");
  td->cgiRoot.assign("");
  td->cgiFile.assign("");
  td->scriptPath.assign("");
  td->scriptDir.assign("");
  td->scriptFile.assign("");
  td->inputDataPath.assign("");
  td->outputDataPath.assign("");
  delete td;
}
