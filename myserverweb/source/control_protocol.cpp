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

#include "../include/protocol.h"
#include "../include/control_protocol.h"
#include "../include/cXMLParser.h"
#include "../include/md5.h"
#include "../include/cserver.h"
#include "../include/lfind.h"
#include "../include/protocols_manager.h"
#include "../include/control_errors.h"
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

extern const char *versionOfSoftware;

#ifdef NOT_WIN
#include "../include/lfind.h"

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

// Bloodshed Dev-C++ Helper
#ifndef intptr_t
#define intptr_t int
#endif


char control_protocol::adminLogin[64]="";
char control_protocol::adminPassword[64]="";
int  control_protocol::controlEnabled = 0;

/*!
 *Returns the name of the protocol. If an out buffer is defined 
 *fullfill it with the name too.
 */
char* control_protocol::registerName(char* out,int len)
{
	if(out)
	{
		strncpy(out,"CONTROL",len);
	}
	return "CONTROL";
}

/*!
 *Class constructor.
 */
control_protocol::control_protocol() 
{
  Ifile=0;
  Ofile=0;
	PROTOCOL_OPTIONS = 0;
}

/*!
 *Destructor for the class.
 */
control_protocol::~control_protocol()
{

}

/*!
 *Load the control protocol.
 */
int control_protocol::loadProtocol(cXMLParser* languageParser, char* /*confFile*/)
{
  char tmpName[64];
  char tmpPassword[64];
  tmpName[0]='\0';
  tmpPassword[0]='\0';

  /*! Is the value in the config file still in MD5? */
  int adminNameMD5ized = 0;

  /*! Is the value in the config file still in MD5? */
  int adminPasswordMD5ized = 0;

  char *data = 0;
  char *main_configuration_file = lserver->getMainConfFile();
	cXMLParser configurationFileManager;
	configurationFileManager.open(main_configuration_file);

	data=configurationFileManager.getValue("CONTROL_ENABLED");
	if(data && (!strcmpi(data, "YES")))
	{
    controlEnabled = 1;
	}	
  else
  {
    controlEnabled = 0;
  }

	data=configurationFileManager.getValue("CONTROL_ADMIN");
	if(data)
	{
    strncpy(tmpName, data, 64);
	}	

	data=configurationFileManager.getValue("CONTROL_PASSWORD");
	if(data)
	{
    strncpy(tmpPassword, data, 64);
	}	

	data=configurationFileManager.getAttr("CONTROL_ADMIN", "MD5");
	if(data)
	{
    if(strcmpi(data, "YES") == 0)
      adminNameMD5ized = 1;
	}	

	data=configurationFileManager.getAttr("CONTROL_PASSWORD", "MD5");
	if(data)
	{
    if(strcmpi(data, "YES") == 0)
      adminPasswordMD5ized = 1;
	}	
  MYSERVER_MD5Context md5;

  if(adminNameMD5ized)
  {
    strncpy(adminLogin, tmpName, 64);
  }
  else
  {
    MYSERVER_MD5Init(&md5);
    MYSERVER_MD5Update(&md5,(const unsigned  char*) tmpName, 
                       (unsigned)strlen(tmpName));
    MYSERVER_MD5End(&md5, adminLogin);
  }

  if(adminPasswordMD5ized)
  {
    strncpy(adminPassword, tmpPassword, 64);
  }
  else
  {
    MYSERVER_MD5Init(&md5);
    MYSERVER_MD5Update(&md5,(const unsigned  char*) tmpPassword, 
                       (unsigned)strlen(tmpPassword));
    MYSERVER_MD5End(&md5, adminPassword);
  }

  
	configurationFileManager.close();

}

/*!
 *Check if the client is allowed to connect to.
 *Return 1 if the client is allowed.
 */
