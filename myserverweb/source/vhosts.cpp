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


/*!
 *SSL password callback function.
 */
static int password_cb(char *buf,int num,int /*!rwflag*/,void *userdata)
{
	if((size_t)num<strlen((char*)userdata)+1)
		return(0);

	strcpy(buf,(char*)userdata);
	return((int)strlen(buf));
}

/*!
 *vhost costructor
 */
Vhost::Vhost()
{
	sslContext.certificateFile=0;
	sslContext.method = 0;
	sslContext.privateKeyFile = 0;
	sslContext.password[0] = '\0';
	ipList=0;
	hostList=0;
  documentRoot=0;
	systemRoot=0;
  accessesLogFileName=0;
  warningsLogFileName=0;
  /*! 
   *By default use a non specified value for the throttling rate. -1 means that the
   *throttling rate was not specified, while 0 means it was specified but there is not
   *a limit.
   */
  throttlingRate = (u_long) -1;
}

/*!
 *Return the throttling rate to use with the virtual host.
 */
u_long Vhost::getThrottlingRate()
{
  return throttlingRate;
}

/*!
 *Destroy the vhost. 
 */
Vhost::~Vhost()
{
	clearHostList();
	clearIPList();
	freeSSL();

  if(documentRoot)
    delete [] documentRoot;
  if(systemRoot)
    delete [] systemRoot;
  if(accessesLogFileName)
    delete [] accessesLogFileName;
  if(warningsLogFileName)
    delete [] warningsLogFileName;

  warningsLogFile.close();
  accessesLogFile.close();
  
  documentRoot=0;
	systemRoot=0;
  accessesLogFileName=0;
  warningsLogFileName=0;

  mime_manager.clean();
}

/*!
 *Check if a MIME type file is defined for the virtual host.
 */
int Vhost::isMIME()
{
  return mime_manager.isLoaded();
}

/*!
 *Get the MIME manager for the virtual host.
 */
MimeManager* Vhost::getMIME()
{
  return &mime_manager;
}

/*!
 *Clear the list of the hosts
 */
void Vhost::clearHostList()
{
	sHostList* shl=hostList;
	sHostList* prevshl=0;
	while(shl)
	{
    prevshl->hostRegex.free();
		if(prevshl)
			delete prevshl;
		prevshl=shl;
		shl=shl->next;
	}
	if(prevshl)
		delete prevshl;
	hostList=0;
}

/*!
 *Clear the list of IPs
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
 *Add an IP address to the list
 */
void Vhost::addIP(char *ip, int isRegex)
{
	sIpList* il=new sIpList();
  if(il==0)
    return;
	strcpy(il->hostIp,ip);
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
void Vhost::removeIP(char *ip)
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
		if(!strcmp(iterator->hostIp,ip))
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
void Vhost::removeHost(char *host)
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
		if(!strcmp(iterator->hostName,host))
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
int Vhost::isHostAllowed(char* host)
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
		if(!strcmp(host,lhl->hostName))
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
 *by the local IP)
 */
int Vhost::isIPAllowed(char* ip)
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
		else if(!strcmp(ip,lipl->hostIp))
			return 1;
		lipl=lipl->next;
	}
	return 0;
}

/*!
 *Add an host to the allowed host list
 */
