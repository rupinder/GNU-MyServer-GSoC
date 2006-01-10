/*
MyServer
Copyright (C) 2005 The MyServer Team
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

#include "../stdafx.h"
#include "../include/server.h"
#include "../include/http.h"
#include "../include/http_headers.h"
#include "../include/http_dir.h"
#include "../include/filters_chain.h"

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

#include "../include/lfind.h"
#ifdef NOT_WIN
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

#include <string>
#include <sstream>

using namespace std;

/*!
 *Constructor for the class.
 */
HttpDir::HttpDir()
{

}

/*!
 *Destroy the object.
 */
HttpDir::~HttpDir()
{

}

/*!
 *Load the static elements.
 */
int HttpDir::load(XmlParser* /*confFile*/)
{
  return 0;
}

/*!
 *Unload the static elements.
 */
int HttpDir::unload()
{
  return 0;
}


/*!
 *Fullfill the string out with a formatted representation fr bytes.
 */
void HttpDir::getFormattedSize(int bytes, string& out)
{
  ostringstream tmp;

  double leftover=0.0;
   
  if(bytes<1024)               //  Byte case
    tmp << bytes << " bytes";
  
  else if ((bytes>=1024) && (bytes<(1048576)))  //  KB case
  {
    u_long kb = static_cast<unsigned long int>(bytes/1024);
    leftover  = bytes%1024;
	  
    if(leftover<50.0)
      leftover=0.0;
    
    // note: this case isn't handled through compiler casting
    // using the output at the end!!!
    // therefore it has to be here ...
    else if(((static_cast<unsigned int>(leftover))%100)>50)
		  leftover+=100.0;
    
    if(leftover>=1000.0)
    {
      leftover=0.0;
      kb++;
    }
    else
    {
      while(leftover>=10)
      {	
        if (leftover)
          leftover/=10;
      }
    }

    // output ---> X.y KB
    tmp << kb <<  "." << static_cast<unsigned int>(leftover) << " kb";
    
  }
  else if ((bytes>=1048576) && (bytes<(1073741824)))  //  MB case
  {
    u_long mb = static_cast<unsigned long int>(bytes/1048576);
    leftover  = bytes%1048576;
	  
    if(leftover<50.0)
      leftover=0.0;
    
    // note: this case isn't handled through compiler casting
    // using the output at the end!!!
    // therefore it has to be here ...
    else if(((static_cast<unsigned int>(leftover))%100)>50)
		  leftover+=100.0;
    
    if(leftover>=1000.0)
    {
      leftover=0.0;
      mb++;
    }
    else
    {
      while(leftover>=10)
      {	
        if (leftover)
          leftover/=10;
      }
    }

    // output ---> X.y MB
    tmp << mb <<  "." << static_cast<unsigned int>(leftover) << " MB";
    
  }
  else //  GB case
  {
    u_long gb = static_cast<unsigned long int>(bytes/1073741824);
    leftover  = bytes%1073741824;
	  
    if(leftover<50.0)
      leftover=0.0;
    
    // note: this case isn't handled through compiler casting
    // using the output at the end!!!
    // therefore it has to be here ...
    else if(((static_cast<unsigned int>(leftover))%100)>50)
		  leftover+=100.0;
    
    if(leftover>=1000.0)
    {
      leftover=0.0;
      gb++;
    }
    else
    {
      while(leftover>=10)
      {	
        if (leftover)
          leftover/=10;
      }
    }

    // output ---> X.y GB
    tmp << gb <<  "." << static_cast<unsigned int>(leftover) << " GB";
    
  }
 

 out.assign(tmp.str());
}

/*!
 *Browse a directory printing its contents in an HTML file.
 */
