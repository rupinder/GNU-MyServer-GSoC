/*
*MyServer
*Copyright (C) 2002, 2003, 2004, 2005 The MyServer Team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "../include/http.h"
#include "../include/http_headers.h"
#include "../include/cserver.h"
#include "../include/security.h"
#include "../include/AMMimeUtils.h"
#include "../include/cgi.h"
#include "../include/filemanager.h"
#include "../include/clientsThread.h"
#include "../include/sockets.h"
#include "../include/winCGI.h"
#include "../include/fastCGI.h"
#include "../include/utility.h"
#include "../include/md5.h"
#include "../include/isapi.h"
#include "../include/stringutils.h"
#include "../include/securestr.h"

#include <string>
#include <ostream>

using namespace std;

extern "C" 
{
#ifdef WIN32
#include <direct.h>
#include <errno.h>
#endif

#ifdef NOT_WIN
#include <string.h>
#include <errno.h>
#endif
}

#ifdef NOT_WIN
#include "../include/lfind.h"
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

// Bloodshed Dev-C++ Helper
#ifndef intptr_t
#define intptr_t int
#endif

/*! Store if the MSCGI library was loaded.  */
int Http::mscgiLoaded=0;

/*! Allow the definition of a MIME file for host. */
int Http::allowVhostMime=1;

/*! Allow the possibility to use MSCGI. */
int Http::allowMscgi=1;

/*! Path to the .css file used by directory browsing.  */
string Http::browseDirCSSpath;

/*! Threshold value to send data in gzip.  */
u_long Http::gzipThreshold=0;

/*!Use files for HTTP errors?  */
int Http::useMessagesFiles=0;

/*! Vector with default filenames.  */
vector<string*> Http::defaultFilename;

/*! Is the HTTP protocol loaded?  */
int Http::initialized=0;

/*! If not specified differently use a timeout of 15 seconds.  */
int Http::cgiTimeout=MYSERVER_SEC(15);

/*! Max number of FastCGI servers allowed to run. */
int Http::fastcgiServers;

/*! Cache for security files. */
SecurityCache Http::secCache;

/*! Access the security cache safely. */
Mutex Http::secCacheMutex;

/*! Initial port for FastCGI servers. */
int Http::fastcgiInitialPort=3333;

/*!
 *Build a response for an OPTIONS request.
 */
int Http::optionsHTTPRESOURCE(HttpThreadContext* td, ConnectionPtr s, 
                              string& /*filename*/, int /*yetmapped*/)
{
	int ret;
	string time;
  try
  {
    getRFC822GMTTime(time, HTTP_RESPONSE_DATE_DIM);
    td->buffer2->SetLength(0);
    *td->buffer2 <<  "HTTP/1.1 200 OK\r\n";
    *td->buffer2 << "Date: " << time ;
    *td->buffer2 <<  "\r\nServer: MyServer "  << versionOfSoftware ;
    if(td->request.CONNECTION.length())
      *td->buffer2 << "\r\nConnection:" << td->request.CONNECTION.c_str() ;
    *td->buffer2 <<"\r\nContent-length: 0\r\nAccept-Ranges: bytes\r\n";
    *td->buffer2 << "Allow: OPTIONS, GET, POST, HEAD, DELETE, PUT";
    
    /*!
     *Check if the TRACE command is allowed on the virtual host.
     */
    if(allowHTTPTRACE(td, s))
      *td->buffer2 << ", TRACE\r\n\r\n";
    else
      *td->buffer2 << "\r\n\r\n";
    
    /*! Send the HTTP header. */
    ret = s->socket.send(td->buffer2->GetBuffer(), 
                         (u_long)td->buffer2->GetLength(), 0);
    if( ret == SOCKET_ERROR )
    {
      return 0;
    }
    return 1;
  }
  catch(...)
  {
    return raiseHTTPError(td, s, e_500); 
  };
}

/*!
 *Handle the HTTP TRACE command.
 */
int Http::traceHTTPRESOURCE(HttpThreadContext* td, ConnectionPtr s, 
                            string& /*filename*/, int /*yetmapped*/)
{
	int ret;
	char tmpStr[12];
	int content_len=(int)td->nHeaderChars;
	string time;
  try
  {
    CMemBuf tmp;
    tmp.IntToStr(content_len, tmpStr, 12);
    getRFC822GMTTime(time, HTTP_RESPONSE_DATE_DIM);
    if(!allowHTTPTRACE(td, s))
      return raiseHTTPError(td, s, e_401);
    td->buffer2->SetLength(0);
    *td->buffer2 <<  "HTTP/1.1 200 OK\r\n";
    *td->buffer2 << "Date: " << time ;
    *td->buffer2 <<  "\r\nServer: MyServer "  << versionOfSoftware ;
    if(td->request.CONNECTION.length())
      *td->buffer2 << "\r\nConnection:" << td->request.CONNECTION.c_str() ;
    *td->buffer2 <<"\r\nContent-length:" << tmp
                 << "\r\nContent-Type: message/http\r\nAccept-Ranges: bytes\r\n\r\n";
    
    /*! Send our HTTP header. */
    ret = s->socket.send(td->buffer2->GetBuffer(), 
                         (u_long)td->buffer2->GetLength(), 0);
    if( ret == SOCKET_ERROR )
    {
      return 0;
    }
    
    /*! Send the client request header as the HTTP body. */
    ret = s->socket.send(td->buffer->GetBuffer(), content_len, 0);
    if(ret == SOCKET_ERROR)
    {
      return 0;
    }
    return 1;
  }
  catch(...)
  {
    return raiseHTTPError(td, s, e_500); 
  };
}
  
/*!
 *Check if the host allows the HTTP TRACE command
 */
int Http::allowHTTPTRACE(HttpThreadContext* td, ConnectionPtr s)
{
	int ret;
	/*! Check if the host allows HTTP trace. */
	ostringstream filename;
  char *http_trace_value;
	XmlParser parser;
	
  filename << ((Vhost*)(s->host))->getDocumentRoot() << "/security" ;
	if(parser.open(filename.str().c_str()))
	{
		return 0;
	}
	http_trace_value=parser.getAttr("HTTP", "TRACE");
	
  /*! 
   *If the returned value is equal to ON so the 
   *HTTP TRACE is active for this vhost.  
   *By default don't allow the trace.
   */
	if(http_trace_value &&  !lstrcmpi(http_trace_value, "ON"))
		ret = 1;
	else
		ret = 0;
	parser.close();
	return ret;
}

/*!
 *Get the timeout for the cgi.
 */
int Http::getCGItimeout()
{
  return cgiTimeout;
}

/*!
 *Main function to handle the HTTP PUT command.
 */