int control_protocol::checkAuth()
{
  char authLoginHeaderMD5[64];
  char authPasswordHeaderMD5[64];

  /*! Return 0 if we haven't enabled the service. */
  if(!controlEnabled)
    return 0;

  MYSERVER_MD5Context md5;

  MYSERVER_MD5Init(&md5);
  MYSERVER_MD5Update(&md5,(const unsigned  char*)  header.getAuthLogin(), 
                       (unsigned)strlen( header.getAuthLogin()));
  MYSERVER_MD5End(&md5, authLoginHeaderMD5);

  MYSERVER_MD5Init(&md5);
  MYSERVER_MD5Update(&md5,(const unsigned  char*)  header.getAuthPassword(), 
                       (unsigned)strlen( header.getAuthPassword()));
  MYSERVER_MD5End(&md5, authPasswordHeaderMD5);

  if((!strcmpi(adminLogin, authLoginHeaderMD5)) &&
     (!strcmpi(adminPassword, authPasswordHeaderMD5)) )
    return 1;
  else
    return 0;
}

/*!
 *Control the connection.
 */
int control_protocol::controlConnection(LPCONNECTION a, char *b1, char *b2, int bs1, 
                                        int bs2, u_long nbtr, u_long id)
{
  int ret;
  int realHeaderLength;
  int dataWritten = 0;
  char *IfilePath=0;
  char *OfilePath=0;

	if(a->getToRemove())
	{
		switch(a->getToRemove())
		{
      /*! Remove the connection from the list. */
			case CONNECTION_REMOVE_OVERLOAD:
        sendResponse(b2, bs2, a, CONTROL_SERVER_BUSY, 0);
				return 0;
      default:
        return 0;
		}
	}


  ret = header.parse_header(b1, nbtr, &realHeaderLength);

  /*! 
   *On errors remove the connection from the connections list.
   *For return values look at control_errors.h.
   *Returning 0 from the controlConnection we will remove the connection
   *from the active connections list.
   */
  if(ret != CONTROL_OK)
  {
    /*! parse_header returns -1 on an incomplete header. */
    if(ret == -1)
    {
      return 2;
    }
    sendResponse(b2, bs2, a, ret,0);
    return 0;
  }
  u_long nbw;
  int specified_length = header.getLength();
  char *version = header.getVersion();
  u_long timeout=get_ticks();
  if(specified_length)
  {
    Ifile = new MYSERVER_FILE();
    if(Ifile == 0)
    {
      sendResponse(b2, bs2, a, CONTROL_INTERNAL, 0);
      return 0;                                   
    }
    int IfilePathLen = getdefaultwdlen() + 20;
    IfilePath = new char[IfilePathLen];
    if(IfilePath == 0)
    {
      delete Ifile;
      sendResponse(b2, bs2, a, CONTROL_INTERNAL, 0);
      return 0;                                   
    }
    getdefaultwd(IfilePath, IfilePathLen);

    sprintf(&(IfilePath)[strlen(IfilePath)], "/ControlInput_%u", (u_int) id);
  
    ret = Ifile->createTemporaryFile(IfilePath);
    if(ret)
    {
      sendResponse(b2, bs2, a, CONTROL_INTERNAL, 0);
      Ifile->closeFile();
      MYSERVER_FILE::deleteFile(IfilePath);
      delete [] IfilePath;
      delete Ifile;
      return 0;
    }
    if(nbtr - realHeaderLength)
    {
      ret = Ifile->writeToFile(b1 + realHeaderLength, nbtr - realHeaderLength, 
                               &nbw);
      dataWritten += nbw;
      if(ret)
      {
        sendResponse(b2, bs2, a, CONTROL_INTERNAL, 0);
        Ifile->closeFile();
        if(IfilePath)
        {
          MYSERVER_FILE::deleteFile(IfilePath);
          delete [] IfilePath;
        }
        delete Ifile;
        Ifile=0;
        return 0;
      }
    }
  }

  /*! Check if there are other bytes waiting to be read. */
  if(specified_length && (specified_length != nbtr - realHeaderLength))
  {
    /*! Check if we can read all the specified data. */
    while(specified_length != nbtr - realHeaderLength)
    {
      if(a->socket.bytesToRead())
      {
        ret = a->socket.recv(b2, bs2, 0);
        if(ret == -1)
        {
          sendResponse(b2, bs2, a, CONTROL_INTERNAL, 0);
          Ifile->closeFile();
          if(IfilePath)
          {
            MYSERVER_FILE::deleteFile(IfilePath);
            delete [] IfilePath;
          }
          delete Ifile;
          Ifile=0;
          return -1;
        }
        ret = Ifile->writeToFile(b2, ret, &nbw);
        dataWritten += nbw;
        if(ret)
        {
          sendResponse(b2, bs2, a, CONTROL_INTERNAL, 0);
          Ifile->closeFile();
          if(IfilePath)
          {
            MYSERVER_FILE::deleteFile(IfilePath);
            delete [] IfilePath;
          }
          delete Ifile;
          Ifile=0;
        }
      
        if(dataWritten >=  specified_length)
          break;
        timeout = get_ticks();
      }
      else if(get_ticks() - timeout > SEC(5))
      {
        sendResponse(b2, bs2, a, CONTROL_BAD_LEN, 0);
        Ifile->closeFile();
        if(IfilePath)
        {
          MYSERVER_FILE::deleteFile(IfilePath);
          delete [] IfilePath;
        }
        delete Ifile;
        Ifile=0;
        return 0;
      }
      else
      {
        /*! Wait a bit. */
        wait(2);
      }
    }
  }
  if(Ifile)
  {
    Ifile->setFilePointer(0);
  }

  if(strcmpi(version, "CONTROL/1.0"))
  {
    if(Ifile)
    {
      Ifile->closeFile();
      if(IfilePath)
      {
        MYSERVER_FILE::deleteFile(IfilePath);
        delete [] IfilePath;
      }
      delete Ifile;
      Ifile=0;
    }
    sendResponse(b2, bs2, a, CONTROL_BAD_VERSION, 0);
    return 0;
  }
     
  int authorized = checkAuth();

  /*! 
   *If the client is not authorized remove the connection.
   */
  if(authorized ==0)
  {
    if(Ifile)
    {
      Ifile->closeFile();
      if(IfilePath)
      {
        MYSERVER_FILE::deleteFile(IfilePath);
        delete [] IfilePath;
      }
      delete Ifile;
      Ifile=0;
    }
    sendResponse(b2, bs2, a, CONTROL_AUTH, 0);
    return 0;
  }
  /*! 
   *If the specified length is different from the length that the 
   *server can read, remove the connection.
   */
  if(dataWritten != specified_length)
  {
    if(Ifile)
    {
      Ifile->closeFile();
      if(IfilePath)
      {
        MYSERVER_FILE::deleteFile(IfilePath);
        delete [] IfilePath;
      }
      delete Ifile;
      Ifile=0;
    }
    sendResponse(b2, bs2, a, CONTROL_BAD_LEN, 0);
    return 0;
  }

  char *command = header.getCommand();
  char *opt     = header.getOptions();

  /*! Is the specified command a know one? */
  int knownCommand = 0;

  /*! 
   *Create an out file. This can be used by commands that
   *needs it.
   */
  Ofile = new MYSERVER_FILE();
  if(Ofile == 0)
  {
    sendResponse(b2, bs2, a, CONTROL_INTERNAL, 0);
    Ifile->closeFile();
    if(IfilePath)
    {
      MYSERVER_FILE::deleteFile(IfilePath);
      delete [] IfilePath;
    }
      delete Ifile;
    Ifile=0;
    return 0;                                   
  }
  int OfilePathLen = getdefaultwdlen() + 20;
  OfilePath = new char[OfilePathLen];
  if(OfilePath == 0)
  {

    delete Ofile;
    Ifile->closeFile();
    if(IfilePath)
    {
      MYSERVER_FILE::deleteFile(IfilePath);
      delete [] IfilePath;
    }
    delete Ifile;
    Ifile=0;
    sendResponse(b2, bs2, a, CONTROL_INTERNAL, 0);
    return 0;                                   
  }
  getdefaultwd(OfilePath, OfilePathLen);

  sprintf(&(OfilePath)[strlen(OfilePath)], "/ControlOutput_%u", (u_int) id);
  
  ret = Ofile->createTemporaryFile(OfilePath);
  if(ret)
  {
    sendResponse(b2, bs2, a, CONTROL_INTERNAL, 0);
    delete Ofile;
    Ifile->closeFile();
    delete Ifile;
    if(IfilePath)
    {
      MYSERVER_FILE::deleteFile(IfilePath);
      delete [] IfilePath;
    }
    if(OfilePath)
    {
      MYSERVER_FILE::deleteFile(OfilePath);
      delete [] OfilePath;
    }
    Ifile=0;
    Ofile=0;
    return 0;
  }
  if(!strcmp(command, "SHOWCONNECTIONS"))
  {
    knownCommand = 1;
    ret = SHOWCONNECTIONS(Ofile, b1, bs1);
  }
  else if(!strcmp(command, "KILLCONNECTION"))
  {
    knownCommand = 1;
    char buff[11];
    strncpy(buff, header.getOptions(), 10 );
    buff[10] = '\0';
    u_long ID = header.getOptions() ? atol(buff) : 0;
    ret = KILLCONNECTION( ID ,Ofile, b1, bs1);
  }
  else if(!strcmp(command, "REBOOT"))
  {
    knownCommand = 1;
    lserver->rebootOnNextLoop();
    ret = 0;
  }
  else if(!strcmp(command, "GETFILE"))
  {
    knownCommand = 1;
    ret = GETFILE(header.getOptions(), Ifile, Ofile, b1, bs1);
  }
  else if(!strcmp(command, "PUTFILE"))
  {
    knownCommand = 1;
    ret = PUTFILE(header.getOptions(), Ifile, Ofile, b1, bs1);
  }
  else if(!strcmp(command, "SHOWLANGUAGEFILES"))
  {
    knownCommand = 1;
    ret = SHOWLANGUAGEFILES(Ofile, b1, bs1);
  }
  else if(!strcmp(command, "SHOWDYNAMICPROTOCOLS"))
  {
    knownCommand = 1;
    ret = SHOWDYNAMICPROTOCOLS(Ofile, b1, bs1);
  }
  else if(!strcmp(command, "VERSION"))
  {
    knownCommand = 1;
    ret = GETVERSION(Ofile, b1, bs1);
  }

  if(knownCommand)
  {
    Ofile->setFilePointer(0);
    if(ret)
      sendResponse(b2, bs2, a, CONTROL_INTERNAL, 0);
    else
      sendResponse(b2, bs2, a, CONTROL_OK, Ofile);
    if(Ifile)
    {
      Ifile->closeFile();
      delete Ifile;
      Ifile=0;
    }
    if(Ofile)
    {
      Ofile->closeFile();
      delete Ofile;
      Ofile=0;
    }
    if(OfilePath)
    {
      MYSERVER_FILE::deleteFile(OfilePath);
      delete [] OfilePath;
    }
    if(IfilePath)
    {
      MYSERVER_FILE::deleteFile(IfilePath);
      delete [] IfilePath;
    }
    char *connection = header.getConnection();
    /*! 
     *If the Keep-Alive was specified keep the connection in the
     *active connections list.
     */
    if(!strcmpi(connection,"Keep-Alive"))
      return 1;
    else 
      return 0;
  }
  else
  {
    sendResponse(b2, bs2, a, CONTROL_CMD_NOT_FOUND, 0);

    if(Ifile)
    {
      Ifile->closeFile();
      delete Ifile;
      Ifile=0;
    }
    if(Ofile)
    {
      Ofile->closeFile();
      delete Ofile;
      Ofile=0;
    }
    if(OfilePath)
    {
      MYSERVER_FILE::deleteFile(OfilePath);
      delete [] OfilePath;
    }

    if(IfilePath)
    {
      MYSERVER_FILE::deleteFile(IfilePath);
      delete [] IfilePath;
    }
    
    return 0;
  }
}