void Vhost::addHost(char *host, int isRegex)
{
	sHostList* hl=new sHostList();
  if(hl==0)
    return;
	strcpy(hl->hostName,host);
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
*Write to the accesses log file
*/
int Vhost::accessesLogWrite(char* str)
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
int Vhost::warningsLogWrite(char* str)
{
	return warningsLogFile.write(str);
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
 *Get the max size of the log files.
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
void VhostManager::addVHost(Vhost* VHost)
{
	if(vhostList==0)
	{
		vhostList=new sVhostList();	
		vhostList->host=VHost;
		vhostList->next =0;
	}
	else
	{
		sVhostList* hostl=vhostList;
    /*!Append the new host to the end of the linked list. */
		for(;;)
		{
			if(hostl->next )
				hostl=hostl->next;
			else
				break;
		}
		hostl->next = new sVhostList();	
    if(hostl->next==0)
      return;
		hostl->next->next =0; /*!Make sure that next is null*/
		hostl->next->host=VHost;
	}
	
}
/*!
 *Get the vhost for the connection(if any)
 */
Vhost* VhostManager::getVHost(char* host,char* ip,u_short port)
{
	sVhostList* vhl;
	for(vhl=vhostList;vhl;vhl=vhl->next )
	{
		if(vhl->host->port!=port)/*!control if the host port is the correct one*/
			continue;
    /*!If ip is defined check that it is allowed to connect to the host.*/
		if(ip && !vhl->host->isIPAllowed(ip))
			continue;
    /*!If host is defined check that it is allowed to connect to the host.*/
		if(host && !vhl->host->isHostAllowed(host))
			continue;
		return vhl->host;/*!we find a valid host*/
	}
	return 0;
}
/*!
 *VhostManager costructor
 */
VhostManager::VhostManager()
{
	vhostList=0;
  
}

/*!
 *Clean the virtual hosts.
 */
void VhostManager::clean()
{
	sVhostList* shl=vhostList;
	sVhostList* prevshl=0;
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
}
/*!
 *vhostmanager destructor.
 */
VhostManager::~VhostManager()
{
	clean();
}
/*!
 *Load the virtual hosts from a configuration file.
 */
int VhostManager::loadConfigurationFile(char* filename,int maxlogSize)
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
	char buffer[MYSERVER_KB(10)];/*!Exists a line greater than 10 KB?!?*/
	char buffer2[256];
	u_long nbr;/*!Number of bytes read from the file*/
	File fh;
	Vhost *vh;
	char c;
	int ret=fh.openFile(filename,FILE_OPEN_IFEXISTS|FILE_OPEN_READ);
	if(ret)/*!If the file cannot be opened simply do nothing*/
		return -1;

	for(;;)
	{
		int cc=0;
		buffer[0]='\0';
		for(;;)/*!Save a line in the buffer. A line ends with a diesis.*/
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
		if(buffer[0]=='\0')/*!If the buffer is only a point, we reached the file end*/
			break;		
		vh=new Vhost();
    if(vh==0)
    {
      fh.closeFile();
      clean();
      return -1;
    }
		/*!Parse the line. */
    vh->documentRoot = new char[MAX_PATH];/*Don't support long files. */
    vh->systemRoot  = new char[MAX_PATH];/*Don't support long files. */
    vh->accessesLogFileName  = new char[MAX_PATH];/*Don't support long files. */
    vh->warningsLogFileName = new char[MAX_PATH];/*Don't support long files. */
    cc = 0;
    /*!Get the hosts list. */
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
		vh->port=(u_short)atoi(buffer2);
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
			vh->protocol=PROTOCOL_HTTP;
		else if(!strcmp(buffer2,"HTTPS"))
			vh->protocol=PROTOCOL_HTTPS;
		else if(!strcmp(buffer2,"FTP"))
			vh->protocol=PROTOCOL_FTP;
		else if(!strcmp(buffer2,"CONTROL"))
			vh->protocol=PROTOCOL_CONTROL;
		else
		{
			vh->protocol=PROTOCOL_UNKNOWN;
		}
		strncpy(vh->protocol_name,buffer2,16);
		
		cc++;
		/*!Get the document root used by the virtual host*/
		buffer2[0]='\0';
		while(buffer[cc]!=';')
		{
			buffer2[strlen(buffer2)+1]='\0';
			buffer2[strlen(buffer2)]=buffer[cc];
			cc++;
		}	
		strcpy(vh->documentRoot,buffer2);
		cc++;
		/*!Get the system folder used by the virtual host*/
		buffer2[0]='\0';
		while(buffer[cc]!=';')
		{
			buffer2[strlen(buffer2)+1]='\0';
			buffer2[strlen(buffer2)]=buffer[cc];
			cc++;
		}	
		strcpy(vh->systemRoot,buffer2);
		cc++;
		/*!Get the accesses log file used by the virtual host*/
		buffer2[0]='\0';
		while(buffer[cc]!=';')
		{
			buffer2[strlen(buffer2)+1]='\0';
			buffer2[strlen(buffer2)]=buffer[cc];
			cc++;
		}	
		strcpy(vh->accessesLogFileName,buffer2);
		LogManager *accesses=vh->getAccessesLog();
    
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
		strcpy(vh->warningsLogFileName,buffer2);
		LogManager * warnings=vh->getWarningsLog();
		warnings->load(buffer2);
		vh->setMaxLogSize(maxlogSize);
		cc++;
		addVHost(vh);
	}
	fh.closeFile();
  return 0;
}

/*!
 *Save the virtual hosts to a configuration file.
 */
void VhostManager::saveConfigurationFile(char *filename)
{
	char buffer[1024];
	if(vhostList==0)
		return;
	sVhostList*vhl=vhostList;
	u_long nbw;
	File fh;
	fh.openFile(filename,FILE_CREATE_ALWAYS|FILE_OPEN_WRITE);
	for(;vhl;vhl=vhl->next )
	{
		Vhost* vh=vhl->host;
		Vhost::sHostList* hl=vh->hostList;
		if(hl)
		{
			while(hl)
			{ 
				fh.writeToFile(hl->hostName,(u_long)strlen(hl->hostName),&nbw);
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
		Vhost::sIpList* il=vh->ipList;
		if(il)
		{
			while(il)
			{ 
				fh.writeToFile(il->hostIp,(u_long)strlen(il->hostIp),&nbw);
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
		sprintf(buffer,";%u;",vh->port);
		fh.writeToFile(buffer,(u_long)strlen(buffer),&nbw);

		if(vh->protocol==PROTOCOL_HTTP)
		{
			strcpy(buffer,"HTTP;");
			fh.writeToFile(buffer,(u_long)strlen(buffer),&nbw);
		}
		else if(vh->protocol==PROTOCOL_HTTPS)
		{
			strcpy(buffer,"HTTPS;");
			fh.writeToFile(buffer,(u_long)strlen(buffer),&nbw);
		}
		else if(vh->protocol==PROTOCOL_CONTROL)
		{
			strcpy(buffer,"CONTROL;");
			fh.writeToFile(buffer,(u_long)strlen(buffer),&nbw);
		}
		else if(vh->protocol==PROTOCOL_FTP)
		{
			strcpy(buffer,"FTP;");
			fh.writeToFile(buffer,(u_long)strlen(buffer),&nbw);
		}
		else
		{
			fh.writeToFile(vh->protocol_name,(u_long)strlen(vh->protocol_name),&nbw);			
		}

		fh.writeToFile(vh->documentRoot,(u_long)strlen(vh->documentRoot),&nbw);
		strcpy(buffer,";");
		fh.writeToFile(buffer,(u_long)strlen(buffer),&nbw);

		fh.writeToFile(vh->systemRoot,(u_long)strlen(vh->systemRoot),&nbw);
		strcpy(buffer,";");
		fh.writeToFile(buffer,(u_long)strlen(buffer),&nbw);

		fh.writeToFile(vh->accessesLogFileName,(u_long)strlen(vh->accessesLogFileName),
                   &nbw);

		strcpy(buffer,";");
		fh.writeToFile(buffer,(u_long)strlen(buffer),&nbw);

		fh.writeToFile(vh->warningsLogFileName,(u_long)strlen(vh->warningsLogFileName),
                   &nbw);
		if(vhl->next )
			strcpy(buffer,";#\r\n");
		else
			strcpy(buffer,";##\r\n\0");
		fh.writeToFile(buffer,(u_long)strlen(buffer),&nbw);
	}
	fh.closeFile();
}
/*!
 *Returns the entire virtual hosts list.
 */
VhostManager::sVhostList* VhostManager::getVHostList()
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
	int i;
	if(max(n1,n2)>=getHostsNumber())
		return 0;
	vh1 = vhostList;
	for(i=0;i<n1;i++)
	{
		vh1=vh1->next;
	}
	sVhostList *vh2 = vhostList;
	for(i=0;i<n2;i++)
	{
		vh2=vh2->next;
	}
	return switchVhosts(vh1,vh2);

}
/*!
 *Switch two virtual hosts.
 */
int VhostManager::switchVhosts(sVhostList * vh1,sVhostList * vh2)
{
	if((vh1==0)|(vh2==0))
		return 0;
	Vhost* vh3=vh1->host;
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
int VhostManager::loadXMLConfigurationFile(char *filename,int maxlogSize)
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
				strcpy(vh->name,((char*)lcur->children->content));
			}
			if(!xmlStrcmp(lcur->name, (const xmlChar *)"SSL_PRIVATEKEY"))
			{
        vh->sslContext.privateKeyFile= 
          new char[strlen( (char*)lcur->children->content ) +1];

        if(vh->sslContext.privateKeyFile==0)
          return -1;
				strcpy(vh->sslContext.privateKeyFile,((char*)lcur->children->content));
			}
			if(!xmlStrcmp(lcur->name, (const xmlChar *)"SSL_CERTIFICATE"))
			{
        vh->sslContext.certificateFile= 
          new char[strlen( (char*)lcur->children->content ) +1];

        if(vh->sslContext.certificateFile==0)
          return -1;
				strcpy(vh->sslContext.certificateFile,((char*)lcur->children->content));
			}
			if(!xmlStrcmp(lcur->name, (const xmlChar *)"SSL_PASSWORD"))
			{
				if(lcur->children)
					strcpy(vh->sslContext.password,((char*)lcur->children->content));
				else
					vh->sslContext.password[0]='\0';
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
				vh->port=(u_short)atoi((char*)lcur->children->content);
			}
			if(!xmlStrcmp(lcur->name, (const xmlChar *)"PROTOCOL"))
			{
				if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"HTTP"))
					vh->protocol=PROTOCOL_HTTP;
				else if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"HTTPS"))
					vh->protocol=PROTOCOL_HTTPS;
				else if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"FTP"))
					vh->protocol=PROTOCOL_FTP;
				else if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"CONTROL"))
					vh->protocol=PROTOCOL_CONTROL;
				else
				{
						vh->protocol=PROTOCOL_UNKNOWN;
				}
				strncpy(vh->protocol_name,(char*)lcur->children->content,16);
				
			}
			if(!xmlStrcmp(lcur->name, (const xmlChar *)"DOCROOT"))
			{
        if(lcur->children && lcur->children->content)
        {
          int documentRootlen = strlen((char*)lcur->children->content)+1;
          vh->documentRoot = new char[documentRootlen];
          if(vh->documentRoot == 0)
          {
            parser.close();
            delete vh;
            clean();
            return -1;
          }
          strcpy(vh->documentRoot,(char*)lcur->children->content);
        }
        else
        {
          vh->documentRoot = 0;
        }
			}
      if(!xmlStrcmp(lcur->name, (const xmlChar *)"SYSFOLDER"))
			{
        if(lcur->children && lcur->children->content)
        {
          int systemRootlen = strlen((char*)lcur->children->content)+1;
          vh->systemRoot = new char[systemRootlen];
          if(vh->systemRoot == 0)
          {
            parser.close();
            delete vh;
            clean();
            return -1;
          }
          strcpy(vh->systemRoot,(char*)lcur->children->content);
        }
        else
        {
          vh->systemRoot=0;
        }
      }
			if(!xmlStrcmp(lcur->name, (const xmlChar *)"ACCESSESLOG"))
			{
        int accessesLogFileNameLen = strlen((char*)lcur->children->content)+1;
        vh->accessesLogFileName = new char[accessesLogFileNameLen];
        if(vh->accessesLogFileName == 0)
        {
          parser.close();
          delete vh;
          clean();
          return -1;
        }
				vh->accessLogOpt[0]='\0';                
				strcpy(vh->accessesLogFileName,(char*)lcur->children->content);

				xmlAttr *attr =  lcur->properties;
				u_long Optslen=LOG_FILES_OPTS_LEN;
				while(attr)
				{
					strncat(vh->accessLogOpt,(char*)attr->name,Optslen);
					Optslen-=(u_long)strlen((char*)attr->name);
					strncat(vh->accessLogOpt,"=",Optslen);
					Optslen-=1;
					strncat(vh->accessLogOpt,(char*)attr->children->content,Optslen);
					Optslen-=(u_long)strlen((char*)attr->children->content);
					if(attr->next)
					{
						strncat(vh->accessLogOpt,",",Optslen);
						Optslen-=1;
					}
					attr=attr->next;
				}
			}
			
			if(!xmlStrcmp(lcur->name, (const xmlChar *)"WARNINGLOG"))
			{
        int warningsLogFileNameLen = strlen((char*)lcur->children->content) +1;
        vh->warningsLogFileName = new char[warningsLogFileNameLen];
        if(vh->warningsLogFileName == 0)
        {
          parser.close();
          delete vh;
          clean();
          return -1;
        }
				strcpy(vh->warningsLogFileName,(char*)lcur->children->content);
				vh->accessLogOpt[0]='\0';                
				xmlAttr *attr =  lcur->properties;
				u_long Optslen=LOG_FILES_OPTS_LEN;
				while(attr)
				{
					strncat(vh->warningLogOpt,(char*)attr->name,Optslen);
					Optslen-=(u_long)strlen((char*)attr->name);
					strncat(vh->warningLogOpt,"=",Optslen);
					Optslen-=1;
					strncat(vh->warningLogOpt,(char*)attr->children->content,Optslen);
					Optslen-=(u_long)strlen((char*)attr->children->content);
					if(attr->next)
          {
            strncat(vh->warningLogOpt,",",Optslen);
						Optslen-=1;
					}

					attr=attr->next;
				}
      }

			if(!xmlStrcmp(lcur->name, (const xmlChar *)"MIME_FILE"))
			{
				if(lcur->children)
          vh->getMIME()->loadXML((char*)lcur->children->content);
			}

			if(!xmlStrcmp(lcur->name, (const xmlChar *)"THROTTLING_RATE"))
			{
				vh->throttlingRate = (u_long)atoi((char*)lcur->children->content);
 			}
      lcur=lcur->next;
    }

    accessLogFile=vh->getAccessesLog();
    accessLogFile->load(vh->accessesLogFileName);
    
    warningLogFile = vh->getWarningsLog();
    warningLogFile->load(vh->warningsLogFileName);

    vh->setMaxLogSize(maxlogSize);
    vh->initializeSSL();
    addVHost(vh);
    
  }
  parser.close();
  return 0;
}

