/*
MyServer
Copyright (C) 2005, 2006, 2007 The MyServer Team
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
#include "../include/http_file.h"
#include "../include/gzip.h"
#include "../include/server.h"
#include "../include/filters_chain.h"
#include "../include/memory_stream.h"

#include <sstream>
#include <algorithm>
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

/*!
 *Send a file to the client using the HTTP protocol.
 *\param td The current HTTP thread context.
 *\param s A pointer to the connection.
 *\param filenamePath The path of the static file to send.
 *\param exec Not used.
 *\param onlyHeader Specify if send only the HTTP header.
  */
int HttpFile::send(HttpThreadContext* td, ConnectionPtr s, 
									 const char *filenamePath, const char* /*exec*/,
									 int onlyHeader)
{
	/*
   *With this routine we send a file through the HTTP protocol.
   *Open the file and save its handle.
   */
	int ret;

	/* 
   *Will we use GZIP compression to send data?
   */
	bool useGzip = false;
  u_long filesize = 0;
	File *file;
	u_long bytesToSend;
  u_long firstByte = td->request.rangeByteBegin; 
  u_long lastByte = td->request.rangeByteEnd;
  bool keepalive = false;
  bool useChunks = false;
  bool useModifiers = false;
  MemoryStream memStream(td->buffer2);
  FiltersChain chain;
	
	u_long dataSent = 0;

  try
  {
		file = Server::getInstance()->getCachedFiles()->open(filenamePath);
    if(file == 0)
    {	
      return td->http->raiseHTTPError(e_500);
    }
    /*
     *Check how many bytes are ready to be send.  
     */
    filesize = file->getFileSize();
    bytesToSend = filesize;
    if(lastByte == 0)
    {
      lastByte = bytesToSend;
    }
    else
    {
      /* 
       *If the client use ranges set the right value 
       *for the last byte number.  
       */
      lastByte = std::min(lastByte + 1, bytesToSend);
    }


    {
      /* 
       *Use GZIP compression to send files bigger than GZIP threshold.  
       */
      const char* val = s->host->getHashedData("GZIP_THRESHOLD");
      useGzip = false;
      if(val)
      {
        u_long gzipThreshold = atoi(val);
        if(bytesToSend >= gzipThreshold)
          useGzip = true;
      }

    }

		{
			HttpRequestHeader::Entry* e = td->request.other.get("Connection");
			if(e)
				keepalive = !lstrcmpi(e->value->c_str(),"keep-alive");
			else
				keepalive = false;
		}

#ifndef DO_NOT_USEGZIP
    /*
     *Be sure that the client accept GZIP compressed data.  
     */
    if(useGzip)
		{
			HttpRequestHeader::Entry* e = td->request.other.get("Accept-Encoding");
			if(e)
			{
				useGzip &= (e->value->find("gzip") != string::npos);
			}
			else
				useGzip = false;

		}
#else
    /* 
     *If compiled without GZIP support force the server to don't use it.  
     */
    useGzip = false;
#endif	
    if(td->appendOutputs)
      useGzip = false;

    /*
     *bytesToSend is the interval between the first and the last byte.  
     */
    bytesToSend = lastByte - firstByte;
    
    /*
     *If fail to set the file pointer returns an internal server error.  
     */
    ret = file->setFilePointer(firstByte);
    if(ret)
    {
      file->closeFile();
			delete file;
      return td->http->raiseHTTPError(e_500);
    }

    td->buffer->setLength(0);

    /* If a Range was requested send 206 and not 200 for success.  */
    if( td->request.rangeByteBegin ||  td->request.rangeByteEnd )
    {	
			HttpResponseHeader::Entry *e;
      ostringstream buffer;
      td->response.httpStatus = 206;
      buffer << "bytes "<< (u_long)firstByte << "-" 
             << (u_long)lastByte << "/" << (u_long)filesize ;

			e = td->response.other.get("Content-Range");
			if(e)
				e->value->assign(buffer.str());
			else
  		{
				e = new HttpResponseHeader::Entry();
				e->name->assign("Transfer-Encoding");
				e->value->assign(buffer.str());
				td->response.other.put(*(e->name), e);
			}


			e = td->response.other.get("Transfer-Encoding");
			if(e)
				e->value->assign("chunked");
			else
  		{
				e = new HttpResponseHeader::Entry();
				e->name->assign("Transfer-Encoding");
				e->value->assign("chunked");
				td->response.other.put(*(e->name), e);
			}

      useGzip = false;
    }
    chain.setProtocol(td->http);
    chain.setProtocolData(td);
    chain.setStream(&memStream);
    if(td->mime)
    {
      u_long nbw;
      if(td->mime && 
				 Server::getInstance()->getFiltersFactory()->chain(&chain, 
																												 td->mime->filters, 
                                                         &memStream, &nbw))
      {
        file->closeFile();
				delete file;
        chain.clearAllFilters();
        return 0;
      }
      dataSent += nbw;
    }
    
    if(useGzip && !chain.isFilterPresent("gzip"))
    {
      Filter* gzipFilter = 
				Server::getInstance()->getFiltersFactory()->getFilter("gzip");
      u_long nbw;
      if(!gzipFilter)
      {
        file->closeFile();
				delete file;
        chain.clearAllFilters();
        return 0;
      }
      if(chain.addFilter(gzipFilter, &nbw))
      {
        delete gzipFilter;
        file->closeFile();
				delete file;
        chain.clearAllFilters();
        return 0;
      }
      dataSent += nbw;
    }

    useModifiers = chain.hasModifiersFilters();
 
    if(keepalive && !useModifiers)
    {
      ostringstream buffer;
      buffer << (u_int)bytesToSend;
      td->response.contentLength.assign(buffer.str());
    }

    /* Specify the connection type.  */
    if(keepalive)
    {
      td->response.connection.assign("keep-alive");
    }
    else
    {
      td->response.connection.assign("close");
    }

    if(useModifiers)
    {
			string s;
			HttpResponseHeader::Entry *e;
			chain.getName(s);
			e = td->response.other.get("Content-Encoding");
			if(e)
				e->value->assign(s);
			else
			{
				e = new HttpResponseHeader::Entry();
				e->name->assign("Content-Encoding");
				e->value->assign(s);
				td->response.other.put(*(e->name), e);
			}
      /* Do not use chunked transfer with old HTTP/1.0 clients.  */
      if(keepalive)
      {
				HttpResponseHeader::Entry *e;
				e = td->response.other.get("Transfer-Encoding");
				if(e)
					e->value->assign("chunked");
				else
  			{
					e = new HttpResponseHeader::Entry();
					e->name->assign("Transfer-Encoding");
					e->value->assign("chunked");
					td->response.other.put(*(e->name), e);
				}

        useChunks = true;
			}
    }
 
    HttpHeaders::buildHTTPResponseHeader(td->buffer->getBuffer(), 
                                         &td->response);
    td->buffer->setLength((u_long)strlen(td->buffer->getBuffer()));
    if(!td->appendOutputs)
    {
      /* Send the HTTP header.  */
      if(s->socket->send(td->buffer->getBuffer(), 
												 (u_long)td->buffer->getLength(), 0) == SOCKET_ERROR)
      {
        file->closeFile();
				delete file;
        chain.clearAllFilters();
        return 0;
      }
    }

    /*
     *If is requested only the header exit from the function; 
     *used by the HEAD request.  
     */
    if(onlyHeader)
    {
      file->closeFile();
			delete file;
      chain.clearAllFilters();
      return 1;
    }

    if(td->appendOutputs)
      chain.setStream(&(td->outputData)); 
    else
      chain.setStream(s->socket);

    
    /* Flush initial data.  */
    if(memStream.availableToRead())
    {
      ostringstream buffer;
      u_long nbw = 0;
      u_long nbr = 0;    
      ret = memStream.read(td->buffer->getBuffer(),
                           td->buffer->getRealLength(), 
                           &nbr);
      
      if(ret)
      {
        file->closeFile();
				delete file;
        chain.clearAllFilters();
        return 1;
      }

      if(nbr)
      {
        if(useChunks)
        {
          buffer << hex << nbr << "\r\n";     
          ret = chain.getStream()->write(buffer.str().c_str(), 
																				 buffer.str().length(), &nbw); 
          if(!ret)
          {
            ret = chain.getStream()->write(td->buffer->getBuffer(), nbr, &nbw);
            if(!ret)
            {
              dataSent += nbw;
              ret = chain.getStream()->write("\r\n", 2, &nbw);
            }
          }

          if(ret)
          {
            file->closeFile();
						delete file;
            chain.clearAllFilters();
            return 1;
          }     
        }
        else
        {
          ret = chain.getStream()->write(td->buffer->getBuffer(), nbr, &nbw);
          if(ret)
          {
            file->closeFile();
						delete file;
            chain.clearAllFilters();
            return 1;
          }     
          dataSent += nbw;
        }
      }/* nbr.  */
    } /* memStream.availableToRead().  */

    /* Flush the rest of the file.  */
    for(;;)
    {
      u_long nbr;
      u_long nbw;
      bool lastChunk = false;
      /* Read from the file the bytes to send.  */
      ret = file->readFromFile(td->buffer->getBuffer(),
														std::min(static_cast<u_long>(bytesToSend), 
												 static_cast<u_long>(td->buffer->getRealLength()/2)), 
                           &nbr);
      if(ret)
        break;
      
      bytesToSend -= nbr;

      /* Check if there are no other bytes to send.  */
      if(!nbr)
      {
        if(useChunks)
        {
          u_long nbw2;
          ostringstream buffer;
          {
            /* Flush to the memory stream and use it to send chunks.  */
            Stream *tmp = chain.getStream();
            chain.setStream(&memStream);
            chain.flush(&nbw);
            chain.setStream(tmp);
          }
          ret = memStream.read(td->buffer->getBuffer(), 
                             td->buffer->getRealLength(), &nbw);
          if(ret)
            break;
          if(nbw)
          {
            buffer << hex << nbw << "\r\n";
            ret = chain.getStream()->write(buffer.str().c_str(), 
																				 buffer.str().length(), &nbw2);
          
            if(ret)
              break;
            /* 
             *Write directly to the stream what we have bufferized in 
             *the memory stream.
             *We need to use the memory stream before send data to the 
						 *final stream as we cannot know the final length for the 
						 *data chunk.
             */
            ret = chain.getStream()->write(td->buffer->getBuffer(), 
																					 nbw, &nbw2);
            if(ret)
              break; 

            dataSent += nbw2;

            ret = chain.getStream()->write("\r\n", 2, &nbw);
            if(ret)
              break;
          }
          
        }
        else
        {
          /* If we don't use chunks we can flush directly.  */
          ret = chain.flush(&nbw);
          if(ret)
            break;
          dataSent += nbw;
        }
        if(ret)
          break;
        /* Set the flag when we reached the end of the file. */
        lastChunk = true; 
      }

      if(nbr)
      {
        if(useChunks)
        {
          u_long nbw2;
          ostringstream buffer;

          /* 
					 *We need to save data in the memory stream as we need to know
					 *the final length before we can flush to the real stream.
					 */
          {
            Stream *tmp = chain.getStream();
            if(!tmp)
            {
              s->host->warningslogRequestAccess(td->id);
              s->host->warningsLogWrite("HttpFile: no stream");
              s->host->warningslogTerminateAccess(td->id);
              break;
            }
            chain.setStream(&memStream);
            chain.write(td->buffer->getBuffer(), nbr, &nbw);
            chain.setStream(tmp);
          }
          ret = memStream.read(td->buffer->getBuffer(), 
															 td->buffer->getRealLength(), &nbw);
                             
          if(ret)
            break;  

          buffer << hex << nbw << "\r\n";
          ret = chain.getStream()->write(buffer.str().c_str(), 
																				 buffer.str().length(), &nbw2);
          if(ret)
            break;

          ret = chain.getStream()->write(td->buffer->getBuffer(), nbw, &nbw2);
          if(ret)
            break; 
          
          dataSent += nbw2;

          ret = chain.getStream()->write("\r\n", 2, &nbw);
          if(ret)
            break;
        }
        else/* Do not use chunks.  */
        {
          ret = chain.write(td->buffer->getBuffer(), nbr, &nbw);
          if(ret)
            break;     
          
          dataSent += nbw;       
        }

      }

      if(lastChunk)
      {
        if(useChunks)
        {
          u_long nbw;
          ret = chain.getStream()->write("0\r\n\r\n", 5, &nbw);
        }
        break;
      }
      memStream.refresh();

    }/* End for loop.  */

    file->closeFile();
		delete file;
  }
  catch(bad_alloc &ba)
  {
    file->closeFile();
		delete file;
    s->host->warningslogRequestAccess(td->id);
    s->host->warningsLogWrite("HttpFile: Error allocating memory");
    s->host->warningslogTerminateAccess(td->id);
    chain.clearAllFilters();
    return td->http->raiseHTTPError(e_500);
  }
  catch(...)
  {
    file->closeFile();
		delete file;
    s->host->warningslogRequestAccess(td->id);
    s->host->warningsLogWrite("HttpFile: Internal error");
    s->host->warningslogTerminateAccess(td->id);
    chain.clearAllFilters();
    return td->http->raiseHTTPError(e_500);
  };
 
  /* For logging activity.  */
	td->sentData += dataSent;

  chain.clearAllFilters();
	return 1;
}

/*!
 *Constructor for the class.
 */
HttpFile::HttpFile()
{

}

/*!
 *Destroy the object.
 */
HttpFile::~HttpFile()
{

}

/*!
 *Load the static elements.
 *\param confFile Not used.
 */
int HttpFile::load(XmlParser* /*confFile*/)
{
  return 0;
}

/*!
 *Unload the static elements.
 */
int HttpFile::unload()
{
  return 0;
}
