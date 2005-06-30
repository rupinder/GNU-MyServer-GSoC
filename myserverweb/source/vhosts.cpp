
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

#include "../include/vhosts.h"
#include "../include/filemanager.h"
#include "../include/cserver.h"
#include "../include/connectionstruct.h"
#include "../include/stringutils.h"
#include "../include/threads.h"
#include "../include/securestr.h"

/*!
 *SSL password callback function.
 */
static int password_cb(char *buf,int num,int /*!rwflag*/,void *userdata)
{
	if((size_t)num<strlen((char*)userdata)+1)
		return 0;

  ((string*)userdata)->assign(buf);

	return ((string*)userdata)->length();
}

/*!
 *vhost costructor
 */
Vhost::Vhost()
{
	sslContext.certificateFile.assign("");
	sslContext.method = 0;
	sslContext.privateKeyFile.assign("");
	sslContext.password.assign("");
	ipList=0;
	hostList=0;
  documentRoot.assign("");
	systemRoot.assign("");
  accessesLogFileName.assign("");
  warningsLogFileName.assign("");
  /*! 
   *By default use a non specified value for the throttling rate. -1 means 
   *that the throttling rate was not specified, while 0 means it was 
   *specified but there is not a limit.
   */
  throttlingRate = (u_long) -1;
  refCount = 0;
}


/*!
 *Destroy the vhost. 
 */
Vhost::~Vhost()
{
	clearHostList();
	clearIPList();
	freeSSL();

  accessesLogFileName.assign("");
  
  warningsLogFileName.assign("");
 
  warningsLogFile.close();
  accessesLogFile.close();
  
  documentRoot.assign("");
	systemRoot.assign("");

  mimeManager.clean();
}

/*!
 *Check if a MIME type file is defined for the virtual host.
 */
int Vhost::isMIME()
{
  return mimeManager.isLoaded();
}

/*!
 *Get the MIME manager for the virtual host.
 */
MimeManager* Vhost::getMIME()
{
  return &mimeManager;
}

/*!
 *Clear the list of the hosts.
 */
void Vhost::clearHostList()
{
	sHostList* shl=hostList;
	sHostList* prevshl=0;
	while(shl)
	{
		if(prevshl)
    {
      prevshl->hostRegex.free();
      delete prevshl;
    }
		prevshl=shl;
		shl=shl->next;
	}
	if(prevshl)
		delete prevshl;
	hostList=0;
}

/*!
 *Clear the list of IPs.
 */
void Vhost::clearIPList()
{
	sIpList* sil = ipList;
	sIpList* prevsil = 0;
	while(sil)
	{
		if(prevsil)
    {
      prevsil->ipRegex.free();
			delete prevsil;
    }
		prevsil = sil;
		sil = sil->next;
	}
	if(prevsil)
		delete prevsil;
	ipList=0;
}

/*!
 *Add an IP address to the list.
 */
void Vhost::addIP(const char *ip, int isRegex)
{
	sIpList* il=new sIpList();
  if(il==0)
    return;
	il->hostIp.assign(ip);
  /*! If is a regular expression, the ip string is a pattern. */
  if(isRegex)
    il->ipRegex.compile(ip, REG_EXTENDED);
	if(ipList)
	{
		il->next = ipList;
	}
	else
	{
		il->next = 0;
	}
	ipList=il;
}
/*!
 *Remove the IP address to the list.
 */
void Vhost::removeIP(const char *ip)
{
	Vhost::sIpList *iterator = ipList;
	Vhost::sIpList *iteratorBack = 0;
	if(iterator==0)
		return;
	while(iterator)
	{
		/*!
     *If this is the virtual host with the right IP.
     */
		if(!stringcmp(iterator->hostIp,ip))
		{
			if(iteratorBack)
			{
				iteratorBack->next = iterator->next;
         iterator->ipRegex.free();
				delete iterator;
				return;
			}
			else
			{
				ipList = iterator->next;
         iterator->ipRegex.free();
				delete iterator;
				return;
			}
		}
		iteratorBack = iterator;	
		iterator = iterator->next;
	}

}

/*!
 *Remove the host address to the list.
 */
void Vhost::removeHost(const char *host)
{
	Vhost::sHostList *iterator = hostList;
	Vhost::sHostList *iteratorBack = 0;
	if(iterator == 0)
		return;
	while(iterator)
	{
		/*!
     *If this is the virtual host with the right host name
     */
		if(!stringcmp(iterator->hostName,host))
		{
			if(iteratorBack)
			{
				iteratorBack->next =iterator->next;
         iterator->hostRegex.free();
				delete iterator;
				return;
			}
			else
			{
				hostList=iterator->next;
         iterator->hostRegex.free();
				delete iterator;
				return;
			}
		}
		iteratorBack = iterator;	
		iterator = iterator->next;
	}
}
/*!
 *Check if an host is allowed to the connection
 */
int Vhost::isHostAllowed(const char* host)
{
	if(hostList == 0)/*If no hosts are specified, every host is allowed to connect to*/
		return 1;
	sHostList *lhl = hostList;
	while(lhl)
	{
    regmatch_t pm;
    if(lhl->hostRegex.isCompiled())
    {
      if (!lhl->hostRegex.exec(host ,1, &pm, REG_NOTBOL))
      {
        return 1;
      }
    }
		if(!stringcmp(lhl->hostName, host))
			return 1;
		lhl = lhl->next;
	}
	return 0;
}

/*!
 *Check if all the host are allowed to the connection.
 */