/*!
 *Send the response with status=errID and the data contained in the outFile.
 *Return nonzero on errors.
 */
int control_protocol::sendResponse(char *buffer, int buffersize, 
                                   LPCONNECTION conn, int errID, 
                                   MYSERVER_FILE* outFile)
{
  u_long dataLength=0;
  int err;
  if(outFile)
    dataLength = outFile->getFileSize();
  /*! Build and send the first line. */
  sprintf(buffer, "/%i\r\n", errID);
  err = conn->socket.send(buffer, strlen(buffer), 0);
  if(err == -1)
    return -1;

  /*! Build and send the Length line. */
  sprintf(buffer, "/LEN %u\r\n", dataLength);
  err = conn->socket.send(buffer, strlen(buffer), 0);
  if(err == -1)
    return -1;

  /*! Send the end of the header. */
  err = conn->socket.send("\r\n", 2, 0);
  if(err == -1)
    return -1;

  /*! Flush the content of the file if any. */
  if(dataLength)
  {
    u_long dataToSend = dataLength;
    u_long nbr;
    for( ; ; )
    {
      err = outFile->readFromFile(buffer, min(dataToSend, buffersize), &nbr);
      if(err)
        return -1;
      dataToSend -= nbr;
      err = conn->socket.send(buffer, nbr, 0);
      if(dataToSend == 0)
        break;
      if(err == -1)
        return -1;
    }
  }

  return 0; 
}