int Http::putHTTPRESOURCE(HttpThreadContext* td, ConnectionPtr s, 
                          string& filename, int, int, int yetmapped)
{
  u_long firstByte = td->request.RANGEBYTEBEGIN; 
  int permissions=-1;
  string directory;
	int httpStatus=td->response.httpStatus;
	int keepalive=0;
  int permissions2=0;
  char auth_type[16];	
  SecurityToken st;

  st.auth_type = auth_type;
  st.len_auth = 16;
  try
  {
    HttpHeaders::buildDefaultHTTPResponseHeader(&td->response);
    if(!stringcmpi(td->request.CONNECTION, "Keep-Alive"))
    {
      td->response.CONNECTION.assign("Keep-Alive");
      keepalive=1;
    }
    td->response.httpStatus=httpStatus;
    /*!
     *td->filenamePath is the file system mapped path while filename 
     *is the URI requested.
     *systemrequest is 1 if the file is in the system directory.
     *If filename is already mapped on the file system don't map it again.
     */
    if(yetmapped)
    {
      td->filenamePath.assign(filename);
    }
    else
    {
      int ret;
      /*! 
       *If the client tries to access files that aren't in the web directory 
       *send a 401 error.  
       */
      translateEscapeString(filename);
      if((filename[0] != '\0') && 
         (File::getPathRecursionLevel(filename)<1))
      {
        return raiseHTTPError(td, s, e_401);
      }
      ret = getPath(td, s, td->filenamePath, filename, 0);
      if(ret!=e_200)
        return raiseHTTPError(td, s, ret);
    }
    if(File::isDirectory(td->filenamePath.c_str()))
    {
      directory.assign(td->filenamePath);
    }
    else
    {
      string fn;
      fn.assign(filename);
      File::splitPath(td->filenamePath, directory, fn);
    }
    if(s->protocolBuffer==0)
    {
      s->protocolBuffer=new HttpUserData;
      if(!s->protocolBuffer)
      {
        return sendHTTPhardError500(td, s);
      }
      ((HttpUserData*)(s->protocolBuffer))->reset();
    }
    if(td->request.AUTH.length())
    {
      st.user = s->getLogin();
      st.password = s->getPassword();
      st.directory = directory.c_str();
      st.sysdirectory = ((Vhost*)(s->host))->getSystemRoot();
      st.filename = filename.c_str();
      st.password2 = ((HttpUserData*)s->protocolBuffer)->neededPassword;
      st.permission2 = &permissions2;
      secCacheMutex.lock();
      try
      {
        permissions=secCache.getPermissionMask(&st);
        secCacheMutex.unlock();  
      }
      catch(...)
      {
        secCacheMutex.unlock();
        throw;
      };
    }
    else/*! The default user is Guest with a null password. */
    {
      secCacheMutex.lock();
      try
      {
        st.user = "Guest";
        st.password = "";
        st.directory = directory.c_str();
        st.sysdirectory = ((Vhost*)(s->host))->getSystemRoot();
        st.filename = filename.c_str();
        st.password2 = 0;
        st.permission2 = 0;
        permissions=secCache.getPermissionMask(&st);
        secCacheMutex.unlock();
      }
      catch(...)
      {
        secCacheMutex.unlock();
        throw;
      };
    }

    if(permissions == -1)
    {
      ((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
      ((Vhost*)td->connection->host)->warningsLogWrite("Error reading security file\n");
      ((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
      return raiseHTTPError(td, s, e_500); 
    }
    /*! Check if we have to use digest for the current directory. */
    if(!lstrcmpi(auth_type, "Digest"))
    {
      if(!td->request.AUTH.compare("Digest"))
      {
        if(!((HttpUserData*)s->protocolBuffer)->digestChecked)
          ((HttpUserData*)s->protocolBuffer)->digest = checkDigest(td, s);
        ((HttpUserData*)s->protocolBuffer)->digestChecked=1;
        if(((HttpUserData*)s->protocolBuffer)->digest==1)
        {
          s->setPassword(((HttpUserData*)s->protocolBuffer)->neededPassword);
          permissions=permissions2;
        }
      }
      td->auth_scheme=HTTP_AUTH_SCHEME_DIGEST;
    }
    else/*! By default use the Basic authentication scheme. */
    {
      td->auth_scheme=HTTP_AUTH_SCHEME_BASIC;
    }	
    /*! If there are no permissions, use the Guest permissions. */
    if(td->request.AUTH.length() && (permissions==0))
    {
      st.user = "Guest";
      st.password = "";
      st.directory = directory.c_str();
      st.sysdirectory = ((Vhost*)(s->host))->getSystemRoot();
      st.filename = filename.c_str();
      st.password2 = 0;
      st.permission2 = 0;
      
      secCacheMutex.lock();
      try
      {
        permissions=secCache.getPermissionMask(&st);		
        secCacheMutex.unlock();
      }
      catch(...)
      {
        secCacheMutex.unlock();
        throw;
      };
    }
    if(permissions == -1)
    {
      ((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
      ((Vhost*)td->connection->host)->warningsLogWrite("Error reading security file\n");
      ((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
      return raiseHTTPError(td, s, e_500); 
    }
    if(!(permissions & MYSERVER_PERMISSION_WRITE))
    {
      return sendAuth(td, s);
    }
    if(File::fileExists(td->filenamePath.c_str()))
    {
      /*! If the file exists update it. */
      File file;
      if(file.openFile(td->filenamePath.c_str(), FILE_OPEN_IFEXISTS | 
                       FILE_OPEN_WRITE))
      {
        /*! Return an internal server error. */
        return raiseHTTPError(td, s, e_500);
      }
      file.setFilePointer(firstByte);
      for(;;)
      {
        u_long nbr=0, nbw=0;
        if(td->inputData.readFromFile(td->buffer->GetBuffer(), 
                                      td->buffer->GetRealLength(), &nbr))
        {
          file.closeFile();
          /*! Return an internal server error. */
          return raiseHTTPError(td, s, e_500);
        }
        if(nbr)
        {
          if(file.writeToFile(td->buffer->GetBuffer(), nbr, &nbw))
          {
            file.closeFile();
            /*! Return an internal server error. */
            return raiseHTTPError(td, s, e_500);
          }
        }
        else
          break;
        if(nbw!=nbr)
        {
          file.closeFile();
          /*! Internal server error. */
          return raiseHTTPError(td, s, e_500);
        }
      }
      file.closeFile();
      /*! Successful updated. */
      raiseHTTPError(td, s, e_200);
      
      return keepalive;
    }
    else
    {
      /*!
       *If the file doesn't exist create it.
       */
      File file;
      if(file.openFile(td->filenamePath.c_str(), 
                       FILE_CREATE_ALWAYS|FILE_OPEN_WRITE))
      {
        /*! Internal server error. */
        return raiseHTTPError(td, s, e_500);
      }
      for(;;)
      {
        u_long nbr=0, nbw=0;
        if(td->inputData.readFromFile(td->buffer->GetBuffer(), 
                                      td->buffer->GetRealLength(), &nbr))
        {
          file.closeFile();
          return raiseHTTPError(td, s, e_500);
        }
        if(nbr)
        {
          if(file.writeToFile(td->buffer->GetBuffer(), nbr, &nbw))
          {
            file.closeFile();
            return raiseHTTPError(td, s, e_500);
          }
        }
        else
          break;
        if( nbw != nbr )
        {
          file.closeFile();
          return raiseHTTPError(td, s, e_500);
        }
      }
      file.closeFile();
      /*! Successful created. */
      raiseHTTPError(td, s, e_201);
      return 1;
    }
	}
  catch(...)
  {
    return raiseHTTPError(td, s, e_500); 
  };



}

/*!
 *Delete the resource identified by filename.
 */
int Http::deleteHTTPRESOURCE(HttpThreadContext* td, ConnectionPtr s, 
                             string& filename, int yetmapped)
{
  int permissions=-1;
  string directory;
	int httpStatus=td->response.httpStatus;
	int permissions2=0;
	char auth_type[16];
  SecurityToken st;
  try
  {
    HttpHeaders::buildDefaultHTTPResponseHeader(&td->response);
    if(!stringcmpi(td->request.CONNECTION, "Keep-Alive"))
    {
      td->response.CONNECTION.assign( "Keep-Alive");
    }
    st.auth_type = auth_type;
    st.len_auth = 16;
    
    td->response.httpStatus=httpStatus;
    /*!
     *td->filenamePath is the file system mapped path while filename 
     *is the URI requested.
     *systemrequest is 1 if the file is in the system directory.
     *If filename is already mapped on the file system don't map it again.
     */
    if(yetmapped)
    {
      td->filenamePath.assign(filename);
    }
    else
    {
      int ret;
      /*!
       *If the client tries to access files that aren't in the web directory 
       *send a HTTP 401 error page.
       */
      translateEscapeString(filename );
      if((filename[0] != '\0')&&(File::getPathRecursionLevel(filename)<1))
      {
        return raiseHTTPError(td, s, e_401);
      }
      ret=getPath(td, s, td->filenamePath, filename, 0);
      if(ret!=e_200)
        return raiseHTTPError(td, s, ret);
    }
    if(File::isDirectory(td->filenamePath.c_str()))
    {
      directory.assign(td->filenamePath);
    }
    else
    {
      string file;
      File::splitPath(td->filenamePath, directory, file);
    }

    if(s->protocolBuffer==0)
    {
      s->protocolBuffer=new HttpUserData;
      if(!s->protocolBuffer)
      {
        return 0;
      }
      ((HttpUserData*)(s->protocolBuffer))->reset();
    }

    if(td->request.AUTH.length())
    {
      st.user = s->getLogin();
      st.password = s->getPassword();
      st.directory = directory.c_str();
      st.sysdirectory = ((Vhost*)(s->host))->getSystemRoot();
      st.filename = filename.c_str();
      st.password2 = ((HttpUserData*)s->protocolBuffer)->neededPassword;
      st.permission2 = &permissions2;
      secCacheMutex.lock();
      try
      {
        permissions=secCache.getPermissionMask(&st);
        secCacheMutex.unlock();
      }
      catch(...)
      {
        secCacheMutex.unlock();
        throw;
      };
    }
    else/*! The default user is Guest with a null password. */
    {
      st.user = "Guest";
      st.password = "";
      st.directory = directory.c_str();
      st.sysdirectory = ((Vhost*)(s->host))->getSystemRoot();
      st.filename = filename.c_str();
      st.password2 = 0;
      st.permission2 = 0;
      secCacheMutex.lock();
      try
      {
        permissions=secCache.getPermissionMask(&st);
        secCacheMutex.unlock();
      }
      catch(...)
      {
        secCacheMutex.unlock();
        throw;
      };
    }	
    if(permissions == -1)
    {
      ((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
      ((Vhost*)td->connection->host)->warningsLogWrite("Error reading security file\n");
      ((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
      return raiseHTTPError(td, s, e_500); 
    }
    /*! Check if we have to use digest for the current directory. */
    if(!lstrcmpi(auth_type, "Digest"))
    {
      if(!td->request.AUTH.compare("Digest"))
      {
        if(!((HttpUserData*)s->protocolBuffer)->digestChecked)
          ((HttpUserData*)s->protocolBuffer)->digest = checkDigest(td, s);
        ((HttpUserData*)s->protocolBuffer)->digestChecked=1;
        if(((HttpUserData*)s->protocolBuffer)->digest==1)
        {
          s->setPassword(((HttpUserData*)s->protocolBuffer)->neededPassword);
           permissions=permissions2;
        }
      }
      td->auth_scheme=HTTP_AUTH_SCHEME_DIGEST;
    }
    /*! By default use the Basic authentication scheme. */
    else
    {
      td->auth_scheme=HTTP_AUTH_SCHEME_BASIC;
    }	
    /*! If there are no permissions, use the Guest permissions. */
    if(td->request.AUTH.length() && (permissions==0))
    {
      st.user = "Guest";
      st.password = "";
      st.directory = directory.c_str();
      st.sysdirectory = ((Vhost*)(s->host))->getSystemRoot();
      st.filename = filename.c_str();
      st.password2 = 0;
      st.permission2 = 0;
      secCacheMutex.lock();
      try
      {
        permissions=secCache.getPermissionMask(&st);	
        secCacheMutex.unlock();
      }
      catch(...)
      {
        secCacheMutex.unlock();
        throw;
      };
    }
    if(permissions == -1)
    {
      ((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
      ((Vhost*)td->connection->host)->warningsLogWrite("Error reading security file\n");
      ((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
      return raiseHTTPError(td, s, e_500); 
    }
    if(!(permissions & MYSERVER_PERMISSION_DELETE))
	  {
      return sendAuth(td, s);
    }
    if(File::fileExists(td->filenamePath))
	  {
      File::deleteFile(td->filenamePath.c_str());
      /*! Successful deleted. */
      return raiseHTTPError(td, s, e_202);
    }
    else
  	{
      /*! No content. */
      return raiseHTTPError(td, s, e_204);
    }
  }
  catch(...)
  {
    return raiseHTTPError(td, s, e_500); 
  };

}

/*!
 *Check the Digest authorization
 */
u_long Http::checkDigest(HttpThreadContext* td, ConnectionPtr s)
{
  Md5 md5;
	char A1[48];
	char A2[48];
	char response[48];
  char *uri;
	u_long digest_count;
  /*! Return 0 if the password is different. */
	if(td->request.digestOpaque[0] && lstrcmp(td->request.digestOpaque, 
                                             ((HttpUserData*)s->protocolBuffer)->opaque))
		return 0;
  /*! If is not equal return 0. */
	if(lstrcmp(td->request.digestRealm, ((HttpUserData*)s->protocolBuffer)->realm))
		return 0;
	
	digest_count = hexToInt(td->request.digestNc);
	
	if(digest_count != ((HttpUserData*)s->protocolBuffer)->nc+1)
		return 0;
	else
		((HttpUserData*)s->protocolBuffer)->nc++;
   
	md5.init();
	td->buffer2->SetLength(0);
	*td->buffer2 << td->request.digestUsername << ":" << td->request.digestRealm 
               << ":" << ((HttpUserData*)s->protocolBuffer)->neededPassword;

	md5.update((unsigned char const*)td->buffer2->GetBuffer(), 
             (unsigned int)td->buffer2->GetLength());
	md5.end(A1);
	
	md5.init();

	if(td->request.digestUri[0])
		uri=td->request.digestUri;
  else
    uri=(char*)td->request.URIOPTS.c_str();

	td->buffer2->SetLength(0);
	*td->buffer2 <<  td->request.CMD.c_str() <<  ":" << uri;
	md5.update((unsigned char const*)td->buffer2->GetBuffer(), 
             (unsigned int)td->buffer2->GetLength());
	md5.end( A2);
	
	md5.init();
	td->buffer2->SetLength(0);
	*td->buffer2 << A1 << ":"  << ((HttpUserData*)s->protocolBuffer)->nonce << ":" 
               << td->request.digestNc << ":"  << td->request.digestCnonce << ":" 
               << td->request.digestQop  << ":" << A2;
	md5.update((unsigned char const*)td->buffer2->GetBuffer(), 
             (unsigned int)td->buffer2->GetLength());
	md5.end(response);	

	if(!lstrcmp(response, td->request.digestResponse))
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
	realm[0]='\0';
	opaque[0]='\0';
	nonce[0]='\0';
	cnonce[0]='\0';
	digestChecked=0;
	neededPassword[0]='\0';
	nc=0;
	digest=0;
}


/*!
 *Main function to send a resource to a client.
 */
int Http::sendHTTPResource(HttpThreadContext* td, ConnectionPtr s, string& URI, 
                           int systemrequest, int onlyHeader, int yetmapped)
{
	/*!
   *With this code we manage a request of a file or a directory or anything 
   *that we must send over the HTTP.
   */
	string filename;
  int permissions;
  int permissions2;
  string dirscan;
	int filenamePathLen;
  string data;
	int mimeCMD;
	time_t lastMT;
  int ret;
  char auth_type[16];
  string tmpTime;
  SecurityToken st;

  try
  {
    st.auth_type = auth_type;
    st.len_auth = 16;
    
    filename.assign(URI);
    td->buffer->SetLength(0);
    
    HttpHeaders::buildDefaultHTTPResponseHeader(&td->response);	
    if(!stringcmpi(td->request.CONNECTION, "Keep-Alive"))
    {
      td->response.CONNECTION.assign("Keep-Alive");
    }
    

    /*!
     *td->filenamePath is the file system mapped path while filename 
     *is the URI requested. systemrequest is 1 if the file is in 
     *the system directory.
     *If filename is already mapped on the file system don't map it again.
     */
    if(yetmapped)
    {
      td->filenamePath.assign(filename);
    }
    else
    {
      int ret;
      /*!
       *If the client tries to access files that aren't in the 
       *web directory send a 401 error.
       */
      translateEscapeString(filename);
      if(filename.length() && (filename[0] != '\0')&&
         (File::getPathRecursionLevel(filename)<1))
      {
        return raiseHTTPError(td, s, e_401);
      }
      /*! getPath will alloc the buffer for filenamePath. */
      ret=getPath(td, s, td->filenamePath, filename.c_str(), systemrequest);
      if(ret!=e_200)
        return raiseHTTPError(td, s, ret);
    }

    /*! By default allows only few actions. */
    permissions= MYSERVER_PERMISSION_READ |  MYSERVER_PERMISSION_BROWSE ;

    if(!systemrequest)
    {
      string directory;
      if(File::isDirectory(td->filenamePath.c_str()))
      {
        directory.assign(td->filenamePath);
      }
      else if(File::isLink(td->filenamePath.c_str())) 
      {
        return raiseHTTPError(td, s, e_401);
      }
      else
      {
        string fn;
        File::splitPath(td->filenamePath, directory, fn);
      }

      if(s->protocolBuffer==0)
      {
        s->protocolBuffer=new HttpUserData;
        if(!s->protocolBuffer)
        {
          return sendHTTPhardError500(td, s);
        }
        ((HttpUserData*)s->protocolBuffer)->reset();
      }
      permissions2=0;
      if(td->request.AUTH.length())
      {
        st.user = s->getLogin();
        st.password = s->getPassword();
        st.directory = directory.c_str();
        st.sysdirectory = ((Vhost*)(s->host))->getSystemRoot();
        st.filename = filename.c_str();
        st.password2 = ((HttpUserData*)s->protocolBuffer)->neededPassword;
        st.permission2 = &permissions2;
        secCacheMutex.lock();
        try
        {
          permissions=secCache.getPermissionMask(&st);
          secCacheMutex.unlock();
        }
        catch(...)
        {
          secCacheMutex.unlock();
          throw;
        };
      }
      else/*! The default user is Guest with a null password. */
      {
        st.user = "Guest";
        st.password = "";
        st.directory = directory.c_str();
        st.sysdirectory = ((Vhost*)(s->host))->getSystemRoot();
        st.filename = filename.c_str();
        st.password2 = 0;
        st.permission2 = 0;
        secCacheMutex.lock();
        try
        {
          permissions=secCache.getPermissionMask(&st);
          secCacheMutex.unlock();
        }
        catch(...)
        {
          secCacheMutex.unlock();
          throw;
        };
      }	

      /*! Check if we have to use digest for the current directory. */
      if(!lstrcmpi(auth_type, "Digest"))
		  {
        if(!td->request.AUTH.compare("Digest"))
			  {
          if(!((HttpUserData*)s->protocolBuffer)->digestChecked)
            ((HttpUserData*)s->protocolBuffer)->digest = checkDigest(td, s);

          ((HttpUserData*)s->protocolBuffer)->digestChecked=1;

          if(((HttpUserData*)s->protocolBuffer)->digest==1)
          {
            s->setPassword(((HttpUserData*)s->protocolBuffer)->neededPassword);
            permissions=permissions2;
          }
        }
        td->auth_scheme=HTTP_AUTH_SCHEME_DIGEST;
      }
      else/*! By default use the Basic authentication scheme. */
      {
        td->auth_scheme=HTTP_AUTH_SCHEME_BASIC;
      }	
 
     /*! If there are no permissions, use the Guest permissions. */
      if(td->request.AUTH.length() && (permissions==0))
      {
        st.user = "Guest";
        st.password = "";
        st.directory = directory.c_str();
        st.sysdirectory = ((Vhost*)(s->host))->getSystemRoot();
        st.filename = filename.c_str();
        st.password2 = 0;
        st.permission2 = 0;
        secCacheMutex.lock();
        try
        {
          permissions=secCache.getPermissionMask(&st);
          secCacheMutex.unlock();
        }
        catch(...)
        {
          secCacheMutex.unlock();
          throw;
        };
      }
    }

    if(permissions==-1)
    {
      ((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
      ((Vhost*)td->connection->host)->warningsLogWrite("Error reading security file\n");
      ((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
      return raiseHTTPError(td, s, e_500); 
    }


    /*! If a throttling rate was specifed use it. */
    if(st.throttlingRate != -1)
      s->socket.setThrottling(st.throttlingRate);


    /*!
     *Get the PATH_INFO value.
     *Use dirscan as a buffer for put temporary directory scan.
     *When an '/' character is present check if the path up to '/' character
     *is a file. If it is a file send the rest of the URI as PATH_INFO.
     */
    td->pathInfo.assign("");
    td->pathTranslated.assign("");
    filenamePathLen=(int)td->filenamePath.length();
    dirscan.assign("");
    for(int i=0, len=0; i<filenamePathLen ; i++)
	  {
      /*!
       *http://host/pathtofile/filetosend.php/PATH_INFO_VALUE?QUERY_INFO_VALUE
       *When a request has this form send the file filetosend.php with the
       *environment string PATH_INFO equals to PATH_INFO_VALUE and QUERY_INFO
       *to QUERY_INFO_VALUE.
       *
       *If there is the '/' character check if dirscan is a file.
       */
      if(i && (td->filenamePath[i]=='/'))
      {
        /*!
         *If the token is a file.
         */
        if(!File::isDirectory(dirscan.c_str()))
        {
          td->pathInfo.assign((char*)&(td->filenamePath[i]));
          td->filenamePath.assign(dirscan);
          break;
        }
      }
      if(len+1 < filenamePathLen)
      {
        char db[2];
        db[0]=(td->filenamePath)[i];
        db[1]='\0';
        dirscan.append(db);
      }
    }

    /*!
     *If there is a PATH_INFO value the get the PATH_TRANSLATED too.
     *PATH_TRANSLATED is the local filesystem mapped version of PATH_INFO.
     */
    if(td->pathInfo.length()>1)
    {
      int ret;
      /*!
       *Start from the second character because the first is a
       *slash character.  
       */
      ret=getPath(td, s, td->pathTranslated, &((td->pathInfo.c_str())[1]), 0);
      if(ret!=e_200)
        return raiseHTTPError(td, s, ret);
      File::completePath(td->pathTranslated);
    }
    else
    {
      td->pathTranslated.assign("");
    }
    File::completePath(td->filenamePath);


    /*!
     *Raise an error if the token is a link.
     */
    if(File::isLink(td->filenamePath.c_str())) 
    {
      return raiseHTTPError(td, s, e_401);
    }

    /*!
     *If there are not any extension then we do one of this in order:
     *1)We send the default files in the directory in order.
     *2)We send the directory content.
     *3)We send an error.
     */
    if(File::isDirectory(td->filenamePath.c_str()))
    {
      int i;
      if(!(permissions & MYSERVER_PERMISSION_BROWSE))
      {
        return sendAuth(td, s);
      }
      for(i=0;;i++)
      {
        const char *defaultFileNamePath=getDefaultFilenamePath(i);
        ostringstream defaultFileName;
        defaultFileName.clear();
        if(defaultFileNamePath)
        {
          defaultFileName << td->filenamePath << "/" << defaultFileNamePath;
        }
        else
        {
          break;
        }

        if(File::fileExists(defaultFileName.str().c_str()))
        { 
          ostringstream nURL;
          if(td->request.uriEndsWithSlash)
          {
            nURL << defaultFileNamePath;
          }
          else
          {
            int last_slash_offset = URI.length();
            while(last_slash_offset && URI[last_slash_offset]!='/')
              --last_slash_offset;
            
            nURL  <<  &(URI.c_str()[last_slash_offset ? last_slash_offset+1 : 0]) 
                  << "/" << defaultFileNamePath;
          }
          /*! Send a redirect to the new location.  */
          if(sendHTTPRedirect(td, s, nURL.str().c_str()))
            ret = 1;
          else
            ret = 0;
          return ret;
        }
      }
      return lhttp_dir.send(td, s, td->filenamePath.c_str(), 0, onlyHeader);
    }
    
    if(!File::fileExists(td->filenamePath.c_str()))
      return raiseHTTPError(td, s, e_404);

    /*!
     *getMIME returns the type of command registered by the extension.
     */
    data.assign("");
    {
      td->response.CONTENT_TYPE[0]='\0';
      td->mime=getMIME(td, td->filenamePath);
      /*! Set the default content type, this can be changed later. */
      if(td->mime)
      {
        td->response.CONTENT_TYPE.assign(td->mime->mime_type);
        mimeCMD = td->mime->command;
        data.assign(td->mime->cgi_manager);
      }
    }

    if(mimeCMD==CGI_CMD_RUNCGI)
    {
      if(!(permissions & MYSERVER_PERMISSION_EXECUTE))
      {
        return sendAuth(td, s);
      }
      ret = lcgi.send(td, s, td->filenamePath.c_str(), data.c_str(), 0,  onlyHeader);
      return ret;
    }
    else if(mimeCMD==CGI_CMD_EXECUTE )
    {
      if(!(permissions & MYSERVER_PERMISSION_EXECUTE))
		  {
        return sendAuth(td, s);
      }
      ret = lcgi.send(td, s, td->filenamePath.c_str(), data.c_str(), 1, onlyHeader);
      return ret;
    }
    else if(mimeCMD == CGI_CMD_RUNISAPI)
    {
      if(!(permissions & MYSERVER_PERMISSION_EXECUTE))
      {
        return sendAuth(td, s);
      }
      ret = lisapi.send(td, s, td->filenamePath.c_str(), data.c_str(), 0, 
                        onlyHeader);
      return ret;

    }
    else if(mimeCMD==CGI_CMD_EXECUTEISAPI)
    {
      if(!(permissions & MYSERVER_PERMISSION_EXECUTE))
      {
        return sendAuth(td, s);
      }
      ret = lisapi.send(td, s, td->filenamePath.c_str(), data.c_str(), 1, 
                        onlyHeader);
      return ret;
    }
    else if( mimeCMD == CGI_CMD_RUNMSCGI )
    {
      char* target;
      if((!allowMscgi) || (!(permissions & MYSERVER_PERMISSION_EXECUTE)))
      {
        return sendAuth(td, s);
      }
      if(td->request.URIOPTSPTR)
        target=td->request.URIOPTSPTR;
      else
        target=(char*)td->request.URIOPTS.c_str();
      /*! Check if the MSCGI library is loaded. */
      if(mscgiLoaded)
      {
        ret=lmscgi.send(td, s, td->filenamePath.c_str(), target, 1, onlyHeader);
        return ret;
      }
      return raiseHTTPError(td, s, e_500);
    }
    else if( mimeCMD == CGI_CMD_EXECUTEWINCGI )
    {
      ostringstream cgipath;
      if(!(permissions & MYSERVER_PERMISSION_EXECUTE))
      {
        return sendAuth(td, s);
      }
      if(data.length())
      {
        cgipath <<  data << " \""<< td->filenamePath <<  "\"";
      }
      else
      {
        cgipath << td->filenamePath;
      }
      ret=lwincgi.send(td, s, cgipath.str().c_str(), 1, onlyHeader);
      return ret;
    }
    else if( mimeCMD == CGI_CMD_RUNFASTCGI )
    {
      if(!(permissions & MYSERVER_PERMISSION_EXECUTE))
      {
        return sendAuth(td, s);
      }	
      ret = lfastcgi.send(td, s, td->filenamePath.c_str(), data.c_str(), 0, 
                          onlyHeader);
      return ret;
    }
    else if(mimeCMD==CGI_CMD_EXECUTEFASTCGI)
    {
      if(!(permissions & MYSERVER_PERMISSION_EXECUTE))
      {
        return sendAuth(td, s);
      }
      ret = lfastcgi.send(td, s, td->filenamePath.c_str(), data.c_str(), 1, 
                          onlyHeader);
      return ret;
    }
    else if( mimeCMD == CGI_CMD_SENDLINK )
    {
      u_long nbr;
      char* linkpath;
      char* pathInfo;
      int linkpathSize;
      File h;
      if(!(permissions & MYSERVER_PERMISSION_READ))
      {
        return sendAuth(td, s);
      }

      if(h.openFile(td->filenamePath.c_str(), 
                    FILE_OPEN_IFEXISTS|FILE_OPEN_READ))
      {
        return raiseHTTPError(td, s, e_500);/*!Internal server error*/
      }
      linkpathSize = h.getFileSize()+td->pathInfo.length()+1;
      if(linkpathSize > MYSERVER_KB(10))
        linkpathSize = MYSERVER_KB(10);
      linkpath=new char[linkpathSize];
      if(linkpath==0)
      {
        return sendHTTPhardError500(td, s);
      }
      if(h.readFromFile(linkpath, linkpathSize, &nbr))
      {
        h.closeFile();
        delete [] linkpath;
        return raiseHTTPError(td, s, e_500);/*!Internal server error*/
      }
      h.closeFile();
      linkpath[nbr]='\0';
      pathInfo=new char[td->pathInfo.length()+1];
      if(pathInfo == 0)
      {
        delete [] linkpath;
        return raiseHTTPError(td, s, e_500);/*!Internal server error*/
      }
      strcpy(pathInfo, td->pathInfo.c_str());
      translateEscapeString(pathInfo);
      strcat(linkpath, pathInfo);
      if(nbr)
      {
        string uri;
        uri.assign(linkpath);
        ret = sendHTTPResource(td, s, uri, systemrequest, onlyHeader, 1);
      }
      else
        ret = raiseHTTPError(td, s, e_404);
      delete [] linkpath;
      delete [] pathInfo;
      return ret;
    }

    /*! By default try to send the file as it is. */
    if(!(permissions & MYSERVER_PERMISSION_READ))
    {     
      return sendAuth(td, s);
    }

    lastMT=File::getLastModTime(td->filenamePath.c_str());
    if(lastMT==-1)
    {
      return raiseHTTPError(td, s, e_500);
    }
    getRFC822GMTTime(lastMT, tmpTime, HTTP_RESPONSE_LAST_MODIFIED_DIM);
    td->response.LAST_MODIFIED.assign(tmpTime);
    if(td->request.IF_MODIFIED_SINCE.length())
    {
      if(!td->request.IF_MODIFIED_SINCE.compare(td->response.LAST_MODIFIED.c_str()))
      {
        return sendHTTPNonModified(td, s);
      }
    }
    ret = lhttp_file.send(td, s, td->filenamePath.c_str(), 0, onlyHeader);
  }
  catch(...)
  {
    return raiseHTTPError(td, s, e_500); 
  };

	return ret;
}
/*!
 *Log the access using the Common Log Format or the Combined one.
 */
int Http::logHTTPaccess(HttpThreadContext* td, ConnectionPtr a)
{
	char tmpStrInt[12];
  string time;

  try
  {
    td->buffer2->SetLength(0);
    *td->buffer2 << a->getIpAddr();
    *td->buffer2<< " ";
	
    if(td->identity[0])
      *td->buffer2 << td->identity;
    else
      *td->buffer2 << "-";

    *td->buffer2<< " ";  

    if(td->identity[0])
      *td->buffer2 << td->identity;
    else
      *td->buffer2 << "-";
    
    *td->buffer2 << " [";
    
    getLocalLogFormatDate(time, HTTP_RESPONSE_DATE_DIM);
    *td->buffer2 <<  time  << "] \"";
    
    if(td->request.CMD.length())
      *td->buffer2 << td->request.CMD.c_str() << "";
  
    if(td->request.CMD.length() || td->request.URI.length())
      *td->buffer2 << " ";

    if(td->request.URI.length() == '\0')
      *td->buffer2 <<  "/";
    else
      *td->buffer2 << td->request.URI.c_str();
  

    if(td->request.URIOPTS.length())
      *td->buffer2 << "?" << td->request.URIOPTS.c_str();
    
    sprintf(tmpStrInt, "%u ",td->response.httpStatus);
  
    if(td->request.VER.length())
      *td->buffer2 << " " << td->request.VER.c_str()  ;
    
    *td->buffer2<< "\" " << tmpStrInt  << " ";
	
    if(td->response.CONTENT_LENGTH.length())
      *td->buffer2  << td->response.CONTENT_LENGTH.c_str();
    else
      *td->buffer2 << "0";
    if(strstr((((Vhost*)(a->host)))->getAccessLogOpt(), "type=combined"))
      *td->buffer2 << " "  << td->request.REFERER.c_str() << " "  
                   << td->request.USER_AGENT.c_str();
#ifdef WIN32
    *td->buffer2  << "\r\n" << end_str;
#else
    *td->buffer2  << "\n" << end_str;
#endif
    /*!
     *Request the access to the log file then write then append the message.
     */
    ((Vhost*)(a->host))->accesseslogRequestAccess(td->id);
    ((Vhost*)(a->host))->accessesLogWrite(td->buffer2->GetBuffer());
    ((Vhost*)(a->host))->accesseslogTerminateAccess(td->id);
    td->buffer2->SetLength(0);
  }
  catch(...)
  {
    return 1;
  };
	return 0;
}

/*!
 *This is the HTTP protocol main procedure to parse a request 
 *over the HTTP.
 */
int Http::controlConnection(ConnectionPtr a, char* /*b1*/, char* /*b2*/, 
                            int bs1, int bs2, u_long nbtr, u_long id)
{
 	int retvalue=-1;
  int ret = 0;
	int validRequest;
  u_long dataRead=0;
  u_long dataToRead=0;
  /*! Dimension of the POST data. */
	int content_len=-1;
  try
  {
    td.buffer=((ClientsThread*)a->thread)->GetBuffer();
    td.buffer2=((ClientsThread*)a->thread)->GetBuffer2();
    td.buffersize=bs1;
    td.buffersize2=bs2;
    td.nBytesToRead=nbtr;
    td.identity[0]='\0';
    td.connection=a;
    td.id=id;
    td.lastError = 0;
    td.lhttp=this;
    td.appendOutputs=0;
    td.onlyHeader = 0;
    td.inputData.setHandle((FileHandle)0);
    td.outputData.setHandle((FileHandle)0);
    td.filenamePath.assign("");
    td.outputDataPath.assign("");
    td.inputDataPath.assign("");
    td.mime=0;
    /*!
     *Reset the request and response structures.
     */
    HttpHeaders::resetHTTPRequest(&td.request);
    HttpHeaders::resetHTTPResponse(&td.response);
    
    /*! Reset the HTTP status once per request. */
    td.response.httpStatus=200;
    
    /*!
     *If the connection must be removed, remove it.
     */
    if(td.connection->getToRemove())
    {
      switch(td.connection->getToRemove())
      {
        /*! Remove the connection from the list. */
      case CONNECTION_REMOVE_OVERLOAD:
        retvalue = raiseHTTPError(&td, a, e_503);
				logHTTPaccess(&td, a);
				return 0;
      default:
        return 0;
      }
    }
    validRequest=HttpHeaders::buildHTTPRequestHeaderStruct(&td.request, &td);
  
    /*! If the header is incomplete returns 2. */
    if(validRequest==-1)/*!If the header is incomplete returns 2*/
    {
      /*! Be sure that the client can handle the 100 status code. */
      if(td.request.VER.compare("HTTP/1.0"))
      {
        char* msg = "HTTP/1.1 100 Continue\r\n\r\n";
        Thread::wait(2);
        if( a->socket.bytesToRead() == 0) 
        {
          if(a->socket.send(msg, (int)strlen(msg), 0)==-1)
            return 0;/*! Remove the connection from the list. */
        }
      }
      return 2;
    }
	
    if(a->protocolBuffer)
      ((HttpUserData*)a->protocolBuffer)->digestChecked=0;	
	
    /*!
     *If the validRequest cointains an error code send it to the user.
     */
    if(validRequest!=e_200)
    {
      retvalue = raiseHTTPError(&td, a, validRequest);
      logHTTPaccess(&td, a);
      return 0;
    }
    /*! Be sure that we can handle the HTTP version. */
    if((td.request.VER.compare("HTTP/1.1")) && 
       (td.request.VER.compare("HTTP/1.0")) && 
       (td.request.VER.compare("HTTP/0.9")))
    {	
      raiseHTTPError(&td, a, e_505);
      logHTTPaccess(&td, a);
      /*! Remove the connection from the list. */
      return 0;
    }
    /*! Do not use Keep-Alive over HTTP version older than 1.1. */
    if(td.request.VER.compare("HTTP/1.1") )
    {
      if(td.request.CONNECTION.length())
        td.request.CONNECTION.assign("close");
    }

    /*!
     *For methods that accept data after the HTTP header set the correct 
     *pointer and create a file containing the informations after the header.
     */
    {
      ostringstream streamIn;
      ostringstream streamOut;
      streamIn << getdefaultwd(0, 0) << "/stdInFile_" <<  (u_int)td.id;
      td.inputDataPath.assign(streamIn.str());
      
      streamOut << getdefaultwd(0, 0) << "/stdOutFile_" <<  (u_int)td.id;
      td.outputDataPath.assign(streamOut.str());
    }

    if((!td.request.CMD.compare("POST"))||(!td.request.CMD.compare("PUT")))
    {
      if(td.request.CONTENT_TYPE.length() == 0)
        td.request.CONTENT_TYPE.assign("application/x-www-form-urlencoded");
      
      /*!
       *Read POST data.
       */
      {
        u_long nbw=0;
        u_long total_nbr=0;
        u_long timeout;
        td.request.URIOPTSPTR=&(td.buffer->GetBuffer())[td.nHeaderChars];
        td.buffer->GetBuffer()[td.nBytesToRead<td.buffer->GetRealLength()-1 
                      ? td.nBytesToRead : td.buffer->GetRealLength()-1]='\0';
        /*!
         *Create the file that contains the data posted.
         *This data is the stdin file in the CGI.
         */
        if(td.inputData.openFile(td.inputDataPath,FILE_CREATE_ALWAYS | 
                                 FILE_OPEN_READ|FILE_OPEN_WRITE))
          return 0;
        nbw=0;
        total_nbr=(td.nBytesToRead<td.buffer->GetRealLength()-1 
           ? td.nBytesToRead : td.buffer->GetRealLength()-1 ) -td.nHeaderChars;
        if(total_nbr)
        {
          if(td.inputData.writeToFile(td.request.URIOPTSPTR, total_nbr, &nbw))
          {
            td.inputDataPath.assign("");
            td.outputDataPath.assign("");
            td.inputData.closeFile();
            return 0;
          }
        }
        if(td.request.CONTENT_LENGTH.length())
          content_len=atoi(td.request.CONTENT_LENGTH.c_str());
        
        /*!
         *If the connection is Keep-Alive be sure that the client specify the
         *HTTP CONTENT-LENGTH field.
         *If a CONTENT-ENCODING is specified the CONTENT-LENGTH is not 
         *always needed.
         */
        if(!stringcmpi(td.request.CONNECTION, "Keep-Alive"))
        {
          if((td.request.CONTENT_ENCODING.length()=='\0') 
             && (td.request.CONTENT_LENGTH.length() == 0))
          {
            /*!
             *If the inputData file was not closed close it.
             */
            if(td.inputData.getHandle())
            {
              td.inputData.closeFile();
              File::deleteFile(td.inputDataPath);
            }
            /*!
             *If the outputData file was not closed close it.
             */
            if(td.outputData.getHandle())
            {
              td.outputData.closeFile();
              File::deleteFile(td.outputDataPath);
            }
            retvalue = raiseHTTPError(&td, a, e_400);
            logHTTPaccess(&td, a);
            return retvalue ? 1 : 0;
          }
        }
        /*!
         *If there are others bytes to read from the socket.
         */
        timeout=get_ticks();
        if((content_len!=-1) && 
           ((content_len!=static_cast<int>(nbw) ) || (nbw==0)))
        {
          int ret;
          u_long fs;
          do
          {
            ret=0;
            fs=td.inputData.getFileSize();
            while(get_ticks()-timeout<MYSERVER_SEC(5))
					  {
              if(content_len==static_cast<int>(total_nbr))
						  {
                /*!
                 *Consider only CONTENT-LENGTH bytes of data.
                 */
                while(td.connection->socket.bytesToRead())
                {
                  /*!
                   *Read the unwanted bytes but do not save them.
                   */
                  ret=td.connection->socket.recv(td.buffer2->GetBuffer(), 
                                                 td.buffer2->GetRealLength(), 0);
                }
                break;
              }
              if((content_len>static_cast<int>(fs)) && 
                 (td.connection->socket.bytesToRead()))
              {				
                u_long tr=static_cast<u_long>(content_len)-total_nbr < 
                  td.buffer2->GetRealLength() 
                  ? static_cast<u_long>(content_len)-total_nbr 
                  : td.buffer2->GetRealLength() ;

                ret=td.connection->socket.recv(td.buffer2->GetBuffer(), tr, 0);
                if(ret==-1)
                {
                  td.inputData.closeFile();
                  td.inputData.deleteFile(td.inputDataPath);
                  return 0;
                }
							
                if(td.inputData.writeToFile(td.buffer2->GetBuffer(), 
                                            static_cast<u_long>(ret), &nbw))
                {
                  td.inputData.closeFile();
                  td.inputData.deleteFile(td.inputDataPath);
                  return 0;
                }
                total_nbr+=nbw;
                timeout=get_ticks();
                break;
              }
            }
            if(get_ticks()-timeout>=MYSERVER_SEC(5))
              break;
          }
          while(content_len!=static_cast<int>(total_nbr));
          
          fs=static_cast<u_long>(td.inputData.getFileSize());
          if(content_len!=static_cast<int>(fs))
          {
            /*!
             *If we get an error remove the file and the connection.
             */
            td.inputData.closeFile();
            td.inputData.deleteFile(td.inputDataPath);
            td.outputData.closeFile();
            td.outputData.deleteFile(td.outputDataPath);
            retvalue = raiseHTTPError(&td, a, e_500);
            logHTTPaccess(&td, a);
            return retvalue ? 1 : 0;
          }
        }
        /*! If CONTENT-LENGTH is not specified read all the data. */
        else if(content_len==0)
        {
          ostringstream buff;
          int ret;
          do
          {
            ret=0;
            while(get_ticks()-timeout<MYSERVER_SEC(3))
            {
              if(td.connection->socket.bytesToRead())
              {
                ret=td.connection->socket.recv(td.buffer2->GetBuffer(), 
                                               td.buffer2->GetRealLength(), 0);

                if(td.inputData.writeToFile(td.buffer2->GetBuffer(), ret, &nbw))
                {
                  td.inputData.deleteFile(td.inputDataPath);
                  td.inputData.closeFile();
                  return 0;
                }
                total_nbr+=nbw;
                timeout=get_ticks();
                break;
              }
            }
            if(get_ticks()-timeout>=MYSERVER_SEC(3))
              break;
            /*! Wait a bit. */
            Thread::wait(2);
          }
          while(content_len!=static_cast<int>(total_nbr));
          buff << td.inputData.getFileSize();
          /*! Store a new value for CONTENT_LENGTH. */
          td.response.CONTENT_LENGTH.assign(buff.str());
        }
        td.inputData.setFilePointer(0);
      }/* End read POST data */
    }
    /*! Methods with no POST data. */
    else
    {
      td.request.URIOPTSPTR=0;
    }
    /*
     *Manage chunked transfers.
     *Data loaded before don't take care of the TRANSFER ENCODING. 
     *Here we clean data, making it usable.
     */
    if(!td.request.TRANSFER_ENCODING.compare("chunked"))
    {
      File newStdIn;
      char buffer[20];
      char c;
      u_long nbr;
      u_long bufferlen;
      ostringstream newfilename;
      
      newfilename <<  td.inputData.getFilename() <<  "_encoded";
      if(newStdIn.openFile(td.inputDataPath, FILE_CREATE_ALWAYS | 
                           FILE_OPEN_READ|FILE_OPEN_WRITE))
      {
        td.inputData.closeFile();
        td.inputData.deleteFile(td.inputDataPath);
        return 0;
      }
      for(;;)
      {
        bufferlen=0;
        buffer[0]='\0';
        for(;;)
        {
          if(td.inputData.readFromFile(&c, 1, &nbr))
          {
            td.inputData.closeFile();
            td.inputData.deleteFile(td.inputDataPath);
            newStdIn.closeFile();
            newStdIn.deleteFile(newfilename.str().c_str());
            return 0;
          }
          if(nbr!=1)
          {
            break;
          }
          if((c!='\r') && (bufferlen<19))
          {
            buffer[bufferlen++]=c;
            buffer[bufferlen]='\0';
          }
          else
          {
            break;
          }
        }
        /*!Read the \n char too. */
        if(td.inputData.readFromFile(&c, 1, &nbr))
        {
          td.inputData.closeFile();
          td.inputData.deleteFile(td.inputDataPath);
          newStdIn.closeFile();
          newStdIn.deleteFile(newfilename.str().c_str());
          return 0;
        }
        dataToRead=(u_long)hexToInt(buffer);

        /*! The last chunk length is 0. */
        if(dataToRead==0)
          break;

        while(dataRead<dataToRead)
        {
          u_long nbw;
          if(td.inputData.readFromFile(td.buffer->GetBuffer(), 
                         dataToRead-dataRead < td.buffer->GetRealLength()-1 
                        ? dataToRead-dataRead: td.buffer->GetRealLength()-1 , &nbr))
          {
            td.inputData.closeFile();
            td.inputData.deleteFile(td.inputDataPath);
            newStdIn.closeFile();
            newStdIn.deleteFile(newfilename.str().c_str());
            return 0;
          }
          if(nbr==0)
            break;
          dataRead+=nbr;
          if(newStdIn.writeToFile(td.buffer->GetBuffer(), nbr, &nbw))
          {
            td.inputData.closeFile();
            td.inputData.deleteFile(td.inputDataPath);
            newStdIn.closeFile();
            newStdIn.deleteFile(newfilename.str().c_str());
            return 0;
          }
          if(nbw!=nbr)
            break;
        }
        
      }
      /*!
       *Now replace the file with the not-chunked one.
       */
      td.inputData.closeFile();
      td.inputData.deleteFile(td.inputDataPath.c_str());
      td.inputDataPath.assign(newfilename.str().c_str());
      td.inputData=newStdIn;
      td.inputData.setFilePointer(0);
    }
    /*!
     *If is specified another Transfer Encoding not supported by the 
     *server send a 501 error.
     */
    else if(td.request.TRANSFER_ENCODING.length())
    {
      raiseHTTPError(&td, a, e_501);
      /*!
       *If the inputData file was not closed close it.
       */
      if(td.inputData.getHandle())
      {
        td.inputData.closeFile();
        File::deleteFile(td.inputDataPath);
      }
      /*!
       *If the outputData file was not closed close it.
       */
      if(td.outputData.getHandle())
      {
        td.outputData.closeFile();
        File::deleteFile(td.outputDataPath);
      }
      logHTTPaccess(&td, a);
      return 0;
    }

    /*! If return value is not configured propertly. */
    if(retvalue==-1)
    {
      /*!
       *How is expressly said in the rfc2616 a client that sends an 
       *HTTP/1.1 request MUST sends a Host header.
       *Servers MUST reports a 400 (Bad request) error if an HTTP/1.1
       *request does not include a Host request-header.
       */
      if((!td.request.VER.compare("HTTP/1.1")) && td.request.HOST.length()==0)
      {
        raiseHTTPError(&td, a, e_400);
        /*!
         *If the inputData file was not closed close it.
         */
        if(td.inputData.getHandle())
        {
          td.inputData.closeFile();
          File::deleteFile(td.inputDataPath);
        }
        /*!
         *If the outputData file was not closed close it.
         */
        if(td.outputData.getHandle())
        {
          td.outputData.closeFile();
          File::deleteFile(td.outputDataPath);
        }
        logHTTPaccess(&td, a);
        return 0;
      }
      else
      {
        /*!
         *Find the virtual host to check both host name and IP value.
         */
        Vhost* newHost=lserver->vhostList->getVHost((char*)td.request.HOST.c_str(), 
                                            a->getLocalIpAddr(), a->getLocalPort());
        if(a->host)
          ((Vhost*)a->host)->removeRef();
        a->host=newHost;
        if(a->host==0)
        {
          raiseHTTPError(&td, a, e_400);
          /*!
           *If the inputData file was not closed close it.
           */
          if(td.inputData.getHandle())
          {
            td.inputData.closeFile();
            File::deleteFile(td.inputDataPath);
          }
          /*!
           *If the outputData file was not closed close it.
           */
          if(td.outputData.getHandle())
          {
            td.outputData.closeFile();
            File::deleteFile(td.outputDataPath);
          }	
          logHTTPaccess(&td, a);
          return 0;
        }
      }
		
      if(!stringcmpi(td.request.CONNECTION, "Keep-Alive")) 
      {
        /*!
         *Support for HTTP pipelining.
         */
        if(content_len==0)
        {
          /*!
           *connectionBuffer is 8 KB, so don't copy more bytes.
           */
          a->setDataRead(MYSERVER_KB(8) < (int)strlen(td.buffer->GetBuffer()) -
                         td.nHeaderChars ? MYSERVER_KB(8) : 
                         (int)strlen(td.buffer->GetBuffer()) - td.nHeaderChars );
          if(a->getDataRead() )
          {
            memcpy(a->connectionBuffer, (td.buffer->GetBuffer() +
                                         td.nHeaderChars), a->getDataRead() );
            retvalue=3;
          }
          else
            retvalue=1;
          
        }	
        else
          retvalue=1;
      }
      else
      {
        retvalue=0;
      }
      
      /*! 
       *Set the throttling rate for the socket. This setting can be 
       *overwritten later. 
       */
      if(((Vhost*)a->host)->getThrottlingRate() == (u_long) -1)
        a->socket.setThrottling(lserver->getThrottlingRate());
      else
        a->socket.setThrottling(((Vhost*)a->host)->getThrottlingRate());
      
      /*!
       *Here we control all the HTTP commands.
       */
      /*! GET REQUEST. */
      if(!td.request.CMD.compare("GET"))
      {
        if(!td.request.RANGETYPE.compare("bytes"))
          ret = sendHTTPResource(&td, a, td.request.URI, 0, 0);
        else
          ret = sendHTTPResource(&td, a, td.request.URI);
      }
      /*! POST REQUEST. */
      else if(!td.request.CMD.compare("POST"))
      {
        if(!td.request.RANGETYPE.compare("bytes"))
          ret = sendHTTPResource(&td, a, td.request.URI, 0, 0);
        else
          ret = sendHTTPResource(&td, a, td.request.URI);
      }
      /*! HEAD REQUEST. */
      else if(!td.request.CMD.compare("HEAD"))
      {
        td.onlyHeader = 1;
        if(!td.request.RANGETYPE.compare("bytes"))
          ret = sendHTTPResource(&td, a,  td.request.URI, 0, 1);
        else
          ret = sendHTTPResource(&td, a,  td.request.URI, 0, 1);
      }
      /*! DELETE REQUEST. */
      else if(!td.request.CMD.compare("DELETE"))
      {
        ret = deleteHTTPRESOURCE(&td, a,  td.request.URI, 0);
      }
      /*! PUT REQUEST. */
      else if(!td.request.CMD.compare("PUT"))
      {
        if(!td.request.RANGETYPE.compare("bytes"))
          ret = putHTTPRESOURCE(&td, a, td.request.URI, 0, 1);
        else
          ret = putHTTPRESOURCE(&td, a, td.request.URI, 0, 1);
      }
      /*! OPTIONS REQUEST. */
      else if(!td.request.CMD.compare("OPTIONS"))
      {
        ret = optionsHTTPRESOURCE(&td, a, td.request.URI, 0);
      }
      /*! TRACE REQUEST. */
      else if(!td.request.CMD.compare("TRACE"))
      {
        ret = traceHTTPRESOURCE(&td, a, td.request.URI, 0);
      }
      /*! Return Method not implemented(501). */
      else
      {
        raiseHTTPError(&td, a, e_501);
        retvalue=0;
      }
      logHTTPaccess(&td, a);
    }

    /*!
     *If the inputData file was not closed close it.
     */
    if(td.inputData.getHandle())
    {
      td.inputData.closeFile();
      File::deleteFile(td.inputDataPath);
    }
    /*!
     *If the outputData file was not closed close it.
     */
    if(td.outputData.getHandle())
    {
      td.outputData.closeFile();
      File::deleteFile(td.outputDataPath);
    }
    ret &= !stringcmpi(td.request.CONNECTION, "Keep-Alive");
    return (ret && retvalue) ? retvalue : 0;
  }
  catch(...)
  {
    logHTTPaccess(&td, a);
    return 0;
  }
}

/*!
 *Compute the Digest outputting it to a buffer.
 */
void Http::computeDigest(HttpThreadContext* td, char* out , char* buffer)
{
	Md5 md5;
	if(!out)
		return;
	sprintf(buffer, "%i-%u-%s", (int)clock(), (u_int)td->id, 
          td->connection->getIpAddr());
	md5.init();
	md5.update((unsigned char const*)buffer , (unsigned int)strlen(buffer));
	md5.end(out);
}

/*!
 *Sends an error page to the client.
 *Nonzero to keep the connection.
 */
int Http::raiseHTTPError(HttpThreadContext* td, ConnectionPtr a, int ID)
{
  try
  {
    string time;
    ostringstream errorFile;
    Md5 md5;
    
    if(td->lastError)
      return sendHTTPhardError500(td, a);

    td->lastError = ID;

    HttpHeaders::buildDefaultHTTPResponseHeader(&(td->response));
    if(!stringcmpi(td->request.CONNECTION, "Keep-Alive"))
    {
      td->response.CONNECTION.assign("Keep-Alive");
    }
    if(ID==e_401AUTH)
    {
      td->response.httpStatus = 401;
      td->buffer2->SetLength(0);
      *td->buffer2 << 
           "HTTP/1.1 401 Unauthorized\r\nAccept-Ranges: bytes\r\nServer: MyServer " ;
      *td->buffer2 << versionOfSoftware ;
      *td->buffer2 << "\r\nContent-type: text/html\r\nConnection: ";
      *td->buffer2 <<td->request.CONNECTION.c_str();
      *td->buffer2 << "\r\nContent-length: 0\r\n";
      if(td->auth_scheme==HTTP_AUTH_SCHEME_BASIC)
      {
        *td->buffer2 <<  "WWW-Authenticate: Basic realm=\"" 
                                         << td->request.HOST.c_str() <<  "\"\r\n";
      }
      else if(td->auth_scheme==HTTP_AUTH_SCHEME_DIGEST)
      {
        char md5_str[256];
        if(a->protocolBuffer==0)
        {
          a->protocolBuffer = new HttpUserData;
          if(!a->protocolBuffer)
          {
            sendHTTPhardError500(td, a);
            return 0;
          }
          ((HttpUserData*)(a->protocolBuffer))->reset();
        }
        myserver_strlcpy(((HttpUserData*)a->protocolBuffer)->realm, 
                         td->request.HOST.c_str(), 48);

        /*! Just a random string. */
        md5_str[0]=(char)td->id;
        md5_str[1]=(char)((clock() >> 24) & 0xFF);
        md5_str[2]=(char)((clock() >> 16) & 0xFF);
        md5_str[3]=(char)((clock() >>  8)   & 0xFF);
        md5_str[4]=(char) (clock() & 0xFF);
        strncpy(&(md5_str[5]), td->request.URI.c_str(), 256-5);
        md5.init();
        md5.update((unsigned char const*)md5_str,  (unsigned int)strlen(md5_str));
        md5.end(((HttpUserData*)a->protocolBuffer)->opaque);
        
        if(a->protocolBuffer && (!(((HttpUserData*)a->protocolBuffer)->digest)) || 
           (((HttpUserData*)a->protocolBuffer)->nonce[0]=='\0'))
        {
          computeDigest(td, ((HttpUserData*)a->protocolBuffer)->nonce, md5_str);
          ((HttpUserData*)a->protocolBuffer)->nc=0;
        }
        *td->buffer2 << "WWW-Authenticate: digest ";
        *td->buffer2 << " qop=\"auth\", algorithm =\"MD5\", realm =\"";
        *td->buffer2 << ((HttpUserData*)a->protocolBuffer)->realm ;
        *td->buffer2 << "\",  opaque =\"" 
                     << ((HttpUserData*)a->protocolBuffer)->opaque;

        *td->buffer2<< "\",  nonce =\""<< ((HttpUserData*)a->protocolBuffer)->nonce;
        *td->buffer2 <<"\" ";
        if(((HttpUserData*)a->protocolBuffer)->cnonce[0])
        {
          *td->buffer2 << ", cnonce =\"";
          *td->buffer2 <<((HttpUserData*)a->protocolBuffer)->cnonce;
          *td->buffer2 <<"\" ";
        }
        *td->buffer2 << "\r\n";
      }		
      else
      {
        /*!
         *Just send a non implemented error page.
         */
        return raiseHTTPError(td, a, 501);
      }				
      *td->buffer2 << "Date: ";
      getRFC822GMTTime(time, HTTP_RESPONSE_DATE_DIM);
      *td->buffer2  << time;
      *td->buffer2 << "\r\n\r\n";
      if(a->socket.send(td->buffer2->GetBuffer(), 
                        td->buffer2->GetLength(), 0)==-1)
      {
        return 0;
      }
      return 1;
    }
    else
    {
      string defFile;
      int ret;
      td->response.httpStatus=getHTTPStatusCodeFromErrorID(ID);
      secCacheMutex.lock();
      /*! 
       *The specified error file name must be in the web directory 
       *of the virtual host. 
       */
      ret = secCache.getErrorFileName(((Vhost*)a->host)->getDocumentRoot(), 
                                       getHTTPStatusCodeFromErrorID(ID),
                                       ((Vhost*)(a->host))->getSystemRoot(), defFile);
      secCacheMutex.unlock();
      if(ret == -1)
      {
        sendHTTPhardError500(td, a);
        return 0;
      }
      else if(ret)
      {
        /*!
         *Change the URI to reflect the default file name.
         */
        ostringstream nURL;
        int isPortSpecified=0;
        nURL << protocolPrefix << td->request.HOST.c_str() ;
        for(int i=0;td->request.HOST[i];i++)
        {
          if(td->request.HOST[i]==':')
          {
            isPortSpecified = 1;
            break;
          }
        }
        if(!isPortSpecified)
          nURL << ":" << ((Vhost*)a->host)->getPort();
        if(nURL.str()[nURL.str().length()-1]!='/')
          nURL << "/";

        nURL << defFile;
        
        return sendHTTPRedirect(td, a, nURL.str().c_str());
      }
    }
    getRFC822GMTTime(time, HTTP_RESPONSE_DATEEXP_DIM);
    td->response.DATEEXP.assign(time);
    td->response.ERROR_TYPE.assign(HTTP_ERROR_MSGS[ID], 
                                   HTTP_RESPONSE_ERROR_TYPE_DIM);
    
    errorFile << ((Vhost*)(a->host))->getSystemRoot() << "/" << HTTP_ERROR_HTMLS[ID];
    if(useMessagesFiles && File::fileExists(errorFile.str().c_str()))
    {
        string tmp;
        tmp.assign(HTTP_ERROR_HTMLS[ID]);
        return sendHTTPResource(td, a, tmp, 1, td->onlyHeader);
    }

  
    /*! Send the error over the HTTP. */
    td->response.CONTENT_LENGTH.assign("0");

    HttpHeaders::buildHTTPResponseHeader(td->buffer->GetBuffer(), &td->response);
    if(a->socket.send(td->buffer->GetBuffer(), 
                      (u_long)strlen(td->buffer->GetBuffer()), 0)==-1)
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
int Http::sendHTTPhardError500(HttpThreadContext* td, ConnectionPtr a)
{
  CMemBuf tmp;
	char tmpStr[12];
	string time;
	const char hardHTML[] = "<!-- Hard Coded 500 Response --><body bgcolor=\"#000000\">"
          "<p align=\"center\">"
     "<font size=\"5\" color=\"#00C800\">Error 500</font></p><p align=\"center\">"    
     "<font size=\"5\" color=\"#00C800\">Internal Server error</font></p>\r\n";
	td->response.httpStatus=500;
	td->buffer->SetLength(0);
	*td->buffer <<  HTTP_ERROR_MSGS[e_500];
	*td->buffer << " from: " ;
	*td->buffer << a->getIpAddr() ;
	*td->buffer << "\r\n";	
	td->buffer2->SetLength(0);
	*td->buffer2 << "HTTP/1.1 500 System Error\r\nServer: MyServer ";
	*td->buffer2 << versionOfSoftware;
	*td->buffer2 <<" \r\nContent-type: text/html\r\nContent-length: ";
  tmp.IntToStr((int)strlen(hardHTML), tmpStr, 12);
	*td->buffer2  <<   tmp;
	*td->buffer2   << "\r\n";
	*td->buffer2 <<"Date: ";
	getRFC822GMTTime(time, HTTP_RESPONSE_DATE_DIM);
	*td->buffer2 << time;
	*td->buffer2 << "\r\n\r\n";
	/*! Send the header. */
	if(a->socket.send(td->buffer2->GetBuffer(), 
                    (u_long)td->buffer2->GetLength(), 0)!= -1)
	{
		/*! Send the body. */
    if(!td->onlyHeader)
   		a->socket.send(hardHTML, (u_long)strlen(hardHTML), 0);
	}
	return 0;
}

/*!
 *Returns the MIME type passing its extension.
 *Returns zero if the file is registered.
 */
MimeManager::MimeRecord* Http::getMIME(HttpThreadContext* td, string &filename)
{
  string ext;
	File::getFileExt(ext, filename);
	
  if(allowVhostMime && ((Vhost*)(td->connection->host))->isMIME() )
  {
    return ((Vhost*)(td->connection->host))->getMIME()->getRecord(ext);
  }
	return lserver->mimeManager.getRecord(ext);
}

/*!
 *Map an URL to the machine file system. Return e_200 on success. 
 *Any other return value is the HTTP error.
 */
int Http::getPath(HttpThreadContext* td, ConnectionPtr /*s*/, 
               string& filenamePath, const char *filename, int systemrequest)
{
	/*!
   *If it is a system request, search the file in the system directory.
   */
	if(systemrequest)
	{
    if(!strlen(((Vhost*)(td->connection->host))->getSystemRoot()) 
       || File::getPathRecursionLevel(filename)< 2 )
    {
      return e_401;
    }
    filenamePath.assign(((Vhost*)(td->connection->host))->getSystemRoot());
    filenamePath.append(filename);
	}
	/*!
   *Else the file is in the web directory.
   */
	else
	{	
		if(filename[0])
		{
      const char *root;
      /*! 
       *URI starting with a /sys/ will use the system directory as
       *the root path. Be sure to don't allow access to the system root
       *but only to subdirectories.
       */
      if(filename[0] == '/' && filename[1] == 's' && filename[2] == 'y'
         && filename[3] == 's' && filename[4] == '/')
      {
        root=((Vhost*)(td->connection->host))->getSystemRoot();
        /*! 
         *Do not allow access to the system directory root but only
         *to subdirectories. 
         */
        if(File::getPathRecursionLevel(filename)< 2 )
        {
          return e_401;
        }
        filename = filename + 5;
      }
      else
      {
        root=((Vhost*)(td->connection->host))->getDocumentRoot();
      }
			filenamePath.assign(root);
      filenamePath.append(filename);
		}
		else
		{
      filenamePath.append(((Vhost*)(td->connection->host))->getDocumentRoot());
		}

	}
  return e_200;
}

/*!
 *Get the CSS file used in a browsed directory.
 */
const char* Http::getBrowseDirCSSFile()
{
  return browseDirCSSpath.c_str();
}

/*!
 *Get the GZIP threshold.
 */
u_long Http::getGzipThreshold()
{
  return gzipThreshold;
}

/*!
 *Send a redirect message to the client.
 */
int Http::sendHTTPRedirect(HttpThreadContext* td, ConnectionPtr a, 
                           const char *newURL)
{
	string time;
	td->response.httpStatus=302;
	td->buffer2->SetLength(0);
	*td->buffer2 << "HTTP/1.1 302 Moved\r\nAccept-Ranges: bytes\r\nServer: MyServer " ;
	*td->buffer2<< versionOfSoftware;
	*td->buffer2 << "\r\nContent-type: text/html\r\nLocation: ";
	*td->buffer2  << newURL ;
	*td->buffer2  << "\r\nContent-length: 0\r\n";

	if(!stringcmpi(td->request.CONNECTION, "Keep-Alive"))
	{
		*td->buffer2 << "Connection: Keep-Alive\r\n";	
	}
	*td->buffer2<< "Date: ";
	getRFC822GMTTime(time, HTTP_RESPONSE_DATE_DIM);
	*td->buffer2 << time ;
	*td->buffer2 << "\r\n\r\n";
	if(a->socket.send(td->buffer2->GetBuffer(), 
                    (int)td->buffer2->GetLength(), 0) == -1)
		return 0;

	return 1;
}

/*!
 *Send a non-modified message to the client.
 */
int Http::sendHTTPNonModified(HttpThreadContext* td, ConnectionPtr a)
{
	string time;
	td->response.httpStatus=304;
	td->buffer2->SetLength(0);
	*td->buffer2 << "HTTP/1.1 304 Not Modified\r\nAccept-Ranges: bytes\r\n"
                  "Server: MyServer " ;

	*td->buffer2 << versionOfSoftware  ;
	*td->buffer2 <<  "\r\n";

	if(!stringcmpi(td->request.CONNECTION, "Keep-Alive"))
	{
		*td->buffer2 << "Connection: Keep-Alive\r\n";	
	}	
	*td->buffer2 << "Date: ";
	
	getRFC822GMTTime(time, HTTP_RESPONSE_DATE_DIM);
	*td->buffer2 << time << "\r\n\r\n";

	if(a->socket.send(td->buffer2->GetBuffer(), 
                    (int)td->buffer2->GetLength(), 0) == -1)
		return 0;
	return 1;
}

/*!
 *Send a 401 error.
 */
int Http::sendAuth(HttpThreadContext* td, ConnectionPtr s)
{
	if(s->getnTries() > 2)
	{
		return raiseHTTPError(td, s, e_401);
	}
	else
	{	
		s->incnTries();
		return raiseHTTPError(td, s, e_401AUTH);
	}
}

/*!
 *Load the HTTP protocol.
 */
int Http::loadProtocol(XmlParser* languageParser)
{
  const char *main_configuration_file;
  char *data;
  int  nDefaultFilename = 0;
	XmlParser configurationFileManager;
  if(initialized)
		return 0;

  main_configuration_file = lserver->getMainConfFile();

  secCacheMutex.init();
		
	/*! 
   *Store defaults value.  
   *By default use GZIP with files bigger than a MB.  
   */
  cgiTimeout = MYSERVER_SEC(15);
  fastcgiServers = 25;
  fastcgiInitialPort = 3333;
	gzipThreshold=1<<20;
	useMessagesFiles=1;
	browseDirCSSpath.assign("");

	configurationFileManager.open(main_configuration_file);

	/*! Initialize ISAPI.  */
	Isapi::load(&configurationFileManager);
	
	/*! Initialize FastCGI.  */
	FastCgi::load(&configurationFileManager);	

	/*! Load the MSCGI library.  */
	mscgiLoaded = MsCgi::load(&configurationFileManager) ? 0 : 1;
	if(mscgiLoaded)
  {
		lserver->logWriteln( languageParser->getValue("MSG_LOADMSCGI") );
  }
	else
	{
		lserver->logPreparePrintError();
		lserver->logWriteln( languageParser->getValue("ERR_LOADMSCGI") );
		lserver->logEndPrintError();
	}
  HttpFile::load(&configurationFileManager);
	
  HttpDir::load(&configurationFileManager);
	
	/*! Determine the min file size that will use GZIP compression.  */
	data=configurationFileManager.getValue("GZIP_THRESHOLD");
	if(data)
	{
		gzipThreshold=atoi(data);
	}	
	data=configurationFileManager.getValue("ALLOW_VHOST_MIME");
	if(data)
	{

    if(!strcmpi(data, "YES"))
      allowVhostMime=1;
    else
      allowVhostMime=0;      
	}	
	data=configurationFileManager.getValue("ALLOW_MSCGI");
	if(data)
	{
    if(!strcmpi(data, "YES"))
      allowMscgi=1;
    else
      allowMscgi=0;      
	}	

	data=configurationFileManager.getValue("CGI_TIMEOUT");
	if(data)
	{
		cgiTimeout=MYSERVER_SEC(atoi(data));
	}	
	data=configurationFileManager.getValue("FASTCGI_MAX_SERVERS");
	if(data)
	{
    fastcgiServers=atoi(data);
	}	
	data=configurationFileManager.getValue("FASTCGI_INITIAL_PORT");
	if(data)
	{
    fastcgiInitialPort=atoi(data);
	}	
	data=configurationFileManager.getValue("USE_ERRORS_FILES");
	if(data)
	{
		if(!lstrcmpi(data, "YES"))
			useMessagesFiles=1;
		else
			useMessagesFiles=0;
	}
	data=configurationFileManager.getValue("BROWSEFOLDER_CSS");
	if(data)
	{
    browseDirCSSpath.append(data);
	}

  Cgi::setTimeout(cgiTimeout);
  FastCgi::setTimeout(cgiTimeout);
  WinCgi::setTimeout(cgiTimeout);
  Isapi::setTimeout(cgiTimeout);
  FastCgi::setMaxFcgiServers(fastcgiServers);
  FastCgi::setInitialPort(fastcgiInitialPort);
	/*! 
   *Determine the number of default filenames written in 
   *the configuration file.  
   */
	nDefaultFilename=0;

	for(;;)
	{
		ostringstream xmlMember;
		xmlMember << "DEFAULT_FILENAME" << nDefaultFilename;
		if(!configurationFileManager.getValue(xmlMember.str().c_str()))
			break;
		nDefaultFilename++;


	}
  
  for(int i=0; i<static_cast<int>(defaultFilename.size()); i++ )
  {
    string *str=defaultFilename[i];
    delete str;
  }
  defaultFilename.clear();
  
	/*!
   *Copy the right values in the buffer.
   */
	if(nDefaultFilename==0)
	{
    string *str=new string("default.html");
    defaultFilename.push_back (str);
	}
	else
	{
		u_long i;
		for(i=0; i<static_cast<u_long>(nDefaultFilename); i++)
		{
			ostringstream xmlMember;
			xmlMember << "DEFAULT_FILENAME"<< (u_int)i;
			data=configurationFileManager.getValue(xmlMember.str().c_str());
			if(data)
      {
        string* str = new string(data);
        defaultFilename.push_back(str);
      }
		}
	}	


	configurationFileManager.close();
	initialized=1;
	return 1;	
}

/*!
 *Unload the HTTP protocol.
 */
int Http::unloadProtocol(XmlParser* /*languageParser*/)
{
	 if(!initialized)
		 return 0;
	/*!
   *Clean ISAPI.
   */
	Isapi::unload();
	/*!
   *Clean FastCGI.
   */
	FastCgi::unload();
	/*!
   *Clean MSCGI.
   */
	MsCgi::unload();
 
  HttpFile::unload();
	
  HttpDir::unload();
  
  secCache.free();

  secCacheMutex.destroy();

  for(int i=0; i<static_cast<int>(defaultFilename.size()); i++ )
  {
    string *str=defaultFilename[i];
    delete str;
  }
  defaultFilename.clear();
  browseDirCSSpath.assign("");

	initialized=0;
	return 1;
}

/*!
 *Returns the default filename.
 */
const char *Http::getDefaultFilenamePath(u_long ID)
{
  if(defaultFilename.size() <= ID)
    return 0;
  return defaultFilename[ID]->c_str();
}

/*!
 *Returns the name of the protocol. If an out buffer is defined fullfill 
 *it with the name too.
 */
char* Http::registerName(char* out, int len)
{
	if(out)
	{
		myserver_strlcpy(out, "HTTP", len);
	}
	return "HTTP";
}

/*!
 *Constructor for the class http.
 */
Http::Http()
{
	protocolPrefix.assign("http://");
	protocolOptions=0;
	td.filenamePath.assign("");
	td.pathInfo.assign("");
	td.pathTranslated.assign("");
	td.cgiRoot.assign("");
	td.cgiFile.assign("");
	td.scriptPath.assign("");
	td.scriptDir.assign("");
	td.scriptFile.assign("");
	td.inputDataPath.assign("");
	td.outputDataPath.assign("");
}

/*!
 *Destructor for the class http.
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
  td.filenamePath.assign("");
  td.pathInfo.assign("");
  td.pathTranslated.assign("");
  td.cgiRoot.assign("");
  td.cgiFile.assign("");
  td.scriptPath.assign("");
  td.scriptDir.assign("");
	td.scriptFile.assign("");
  td.inputDataPath.assign("");
  td.outputDataPath.assign("");
}