int Vhost::areAllHostAllowed()
{
	if(hostList==0)
		return 1;
	return 0;
}

/*!
 *Check if all the IPs are allowed to the connection.
 */
int Vhost::areAllIPAllowed()
{
	if(ipList == 0)
		return 1;
	return 0;
}

/*!
 *Check if the network is allowed to the connection(control the network 
 *by the local IP).
 */
int Vhost::isIPAllowed(const char* ip)
{
  /*! If no IPs are specified, every IP is allowed to connect to. */
	if(ipList == 0)
		return 1;
	sIpList *lipl=ipList;
	while(lipl)
	{
    regmatch_t pm;
    if(lipl->ipRegex.isCompiled())
    {
      if (!lipl->ipRegex.exec(ip ,1, &pm, REG_NOTBOL))
      {
        return 1;
      }
    }
		else if(!stringcmp(lipl->hostIp, ip))
			return 1;
		lipl=lipl->next;
	}
	return 0;
}

/*!
 *Add an host to the allowed host list.
 */
void Vhost::addHost(const char *host, int isRegex)
{
	sHostList* hl=new sHostList();
  if(hl==0)
    return;
	hl->hostName.assign( host );
  if(isRegex)
    hl->hostRegex.compile(host, REG_EXTENDED);
	if(hostList)
	{
		hl->next =hostList;
	}
	else
	{
		hl->next =0;
	}
	hostList=hl;
}

/*!
 *Here threads get the permission to use the access log file.
 */
u_long Vhost::accesseslogRequestAccess(int id)
{
	accessesLogFile.requestAccess();
  return 0;
}

/*!
 *Here threads get the permission to use the warnings log file.
 */
u_long Vhost::warningslogRequestAccess(int id)
{
	warningsLogFile.requestAccess();
  return 0;
}

/*!
 *Here threads release the permission to use the access log file.
 */
u_long Vhost::accesseslogTerminateAccess(int id)
{
	accessesLogFile.terminateAccess();
  return 0;
}

/*!
 *Here threads release the permission to use the warnings log file.
 */
u_long Vhost::warningslogTerminateAccess(int id)
{
	warningsLogFile.terminateAccess();
  return 0;
}

/*!
 *Write to the accesses log file.
 */
int Vhost::accessesLogWrite(const char* str)
{
	return accessesLogFile.write(str);
}

/*!
 *Return a pointer to the file used by the accesses log.
 */
File* Vhost::getAccessesLogFile()
{
	return accessesLogFile.getFile();
}

/*!
 *Get the log object for the warnings.
 */
LogManager* Vhost::getWarningsLog()
{
  return &warningsLogFile;
}

/*!
 *Get the log object for the accesses.
 */
LogManager* Vhost::getAccessesLog()
{
  return &accessesLogFile;
}

/*!
 *Write to the warnings log file.
 */
int Vhost::warningsLogWrite(const char* str)
{
  string msg;
  getLocalLogFormatDate(msg, 100);
  msg.append(" -- ");
  msg.append(str);
	return warningsLogFile.write(msg.c_str());
}

/*!
 *Return a pointer to the file used by the warnings log.
 */
File* Vhost::getWarningsLogFile()
{
	return warningsLogFile.getFile();
}

/*!
 *Set the max size of the log files.
 */
void Vhost::setMaxLogSize(int newSize)
{
  warningsLogFile.setMaxSize(newSize);
  accessesLogFile.setMaxSize(newSize);  
}

/*!
 *Get the max size of the log files. Return 0 on success.
 */
int Vhost::getMaxLogSize()
{
  /*!
   *warningsLogFile max log size is equal to the  
   *accessesLogFile one.
   */
	return warningsLogFile.getMaxSize( );
}

/*!
 *vhostmanager costructor.
 */
int VhostManager::addVHost(Vhost* VHost)
{
  sVhostList* hostl;
  sVhostList* lastHost;

  mutex.lock();

  hostl=vhostList;
  lastHost=vhostList;
  try
  {
    while(hostl)
    {
      if(hostl->next)
        lastHost=hostl->next;

      /*! Do a case sensitive compare under windows. */
#ifdef WIN32
      if(!stringcmpi(VHost->getAccessesLogFileName(), 
                     hostl->host->getAccessesLogFileName()))
#else
        if(!stringcmp(VHost->getAccessesLogFileName(), 
                      hostl->host->getAccessesLogFileName()))
#endif
        {
          string error;
          error.assign("Warning: multiple hosts use the same log file:" );
          error.append(VHost->getAccessesLogFileName());
          lserver->logPreparePrintError();
          lserver->logWriteln(error.c_str());     
          lserver->logEndPrintError();
        }

#ifdef WIN32
      if(!stringcmpi(VHost->getWarningsLogFileName(), 
                     hostl->host->getWarningsLogFileName()))
#else
        if(!stringcmp(VHost->getWarningsLogFileName(), 
                      hostl->host->getWarningsLogFileName()))
#endif
        {
          string error;
          error.assign("Warning: multiple hosts use the same log file:" );
          error.append(VHost->getWarningsLogFileName());
          lserver->logPreparePrintError();
          lserver->logWriteln(error.c_str());     
          lserver->logEndPrintError();
        }
      hostl=hostl->next;
    }

    if(vhostList==0)
    {
      vhostList=new sVhostList();	
      if(vhostList == 0)
      {
        lserver->logPreparePrintError();
        lserver->logWriteln( "Error allocating memory" );     
        lserver->logEndPrintError();
        mutex.unlock();
        return 1;
      }
      vhostList->host=VHost;
      vhostList->next =0;
    }
    else
    {
      /*! Append the new host to the end of the linked list. */
      lastHost->next = new sVhostList();	
      if(lastHost->next==0)
      {
        lserver->logPreparePrintError();
        lserver->logWriteln( "Error allocating memory" );     
        lserver->logEndPrintError();
        mutex.unlock();
        return 1;
      }
      /*! Make sure that next is null. */
      lastHost->next->next =0;
      lastHost->next->host=VHost;
    }
    mutex.unlock();
    return 0;
  }
  catch(...)
  {
    mutex.unlock();
    return 0;
  };
}
/*!
 *Get the vhost for the connection. A return value of 0 means that
 *a valid host was not found. 
 */
