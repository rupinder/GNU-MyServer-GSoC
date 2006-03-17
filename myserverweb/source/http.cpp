/*
MyServer
Copyright (C) 2002, 2003, 2004, 2005 The MyServer Team
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "../include/http.h"
#include "../include/http_headers.h"
#include "../include/server.h"
#include "../include/security.h"
#include "../include/mime_utils.h"
#include "../include/cgi.h"
#include "../include/file.h"
#include "../include/clients_thread.h"
#include "../include/sockets.h"
#include "../include/wincgi.h"
#include "../include/fastcgi.h"
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

/*! Path to the .css file used by directory browsing.  */
string Http::browseDirCSSpath;

/*! Threshold value to send data in gzip.  */
u_long Http::gzipThreshold=0;

/*! Vector with default filenames.  */
vector<string> Http::defaultFilename;

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

/*! Dynamic commands manager. */
DynHttpCommandManager Http::dynCmdManager;

/*! Dynamic managers. */
DynHttpManagerList Http::dynManagerList;

/*!
 *Build a response for an OPTIONS request.
 */
int Http::optionsHTTPRESOURCE(string& /*filename*/, int /*yetmapped*/)
{
	int ret;
	string time;
  try
  {
		HttpRequestHeader::Entry *connection = td.request.other.get("Connection");
    getRFC822GMTTime(time, HTTP_RESPONSE_DATE_DIM);
    td.buffer2->setLength(0);
    *td.buffer2 <<  "HTTP/1.1 200 OK\r\n";
    *td.buffer2 << "Date: " << time ;
    *td.buffer2 <<  "\r\nServer: MyServer "  << versionOfSoftware ;
    if(connection && connection->value->length())
      *td.buffer2 << "\r\nConnection:" << connection->value->c_str() ;
    *td.buffer2 <<"\r\nContent-Length: 0\r\nAccept-Ranges: bytes\r\n";
    *td.buffer2 << "Allow: OPTIONS, GET, POST, HEAD, DELETE, PUT";
    
    /*!
     *Check if the TRACE command is allowed on the virtual host.
     */
    if(allowHTTPTRACE())
      *td.buffer2 << ", TRACE\r\n\r\n";
    else
      *td.buffer2 << "\r\n\r\n";
    
    /*! Send the HTTP header. */
    ret = td.connection->socket.send(td.buffer2->getBuffer(), 
														 (u_long)td.buffer2->getLength(), 0);
    if( ret == SOCKET_ERROR )
    {
      return 0;
    }
    return 1;
  }
  catch(...)
  {
    return raiseHTTPError(e_500); 
  };
}

/*!
 *Handle the HTTP TRACE command.
 */
int Http::traceHTTPRESOURCE(string& /*filename*/, int /*yetmapped*/)
{
	int ret;
	char tmpStr[12];
	int content_len=(int)td.nHeaderChars;
	string time;
  try
  {
    MemBuf tmp;
		HttpRequestHeader::Entry *connection;

    tmp.intToStr(content_len, tmpStr, 12);
    getRFC822GMTTime(time, HTTP_RESPONSE_DATE_DIM);
    if(!allowHTTPTRACE())
      return raiseHTTPError(e_401);
    td.buffer2->setLength(0);
    *td.buffer2 <<  "HTTP/1.1 200 OK\r\n";
    *td.buffer2 << "Date: " << time ;
    *td.buffer2 <<  "\r\nServer: MyServer "  << versionOfSoftware ;
		connection = td.request.other.get("Connection");
    if(connection && connection->value->length())
      *td.buffer2 << "\r\nConnection:" << connection->value->c_str();
    *td.buffer2 <<"\r\nContent-Length:" << tmp
                 << "\r\nContent-Type: message/http\r\nAccept-Ranges: bytes\r\n\r\n";
    
    /*! Send our HTTP header. */
    ret = td.connection->socket.send(td.buffer2->getBuffer(), 
                         (u_long)td.buffer2->getLength(), 0);
    if( ret == SOCKET_ERROR )
    {
      return 0;
    }
    
    /*! Send the client request header as the HTTP body. */
    ret = td.connection->socket.send(td.buffer->getBuffer(), content_len, 0);
    if(ret == SOCKET_ERROR)
    {
      return 0;
    }
    return 1;
  }
  catch(...)
  {
    return raiseHTTPError(e_500); 
  };
}
  
/*!
 *Check if the host allows the HTTP TRACE command
 */
int Http::allowHTTPTRACE()
{
	int ret;
	/*! Check if the host allows HTTP trace. */
	ostringstream filename;
  char *http_trace_value;
	XmlParser parser;
	
  filename << td.getVhostDir() << "/security" ;
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
int Http::putHTTPRESOURCE(string& filename, int, int, 
													int yetmapped)
{
  u_long firstByte = td.request.rangeByteBegin; 
  int permissions=-1;
	int httpStatus=td.response.httpStatus;
	int keepalive=0;
  int permissions2=0;
  char auth_type[16];	
  SecurityToken st;
  string directory;
  string file;

  st.td=&td;
  st.auth_type = auth_type;
  st.len_auth = 16;
  try
  {
    HttpHeaders::buildDefaultHTTPResponseHeader(&td.response);
		HttpRequestHeader::Entry *connection = td.request.other.get("Connection");
    if(connection && !stringcmpi(connection->value->c_str(), "keep-alive"))
    {
      td.response.connection.assign("keep-alive");
      keepalive=1;
    }
    File::splitPath(td.filenamePath, directory, file);
    td.response.httpStatus=httpStatus;
    /*!
     *td.filenamePath is the file system mapped path while filename 
     *is the uri requested.
     *systemrequest is 1 if the file is in the system directory.
     *If filename is already mapped on the file system don't map it again.
     */
    if(yetmapped)
    {
      td.filenamePath.assign(filename);
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
        return raiseHTTPError(e_401);
      }
      ret = getPath(td.filenamePath, filename, 0);
      if(ret!=e_200)
        return raiseHTTPError(ret);
    }
    if(File::isDirectory(td.filenamePath.c_str()))
    {
      directory.assign(td.filenamePath);
    }
    else
    {
      string fn;
      fn.assign(filename);
      File::splitPath(td.filenamePath, directory, fn);
    }
    if(td.connection->protocolBuffer==0)
    {
      td.connection->protocolBuffer=new HttpUserData;
      if(!td.connection->protocolBuffer)
      {
        return sendHTTPhardError500();
      }
      ((HttpUserData*)(td.connection->protocolBuffer))->reset();
    }
    if(td.request.auth.length())
    {
      st.user = td.connection->getLogin();
      st.password = td.connection->getPassword();
      st.directory = directory.c_str();
      st.sysdirectory = td.getVhostSys();
      st.filename = file.c_str();
      st.password2 = ((HttpUserData*)td.connection->protocolBuffer)->neededPassword;
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
        st.sysdirectory = td.getVhostSys();
        st.filename = file.c_str();
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
      td.connection->host->warningslogRequestAccess(td.id);
      td.connection->host->warningsLogWrite("Http: Error reading security file");
      td.connection->host->warningslogTerminateAccess(td.id);
      return raiseHTTPError(e_500); 
    }
    /*! Check if we have to use digest for the current directory. */
    if(!lstrcmpi(auth_type, "Digest"))
    {
      if(!td.request.auth.compare("Digest"))
      {
        if(!((HttpUserData*)td.connection->protocolBuffer)->digestChecked)
          ((HttpUserData*)td.connection->protocolBuffer)->digest = checkDigest();
        ((HttpUserData*)td.connection->protocolBuffer)->digestChecked=1;
        if(((HttpUserData*)td.connection->protocolBuffer)->digest==1)
        {
          td.connection->setPassword(((HttpUserData*)td.connection->protocolBuffer)->neededPassword);
          permissions=permissions2;
        }
      }
      td.auth_scheme=HTTP_AUTH_SCHEME_DIGEST;
    }
    else/*! By default use the Basic authentication scheme. */
    {
      td.auth_scheme=HTTP_AUTH_SCHEME_BASIC;
    }	
    /*! If there are no permissions, use the Guest permissions. */
    if(td.request.auth.length() && (permissions==0))
    {
      st.user = "Guest";
      st.password = "";
      st.directory = directory.c_str();
      st.sysdirectory = td.getVhostSys();
      st.filename = file.c_str();
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
      td.connection->host->warningslogRequestAccess(td.id);
      td.connection->host->warningsLogWrite("Http: Error reading security file");
      td.connection->host->warningslogTerminateAccess(td.id);
      return raiseHTTPError(e_500); 
    }
    if(!(permissions & MYSERVER_PERMISSION_WRITE))
    {
      return sendAuth();
    }
    if(File::fileExists(td.filenamePath.c_str()))
    {
      /*! If the file exists update it. */
      File file;
      if(file.openFile(td.filenamePath.c_str(), FILE_OPEN_IFEXISTS | 
                       FILE_OPEN_WRITE))
      {
        /*! Return an internal server error. */
        return raiseHTTPError(e_500);
      }
      file.setFilePointer(firstByte);
      for(;;)
      {
        u_long nbr=0, nbw=0;
        if(td.inputData.readFromFile(td.buffer->getBuffer(), 
																		 td.buffer->getRealLength(), &nbr))
        {
          file.closeFile();
          /*! Return an internal server error. */
          return raiseHTTPError(e_500);
        }
        if(nbr)
        {
          if(file.writeToFile(td.buffer->getBuffer(), nbr, &nbw))
          {
            file.closeFile();
            /*! Return an internal server error. */
            return raiseHTTPError(e_500);
          }
        }
        else
          break;
        if(nbw!=nbr)
        {
          file.closeFile();
          /*! Internal server error. */
          return raiseHTTPError(e_500);
        }
      }
      file.closeFile();
      /*! Successful updated. */
      raiseHTTPError(e_200);
      
      return keepalive;
    }
    else
    {
      /*!
       *If the file doesn't exist create it.
       */
      File file;
      if(file.openFile(td.filenamePath.c_str(), 
                       FILE_CREATE_ALWAYS|FILE_OPEN_WRITE))
      {
        /*! Internal server error. */
        return raiseHTTPError(e_500);
      }
      for(;;)
      {
        u_long nbr=0, nbw=0;
        if(td.inputData.readFromFile(td.buffer->getBuffer(), 
                                      td.buffer->getRealLength(), &nbr))
        {
          file.closeFile();
          return raiseHTTPError(e_500);
        }
        if(nbr)
        {
          if(file.writeToFile(td.buffer->getBuffer(), nbr, &nbw))
          {
            file.closeFile();
            return raiseHTTPError(e_500);
          }
        }
        else
          break;
        if( nbw != nbr )
        {
          file.closeFile();
          return raiseHTTPError(e_500);
        }
      }
      file.closeFile();
      /*! Successful created. */
      raiseHTTPError(e_201);
      return 1;
    }
	}
  catch(...)
  {
    return raiseHTTPError(e_500); 
  };



}

