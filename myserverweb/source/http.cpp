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

#undef min
#define min( a, b )( ( a < b ) ? a : b  )

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

/*! Path to the .css file used by directory browsing.  */
char *Http::browseDirCSSpath=0;

/*! Threshold value to send data in gzip.  */
u_long Http::gzip_threshold=0;

/*!Use files for HTTP errors?  */
int Http::useMessagesFiles=0;

/*! Array with default filenames.  */
char *Http::defaultFilename=0;

/*! Number of the elements in the array.  */
u_long Http::nDefaultFilename=0;

/*! Is the HTTP protocol loaded?  */
int Http::initialized=0;

/*! If not specified differently use a timeout of 15 seconds.  */
int Http::cgi_timeout=MYSERVER_SEC(15);

/*! Max number of FastCGI servers allowed to run. */
int Http::fastcgi_servers;

/*! Cache for security files. */
SecurityCache Http::sec_cache;

/*! Access the security cache safely. */
Mutex Http::sec_cache_mutex;

/*!
 *Build a response for an OPTIONS request.
 */
int Http::optionsHTTPRESOURCE(HttpThreadContext* td, ConnectionPtr s, 
                              char* /*filename*/, int /*yetmapped*/)
{
	int ret;
	char time[HTTP_RESPONSE_DATE_DIM];
	getRFC822GMTTime(time, HTTP_RESPONSE_DATE_DIM);
	td->buffer2->SetLength(0);
	*td->buffer2 <<  "HTTP/1.1 200 OK\r\n";
	*td->buffer2 << "Date: " << time ;
	*td->buffer2 <<  "\r\nServer: MyServer "  << versionOfSoftware ;
	*td->buffer2 << "\r\nConnection:" << td->request.CONNECTION ;
	*td->buffer2 <<"\r\nContent-length: 0\r\nAccept-Ranges: bytes\r\n";
	*td->buffer2 << "Allow: OPTIONS, GET, POST, HEAD, DELETE, PUT";

	/*!
   *Check if the TRACE command is allowed on the virtual host.
   */
	if(allowHTTPTRACE(td, s))
		*td->buffer2 << ", TRACE\r\n\r\n";
	else
		*td->buffer2 << "\r\n\r\n";
	
	/*! Send the HTTP header.  */
	ret = s->socket.send(td->buffer2->GetBuffer(), 
                       (u_long)td->buffer2->GetLength(), 0);
	if( ret == SOCKET_ERROR )
	{
		return 0;
	}
	return 1;
}

/*!
 *Handle the HTTP TRACE command.
 */
int Http::traceHTTPRESOURCE(HttpThreadContext* td, ConnectionPtr s, 
                            char* /*filename*/, int /*yetmapped*/)
{
	int ret;
	char tmpStr[12];
	int content_len=(int)td->nHeaderChars;
	char time[HTTP_RESPONSE_DATE_DIM];
	getRFC822GMTTime(time, HTTP_RESPONSE_DATE_DIM);
	if(!allowHTTPTRACE(td, s))
		return raiseHTTPError(td, s, e_401);
	td->buffer2->SetLength(0);
	*td->buffer2 <<  "HTTP/1.1 200 OK\r\n";
	*td->buffer2 << "Date: " << time ;
	*td->buffer2 <<  "\r\nServer: MyServer "  << versionOfSoftware ;
	*td->buffer2 << "\r\nConnection:" << td->request.CONNECTION ;
	*td->buffer2 <<"\r\nContent-length:" << CMemBuf::IntToStr(content_len, tmpStr, 12) 
               << "\r\nContent-Type: message/http\r\nAccept-Ranges: bytes\r\n\r\n";
	
	/*! Send our HTTP header.  */
	ret = s->socket.send(td->buffer2->GetBuffer(), 
                       (u_long)td->buffer2->GetLength(), 0);
	if( ret == SOCKET_ERROR )
	{
		return 0;
	}
	
	/*! Send the client request header as the HTTP body.  */
	ret = s->socket.send(td->buffer->GetBuffer(), content_len, 0);
	if(ret == SOCKET_ERROR)
	{
		return 0;
	}
	return 1;

}

/*!
 *Check if the host allows the HTTP TRACE command
 */
int Http::allowHTTPTRACE(HttpThreadContext* td, ConnectionPtr s)
{
	int ret;
	/*! Check if the host allows HTTP trace.  */
	char *filename;
  char *http_trace_value;
	XmlParser parser;
  filename = new char[strlen(((Vhost*)(s->host))->documentRoot) + 10];
  if(filename==0)
  {
    return sendHTTPhardError500(td, s);
  }
	sprintf(filename, "%s/security", ((Vhost*)(s->host))->documentRoot);
	if(parser.open(filename))
	{
		return 0;
	}
  delete []filename;
  filename = 0;
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
  return cgi_timeout;
}

/*!
 *Main function to handle the HTTP PUT command.
 */