Vhost* VhostManager::getVHost(const char* host, const char* ip, u_short port)
{
	sVhostList* vhl;
  
  mutex.lock();
  
  try
  {
    if(extSource)
    {
      Vhost*ret=extSource->getVHost(host, ip, port);
      mutex.unlock();
      return ret;
    }
    for(vhl=vhostList;vhl;vhl=vhl->next )
    {
      /*! Control if the host port is the correct one. */
      if(vhl->host->getPort()!=port)
        continue;
      /*! If ip is defined check that it is allowed to connect to the host. */
      if(ip && !vhl->host->isIPAllowed(ip))
        continue;
      /*! If host is defined check if it is allowed to connect to the host. */
      if(host && !vhl->host->isHostAllowed(host))
        continue;
      /*! We find a valid host. */
      mutex.unlock();
      /*! Add a reference. */
      vhl->host->addRef();
      return vhl->host;
    }
    mutex.unlock();
    return 0;
  }
  catch(...)
  {
    mutex.unlock();
    return 0;   
  };
}

/*!
 *VhostManager costructor
 */
VhostManager::VhostManager()
{
	vhostList=0;
  extSource=0;
  mutex.init();
}

/*!
 *Clean the virtual hosts.
 */
void VhostManager::clean()
{
  sVhostList* shl;
	sVhostList* prevshl=0;

	mutex.lock();
  
  try
  {
    shl=vhostList;
    while(shl)
    {
      if(prevshl)
      {
        delete prevshl->host;
        delete prevshl;
      }
      prevshl=shl;
      shl=shl->next;
    }
    if(prevshl)
    {
      delete prevshl->host;
      delete prevshl;
    }
    vhostList = 0;
    mutex.unlock();
  }
  catch(...)
  {
    mutex.unlock();
    return;  
  };
}

/*!
 *vhostmanager destructor.
 */
VhostManager::~VhostManager()
{
	clean();
  mutex.destroy();
}

/*!
 *Load the virtual hosts from a configuration file.
 */