/*!
 *Show the currect active connections.
 */
int  control_protocol::SHOWCONNECTIONS(MYSERVER_FILE* out, char *b1, int bs1)
{
  int ret =  0;
  u_long nbw;
  lserver->connections_mutex_lock();
  LPCONNECTION con = lserver->getConnections();
  while(con)
  {
    sprintf(b1, "%i - %s - %u - %s - %u - %s - %s\r\n", 
            con->getID(),  con->getipAddr(), con->getPort(), con->getlocalIpAddr(),  
            con->getLocalPort(), con->getLogin(), con->getPassword());
    ret = out->writeToFile(b1, strlen(b1), &nbw);   
    con = con->next;
  }
  lserver->connections_mutex_unlock();
  return ret;
}

/*!
 *Kill a connection by its ID.
 */
int  control_protocol::KILLCONNECTION(u_long ID, MYSERVER_FILE* out, 
                                      char *b1, int bs1)
{
  int ret = 0;
  u_long nbw;
  if(ID == 0)
    return -1;
  LPCONNECTION con = lserver->findConnectionByID(ID);
  lserver->connections_mutex_lock();
  if(con)
  {
    /*! Define why the connection is killed. */
    con->setToRemove(CONNECTION_USER_KILL);
  }
  lserver->connections_mutex_unlock();
  return ret;
}

