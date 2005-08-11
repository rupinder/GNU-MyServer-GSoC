/*
MyServer
Copyright (C) 2002, 2003, 2004 The MyServer Team
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
#include "../include/cserver.h"
#include "../include/security.h"
#include "../include/cgi.h"
#include "../include/AMMimeUtils.h"
#include "../include/filemanager.h"
#include "../include/sockets.h"
#include "../include/utility.h"
#include "../include/filters_chain.h"
#include <sstream>
using namespace std; 

/*!
 *Sends the MyServer CGI; differently from standard CGI this don't 
 *need a new process to run so it is faster.
 */
int MsCgi::send(HttpThreadContext* td, ConnectionPtr s,const char* exec,
                char* cmdLine, int /*execute*/, int onlyHeader)
{
	/*!
   *This is the code for manage a .mscgi file.
   *This files differently from standard CGI don't need a new process to run
   *but are allocated in the caller process virtual space.
   *Usually these files are faster than standard CGI.
   *Actually myServerCGI(.mscgi) is only at an alpha status.
   */
  ostringstream tmpStream;
  ostringstream outDataPath;
  FiltersChain chain;
  u_long nbw;
#ifndef WIN32
#ifdef DO_NOT_USE_MSCGI
	/*!
   *On the platforms where is not available the MSCGI support send a 
   *non implemented error.
   */
	return ((Http*)td->lhttp)->raiseHTTPError(td,s,e_501);
#endif
#endif

#ifndef DO_NOT_USE_MSCGI 
	DynamicLibrary hinstLib; 
  CGIMAIN ProcMain=0;
	u_long nbr=0;
  int ret = 0;
  u_long nbs=0;
	MsCgiData data;
 	data.envString=td->request.uriOptsPtr ?
                    td->request.uriOptsPtr : (char*) td->buffer->getBuffer();
	
	data.td = td;
	data.errorPage=0;

 	td->scriptPath.assign(exec);

  {
    string tmp;
    tmp.assign(exec);
    File::splitPath(tmp, td->cgiRoot, td->cgiFile);
    File::splitPath(exec, td->scriptDir, td->scriptFile);
  }

	Cgi::buildCGIEnvironmentString(td,data.envString);
	
	if(!td->appendOutputs)
	{	
		outDataPath << getdefaultwd(0, 0 ) << "/stdOutFileMSCGI_" << (u_int)td->id;
		if(data.stdOut.openFile(outDataPath.str().c_str(), FILE_CREATE_ALWAYS | 
                                FILE_OPEN_READ | FILE_OPEN_WRITE))
    {
      return ((Http*)td->lhttp)->raiseHTTPError(td,s,e_500);
    }
	}
	else
	{
		data.stdOut.setHandle(td->outputData.getHandle());
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
        ((Vhost*)td->connection->host)->warningsLogWrite("Error loading filters\r\n");
        ((Vhost*)(td->connection->host))->warningslogTerminateAccess(td->id);
        chain.clearAllFilters(); 
        return ((Http*)td->lhttp)->raiseHTTPError(td, s, e_500);
      }
  }

	ret = hinstLib.loadLibrary(exec, 0);

	if (!ret) 
	{ 
		/*!
     *Set the working directory to the MSCGI file one.
     */
		setcwd(td->scriptDir.c_str());
		td->buffer2->getAt(0)='\0';

		ProcMain = (CGIMAIN) hinstLib.getProc( "main"); 

		if(ProcMain)
		{
			(ProcMain)(cmdLine,&data);
		}
    else
    {
      string msg;
      msg.assign("Error accessing entrypoint: ");
      msg.append(exec);
      msg.append("\r\n");
      ((Vhost*)td->connection->host)->warningslogRequestAccess(td->id);
      ((Vhost*)td->connection->host)->warningsLogWrite(msg.c_str());
      ((Vhost*)td->connection->host)->warningslogTerminateAccess(td->id);
    }
		hinstLib.close();

		/*
		*Restore the working directory.
		*/
		setcwd(getdefaultwd(0,0));
	} 
	else
	{
    if(!td->appendOutputs)
    {	
      data.stdOut.closeFile();
      File::deleteFile(outDataPath.str().c_str());
    }
    chain.clearAllFilters(); 
    /*! Internal server error. */
    return ((Http*)td->lhttp)->raiseHTTPError(td,s,e_500);
	}
	if(data.errorPage)
	{
		int errID=getErrorIDfromHTTPStatusCode(data.errorPage);
		if(errID!=-1)
    {
      chain.clearAllFilters(); 
			data.stdOut.closeFile();
			File::deleteFile(outDataPath.str().c_str());
			return ((Http*)td->lhttp)->raiseHTTPError(td, s, errID);
    }
	}
	/*!
   *Compute the response length.
   */
  tmpStream << (u_int)data.stdOut.getFileSize();

  td->response.contentLength.assign(tmpStream.str());
	/*!
   *Send all the data to the client if the append is not used.
   */
	if(!td->appendOutputs)
	{
		char *buffer = td->buffer2->getBuffer();
		u_long bufferSize= td->buffer2->getRealLength();
		data.stdOut.setFilePointer(0);
		HttpHeaders::buildHTTPResponseHeader(buffer,&(td->response));
		if(s->socket.send(buffer,(int)strlen(buffer), 0)==SOCKET_ERROR)
		{
			if(!td->appendOutputs)
			{
				data.stdOut.closeFile();
				File::deleteFile(outDataPath.str().c_str());
			}

      /*! Internal server error. */
      return ((Http*)td->lhttp)->raiseHTTPError(td,s,e_500);
		}

    if(onlyHeader)
    {
      data.stdOut.closeFile();
      File::deleteFile(outDataPath.str().c_str());
      chain.clearAllFilters(); 
      return 1;
    }
		do
		{
			data.stdOut.readFromFile(buffer,bufferSize,&nbr);
			if(nbr)
			{
				if(chain.write(buffer,nbr,&nbs))
				{
					if(!td->appendOutputs)
					{
						data.stdOut.closeFile();
						File::deleteFile(outDataPath.str().c_str());
          }
          chain.clearAllFilters(); 
          /*! Internal server error. */
          return ((Http*)td->lhttp)->raiseHTTPError(td,s,e_500);
				}	
        nbw += nbs;
			}
		}while(nbr && nbs);
		if(!td->appendOutputs)
		{
			data.stdOut.closeFile();
			File::deleteFile(outDataPath.str().c_str());
		}
	}

	{
    ostringstream tmp;
    tmp <<  nbw;
    td->response.contentLength.assign(tmp.str()); 
  } 
  chain.clearAllFilters(); 
	return 1;

#endif
}

/*!
 *Store the MSCGI library module handle.
 */
static DynamicLibrary mscgiModule;

/*!
 *Map the library in the application address space.
 */
int MsCgi::load(XmlParser* /*confFile*/)
{
  int ret=1;
#ifdef WIN32
  ret =	mscgiModule.loadLibrary("CGI-LIB\\CGI-LIB.dll", 1);
#endif

#ifdef HAVE_DL
	ostringstream mscgi_path;
	
	if(File::fileExists("cgi-lib/cgi-lib.so"))
	{
    mscgi_path << "cgi-lib/cgi-lib.so";
	}
	else
	{
#ifdef PREFIX
    mscgi_path << PREFIX << "/lib/myserver/cgi-lib.so";
#else
    mscgi_path << "/usr/lib/myserver/cgi-lib.so";
#endif
	}
  if(mscgi_path.str().length())
  {
    ret = mscgiModule.loadLibrary(mscgi_path.str().c_str(), 1);
  }
#endif
	return ret;
}

/*!
*free the memory allocated by the MSCGI library.
*/
int MsCgi::unload()
{
	/*!
   *Return 1 if the library was closed correctly returns successfully.
   */
	return mscgiModule.close();
}