int VhostManager::loadConfigurationFile(const char* filename,int maxlogSize)
{
	/*!
   *FILE STRUCTURE:
   *hosts;IPs;port;protocol;documentRoot;systemFolder;accessesLog;warningLog#
   *1)hosts is a list of valid hosts name that the virtual host can accept, 
   * every value is separated by a comma. If host is 0 every host name is valid.
   *2)IPs is a list of valid IP that the virtual host can accept, 
   * every value is separated by a comma. If IPs is 0 every IP name is valid.
   *3)port in the port used to listen.
   *4)protocol is the protocol used(HTTP,FTP...). By default is HTTP.
   *5)documentRoot is the path to the web folder.
   *6)systemFolder is the path to the system folder.
   *7-8)accessesLog;warningLog are strings saying where put the 
   * accesses and the warningLog file for this virtual host.
   *The last virtual host ends with two final diesis, '##'.
   *Comments can be done like are done comments on multiline in C++, 
   *between "/ *" and "* /" without the space.
   *In 5) and 6) for use absolute path use the character | before the full path.
   */
	char buffer[MYSERVER_KB(10)];/*! Max line length=10KB*/
	char buffer2[256];
	u_long nbr;/*! Number of bytes read from the file. */
	File fh;
	Vhost *vh;
  LogManager *accesses;
	LogManager * warnings;
	char c;
	int ret=fh.openFile(filename,FILE_OPEN_IFEXISTS|FILE_OPEN_READ);
  /*! If the file cannot be opened simply do nothing. */
	if(ret)
		return -1;

	for(;;)
	{
		int cc=0;
		buffer[0]='\0';
    /*! Save a line in the buffer. A line ends with a diesis.*/
		for(;;)
		{
			fh.readFromFile(&c,1,&nbr);
			if(c!='#')
			{
				if( (c!='\n')&&(c!='\r'))
				{
					buffer[strlen(buffer)+1]='\0';
					buffer[strlen(buffer)]=c;
				}
				if(buffer[0]=='/' && buffer[1]=='*')
				{
					char oldc;
					do
					{
						oldc = c;
						fh.readFromFile(&c,1,&nbr);
					}while(c!='/' && oldc!='*');
					buffer[0]='\0';
					continue;
				}
			}
			else
				break;
		}
		if(buffer[0]=='\0')
			break;		
		vh=new Vhost();
    if(vh==0)
    {
      fh.closeFile();
      clean();
      return -1;
    }
		/*! Parse the line. */
    cc = 0;
    /*! Get the hosts list. */
		for(;;)
		{
			buffer2[0]='\0';
			while((buffer[cc]!=',')&&(buffer[cc]!=';'))
			{
				buffer2[strlen(buffer2)+1]='\0';
				buffer2[strlen(buffer2)]=buffer[cc];
				cc++;
			}
      /*!
       *If the host list is equal 
       *to 0 don't add anything to the list
       */
			if(buffer2[0]&&buffer2[0]!='0')
				vh->addHost(buffer2, 0);
			if(buffer[cc]==';')
				break;
			cc++;
		}
		cc++;
		for(;;)/*!Get the ip list*/
		{
			buffer2[0]='\0';

			while((buffer[cc]!=',')&&(buffer[cc]!=';'))
			{
				buffer2[strlen(buffer2)+1]='\0';
				buffer2[strlen(buffer2)]=buffer[cc];
				cc++;
			}
      /*!
       *If the ip list is equal to 0 don't add anything to the list.
       */
			if(buffer2[0]&&buffer2[0]!='0')
				vh->addIP(buffer2, 0);
			if(buffer[cc]==';')
				break;
			cc++;
		}
		cc++;
		/*!Get the port used by the virtual host*/
		buffer2[0]='\0';
		while(buffer[cc]!=';')
		{
			buffer2[strlen(buffer2)+1]='\0';
			buffer2[strlen(buffer2)]=buffer[cc];
			cc++;
		}
		vh->setPort((u_short)atoi(buffer2));
		cc++;		
		/*!Get the protocol used by the virtual host*/
		buffer2[0]='\0';
		while(buffer[cc]!=';')
		{
			buffer2[strlen(buffer2)+1]='\0';
			buffer2[strlen(buffer2)]=buffer[cc];
			cc++;
		}

		if(!strcmp(buffer2,"HTTP"))
			vh->setProtocol(PROTOCOL_HTTP);
		else if(!strcmp(buffer2,"HTTPS"))
			vh->setProtocol(PROTOCOL_HTTPS);
		else if(!strcmp(buffer2,"FTP"))
			vh->setProtocol(PROTOCOL_FTP);
		else if(!strcmp(buffer2,"CONTROL"))
			vh->setProtocol(PROTOCOL_CONTROL);
		else
		{
			vh->setProtocol(PROTOCOL_UNKNOWN);
		}

		vh->setProtocolName(buffer2);
		
		cc++;
		/*!Get the document root used by the virtual host*/
		buffer2[0]='\0';
		while(buffer[cc]!=';')
		{
			buffer2[strlen(buffer2)+1]='\0';
			buffer2[strlen(buffer2)]=buffer[cc];
			cc++;
		}	
		vh->setDocumentRoot(buffer2);
		cc++;
		/*!Get the system folder used by the virtual host*/
		buffer2[0]='\0';
		while(buffer[cc]!=';')
		{
			buffer2[strlen(buffer2)+1]='\0';
			buffer2[strlen(buffer2)]=buffer[cc];
			cc++;
		}	
		vh->setSystemRoot(buffer2);
		cc++;
		/*!Get the accesses log file used by the virtual host*/
		buffer2[0]='\0';
		while(buffer[cc]!=';')
		{
			buffer2[strlen(buffer2)+1]='\0';
			buffer2[strlen(buffer2)]=buffer[cc];
			cc++;
		}	
		vh->setAccessesLogFileName(buffer2);
		accesses=vh->getAccessesLog();
    
		accesses->load(buffer2);

		cc++;
		/*!Get the warnings log file used by the virtual host*/
		buffer2[0]='\0';
		while(buffer[cc]!=';' && buffer[cc])
		{
			buffer2[strlen(buffer2)+1]='\0';
			buffer2[strlen(buffer2)]=buffer[cc];
			cc++;
		}	
		vh->setWarningsLogFileName(buffer2);
		warnings=vh->getWarningsLog();
		warnings->load(buffer2);
		vh->setMaxLogSize(maxlogSize);
		cc++;
		if(addVHost(vh))
    {
      clean();
      fh.closeFile();
      return 1;
    }
	}
	fh.closeFile();
  return 0;
}

/*!
 *Save the virtual hosts to a configuration file.
 */