/*!
 *Delete the resource identified by filename.
 */
int Http::deleteHTTPRESOURCE(string& filename, int yetmapped)
{
  int permissions=-1;
  string directory;
	int httpStatus=td.response.httpStatus;
	int permissions2=0;
	char auth_type[16];
  SecurityToken st;
  string file;
  try
  {
    HttpHeaders::buildDefaultHTTPResponseHeader(&td.response);
		HttpRequestHeader::Entry *connection = td.request.other.get("Connection");

    if(connection && !stringcmpi(connection->value->c_str(), "keep-alive"))
    {
      td.response.connection.assign( "keep-alive");
    }
    st.auth_type = auth_type;
    st.len_auth = 16;
    st.td=&td;
    File::splitPath(td.filenamePath, directory, file);    
    td.response.httpStatus=httpStatus;
    /*!
     *td.filenamePath is the file system mapped path while filename 
     *is the uri requested.
     *systemrequest is 1 if the file is in the system directory.
     *If filename is already mapped on the file system don't map it again.
     */
    if(yetmapped)
    {
      td.filenamePath.assign(filename);
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
        return raiseHTTPError(e_401);
      }
      ret=getPath(td.filenamePath, filename, 0);
      if(ret!=e_200)
        return raiseHTTPError(ret);
    }
    if(File::isDirectory(td.filenamePath.c_str()))
    {
      directory.assign(td.filenamePath);
    }
    else
    {
      File::splitPath(td.filenamePath, directory, file);
    }

    if(td.connection->protocolBuffer==0)
    {
      td.connection->protocolBuffer=new HttpUserData;
      if(!td.connection->protocolBuffer)
      {
        return 0;
      }
      ((HttpUserData*)(td.connection->protocolBuffer))->reset();
    }

    if(td.request.auth.length())
    {
      st.user = td.connection->getLogin();
      st.password = td.connection->getPassword();
      st.directory = directory.c_str();
      st.sysdirectory = td.getVhostSys();
      st.filename = file.c_str();
      st.password2 = ((HttpUserData*)td.connection->protocolBuffer)->neededPassword;
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
      st.sysdirectory = td.getVhostSys();
      st.filename = file.c_str();
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
      td.connection->host->warningslogRequestAccess(td.id);
      td.connection->host->warningsLogWrite("Http: Error reading security file");
      td.connection->host->warningslogTerminateAccess(td.id);
      return raiseHTTPError(e_500); 
    }
    /*! Check if we have to use digest for the current directory. */
    if(!lstrcmpi(auth_type, "Digest"))
    {
      if(!td.request.auth.compare("Digest"))
      {
        if(!((HttpUserData*)td.connection->protocolBuffer)->digestChecked)
          ((HttpUserData*)td.connection->protocolBuffer)->digest = checkDigest();
        ((HttpUserData*)td.connection->protocolBuffer)->digestChecked=1;
        if(((HttpUserData*)td.connection->protocolBuffer)->digest==1)
        {
          td.connection->setPassword(((HttpUserData*)td.connection->protocolBuffer)->neededPassword);
					permissions=permissions2;
        }
      }
      td.auth_scheme=HTTP_AUTH_SCHEME_DIGEST;
    }
    /*! By default use the Basic authentication scheme. */
    else
    {
      td.auth_scheme=HTTP_AUTH_SCHEME_BASIC;
    }	
    /*! If there are no permissions, use the Guest permissions. */
    if(td.request.auth.length() && (permissions==0))
    {
      st.user = "Guest";
      st.password = "";
      st.directory = directory.c_str();
      st.sysdirectory = td.getVhostSys();
      st.filename = file.c_str();
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
      td.connection->host->warningslogRequestAccess(td.id);
      td.connection->host->warningsLogWrite("Http: Error reading security file");
      td.connection->host->warningslogTerminateAccess(td.id);
      return raiseHTTPError(e_500);
    }
    if(!(permissions & MYSERVER_PERMISSION_DELETE))
	  {
      return sendAuth();
    }
    if(File::fileExists(td.filenamePath))
	  {
      File::deleteFile(td.filenamePath.c_str());
      /*! Successful deleted. */
      return raiseHTTPError(e_202);
    }
    else
  	{
      /*! No content. */
      return raiseHTTPError(e_204);
    }
  }
  catch(...)
  {
    return raiseHTTPError(e_500); 
  };

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
	u_long digest_count;
  /*! Return 0 if the password is different. */
	if(td.request.digestOpaque[0] && lstrcmp(td.request.digestOpaque, 
                                 ((HttpUserData*)td.connection->protocolBuffer)->opaque))
		return 0;
  /*! If is not equal return 0. */
	if(lstrcmp(td.request.digestRealm, ((HttpUserData*)td.connection->protocolBuffer)->realm))
		return 0;
	
	digest_count = hexToInt(td.request.digestNc);
	
	if(digest_count != ((HttpUserData*)td.connection->protocolBuffer)->nc+1)
		return 0;
	else
		((HttpUserData*)td.connection->protocolBuffer)->nc++;
   
	md5.init();
	td.buffer2->setLength(0);
	*td.buffer2 << td.request.digestUsername << ":" << td.request.digestRealm 
							<< ":" << ((HttpUserData*)td.connection->protocolBuffer)->neededPassword;

	md5.update((unsigned char const*)td.buffer2->getBuffer(), 
             (unsigned int)td.buffer2->getLength());
	md5.end(A1);
	
	md5.init();

	if(td.request.digestUri[0])
		uri=td.request.digestUri;
  else
    uri=(char*)td.request.uriOpts.c_str();

	td.buffer2->setLength(0);
	*td.buffer2 <<  td.request.cmd.c_str() <<  ":" << uri;
	md5.update((unsigned char const*)td.buffer2->getBuffer(), 
             (unsigned int)td.buffer2->getLength());
	md5.end( A2);
	
	md5.init();
	td.buffer2->setLength(0);
	*td.buffer2 << A1 << ":"  << ((HttpUserData*)td.connection->protocolBuffer)->nonce << ":" 
               << td.request.digestNc << ":"  << td.request.digestCnonce << ":" 
               << td.request.digestQop  << ":" << A2;
	md5.update((unsigned char const*)td.buffer2->getBuffer(), 
             (unsigned int)td.buffer2->getLength());
	md5.end(response);	

	if(!lstrcmp(response, td.request.digestResponse))
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
int Http::sendHTTPResource(string& uri, int systemrequest, int onlyHeader, 
													 int yetmapped)
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
	int mimecmd;
	time_t lastMT;
  int ret;
  char auth_type[16];
  string tmpTime;
  SecurityToken st;
  string directory;
  string file;
  try
  {
		HttpRequestHeader::Entry *connection = td.request.other.get("Connection");

    st.auth_type = auth_type;
    st.len_auth = 16;
    st.td=&td;
    filename.assign(uri);
    td.buffer->setLength(0);

    if(connection && !stringcmpi(connection->value->c_str(), "keep-alive"))
    {
      td.response.connection.assign("keep-alive");
    }
    

    /*!
     *td.filenamePath is the file system mapped path while filename 
     *is the uri requested. systemrequest is 1 if the file is in 
     *the system directory.
     *If filename is already mapped on the file system don't map it again.
     */
    if(yetmapped)
    {
      td.filenamePath.assign(filename);
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
        return raiseHTTPError(e_401);
      }
      /*! getPath will alloc the buffer for filenamePath. */
      ret=getPath(td.filenamePath, filename.c_str(), systemrequest);
      if(ret!=e_200)
        return raiseHTTPError(ret);
    }

    /*! By default allows only few actions. */
    permissions= MYSERVER_PERMISSION_READ |  MYSERVER_PERMISSION_BROWSE ;

    if(!systemrequest)
    {
      if(File::isLink(td.filenamePath.c_str())) 
      {
        const char *perm = td.connection->host->getHashedData("FOLLOW_LINKS");
        if(perm && !strcmpi(perm, "YES"))
          mimecmd = CGI_CMD_SEND;
        else
          return raiseHTTPError(e_401);
      }
      else if(File::isDirectory(td.filenamePath.c_str()))
      {
        directory.assign(td.filenamePath);
      }
      else
      {
        File::splitPath(td.filenamePath, directory, file);
      }

      if(td.connection->protocolBuffer==0)
      {
        td.connection->protocolBuffer=new HttpUserData;
        if(!td.connection->protocolBuffer)
        {
          return sendHTTPhardError500();
        }
        ((HttpUserData*)td.connection->protocolBuffer)->reset();
      }
      permissions2=0;
      if(td.request.auth.length())
      {
        st.user = td.connection->getLogin();
        st.password = td.connection->getPassword();
        st.directory = directory.c_str();
        st.sysdirectory = td.getVhostSys();
        st.filename = file.c_str();
        st.password2 = ((HttpUserData*)td.connection->protocolBuffer)->neededPassword;
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
        st.sysdirectory = td.getVhostSys();
        st.filename = file.c_str();
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
        if(!td.request.auth.compare("Digest"))
			  {
          if(!((HttpUserData*)td.connection->protocolBuffer)->digestChecked)
            ((HttpUserData*)td.connection->protocolBuffer)->digest = checkDigest();

          ((HttpUserData*)td.connection->protocolBuffer)->digestChecked=1;

          if(((HttpUserData*)td.connection->protocolBuffer)->digest==1)
          {
            td.connection->setPassword(((HttpUserData*)td.connection->protocolBuffer)->neededPassword);
            permissions=permissions2;
          }
        }
        td.auth_scheme=HTTP_AUTH_SCHEME_DIGEST;
      }
      else/*! By default use the Basic authentication scheme. */
      {
        td.auth_scheme=HTTP_AUTH_SCHEME_BASIC;
      }	
 
     /*! If there are no permissions, use the Guest permissions. */
      if(td.request.auth.length() && (permissions==0))
      {
        st.user = "Guest";
        st.password = "";
        st.directory = directory.c_str();
        st.sysdirectory = td.getVhostSys();
        st.filename = file.c_str();
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
      td.connection->host->warningslogRequestAccess(td.id);
      td.connection->host->warningsLogWrite("Http: Error reading security file");
      td.connection->host->warningslogTerminateAccess(td.id);
      return raiseHTTPError(e_500); 
    }


    /*! If a throttling rate was specifed use it. */
    if(st.throttlingRate != -1)
      td.connection->socket.setThrottling(st.throttlingRate);


    /*!
     *Get the PATH_INFO value.
     *Use dirscan as a buffer for put temporary directory scan.
     *When an '/' character is present check if the path up to '/' character
     *is a file. If it is a file send the rest of the uri as PATH_INFO.
     */
    td.pathInfo.assign("");
    td.pathTranslated.assign("");
    filenamePathLen=(int)td.filenamePath.length();
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
      if(i && (td.filenamePath[i]=='/'))
      {
        /*!
         *If the token is a file.
         */
        if(!File::isDirectory(dirscan.c_str()))
        {
          td.pathInfo.assign((char*)&(td.filenamePath[i]));
          td.filenamePath.assign(dirscan);
          break;
        }
      }
      if(len+1 < filenamePathLen)
      {
        char db[2];
        db[0]=(td.filenamePath)[i];
        db[1]='\0';
        dirscan.append(db);
      }
    }

    /*!
     *If there is a PATH_INFO value the get the PATH_TRANSLATED too.
     *PATH_TRANSLATED is the local filesystem mapped version of PATH_INFO.
     */
    if(td.pathInfo.length()>1)
    {
      int ret;
      /*!
       *Start from the second character because the first is a
       *slash character.  
       */
      ret=getPath(td.pathTranslated, &((td.pathInfo.c_str())[1]), 0);
      if(ret!=e_200)
        return raiseHTTPError(ret);
      File::completePath(td.pathTranslated);
    }
    else
    {
      td.pathTranslated.assign("");
    }
    File::completePath(td.filenamePath);

    /*!
     *If there are not any extension then we do one of this in order:
     *1)We send the default files in the directory in order.
     *2)We send the directory content.
     *3)We send an error.
     */
    if(File::isDirectory(td.filenamePath.c_str()))
    {
      int i;
      if(!(permissions & MYSERVER_PERMISSION_BROWSE))
      {
        return sendAuth();
      }
      for(i=0;;i++)
      {
        const char *defaultFileNamePath=getDefaultFilenamePath(i);
        ostringstream defaultFileName;
        defaultFileName.clear();
        if(defaultFileNamePath)
        {
          defaultFileName << td.filenamePath << "/" << defaultFileNamePath;
        }
        else
        {
          break;
        }

        if(File::fileExists(defaultFileName.str().c_str()))
        { 
          ostringstream nURL;
          if(td.request.uriEndsWithSlash)
          {
            nURL << defaultFileNamePath;
          }
          else
          {
            int last_slash_offset = uri.length();
            while(last_slash_offset && uri[last_slash_offset]!='/')
              --last_slash_offset;
						
            nURL  <<  &(uri.c_str()[last_slash_offset ? 
																		last_slash_offset+1 : 0]) 
                  << "/" << defaultFileNamePath;
          }
          /*! Send a redirect to the new location.  */
          if(sendHTTPRedirect(nURL.str().c_str()))
            ret = 1;
          else
            ret = 0;
          return ret;
        }
      }
      return lhttp_dir.send(&td, td.connection, td.filenamePath.c_str(), 0, onlyHeader);
    }
    
    if(!File::fileExists(td.filenamePath.c_str()))
      return raiseHTTPError(e_404);

    /*!
     *getMIME returns the type of command registered by the extension.
     */
    data.assign("");
    {
      td.response.contentType[0]='\0';
      td.mime=getMIME(td.filenamePath);
      /*! Set the default content type, this can be changed later. */
      if(td.mime)
      {
        td.response.contentType.assign(((MimeManager::MimeRecord*)td.mime)->mime_type);
        mimecmd = ((MimeManager::MimeRecord*)td.mime)->command;
        data.assign(((MimeManager::MimeRecord*)td.mime)->cgi_manager);
      }
      else
      {
        td.response.contentType.assign("text/html");
        mimecmd = CGI_CMD_SEND;
        data.assign("");
      }
    }

    if(td.mime && 
       !((MimeManager::MimeRecord*)td.mime)->headerChecker.isAllowed(&(td.request) ))
    {
      return sendAuth();
    }

    if(mimecmd==CGI_CMD_RUNCGI)
    {
      int allowCgi = 1;
	    const char *dataH=td.connection->host->getHashedData("ALLOW_CGI");
	    if(dataH)
	    {
        if(!strcmpi(dataH, "YES"))
          allowCgi=1;
        else
          allowCgi=0;      
	    }

      if(!allowCgi || !(permissions & MYSERVER_PERMISSION_EXECUTE))
      {
        return sendAuth();
      }
      ret = lcgi.send(&td, td.connection, td.filenamePath.c_str(), data.c_str(), 0,  onlyHeader);
      return ret;
    }
    else if(mimecmd==CGI_CMD_EXECUTE )
    {
      int allowCgi = 1;
	    const char *dataH=td.connection->host->getHashedData("ALLOW_CGI");
	    if(dataH)
	    {
        if(!strcmpi(dataH, "YES"))
          allowCgi=1;
        else
          allowCgi=0;      
	    }

      if(!allowCgi || !(permissions & MYSERVER_PERMISSION_EXECUTE))
		  {
        return sendAuth();
      }
      ret = lcgi.send(&td, td.connection, td.filenamePath.c_str(), data.c_str(), 1, onlyHeader);
      return ret;
    }
    else if(mimecmd == CGI_CMD_RUNISAPI)
    {
      int allowIsapi = 1;
	    const char *dataH=td.connection->host->getHashedData("ALLOW_ISAPI");
	    if(dataH)
	    {
        if(!strcmpi(dataH, "YES"))
          allowIsapi=1;
        else
          allowIsapi=0;      
	    }
      if(!allowIsapi || !(permissions & MYSERVER_PERMISSION_EXECUTE))
      {
        return sendAuth();
      }
      ret = lisapi.send(&td, td.connection, td.filenamePath.c_str(), data.c_str(), 0, 
                        onlyHeader);
      return ret;

    }
    else if(mimecmd==CGI_CMD_EXECUTEISAPI)
    {
      if(!(permissions & MYSERVER_PERMISSION_EXECUTE))
      {
        return sendAuth();
      }
      ret = lisapi.send(&td, td.connection, td.filenamePath.c_str(), data.c_str(), 1, 
                        onlyHeader);
      return ret;
    }
    else if( mimecmd == CGI_CMD_RUNMSCGI )
    {
      char* target;
      int allowMscgi = 1;
	    const char *dataH=td.connection->host->getHashedData("ALLOW_MSCGI");
	    if(dataH)
	    {
        if(!strcmpi(dataH, "YES"))
          allowMscgi=1;
        else
          allowMscgi=0;      
	    }

      if(!allowMscgi || !(permissions & MYSERVER_PERMISSION_EXECUTE))
      {
        return sendAuth();
      }
      if(td.request.uriOptsPtr)
        target=td.request.uriOptsPtr;
      else
        target=(char*)td.request.uriOpts.c_str();
      /*! Check if the MSCGI library is loaded. */
      if(mscgiLoaded)
      {
        ret=lmscgi.send(&td, td.connection, td.filenamePath.c_str(), target, 1, onlyHeader);
        return ret;
      }
      return raiseHTTPError(e_500);
    }
    else if( mimecmd == CGI_CMD_EXECUTEWINCGI )
    {
      ostringstream cgipath;
      int allowWincgi = 1;
	    const char *dataH=td.connection->host->getHashedData("ALLOW_WINCGI");
	    if(dataH)
	    {
        if(!strcmpi(dataH, "YES"))
          allowWincgi=1;
        else
          allowWincgi=0;      
	    }

      if(!allowWincgi || !(permissions & MYSERVER_PERMISSION_EXECUTE))
      {
        return sendAuth();
      }
      if(data.length())
      {
        cgipath <<  data << " \""<< td.filenamePath <<  "\"";
      }
      else
      {
        cgipath << td.filenamePath;
      }
      ret=lwincgi.send(&td, td.connection, cgipath.str().c_str(), 1, onlyHeader);
      return ret;
    }
    else if( mimecmd == CGI_CMD_RUNFASTCGI )
    {
      int allowFastcgi = 1;
	    const char *dataH=td.connection->host->getHashedData("ALLOW_FASTCGI");
	    if(dataH)
	    {
        if(!strcmpi(dataH, "YES"))
          allowFastcgi=1;
        else
          allowFastcgi=0;      
	    }
      if(!allowFastcgi || !(permissions & MYSERVER_PERMISSION_EXECUTE))
      {
        return sendAuth();
      }	
      ret = lfastcgi.send(&td, td.connection, td.filenamePath.c_str(), data.c_str(), 0, 
                          onlyHeader);
      return ret;
    }
    else if(mimecmd==CGI_CMD_EXECUTEFASTCGI)
    {
      int allowFastcgi = 1;
	    const char *dataH=td.connection->host->getHashedData("ALLOW_FASTCGI");
	    if(dataH)
	    {
        if(!strcmpi(dataH, "YES"))
          allowFastcgi=1;
        else
          allowFastcgi=0;      
	    }
      if(!allowFastcgi || !(permissions & MYSERVER_PERMISSION_EXECUTE))
      {
        return sendAuth();
      }
      ret = lfastcgi.send(&td, td.connection, td.filenamePath.c_str(), data.c_str(), 1, 
                          onlyHeader);
      return ret;
    }
    else if( mimecmd == CGI_CMD_SENDLINK )
    {
      u_long nbr;
      char* linkpath;
      char* pathInfo;
      int linkpathSize;
      File h;
      int allowSendlink = 1;
	    const char *dataH=td.connection->host->getHashedData("ALLOW_SEND_LINK");
	    if(dataH)
	    {
        if(!strcmpi(dataH, "YES"))
          allowSendlink=1;
        else
          allowSendlink=0;      
	    }
      if(!allowSendlink || !(permissions & MYSERVER_PERMISSION_READ))
      {
        return sendAuth();
      }

      if(h.openFile(td.filenamePath.c_str(), 
                    FILE_OPEN_IFEXISTS|FILE_OPEN_READ))
      {
        return raiseHTTPError(e_500);/*!Internal server error*/
      }
      linkpathSize = h.getFileSize()+td.pathInfo.length()+1;
      if(linkpathSize > MYSERVER_KB(10))
        linkpathSize = MYSERVER_KB(10);
      linkpath=new char[linkpathSize];
      if(linkpath==0)
      {
        return sendHTTPhardError500();
      }
      if(h.readFromFile(linkpath, linkpathSize, &nbr))
      {
        h.closeFile();
        delete [] linkpath;
        return raiseHTTPError(e_500);/*!Internal server error*/
      }
      h.closeFile();
      linkpath[nbr]='\0';
      pathInfo=new char[td.pathInfo.length()+1];
      if(pathInfo == 0)
      {
        delete [] linkpath;
        return raiseHTTPError(e_500);/*!Internal server error*/
      }
      strcpy(pathInfo, td.pathInfo.c_str());
      translateEscapeString(pathInfo);
      strncat(linkpath, pathInfo,strlen(linkpath));
      if(nbr)
      {
        string uri;
        uri.assign(linkpath);
        ret = sendHTTPResource(uri, systemrequest, onlyHeader, 1);
      }
      else
        ret = raiseHTTPError(e_404);
      delete [] linkpath;
      delete [] pathInfo;
      return ret;
    }
    else if( mimecmd == CGI_CMD_EXTERNAL )
    {
      int allowExternal = 1;
	    const char *dataH=td.connection->host->getHashedData("ALLOW_EXTERNAL_COMMANDS");
	    if(dataH)
	    {
        if(!strcmpi(dataH, "YES"))
          allowExternal=1;
        else
          allowExternal=0;      
	    }     
      if(allowExternal && (MimeManager::MimeRecord*)td.mime)
      {
        DynamicHttpManager* manager = dynManagerList.getManagerByName(
                             ((MimeManager::MimeRecord*)td.mime)->cmd_name.c_str());

        if(manager)
          return manager->send(&td, td.connection, td.filenamePath.c_str(), 
                               data.c_str(), onlyHeader);
        else
          return raiseHTTPError(e_501);
      }

    }

    {
      int allowSend = 1;
	    const char *dataH=td.connection->host->getHashedData("ALLOW_SEND_FILE");
	    if(dataH)
	    {
        if(!strcmpi(dataH, "YES"))
          allowSend=1;
        else
          allowSend=0;      
	    }   
      if(!allowSend)
      {     
        return sendAuth();
      }       
    }

    /*! By default try to send the file as it is. */
    if(!(permissions & MYSERVER_PERMISSION_READ))
    {     
      return sendAuth();
    }

    lastMT=File::getLastModTime(td.filenamePath.c_str());
    if(lastMT==-1)
    {
      return raiseHTTPError(e_500);
    }
    getRFC822GMTTime(lastMT, tmpTime, HTTP_RESPONSE_LAST_MODIFIED_DIM);
		td.response.lastModified.assign(tmpTime);
    
		{
			HttpRequestHeader::Entry *ifModifiedSince = td.request.other.get("Last-Modified");

			if(ifModifiedSince && ifModifiedSince->value->length())
			{
				if(!ifModifiedSince->value->compare(td.response.lastModified.c_str()))
				{
					return sendHTTPNonModified();
				}
			}
		}
    ret = lhttp_file.send(&td, td.connection, td.filenamePath.c_str(), 0, onlyHeader);
  }
  catch(...)
  {
    return raiseHTTPError(e_500); 
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
    td.buffer2->setLength(0);
    *td.buffer2 << td.connection->getIpAddr();
    *td.buffer2<< " ";
	
	  if(td.connection->getLogin())
      *td.buffer2 << td.connection->getLogin();
    else
      *td.buffer2 << "-";

    *td.buffer2<< " ";  

    if(td.connection->getLogin())
      *td.buffer2 << td.connection->getLogin();
    else
      *td.buffer2 << "-";
    
    *td.buffer2 << " [";
    
    getLocalLogFormatDate(time, HTTP_RESPONSE_DATE_DIM);
    *td.buffer2 <<  time  << "] \"";
    
    if(td.request.cmd.length())
      *td.buffer2 << td.request.cmd.c_str() << "";
  
    if(td.request.cmd.length() || td.request.uri.length())
      *td.buffer2 << " ";

    if(td.request.uri.length() == '\0')
      *td.buffer2 <<  "/";
    else
      *td.buffer2 << td.request.uri.c_str();
  

    if(td.request.uriOpts.length())
      *td.buffer2 << "?" << td.request.uriOpts.c_str();
    
    sprintf(tmpStrInt, "%u ",td.response.httpStatus);
  
    if(td.request.ver.length())
      *td.buffer2 << " " << td.request.ver.c_str()  ;
    
    *td.buffer2<< "\" " << tmpStrInt  << " ";
	
    if(td.response.contentLength.length())
      *td.buffer2  << td.response.contentLength.c_str();
    else
      *td.buffer2 << "0";
    if(td.connection->host)
    {
			HttpRequestHeader::Entry *userAgent = td.request.other.get("User-Agent");
			HttpRequestHeader::Entry *referer = td.request.other.get("Refer");

      if(strstr((td.connection->host)->getAccessLogOpt(), "type=combined"))
        *td.buffer2 << " "  << (referer   ? referer->value->c_str() : "") 
										<< " "  << (userAgent ? userAgent->value->c_str() : "");
    }  
#ifdef WIN32
    *td.buffer2  << "\r\n" << end_str;
#else
    *td.buffer2  << "\n" << end_str;
#endif
    /*!
     *Request the access to the log file then append the message.
     */
     if(td.connection->host)
     {
       td.connection->host->accesseslogRequestAccess(td.id);
       td.connection->host->accessesLogWrite(td.buffer2->getBuffer());
       td.connection->host->accesseslogTerminateAccess(td.id);
     }
    td.buffer2->setLength(0);
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
  DynamicHttpCommand *dynamicCommand;
  try
  {
    td.buffer=((ClientsThread*)a->thread)->getBuffer();
    td.buffer2=((ClientsThread*)a->thread)->getBuffer2();
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
		td.vhostDir.assign("");
		td.vhostSys.assign("");
		{
			HashMap<string,string*>::Iterator it = td.other.begin();
			while(it != td.other.end())
				delete (*it);
		}
		td.other.clear();

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
        retvalue = raiseHTTPError(e_503);
				logHTTPaccess();
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
      if(td.request.ver.compare("HTTP/1.0"))
      {
        char* msg = "HTTP/1.1 100 Continue\r\n\r\n";
        Thread::wait(2);
        if(a->socket.bytesToRead() == 0) 
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
      retvalue = raiseHTTPError(validRequest);
      logHTTPaccess();
      return 0;
    }
    /*! Be sure that we can handle the HTTP version. */
    if((td.request.ver.compare("HTTP/1.1")) && 
       (td.request.ver.compare("HTTP/1.0")) && 
       (td.request.ver.compare("HTTP/0.9")))
    {	
      raiseHTTPError(e_505);
      logHTTPaccess();
      /*! Remove the connection from the list. */
      return 0;
    }

		td.response.ver.assign(td.request.ver.c_str());

    /*! Do not use Keep-Alive over HTTP version older than 1.1. */
    if(td.request.ver.compare("HTTP/1.1") )
    {
			HttpRequestHeader::Entry *connection = td.request.other.get("Connection");

      if(connection && connection->value->length())
			{
        connection->value->assign("close");
			}
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

    dynamicCommand=dynCmdManager.getMethodByName(td.request.cmd.c_str());

    if((!td.request.cmd.compare("POST"))||(!td.request.cmd.compare("PUT")) || 
       (dynamicCommand && dynamicCommand->acceptData() ))
    {
			HttpRequestHeader::Entry *contentType = td.request.other.get("Content-Type");

			if(contentType == 0)
			{
				contentType = new HttpRequestHeader::Entry();
				contentType->name->assign("Content-Type");
				contentType->value->assign("application/x-www-form-urlencoded");
			}
			else if(contentType->value->length() == 0)
        contentType->value->assign("application/x-www-form-urlencoded");
		
      /*!
       *Read POST data.
       */
      {
        u_long nbw=0;
        u_long total_nbr=0;
        u_long timeout;
        td.request.uriOptsPtr=&(td.buffer->getBuffer())[td.nHeaderChars];
        td.buffer->getBuffer()[td.nBytesToRead<td.buffer->getRealLength()-1 
                      ? td.nBytesToRead : td.buffer->getRealLength()-1]='\0';
        /*!
         *Create the file that contains the posted data.
         *This data is the stdin file in the CGI.
         */
        if(td.inputData.openFile(td.inputDataPath,FILE_CREATE_ALWAYS | 
                                 FILE_OPEN_READ|FILE_OPEN_WRITE))
          return 0;
          
        nbw=0;
        total_nbr=(td.nBytesToRead<td.buffer->getRealLength()-1 
           ? td.nBytesToRead : td.buffer->getRealLength()-1 ) -td.nHeaderChars;
        if(total_nbr)
        {
          if(td.inputData.writeToFile(td.request.uriOptsPtr, total_nbr, &nbw))
          {
            td.inputDataPath.assign("");
            td.outputDataPath.assign("");
            td.inputData.closeFile();
            return 0;
          }
        }
        if(td.request.contentLength.length())
          content_len=atoi(td.request.contentLength.c_str());
        
        /*!
         *If the connection is Keep-Alive be sure that the client specify the
         *HTTP CONTENT-LENGTH field.
         *If a CONTENT-ENCODING is specified the CONTENT-LENGTH is not 
         *always needed.
         */
				{
					HttpRequestHeader::Entry *connection = td.request.other.get("Connection");
					
					if(connection && !stringcmpi(connection->value->c_str(), "keep-alive"))
					{
						HttpRequestHeader::Entry *content = td.request.other.get("Content-Encoding");
					
						if(content && (content->value->length()=='\0') 
							 && (td.request.contentLength.length() == 0))
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
							retvalue = raiseHTTPError(e_400);
							logHTTPaccess();
							return retvalue ? 1 : 0;
						}
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
                  ret=td.connection->socket.recv(td.buffer2->getBuffer(), 
                                                 td.buffer2->getRealLength(), 0);
                }
                break;
              }
              if((content_len>static_cast<int>(fs)) && 
                 (td.connection->socket.bytesToRead()))
              {				
                u_long tr = static_cast<u_long>(content_len)-total_nbr < 
								td.buffer2->getRealLength() 
                  ? static_cast<u_long>(content_len)-total_nbr 
                  : td.buffer2->getRealLength() ;

                ret=td.connection->socket.recv(td.buffer2->getBuffer(), tr, 0);
                if(ret==-1)
                {
                  td.inputData.closeFile();
                  td.inputData.deleteFile(td.inputDataPath);
                  return 0;
                }
							
                if(td.inputData.writeToFile(td.buffer2->getBuffer(), 
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
            retvalue = raiseHTTPError(e_500);
            logHTTPaccess();
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
                ret=td.connection->socket.recv(td.buffer2->getBuffer(), 
                                               td.buffer2->getRealLength(), 0);

                if(td.inputData.writeToFile(td.buffer2->getBuffer(), ret, &nbw))
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
          /*! Store a new value for contentLength. */
          td.response.contentLength.assign(buff.str());
        }
        td.inputData.setFilePointer(0);
      }/* End read POST data */
    }
    /*! Methods with no POST data. */
    else
    {
      td.request.uriOptsPtr=0;
    }
    /*
     *Manage chunked transfers.
     *Data loaded before don't take care of the TRANSFER ENCODING. 
     *Here we clean data, making it usable.
     */
		{
			HttpRequestHeader::Entry *encoding = td.request.other.get("Transfer-Encoding");

			if(encoding && !encoding->value->compare("chunked"))
			{
				File newStdIn;
				char buffer[20];
				char c;
				u_long nbr;
				u_long bufferlen;
				ostringstream newfilename;
				
				newfilename <<  td.inputData.getFilename() <<  "_encoded";
				if(newStdIn.openFile(td.inputDataPath, FILE_CREATE_ALWAYS | 
														 FILE_NO_INHERIT|FILE_OPEN_READ|FILE_OPEN_WRITE))
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
							break;
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
						if(td.inputData.readFromFile(td.buffer->getBuffer(), 
																				 dataToRead-dataRead < td.buffer->getRealLength()-1 
																				 ? dataToRead-dataRead: td.buffer->getRealLength()-1 , &nbr))
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
						if(newStdIn.writeToFile(td.buffer->getBuffer(), nbr, &nbw))
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
			else if(encoding && encoding->value->length())
			{
				raiseHTTPError(e_501);
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
				logHTTPaccess();
				return 0;
			}
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
			HttpRequestHeader::Entry *host = td.request.other.get("Host");
			HttpRequestHeader::Entry *connection = td.request.other.get("Connection");
		
      if((!td.request.ver.compare("HTTP/1.1")) && ((host && host->value->length()==0) || (host==0)) )
      {
        raiseHTTPError(e_400);
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
        logHTTPaccess();
        return 0;
      }
      else
      {
        /*!
         *Find the virtual host to check both host name and IP value.
         */
        Vhost* newHost=Server::getInstance()->vhostList->getVHost(host ? host->value->c_str() : "", 
																												a->getLocalIpAddr(), a->getLocalPort());
        if(a->host)
          a->host->removeRef();
        a->host=newHost;
        if(a->host==0)
        {
          raiseHTTPError(e_400);
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
          logHTTPaccess();
          return 0;
        }
      }

			if(td.request.uri.length() > 2 && td.request.uri[1] == '~'){
				string documentRoot;
				u_long len = 2;
				while(len < td.request.uri.length())
					if(td.request.uri[++len] == '/')
						break;
				documentRoot.assign("/home/");
				documentRoot.append(td.request.uri.substr(2, len-2));
				documentRoot.append("/public_html");
	
				td.vhostDir.assign(documentRoot);
				if(!td.request.uriEndsWithSlash)
				{
					td.request.uri.append("/");
					return sendHTTPRedirect(td.request.uri.c_str());
				}

				if(td.request.uri.length() - len)
					td.request.uri.assign(td.request.uri.substr(len, 
		 																					td.request.uri.length()));
				else
					td.request.uri.assign("");
			}
			
		  /*! 
       *Check if there is a limit for the number of connections in the 
			 *virtual host A value of zero means no limit.
       */
		  {
        const char* val = a->host->getHashedData("MAX_CONNECTIONS");
        if(val)
        {
          u_long limit = (u_long)atoi(val);
          if(limit && (u_long)a->host->getRef() >= limit)
          {
            retvalue = raiseHTTPError(e_500);
            logHTTPaccess();
            return retvalue ? 1 : 0;
          }     
        }
      }
		
      if(connection && !stringcmpi(connection->value->c_str(), "keep-alive")) 
      {
        /*!
         *Support for HTTP pipelining.
         */
        if(content_len==0)
        {
          /*!
           *connectionBuffer is 8 KB, so don't copy more bytes.
           */
          a->setDataRead(MYSERVER_KB(8) < (int)strlen(td.buffer->getBuffer()) -
                         td.nHeaderChars ? MYSERVER_KB(8) : 
                         (int)strlen(td.buffer->getBuffer()) - td.nHeaderChars );
          if(a->getDataRead() )
          {
            memcpy(a->connectionBuffer, (td.buffer->getBuffer() +
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
      if(a->host->getThrottlingRate() == (u_long) -1)
        a->socket.setThrottling(Server::getInstance()->getThrottlingRate());
      else
        a->socket.setThrottling(a->host->getThrottlingRate());

      
      /*!
       *Here we control all the HTTP commands.
       */
       
      /*! GET REQUEST. */
      if(!td.request.cmd.compare("GET"))
      {
        if(!td.request.rangeType.compare("bytes"))
          ret = sendHTTPResource(td.request.uri, 0, 0);
        else
          ret = sendHTTPResource(td.request.uri);
      }
      /*! POST REQUEST. */
      else if(!td.request.cmd.compare("POST"))
      {
        if(!td.request.rangeType.compare("bytes"))
          ret = sendHTTPResource(td.request.uri, 0, 0);
        else
          ret = sendHTTPResource(td.request.uri);
      }
      /*! HEAD REQUEST. */
      else if(!td.request.cmd.compare("HEAD"))
      {
        td.onlyHeader = 1;
        ret = sendHTTPResource(td.request.uri, 0, 1);
      }
      /*! DELETE REQUEST. */
      else if(!td.request.cmd.compare("DELETE"))
      {
        ret = deleteHTTPRESOURCE(td.request.uri, 0);
      }
      /*! PUT REQUEST. */
      else if(!td.request.cmd.compare("PUT"))
      {
        ret = putHTTPRESOURCE(td.request.uri, 0, 1);
      }
      /*! OPTIONS REQUEST. */
      else if(!td.request.cmd.compare("OPTIONS"))
      {
        ret = optionsHTTPRESOURCE(td.request.uri, 0);
      }
      /*! TRACE REQUEST. */
      else if(!td.request.cmd.compare("TRACE"))
      {
        ret = traceHTTPRESOURCE(td.request.uri, 0);
      }
      else
      {
        /*! 
         *Return Method not implemented(501) if there 
         *is not a dynamic methods manager. 
         */
        if(!dynamicCommand)
          ret=raiseHTTPError(e_501);
        else
          retvalue=dynamicCommand->send(&td, a, td.request.uri, 
																				0, 0, 0);
      }
      logHTTPaccess();
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
		
		{
			HttpRequestHeader::Entry *connection = td.request.other.get("Connection");
			if(connection)
				ret &= !stringcmpi(connection->value->c_str(), "keep-alive");
			else
				ret = 0;
    }
		return (ret && retvalue) ? retvalue : 0;
  }
  catch(...)
  {
    logHTTPaccess();
    return 0;
  }
}

/*!
 *Compute the Digest outputting it to a buffer.
 */
void Http::computeDigest(char* out , char* buffer)
{
	Md5 md5;
	if(!out)
		return;
	sprintf(buffer, "%i-%u-%s", (int)clock(), (u_int)td.id, 
          td.connection->getIpAddr());
	md5.init();
	md5.update((unsigned char const*)buffer , (unsigned int)strlen(buffer));
	md5.end(out);
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
    Md5 md5;
    int errorBodyLength=0;
    int useMessagesFiles = 1;
		HttpRequestHeader::Entry *host = td.request.other.get("Host");
		HttpRequestHeader::Entry *connection = td.request.other.get("Connection");
    const char *useMessagesVal = td.connection->host ? 
			td.connection->host->getHashedData("USE_ERROR_FILE") : 0;
    if(useMessagesVal)
    {
	    if(!lstrcmpi(useMessagesVal, "YES"))
	   	  useMessagesFiles=1;
	   	else
	      useMessagesFiles=0;
   	}

    if(td.lastError)
      return sendHTTPhardError500();

    td.lastError = ID;

    HttpHeaders::buildDefaultHTTPResponseHeader(&(td.response));
    if(connection && !stringcmpi(connection->value->c_str(), "keep-alive"))
    {
      td.response.connection.assign("keep-alive");
    }
    
    if(ID==e_401AUTH)
    {
      td.response.httpStatus = 401;
      td.buffer2->setLength(0);
      *td.buffer2 << 
           "HTTP/1.1 401 Unauthorized\r\nAccept-Ranges: bytes\r\nServer: MyServer " ;
      *td.buffer2 << versionOfSoftware ;
      *td.buffer2 << "\r\nContent-type: text/html\r\nConnection: ";
      *td.buffer2 << (connection ? connection->value->c_str() : "");
      *td.buffer2 << "\r\nContent-length: 0\r\n";
      if(td.auth_scheme==HTTP_AUTH_SCHEME_BASIC)
      {
        *td.buffer2 <<  "WWW-Authenticate: Basic realm=\"" 
										<< (host ? host->value->c_str() : "") <<  "\"\r\n";
      }
      else if(td.auth_scheme==HTTP_AUTH_SCHEME_DIGEST)
      {
        char md5_str[256];

        if(td.connection->protocolBuffer==0)
        {
          td.connection->protocolBuffer = new HttpUserData;
          if(!td.connection->protocolBuffer)
          {
            sendHTTPhardError500();
            return 0;
          }
          ((HttpUserData*)(td.connection->protocolBuffer))->reset();
        }
        myserver_strlcpy(((HttpUserData*)td.connection->protocolBuffer)->realm, 
                         host ? host->value->c_str() : "", 48);

        /*! Just a random string. */
        md5_str[0]=(char)td.id;
        md5_str[1]=(char)((clock() >> 24) & 0xFF);
        md5_str[2]=(char)((clock() >> 16) & 0xFF);
        md5_str[3]=(char)((clock() >>  8)   & 0xFF);
        md5_str[4]=(char) (clock() & 0xFF);
        strncpy(&(md5_str[5]), td.request.uri.c_str(), 256-5);
        md5.init();
        md5.update((unsigned char const*)md5_str,  (unsigned int)strlen(md5_str));
        md5.end(((HttpUserData*)td.connection->protocolBuffer)->opaque);
        
        if(td.connection->protocolBuffer && (!(((HttpUserData*)td.connection->protocolBuffer)->digest)) || 
           (((HttpUserData*)td.connection->protocolBuffer)->nonce[0]=='\0'))
        {
          computeDigest(((HttpUserData*)td.connection->protocolBuffer)->nonce, md5_str);
          ((HttpUserData*)td.connection->protocolBuffer)->nc=0;
        }
        *td.buffer2 << "WWW-Authenticate: digest ";
        *td.buffer2 << " qop=\"auth\", algorithm =\"MD5\", realm =\"";
        *td.buffer2 << ((HttpUserData*)td.connection->protocolBuffer)->realm ;
        *td.buffer2 << "\",  opaque =\"" 
                     << ((HttpUserData*)td.connection->protocolBuffer)->opaque;

        *td.buffer2<< "\",  nonce =\""<< ((HttpUserData*)td.connection->protocolBuffer)->nonce;
        *td.buffer2 <<"\" ";
        if(((HttpUserData*)td.connection->protocolBuffer)->cnonce[0])
        {
          *td.buffer2 << ", cnonce =\"";
          *td.buffer2 <<((HttpUserData*)td.connection->protocolBuffer)->cnonce;
          *td.buffer2 <<"\" ";
        }
        *td.buffer2 << "\r\n";
      }		
      else
      {
        /*!
         *Send a non implemented error page if the auth scheme is not known.
         */
        return raiseHTTPError(501);
      }				
      *td.buffer2 << "Date: ";
      getRFC822GMTTime(time, HTTP_RESPONSE_DATE_DIM);
      *td.buffer2  << time;
      *td.buffer2 << "\r\n\r\n";
      if(td.connection->socket.send(td.buffer2->getBuffer(), 
																		td.buffer2->getLength(), 0)==-1)
      {
        return 0;
      }
      return 1;
    }
    else
    {
      string defFile;
      int ret=0;
      td.response.httpStatus = getHTTPStatusCodeFromErrorID(ID);
      secCacheMutex.lock();
      
      /*! 
       *The specified error file name must be in the web directory 
       *of the virtual host. 
       */
      if(td.connection->host)
        ret=secCache.getErrorFileName(td.getVhostDir(), 
																			getHTTPStatusCodeFromErrorID(ID),
																			td.getVhostSys(), defFile);
      else
        ret=-1;
      
      secCacheMutex.unlock();
      
      if(ret == -1)
      {
        sendHTTPhardError500();
        return 0;
      }
      else if(ret)
      {
        ostringstream nURL;
        int isPortSpecified=0;
				const char* hostStr = host ? host->value->c_str() : "";
        /*!
         *Change the Uri to reflect the default file name.
         */
        nURL << protocolPrefix << hostStr;
        for(int i=0; hostStr[i]; i++)
        {
          if(hostStr[i]==':')
          {
            isPortSpecified = 1;
            break;
          }
        }
        if(!isPortSpecified)
          nURL << ":" << td.connection->host->getPort();
        if(nURL.str()[nURL.str().length()-1]!='/')
          nURL << "/";

        nURL << defFile;
        
        return sendHTTPRedirect(nURL.str().c_str());
      }
    }
    
    getRFC822GMTTime(time, HTTP_RESPONSE_DATE_EXPIRES_DIM);
    td.response.dateExp.assign(time);
    td.response.errorType.assign(HTTP_ERROR_MSGS[ID], 
                                   HTTP_RESPONSE_ERROR_TYPE_DIM);
    
    errorFile << td.getVhostSys() << "/" << HTTP_ERROR_HTMLS[ID];
    
    if(useMessagesFiles && File::fileExists(errorFile.str().c_str()))
    {
        string tmp;
        tmp.assign(HTTP_ERROR_HTMLS[ID]);
        return sendHTTPResource(tmp, 1, td.onlyHeader);
    }

    /*! Send only the header(and the body if specified). */

    {
      const char* value = td.connection->host->getHashedData("ERRORS_INCLUDE_BODY");  
      if(value && !strcmpi(value, "YES"))
      {
        ostringstream s;
        errorBodyLength = strlen(HTTP_ERROR_MSGS[ID]);
        s << errorBodyLength;
        td.response.contentLength.assign(s.str());
      }
      else
      {
        errorBodyLength = 0;
        td.response.contentLength.assign("0");
      }
      
    }
    
    HttpHeaders::buildHTTPResponseHeader(td.buffer->getBuffer(), &td.response);
    if(td.connection->socket.send(td.buffer->getBuffer(), 
                      (u_long)strlen(td.buffer->getBuffer()), 0)==-1)
      return 0;

    if(errorBodyLength && (td.connection->socket.send(HTTP_ERROR_MSGS[ID],errorBodyLength, 
																											0)==-1))
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
	td.response.httpStatus=500;
	td.buffer->setLength(0);
	*td.buffer <<  HTTP_ERROR_MSGS[e_500];
	*td.buffer << " from: " ;
	*td.buffer << td.connection->getIpAddr() ;
	*td.buffer << "\r\n";	
	td.buffer2->setLength(0);
	*td.buffer2 << "HTTP/1.1 500 System Error\r\nServer: MyServer ";
	*td.buffer2 << versionOfSoftware;
	*td.buffer2 <<" \r\nContent-type: text/html\r\nContent-length: ";
  tmp.intToStr((int)strlen(hardHTML), tmpStr, 12);
	*td.buffer2  <<   tmp;
	*td.buffer2   << "\r\n";
	*td.buffer2 <<"Date: ";
	getRFC822GMTTime(time, HTTP_RESPONSE_DATE_DIM);
	*td.buffer2 << time;
	*td.buffer2 << "\r\n\r\n";
	/*! Send the header. */
	if(td.connection->socket.send(td.buffer2->getBuffer(), 
                    (u_long)td.buffer2->getLength(), 0)!= -1)
	{
		/*! Send the body. */
    if(!td.onlyHeader)
   		td.connection->socket.send(hardHTML, (u_long)strlen(hardHTML), 0);
	}
	return 0;
}

/*!
 *Returns the MIME type passing its extension.
 *Returns zero if the file is registered.
 */
MimeManager::MimeRecord* Http::getMIME(string &filename)
{
  string ext;
	File::getFileExt(ext, filename);
	
  if(allowVhostMime && td.connection->host->isMIME() )
  {
    return td.connection->host->getMIME()->getRecord(ext);
  }
	return Server::getInstance()->mimeManager->getRecord(ext);
}

/*!
 *Map an URL to the machine file system. Return e_200 on success. 
 *Any other return value is the HTTP error.
 */
int Http::getPath(string& filenamePath, const char *filename, int systemrequest)
{
	/*!
   *If it is a system request, search the file in the system directory.
   */
	if(systemrequest)
	{
    if(!strlen(td.getVhostSys()) 
       || File::getPathRecursionLevel(filename)< 2 )
    {
      return e_401;
    }
    filenamePath.assign(td.getVhostSys());
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
       *uri starting with a /sys/ will use the system directory as
       *the root path. Be sure to don't allow access to the system root
       *but only to subdirectories.
       */
      if(filename[0] == '/' && filename[1] == 's' && filename[2] == 'y'
         && filename[3] == 's' && filename[4] == '/')
      {
        root = td.getVhostDir();
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
        root = td.getVhostDir();
      }
			filenamePath.assign(root);
      filenamePath.append(filename);
		}
		else
		{
      filenamePath.append(td.getVhostDir());
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
int Http::sendHTTPRedirect(const char *newURL)
{
	string time;
	HttpRequestHeader::Entry *connection = td.request.other.get("Connection");

	td.response.httpStatus=302;
	td.buffer2->setLength(0);
	*td.buffer2 << "HTTP/1.1 302 Moved\r\nAccept-Ranges: bytes\r\nServer: MyServer " ;
	*td.buffer2<< versionOfSoftware;
	*td.buffer2 << "\r\nContent-type: text/html\r\nLocation: ";
	*td.buffer2  << newURL ;
	*td.buffer2  << "\r\nContent-length: 0\r\n";

	if(connection && !stringcmpi(connection->value->c_str(), "keep-alive"))
	{
		*td.buffer2 << "Connection: keep-alive\r\n";	
	}
	*td.buffer2<< "Date: ";
	getRFC822GMTTime(time, HTTP_RESPONSE_DATE_DIM);
	*td.buffer2 << time ;
	*td.buffer2 << "\r\n\r\n";
	if(td.connection->socket.send(td.buffer2->getBuffer(), 
												   (int)td.buffer2->getLength(), 0) == -1)
		return 0;

	return 1;
}

/*!
 *Send a non-modified message to the client.
 */
int Http::sendHTTPNonModified()
{
	string time;
	HttpRequestHeader::Entry *connection = td.request.other.get("Connection");

	td.response.httpStatus=304;
	td.buffer2->setLength(0);
	*td.buffer2 << "HTTP/1.1 304 Not Modified\r\nAccept-Ranges: bytes\r\n"
                  "Server: MyServer " ;

	*td.buffer2 << versionOfSoftware  ;
	*td.buffer2 <<  "\r\n";

	if(connection && !stringcmpi(connection->value->c_str(), "keep-alive"))
	{
		*td.buffer2 << "Connection: keep-alive\r\n";	
	}	
	*td.buffer2 << "Date: ";
	
	getRFC822GMTTime(time, HTTP_RESPONSE_DATE_DIM);
	*td.buffer2 << time << "\r\n\r\n";

	if(td.connection->socket.send(td.buffer2->getBuffer(), 
                    (int)td.buffer2->getLength(), 0) == -1)
		return 0;
	return 1;
}

/*!
 *Send a 401 error.
 */
int Http::sendAuth()
{
	if(td.connection->getnTries() > 2)
	{
		return raiseHTTPError(e_401);
	}
	else
	{	
		td.connection->incnTries();
		return raiseHTTPError(e_401AUTH);
	}
}

/*!
 *Load the HTTP protocol.
 */
int Http::loadProtocol(XmlParser* languageParser)
{
  const char *mainConfigurationFile;
  char *data;
  int  nDefaultFilename = 0;
	XmlParser configurationFileManager;
  if(initialized)
		return 0;

  mainConfigurationFile = Server::getInstance()->getMainConfFile();

  secCacheMutex.init();
		
	/*! 
   *Store defaults value.  
   *By default use GZIP with files bigger than a MB.  
   */
  cgiTimeout = MYSERVER_SEC(15);
  fastcgiServers = 25;
  fastcgiInitialPort = 3333;
	gzipThreshold=1<<20;
	browseDirCSSpath.assign("");

	configurationFileManager.open(mainConfigurationFile);

	/*! Initialize ISAPI.  */
	Isapi::load(&configurationFileManager);
	
	/*! Initialize FastCGI.  */
	FastCgi::load(&configurationFileManager);	

	/*! Load the MSCGI library.  */
	mscgiLoaded = MsCgi::load(&configurationFileManager) ? 0 : 1;
	if(mscgiLoaded)
  {
		Server::getInstance()->logWriteln(languageParser->getValue("MSG_LOADMSCGI") );
  }
	else
	{
		Server::getInstance()->logPreparePrintError();
		Server::getInstance()->logWriteln(languageParser->getValue("ERR_LOADMSCGI") );
		Server::getInstance()->logEndPrintError();
	}
  HttpFile::load(&configurationFileManager);
  HttpDir::load(&configurationFileManager);

  dynCmdManager.loadMethods(0, languageParser, Server::getInstance());

  dynManagerList.loadManagers(0, languageParser, Server::getInstance());
	
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

  defaultFilename.clear();
  
	/*!
   *Copy the right values in the buffer.
   */
	if(nDefaultFilename==0)
	{
    defaultFilename.push_back("default.html");
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
        defaultFilename.push_back(data);
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

   dynCmdManager.clean();
   dynManagerList.clean();
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
  return defaultFilename[ID].c_str();
}

/*!
 *Returns the name of the protocol. If an out buffer 
 *is defined fullfill it with the name too.
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