/*!
 *Save the virtual hosts to a XML configuration file.
 */
void VhostManager::saveXMLConfigurationFile(char *filename)
{
	File out;
	u_long nbw;
	out.openFile(filename,FILE_CREATE_ALWAYS|FILE_OPEN_WRITE);
	out.writeToFile("<?xml version=\"1.0\"?>\r\n<VHOSTS>\r\n",33,&nbw);
	sVhostList *list=this->getVHostList();
	while(list)
	{
		char port[6];
		out.writeToFile("<VHOST>\r\n",9,&nbw);

		out.writeToFile("<NAME>",6,&nbw);
		out.writeToFile(list->host->name,(u_long)strlen(list->host->name),&nbw);
		out.writeToFile("</NAME>\r\n",9,&nbw);

		Vhost::sIpList *ipList = list->host->ipList;
		while(ipList)
		{
			out.writeToFile("<IP>",4,&nbw);
			out.writeToFile(ipList->hostIp,(u_long)strlen(ipList->hostIp),&nbw);
			out.writeToFile("</IP>\r\n",7,&nbw);
			ipList=ipList->next;
		}
		Vhost::sHostList *hostList = list->host->hostList;
		while(hostList)
		{
			out.writeToFile("<HOST>",6,&nbw);
			out.writeToFile(hostList->hostName,(u_long)strlen(hostList->hostName),&nbw);
			out.writeToFile("</HOST>\r\n",9,&nbw);
			hostList=hostList->next;
		}
		out.writeToFile("<PORT>",6,&nbw);
		sprintf(port,"%i",list->host->port);
		out.writeToFile(port,(u_long)strlen(port),&nbw);
		out.writeToFile("</PORT>\r\n",9,&nbw);

		if(list->host->sslContext.privateKeyFile)
		{
			out.writeToFile("<SSL_PRIVATEKEY>",16,&nbw);
			out.writeToFile(list->host->sslContext.privateKeyFile,
                      (u_long)strlen(list->host->sslContext.privateKeyFile),&nbw);
			out.writeToFile("</SSL_PRIVATEKEY>\r\n",19,&nbw);
		}

		if(list->host->sslContext.certificateFile)
		{
			out.writeToFile("<SSL_CERTIFICATE>",17,&nbw);
			out.writeToFile(list->host->sslContext.certificateFile,
                      (u_long)strlen(list->host->sslContext.certificateFile),&nbw);
			out.writeToFile("</SSL_CERTIFICATE>\r\n",20,&nbw);
		}

		if(list->host->sslContext.password[0])
		{
			out.writeToFile("<SSL_PASSWORD>",14,&nbw);
			out.writeToFile(list->host->sslContext.password,
                      (u_long)strlen(list->host->sslContext.password),&nbw);
			out.writeToFile("</SSL_PASSWORD>\r\n",17,&nbw);
		}

		out.writeToFile("<PROTOCOL>",10,&nbw);
		switch( list->host->protocol)
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
				out.writeToFile(list->host->protocol_name,3,&nbw);
				break;
		}
		out.writeToFile("</PROTOCOL>\r\n",13,&nbw);

		out.writeToFile("<DOCROOT>",9,&nbw);
		out.writeToFile(list->host->documentRoot,(u_long)strlen(list->host->documentRoot),
                    &nbw);
		out.writeToFile("</DOCROOT>\r\n",12,&nbw);

		out.writeToFile("<SYSFOLDER>",11,&nbw);
		out.writeToFile(list->host->systemRoot,(u_long)strlen(list->host->systemRoot),
                    &nbw);

		out.writeToFile("</SYSFOLDER>\r\n",14,&nbw);

		out.writeToFile("<ACCESSESLOG>",13,&nbw);
		out.writeToFile(list->host->accessesLogFileName,
                    (u_long)strlen(list->host->accessesLogFileName),&nbw);
		out.writeToFile("</ACCESSESLOG>\r\n",16,&nbw);

		out.writeToFile("<WARNINGLOG>",12,&nbw);
		out.writeToFile(list->host->warningsLogFileName,
                    (u_long)strlen(list->host->warningsLogFileName),&nbw);
		out.writeToFile("</WARNINGLOG>\r\n",15,&nbw);

		out.writeToFile("</VHOST>\r\n",10,&nbw);
		list=list->next;
	}
	out.writeToFile("</VHOSTS>\r\n",11,&nbw);
	out.closeFile();
}