void VhostManager::saveConfigurationFile(const char *filename)
{
	char buffer[1024];
	if(vhostList==0)
		return;
	u_long nbw;
	File fh;
	sVhostList*vhl;

  mutex.lock();

  vhl=vhostList;
  try
  {
    fh.openFile(filename,FILE_CREATE_ALWAYS|FILE_OPEN_WRITE);
    for(;vhl;vhl=vhl->next )
    {
      Vhost* vh=vhl->host;
      Vhost::sIpList* il;
      Vhost::sHostList* hl=vh->getHostList();
      if(hl)
      {
        while(hl)
        { 
          fh.writeToFile(hl->hostName.c_str(),hl->hostName.length(),&nbw);
          strcpy(buffer,",");
          fh.writeToFile(buffer,(u_long)strlen(buffer),&nbw);
          if(hl->next )
            hl=hl->next;
        }
      }
      else
      {
        strcpy(buffer,"0");
        fh.writeToFile(buffer,(u_long)strlen(buffer),&nbw);
      }
      strcpy(buffer,";");
      fh.writeToFile(buffer,(u_long)strlen(buffer),&nbw);
      il=vh->getIpList();
      if(il)
      {
        while(il)
        { 
          fh.writeToFile(il->hostIp.c_str(),(u_long)il->hostIp.length(),&nbw);
          if(il->next )
          {
            strcpy(buffer,",");
            fh.writeToFile(buffer,(u_long)strlen(buffer),&nbw);
          }

          il=il->next;
        }
      }
      else
      {
        strcpy(buffer,"0");
        fh.writeToFile(buffer,(u_long)strlen(buffer),&nbw);
      }
      sprintf(buffer,";%u;",vh->getPort());
      fh.writeToFile(buffer,(u_long)strlen(buffer),&nbw);

      if(vh->getProtocol()==PROTOCOL_HTTP)
      {
        strcpy(buffer,"HTTP;");
        fh.writeToFile(buffer,(u_long)strlen(buffer),&nbw);
      }
      else if(vh->getProtocol()==PROTOCOL_HTTPS)
      {
        strcpy(buffer,"HTTPS;");
        fh.writeToFile(buffer,(u_long)strlen(buffer),&nbw);
      }
      else if(vh->getProtocol()==PROTOCOL_CONTROL)
      {
        strcpy(buffer,"CONTROL;");
        fh.writeToFile(buffer,(u_long)strlen(buffer),&nbw);
      }
      else if(vh->getProtocol()==PROTOCOL_FTP)
      {
        strcpy(buffer,"FTP;");
        fh.writeToFile(buffer,(u_long)strlen(buffer),&nbw);
      }
      else
      {
        fh.writeToFile(vh->getProtocolName(), 
                       strlen(vh->getProtocolName()),&nbw);
      }

      fh.writeToFile(vh->getDocumentRoot(), 
                     (u_long)strlen(vh->getDocumentRoot()),&nbw);
      strcpy(buffer,";");
      fh.writeToFile(buffer,(u_long)strlen(buffer),&nbw);

      fh.writeToFile(vh->getSystemRoot(),(u_long)strlen(vh->getSystemRoot()),&nbw);
      strcpy(buffer,";");
      fh.writeToFile(buffer,(u_long)strlen(buffer),&nbw);
      
      fh.writeToFile(vh->getAccessesLogFileName(), 
                     (u_long)strlen(vh->getAccessesLogFileName()), &nbw);

      strcpy(buffer,";");
      fh.writeToFile(buffer,(u_long)strlen(buffer),&nbw);
      
      fh.writeToFile(vh->getWarningsLogFileName(), 
                     (u_long)strlen(vh->getWarningsLogFileName()), &nbw);
      if(vhl->next )
        strcpy(buffer,";#\r\n");
      else
        strcpy(buffer,";##\r\n\0");
      fh.writeToFile(buffer,(u_long)strlen(buffer),&nbw);
    }
    fh.closeFile();
    mutex.unlock();
  }
  catch(...)
  {
    mutex.unlock();
  };
}

/*!
 *Returns the entire virtual hosts list.
 */
sVhostList* VhostManager::getVHostList()
{
	return this->vhostList;
}

/*!
 *Switch the position between two virtual hosts
 *Zero based index.
 */
int VhostManager::switchVhosts(int n1,int n2)
{
	sVhostList *vh1;
	sVhostList *vh2;
	int i;
	if(max(n1,n2)>=getHostsNumber())
		return 0;
	vh1 = vhostList;
	for(i=0;i<n1;i++)
	{
		vh1=vh1->next;
	}
	vh2 = vhostList;
	for(i=0;i<n2;i++)
	{
		vh2=vh2->next;
	}
	return switchVhosts(vh1,vh2);

}

/*!
 *Switch two virtual hosts.
 */
int VhostManager::switchVhosts(sVhostList * vh1, sVhostList * vh2)
{
  Vhost* vh3;
	if((vh1==0)||(vh2==0))
		return 0;
	vh3=vh1->host;
	vh1->host = vh2->host;
	vh2->host = vh3;
	return 1;
}

/*!
 *Returns the number of hosts in the list
 */
int VhostManager::getHostsNumber()
{
	int i;
	sVhostList *vh = vhostList;
	for(i=0;vh;i++,vh=vh->next );
	return i;
}

/*!
 *Load the virtual hosts from a XML configuration file
 *Returns non-null on errors.
 */
