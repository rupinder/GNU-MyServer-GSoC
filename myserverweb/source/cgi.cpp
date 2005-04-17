/*
*MyServer
*Copyright (C) 2002, 2003, 2004 The MyServer Team
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

#include "../stdafx.h"
#include "../include/cgi.h"
#include "../include/http_headers.h"
#include "../include/http.h"
#include "../include/HTTPmsg.h"
#include "../include/cserver.h"
#include "../include/security.h"
#include "../include/AMMimeUtils.h"
#include "../include/filemanager.h"
#include "../include/sockets.h"
#include "../include/utility.h"
#include "../include/MemBuf.h"

#include <string>
#include <sstream>

extern "C" {
#ifdef WIN32
#include <direct.h>
#endif
#include <string.h>
}

#ifndef lstrcpy
#define lstrcpy strcpy
#endif
#ifndef lstrlen
#define lstrlen strlen
#endif
#ifndef lstrcpyn
#define lstrcpyn strncpy
#endif
#ifndef strcat
#define strcat strcat
#endif
#ifndef strnicmp
#define strnicmp strncmp
#endif

using namespace std;

/*!
 *By default use a timeout of 15 seconds on new processes.
 */
int Cgi::cgi_timeout = MYSERVER_SEC(15);

/*!
 *Run the standard CGI and send the result to the client.
 */
