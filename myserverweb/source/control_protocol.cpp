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


char ControlProtocol::adminLogin[64]="";
char ControlProtocol::adminPassword[64]="";
int  ControlProtocol::controlEnabled = 0;

/*!
 *Returns the name of the protocol. If an out buffer is defined 
 *fullfill it with the name too.
 */
char* ControlProtocol::registerName(char* out,int len)
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
ControlProtocol::ControlProtocol() 
{
  Ifile=0;
  Ofile=0;
	PROTOCOL_OPTIONS = PROTOCOL_USES_SSL;
  Reboot = true;
}

/*!
 *Destructor for the class.
 */
ControlProtocol::~ControlProtocol()
{

}

/*!
 *Load the control protocol.
 */
int ControlProtocol::loadProtocol(XmlParser* languageParser, char* /*confFile*/)
{
  char tmpName[64];
  char tmpPassword[64];
  tmpName[0]='\0';
  tmpPassword[0]='\0';
  Md5 md5;

  /*! Is the value in the config file still in MD5? */
  int adminNameMD5ized = 0;

  /*! Is the value in the config file still in MD5? */
  int adminPasswordMD5ized = 0;

  char *data = 0;
  char *main_configuration_file = lserver->getMainConfFile();
	XmlParser configurationFileManager;
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

  if(adminNameMD5ized)
  {
    strncpy(adminLogin, tmpName, 64);
  }
  else
  {
    md5.init();
    md5.update((unsigned char const*)tmpName, (unsigned int)strlen(tmpName) );
    md5.end( adminLogin);
  }

  if(adminPasswordMD5ized)
  {
    strncpy(adminPassword, tmpPassword, 64);
  }
  else
  {
    md5.init();
    md5.update((unsigned char const*)tmpPassword,(unsigned int)strlen(tmpPassword) );
    md5.end(adminPassword);
  }

  
	configurationFileManager.close();
  return 0;
}

/*!
 *Check if the client is allowed to connect to.
 *Return 1 if the client is allowed.
 */
int ControlProtocol::checkAuth()
{
  char authLoginHeaderMD5[64];
  char authPasswordHeaderMD5[64];
  char *headerLogin;
  char *headerPassword;
  Md5 md5;
  /*! Return 0 if we haven't enabled the service. */
  if(!controlEnabled)
    return 0;
  headerLogin = header.getAuthLogin();
  headerPassword = header.getAuthPassword();
  authLoginHeaderMD5[0] = authPasswordHeaderMD5[0] = '\0';
  md5.init();
  md5.update((unsigned char const*) headerLogin, (unsigned int)strlen( headerLogin ) );
  md5.end( authLoginHeaderMD5);

  md5.init();
  md5.update((unsigned char const*) headerPassword,(unsigned int)strlen(headerPassword) );
  md5.end(authPasswordHeaderMD5);

  if((!strcmpi(adminLogin, authLoginHeaderMD5)) &&
     (!strcmpi(adminPassword, authPasswordHeaderMD5)) )
    return 1;
  else
    return 0;
}

/*!
 *Control the connection.
 */