int VhostManager::loadXMLConfigurationFile(const char *filename,int maxlogSize)
{
	XmlParser parser;
	xmlDocPtr doc;
	xmlNodePtr node;
  LogManager *warningLogFile ;
  LogManager *accessLogFile;
	if(parser.open(filename))
	{
		return -1;
	}
	doc = parser.getDoc();
	node=doc->children->children;
	for(;node;node=node->next )
	{
		xmlNodePtr lcur;
		Vhost *vh;
		if(xmlStrcmp(node->name, (const xmlChar *)"VHOST"))
			continue;
		lcur=node->children;
		vh=new Vhost();
    if(vh==0)
    {
      parser.close();
      clean();
			return -1;
    }
		while(lcur)
    {
      Vhost::vhsslcontext * sslContext = vh->getVhostSSLContext();
			if(!xmlStrcmp(lcur->name, (const xmlChar *)"HOST"))
			{
        int useRegex = 0;
        xmlAttr *attrs = lcur->properties;
        while(attrs)
        {
            if(!xmlStrcmp(attrs->name, (const xmlChar *)"isRegex"))
            {
              if(attrs->children && attrs->children->content && 
                 (!xmlStrcmp(attrs->children->content, (const xmlChar *)"YES")))
              {
                useRegex = 1;
              }
            }
            attrs=attrs->next;
        }
			  vh->addHost((char*)lcur->children->content, useRegex);
			}
			if(!xmlStrcmp(lcur->name, (const xmlChar *)"NAME"))
			{
				vh->setName((char*)lcur->children->content);
			}
			if(!xmlStrcmp(lcur->name, (const xmlChar *)"SSL_PRIVATEKEY"))
			{
				sslContext->privateKeyFile.assign((char*)lcur->children->content);
			}
			if(!xmlStrcmp(lcur->name, (const xmlChar *)"SSL_CERTIFICATE"))
			{
				sslContext->certificateFile.assign((char*)lcur->children->content);
			}
			if(!xmlStrcmp(lcur->name, (const xmlChar *)"SSL_PASSWORD"))
			{
				if(lcur->children)
					sslContext->password.assign((char*)lcur->children->content);
				else
					sslContext->password.assign("");
			}
			if(!xmlStrcmp(lcur->name, (const xmlChar *)"IP"))
			{
        int useRegex = 0;
        xmlAttr *attrs =  lcur->properties;
        while(attrs)
        {
            if(!xmlStrcmp(attrs->name, (const xmlChar *)"isRegex"))
            {
              if(attrs->children && attrs->children->content && 
                 (!xmlStrcmp(attrs->children->content, (const xmlChar *)"YES")))
              {
                useRegex = 1;
              }
            }
            attrs=attrs->next;
        }
	  	 vh->addIP((char*)lcur->children->content, useRegex);
			}
			if(!xmlStrcmp(lcur->name, (const xmlChar *)"PORT"))
			{
				vh->setPort((u_short)atoi((char*)lcur->children->content));
			}
			if(!xmlStrcmp(lcur->name, (const xmlChar *)"PROTOCOL"))
			{
				if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"HTTP"))
					vh->setProtocol(PROTOCOL_HTTP);
				else if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"HTTPS"))
					vh->setProtocol(PROTOCOL_HTTPS);
				else if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"FTP"))
					vh->setProtocol(PROTOCOL_FTP);
				else if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"CONTROL"))
					vh->setProtocol(PROTOCOL_CONTROL);
				else
				{
						vh->setProtocol(PROTOCOL_UNKNOWN);
				}
				vh->setProtocolName((char*)lcur->children->content);
				
			}
			if(!xmlStrcmp(lcur->name, (const xmlChar *)"DOCROOT"))
			{
        if(lcur->children && lcur->children->content)
        {
          char lastChar;
          vh->setDocumentRoot((char*)lcur->children->content);
          lastChar = vh->getDocumentRoot()[strlen(vh->getDocumentRoot())-1];
          if(lastChar != '\\' && lastChar != '/')
          {
            string tmp;
            tmp.assign(vh->getDocumentRoot());
            tmp.append("/");
            vh->setDocumentRoot(tmp.c_str());
          }
        }
        else
        {
          vh->setDocumentRoot("");
        }
			}
      if(!xmlStrcmp(lcur->name, (const xmlChar *)"SYSFOLDER"))
			{
        if(lcur->children && lcur->children->content)
        {
          char lastChar ;
          vh->setSystemRoot((char*)lcur->children->content);
          lastChar = vh->getSystemRoot()[strlen(vh->getSystemRoot())-1];
          if(lastChar != '\\' && lastChar != '/')
          {
            string tmp;
            tmp.assign(vh->getSystemRoot());
            tmp.append("/");
            vh->setSystemRoot(tmp.c_str());
          }
        }
        else
        {
          vh->setSystemRoot("");
        }
      }
			if(!xmlStrcmp(lcur->name, (const xmlChar *)"ACCESSESLOG"))
			{
        xmlAttr *attr;
        string opt;
        opt.assign("");
				vh->setAccessLogOpt("");                
				vh->setAccessesLogFileName((char*)lcur->children->content);

				attr =  lcur->properties;
        while(attr)
        {
					opt.append((char*)attr->name);
					opt.append("=");
          opt.append((char*)attr->children->content);

					if(attr->next)
					{
						opt.append(",");
					}
					attr=attr->next;
        }
        vh->setAccessLogOpt(opt.c_str());
			}
			
			if(!xmlStrcmp(lcur->name, (const xmlChar *)"WARNINGLOG"))
      {
				xmlAttr *attr;
        string opt;
        opt.assign("");
				vh->setWarningsLogFileName((char*)lcur->children->content);
				vh->setWarningLogOpt("");                
				attr =  lcur->properties;
				while(attr)
				{
					opt.append((char*)attr->name);
					opt.append("=");
					opt.append((char*)attr->children->content);
					if(attr->next)
          {
     
            opt.append(",");
          }

					attr=attr->next;
				}
        
        vh->setWarningLogOpt(opt.c_str());
      }

			if(!xmlStrcmp(lcur->name, (const xmlChar *)"MIME_FILE"))
			{
				if(lcur->children)
          vh->getMIME()->loadXML((char*)lcur->children->content);
			}

			if(!xmlStrcmp(lcur->name, (const xmlChar *)"THROTTLING_RATE"))
			{
				vh->setThrottlingRate((u_long)atoi((char*)lcur->children->content));
 			}
      lcur=lcur->next;
    }

    accessLogFile=vh->getAccessesLog();
    accessLogFile->load(vh->getAccessesLogFileName());
    
    if(strstr(vh->getAccessLogOpt(), "cycle=yes"))
    {
      accessLogFile->setCycleLog(1);
    }
    if(strstr(vh->getAccessLogOpt(), "cycle_gzip=no"))
    {
      accessLogFile->setGzip(0);
    }
    else
    {
      accessLogFile->setGzip(1);
    }  
 
    warningLogFile = vh->getWarningsLog();
    warningLogFile->load(vh->getWarningsLogFileName());
    if(strstr(vh->getWarningLogOpt(), "cycle=yes"))
    {
      warningLogFile->setCycleLog(1);
    }

    if(strstr(vh->getWarningLogOpt(), "cycle_gzip=no"))
    {
      warningLogFile->setGzip(0);
    }
    else
    {
      warningLogFile->setGzip(1);
    }

    vh->setMaxLogSize(maxlogSize);
    vh->initializeSSL();
    if(addVHost(vh))
    {
      parser.close();
      clean();
      return 1;
    }
    
  }
  parser.close();
  return 0;
}