/*!
 *List all the dynamic protocols used by the server.
 */
int control_protocol::SHOWDYNAMICPROTOCOLS(MYSERVER_FILE* out, char *b1,int bs1)
{
  int i = 0;
  dynamic_protocol* dp;
  u_long nbw;
  int ret;
  for( ;;)
  {
    dp = lserver->getProtocolsManager()->getDynProtocolByOrder(i);
    if(dp == 0)
      break;
    sprintf(b1,"%s\r\n", dp->getProtocolName() );
    ret = out->writeToFile(b1, strlen(b1), &nbw);
    if(ret)
      return CONTROL_INTERNAL;
    i++;
  }
  return 0;
}

/*!
 *Return the requested file to the client.
 */
int control_protocol::GETFILE(char* fn, MYSERVER_FILE* in, 
                              MYSERVER_FILE* out, char *b1,int bs1 )
{
  char *filename = 0;
  MYSERVER_FILE localfile;
  int ret = 0;
  if(!lstrcmpi(fn, "myserver.xml"))
  {
    filename = lserver->getMainConfFile();
  }
  else if(!lstrcmpi(fn, "MIMEtypes.xml"))
  {
    filename = lserver->getMIMEConfFile();
  }
  else if(!lstrcmpi(fn, "virtualhosts.xml"))
  {
    filename = lserver->getVhostConfFile();
  }  
  /*! If we cannot find the file send the right error ID. */
  if(!filename)
    return CONTROL_FILE_NOT_FOUND;

  ret = localfile.openFile(fn, MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_IFEXISTS);

  /*! An internal server error happens. */
  if(ret)
    return CONTROL_INTERNAL;
  /*! # of bytes read. */
  u_long nbr = 0;

  /*! # of bytes written. */
  u_long nbw = 0;
  for( ; ;)
  {
    ret = localfile.readFromFile(b1, bs1, &nbr);
    if(ret)
      return CONTROL_INTERNAL;

    /*! Break the loop when we can't read no more data.*/
    if(!nbr)
      break;
    
    ret = Ofile->writeToFile(b1, nbr, &nbw);

    if(ret)
      return CONTROL_INTERNAL;

  }

  localfile.closeFile();
  return 0;
}

