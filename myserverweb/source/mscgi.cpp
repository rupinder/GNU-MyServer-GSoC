/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
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
#include "../include/cserver.h"
#include "../include/security.h"
#include "../include/cgi.h"
#include "../include/AMMimeUtils.h"
#include "../include/filemanager.h"
#include "../include/sockets.h"
#include "../include/utility.h"


/*!
 *Sends the MyServer CGI; differently from standard CGI this don't 
 *need a new process to run so it is faster.
 */
int MsCgi::send(HttpThreadContext* td, ConnectionPtr s,char* exec,
                char* cmdLine, int /*execute*/, int only_header)
{
	/*!
   *This is the code for manage a .mscgi file.
   *This files differently from standard CGI don't need a new process to run
   *but are allocated in the caller process virtual space.
   *Usually these files are faster than standard CGI.
   *Actually myServerCGI(.mscgi) is only at an alpha status.
   */

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
  int nbs=0;
	MsCgiData data;
  int scriptDirLen = 0;
  int scriptFileLen = 0;
  int cgiRootLen = 0;
  int cgiFileLen = 0;
  int scriptpathLen = strlen(exec) + 1;
	char *outDataPath=0;
	data.envString=td->request.URIOPTSPTR ?
                    td->request.URIOPTSPTR : (char*) td->buffer->GetBuffer();
	
	data.td = td;
	data.errorPage=0;

  if(td->scriptPath)
    delete [] td->scriptPath;
  td->scriptPath = 0;
  td->scriptPath = new char[scriptpathLen];
  if(td->scriptPath == 0)
    return 0;
	lstrcpy(td->scriptPath, exec);

  File::splitPathLength(exec, &scriptDirLen, &scriptFileLen);
  File::splitPathLength(exec, &cgiRootLen, &cgiFileLen);

  if(td->scriptDir)
    delete [] td->scriptDir;
  td->scriptDir = new char[scriptDirLen+1];
  if(td->scriptDir == 0)
    return 0;

  if(td->scriptFile)
    delete [] td->scriptFile;
  td->scriptFile = new char[scriptFileLen+1];
  if(td->scriptFile == 0)
    return 0;

  if(td->cgiRoot)
    delete [] td->cgiRoot;
  td->cgiRoot = new char[cgiRootLen+1];
  if(td->cgiRoot == 0)
    return 0;

  if(td->cgiFile)
    delete [] td->cgiFile;
  td->cgiFile = new char[cgiFileLen+1];
  if(td->cgiFile == 0)
    return 0;

	File::splitPath(exec, td->scriptDir, td->scriptFile);
	File::splitPath(exec, td->cgiRoot, td->cgiFile);

	Cgi::buildCGIEnvironmentString(td,data.envString);
	
	if(!td->appendOutputs)
	{	
    int wdLen =  getdefaultwdlen();
    int outDataPathLen = wdLen + 24;
    outDataPath=new char[ outDataPathLen ];
    if(outDataPath == 0)
    {
      return ((Http*)td->lhttp)->raiseHTTPError(td,s,e_500);
    }
		getdefaultwd(outDataPath, outDataPathLen );	
		sprintf(&(outDataPath)[wdLen-1],"/stdOutFileMSCGI_%u",(u_int)td->id);
		if(data.stdOut.openFile(outDataPath, FILE_CREATE_ALWAYS | 
                            FILE_OPEN_READ | FILE_OPEN_WRITE))
    {
      delete [] outDataPath;
      return ((Http*)td->lhttp)->raiseHTTPError(td,s,e_500);
    
    }
	}
	else
	{
		data.stdOut.setHandle(td->outputData.getHandle());
	}

	ret = hinstLib.loadLibrary(exec);

	if (!ret) 
	{ 
		/*!
     *Set the working directory to the MSCGI file one.
     */
		setcwd(td->scriptDir);
		td->buffer2->GetAt(0)='\0';

		ProcMain = (CGIMAIN) hinstLib.getProc( "main"); 

		if(ProcMain)
		{
			(ProcMain)(cmdLine,&data);
		}
    else
      {
        ((Vhost*)td->connection->host)->warningslogRequestAccess(td->id);
        ((Vhost*)td->connection->host)->warningsLogWrite("Error accessing entrypoint: ");
        ((Vhost*)td->connection->host)->warningsLogWrite(exec);
        ((Vhost*)td->connection->host)->warningsLogWrite("\r\n");
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
      File::deleteFile(outDataPath);
    }
    if(outDataPath)
      delete [] outDataPath;
    /*! Internal server error. */
    return ((Http*)td->lhttp)->raiseHTTPError(td,s,e_500);
	}
	if(data.errorPage)
	{
		int errID=getErrorIDfromHTTPStatusCode(data.errorPage);
		if(errID!=-1)
    {
			data.stdOut.closeFile();
			File::deleteFile(outDataPath);
      if(outDataPath)
        delete [] outDataPath;
			return ((Http*)td->lhttp)->raiseHTTPError(td,s,errID);
    }
	}
	/*!
   *Compute the response length.
   */
	sprintf(td->response.CONTENT_LENGTH,"%u",(u_int)data.stdOut.getFileSize());

	/*!
   *Send all the data to the client if the append is not used.
   */
	if(!td->appendOutputs)
	{
		char *buffer = td->buffer2->GetBuffer();
		u_long bufferSize= td->buffer2->GetRealLength();
		data.stdOut.setFilePointer(0);
		HttpHeaders::buildHTTPResponseHeader(buffer,&(td->response));
		if(s->socket.send(buffer,(int)strlen(buffer), 0)==SOCKET_ERROR)
		{
			if(!td->appendOutputs)
			{
				data.stdOut.closeFile();
				File::deleteFile(outDataPath);
        if(outDataPath)
          delete [] outDataPath;
			}

      /*! Internal server error. */
      return ((Http*)td->lhttp)->raiseHTTPError(td,s,e_500);
		}
    if(only_header)
    {
      return 1;
    }
		do
		{
			data.stdOut.readFromFile(buffer,bufferSize,&nbr);
			if(nbr)
			{
				nbs=s->socket.send(buffer,nbr,0);
				if(nbs==-1)
				{
					if(!td->appendOutputs)
					{
						data.stdOut.closeFile();
						File::deleteFile(outDataPath);
          }
          if(outDataPath)
            delete [] outDataPath;	
          /*! Internal server error. */
          return ((Http*)td->lhttp)->raiseHTTPError(td,s,e_500);
				}	
			}
		}while(nbr && nbs);
		if(!td->appendOutputs)
		{
			data.stdOut.closeFile();
			File::deleteFile(outDataPath);
		}
	}
  if(outDataPath)
    delete [] outDataPath;
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
int MsCgi::load()
{
  int ret=1;
#ifdef WIN32
  ret =	mscgiModule.loadLibrary("CGI-LIB\\CGI-LIB.dll", 1);
#endif

#ifdef HAVE_DL
	char *mscgi_path=0;
	
	if(File::fileExists("cgi-lib/cgi-lib.so"))
	{
    mscgi_path = new char[19];
    if(mscgi_path)
      strcpy(mscgi_path, "cgi-lib/cgi-lib.so");
	}
	else
	{
#ifdef PREFIX
    mscgi_path = new char[strlen(PREFIX)+25];
    if(mscgi_path)
      sprintf(mscgi_path, "%s/lib/myserver/cgi-lib.so", PREFIX );
#else
    mscgi_path = new char[29];
    if(mscgi_path)
      strcpy(mscgi_path, "/usr/lib/myserver/cgi-lib.so");
#endif
	}
  if(mscgi_path)
  {
    ret = mscgiModule.loadLibrary(mscgi_path, 1);
    delete [] mscgi_path;
  }
#endif
	return ret;
}

/*!
*Free the memory allocated by the MSCGI library.
*/
int MsCgi::unload()
{
	/*!
   *Return 1 if the library was closed correctly returns successfully.
   */
	return mscgiModule.close();
}