/*!
 *Save the virtual hosts to a XML configuration file.
 */
void VhostManager::saveXMLConfigurationFile(const char *filename)
{
	sVhostList *list;
	File out;
	u_long nbw;
	out.openFile(filename,FILE_CREATE_ALWAYS|FILE_OPEN_WRITE);
	out.writeToFile("<?xml version=\"1.0\"?>\r\n<VHOSTS>\r\n",33,&nbw);

  mutex.lock();
  try
  {
    list=this->getVHostList();
    while(list)
    {
      char port[6];
      Vhost::sIpList *ipList;
      Vhost::sHostList *hostList;
      out.writeToFile("<VHOST>\r\n",9,&nbw);
      
      out.writeToFile("<NAME>",6,&nbw);
      out.writeToFile(list->host->getName(), strlen(list->host->getName()), &nbw);
      out.writeToFile("</NAME>\r\n",9,&nbw);
      
      ipList = list->host->getIpList();
      while(ipList)
      {
        out.writeToFile("<IP>",4,&nbw);
        out.writeToFile(ipList->hostIp.c_str(), ipList->hostIp.length(), &nbw);
        out.writeToFile("</IP>\r\n",7,&nbw);
        ipList=ipList->next;
      }
      hostList = list->host->getHostList();
      while(hostList)
      {
        out.writeToFile("<HOST>",6,&nbw);
        out.writeToFile(hostList->hostName.c_str(), 
                        hostList->hostName.length(), &nbw);
        out.writeToFile("</HOST>\r\n",9,&nbw);
        hostList=hostList->next;
      }
      out.writeToFile("<PORT>",6,&nbw);
      sprintf(port,"%i",list->host->getPort());
      out.writeToFile(port,(u_long)strlen(port),&nbw);
      out.writeToFile("</PORT>\r\n",9,&nbw);

      if(list->host->getVhostSSLContext()->privateKeyFile.length())
      {
        out.writeToFile("<SSL_PRIVATEKEY>",16,&nbw);
        out.writeToFile(list->host->getVhostSSLContext()->privateKeyFile.c_str(),
            (u_long)list->host->getVhostSSLContext()->privateKeyFile.length(),&nbw);
        out.writeToFile("</SSL_PRIVATEKEY>\r\n",19,&nbw);
      }
      
      if(list->host->getVhostSSLContext()->certificateFile.length())
      {
        out.writeToFile("<SSL_CERTIFICATE>",17,&nbw);
        out.writeToFile(list->host->getVhostSSLContext()->certificateFile.c_str(),
          (u_long)list->host->getVhostSSLContext()->certificateFile.length(),&nbw);
        out.writeToFile("</SSL_CERTIFICATE>\r\n",20,&nbw);
      }

      if(list->host->getVhostSSLContext()->password.length())
      {
        out.writeToFile("<SSL_PASSWORD>",14,&nbw);
        out.writeToFile(list->host->getVhostSSLContext()->password.c_str(),
                        list->host->getVhostSSLContext()->password.length(),&nbw);
        out.writeToFile("</SSL_PASSWORD>\r\n",17,&nbw);
      }

      out.writeToFile("<PROTOCOL>",10,&nbw);
      switch( list->host->getProtocol())
      {
			  case PROTOCOL_HTTP:
          out.writeToFile("HTTP",4,&nbw);
          break;
			  case PROTOCOL_HTTPS:
			  	out.writeToFile("HTTPS",5,&nbw);
			  	break;
			  case PROTOCOL_FTP:
			  	out.writeToFile("FTP",3,&nbw);
			  	break;
			  case PROTOCOL_CONTROL:
				  out.writeToFile("CONTROL", 7, &nbw);
				  break;
			  default:			
				  out.writeToFile(list->host->getProtocolName(), 
                          strlen(list->host->getProtocolName()), &nbw);
				  break;
      }
      out.writeToFile("</PROTOCOL>\r\n",13,&nbw);
      
      out.writeToFile("<DOCROOT>",9,&nbw);
      out.writeToFile(list->host->getDocumentRoot(), 
                      (u_long)strlen(list->host->getDocumentRoot()), &nbw);
      out.writeToFile("</DOCROOT>\r\n",12,&nbw);
      
      out.writeToFile("<SYSFOLDER>",11,&nbw);
      out.writeToFile(list->host->getSystemRoot(), 
                      (u_long)strlen(list->host->getSystemRoot()), &nbw);
      
      out.writeToFile("</SYSFOLDER>\r\n",14,&nbw);
      
      out.writeToFile("<ACCESSESLOG>",13,&nbw);
      out.writeToFile(list->host->getAccessesLogFileName(),
                      (u_long)strlen(list->host->getAccessesLogFileName()), &nbw);
      out.writeToFile("</ACCESSESLOG>\r\n",16,&nbw);
      
      out.writeToFile("<WARNINGLOG>",12,&nbw);
      out.writeToFile(list->host->getWarningsLogFileName(),
                      (u_long)strlen(list->host->getWarningsLogFileName()),&nbw);
      out.writeToFile("</WARNINGLOG>\r\n",15,&nbw);
      
      out.writeToFile("</VHOST>\r\n",10,&nbw);
      list=list->next;
    }
    out.writeToFile("</VHOSTS>\r\n",11,&nbw);
    out.closeFile();
    mutex.unlock();
  }
  catch(...)
  {
    mutex.unlock();
  };
}