/*!
 *Save the file on the local FS.
 */
int control_protocol::PUTFILE(char* fn, MYSERVER_FILE* in, 
                              MYSERVER_FILE* out, char *b1,int bs1 )
{
  char *filename = 0;
  MYSERVER_FILE localfile;
  int ret = 0;
  lserver->disableAutoReboot();
  if(!lstrcmpi(fn, "myserver.xml"))
  {
    filename = lserver->getMainConfFile();
  }
  else if(!lstrcmpi(fn, "MIMEtypes.xml"))
  {
    filename = lserver->getMIMEConfFile();
  }
  else if(!lstrcmpi(fn, "virtualhosts.xml"))
  {
    filename = lserver->getVhostConfFile();
  }  
  /*! If we cannot find the file send the right error ID. */
  if(!filename)
  {
    lserver->enableAutoReboot();
    return CONTROL_FILE_NOT_FOUND;
  }

  ret = MYSERVER_FILE::deleteFile(fn);

  /*! An internal server error happens. */
  if(ret)
  {
    lserver->enableAutoReboot();
    return CONTROL_INTERNAL;
  }

  ret = localfile.openFile(fn, MYSERVER_FILE_OPEN_WRITE | MYSERVER_FILE_OPEN_ALWAYS);

  /*! An internal server error happens. */
  if(ret)
  {
    lserver->enableAutoReboot();
    return CONTROL_INTERNAL;
  }

  /*! # of bytes read. */
  u_long nbr = 0;

  /*! # of bytes written. */
  u_long nbw = 0;
  for( ; ;)
  {
    ret = Ifile->readFromFile(b1, bs1, &nbr);
    if(ret)
    {
      lserver->enableAutoReboot();
      return CONTROL_INTERNAL;
    }

    /*! Break the loop when we can't read no more data.*/
    if(!nbr)
      break;
    
    ret = localfile.writeToFile(b1, nbr, &nbw);

    if(ret)
    {
      lserver->enableAutoReboot();
      return CONTROL_INTERNAL;
    }
  }
  localfile.closeFile();
  lserver->enableAutoReboot();
  return 0;
}

/*!
 *Show all the language files that the server can use.
 */
int control_protocol::SHOWLANGUAGEFILES(MYSERVER_FILE* out, char *b1,int bs1)
{
  char *path = lserver->getLanguagesPath();
  if(path == 0)
    return CONTROL_INTERNAL;
  _finddata_t fd;
  intptr_t ff;
  ff=_findfirst(path, &fd);
  if(ff == -1)
  {
    return CONTROL_INTERNAL;
  }
	do
	  {
	     char *dir;
	     char *filename;
       int dirLen = 0;
       int filenameLen = 0;
       u_long nbw = 0;
       /*! Do not show files starting with a dot. */
	     if(fd.name[0]=='.')
	       continue;
       MYSERVER_FILE::splitPathLength(fd.name, &dirLen, &filenameLen);
       dir = new char[dirLen + 1];
       if(dir == 0)
       {
         _findclose(ff);
         return CONTROL_INTERNAL;
       }
       filename = new char[filenameLen + 1];
       if(filename == 0)
       {
         delete [] dir;
         _findclose(ff);
         return CONTROL_INTERNAL;
       }
	     MYSERVER_FILE::splitPath(fd.name,dir,filename);
       int ret = 0;
	     if(strcmpi(&(filename[strlen(filename) - 3]), "xml") == 0)
       {
	       ret = out->writeToFile(filename, strlen(filename), &nbw);
         if(ret == 0)
           ret = out->writeToFile("\r\n", 2, &nbw);
       }

       delete [] dir;
       delete [] filename;
       if(ret)
       {
         return CONTROL_INTERNAL;
       }
	  }
	while(!_findnext(ff,&fd));
  _findclose(ff);
  return 0;
}

/*!
 *Return the current MyServer version.
 */
int control_protocol::GETVERSION(MYSERVER_FILE* out, char *b1,int bs1)
{
  u_long nbw;
  sprintf(b1, "MyServer %s", versionOfSoftware);
  return Ofile->writeToFile(b1, strlen(b1), &nbw);
}
