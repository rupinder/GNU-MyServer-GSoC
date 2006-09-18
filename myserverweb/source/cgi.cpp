/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006 The MyServer Team
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
#include "../include/cgi.h"
#include "../include/http_headers.h"
#include "../include/http.h"
#include "../include/http_constants.h"
#include "../include/server.h"
#include "../include/security.h"
#include "../include/mime_utils.h"
#include "../include/file.h"
#include "../include/sockets.h"
#include "../include/utility.h"
#include "../include/mem_buff.h"
#include "../include/filters_chain.h"
#include "../include/pipe.h"

#include <string>
#include <sstream>

extern "C" {
#ifdef WIN32
#include <direct.h>
#endif
#include <string.h>
}

using namespace std;

/*!
 *By default use a timeout of 15 seconds on new processes.
 */
int Cgi::cgiTimeout = MYSERVER_SEC(15);

/*!
 *Run the standard CGI and send the result to the client.
 *\param td The HTTP thread context.
 *\param s A pointer to the connection structure.
 *\param scriptpath The script path.
 *\param cgipath The CGI handler path as specified by the MIME type.
 *\param execute Specify if the script has to be executed.
 *\param onlyHeader Specify if send only the HTTP header.
 */
int Cgi::send(HttpThreadContext* td, ConnectionPtr s, 
							const char* scriptpath, const char *cgipath, 
							int execute, int onlyHeader)
{
 	/* 
	 *Use this flag to check if the CGI executable is 
	 *nph (Non Parsed Header).  
	 */
	int nph = 0;
	ostringstream cmdLine;
  u_long nbw = 0;
	u_long nbw2 = 0;

  FiltersChain chain;
	Process cgiProc;
	u_long procStartTime;

	StartProcInfo spi;
	string moreArg;
	string tmpCgiPath;
	u_long nBytesRead;
	u_long headerSize = 0;
	bool useChunks = false;
	bool keepalive = false;
	bool headerCompleted = false;
	u_long headerOffset = 0;

	/*!
   *Standard CGI uses STDOUT to output the result and the STDIN 
   *to get other params like in a POST request.
   */
	Pipe stdOutFile;
	File stdInFile;

	td->scriptPath.assign(scriptpath);
  
  {
    /* Do not modify the text between " and ".  */
    int x;
    int subString = cgipath[0] == '"';
    int len = strlen(cgipath);
    for(x = 1; x < len; x++)
    {
      if(!subString && cgipath[x] == ' ')
        break;
      if(cgipath[x] == '"')
        subString = !subString;
    }


		{
			HttpRequestHeader::Entry* e = td->request.other.get("Connection");
			if(e)
				keepalive = !lstrcmpi(e->value->c_str(),"keep-alive");
			else
				keepalive = false;
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

    /*
     *Save the cgi path and the possible arguments.
     *the (x<len) case is when additional arguments are specified. 
     *If the cgipath is enclosed between " and " do not consider them 
     *when splitting directory and file name.
     */
    if(x < len)
    {
      string tmpString(cgipath);
      int begin = tmpString[0] == '"' ? 1: 0;
      int end = tmpString[x] == '"' ? x: x-1;
      tmpCgiPath.assign(tmpString.substr(begin, end-1));
      moreArg.assign(tmpString.substr(x, len-1));  
    }
    else
    {
      int begin = (cgipath[0] == '"') ? 1 : 0;
      int end   = (cgipath[len] == '"') ? len-1 : len;
      tmpCgiPath.assign(&cgipath[begin], end-begin);
      moreArg.assign("");
    }
    File::splitPath(tmpCgiPath, td->cgiRoot, td->cgiFile);
    
    tmpCgiPath.assign(scriptpath);
    File::splitPath(tmpCgiPath, td->scriptDir, td->scriptFile);
  }

  chain.setProtocol(td->http);
  chain.setProtocolData(td);
  chain.setStream(&(td->connection->socket));
  if(td->mime)
  {
    if(td->mime && Server::getInstance()->getFiltersFactory()->chain(&chain,
															td->mime->filters, 
                              &(td->connection->socket) , &nbw, 1))
      {
        td->connection->host->warningslogRequestAccess(td->id);
        td->connection->host->warningsLogWrite("Cgi: Error loading filters");
        td->connection->host->warningslogTerminateAccess(td->id);
        chain.clearAllFilters(); 
        return td->http->raiseHTTPError(e_500);
      }
  }
	
  if(execute)
  {
    const char *args = 0;
    if(td->request.uriOpts.length())
      args = td->request.uriOpts.c_str();
    else if(td->pathInfo.length())
      args = &td->pathInfo[1];

    if(cgipath && strlen(cgipath))
      cmdLine << td->cgiRoot << "/" << td->cgiFile << " " << moreArg << " " 
							<< td->scriptFile <<  (args ? args : "" ) ;
    else
      cmdLine << td->scriptDir << "/" << td->scriptFile << " " 
							<< moreArg << " " << (args ? args : "" );

    if(td->scriptFile.length() > 4 && td->scriptFile[0] == 'n'
			 && td->scriptFile[1] == 'p' && td->scriptFile[2] == 'h' 
			 && td->scriptFile[3] == '-' )
      nph = 1; 
    else
      nph = 0;

    if(cgipath && strlen(cgipath))
    {
      spi.cmd.assign(td->cgiRoot);
      spi.cmd.append("/");
      spi.cmd.append(td->cgiFile);
      spi.arg.assign(moreArg);
      spi.arg.append(" ");
      spi.arg.append(td->scriptFile);
      if(args)
      {
        spi.arg.append(" ");
        spi.arg.append(args);
      }
    }
    else
    {
      spi.cmd.assign(scriptpath);
      spi.arg.assign(moreArg);
      if(args)
      {
        spi.arg.append(" ");
        spi.arg.append(args);
      }
    }

	}
	else
	{
     /* Check if the CGI executable exists.  */
		if((!cgipath) || (!File::fileExists(tmpCgiPath.c_str())))
		{
			td->connection->host->warningslogRequestAccess(td->id);
      if(cgipath && strlen(cgipath))
      {
        string msg;
        msg.assign("Cgi: Cannot find the ");
        msg.append(cgipath);
        msg.append("executable");
        td->connection->host->warningsLogWrite(msg.c_str());
			}
      else
      {
        td->connection->host->warningsLogWrite(
                                    "Cgi: Executable file not specified");
      }
      td->connection->host->warningslogTerminateAccess(td->id);		
      td->scriptPath.assign("");
      td->scriptFile.assign("");
      td->scriptDir.assign("");
      chain.clearAllFilters(); 
			return td->http->raiseHTTPError(e_500);
		}

    spi.arg.assign(moreArg);
    spi.arg.append(" ");
    spi.arg.append(td->scriptFile);		
    
    cmdLine << "\"" << td->cgiRoot << "/" << td->cgiFile << "\" " 
						<< moreArg << " " << td->scriptFile;
  
    spi.cmd.assign(td->cgiRoot);
    spi.cmd.append("/");
    spi.cmd.append(td->cgiFile);
    
    if(td->cgiFile.length() > 4 && td->cgiFile[0] == 'n'  
			 && td->cgiFile[1] == 'p' && td->cgiFile[2] == 'h' 
			 && td->cgiFile[3] == '-' )
      nph = 1;
    else
      nph = 0;
	}

  /*
   *Open the stdout file for the new CGI process. 
   */
	if(stdOutFile.create())
	{
		td->connection->host->warningslogRequestAccess(td->id);
		td->connection->host->warningsLogWrite
			      ("Cgi: Cannot create CGI stdout file");
		td->connection->host->warningslogTerminateAccess(td->id);
    chain.clearAllFilters(); 
		return td->http->raiseHTTPError(e_500);
	}

  /*! Open the stdin file for the new CGI process. */
  if(stdInFile.openFile(td->inputDataPath, 
                        FILE_OPEN_READ|FILE_OPEN_ALWAYS))
  {
		td->connection->host->warningslogRequestAccess(td->id);
		td->connection->host->warningsLogWrite("Cgi: Cannot open CGI stdin file");
		td->connection->host->warningslogTerminateAccess(td->id);
		stdOutFile.close();
    chain.clearAllFilters(); 
		return td->http->raiseHTTPError(e_500);
  }
  
	/*
   *Build the environment string used by the CGI started
   *by the execHiddenProcess(...) function.
   *Use the td->buffer2 to build the environment string.
   */
	(td->buffer2->getBuffer())[0] = '\0';
	buildCGIEnvironmentString(td, td->buffer2->getBuffer());
  
	/*
   *With this code we execute the CGI process.
   *Fill the StartProcInfo struct with the correct values and use it
   *to run the process.
   */
	spi.cmdLine = cmdLine.str();
	spi.cwd.assign(td->scriptDir);

	spi.stdError = (FileHandle) stdOutFile.getWriteHandle();
	spi.stdIn = (FileHandle) stdInFile.getHandle();
	spi.stdOut = (FileHandle) stdOutFile.getWriteHandle();
	spi.envString = td->buffer2->getBuffer();
  
  /* Execute the CGI process. */
  {
    if( cgiProc.execConcurrentProcess(&spi) == -1)
    {
      stdInFile.closeFile();
      stdOutFile.close();
      td->connection->host->warningslogRequestAccess(td->id);
      td->connection->host->warningsLogWrite
                                       ("Cgi: Error in the CGI execution");
      td->connection->host->warningslogTerminateAccess(td->id);
      chain.clearAllFilters(); 
      return td->http->raiseHTTPError(e_500);
    }
    /* Close the write stream of the pipe on the server.  */
		stdOutFile.closeWrite();	
  }

  /* Reset the buffer2 length counter. */
	td->buffer2->setLength(0);

	/* Read the CGI output.  */
	nBytesRead = 0;

	procStartTime = getTicks();

	/* Parse initial chunks of data looking for the HTTP header.  */
	while(!headerCompleted)
	{
		bool term;
		/* Do not try to read using a small buffer as this has some
			 bad influence on the performances.  */
		if(td->buffer2->getRealLength() - headerOffset - 1 < 512)
			break;
		
		nBytesRead = 0;
		
		term = stdOutFile.pipeTerminated();
			
		if(stdOutFile.read(td->buffer2->getBuffer() + headerOffset, 
											 td->buffer2->getRealLength() - headerOffset - 1, 
											 &nBytesRead))
		{
			stdInFile.closeFile();
			stdOutFile.close();
			td->connection->host->warningslogRequestAccess(td->id);
			td->connection->host->warningsLogWrite
				("Cgi: Error reading from CGI std out file");
			td->connection->host->warningslogTerminateAccess(td->id);
			chain.clearAllFilters();
			return td->http->raiseHTTPError(e_500);
		}
			
		if(nBytesRead == 0)
		{
			if((int)(getTicks() - procStartTime) > cgiTimeout)
			 {
				 break;
			 }
			else
				{
					if(term)
						break;
					continue;
				}
		}

		headerOffset += nBytesRead;


		if(headerOffset > td->buffersize2 - 5)
			(td->buffer2->getBuffer())[headerOffset] = '\0';
		
		if(headerOffset == 0)
		{
			td->connection->host->warningslogRequestAccess(td->id);
			td->connection->host->warningsLogWrite("Cgi: Error CGI zero bytes read");
			td->connection->host->warningslogTerminateAccess(td->id);
			td->http->raiseHTTPError(e_500);
			stdOutFile.close();
			stdInFile.closeFile();
			chain.clearAllFilters(); 
			cgiProc.terminateProcess();
			return 0;
		}

		/* Standard CGI can include an extra HTTP header.  */
		headerSize = 0;
		nbw = 0;
		for(u_long i = std::max(0, (int)headerOffset - (int)nBytesRead - 10); 
				i < headerOffset; i++)
		{
			char *buff = td->buffer2->getBuffer();
			if( (buff[i] == '\r') && (buff[i+1] == '\n') 
					&& (buff[i+2] == '\r') && (buff[i+3] == '\n') )
			{
				/*
				 *The HTTP header ends with a \r\n\r\n sequence so 
				 *determine where it ends and set the header size
				 *to i + 4.
				 */
				headerSize = i + 4 ;
				headerCompleted = true;
				break;
			}
			else if((buff[i] == '\n') && (buff[i+1] == '\n'))
			{
				/*
				 *\n\n case.
				 */
				headerSize = i + 2;
				headerCompleted = true;
				break;
			}

			/*
			 *If it is present Location: xxx in the header 
			 *send a redirect to xxx.  
			 */
			else if(!strncmp(&(td->buffer2->getBuffer())[i], "Location:", 9))
			{
				string nURL;
				u_long len = 0;
				while( (td->buffer2->getBuffer())[i + len + 9] != '\r' &&
							 len < td->buffer2->getRealLength() - i - 9)
				{
					len++;
				}
				nURL.assign(&(td->buffer2->getBuffer()[i + 9]), len);
				td->http->sendHTTPRedirect(nURL.c_str());
				stdOutFile.close();
				stdInFile.closeFile();
				chain.clearAllFilters(); 
				cgiProc.terminateProcess();
				return 1;
			}
		}
	}

	/* Send the header.  */
	{
		HttpRequestHeader::Entry *connection = 
			td->request.other.get("Connection");
		
		if(connection && !lstrcmpi(connection->value->c_str(), "keep-alive"))
			td->response.connection.assign("keep-alive");
		/*
		 *Do not send any other HTTP header if the CGI executable
		 *has the nph-. form name.  
		 */
		if(nph)
		{
			/*
			 *Resetting the structure we send only the information gived 
			 *by the CGI.  
			 */
			HttpHeaders::resetHTTPResponse(&(td->response));
			td->response.ver.assign(td->request.ver.c_str());
		}
		
		/*
		 *If we have not to append the output send data 
		 *directly to the client.  
		 */
		if(!td->appendOutputs)
		{
			/* Send the header.  */
			if(headerSize)
				HttpHeaders::buildHTTPResponseHeaderStruct(&td->response, td, 
																									 td->buffer2->getBuffer());
			
			HttpHeaders::buildHTTPResponseHeader(td->buffer->getBuffer(),
																					 &td->response);
			
			td->buffer->setLength((u_int)strlen(td->buffer->getBuffer()));
			
			if(chain.write(td->buffer->getBuffer(),
										 static_cast<int>(td->buffer->getLength()), &nbw2))
			{
				stdInFile.closeFile();
				stdOutFile.close();
				chain.clearAllFilters(); 
				/* Remove the connection on sockets error. */
				cgiProc.terminateProcess();
				return 0;
			}
			
			if(onlyHeader)
			{
				stdOutFile.close();
				stdInFile.closeFile();
				chain.clearAllFilters(); 
				cgiProc.terminateProcess();
				return 1;
			}
		}
	}

	/* Flush the buffer.  Data from the header parsing can be present.  */
	if(headerOffset-headerSize)
	{
		if(useChunks)
		{
			ostringstream tmp;
			tmp << hex << (headerOffset-headerSize) << "\r\n";
			td->response.contentLength.assign(tmp.str());
			if(chain.write(tmp.str().c_str(), tmp.str().length(), &nbw2))
			{
				stdOutFile.close();
				stdInFile.closeFile();
				chain.clearAllFilters(); 
				/* Remove the connection on sockets error.  */
				cgiProc.terminateProcess();
				return 0;       
			}
		}

		if(chain.write((td->buffer2->getBuffer() + headerSize), 
									 headerOffset-headerSize, &nbw2))
		{
			stdOutFile.close();
			stdInFile.closeFile();
			chain.clearAllFilters(); 
			/* Remove the connection on sockets error.  */
			cgiProc.terminateProcess();
			return 0;       
		}
		
		nbw += nbw2;
		
		if(useChunks && chain.write("\r\n", 2, &nbw2))
		{
			stdOutFile.close();
			stdInFile.closeFile();
			chain.clearAllFilters(); 
			/* Remove the connection on sockets error.  */
			cgiProc.terminateProcess();
			return 0;       
		}
	}
	
	/* Send the rest of the data until we can read from the pipe.  */
	do
	{
		/* Process timeout.  */
		if((int)(getTicks() - procStartTime) > cgiTimeout)
		{
			stdOutFile.close();
			stdInFile.closeFile();
			chain.clearAllFilters(); 
			/* Remove the connection on sockets error.  */
			cgiProc.terminateProcess();
			return 0;       
		}
		
		if(stdOutFile.pipeTerminated() || 
			 (!nBytesRead && !cgiProc.isProcessAlive()))
		{
			nBytesRead = 0;   
		}
		else
		{
			/* Read data from the process standard output file.  */
			if(stdOutFile.read(td->buffer2->getBuffer(), 
												 td->buffer2->getRealLength(), 
												 &nBytesRead))
			{
				stdOutFile.close();
				stdInFile.closeFile();
				chain.clearAllFilters(); 
				/* Remove the connection on sockets error.  */
				cgiProc.terminateProcess();
				return 0;      
			}
			
		}

		if(nBytesRead)
		{
			if(!td->appendOutputs)
			{
				if(useChunks)
				{
					ostringstream tmp;
					tmp << hex <<  nBytesRead << "\r\n";
					td->response.contentLength.assign(tmp.str());
					if(chain.write(tmp.str().c_str(), tmp.str().length(), &nbw2))
					{
						stdOutFile.close();
						stdInFile.closeFile();
						chain.clearAllFilters(); 
						/* Remove the connection on sockets error.  */
						cgiProc.terminateProcess();
						return 0;       
					}
				}

				if(chain.write(td->buffer2->getBuffer(), nBytesRead, &nbw2))
				{
					stdOutFile.close();
					stdInFile.closeFile();
					chain.clearAllFilters(); 
					/* Remove the connection on sockets error.  */
					cgiProc.terminateProcess();
					return 0;      
				}

				nbw += nbw2;
				
				if(useChunks && chain.write("\r\n", 2, &nbw2))
				{
					stdOutFile.close();
					stdInFile.closeFile();
					chain.clearAllFilters(); 
					/* Remove the connection on sockets error.  */
					cgiProc.terminateProcess();
					return 0;       
				}
			}
			else/* !td->appendOutputs.  */
			{
				if(td->outputData.writeToFile(td->buffer2->getBuffer(), 
																			nBytesRead, &nbw2))
				{
					stdOutFile.close();
					stdInFile.closeFile();
					chain.clearAllFilters(); 
					File::deleteFile(td->inputDataPath);
					/* Remove the connection on sockets error.  */
					cgiProc.terminateProcess();
					return 0;      
				}
				nbw += nbw2;
			}
		}
	} 
	while(!stdOutFile.pipeTerminated() && 
				( nBytesRead || cgiProc.isProcessAlive()));
	
	/* Send the last null chunk if needed.  */
	if(useChunks && chain.write("0\r\n\r\n", 5, &nbw2))
	{
		stdOutFile.close();
		stdInFile.closeFile();
		chain.clearAllFilters(); 
	
		/* Remove the connection on sockets error.  */
		cgiProc.terminateProcess();
		return 0;       
	}

	/* Update the Content-Length field for logging activity.  */
	td->sentData += nbw;

  chain.clearAllFilters(); 	

	cgiProc.terminateProcess();
	
  /* Close the stdin and stdout files used by the CGI.  */
	stdOutFile.close();
	stdInFile.closeFile();
	
	/* Delete the file only if it was created by the CGI module.  */
	if(!td->inputData.getHandle())
	  File::deleteFile(td->inputDataPath.c_str());
  
	return 1;  
}

/*!
 *Write the string that contain the CGI environment to cgiEnvString.
 *This function is used by other server side protocols too.
 *\param td The HTTP thread context.
 *\param cgiEnv The zero terminated list of environment string.
 *\param processEnv Specify if add current process environment 
 *variables too.
 */
void Cgi::buildCGIEnvironmentString(HttpThreadContext* td, char *cgiEnv, 
                                    int processEnv)
{
	MemBuf memCgi;
	char strTmp[32];

	memCgi.setExternalBuffer(cgiEnv, td->buffer2->getRealLength());
	memCgi << "SERVER_SOFTWARE=MyServer " << versionOfSoftware;

#ifdef WIN32
	memCgi << " (WIN32)";
#else
#ifdef HOST_STR
	memCgi << " " << HOST_STR;
#else
	memCgi << " (Unknown)";
#endif
#endif
	/* Must use REDIRECT_STATUS for php and others.  */
	memCgi << end_str << "REDIRECT_STATUS=TRUE";
	
	memCgi << end_str << "SERVER_NAME=";
 	memCgi << Server::getInstance()->getServerName();

	memCgi << end_str << "SERVER_SIGNATURE=";
	memCgi << "<address>MyServer ";
	memCgi << versionOfSoftware;
	memCgi << "</address>";

	memCgi << end_str << "SERVER_PROTOCOL=";
	memCgi << td->request.ver.c_str();	
	
  {
    MemBuf portBuffer;
    portBuffer.uintToStr( td->connection->getLocalPort());
    memCgi << end_str << "SERVER_PORT="<< portBuffer;
  }

	memCgi << end_str << "SERVER_ADMIN=";
	memCgi << Server::getInstance()->getServerAdmin();

	memCgi << end_str << "REQUEST_METHOD=";
	memCgi << td->request.cmd.c_str();

	memCgi << end_str << "REQUEST_URI=";
	
 	memCgi << td->request.uri.c_str();

	memCgi << end_str << "QUERY_STRING=";
	memCgi << td->request.uriOpts.c_str();

	memCgi << end_str << "GATEWAY_INTERFACE=CGI/1.1";

	if(td->request.contentLength.length())
	{
		memCgi << end_str << "CONTENT_LENGTH=";
		memCgi << td->request.contentLength.c_str();
	}
	else
	{
		u_long fs = 0;
    ostringstream stream;
 
		if(td->inputData.getHandle())
			fs = td->inputData.getFileSize();

    stream << fs;

		memCgi << end_str << "CONTENT_LENGTH=" << stream.str().c_str();
	}


	if(td->request.rangeByteBegin || td->request.rangeByteEnd)
	{
    ostringstream rangeBuffer;
		memCgi << end_str << "HTTP_RANGE=" << td->request.rangeType << "=" ;
    if(td->request.rangeByteBegin)
    {
      rangeBuffer << static_cast<int>(td->request.rangeByteBegin);
      memCgi << rangeBuffer.str();
    }
    memCgi << "-";
    if(td->request.rangeByteEnd)
    {
      rangeBuffer << td->request.rangeByteEnd;
      memCgi << rangeBuffer.str();
    }   

	}

	if(td->cgiRoot.length())
	{
		memCgi << end_str << "CGI_ROOT=";
		memCgi << td->cgiRoot;
	}

	if(td->connection->getIpAddr()[0])
	{
		memCgi << end_str << "REMOTE_ADDR=";
		memCgi << td->connection->getIpAddr();
	}

	if(td->connection->getPort())
	{
    MemBuf remotePortBuffer;
    remotePortBuffer.MemBuf::uintToStr(td->connection->getPort() );
	 	memCgi << end_str << "REMOTE_PORT=" << remotePortBuffer;
	}

	if(td->connection->getLogin()[0])
	{
    memCgi << end_str << "REMOTE_USER=";
		memCgi << td->connection->getLogin();
	}
	
	if(td->connection->host->getProtocol() == PROTOCOL_HTTPS)
		memCgi << end_str << "SSL=ON";
	else
		memCgi << end_str << "SSL=OFF";

	if(td->request.auth.length())
	{
		memCgi << end_str << "AUTH_TYPE=";
		memCgi << td->request.auth.c_str();
	}

	{
		HttpRequestHeader::Entry* e = td->request.other.get("Host");
		if(e)
		{
			memCgi << end_str << "HTTP_HOST=";
			memCgi << e->value->c_str();
		}
	}

	{
		HttpRequestHeader::Entry* e = td->request.other.get("Cookie");
		if(e)
		{
			memCgi << end_str << "HTTP_COOKIE=";
			memCgi << e->value->c_str();
		}
	}

	{
		HttpRequestHeader::Entry* e = td->request.other.get("Connection");
		if(e)
		{
			memCgi << end_str << "HTTP_CONNECTION=";
			memCgi << e->value->c_str();
		}
	}


	{
		HttpRequestHeader::Entry* e = td->request.other.get("User-Agent");
		if(e)
		{
			memCgi << end_str << "HTTP_USER_AGENT=";
			memCgi << e->value->c_str();
		}
	}

	{
		HttpRequestHeader::Entry* e = td->request.other.get("Accept");
		if(e)
		{
			memCgi << end_str << "HTTP_ACCEPT=";
			memCgi << e->value->c_str();
		}
	}

	{
		HttpRequestHeader::Entry* e = td->request.other.get("Content-Type");
		if(e)
		{
			memCgi << end_str << "CONTENT_TYPE=";
			memCgi << e->value->c_str();
		}
	}

	{
		HttpRequestHeader::Entry* e = td->request.other.get("Cache-Control");
		if(e)
		{
			memCgi << end_str << "HTTP_CACHE_CONTROL=";
			memCgi << e->value->c_str();
		}
	}

	{
		HttpRequestHeader::Entry* e = td->request.other.get("Referer");
		if(e)
		{
			memCgi << end_str << "HTTP_REFERER=";
			memCgi << e->value->c_str();
		}
	}

	{
		HttpRequestHeader::Entry* e = td->request.other.get("Accept-Encoding");
		if(e)
		{
			memCgi << end_str << "HTTP_ACCEPT_ENCODING=";
			memCgi << e->value->c_str();
		}
	}

	{
		HttpRequestHeader::Entry* e = td->request.other.get("From");
		if(e)
		{
			memCgi << end_str << "HTTP_FROM=";
			memCgi << e->value->c_str();
		}
	}

	{
		HttpRequestHeader::Entry* e = td->request.other.get("Accept-Language");
		if(e)
		{
			memCgi << end_str << "HTTP_ACCEPT_LANGUAGE=";
			memCgi << e->value->c_str();
		}
	}

	{
		HttpRequestHeader::Entry* e = td->request.other.get("Accept-Charset");
		if(e)
		{
			memCgi << end_str << "HTTP_ACCEPT_CHARSET=";
			memCgi << e->value->c_str();
		}
	}

	if(td->pathInfo.length())
	{
		memCgi << end_str << "PATH_INFO=";
    memCgi << td->pathInfo;
	  	
    memCgi << end_str << "PATH_TRANSLATED=";
		memCgi << td->pathTranslated;
	}
	else
  {
    memCgi << end_str << "PATH_TRANSLATED=";
		memCgi << td->filenamePath;
	}

 	memCgi << end_str << "SCRIPT_FILENAME=";
	memCgi << td->filenamePath;
	
	/*
   *For the DOCUMENT_URI and SCRIPT_NAME copy the 
   *requested uri without the pathInfo.
   */
	memCgi << end_str << "SCRIPT_NAME=";
	memCgi << td->request.uri.c_str();

	memCgi << end_str << "SCRIPT_URL=";
	memCgi << td->request.uri.c_str();

	memCgi << end_str << "DATE_GMT=";
	getRFC822GMTTime(strTmp, HTTP_RESPONSE_DATE_DIM);
	memCgi << strTmp;

 	memCgi << end_str << "DATE_LOCAL=";
	getRFC822LocalTime(strTmp, HTTP_RESPONSE_DATE_DIM);
	memCgi << strTmp;

	memCgi << end_str << "DOCUMENT_ROOT=";
	memCgi << td->connection->host->getDocumentRoot();

	memCgi << end_str << "DOCUMENT_URI=";
	memCgi << td->request.uri.c_str();
	
	memCgi << end_str << "DOCUMENT_NAME=";
	memCgi << td->filenamePath;

	if(td->identity[0])
	{
  		memCgi << end_str << "REMOTE_IDENT=";
      memCgi << td->identity;
  }
#ifdef WIN32
	if(processEnv)
	{
    LPTSTR lpszVariable; 
		LPVOID lpvEnv; 
		lpvEnv = Server::getInstance()->envString; 
		memCgi << end_str;
		for (lpszVariable = (LPTSTR) lpvEnv; *lpszVariable; lpszVariable++) 
		{ 
			if(((char*)lpszVariable)[0]  != '=' )
			{
				memCgi << (char*)lpszVariable << end_str;
			}
			while(*lpszVariable)
				*lpszVariable++;
		} 
	}
#endif
	memCgi << end_str << end_str  << end_str  << end_str  << end_str  ;
}

/*!
 *Set the CGI timeout for the new processes.
 *\param nt The new timeout value.
 */
void Cgi::setTimeout(int nt)
{
   cgiTimeout = nt;
}

/*!
 *Get the timeout value for CGI processes.
 */
int Cgi::getTimeout()
{
  return cgiTimeout;
}