/*!
 *Initialize SSL on the virtual host.
 */
int Vhost::initializeSSL()
{
	DynamicProtocol* dp;
  sslContext.context = 0;
  sslContext.method = 0;
#ifndef DO_NOT_USE_SSL

	dp = lserver->getDynProtocol(protocolName.c_str());
  if(this->protocol<1000 && !(dp && 
                              (dp->getOptions() & PROTOCOL_USES_SSL)) )
    return -2;
  sslContext.method = SSLv23_method();
  sslContext.context = SSL_CTX_new(sslContext.method);
  if(sslContext.context==0)
    return -1;
  
  /*!
   *The specified file doesn't exist.
   */
  if(File::fileExists(sslContext.certificateFile.c_str()) == 0)
  {
    return -1;
  }
  
  if(!(SSL_CTX_use_certificate_chain_file(sslContext.context,
                                          sslContext.certificateFile.c_str())))
    return -1;
  SSL_CTX_set_default_passwd_cb_userdata(sslContext.context, 
                                         &sslContext.password);
  SSL_CTX_set_default_passwd_cb(sslContext.context, password_cb);
  /*!
   *The specified file doesn't exist.
   */
  if(File::fileExists(sslContext.privateKeyFile) == 0)
    return -1;
  if(!(SSL_CTX_use_PrivateKey_file(sslContext.context, 
            sslContext.privateKeyFile.c_str(), SSL_FILETYPE_PEM)))
    return -1;

#if (OPENSSL_VERSION_NUMBER < 0x0090600fL)
  SSL_CTX_set_verify_depth(ctx,1);
#endif
	return 1;
#else
	return 1;
#endif
}	

/*!
 *Generate a RSA key and pass it to the SSL context.
 */
void Vhost::generateRsaKey()
{
#ifndef DO_NOT_USE_SSL
  RSA *rsa;

  rsa = RSA_generate_key(512, RSA_F4, NULL, NULL);

  if (!SSL_CTX_set_tmp_rsa(sslContext.context, rsa))
    return;

  RSA_free(rsa);
#endif
}

#ifndef DO_NOT_USE_SSL

/*!
 *Get the SSL context.
 */
SSL_CTX* Vhost::getSSLContext()
{
	return sslContext.context;
}
#endif

/*!
 *Clean the memory used by the SSL context.
 */
int Vhost::freeSSL()
{
#ifndef DO_NOT_USE_SSL
  int ret=0;
	if(sslContext.context)
  {
    SSL_CTX_free(sslContext.context);
    ret=1;
  }
	else 
		ret = 0;
  sslContext.certificateFile.assign("");
  sslContext.privateKeyFile.assign("");
  return ret;
#else
	return 1;
#endif
}

/*!
 *Get a virtual host by its position in the list.
 *Zero based list.
 */
Vhost* VhostManager::getVHostByNumber(int n)
{
	sVhostList *hl;
  mutex.lock();
  try
  {
    int i;
    hl=vhostList;
    if(extSource)
    {
      Vhost* ret=extSource->getVHostByNumber(n);
      mutex.unlock();
      return ret;
    }

    for(i=0;(i<n)&& hl;i++)
    {
      hl=hl->next;
    }
    if(hl)
    {
        hl->host->addRef();
    }
    mutex.unlock();
    return hl ? hl->host : 0;
  }
  catch(...)
  {
    mutex.unlock();
    return hl->host;
  };
}

/*!
 *Remove a virtual host by its position in the list
 *First position is zero.
 */
int VhostManager::removeVHost(int n)
{
	sVhostList *hl;
	sVhostList *bl=0;
	
  mutex.lock();
	try
  {
    hl=vhostList;
    for(int i=0;hl;i++)
    {
      if(i==n)
      {
        if(bl)
          bl->next =hl->next;
        else
          vhostList->next =hl->next;
        delete hl->host;
        mutex.unlock();
        return 1;
      }
      bl=hl;
      hl=hl->next;
    }
    mutex.unlock();
    return 0;
  }
  catch(...)
  {
    mutex.unlock();
    return 0;   
  };
}

/*!
 *Set an external source for the virtual hosts.
 */
void VhostManager::setExternalSource(VhostSource* nExtSource)
{
  mutex.lock();
  extSource = nExtSource;
  mutex.unlock();
}

/*!
 *Construct the object.
 */
VhostSource::VhostSource()
{

}

/*!
 *Destroy the object.
 */
VhostSource::~VhostSource()
{

}

/*!
 *Load the object.
 */
int VhostSource::load()
{
  return 0;
}

/*!
 *Free the object.
 */
int VhostSource::free()
{
  return 0;
}

/*!
 *Get a virtual host.
 */
Vhost* VhostSource::getVHost(const char*, const char*, u_short)
{
  return 0;
}

/*!
 *Get a virtual host by its number.
 */
Vhost* VhostSource::getVHostByNumber(int n)
{
  return 0;
}

/*!
 *Increment current references counter by 1.
 */
void Vhost::addRef()
{
  refCount++;
}

/*!
 *Decrement current references counter by 1.
 */
void Vhost::removeRef()
{
  refCount--;
}

/*!
 *Get the references counter.
 */
int Vhost::getRef()
{
  return refCount;
}

/*!
 *Set the reference counter for the virtual host.
 */
void Vhost::setRef(int n)
{
  refCount=n;
}