/*!
 *Initialize SSL on the virtual host.
 */
int Vhost::initializeSSL()
{
  sslContext.context = 0;
  sslContext.method = 0;
#ifndef DO_NOT_USE_SSL

	DynamicProtocol* dp = lserver->getDynProtocol(protocol_name);
  if(this->protocol<1000 && !(dp && ( dp->getOptions() &  PROTOCOL_USES_SSL ))  )
    return -2;
  sslContext.method = SSLv23_method();
  sslContext.context = SSL_CTX_new(sslContext.method);
  if(sslContext.context==0)
    return -1;
  
  /*!
   *The specified file doesn't exist.
   */
  if(File::fileExists(sslContext.certificateFile) == 0)
  {
    return -1;
  }
  
  if(!(SSL_CTX_use_certificate_chain_file(sslContext.context,
                                          sslContext.certificateFile)))
    return -1;
  SSL_CTX_set_default_passwd_cb_userdata(sslContext.context, sslContext.password);
  SSL_CTX_set_default_passwd_cb(sslContext.context, password_cb);
  /*!
   *The specified file doesn't exist.
   */
  if(File::fileExists(sslContext.privateKeyFile) == 0)
    return -1;
  if(!(SSL_CTX_use_PrivateKey_file(sslContext.context, sslContext.privateKeyFile, 
                                   SSL_FILETYPE_PEM)))
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
  if(sslContext.certificateFile)
    delete [] sslContext.certificateFile;
  if(sslContext.privateKeyFile)
    delete [] sslContext.privateKeyFile;
  sslContext.certificateFile = 0;
  sslContext.privateKeyFile = 0;
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
	sVhostList *hl=vhostList;
	for(int i=0;(i<n)&& hl;i++)
	{
		hl=hl->next;
	}
	return hl->host;
}

/*!
 *Remove a virtual host by its position in the list
 *First position is zero.
 */
int VhostManager::removeVHost(int n)
{
	sVhostList *hl=vhostList;
	sVhostList *bl=0;
	for(int i=0;hl;i++)
	{
		if(i==n)
		{
			if(bl)
			{
				bl->next =hl->next;
			}
			else
			{
				vhostList->next =hl->next;
			}
			delete hl->host;
			return 1;
		}
		bl=hl;
		hl=hl->next;
	}
	return 0;
}