int Http::putHTTPRESOURCE(HttpThreadContext* td, ConnectionPtr s, 
                          char *filename, int, int, int yetmapped)
{
  u_long firstByte = td->request.RANGEBYTEBEGIN; 
  int permissions=-1;
  char *directory=0;
	int httpStatus=td->response.httpStatus;
	int keepalive=0;
  int directoryLen=0;
  int permissions2=0;
  char auth_type[16];	
	HttpHeaders::buildDefaultHTTPResponseHeader(&td->response);
	if(!lstrcmpi(td->request.CONNECTION, "Keep-Alive"))
	{
		strcpy(td->response.CONNECTION, "Keep-Alive");
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
    if(td->filenamePath)
      delete [] td->filenamePath;
    td->filenamePath = 0;
    td->filenamePath = new char[strlen(filename)+1];
    if(td->filenamePath == 0)
      return sendHTTPhardError500(td, s);
		strcpy(td->filenamePath, filename);
	}
	else
	{
		/*! 
     *If the client try to access files that aren't in the web directory 
     *send a 401 error.  
     */
		translateEscapeString(filename);
		if((filename[0] != '\0') && 
       (File::getPathRecursionLevel(filename)<1))
		{
			return raiseHTTPError(td, s, e_401);
		}
    if(td->filenamePath)
    {
      delete [] td->filenamePath;
      td->filenamePath = 0;
    }
		getPath(td, s, &(td->filenamePath), filename, 0);
	}
	if(File::isDirectory(td->filenamePath))
  {
    int directory_size = strlen(td->filenamePath);
    directory = new char[directory_size + 1];
    if(directory == 0)
    {
      delete [] td->filenamePath;
      td->filenamePath = 0;
      return sendHTTPhardError500(td, s);
    }
		strncpy(directory, td->filenamePath, directory_size);
  }
	else
  {
		File::splitPathLength(td->filenamePath, &directoryLen, 0);
    directory = new char[directoryLen + 1];
    if(directory == 0)
    {
      delete [] td->filenamePath;
      td->filenamePath = 0;
      return sendHTTPhardError500(td, s);
    }
		File::splitPath(td->filenamePath, directory, filename);
  }
	if(s->protocolBuffer==0)
	{
		s->protocolBuffer=new HttpUserData;
		if(!s->protocolBuffer)
    {
      delete [] directory;
      delete [] td->filenamePath;
      td->filenamePath = 0;
			return sendHTTPhardError500(td, s);
    }
		((HttpUserData*)(s->protocolBuffer))->reset();
	}
	if(td->request.AUTH[0])
  {
    sec_cache_mutex.lock();
		permissions=sec_cache.getPermissionMask(s->getLogin(), s->getPassword(), directory,
                                            filename, ((Vhost*)(s->host))->systemRoot, 
                                      ((HttpUserData*)s->protocolBuffer)->needed_password,
                                            auth_type, 16, &permissions2);
    sec_cache_mutex.unlock();  
  }
	else/*!The default user is Guest with a null password*/
  {
    sec_cache_mutex.lock();
		permissions=sec_cache.getPermissionMask("Guest", "", directory, filename, 
                                  ((Vhost*)(s->host))->systemRoot, 
                                  ((HttpUserData*)s->protocolBuffer)->needed_password
                                  , auth_type, 16);
    sec_cache_mutex.unlock();
  }

	/*! Check if we have to use digest for the current directory.  */
	if(!lstrcmpi(auth_type, "Digest"))
	{
		if(!lstrcmpi(td->request.AUTH, "Digest"))
		{
			if(!((HttpUserData*)s->protocolBuffer)->digest_checked)
				((HttpUserData*)s->protocolBuffer)->digest = checkDigest(td, s);
			((HttpUserData*)s->protocolBuffer)->digest_checked=1;
			if(((HttpUserData*)s->protocolBuffer)->digest==1)
			{
				strcpy(s->getPassword(), 
               ((HttpUserData*)s->protocolBuffer)->needed_password);

				permissions=permissions2;
			}
		}
		td->auth_scheme=HTTP_AUTH_SCHEME_DIGEST;
	}
	else/*! By default use the Basic authentication scheme. */
	{
		td->auth_scheme=HTTP_AUTH_SCHEME_BASIC;
	}	
	/*! If there are no permissions, use the Guest permissions.  */
	if(td->request.AUTH[0] && (permissions==0))
  {
    sec_cache_mutex.lock();
		permissions=sec_cache.getPermissionMask("Guest", "", directory, filename, 
                                            ((Vhost*)(s->host))->systemRoot, 
                         ((HttpUserData*)s->protocolBuffer)->needed_password, 
                                            auth_type, 16);		
    sec_cache_mutex.unlock();
  }
  delete [] directory;

	if(!(permissions & MYSERVER_PERMISSION_WRITE))
	{
    delete [] td->filenamePath;
    td->filenamePath = 0;
		return sendAuth(td, s);
	}
	if(File::fileExists(td->filenamePath))
	{
		/*! If the file exists update it.  */
		File file;
		if(file.openFile(td->filenamePath, FILE_OPEN_IFEXISTS | 
                     FILE_OPEN_WRITE))
		{
      delete [] td->filenamePath;
      td->filenamePath = 0;
			/*! Return an internal server error.  */
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
        delete [] td->filenamePath;
        td->filenamePath = 0;
        /*! Return an internal server error. */
				return raiseHTTPError(td, s, e_500);
			}
			if(nbr)
			{
				if(file.writeToFile(td->buffer->GetBuffer(), nbr, &nbw))
				{
					file.closeFile();
          delete [] td->filenamePath;
          td->filenamePath = 0;
          /*! Return an internal server error. */
					return raiseHTTPError(td, s, e_500);
				}
			}
			else
				break;
			if(nbw!=nbr)
			{
        delete [] td->filenamePath;
        td->filenamePath = 0;
				file.closeFile();
        /*! Internal server error. */
				return raiseHTTPError(td, s, e_500);
			}
		}
    delete [] td->filenamePath;
    td->filenamePath = 0;
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
		if(file.openFile(td->filenamePath, 
                      FILE_CREATE_ALWAYS|FILE_OPEN_WRITE))
    {
      delete [] td->filenamePath;
      td->filenamePath = 0;
      /*! Internal server error. */
			return raiseHTTPError(td, s, e_500);
    }
    delete [] td->filenamePath;
    td->filenamePath = 0;
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

/*!
 *Delete the resource identified by filename.
 */
int Http::deleteHTTPRESOURCE(HttpThreadContext* td, ConnectionPtr s, 
                             char *filename, int yetmapped)
{
  int permissions=-1;
  char *directory;
	int httpStatus=td->response.httpStatus;
	int permissions2=0;
	char auth_type[16];
	HttpHeaders::buildDefaultHTTPResponseHeader(&td->response);
	if(!lstrcmpi(td->request.CONNECTION, "Keep-Alive"))
	{
		strcpy(td->response.CONNECTION, "Keep-Alive");
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
    if(td->filenamePath)
      delete [] td->filenamePath;
    td->filenamePath = new char[strlen(filename)+1];
    if(td->filenamePath == 0)
      return sendHTTPhardError500(td, s);
		strcpy(td->filenamePath, filename);
	}
	else
	{
		/*!
     *If the client try to access files that aren't in the web directory 
     *send a HTTP 401 error page.
     */
		translateEscapeString(filename );
		if((filename[0] != '\0')&&(File::getPathRecursionLevel(filename)<1))
		{
			return raiseHTTPError(td, s, e_401);
		}
    if(td->filenamePath)
      delete [] td->filenamePath;
    td->filenamePath = 0;
		getPath(td, s, &(td->filenamePath), filename, 0);
	}
	if(File::isDirectory(td->filenamePath))
  {
    int directory_len = strlen(td->filenamePath)+1;
    directory = new char[directory_len];
    if(directory == 0)
    {
      delete [] td->filenamePath;
      td->filenamePath = 0;
      return sendHTTPhardError500(td, s);
    }
		strncpy(directory, td->filenamePath, directory_len);
  }
	else
  {
    int directory_len=0;
    File::splitPathLength(td->filenamePath, &directory_len, 0);
    directory = new char[directory_len];
    if(directory == 0)
    {
      delete [] td->filenamePath;
      td->filenamePath = 0;
      return sendHTTPhardError500(td, s);
    }
		File::splitPath(td->filenamePath, directory, 0);
  }

	if(s->protocolBuffer==0)
	{
		s->protocolBuffer=new HttpUserData;
		if(!s->protocolBuffer)
    {
      delete [] directory;
      delete [] td->filenamePath;
      td->filenamePath = 0;
			return 0;
    }
		((HttpUserData*)(s->protocolBuffer))->reset();
	}

	if(td->request.AUTH[0])
  {
    sec_cache_mutex.lock();
		permissions=sec_cache.getPermissionMask(s->getLogin(), s->getPassword(), directory,
                                            filename,((Vhost*)(s->host))->systemRoot, 
                               ((HttpUserData*)s->protocolBuffer)->needed_password,
                                            auth_type, 16, &permissions2);
    sec_cache_mutex.unlock();
  }
	else/*!The default user is Guest with a null password*/
  {
    sec_cache_mutex.lock();
		permissions=sec_cache.getPermissionMask("Guest", "", directory, filename, 
                               ((Vhost*)(s->host))->systemRoot, 
                               ((HttpUserData*)s->protocolBuffer)->needed_password,
                                  auth_type, 16);
    sec_cache_mutex.unlock();
	}	
  /*! Check if we have to use digest for the current directory. */
	if(!lstrcmpi(auth_type, "Digest"))
	{
		if(!lstrcmpi(td->request.AUTH, "Digest"))
		{
			if(!((HttpUserData*)s->protocolBuffer)->digest_checked)
				((HttpUserData*)s->protocolBuffer)->digest = checkDigest(td, s);
			((HttpUserData*)s->protocolBuffer)->digest_checked=1;
			if(((HttpUserData*)s->protocolBuffer)->digest==1)
			{
				strncpy(s->getPassword(), 
               ((HttpUserData*)s->protocolBuffer)->needed_password, 32);

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
	/*If there are no permissions, use the Guest permissions*/
	if(td->request.AUTH[0] && (permissions==0))
  {
    sec_cache_mutex.lock();
		permissions=sec_cache.getPermissionMask("Guest", "", directory, filename, 
                                  ((Vhost*)(s->host))->systemRoot, 
                                ((HttpUserData*)s->protocolBuffer)->needed_password,
                                 auth_type, 16);	
    sec_cache_mutex.unlock();
  }
	if(!(permissions & MYSERVER_PERMISSION_DELETE))
	{
    delete [] directory;
    delete [] td->filenamePath;
    td->filenamePath = 0;
		return sendAuth(td, s);
	}
	if(File::fileExists(td->filenamePath))
	{
    delete [] directory;
    delete [] td->filenamePath;
    td->filenamePath = 0;
		File::deleteFile(td->filenamePath);
    /*! Successful deleted. */
		return raiseHTTPError(td, s, e_202);
	}
	else
	{
    delete [] directory;
    delete [] td->filenamePath;
    td->filenamePath = 0;
    /*! No content. */
		return raiseHTTPError(td, s, e_204);
	}
}

/*!
 *Check the Digest authorization
 */
u_long Http::checkDigest(HttpThreadContext* td, ConnectionPtr s)
{
	char A1[48];
	char A2[48];
	char response[48];
	char *method;
	char *uri;
	u_long digest_count;
  Md5 md5;
  /*! Return 0 if the password is different. */
	if(td->request.digest_opaque[0]&& lstrcmp(td->request.digest_opaque,
       ((HttpUserData*)s->protocolBuffer)->opaque))
		return 0;
  /*! If is not equal return 0. */
	if(lstrcmp(td->request.digest_realm, ((HttpUserData*)s->protocolBuffer)->realm))
		return 0;
	
	digest_count = hexToInt(td->request.digest_nc);
	
	if(digest_count != ((HttpUserData*)s->protocolBuffer)->nc+1)
		return 0;
	else
		((HttpUserData*)s->protocolBuffer)->nc++;
   
	md5.init();
	td->buffer2->SetLength(0);
	*td->buffer2 << td->request.digest_username << ":" << td->request.digest_realm 
               << ":" << ((HttpUserData*)s->protocolBuffer)->needed_password;

	md5.update((unsigned char const*)td->buffer2->GetBuffer(), (unsigned int)td->buffer2->GetLength());
	md5.end(A1);
	
	md5.init();
	method=td->request.CMD;
	uri=td->request.URIOPTS;
	if(td->request.digest_uri[0])
		uri=td->request.digest_uri;
	td->buffer2->SetLength(0);
	*td->buffer2 << method << ":" << uri;
	md5.update((unsigned char const*)td->buffer2->GetBuffer(), (unsigned int)td->buffer2->GetLength());
	md5.end( A2);
	
	md5.init();
	td->buffer2->SetLength(0);
	*td->buffer2 << A1 << ":"  << ((HttpUserData*)s->protocolBuffer)->nonce << ":" 
               << td->request.digest_nc << ":"  << td->request.digest_cnonce << ":" 
               << td->request.digest_qop  << ":" << A2;
	md5.update((unsigned char const*)td->buffer2->GetBuffer(), (unsigned int)td->buffer2->GetLength());
	md5.end(response);	

	if(!lstrcmp(response, td->request.digest_response))
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
	digest_checked=0;
	needed_password[0]='\0';
	nc=0;
	digest=0;
}


/*!
 *Main function to send a resource to a client.
 */
int Http::sendHTTPResource(HttpThreadContext* td, ConnectionPtr s, char *URI, 
                           int systemrequest, int only_header, int yetmapped)
{
	/*!
   *With this code we manage a request of a file or a directory or anything 
   *that we must send over the HTTP.
   */
	char* filename;
	int keepalive;
  char ext[10];
  int permissions;
  int permissions2;
  char* dirscan;
	int filenamePathLen;
  char *data = 0;
	int mimeCMD;
	time_t lastMT;
  int ret;
  if(URI == 0)
    return sendHTTPhardError500(td, s);
  filename=new char[strlen(URI)+1];
  if(filename == 0)
    return sendHTTPhardError500(td, s);
	strcpy(filename, URI);
	td->buffer->SetLength(0);
	
	HttpHeaders::buildDefaultHTTPResponseHeader(&td->response);	
	keepalive=0;
	if(!lstrcmpi(td->request.CONNECTION, "Keep-Alive"))
	{
		strcpy(td->response.CONNECTION, "Keep-Alive");
		keepalive=1;
	}
	/*!
   *td->filenamePath is the file system mapped path while filename 
   *is the URI requested. systemrequest is 1 if the file is in 
   *the system directory.
   *If filename is already mapped on the file system don't map it again.
   */
	if(yetmapped)
	{
    if(td->filenamePath)
      delete [] td->filenamePath;
    td->filenamePath = new char[strlen(filename)+1];
    if(td->filenamePath == 0)
      return sendHTTPhardError500(td, s);
		strcpy(td->filenamePath, filename);
	}
	else
	{
		/*!
     *If the client try to access files that aren't in the 
     *web directory send a 401 error.
     */
		translateEscapeString(filename );
		if((filename[0] != '\0')&&
       (File::getPathRecursionLevel(filename)<1))
		{
      delete [] filename;
      if(td->filenamePath)
        delete [] td->filenamePath;   
      td->filenamePath = 0;
			return raiseHTTPError(td, s, e_401);
		}
    if(td->filenamePath)
      delete [] td->filenamePath;
    td->filenamePath = 0;
    /*! getPath will alloc the buffer for filenamePath. */
		getPath(td, s, &(td->filenamePath), filename, systemrequest);
	}
  /*! By default allows only few actions. */
	permissions= MYSERVER_PERMISSION_READ |  MYSERVER_PERMISSION_BROWSE ;
	if(!systemrequest)
	{
		char* directory;
		char auth_type[16];
		if(File::isDirectory(td->filenamePath))
    {
      directory = new char[strlen(td->filenamePath) + 1];
      if(directory == 0)
      {
        delete [] td->filenamePath;
        delete [] filename;
        td->filenamePath = 0;
        return sendHTTPhardError500(td, s);
      }
			strcpy(directory, td->filenamePath);
    }
		else
    {
      int directorylen=0;
      File::splitPathLength(td->filenamePath, &directorylen, 0);
      directory = new char[directorylen];
      if(directory == 0)
      {
        delete [] td->filenamePath;
        delete [] filename;
        td->filenamePath = 0;
        return sendHTTPhardError500(td, s);
      }
			File::splitPath(td->filenamePath, directory, filename);
    }

		if(s->protocolBuffer==0)
		{
			s->protocolBuffer=new HttpUserData;
			if(!s->protocolBuffer)
      {
        delete []  filename;
				return sendHTTPhardError500(td, s);
      }
			((HttpUserData*)s->protocolBuffer)->reset();
		}
		permissions2=0;
		if(td->request.AUTH[0])
    {
      sec_cache_mutex.lock();
			permissions=sec_cache.getPermissionMask(s->getLogin(), s->getPassword(), 
                                              directory, filename,
                                              ((Vhost*)(s->host))->systemRoot,
                                 ((HttpUserData*)s->protocolBuffer)->needed_password,
                                    auth_type, 16, &permissions2);
      sec_cache_mutex.unlock();
    }
		else/*!The default user is Guest with a null password*/
    {
      sec_cache_mutex.lock();
			permissions=sec_cache.getPermissionMask("Guest", "", directory, filename,
                                  ((Vhost*)(s->host))->systemRoot,
                                 ((HttpUserData*)s->protocolBuffer)->needed_password,
                                  auth_type, 16);
      sec_cache_mutex.unlock();
		}	
    /*! Check if we have to use digest for the current directory. */
		if(!lstrcmpi(auth_type, "Digest"))
		{
			if(!lstrcmpi(td->request.AUTH, "Digest"))
			{
				if(!((HttpUserData*)s->protocolBuffer)->digest_checked)
					((HttpUserData*)s->protocolBuffer)->digest = checkDigest(td, s);
				((HttpUserData*)s->protocolBuffer)->digest_checked=1;
				if(((HttpUserData*)s->protocolBuffer)->digest==1)
				{
					strcpy(s->getPassword(), 
                 ((HttpUserData*)s->protocolBuffer)->needed_password);

					permissions=permissions2;
				}
			}
			td->auth_scheme=HTTP_AUTH_SCHEME_DIGEST;
		}
		else/*! By default use the Basic authentication scheme. */
		{
			td->auth_scheme=HTTP_AUTH_SCHEME_BASIC;
		}	
		/*!If there are no permissions, use the Guest permissions. */
		if(td->request.AUTH[0] && (permissions==0))
    {
      sec_cache_mutex.lock();
			permissions=sec_cache.getPermissionMask("Guest", "", directory, filename,
                                ((Vhost*)(s->host))->systemRoot,
                                ((HttpUserData*)s->protocolBuffer)->needed_password,
                                   auth_type, 16);
      sec_cache_mutex.unlock();
		}
    delete [] directory;
	}
	/*!
   *Get the PATH_INFO value.
   *Use dirscan as a buffer for put temporary directory scan.
   *When an '/' character is present check if the path up to '/' character
   *is a file. If it is a file send the rest of the URI as PATH_INFO.
   */
	dirscan=0;
  if(td->pathInfo)
    delete [] td->pathInfo;
	td->pathInfo=0;
  if(td->pathTranslated)
    delete [] td->pathTranslated;
	td->pathTranslated=0;
	filenamePathLen=(int)strlen(td->filenamePath)+1;
  dirscan=new char[filenamePathLen];
  if( dirscan == 0 )
    return sendHTTPhardError500(td, s);
	for(int i=0, len=0;i<filenamePathLen ;i++)
	{
		/*!
     *http://host/pathtofile/filetosend.php/PATH_INFO_VALUE?QUERY_INFO_VALUE
     *When a request has this form send the file filetosend.php with the
     *environment string PATH_INFO equals to PATH_INFO_VALUE and QUERY_INFO
     *to QUERY_INFO_VALUE.
     */

    /*!
     *If there is the '/' character check if dirscan is a file.
     */
		if(i && (td->filenamePath[i]=='/'))
		{
      /*!
       *If the token is a file.
       */
			if(!File::isDirectory(dirscan))
			{
        if(td->pathInfo)
          delete [] td->pathInfo;
        td->pathInfo = new char[strlen(&td->filenamePath[len])+1];
        if(td->pathInfo == 0)
        {
          delete [] dirscan;
          if(td->filenamePath)
            delete [] td->filenamePath;
          td->filenamePath = 0;
          delete [] filename;
          return sendHTTPhardError500(td, s);
        }
        strcpy(td->pathInfo, &td->filenamePath[len]);
        if(td->filenamePath)
          delete [] td->filenamePath;
        td->filenamePath = new char[strlen(dirscan)+1];
        if(td->filenamePath == 0)
        {
          delete [] dirscan;
          delete [] td->pathInfo;
          return sendHTTPhardError500(td, s);
        }
				strcpy(td->filenamePath, dirscan);
				break;
			}
		}
		if(len+1 < filenamePathLen)
		{
			dirscan[len++]=(td->filenamePath)[i];
			dirscan[len]='\0';
		}
	}
	delete []  dirscan;
	/*!
   *If there is a PATH_INFO value the get the PATH_TRANSLATED too.
   *PATH_TRANSLATED is the mapped to the local filesystem version of PATH_INFO.
   */
	if(td->pathInfo)
	{
    int ptlen;
   	if(td->pathTranslated)
      delete [] td->pathTranslated;
    td->pathTranslated = 0;
		/*!
     * Start from the second character because the first is a
     * slash character.  
     */
		ptlen = getPath(td, s, &(td->pathTranslated), &((td->pathInfo)[1]), 0);
		File::completePath(&(td->pathTranslated), &ptlen );
	}
	else
	{
    td->pathTranslated=0;
	}
	File::completePath(&(td->filenamePath), &filenamePathLen);

	/*!
   *  If there are not any extension then we do one of this in order:
   *	1)We send the default files in the directory in order.
   *	2)We send the directory content.
   *	3)We send an error.
   */
	if(File::isDirectory((char *)(td->filenamePath)))
	{
		int i;
		if(!(permissions & MYSERVER_PERMISSION_BROWSE))
		{
      delete []  filename;
      delete []  td->filenamePath;
      td->filenamePath = 0;
      if(td->pathTranslated)
      {
        delete [] td->pathTranslated;
        td->pathTranslated = 0;
      }
			return sendAuth(td, s);
		}
		for(i=0;;i++)
    {
			char *defaultFileNamePath=getDefaultFilenamePath(i);
			char *defaultFileName;
      
      if(defaultFileNamePath)
      {
        defaultFileName=new char[strlen(td->filenamePath)+
                               strlen(defaultFileNamePath)+ 2 ];
        if(defaultFileName == 0)
          return sendHTTPhardError500(td, s);
      }
			if(defaultFileNamePath)
      {
				sprintf(defaultFileName, "%s/%s", td->filenamePath, 
                defaultFileNamePath);
      }
			else
      {
				break;
      }

			if(File::fileExists(defaultFileName))
			{
				char *nURL;
				if(td->request.uriEndsWithSlash)
        {
          nURL = new char[strlen(defaultFileNamePath)+1];
          if(nURL == 0)
          {
            delete [] defaultFileName;
            delete [] filename;
            if(td->pathTranslated)
            {
              delete [] td->pathTranslated;
              td->pathTranslated = 0;
            }
            return sendHTTPhardError500(td, s);
          }
					strcpy(nURL, defaultFileNamePath);
        }
				else
        {
					int last_slash_offset = (int) strlen(URI);
					while(last_slash_offset && URI[last_slash_offset]!='/')
						--last_slash_offset;
          
          nURL = new char[strlen(defaultFileNamePath)+
                          strlen(&URI[last_slash_offset+1])+2];
          if(nURL == 0)
          {
            delete [] defaultFileName;
            delete [] filename;
            return sendHTTPhardError500(td, s);
          }
 					sprintf(nURL, "%s/%s",&URI[last_slash_offset  ? last_slash_offset+1 : 0], 
                  defaultFileNamePath);
				}
 				/*! Send a redirect to the new location.  */
				if(sendHTTPRedirect(td, s, nURL))
					ret = keepalive;
				else
					ret = 0;
        delete [] nURL;
        delete [] defaultFileName;
        delete [] filename;
        return ret;
			}
      delete [] defaultFileName;
    }
    delete [] filename;
		return lhttp_dir.send(td, s, td->filenamePath, 0, only_header) & keepalive;
	}

  delete [] filename;

	if(!File::fileExists(td->filenamePath))
		return raiseHTTPError(td, s, e_404);

	/*!
   *getMIME returns the type of command registered by the extension.
   */
  data = 0;
	mimeCMD=getMIME(td, td->response.CONTENT_TYPE, td->filenamePath, ext, &data);

	if(mimeCMD==CGI_CMD_RUNCGI)
	{
		if(!(permissions & MYSERVER_PERMISSION_EXECUTE))
		{
      if(data)
        delete [] data;
			return sendAuth(td, s);
		}
    ret = lcgi.send(td, s, td->filenamePath, data, 0,  only_header);
    if(data)
      delete [] data;
    return (ret & keepalive);
	}else	if(mimeCMD==CGI_CMD_EXECUTE )
	{
		if(!(permissions & MYSERVER_PERMISSION_EXECUTE))
		{
      if(data)
        delete [] data;
			return sendAuth(td, s);
		}
    ret = lcgi.send(td, s, td->filenamePath, data, 1, only_header);
    if(data)
      delete [] data;
    return (ret & keepalive);
	}
  else if(mimeCMD == CGI_CMD_RUNISAPI)
	{
		if(!(permissions & MYSERVER_PERMISSION_EXECUTE))
		{
      if(data)
        delete [] data;
			return sendAuth(td, s);
		}
		ret = lisapi.send(td, s, td->filenamePath, data, 0, only_header);
    if(data)
      delete [] data;
    return (ret & keepalive);

	}
  else if(mimeCMD==CGI_CMD_EXECUTEISAPI)
	{
		if(!(permissions & MYSERVER_PERMISSION_EXECUTE))
		{
      if(data)
        delete [] data;
			return sendAuth(td, s);
		}
  	ret = lisapi.send(td, s, td->filenamePath, data, 1, only_header);
    if(data)
      delete [] data;
    return (ret & keepalive);
	}
	else if( mimeCMD == CGI_CMD_RUNMSCGI )
	{
		char* target;
		if(!(permissions & MYSERVER_PERMISSION_EXECUTE))
		{
      if(data)
        delete [] data;
			return sendAuth(td, s);
		}
		if(td->request.URIOPTSPTR)
			target=td->request.URIOPTSPTR;
		else
			target=td->request.URIOPTS;
    /*! Check if the MSCGI library is loaded. */
		if(mscgiLoaded)
		{
      if(data)
        delete [] data;
			ret=lmscgi.send(td, s, td->filenamePath, target, 1, only_header);
      return (ret & keepalive);
    }
    if(data)
      delete [] data;
		return raiseHTTPError(td, s, e_500);
	}
  else if( mimeCMD == CGI_CMD_EXECUTEWINCGI )
	{
		char* cgipath=0;
		if(!(permissions & MYSERVER_PERMISSION_EXECUTE))
		{
      if(data)
        delete [] data;
			return sendAuth(td, s);
		}
		if(data)
    {
      cgipath = new char[strlen(data)+strlen(td->filenamePath)+4];
      if(cgipath==0)
      {
        if(data)
          delete [] data;
        return sendHTTPhardError500(td, s);
      }
			sprintf(cgipath, "%s \"%s\"", data, td->filenamePath);
    }
		else
    {
      cgipath = new char[strlen(td->filenamePath)+1];
      if(cgipath==0)
      {
        if(data)
          delete [] data;
        return sendHTTPhardError500(td, s);
      }
			sprintf(cgipath, "%s", td->filenamePath);
    }
    if(data)
      delete [] data;
    ret=lwincgi.send(td, s, cgipath, 1, only_header);
		if(cgipath)
	      delete [] cgipath;
		return (ret&keepalive);

	}
	else if( mimeCMD == CGI_CMD_RUNFASTCGI )
	{
  	if(!(permissions & MYSERVER_PERMISSION_EXECUTE))
		{
      if(data)
        delete [] data;
			return sendAuth(td, s);
		}	
		ret = lfastcgi.send(td, s, td->filenamePath, data, 0, only_header);
    if(data)
      delete [] data;
		return (ret & keepalive);
	}
	else if(mimeCMD==CGI_CMD_EXECUTEFASTCGI)
	{
    if(!(permissions & MYSERVER_PERMISSION_EXECUTE))
		{
      if(data)
        delete [] data;
			return sendAuth(td, s);
		}
		ret = lfastcgi.send(td, s, td->filenamePath, data, 1, only_header);
    if(data)
      delete [] data;
		return (ret & keepalive);
	}
	else if( mimeCMD == CGI_CMD_SENDLINK )
	{
		u_long nbr;
		char* linkpath;
		char* pathInfo;
    int linkpathSize;
		if(!(permissions & MYSERVER_PERMISSION_READ))
		{
      if(data)
        delete [] data;
			return sendAuth(td, s);
		}
		File h;
		if(h.openFile(td->filenamePath, 
                   FILE_OPEN_IFEXISTS|FILE_OPEN_READ))
    {
      if(data)
        delete [] data;
			return raiseHTTPError(td, s, e_500);/*!Internal server error*/
    }
    linkpathSize = h.getFileSize()+strlen(td->pathInfo) +1;
    linkpath=new char[linkpathSize];
    if(linkpath==0)
    {
      if(data)
        delete [] data;   
      return sendHTTPhardError500(td, s);
    }
		if(h.readFromFile(linkpath, linkpathSize , &nbr))
		{
			h.closeFile();
      if(data)
        delete [] data;
      delete [] linkpath;
			return raiseHTTPError(td, s, e_500);/*!Internal server error*/
		}
		h.closeFile();
		linkpath[nbr]='\0';
    pathInfo=new char[strlen(td->pathInfo)+1];
    if(pathInfo == 0)
    {
      if(data)
        delete [] data;
      delete [] linkpath;
			return raiseHTTPError(td, s, e_500);/*!Internal server error*/
    }
		strcpy(pathInfo, td->pathInfo);
		translateEscapeString(pathInfo);
		strcat(linkpath, pathInfo);
		if(nbr)
			ret = sendHTTPResource(td, s, linkpath, systemrequest, only_header, 1);
		else
			ret = raiseHTTPError(td, s, e_404);
    delete [] linkpath;
    delete [] pathInfo;
    if(data)
      delete [] data;
    return (ret & keepalive);
	}
  
  if(data)
    delete [] data;

	if(!(permissions & MYSERVER_PERMISSION_READ))
	{     
		return sendAuth(td, s);
	}

	lastMT=File::getLastModTime(td->filenamePath);
	if(lastMT==-1)
  {
		return raiseHTTPError(td, s, e_500);
  }
	getRFC822GMTTime(lastMT, td->response.LAST_MODIFIED, 
                   HTTP_RESPONSE_LAST_MODIFIED_DIM);
	if(td->request.IF_MODIFIED_SINCE[0])
	{
		if(!strcmp(td->request.IF_MODIFIED_SINCE, td->response.LAST_MODIFIED))
    {
			return sendHTTPNonModified(td, s);
    }
	}
  ret = lhttp_file.send(td, s, td->filenamePath, 0, only_header);
	return (keepalive & ret);
}
/*!
 *Log the access using the Common Log Format or the Combined one.
 */
int Http::logHTTPaccess(HttpThreadContext* td, ConnectionPtr a)
{
	char tmpStrInt[12];
	char time[HTTP_RESPONSE_DATE_DIM + 1];

	td->buffer2->SetLength(0);
	*td->buffer2 << a->getipAddr();
	*td->buffer2<< " ";
	
	if(td->identity[0])
		*td->buffer2 << td->identity;
	else
		*td->buffer2 << "- ";

	if(td->identity[0])
		*td->buffer2 << td->identity;
	else
		*td->buffer2 << "-";

	*td->buffer2 << " [";
	
	getRFC822GMTTime(time, HTTP_RESPONSE_DATE_DIM);
	*td->buffer2 <<  time  << "] \"";
	
  if(td->request.CMD[0])
    {
      *td->buffer2 << td->request.CMD << "";
    }

  if(td->request.CMD[0] || td->request.URI[0])
      *td->buffer2 << " ";

  if(td->request.URI[0] == '\0')
    *td->buffer2 <<  "/";
  else
  {
    *td->buffer2 << td->request.URI;
  }

  if(td->request.URIOPTS[0])
    {
      *td->buffer2 << "?" << td->request.URIOPTS;
    }
  sprintf(tmpStrInt, "%u ",td->response.httpStatus);
  
  if(td->request.VER[0])
    *td->buffer2 << " " << td->request.VER  ;


  *td->buffer2<< "\" " << tmpStrInt  << " ";
	
	if(td->response.CONTENT_LENGTH[0])
		*td->buffer2  << td->response.CONTENT_LENGTH;
	else
		*td->buffer2 << "0";
  if(strstr((((Vhost*)(a->host)))->accessLogOpt, "type=combined"))
  {
    *td->buffer2 << " "  << td->request.REFERER << " "  
                 << td->request.USER_AGENT;            
  }
	*td->buffer2 << "\r\n" <<end_str;
  /*!
   *Request the access to the log file then write then append the message.
   */
	((Vhost*)(a->host))->accesseslogRequestAccess(td->id);
	((Vhost*)(a->host))->accessesLogWrite(td->buffer2->GetBuffer());
	((Vhost*)(a->host))->accesseslogTerminateAccess(td->id);
	td->buffer2->SetLength(0);

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
  int cwdLen;
  u_long dataRead=0;
  u_long dataToRead=0;
  /*! Dimension of the POST data. */
	u_long content_len=0;
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
  td.only_header = 0;
	td.inputData.setHandle((FileHandle)0);
	td.outputData.setHandle((FileHandle)0);
	if(td.outputDataPath)
    delete [] td.outputDataPath;
	if(td.filenamePath)
    delete [] td.filenamePath;
  if(td.inputDataPath)
    delete [] td.inputDataPath;
	td.outputDataPath=0;
	td.inputDataPath=0;
  td.filenamePath=0;
	/*!
	 *Reset the request structure.
	 */
	HttpHeaders::resetHTTPRequest(&td.request);
	
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
    if(strcmp(td.request.VER, "HTTP/1.0"))
    {
      char* msg = "HTTP/1.1 100 Continue\r\n\r\n";
      wait(2);
			if( a->socket.bytesToRead() == 0) 
        {
          if(a->socket.send(msg, (int)strlen(msg), 0)==-1)
            return 0;/*Remove the connection from the list*/
        }
    }
		return 2;
	}
	
	/*! Be sure that we can handle the HTTP version. */
	if((!strcmp(td.request.VER, "HTTP/1.1")) && 
     (!strcmp(td.request.VER, "HTTP/1.0")))
	{	
		retvalue = raiseHTTPError(&td, a, e_505);
		logHTTPaccess(&td, a);
    /*! Remove the connection from the list. */
		return 0;
	}
		
	if(a->protocolBuffer)
		((HttpUserData*)a->protocolBuffer)->digest_checked=0;	
	
	/*!
	 *If the header is an invalid request send the correct error 
   *message to the client and return immediately.
	 */
	if(validRequest==0)
	{
		retvalue = raiseHTTPError(&td, a, e_400);
		logHTTPaccess(&td, a);
		return retvalue;
	}
  /*! if the URI is too long. */ 
	else if(validRequest==414)
	{
		retvalue = raiseHTTPError(&td, a, e_414);
		logHTTPaccess(&td, a);
		return retvalue;
	}
	/*! Do not use Keep-Alive over HTTP version older than 1.1. */
	if( strcmp(td.request.VER, "HTTP/1.1") )
	{
    /*! Check if we support the HTTP version specified. */
    if( strcmp(td.request.VER, "HTTP/1.0") &&  strcmp(td.request.VER, "HTTP/0.9") )
    {
      /*! Raise a version not supported error. */
      raiseHTTPError(&td, a, e_505);
      logHTTPaccess(&td, a);
      return 0;      
    }
		if(td.request.CONNECTION[0])
			strcpy(td.request.CONNECTION, "close");
	}

	/*!
	 *For methods that accept data after the HTTP header set the correct 
   *pointer and create a file containing the informations after the header.
	 */
  cwdLen = getdefaultwdlen();

  if(td.inputDataPath)
  {
    delete [] td.inputDataPath;
    td.inputDataPath=0;
  }

  if(td.outputDataPath)
  {
    delete [] td.outputDataPath;
    td.outputDataPath=0;
  }

  td.inputDataPath = new char [cwdLen + 20];
  if(td.inputDataPath==0)
    return sendHTTPhardError500(&td, a);

  td.outputDataPath = new char [cwdLen + 20];
  if(td.outputDataPath==0)
  {
    delete [] td.inputDataPath;
    td.inputDataPath=0;
    return sendHTTPhardError500(&td, a);
  }
  getdefaultwd(td.inputDataPath, cwdLen+1);
	sprintf(&td.inputDataPath[(u_long)strlen(td.inputDataPath)], 
          "/stdInFile_%u", (u_int)td.id);
	getdefaultwd(td.outputDataPath, cwdLen+1);
	sprintf(&td.outputDataPath[(u_long)strlen(td.outputDataPath)], 
          "/stdOutFile_%u", (u_int)td.id);
	if((!lstrcmpi(td.request.CMD, "POST"))||(!lstrcmpi(td.request.CMD, "PUT")))
	{
		if(td.request.CONTENT_TYPE[0]=='\0')
			strcpy(td.request.CONTENT_TYPE, "application/x-www-form-urlencoded");

		/*!
		 *Read POST data
		 */
		{
      u_long nbw=0;
			u_long total_nbr=0;
			u_long timeout;
      td.request.URIOPTSPTR=&(td.buffer->GetBuffer())[td.nHeaderChars];
			td.buffer->GetBuffer()[min(td.nBytesToRead, 
                                 td.buffer->GetRealLength()-1)]='\0';
			/*!
       *Create the file that contains the data posted.
       *This data is the stdin file in the CGI.
       */
			if(td.inputData.openFile(td.inputDataPath,FILE_CREATE_ALWAYS | 
                             FILE_OPEN_READ|FILE_OPEN_WRITE))
				return 0;
			nbw=0;
      total_nbr=min(td.nBytesToRead, 
                           td.buffer->GetRealLength()-1)-td.nHeaderChars;
			if(total_nbr)
      {
        if(td.inputData.writeToFile(td.request.URIOPTSPTR, total_nbr, &nbw))
        {
          delete [] td.inputDataPath;
          td.inputDataPath=0;
          delete [] td.outputDataPath;
          td.outputDataPath = 0;
          td.inputData.closeFile();
          return 0;
        }
      }
			content_len=atoi(td.request.CONTENT_LENGTH);
      
			/*!
       *If the connection is Keep-Alive be sure that the client specify the
       *HTTP CONTENT-LENGTH field.
       *If a CONTENT-ENCODING is specified the CONTENT-LENGTH 
       *is not always needed.
       */
			if(!lstrcmpi(td.request.CONNECTION, "Keep-Alive"))
			{
				if((td.request.CONTENT_ENCODING[0]=='\0') 
              && (td.request.CONTENT_LENGTH[0]=='\0'))
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
					return retvalue;
				}
			}
			/*!
       *If there are others bytes to read from the socket.
       */
			timeout=get_ticks();
			if((content_len)&&(content_len!=nbw))
			{
				int ret;
				u_long fs;
				do
				{
					ret=0;
					fs=td.inputData.getFileSize();
					while(get_ticks()-timeout<MYSERVER_SEC(5))
					{
						if(content_len==total_nbr)
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
						if((content_len>fs) && (td.connection->socket.bytesToRead()))
						{				
							u_long tr=min(content_len-total_nbr, 
                            td.buffer2->GetRealLength());

							ret=td.connection->socket.recv(td.buffer2->GetBuffer(), tr, 0);
							if(ret==-1)
							{
								td.inputData.closeFile();
								td.inputData.deleteFile(td.inputDataPath);
								return 0;
							}
							
							if(td.inputData.writeToFile(td.buffer2->GetBuffer(), (u_long)ret, &nbw))
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
				while(content_len!=total_nbr);

				fs=td.inputData.getFileSize();
				if(content_len!=fs)
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
					return retvalue;
				}
			}
			/*! If CONTENT-LENGTH is not specified read all the data. */
			else if(content_len==0)
			{
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
          wait(2);
				}
				while(content_len!=total_nbr);
				sprintf(td.response.CONTENT_LENGTH, "%u", 
                (u_int)td.inputData.getFileSize());

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
	if(!lstrcmpi(td.request.TRANSFER_ENCODING, "chunked"))
	{
		File newStdIn;
		char buffer[20];
		char c;
    u_long nbr;
		u_long bufferlen;
    char *newfilename = new char[strlen(td.inputDataPath)+10];
    if(newfilename == 0)
    {
      td.inputData.closeFile();
			td.inputData.deleteFile(td.inputDataPath);
			return 0;
    }
		sprintf(newfilename, "%s_encoded", td.inputData.getFilename());
		if(newStdIn.openFile(td.inputDataPath, FILE_CREATE_ALWAYS | 
                         FILE_OPEN_READ|FILE_OPEN_WRITE))
		{
			td.inputData.closeFile();
			td.inputData.deleteFile(td.inputDataPath);
      delete [] newfilename;
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
					newStdIn.deleteFile(newfilename);
          delete [] newfilename;
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
        newStdIn.deleteFile(newfilename);
        delete [] newfilename;
				return 0;
			}
			dataToRead=(u_long)hexToInt(buffer);

			/*! The last chunk length is 0. */
			if(dataToRead==0)
				break;

			while(dataRead<dataToRead)
			{
				u_long nbw;
				if(td.inputData.readFromFile(td.buffer->GetBuffer(), min(dataToRead-dataRead, 
                                          td.buffer->GetRealLength()-1), &nbr))
				{
					td.inputData.closeFile();
					td.inputData.deleteFile(td.inputDataPath);
					newStdIn.closeFile();
          newStdIn.deleteFile(newfilename);
          delete [] newfilename;
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
          newStdIn.deleteFile(newfilename);
          delete [] newfilename;
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
		td.inputData.deleteFile(td.inputDataPath);
    delete[] td.inputDataPath;
    td.inputDataPath = newfilename;
		td.inputData=newStdIn;
    td.inputData.setFilePointer(0);
	}else	
	/*!
   *If is specified another Transfer Encoding not supported by the 
   *server send a 501 error.
   */
	if(strlen(td.request.TRANSFER_ENCODING))
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
  /*!If return value is not configured propertly.*/
	if(retvalue==-1)
	{
		/*!
     *How is expressly said in the rfc2616 a client that sends an 
     *HTTP/1.1 request MUST sends a Host header.
     *Servers MUST reports a 400 (Bad request) error if an HTTP/1.1
     *request does not include a Host request-header.
     */
		if((!strcmp(td.request.VER, "HTTP/1.1")) && td.request.HOST[0]==0)
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
			a->host=lserver->vhostList->getVHost(td.request.HOST, a->getlocalIpAddr(), 
                                           a->getLocalPort());
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
		
		if(!lstrcmpi(td.request.CONNECTION, "Keep-Alive")) 
		{
			/*!
			 *Support for HTTP pipelining.
			 */
			if(content_len==0)
			{
				/*!
         *connectionBuffer is 8 KB, so don't copy more bytes.
         */
				a->setDataRead(min(MYSERVER_KB(8), (int)strlen(td.buffer->GetBuffer()) -
                           td.nHeaderChars ) );
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
			retvalue=0;

		/*!
		 *Here we control all the HTTP commands.
		 */
		/*! GET REQUEST. */
		if(!lstrcmpi(td.request.CMD, "GET"))
		{
			if(!lstrcmpi(td.request.RANGETYPE, "bytes"))
				ret = sendHTTPResource(&td, a, td.request.URI, 0, 0);
			else
				ret = sendHTTPResource(&td, a, td.request.URI);
		}
		/*! POST REQUEST. */
		else if(!lstrcmpi(td.request.CMD, "POST"))
		{
			if(!lstrcmpi(td.request.RANGETYPE, "bytes"))
				ret = sendHTTPResource(&td, a, td.request.URI, 0, 0);
			else
				ret = sendHTTPResource(&td, a, td.request.URI);
		}
		/*! HEAD REQUEST. */
		else if(!lstrcmpi(td.request.CMD, "HEAD"))
		{
      td.only_header = 1;
			if(!lstrcmpi(td.request.RANGETYPE, "bytes"))
				ret = sendHTTPResource(&td, a, td.request.URI, 0, 1);
			else
				ret = sendHTTPResource(&td, a, td.request.URI, 0, 1);
		}
		/*! DELETE REQUEST. */
		else if(!lstrcmpi(td.request.CMD, "DELETE"))
		{
			ret = deleteHTTPRESOURCE(&td, a, td.request.URI, 0);
		}
		/*! PUT REQUEST. */
		else if(!lstrcmpi(td.request.CMD, "PUT"))
		{
			if(!lstrcmpi(td.request.RANGETYPE, "bytes"))
				ret = putHTTPRESOURCE(&td, a, td.request.URI, 0, 1);
			else
        ret = putHTTPRESOURCE(&td, a, td.request.URI, 0, 1);
		}
		/*! OPTIONS REQUEST. */
		else if(!lstrcmpi(td.request.CMD, "OPTIONS"))
		{
			ret = optionsHTTPRESOURCE(&td, a, td.request.URI, 0);
		}
		/*! TRACE REQUEST. */
		else if(!lstrcmpi(td.request.CMD, "TRACE"))
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
  ret &= (!strcmpi(td.request.CONNECTION,"keep-alive"));
	return ret? ((retvalue!=0) ?retvalue:1 ) :0;
}

/*!
 *Compute the Digest to out using a buffer.
 */
void Http::computeDigest(HttpThreadContext* td, char* out , char* buffer)
{
	Md5 md5;
	if(!out)
		return;
	sprintf(buffer, "%i-%u-%s", (int)clock(), (u_int)td->id, 
          td->connection->getipAddr());
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
  char time[HTTP_RESPONSE_DATE_DIM];
  char* errorFile;
  u_long lenErrorFile;
  td->lastError = ID;
	HttpHeaders::buildDefaultHTTPResponseHeader(&(td->response));
	if(!lstrcmpi(td->request.CONNECTION, "Keep-Alive"))
	{
		strcpy(td->response.CONNECTION, "Keep-Alive");
	}
	if(ID==e_401AUTH)
	{
		td->response.httpStatus = 401;
		td->buffer2->SetLength(0);
		*td->buffer2 << 
           "HTTP/1.1 401 Unauthorized\r\nAccept-Ranges: bytes\r\nServer: MyServer " ;
		*td->buffer2 << versionOfSoftware ;
		*td->buffer2 << "\r\nContent-type: text/html\r\nConnection:";
		*td->buffer2 <<td->request.CONNECTION;
		*td->buffer2 << "\r\nContent-length: 0\r\n";
		if(td->auth_scheme==HTTP_AUTH_SCHEME_BASIC)
		{
			*td->buffer2<<"WWW-Authenticate: Basic\r\n";
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
			strncpy(((HttpUserData*)a->protocolBuffer)->realm, td->request.HOST, 48);

			/*! Just a random string. */
			strncpy(&(md5_str[2]), td->request.URI, 256-2);
			md5_str[0]=(char)td->id;
			md5_str[1]=(char)clock();
			Md5 md5;
			md5.init();
			md5.update((unsigned char const*)md5_str,  (unsigned int)strlen(md5_str));
			md5.end(((HttpUserData*)a->protocolBuffer)->opaque);
			
			if(a->protocolBuffer && (!(((HttpUserData*)a->protocolBuffer)->digest)) || 
         (((HttpUserData*)a->protocolBuffer)->nonce[0]=='\0'))
			{
				computeDigest(td, ((HttpUserData*)a->protocolBuffer)->nonce, md5_str);
				((HttpUserData*)a->protocolBuffer)->nc=0;
			}
			*td->buffer2 << "WWW-Authenticate: Digest ";
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
		char *defFile = 0 ;
    int ret;
		td->response.httpStatus=getHTTPStatusCodeFromErrorID(ID);
    sec_cache_mutex.lock();
		ret = sec_cache.getErrorFileName(((Vhost*)a->host)->documentRoot, 
                                     getHTTPStatusCodeFromErrorID(ID),
                                     ((Vhost*)(a->host))->systemRoot, &defFile);
    sec_cache_mutex.unlock();
    if(ret == -1)
    {
      if(defFile)
      {
        delete [] defFile;
      }
      sendHTTPhardError500(td, a);
      return 0;
    }
    else if(ret)
    {
			/*!
       *Change the URI to reflect the default file name.
       */
			char nURL[HTTP_REQUEST_URI_DIM+6];
			int isPortSpecified=0;
			strcpy(nURL, protocolPrefix);
			strcat(nURL, td->request.HOST);
			for(int i=0;td->request.HOST[i];i++)
			{
				if(td->request.HOST[i]==':')
				{
					isPortSpecified = 1;
					break;
				}
			}
			if(!isPortSpecified)
				sprintf(&nURL[strlen(nURL)], ":%u", ((Vhost*)a->host)->port);
			if(nURL[strlen(nURL)-1]!='/')
				strcat(nURL, "/");
      if(defFile)
      {
        strcat(nURL, defFile);
        delete [] defFile;
      }
			return sendHTTPRedirect(td, a, nURL);
		}
	}
	getRFC822GMTTime(td->response.DATEEXP, HTTP_RESPONSE_DATEEXP_DIM);
	strncpy(td->response.ERROR_TYPE, HTTP_ERROR_MSGS[ID], HTTP_RESPONSE_ERROR_TYPE_DIM);
	lenErrorFile=(u_long)strlen(((Vhost*)(a->host))->systemRoot)+
                              (u_long)strlen(HTTP_ERROR_HTMLS[ID])+2;
	errorFile=new char[lenErrorFile];
	if(errorFile)
  {
    sprintf(errorFile, "%s/%s", ((Vhost*)(a->host))->systemRoot, 
            HTTP_ERROR_HTMLS[ID]);
    if(useMessagesFiles && File::fileExists(errorFile))
		{
			delete [] errorFile;
			return sendHTTPResource(td, a, HTTP_ERROR_HTMLS[ID], 1, td->only_header);
		}
		delete [] errorFile;
	}
	/*! Send the error over the HTTP. */
	sprintf(td->response.CONTENT_LENGTH, "%i", 0);

	HttpHeaders::buildHTTPResponseHeader(td->buffer->GetBuffer(), &td->response);
	if(a->socket.send(td->buffer->GetBuffer(), 
                    (u_long)strlen(td->buffer->GetBuffer()), 0)==-1)
		return 0;
	return 1;
}

/*!
 *Send a hard wired 500 error when we have a system error
 */
int Http::sendHTTPhardError500(HttpThreadContext* td, ConnectionPtr a)
{
	char tmpStr[12];
	char time[HTTP_RESPONSE_DATE_DIM];
	const char hardHTML[] = "<!-- Hard Coded 500 Response --><body bgcolor=\"#000000\">"
          "<p align=\"center\">"
     "<font size=\"5\" color=\"#00C800\">Error 500</font></p><p align=\"center\">"    
     "<font size=\"5\" color=\"#00C800\">Internal Server error</font></p>\r\n";
	td->response.httpStatus=500;
	td->buffer->SetLength(0);
	*td->buffer <<  HTTP_ERROR_MSGS[e_500];
	*td->buffer << " from: " ;
	*td->buffer << a->getipAddr() ;
	*td->buffer << "\r\n";	
	td->buffer2->SetLength(0);
	*td->buffer2 << "HTTP/1.1 500 System Error\r\nServer: MyServer ";
	*td->buffer2 << versionOfSoftware;
	*td->buffer2 <<" \r\nContent-type: text/html\r\nContent-length: ";
	*td->buffer2  <<   CMemBuf::IntToStr((int)strlen(hardHTML), tmpStr, 12);
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
    if(!td->only_header)
   		a->socket.send(hardHTML, (u_long)strlen(hardHTML), 0);
	}
	return 0;
}

/*!
 *Returns the MIME type passing its extension.
 *Returns 1 if the file is registered.
 */
int Http::getMIME(HttpThreadContext* td, char *MIME, char *filename, 
                  char *ext, char **dest2)
{
	File::getFileExt(ext, filename);
	
  if(((Vhost*)(td->connection->host))->isMIME() )
  {
    return ((Vhost*)(td->connection->host))->getMIME()->getMIME(ext, MIME, dest2);
  }
	return lserver->mimeManager.getMIME(ext, MIME, dest2);
}

/*!
 *Map an URL to the machine file system.
 */
int Http::getPath(HttpThreadContext* td, ConnectionPtr /*s*/, char **filenamePath, 
                   const char *filename, int systemrequest)
{
	/*!
   *If it is a system request, search the file in the system directory.
   */
	if(systemrequest)
	{
    int filenamePathLen;
    if(*filenamePath)
      delete [] (*filenamePath);
    filenamePathLen = strlen(((Vhost*)(td->connection->host))->systemRoot) + 
                          strlen(filename) + 2;
    *filenamePath = new char[filenamePathLen];
    if(*filenamePath == 0)
      return 0;
		sprintf(*filenamePath, "%s/%s", ((Vhost*)(td->connection->host))->systemRoot, 
            filename);
	}
	/*!
   *Else the file is in the web directory.
   */
	else
	{	
		if(filename[0])
		{
      int filenamePathLen;
      u_long len;
      if(*filenamePath)
        delete [] (*filenamePath);
      filenamePathLen = strlen(((Vhost*)(td->connection->host))->documentRoot) + 
                            strlen(filename) + 3;
      *filenamePath = new char[filenamePathLen];
      if(*filenamePath == 0)
        return 0;
			strcpy(*filenamePath, ((Vhost*)(td->connection->host))->documentRoot);
      len=(u_long)(strlen(*filenamePath)+1);
			(*filenamePath)[len-1]='/';
			strcpy(&(*filenamePath)[len], filename);
		}
		else
		{
      int filenamePathLen;
      if(*filenamePath)
        delete [] (*filenamePath);
      filenamePathLen = strlen(((Vhost*)(td->connection->host))->documentRoot)+1;
      *filenamePath = new char[filenamePathLen];
      if(filenamePath == 0)
        return 0;
			strcpy(*filenamePath, ((Vhost*)(td->connection->host))->documentRoot);
		}

	}
  return 0;
}

/*!
 *Get the CSS file used in a direcotry browsing.
 */
char* Http::getBrowseDirCSSFile()
{
  return browseDirCSSpath;
}

/*!
 *Get the GZIP threshold.
 */
u_long Http::getGzipThreshold()
{
  return gzip_threshold;
}

/*!
 *Send a redirect message to the client.
 */
int Http::sendHTTPRedirect(HttpThreadContext* td, ConnectionPtr a, char *newURL)
{
	int keepalive=0;
	char time[HTTP_RESPONSE_DATE_DIM];
	td->response.httpStatus=302;
	td->buffer2->SetLength(0);
	*td->buffer2 << "HTTP/1.1 302 Moved\r\nAccept-Ranges: bytes\r\nServer: MyServer " ;
	*td->buffer2<< versionOfSoftware;
	*td->buffer2 << "\r\nContent-type: text/html\r\nLocation: ";
	*td->buffer2  << newURL ;
	*td->buffer2  << "\r\nContent-length: 0\r\n";

	if(!lstrcmpi(td->request.CONNECTION, "Keep-Alive"))
	{
		*td->buffer2 << "Connection: Keep-Alive\r\n";	
		keepalive = 1;
	}
	*td->buffer2<< "Date: ";
	getRFC822GMTTime(time, HTTP_RESPONSE_DATE_DIM);
	*td->buffer2 << time ;
	*td->buffer2 << "\r\n\r\n";
	if(!a->socket.send(td->buffer2->GetBuffer(), 
                     (int)td->buffer2->GetLength(), 0))
		return 0;

	return keepalive;
}

/*!
 *Send a non-modified message to the client.
 */
int Http::sendHTTPNonModified(HttpThreadContext* td, ConnectionPtr a)
{
	int keepalive=0;
	char time[HTTP_RESPONSE_DATE_DIM];
	td->response.httpStatus=304;
	td->buffer2->SetLength(0);
	*td->buffer2 << "HTTP/1.1 304 Not Modified\r\nAccept-Ranges: bytes\r\n"
                  "Server: MyServer " ;

	*td->buffer2 << versionOfSoftware  ;
	*td->buffer2 <<  "\r\n";

	if(!lstrcmpi(td->request.CONNECTION, "Keep-Alive"))
	{
		*td->buffer2 << "Connection: Keep-Alive\r\n";	
		keepalive = 1;
	}	
	*td->buffer2 << "Date: ";
	
	getRFC822GMTTime(time, HTTP_RESPONSE_DATE_DIM);
	*td->buffer2 << time << "\r\n\r\n";

	if(!a->socket.send(td->buffer2->GetBuffer(), 
                     (int)td->buffer2->GetLength(), 0))
		return 0;
	return keepalive;
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
int Http::loadProtocol(XmlParser* languageParser, char* /*confFile*/)
{
  char *main_configuration_file;
  char *data;
  int defaultFilenameSize = 1;
  
  if(initialized)
		return 0;

  main_configuration_file = lserver->getMainConfFile();

  sec_cache_mutex.init();
		
	/*! Initialize ISAPI.  */
	Isapi::load();
	
	/*! Initialize FastCGI.  */
	FastCgi::load();	

	/*! Load the MSCGI library.  */
	mscgiLoaded=MsCgi::load();

  HttpFile::load();
	
  HttpDir::load();

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
	/*! 
   *Store defaults value.  
   *By default use GZIP with files bigger than a MB.  
   */
  cgi_timeout = MYSERVER_SEC(15);
  fastcgi_servers = 25;
	gzip_threshold=1<<20;
	useMessagesFiles=1;
  if(browseDirCSSpath)
  {
    delete [] browseDirCSSpath;
  }
	browseDirCSSpath = 0;
	XmlParser configurationFileManager;
	configurationFileManager.open(main_configuration_file);
	
	/*! Determine the min file size that will use GZIP compression.  */
	data=configurationFileManager.getValue("GZIP_THRESHOLD");
	if(data)
	{
		gzip_threshold=atoi(data);
	}	
	data=configurationFileManager.getValue("CGI_TIMEOUT");
	if(data)
	{
		cgi_timeout=MYSERVER_SEC(atoi(data));
	}	
	data=configurationFileManager.getValue("FASTCGI_MAX_SERVERS");
	if(data)
	{
    fastcgi_servers=atoi(data);
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
    int browseDirCSSpathlen = strlen(data)+1;
    browseDirCSSpath = new char[browseDirCSSpathlen];
    if(browseDirCSSpath)
    {
      strcpy(browseDirCSSpath, data);
    }
	}

  Cgi::setTimeout(cgi_timeout);
  FastCgi::setTimeout(cgi_timeout);
  WinCgi::setTimeout(cgi_timeout);
  Isapi::setTimeout(cgi_timeout);
  FastCgi::setMaxFcgiServers(fastcgi_servers);

	/*! 
   *Determine the number of default filenames written in 
   *the configuration file.  
   */
	nDefaultFilename=0;

	for(;;)
	{
		char xmlMember[32];
		sprintf(xmlMember, "DEFAULT_FILENAME%u", (u_int)nDefaultFilename);
		if(!configurationFileManager.getValue(xmlMember))
			break;
		nDefaultFilename++;
    defaultFilenameSize += strlen(configurationFileManager.getValue(xmlMember)) +1 ;

	}
	if(defaultFilename)
		delete [] defaultFilename;
	defaultFilename=0;
	/*!
   *Copy the right values in the buffer.
   */
	if(nDefaultFilename==0)
	{
		defaultFilename = new char [13];
		if(defaultFilename)
			strcpy(defaultFilename, "default.html");
	}
	else
	{
		u_long i;
    int cursor = 0;
		if(defaultFilename)
			delete [] defaultFilename;
		defaultFilename = new char [defaultFilenameSize];
		for(i=0;defaultFilename && (i<nDefaultFilename);i++)
		{
			char xmlMember[21];
			sprintf(xmlMember, "DEFAULT_FILENAME%u", (u_int)i);
			data=configurationFileManager.getValue(xmlMember);
			if(data)
				strcpy(&defaultFilename[cursor], data);
      cursor+=strlen(data) + 1;
		}
    defaultFilename[cursor] = '\0';
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
  
  sec_cache.free();

  sec_cache_mutex.destroy();

	if(defaultFilename)
	{
		delete [] defaultFilename;
		defaultFilename=0;
	}
  if(browseDirCSSpath)
  {
    delete [] browseDirCSSpath;
    browseDirCSSpath = 0;
  }
	initialized=0;
	return 1;
}

/*!
 *Returns the default filename.
 */
char *Http::getDefaultFilenamePath(u_long ID)
{
  u_long pos = 0;
  char *cursor = defaultFilename;
	if(ID<nDefaultFilename)
  {
    while(pos != ID)
    {
      while(*cursor != '\0')
        cursor++;
      pos++;
      cursor++;
    }
		return cursor;
  }
	else
    /*! Index out of bounds. */
		return 0;
}

/*!
 *Returns the name of the protocol. If an out buffer is defined fullfill 
 *it with the name too.
 */
char* Http::registerName(char* out, int len)
{
	if(out)
	{
		strncpy(out, "HTTP", len);
	}
	return "HTTP";
}

/*!
 *Constructor for the class http.
 */
Http::Http()
{
	strcpy(protocolPrefix, "http://");
	PROTOCOL_OPTIONS=0;
	td.filenamePath=0;
	td.pathInfo=0;
	td.pathTranslated=0;
	td.cgiRoot=0;
	td.cgiFile=0;
	td.scriptPath=0;
	td.scriptDir=0;
	td.scriptFile=0;
	td.inputDataPath=0;
	td.outputDataPath=0;
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
  if(td.filenamePath)
    delete [] td.filenamePath;
  if(td.pathInfo)
    delete [] td.pathInfo;
  if(td.pathTranslated)
    delete [] td.pathTranslated;
  if(td.cgiRoot)
    delete [] td.cgiRoot;
  if(td.cgiFile)
    delete [] td.cgiFile;
  if(td.scriptPath)
    delete [] td.scriptPath;
  if(td.scriptDir)
    delete [] td.scriptDir;
	if(td.scriptFile)
    delete [] td.scriptFile;
  if(td.inputDataPath)
    delete [] td.inputDataPath;
  if(td.outputDataPath)
    delete [] td.outputDataPath;

  td.filenamePath=0;
  td.pathInfo=0;
  td.pathTranslated=0;
  td.cgiRoot=0;
  td.cgiFile=0;
  td.scriptPath=0;
  td.scriptDir=0;
	td.scriptFile=0;
  td.inputDataPath=0;
  td.outputDataPath=0;
}