int HttpDir::send(HttpThreadContext* td, ConnectionPtr s, const char* directory, 
                  const char* /*cgi*/, int onlyHeader)
{
	u_long nbw;
  string filename;
	int ret;
	FindData fd;
  FiltersChain chain;
	int startchar=0;
	int nDirectories=0;
	int i;
	char fileTime[32];
	char* bufferloop;
  const char* browseDirCSSpath;
	FileHandle outputDataHandle = td->outputData.getHandle();
  /*!
   *Create a new file if the old is not valid.
   */
	if( outputDataHandle == 0 )
	{
    ostringstream stream;
    stream << getdefaultwd(0, 0) <<  "/stdOutFile_" << td->id;
		td->outputDataPath.assign(stream.str());
		ret = td->outputData.openFile(td->outputDataPath.c_str(), 
                     FILE_CREATE_ALWAYS |FILE_OPEN_READ |
                                  FILE_OPEN_WRITE);
		if(ret)
		{
			/*! Return an internal server error.  */
			return ((Http*)td->lhttp)->raiseHTTPError(s, e_500);
		}
	}


  chain.setProtocol((Http*)td->lhttp);
  chain.setProtocolData(td);
  chain.setStream(&(td->connection->socket));
  if(td->mime)
  {
    if(td->mime && lserver->getFiltersFactory()->chain(&chain, 
                                   ((MimeManager::MimeRecord*)td->mime)->filters, 
                                   &(td->connection->socket) , &nbw, 1))
      {
        ((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
        ((Vhost*)td->connection->host)->warningsLogWrite("HttpDir: Error loading filters");
        ((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
        chain.clearAllFilters(); 
        return ((Http*)td->lhttp)->raiseHTTPError(s, e_500);
      }
  }

	for(i=0;td->request.uri[i];i++)
	{
		if(td->request.uri[i]=='/')
			nDirectories++;
	}
	for(startchar=0, i=0;td->request.uri[i];i++)
	{
		if(td->request.uri[i]=='/')
		{
			startchar++;
			if(startchar==nDirectories)
			{
				/*!
         *At the end of the loop set startchar to te real value.
         *startchar indicates the initial position in td->request.uri 
         *of the file path.
         */
				startchar=i+1;
				break;
			}
		}
	}
	td->buffer2->setLength(0);
	*td->buffer2<<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
    "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\"\r\n"
    "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\r\n"
    "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\">"
    "\r\n<head>\r\n<title>" ;
	*td->buffer2<< td->request.uri.c_str() ;
	*td->buffer2<< "</title>\r\n<meta http-equiv=\"content-type\" content=\"text/html;charset=UTF-8\" />\r\n</head>\r\n"; 
	ret = td->outputData.writeToFile(td->buffer2->getBuffer(), 
                                    (u_long)td->buffer2->getLength(), &nbw);
	if(ret)
	{
    /*! Return an internal server error. */
		td->outputData.closeFile();
    chain.clearAllFilters(); 
		return ((Http*)td->lhttp)->raiseHTTPError(s, e_500);
	}
  browseDirCSSpath = ((Http*)td->lhttp)->getBrowseDirCSSFile();

	/*! 
   *If it is defined a CSS file for the graphic layout of 
   *the browse directory insert it in the page.  
   */
	if(browseDirCSSpath != 0)
	{
		File cssHandle;
		ret=cssHandle.openFile(browseDirCSSpath, FILE_OPEN_IFEXISTS | 
                           FILE_OPEN_READ);
		if(ret == 0)
		{
			u_long nbr;
			ret = cssHandle.readFromFile(td->buffer->getBuffer(), 
                                   td->buffer->getRealLength(), &nbr);
			if(ret == 0)
			{
				td->buffer2->setLength(0);
				*td->buffer2 << "<style type=\"text/css\">\r\n<!--\r\n" ;
				ret=td->outputData.writeToFile(td->buffer2->getBuffer(), 
                                       (u_long)td->buffer2->getLength(), &nbw);
				if(ret)
				{
					td->outputData.closeFile();
          chain.clearAllFilters(); 
					/* Return an internal server error.  */
					return ((Http*)td->lhttp)->raiseHTTPError(s, e_500);
				}
				ret = td->outputData.writeToFile(td->buffer->getBuffer(),
                                          (u_long)nbr, &nbw);
				if(ret)
				{
					td->outputData.closeFile();
					/* Return an internal server error.  */
					return ((Http*)td->lhttp)->raiseHTTPError(s, e_500);
				}

				td->buffer2->setLength(0);
				*td->buffer2 << "-->\r\n</style>\r\n";
				ret=td->outputData.writeToFile(td->buffer2->getBuffer(),
                                       (u_long)td->buffer2->getLength(), &nbw);
				if(ret)
				{
					td->outputData.closeFile();
          chain.clearAllFilters(); 
					/* Return an internal server error.  */
					return ((Http*)td->lhttp)->raiseHTTPError(s, e_500);
				}
			}
			cssHandle.closeFile();

		}
	}

  filename = directory;
#ifdef WIN32
  filename.append("/*");
#endif
	td->buffer2->setLength(0);
	*td->buffer2 << "<body>\r\n<h1>Contents of directory ";
	*td->buffer2 <<  &td->request.uri[startchar] ;
	*td->buffer2 << "</h1>\r\n<hr />\r\n";
	ret = td->outputData.writeToFile(td->buffer2->getBuffer(), 
                                   (u_long)td->buffer2->getLength(), &nbw);
	if(ret)
	{
		td->outputData.closeFile();
		/*! Return an internal server error.  */
		return ((Http*)td->lhttp)->raiseHTTPError(s, e_500);
	}
	ret=fd.findfirst(filename.c_str());
	if(ret==-1)
	{
    chain.clearAllFilters(); 
		return ((Http*)td->lhttp)->raiseHTTPError(s, e_404);
	}
	/*! 
   *With the current code we build the HTML TABLE to indicize the
   *files in the directory.
   */
	td->buffer2->setLength(0);
	*td->buffer2 << "<table width=\"100%\">\r\n<tr>\r\n<td><b>File</b></td>\r\n<td><b>Last Modified</b></td>\r\n<td><b>Size</b></td>\r\n</tr>\r\n";
	ret = td->outputData.writeToFile(td->buffer2->getBuffer(), 
                                    (u_long)td->buffer2->getLength(), &nbw);
	if(ret)
	{
		td->outputData.closeFile();
    chain.clearAllFilters(); 
		/* Return an internal server error.  */
		return ((Http*)td->lhttp)->raiseHTTPError(s, e_500);
	}


  td->buffer2->setLength(0);

  if(File::getPathRecursionLevel(td->request.uri) >= 1)
  {
    string file;
    file.assign(td->request.uri);
    file.append("/../");
    
    *td->buffer2 << "<tr>\r\n<td colspan=\"2\">"
                 << "<a href=\""
                 << (td->request.uriEndsWithSlash ? ".." : ".")
	         << "\">[ .. ]</a></td>\n"
                 << "<td>[directory]</td></tr>\r\n";

    
  }
  
  ret = td->buffer2->getLength()  ? td->outputData.writeToFile(td->buffer2->getBuffer(), 
                                                 (u_long)td->buffer2->getLength(), &nbw)
                                  : 0;
  if(ret)
	{
    fd.findclose();
    td->outputData.closeFile();
    chain.clearAllFilters(); 
    /* Return an internal server error.  */
    return ((Http*)td->lhttp)->raiseHTTPError(s, e_500);
  }

	do
	{	
		if(fd.name[0]=='.')
			continue;
		/*! Do not show the security file.  */
		if(!strcmp(fd.name, "security"))
			continue;

    td->buffer2->setLength(0);

		*td->buffer2 << "<tr>\r\n<td><a href=\"";
		if(!td->request.uriEndsWithSlash)
		{
			*td->buffer2 << &td->request.uri[startchar];
			*td->buffer2 << "/" ;
		}
		*td->buffer2 << fd.name ;
		*td->buffer2 << "\">" ;
		*td->buffer2 << fd.name;
		*td->buffer2 << "</a></td>\r\n<td>";
	
		getRFC822GMTTime((time_t)fd.time_write, fileTime, 32);

		*td->buffer2 << fileTime ;
		*td->buffer2 << "</td>\r\n<td>";
		
		if(fd.attrib & FILE_ATTRIBUTE_DIRECTORY)
		{
			*td->buffer2 << "[directory]";
		}
		else
		{
      string out;
      getFormattedSize(fd.size, out);
 			*td->buffer2 << out;
		}

		*td->buffer2 << "</td>\r\n</tr>\r\n";
		ret = td->outputData.writeToFile(td->buffer2->getBuffer(), 
                                     (u_long)td->buffer2->getLength(), &nbw);
		if(ret)
		{
			fd.findclose();
			td->outputData.closeFile();
      chain.clearAllFilters(); 
			/* Return an internal server error.  */
			return ((Http*)td->lhttp)->raiseHTTPError(s, e_500);
		}
	}while(!fd.findnext());
	td->buffer2->setLength(0);
	*td->buffer2 << "</table>\r\n<hr />\r\n<address>MyServer " << versionOfSoftware;
              
  if(td->request.host[0])
  {    
    ostringstream portBuff;
    *td->buffer2 << " on ";
    *td->buffer2 << td->request.host.c_str() ;
    *td->buffer2 << " Port ";
    portBuff << td->connection->getLocalPort();
    *td->buffer2 << portBuff.str();
  }
	*td->buffer2 << "</address>\r\n</body>\r\n</html>\r\n";
	ret = td->outputData.writeToFile(td->buffer2->getBuffer(),
                                   (u_long)td->buffer2->getLength(), &nbw);
	if(ret)
	{
		fd.findclose();
		td->outputData.closeFile();
		/* Return an internal server error.  */
		return ((Http*)td->lhttp)->raiseHTTPError(s, e_500);
	}	
	fd.findclose();
  *td->buffer2 << end_str;
	/*! Changes the \ character in the / character.  */
	bufferloop=td->buffer2->getBuffer();
	while(*bufferloop++)
		if(*bufferloop=='\\')
			*bufferloop='/';
	if(!lstrcmpi(td->request.connection.c_str(), "Keep-Alive"))
		td->response.connection.assign( "Keep-Alive");

  {
    ostringstream tmp;
    tmp << td->outputData.getFileSize();
    td->response.contentLength.assign(tmp.str());
  }

	/*! 
   *If we haven't to append the output build the HTTP header 
   *and send the data.  
   */
	if(!td->appendOutputs)
  {
		u_long nbr=0;
    int nbs=0;
		td->outputData.setFilePointer(0);
		HttpHeaders::buildHTTPResponseHeader(td->buffer->getBuffer(), 
                                          &(td->response));
		nbs=s->socket.send(td->buffer->getBuffer(), 
                       (u_long)strlen(td->buffer->getBuffer()), 0);
		if(nbs == SOCKET_ERROR)
		{
			/*! Remove the connection.  */
			return 0;
		}	
    
    if(onlyHeader)
      return 1;
    
    do
		{
			ret = td->outputData.readFromFile(td->buffer->getBuffer(), 
                                        td->buffer->getRealLength(), &nbr);
			if(ret)
			{
				td->outputData.closeFile();
        chain.clearAllFilters(); 
				/*! Return an internal server error.  */
				return 0;
			}
      if(nbr)
      {
        u_long nbw;
        if(chain.write(td->buffer->getBuffer(), nbr, &nbw))
        {
          chain.clearAllFilters(); 
          /*! Return an internal server error.  */
          return 0;
        }
        nbs = static_cast<int>(nbw);
      }
      else
        nbs = 0;
		}while(nbr && nbs);
	}
  else
  {
    HttpHeaders::buildHTTPResponseHeader(td->buffer->getBuffer(), 
                                          &(td->response));
  }

  chain.clearAllFilters(); 
  return 1;

}