int Cgi::send(HttpThreadContext* td, ConnectionPtr s, const char* scriptpath, 
              const char *cgipath, int execute, int only_header)
{
 	/*! Use this flag to check if the CGI executable is nph(Non Parsed Header).  */
	int nph = 0;
	ostringstream cmdLine;
	int yetoutputted=0;

	ostringstream outputDataPath;
	StartProcInfo spi;
	u_long nBytesRead;
	u_long headerSize;
	/*!
   *Standard CGI uses STDOUT to output the result and the STDIN 
   *to get other params like in a POST request.
   */
	File stdOutFile;
	File stdInFile;

	td->scriptPath.assign(scriptpath);
  
  {
    string tmp;
    tmp.assign(cgipath);
    File::splitPath(tmp, td->cgiRoot, td->cgiFile);
    
    tmp.assign(scriptpath);
    File::splitPath(tmp, td->scriptDir, td->scriptFile);
  }
	
  if(execute)
  {
#ifdef WIN32
		/*!
     *Under the windows platform to run a file like an executable
     *use the sintact "cmd /c filename".
     */
    if(cgipath && strlen(cgipath))
      cmdLine << "cmd /c " << cgipath << " " << 
        td->scriptFile <<  (td->pathInfo.length() ? &td->pathInfo[1]:td->pathInfo) ;
    else
      cmdLine << "cmd /c " << td->scriptFile << " " <<
              (td->pathInfo.length() ? &td->pathInfo[1] : td->pathInfo);
#endif
    if(td->scriptFile.length()>4 && td->scriptFile[0]=='n'  && td->scriptFile[1]=='p'
       && td->scriptFile[2]=='h' && td->scriptFile[3]=='-' )
      nph=1; 
    else
      nph=0;
   
    if(cgipath && strlen(cgipath))
    {
      spi.cmd.assign(cgipath);
      spi.arg.assign(td->scriptFile);
    }
    else
    {
      spi.cmd.assign(scriptpath);
      spi.arg.assign(td->pathInfo);
    }

	}
	else
	{
     /*! Check if the CGI executable exists. */
		if((!cgipath) || (!File::fileExists(cgipath)))
		{
			((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
      if(cgipath && strlen(cgipath))
      {
        ((Vhost*)td->connection->host)->warningsLogWrite("Cannot find ");
        ((Vhost*)td->connection->host)->warningsLogWrite(cgipath);
        ((Vhost*)td->connection->host)->warningsLogWrite(" executable\r\n");
			}
      else
      {
        ((Vhost*)td->connection->host)->warningsLogWrite(
                                          "Executable file not specified\r\n");
      }
      ((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);		
      td->scriptPath.assign("");
      td->scriptFile.assign("");
      td->scriptDir.assign("");
			return ((Http*)td->lhttp)->raiseHTTPError(td, s, e_500);
		}
    cmdLine << cgipath << " " << td->scriptFile;
    if(td->cgiFile.length()>4 && td->cgiFile[0]=='n'  && td->cgiFile[1]=='p'
                              && td->cgiFile[2]=='h' && td->cgiFile[3]=='-' )
      nph=1;
    else
      nph=0;
    spi.cmd.assign(cgipath);
    spi.arg.assign(td->scriptFile);
	}
  
	/*!
   *Use a temporary file to store CGI output.
   *Every thread has it own tmp file name(td->outputDataPath), 
   *so use this name for the file that is going to be
   *created because more threads can access more CGI at the same time.
   */

  outputDataPath << getdefaultwd(0,0) << "/stdOutFileCGI_" <<  (unsigned int)td->id;
  
  /*!
   *Open the stdout file for the new CGI process. 
   */
	if(stdOutFile.createTemporaryFile( outputDataPath.str().c_str() ))
	{
		((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((Vhost*)td->connection->host)->warningsLogWrite
                                        ("Cannot create CGI stdout file\r\n" );
		((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		return ((Http*)td->lhttp)->raiseHTTPError(td, s, e_500);
	}
  
	td->inputData.closeFile();
  
  /*! Open the stdin file for the new CGI process. */
	if(stdInFile.openFile(td->inputDataPath, FILE_OPEN_READ|
                        FILE_OPEN_ALWAYS))
	{
		((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((Vhost*)td->connection->host)->warningsLogWrite("Cannot open CGI stdin file\r\n");
		((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		stdOutFile.closeFile();
		return ((Http*)td->lhttp)->raiseHTTPError(td, s, e_500);
  }
  
	/*!
   *Build the environment string used by the CGI started
   *by the execHiddenProcess(...) function.
   *Use the td->buffer2 to build the environment string.
   */
	(td->buffer2->GetBuffer())[0]='\0';
	buildCGIEnvironmentString(td, td->buffer2->GetBuffer());
  
	/*!
   *With this code we execute the CGI process.
   *Fill the StartProcInfo struct with the correct values and use it
   *to run the process.
   */
	spi.cmdLine = cmdLine.str();
	spi.cwd.assign(td->scriptDir);
  
	spi.stdError = stdOutFile.getHandle();
	spi.stdIn = stdInFile.getHandle();
	spi.stdOut = stdOutFile.getHandle();
	spi.envString=td->buffer2->GetBuffer();
  
  /*! Execute the CGI process. */
  {
    Process cgiProc;
    if( cgiProc.execHiddenProcess(&spi, cgi_timeout) )
    {
      stdInFile.closeFile();
      stdOutFile.closeFile();
      ((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
      ((Vhost*)(td->connection->host))->warningsLogWrite
                                       ("Error in the CGI execution\r\n");
      ((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
      return ((Http*)td->lhttp)->raiseHTTPError(td, s, e_500);
    }
  }
  /*! Reset the buffer2 length counter. */
	td->buffer2->SetLength(0);

	/*! Read the CGI output.  */
	nBytesRead=0;

  /*! Return an internal error if we cannot seek on the file. */
	if(stdOutFile.setFilePointer(0))
  {
    stdInFile.closeFile();
		stdOutFile.closeFile();
		((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((Vhost*)(td->connection->host))->warningsLogWrite
                               ("Error setting file pointer\r\n");
		((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		return ((Http*)td->lhttp)->raiseHTTPError(td, s, e_500);
  }
  
  if(stdOutFile.readFromFile(td->buffer2->GetBuffer(), 
                             td->buffer2->GetRealLength()-1, &nBytesRead))
  {
    stdInFile.closeFile();
		stdOutFile.closeFile();
		((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((Vhost*)(td->connection->host))->warningsLogWrite
                               ("Error reading from CGI std out file\r\n");
		((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		return ((Http*)td->lhttp)->raiseHTTPError(td, s, e_500);
  }
		
	(td->buffer2->GetBuffer())[nBytesRead]='\0';
		
	if(nBytesRead==0)
	{
		((Vhost*)(td->connection->host))->warningslogRequestAccess(td->id);
		((Vhost*)td->connection->host)->warningsLogWrite("Error CGI zero bytes read\r\n" );
		((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
		((Http*)td->lhttp)->raiseHTTPError(td, s, e_500);
		yetoutputted=1;
	}
	/*! Standard CGI can include an extra HTTP header.  */
	headerSize=0;

	for(u_long i=0; i<nBytesRead; i++)
	{
		char *buff=td->buffer2->GetBuffer();
		if( (buff[i]=='\r') && (buff[i+1]=='\n') 
        && (buff[i+2]=='\r') && (buff[i+3]=='\n') )
		{
			/*!
       *The HTTP header ends with a \r\n\r\n sequence so 
       *determinate where it ends and set the header size
       *to i + 4.
       */
			headerSize= i + 4 ;
			break;
		}
		/*! If it is present Location: xxx in the header send a redirect to xxx.  */
		else if(!strncmp(&(td->buffer2->GetBuffer())[i], "Location:", 9))
		{
      /*! If no other data was send, send the redirect. */
			if(!yetoutputted)
			{
        string nURL;
        u_long len = 0;

        while( (td->buffer2->GetBuffer())[i + len + 9] != '\r' )
        {
            len++;
        }
        nURL.assign(&(td->buffer2->GetBuffer()[i + 9]), len);
				((Http*)td->lhttp)->sendHTTPRedirect(td, s, nURL.c_str());
        /*! Store the new flag. */
        yetoutputted=1;

      }
		}
	}

	if(!yetoutputted)
	{
		u_long nbw=0;
		if(!lstrcmpi(td->request.CONNECTION.c_str(), "Keep-Alive"))
			td->response.CONNECTION.assign("Keep-Alive");
		/*!
     *Do not send any other HTTP header if the CGI executable
     *has the nph-. form name.  
     */
		if(nph)
		{
			/*! Resetting the structure we send only the information gived by the CGI. */
			HttpHeaders::resetHTTPResponse(&(td->response));
      td->response.VER.assign(td->request.VER.c_str());
		}

    /*! If we have not to append the output send data directly to the client. */
		if(!td->appendOutputs)
		{
      ostringstream tmp;
      /*! Send the header.  */
			if(headerSize)
				HttpHeaders::buildHTTPResponseHeaderStruct(&td->response, td, 
                                                    td->buffer2->GetBuffer());
      /*! Always specify the size of the HTTP contents.  */
			tmp << (u_int) (stdOutFile.getFileSize()-headerSize);
      td->response.CONTENT_LENGTH.assign(tmp.str());
			HttpHeaders::buildHTTPResponseHeader(td->buffer->GetBuffer(),
                                            &td->response);

			td->buffer->SetLength((u_int)strlen(td->buffer->GetBuffer()));

			if(s->socket.send(td->buffer->GetBuffer(),
                        (int)(td->buffer->GetLength()), 0)==SOCKET_ERROR)
      {
        stdInFile.closeFile();
        stdOutFile.closeFile();
        File::deleteFile(td->inputDataPath);
        /*! Remove the connection on sockets error. */
        return 0;
      }
      if(only_header)
      {
        stdOutFile.closeFile();
        stdInFile.closeFile();
        File::deleteFile(td->inputDataPath);
        return 1;
      }

      /*! Send other remaining data in the buffer. */
			if(s->socket.send((td->buffer2->GetBuffer() + headerSize), 
                        nBytesRead-headerSize, 0)==SOCKET_ERROR)
      {
        stdOutFile.closeFile();
        stdInFile.closeFile();
        File::deleteFile(td->inputDataPath);
        /*! Remove the connection on sockets error. */
        return 0;       
      }
		}
		else
    {
      if(only_header)
      {
        stdOutFile.closeFile();
        stdInFile.closeFile();
        File::deleteFile(td->inputDataPath);
        return 1;
      }

      /*! Do not put the HTTP header if appending. */
			td->outputData.writeToFile(td->buffer2->GetBuffer()+headerSize, 
                                 nBytesRead-headerSize, &nbw);
    }
    do
    {
      /*! Flush other data. */
      if(stdOutFile.readFromFile(td->buffer2->GetBuffer(), 
                                 td->buffer2->GetRealLength(), &nBytesRead))
      {
        stdOutFile.closeFile();
        stdInFile.closeFile();
        File::deleteFile(td->inputDataPath);
        /*! Remove the connection on sockets error. */
        return 0;      
      }
      if(nBytesRead)
      {

        if(!td->appendOutputs)
        {
          if(s->socket.send(td->buffer2->GetBuffer(), nBytesRead, 0)
             ==SOCKET_ERROR)
          {
            stdOutFile.closeFile();
            stdInFile.closeFile();
            File::deleteFile(td->inputDataPath);
            /*! Remove the connection on sockets error. */
            return 0;      
          }
        }
        else
        {
          if(td->outputData.writeToFile(td->buffer2->GetBuffer(), 
                                        nBytesRead, &nbw))
          {
            stdOutFile.closeFile();
            stdInFile.closeFile();
            File::deleteFile(td->inputDataPath);
            /*! Remove the connection on sockets error. */
            return 0;      
          }
        }
      }

		}while(nBytesRead);

	}
	
	/*! Close and delete the stdin and stdout files used by the CGI.  */
	stdOutFile.closeFile();
	stdInFile.closeFile();
	File::deleteFile(td->inputDataPath);
	return 1;  
}

/*!
 *Write the string that contain the CGI environment to cgiEnvString.
 *This function is used by other server side protocols too.
 */
void Cgi::buildCGIEnvironmentString(HttpThreadContext* td, char *cgi_env_string, 
                                    int processEnv)
{
	/*!
   *The Environment string is a null-terminated block of null-terminated strings.
   *For no problems with the function strcat we use the character \r for 
   *the \0 character and at the end we change every \r in \0.
   */
	CMemBuf memCgi;
	char strTmp[32];

	memCgi.SetExternalBuffer(cgi_env_string, td->buffer2->GetRealLength());
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
	/*! *Must use REDIRECT_STATUS for php and others.  */
	memCgi << end_str << "REDIRECT_STATUS=TRUE";
	
	memCgi << end_str << "SERVER_NAME=";
 	memCgi << lserver->getServerName();

	memCgi << end_str << "SERVER_SIGNATURE=";
	memCgi << "<address>MyServer ";
	memCgi << versionOfSoftware;
	memCgi << "</address>";

	memCgi << end_str << "SERVER_PROTOCOL=";
	memCgi << td->request.VER.c_str();	
	
	memCgi << end_str << "SERVER_PORT="<< CMemBuf::UIntToStr(
                                                 td->connection->getLocalPort());

	memCgi << end_str << "SERVER_ADMIN=";
	memCgi << lserver->getServerAdmin();

	memCgi << end_str << "REQUEST_METHOD=";
	memCgi << td->request.CMD.c_str();

	memCgi << end_str << "REQUEST_URI=";
	
 	memCgi << td->request.URI.c_str();

	memCgi << end_str << "QUERY_STRING=";
	memCgi << td->request.URIOPTS.c_str();

	memCgi << end_str << "GATEWAY_INTERFACE=CGI/1.1";

	if(td->request.CONTENT_TYPE.length())
	{
		memCgi << end_str << "CONTENT_TYPE=";
		memCgi << td->request.CONTENT_TYPE.c_str();
	}

	if(td->request.CONTENT_LENGTH.length())
	{
		memCgi << end_str << "CONTENT_LENGTH=";
		memCgi << td->request.CONTENT_LENGTH.c_str();
	}
	else
	{
		u_long fs=0;
		if(td->inputData.getHandle())
			fs=td->inputData.getFileSize();
		memCgi << end_str << "CONTENT_LENGTH=" << CMemBuf::UIntToStr(fs);
	}

	if(td->request.COOKIE.length())
	{
		memCgi << end_str << "HTTP_COOKIE=";
		memCgi << td->request.COOKIE.c_str();
	}

	if(td->request.RANGEBYTEBEGIN || td->request.RANGEBYTEEND)
	{
    ostringstream rangeBuffer;
		memCgi << end_str << "HTTP_RANGE=" << td->request.RANGETYPE << "=" ;
    if(td->request.RANGEBYTEBEGIN)
    {
      rangeBuffer << (int)td->request.RANGEBYTEBEGIN;
      memCgi << rangeBuffer.str();
    }
    memCgi << "-";
    if(td->request.RANGEBYTEEND)
    {
      rangeBuffer << td->request.RANGEBYTEEND;
      memCgi << rangeBuffer.str();
    }   

	}

	if(td->request.REFERER.length())
	{
		memCgi << end_str << "HTTP_REFERER=";
		memCgi << td->request.REFERER.c_str();
	}

	if(td->request.CACHE_CONTROL.length())
	{
		memCgi << end_str << "HTTP_CACHE_CONTROL=";
		memCgi << td->request.CACHE_CONTROL.c_str();
	}

	if(td->request.ACCEPT.length())
	{
		memCgi << end_str << "HTTP_ACCEPT=";
		memCgi << td->request.ACCEPT.c_str();
	}

	if(td->cgiRoot.length())
	{
		memCgi << end_str << "CGI_ROOT=";
		memCgi << td->cgiRoot;
	}

	if(td->request.HOST.length())
	{
		memCgi << end_str << "HTTP_HOST=";
		memCgi << td->request.HOST.c_str();
	}

	if(td->connection->getipAddr()[0])
	{
		memCgi << end_str << "REMOTE_ADDR=";
		memCgi << td->connection->getipAddr();
	}

	if(td->connection->getPort())
	{
	 	memCgi << end_str << "REMOTE_PORT=";
		memCgi << CMemBuf::UIntToStr(td->connection->getPort() );
	}

	if(td->connection->getLogin()[0])
	{
    memCgi << end_str << "REMOTE_USER=";
		memCgi << td->connection->getLogin();
	}
	
	if(((Vhost*)(td->connection->host))->protocol==PROTOCOL_HTTPS)
		memCgi << end_str << "SSL=ON";
	else
		memCgi << end_str << "SSL=OFF";

	if(td->request.CONNECTION.length())
	{
		memCgi << end_str << "HTTP_CONNECTION=";
		memCgi << td->request.CONNECTION.c_str();
	}

	if(td->request.AUTH.length())
	{
		memCgi << end_str << "AUTH_TYPE=";
		memCgi << td->request.AUTH.c_str();
	}

	if(td->request.USER_AGENT.length())
	{
		memCgi << end_str << "HTTP_USER_AGENT=";
		memCgi << td->request.USER_AGENT.c_str();
	}

	if(td->request.ACCEPTENC.length())
	{
		memCgi << end_str << "HTTP_ACCEPT_ENCODING=";
		memCgi << td->request.ACCEPTENC.c_str();
	}

	if(td->request.ACCEPTLAN.length())
	{
		memCgi << end_str << "HTTP_ACCEPT_LANGUAGE=";
    memCgi << td->request.ACCEPTLAN.c_str();
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
	
	/*!
   *For the DOCUMENT_URI and SCRIPT_NAME copy the 
   *requested URI without the pathInfo.
   */
	memCgi << end_str << "SCRIPT_NAME=";
	memCgi << td->request.URI.c_str();

	memCgi << end_str << "SCRIPT_URL=";
	memCgi << td->request.URI.c_str();

	memCgi << end_str << "DATE_GMT=";
	getRFC822GMTTime(strTmp, HTTP_RESPONSE_DATE_DIM);
	memCgi << strTmp;

 	memCgi << end_str << "DATE_LOCAL=";
	getRFC822LocalTime(strTmp, HTTP_RESPONSE_DATE_DIM);
	memCgi << strTmp;

	memCgi << end_str << "DOCUMENT_ROOT=";
	memCgi << ((Vhost*)(td->connection->host))->documentRoot;

	memCgi << end_str << "DOCUMENT_URI=";
	memCgi << td->request.URI.c_str();
	
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
		lpvEnv = lserver->envString; 
		memCgi << end_str;
		for (lpszVariable = (LPTSTR) lpvEnv; *lpszVariable; lpszVariable++) 
		{ 
			if(((char*)lpszVariable)[0]  != '=' )
			{
				memCgi << (char*)lpszVariable << end_str;
			}
			while (*lpszVariable)*lpszVariable++;
		} 
	}
#endif

	memCgi << end_str << end_str  << end_str  << end_str  << end_str  ;
}

/*!
 *Set the CGI timeout for the new processes.
 */
void Cgi::setTimeout(int nt)
{
   cgi_timeout = nt;
}

/*!
 *Get the timeout value for CGI processes.
 */
int Cgi::getTimeout()
{
  return cgi_timeout;
}