int ControlProtocol::controlConnection(ConnectionPtr a, char *b1, char *b2, 
                                        int bs1, int bs2, u_long nbtr, u_long id)
{
  int ret;
  int realHeaderLength;
  int dataWritten = 0;
  char *IfilePath=0;
  char *OfilePath=0;
  ControlProtocol::id = id;
  u_long nbw;
  int specified_length;
  char *version;
  u_long timeout;
  int authorized;
  char *command ;
  char *opt  ;
  int OfilePathLen;

  /*! Is the specified command a know one? */
  int knownCommand;
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
  specified_length = header.getLength();
  version = header.getVersion();
  timeout=get_ticks();
  if(specified_length)
  {
    int IfilePathLen;
    Ifile = new File();
    if(Ifile == 0)
    {
      strcpy(b2,"Error allocating memory");
      addToErrorLog(a,b2, strlen(b2));
      sendResponse(b2, bs2, a, CONTROL_INTERNAL, 0);
      return 0;                                   
    }
    IfilePathLen = getdefaultwdlen() + 20;
    IfilePath = new char[IfilePathLen];
    if(IfilePath == 0)
    {
      strcpy(b2,"Error allocating memory");
      addToErrorLog(a,b2, strlen(b2));
      delete Ifile;
      sendResponse(b2, bs2, a, CONTROL_INTERNAL, 0);
      return 0;                                   
    }
    getdefaultwd(IfilePath, IfilePathLen);

    sprintf(&(IfilePath)[strlen(IfilePath)], "/ControlInput_%u", (u_int) id);
  
    ret = Ifile->createTemporaryFile(IfilePath);
    if(ret)
    {
      strcpy(b2,"Error in the temporary file creation");
      addToErrorLog(a,b2, strlen(b2));
      sendResponse(b2, bs2, a, CONTROL_INTERNAL, 0);
      Ifile->closeFile();
      File::deleteFile(IfilePath);
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
        strcpy(b2,"Error writing to temporary file");
        addToErrorLog(a,b2, strlen(b2));
        sendResponse(b2, bs2, a, CONTROL_INTERNAL, 0);
        Ifile->closeFile();
        if(IfilePath)
        {
          File::deleteFile(IfilePath);
          delete [] IfilePath;
        }
        delete Ifile;
        Ifile=0;
        return 0;
      }
    }
  }

  /*! Check if there are other bytes waiting to be read. */
  if(specified_length && (specified_length != (int)(nbtr - realHeaderLength) ))
  {
    /*! Check if we can read all the specified data. */
    while(specified_length != (int)(nbtr - realHeaderLength))
    {
      if(a->socket.bytesToRead())
      {
        ret = a->socket.recv(b2, bs2, 0);
        if(ret == -1)
        {
          strcpy(b2,"Error in communication");
          addToErrorLog(a,b2, strlen(b2));
          sendResponse(b2, bs2, a, CONTROL_INTERNAL, 0);
          Ifile->closeFile();
          if(IfilePath)
          {
            File::deleteFile(IfilePath);
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
          strcpy(b2,"Error trying to write on the temporary file");
          addToErrorLog(a,b2, strlen(b2));
          sendResponse(b2, bs2, a, CONTROL_INTERNAL, 0);
          Ifile->closeFile();
          if(IfilePath)
          {
            File::deleteFile(IfilePath);
            delete [] IfilePath;
          }
          delete Ifile;
          Ifile=0;
        }
      
        if(dataWritten >=  specified_length)
          break;
        timeout = get_ticks();
      }
      else if(get_ticks() - timeout > MYSERVER_SEC(5))
      {
        strcpy(b2,"Bad content length specified");
        addToErrorLog(a,b2, strlen(b2));
        sendResponse(b2, bs2, a, CONTROL_BAD_LEN, 0);
        Ifile->closeFile();
        if(IfilePath)
        {
          File::deleteFile(IfilePath);
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
    strcpy(b2,"Bad version specified");
    addToErrorLog(a,b2, strlen(b2));
    if(Ifile)
    {
      Ifile->closeFile();
      if(IfilePath)
      {
        File::deleteFile(IfilePath);
        delete [] IfilePath;
      }
      delete Ifile;
      Ifile=0;
    }
    sendResponse(b2, bs2, a, CONTROL_BAD_VERSION, 0);
    return 0;
  }
     
  authorized = checkAuth();

  /*! 
   *If the client is not authorized remove the connection.
   */
  if(authorized ==0)
  {
    strcpy(b2,"Bad authorization");
    addToErrorLog(a,b2, strlen(b2));
    if(Ifile)
    {
      Ifile->closeFile();
      if(IfilePath)
      {
        File::deleteFile(IfilePath);
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
    strcpy(b2,"Bad content length specified");
    addToErrorLog(a,b2, strlen(b2));
    if(Ifile)
    {
      Ifile->closeFile();
      if(IfilePath)
      {
        File::deleteFile(IfilePath);
        delete [] IfilePath;
      }
      delete Ifile;
      Ifile=0;
    }
    sendResponse(b2, bs2, a, CONTROL_BAD_LEN, 0);
    return 0;
  }

  command = header.getCommand();
  opt     = header.getOptions();

  knownCommand = 0;

  /*! 
   *Create an out file. This can be used by commands that
   *needs it.
   */
  Ofile = new File();
  if(Ofile == 0)
  {
    strcpy(b2,"Error in allocating memory");
    addToErrorLog(a,b2, strlen(b2));
    sendResponse(b2, bs2, a, CONTROL_INTERNAL, 0);
    Ifile->closeFile();
    if(IfilePath)
    {
      File::deleteFile(IfilePath);
      delete [] IfilePath;
    }
      delete Ifile;
    Ifile=0;
    return 0;                                   
  }
  OfilePathLen = getdefaultwdlen() + 20;
  OfilePath = new char[OfilePathLen];
  if(OfilePath == 0)
  {
    strcpy(b2,"Error in allocating memory");
    addToErrorLog(a,b2, strlen(b2));
    delete Ofile;
    Ifile->closeFile();
    if(IfilePath)
    {
      File::deleteFile(IfilePath);
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
    strcpy(b2,"Error creating temporary file");
    addToErrorLog(a,b2, strlen(b2));
    sendResponse(b2, bs2, a, CONTROL_INTERNAL, 0);
    delete Ofile;
    Ifile->closeFile();
    delete Ifile;
    if(IfilePath)
    {
      File::deleteFile(IfilePath);
      delete [] IfilePath;
    }
    if(OfilePath)
    {
      File::deleteFile(OfilePath);
      delete [] OfilePath;
    }
    Ifile=0;
    Ofile=0;
    return 0;
  }
  if(!strcmp(command, "SHOWCONNECTIONS"))
  {
    knownCommand = 1;
    ret = SHOWCONNECTIONS(a, Ofile, b1, bs1);
  }
  else if(!strcmp(command, "KILLCONNECTION"))
  {
    char buff[11];
    knownCommand = 1;
    strncpy(buff, header.getOptions(), 10 );
    buff[10] = '\0';
    u_long ID = header.getOptions() ? atol(buff) : 0;
    ret = KILLCONNECTION(a, ID ,Ofile, b1, bs1);
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
    ret = GETFILE(a,header.getOptions(), Ifile, Ofile, b1, bs1);
  }
  else if(!strcmp(command, "PUTFILE"))
  {
    knownCommand = 1;
    ret = PUTFILE(a,header.getOptions(), Ifile, Ofile, b1, bs1);
  }
  else if(!strcmp(command, "DISABLEREBOOT"))
  {
    lserver->disableAutoReboot();
    knownCommand = 1;

  }
  else if(!strcmp(command, "ENABLEREBOOT"))
  {
    lserver->enableAutoReboot();
    knownCommand = 1;

  }
  else if(!strcmp(command, "SHOWLANGUAGEFILES"))
  {
    knownCommand = 1;
    ret = SHOWLANGUAGEFILES(a, Ofile, b1, bs1);
  }
  else if(!strcmp(command, "SHOWDYNAMICPROTOCOLS"))
  {
    knownCommand = 1;
    ret = SHOWDYNAMICPROTOCOLS(a, Ofile, b1, bs1);
  }
  else if(!strcmp(command, "VERSION"))
  {
    knownCommand = 1;
    ret = GETVERSION(a, Ofile, b1, bs1);
  }

  if(knownCommand)
  {
    char *connection;
    Ofile->setFilePointer(0);
    if(ret)
    {
      sendResponse(b2, bs2, a, CONTROL_INTERNAL, 0);
    }
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
      File::deleteFile(OfilePath);
      delete [] OfilePath;
    }
    if(IfilePath)
    {
      File::deleteFile(IfilePath);
      delete [] IfilePath;
    }
    connection = header.getConnection();
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
    strcpy(b2,"Command unknown");
    addToErrorLog(a,b2, strlen(b2));
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
      File::deleteFile(OfilePath);
      delete [] OfilePath;
    }

    if(IfilePath)
    {
      File::deleteFile(IfilePath);
      delete [] IfilePath;
    }
    
    return 0;
  }
}

/*!
 *Add the entry to the error log file.
 */
int ControlProtocol::addToErrorLog(ConnectionPtr con, char *b1, int bs1)
{
	char time[33];
  /*!
   *Check that the verbosity is at least 1.
   */
  if(lserver->getVerbosity() < 1)
    return 0;
	getRFC822GMTTime(time, 32);
  sprintf(b1,"%s [%s] %s:%s:%s - %s\r\n", con->getipAddr(), time, 
          header.getCommand(),  header.getVersion(), header.getOptions(), 
          header.getAuthLogin());
	((Vhost*)(con->host))->warningslogRequestAccess(id);
	((Vhost*)(con->host))->warningsLogWrite(b1);
	((Vhost*)(con->host))->warningslogTerminateAccess(id);
  return 0;
}

/*!
 *Add the entry to the log file.
 */
int ControlProtocol::addToLog(int retCode, ConnectionPtr con, char *b1, int bs1)
{
	char time[33];
	getRFC822GMTTime(time, 32);
  sprintf(b1,"%s [%s] %s:%s:%s - %s  - %i\r\n", con->getipAddr(), time, 
          header.getCommand(),  header.getVersion(), header.getOptions(), 
          header.getAuthLogin() , retCode);
	((Vhost*)(con->host))->accesseslogRequestAccess(id);
	((Vhost*)(con->host))->accessesLogWrite(b1);
	((Vhost*)(con->host))->accesseslogTerminateAccess(id);
  return 0;
}

/*!
 *Send the response with status=errID and the data contained in the outFile.
 *Return nonzero on errors.
 */
int ControlProtocol::sendResponse(char *buffer, int buffersize, 
                                   ConnectionPtr conn, int errID, 
                                   File* outFile)
{
  u_long dataLength=0;
  int err;
  err = addToLog(errID,conn,buffer, buffersize);
  if(err)
  {
    strcpy(buffer,"Error registering the connection to the log");
    addToErrorLog(conn,buffer, strlen(buffer));
    return err;
  }
  if(outFile)
    dataLength = outFile->getFileSize();
  /*! Build and send the first line. */
  sprintf(buffer, "/%i\r\n", errID);
  err = conn->socket.send(buffer, strlen(buffer), 0);
  if(err == -1)
  {
    strcpy(buffer,"Error sending data");
    addToErrorLog(conn,buffer, strlen(buffer));
    return -1;
  }

  /*! Build and send the Length line. */
  sprintf(buffer, "/LEN %u\r\n", (u_int)dataLength);
  err = conn->socket.send(buffer, strlen(buffer), 0);
  if(err == -1)
  {
    strcpy(buffer,"Error sending data");
    addToErrorLog(conn,buffer, strlen(buffer));
    return -1;
  }

  /*! Send the end of the header. */
  err = conn->socket.send("\r\n", 2, 0);
  if(err == -1)
  {
    strcpy(buffer,"Error sending data");
    addToErrorLog(conn,buffer, strlen(buffer));
    return -1;
  }

  /*! Flush the content of the file if any. */
  if(dataLength)
  {
    int dataToSend = dataLength;
    u_long nbr;
    for( ; ; )
    {
      err = outFile->readFromFile(buffer, min(dataToSend, buffersize), &nbr);
      if(err)
      {
        strcpy(buffer,"Error reading from temporary file");
        addToErrorLog(conn,buffer, strlen(buffer));
        return -1;
      }
      dataToSend -= nbr;
      err = conn->socket.send(buffer, nbr, 0);
      if(dataToSend == 0)
        break;
      if(err == -1)
      {
        strcpy(buffer,"Error sending data");
        addToErrorLog(conn,buffer, strlen(buffer));
        return -1;
      }
    }
  }

  return 0; 
}

/*!
 *Show the currect active connections.
 */
int  ControlProtocol::SHOWCONNECTIONS(ConnectionPtr a,File* out, char *b1, 
                                       int bs1)
{
  int ret =  0;
  u_long nbw;
  ConnectionPtr con;
  lserver->connections_mutex_lock();
  con = lserver->getConnections();
  while(con)
  {
    sprintf(b1, "%i - %s - %i - %s - %i - %s - %s\r\n", 
            (int)con->getID(),  con->getipAddr(), (int)con->getPort(), 
            con->getlocalIpAddr(),  (int)con->getLocalPort(), 
            con->getLogin(), con->getPassword());
    ret = out->writeToFile(b1, strlen(b1), &nbw);   
    if(ret)
    {
      strcpy(b1,"Error in writing to file");
      addToErrorLog(a,b1, strlen(b1));
    }
    con = con->next;
  }
  lserver->connections_mutex_unlock();
  return ret;
}

/*!
 *Kill a connection by its ID.
 */
int  ControlProtocol::KILLCONNECTION(ConnectionPtr a, u_long ID, File* out, 
                                      char *b1, int bs1)
{
  int ret = 0;
  ConnectionPtr con;
  if(ID == 0)
    return -1;
  con = lserver->findConnectionByID(ID);
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
int ControlProtocol::SHOWDYNAMICPROTOCOLS(ConnectionPtr a, File* out, 
                                           char *b1,int bs1)
{
  int i = 0;
  DynamicProtocol* dp;
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
    {
      strcpy(b1,"Error in writing to file");
      addToErrorLog(a,b1, strlen(b1));
      return CONTROL_INTERNAL;
    }
    i++;
  }
  return 0;
}

/*!
 *Return the requested file to the client.
 */
int ControlProtocol::GETFILE(ConnectionPtr a, char* fn, File* in, 
                              File* out, char *b1,int bs1 )
{
  char *filename = 0;
  File localfile;
  int ret = 0;
  /*! # of bytes read. */
  u_long nbr = 0;

  /*! # of bytes written. */
  u_long nbw = 0;

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
    strcpy(b1,"Requested file doesn't exist");
    addToErrorLog(a,b1, strlen(b1));
    return CONTROL_FILE_NOT_FOUND;
  }

  ret = localfile.openFile(fn, File::OPEN_READ|File::OPEN_IFEXISTS);

  /*! An internal server error happens. */
  if(ret)
  {
    strcpy(b1,"Error in opening the file");
    addToErrorLog(a,b1, strlen(b1));
    return CONTROL_INTERNAL;
  }
  for( ; ;)
  {
    ret = localfile.readFromFile(b1, bs1, &nbr);
    if(ret)
    {
      strcpy(b1,"Error in reading from file");
      addToErrorLog(a,b1, strlen(b1));
      return CONTROL_INTERNAL;
    }

    /*! Break the loop when we can't read no more data.*/
    if(!nbr)
      break;
    
    ret = Ofile->writeToFile(b1, nbr, &nbw);

    if(ret)
    {
      strcpy(b1,"Error in writing to file");
      addToErrorLog(a,b1, strlen(b1));
      return CONTROL_INTERNAL;
    }

  }

  localfile.closeFile();
  return 0;
}

/*!
 *Save the file on the local FS.
 */
int ControlProtocol::PUTFILE(ConnectionPtr a, char* fn, File* in, 
                              File* out, char *b1,int bs1 )
{
  char *filename = 0;
  File localfile;
  int isAutoRebootToEnable = lserver->isAutorebootEnabled();
  int ret = 0;
  /*! # of bytes read. */
  u_long nbr = 0;
  /*! # of bytes written. */
  u_long nbw = 0;
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
    strcpy(b1,"Requested file doesn't exist");
    addToErrorLog(a, b1, strlen(b1));
    Reboot = true;
    if(isAutoRebootToEnable)
      lserver->enableAutoReboot();
    return CONTROL_FILE_NOT_FOUND;
  }

  ret = File::deleteFile(fn);

  /*! An internal server error happens. */
  if(ret)
  {
    strcpy(b1,"Error deleting the file");
    addToErrorLog(a, b1, strlen(b1));
    Reboot = true;
    if(isAutoRebootToEnable)
      lserver->enableAutoReboot();
    return CONTROL_INTERNAL;
  }

  ret = localfile.openFile(fn, File::OPEN_WRITE | File::OPEN_ALWAYS);

  /*! An internal server error happens. */
  if(ret)
  {
    strcpy(b1,"Error opening the file");
    addToErrorLog(a, b1, strlen(b1));
    Reboot = true;
    if(isAutoRebootToEnable)
      lserver->enableAutoReboot();
    return CONTROL_INTERNAL;
  }
  for( ; ;)
  {
    ret = Ifile->readFromFile(b1, bs1, &nbr);
    if(ret)
    {
      strcpy(b1,"Error in reading from file");
      addToErrorLog(a, b1, strlen(b1));
      Reboot = true;
      if(isAutoRebootToEnable)
        lserver->enableAutoReboot();
      return CONTROL_INTERNAL;
    }

    /*! Break the loop when we can't read no more data.*/
    if(!nbr)
      break;
    
    ret = localfile.writeToFile(b1, nbr, &nbw);

    if(ret)
    {
      strcpy(b1,"Error in writing to file");
      addToErrorLog(a, b1, strlen(b1));
      Reboot = true;
      if(isAutoRebootToEnable)
        lserver->enableAutoReboot();
      return CONTROL_INTERNAL;
    }
  }
  localfile.closeFile();
  
  if(isAutoRebootToEnable)
    lserver->enableAutoReboot();

  return 0;
}

/*!
 *Show all the language files that the server can use.
 */
int ControlProtocol::SHOWLANGUAGEFILES(ConnectionPtr a, File* out, 
                                        char *b1,int bs1)
{
  char *path = lserver->getLanguagesPath();
  FindData fd;
  int ret = 0;
  if(path == 0)
  {
    strcpy(b1,"Error retrieving the language files path");
    addToErrorLog(a,b1, strlen(b1));
    return CONTROL_INTERNAL;
  }
  ret=fd.findfirst(path);
  if(ret == -1)
  {
    strcpy(b1,"Error in directory lookup");
    addToErrorLog(a,b1, strlen(b1));
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
    File::splitPathLength(fd.name, &dirLen, &filenameLen);
    dir = new char[dirLen + 1];
    if(dir == 0)
    {
      strcpy(b1,"Error in allocating memory");
      addToErrorLog(a, b1, strlen(b1));
      fd.findclose();
      return CONTROL_INTERNAL;
    }
    filename = new char[filenameLen + 1];
    if(filename == 0)
    {
      strcpy(b1,"Error in allocating memory");
      addToErrorLog(a,b1, strlen(b1));
      delete [] dir;
      fd.findclose();
      return CONTROL_INTERNAL;
    }
    File::splitPath(fd.name,dir,filename);
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
      strcpy(b1,"Error in writing to temporary file");
      addToErrorLog(a,b1, strlen(b1));
      return CONTROL_INTERNAL;
    }
  }
	while(!fd.findnext());
  fd.findclose();
  return 0;
}

/*!
 *Return the current MyServer version.
 */
int ControlProtocol::GETVERSION(ConnectionPtr a, File* out, char *b1,int bs1)
{
  u_long nbw;
  sprintf(b1, "MyServer %s", versionOfSoftware);
  return Ofile->writeToFile(b1, strlen(b1), &nbw);
}
