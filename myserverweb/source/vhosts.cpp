/*
*MyServer
*Copyright (C) 2002 The MyServer Team
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
#include "../include/cXMLParser.h"
#include "../include/connectionstruct.h"/*!Used for protocols IDs*/
#include "../include/stringutils.h"
#include "../include/threads.h"

/*!
*vhost costructor
*/
vhost::vhost()
{
	sslContext.certificateFile[0]='\0';
	sslContext.method = 0;
	sslContext.privateKeyFile[0] = '\0';
	sslContext.password[0] = '\0';
	ipList=0;
	hostList=0;
	accessesLogFileAccess.myserver_mutex_init();
	warningsLogFileAccess.myserver_mutex_init();
}
/*!
*vhost class destructor
*/
vhost::~vhost()
{
	clearHostList();
	clearIPList();
	freeSSL();
	if(accessesLogFile.getHandle())
		accessesLogFile.closeFile();
	if(warningsLogFile.getHandle())
		warningsLogFile.closeFile();
	memset(this,0,sizeof(vhost));
	accessesLogFileAccess.myserver_mutex_destroy();
	warningsLogFileAccess.myserver_mutex_destroy();
	
}
/*!
*Clear the list of the hosts
*/
void vhost::clearHostList()
{
	sHostList* shl=hostList;
	sHostList* prevshl=0;
	while(shl)
	{
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
void vhost::clearIPList()
{
	sIpList* sil=ipList;
	sIpList* prevsil=0;
	while(sil)
	{
		if(prevsil)
			delete prevsil;
		prevsil=sil;
		sil=sil->next;
	}
	if(prevsil)
		delete prevsil;
	ipList=0;
}
/*!
*Add an IP address to the list
*/
void vhost::addIP(char *ip)
{
	sIpList* il=new sIpList();
	strcpy(il->hostIp,ip);
	if(ipList)
	{
		il->next =ipList;
	}
	else
	{
		il->next =0;
	}
	ipList=il;
}
/*!
*Remove the IP address to the list
*/
void vhost::removeIP(char *ip)
{
	vhost::sIpList *iterator=ipList;
	vhost::sIpList *iteratorBack=0;
	if(iterator==0)
		return;
	while(iterator)
	{
		/*!
		*If this is the virtual host with the right IP
		*/
		if(!strcmp(iterator->hostIp,ip))
		{
			if(iteratorBack)
			{
				iteratorBack->next =iterator->next;
				delete iterator;
				return;
			}
			else
			{
				ipList=iterator->next;
				delete iterator;
				return;
			}
		}
		iteratorBack=iterator;	
		iterator=iterator->next;
	}

}
/*!
*Remove the host address to the list
*/
void vhost::removeHost(char *host)
{
	vhost::sHostList *iterator=hostList;
	vhost::sHostList *iteratorBack=0;
	if(iterator==0)
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
				delete iterator;
				return;
			}
			else
			{
				hostList=iterator->next;
				delete iterator;
				return;
			}
		}
		iteratorBack=iterator;	
		iterator=iterator->next;
	}
}
/*!
*Check if an host is allowed to the connection
*/
int vhost::isHostAllowed(char* host)
{
	if(hostList==0)
		return 1;
	sHostList *lhl=hostList;
	while(lhl)
	{
		if(!strcmp(host,lhl->hostName))
			return 1;
		lhl=lhl->next;
	}
	return 0;
}
/*!
*Check if all the host are allowed to the connection
*/
int vhost::areAllHostAllowed()
{
	if(hostList==0)
		return 1;
	return 0;
}
/*!
*Check if all the IPs are allowed to the connection
*/
int vhost::areAllIPAllowed()
{
	if(ipList==0)
		return 1;
	return 0;
}
/*!
*Check if the network is allowed to the connection(control the network by the local IP)
*/
int vhost::isIPAllowed(char* ip)
{
	if(ipList==0)
		return 1;
	sIpList *lipl=ipList;
	while(lipl)
	{
		if(!strcmp(ip,lipl->hostIp))
			return 1;
		lipl=lipl->next;
	}
	return 0;
}
/*!
*Add an host to the allowed host list
*/
void vhost::addHost(char *host)
{
	sHostList* hl=new sHostList();
	strcpy(hl->hostName,host);
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
u_long vhost::accesseslogRequestAccess(int id)
{
	return accessesLogFileAccess.myserver_mutex_lock(id);
}
/*!
*Here threads get the permission to use the warnings log file.
*/
u_long vhost::warningslogRequestAccess(int id)
{
	return warningsLogFileAccess.myserver_mutex_lock(id);
}
/*!
*Here threads release the permission to use the access log file.
*/
u_long vhost::accesseslogTerminateAccess(int id)
{
	return accessesLogFileAccess.myserver_mutex_unlock(id);
}
/*!
*Here threads release the permission to use the warnings log file.
*/
u_long vhost::warningslogTerminateAccess(int id)
{
	return warningsLogFileAccess.myserver_mutex_unlock(id);
}

/*!
*Write to the accesses log file
*/
u_long vhost::accessesLogWrite(char* str)
{
	u_long nbw;
	if(accessesLogFile.getFileSize()>getMaxLogSize())
		return 0;
	accessesLogFile.writeToFile(str,(u_long)strlen(str),&nbw);
	return nbw;
}
/*!
*Return a pointer to the file used by the accesses log
*/
MYSERVER_FILE* vhost::getAccessesLogFile()
{
	return &accessesLogFile;
}
/*!
*Write to the warnings log file
*/
u_long vhost::warningsLogWrite(char* str)
{
	u_long nbw;
	if(warningsLogFile.getFileSize()>getMaxLogSize())
		return 0;
	warningsLogFile.writeToFile(str,(u_long)strlen(str),&nbw);
	return nbw;
}
/*!
*Return a pointer to the file used by the warnings log
*/
MYSERVER_FILE* vhost::getWarningsLogFile()
{
	return &warningsLogFile;
}
/*!
*Set the max size of the log files.
*/
void vhost::setMaxLogSize(u_long newSize)
{
	maxLogSize=newSize;
}
/*!
*Get the max size of the log files.
*/
u_long vhost::getMaxLogSize()
{
	return maxLogSize;
}

/*!
*vhostmanager costructor
*/
void vhostmanager::addvHost(vhost* vHost)
{
	if(vhostList==0)
	{
		vhostList=new sVhostList();	
		vhostList->host=vHost;
		vhostList->next =0;
	}
	else
	{
		sVhostList* hostl=vhostList;
		for(;;)/*!Append the new host to the end of the linked list*/
		{
			if(hostl->next )
				hostl=hostl->next;
			else
				break;
		}
		hostl->next =new sVhostList();	

		hostl->next ->next =0;/*!Make sure that next is null*/
		hostl->next ->host=vHost;
	}
	
}
/*!
*Get the vhost for the connection(if any)
*/
vhost* vhostmanager::getvHost(char* host,char* ip,u_short port)
{
	sVhostList* vhl;
	for(vhl=vhostList;vhl;vhl=vhl->next )
	{
		if(vhl->host->port!=port)/*!control if the host port is the correct one*/
			continue;
		if(ip && !vhl->host->isIPAllowed(ip)) /*!If ip is defined check that it is allowed to connect to the host*/
			continue;
		if(host && !vhl->host->isHostAllowed(host))/*!If host is defined check that it is allowed to connect to the host*/
			continue;
		return vhl->host;/*!we find a valid host*/
	}
	return 0;
}
/*!
*vhostmanager costructor
*/
vhostmanager::vhostmanager()
{
	vhostList=0;
}
/*!
*Clean the virtual hosts
*/
void vhostmanager::clean()
{
	sVhostList* shl=vhostList;
	sVhostList* prevshl=0;
	while(shl)
	{
		if(prevshl)
		{
			delete prevshl;
		}
		prevshl=shl;
		shl=shl->next;
	}
	if(prevshl)
		delete prevshl;
	memset(this,0,sizeof(vhostmanager));
}
/*!
*vhostmanager destructor
*/
vhostmanager::~vhostmanager()
{
	clean();
}
/*!
*Load the virtual hosts from a configuration file
*/
void vhostmanager::loadConfigurationFile(char* filename,int maxlogSize)
{
	/*!
	FILE STRUCTURE:
	hosts;IPs;port;protocol;documentRoot;systemFolder;accessesLog;warningLog#
	1)hosts is a list of valid hosts name that the virtual host can accept, every value is separated
	by a comma. If host is 0 every host name is valid.
	2)IPs is a list of valid IP that the virtual host can accept, every value is separated
	by a comma. If IPs is 0 every IP name is valid.
	3)port in the port used to listen.
	4)protocol is the protocol used(HTTP,FTP...). By default is HTTP.
	5)documentRoot is the path to the web folder.
	6)systemFolder is the path to the system folder.
	7-8)accessesLog;warningLog are strings saying where put the accesses and the warningLog file for
		this virtual host.
	The last virtual host ends with two final diesis, '##'.
	Comments can be done like are done comments on multiline in C++, between "/ *" and "* /" without the
	space.
	In 5) and 6) for use absolute path use the character | before the full path.
	*/
	char path[MAX_PATH];
	getdefaultwd(path,MAX_PATH);
	char buffer[KB(10)];/*!Exists a line greater than 10 KB?!?*/
	char buffer2[256];
	u_long nbr;/*!Number of bytes read from the file*/
	MYSERVER_FILE fh;
	int ret=fh.openFile(filename,MYSERVER_FILE_OPEN_IFEXISTS|MYSERVER_FILE_OPEN_READ);
	if((ret==0)||(ret==-1))/*!If the file cannot be opened simply do nothing*/
		return;
	char c;
	for(;;)
	{
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
		vhost *vh=new vhost();
		/*!Parse the line*/

		int cc=0;
		for(;;)/*!Get the hosts list*/
		{
			buffer2[0]='\0';
			while((buffer[cc]!=',')&&(buffer[cc]!=';'))
			{
				buffer2[strlen(buffer2)+1]='\0';
				buffer2[strlen(buffer2)]=buffer[cc];
				cc++;
			}
			if(buffer2[0]&&buffer2[0]!='0')/*!If the host list is equal to 0 don't add anything to the list*/
				vh->addHost(buffer2);
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
			if(buffer2[0]&&buffer2[0]!='0')/*!If the ip list is equal to 0 don't add anything to the list*/
				vh->addIP(buffer2);
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
		MYSERVER_FILE *accesses=vh->getAccessesLogFile();
		accesses->openFile(buffer2,MYSERVER_FILE_OPEN_APPEND|MYSERVER_FILE_OPEN_ALWAYS|MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_NO_INHERIT );
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
		MYSERVER_FILE * warnings=vh->getWarningsLogFile();
		warnings->openFile(buffer2,MYSERVER_FILE_OPEN_APPEND|MYSERVER_FILE_OPEN_ALWAYS|MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_NO_INHERIT);
		vh->setMaxLogSize(maxlogSize);
		cc++;
		addvHost(vh);
	}
	fh.closeFile();
}
/*!
*Save the virtual hosts to a configuration file
*/
void vhostmanager::saveConfigurationFile(char *filename)
{
	char buffer[1024];
	if(vhostList==0)
		return;
	sVhostList*vhl=vhostList;
	u_long nbw;
	MYSERVER_FILE fh;
	fh.openFile(filename,MYSERVER_FILE_CREATE_ALWAYS|MYSERVER_FILE_OPEN_WRITE);
	for(;vhl;vhl=vhl->next )
	{
		vhost*vh=vhl->host;
		vhost::sHostList* hl=vh->hostList;
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
		vhost::sIpList* il=vh->ipList;
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

		fh.writeToFile(vh->accessesLogFileName,(u_long)strlen(vh->accessesLogFileName),&nbw);
		strcpy(buffer,";");
		fh.writeToFile(buffer,(u_long)strlen(buffer),&nbw);

		fh.writeToFile(vh->warningsLogFileName,(u_long)strlen(vh->warningsLogFileName),&nbw);
		if(vhl->next )
			strcpy(buffer,";#\r\n");
		else
			strcpy(buffer,";##\r\n\0");
		fh.writeToFile(buffer,(u_long)strlen(buffer),&nbw);
	}
	fh.closeFile();
}
/*!
*returns the entire virtual hosts list
*/
vhostmanager::sVhostList* vhostmanager::getvHostList()
{
	return this->vhostList;
}
/*!
*Switch the position between two virtual hosts
*Zero based index
*/
int vhostmanager::switchVhosts(int n1,int n2)
{
	if(max(n1,n2)>=getHostsNumber())
		return 0;
	sVhostList *vh1 = vhostList;
	int i;
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
*Switch two virtual hosts
*/
int vhostmanager::switchVhosts(sVhostList * vh1,sVhostList * vh2)
{
	if((vh1==0)|(vh2==0))
		return 0;
	vhost* vh3=vh1->host;
	vh1->host = vh2->host;
	vh2->host = vh3;
	return 1;
}
/*!
*Returns the number of hosts in the list
*/
int vhostmanager::getHostsNumber()
{
	sVhostList *vh = vhostList;
	int i;
	for(i=0;vh;i++,vh=vh->next );
	return i;
}

/*!
*Load the virtual hosts from a XML configuration file
*/
void vhostmanager::loadXMLConfigurationFile(char *filename,int maxlogSize)
{
	char path[MAX_PATH];
	getdefaultwd(path,MAX_PATH);
	cXMLParser parser;
	if(int r=parser.open(filename))
	{
		return;
	}
	xmlDocPtr doc = parser.getDoc();
	xmlNodePtr node=doc->children->children;
	for(;node;node=node->next )
	{
		if(xmlStrcmp(node->name, (const xmlChar *)"VHOST"))
			continue;
		xmlNodePtr lcur=node->children;
		vhost *vh=new vhost();
		memset(vh,0,sizeof(vh));

		while(lcur)
		{
			if(!xmlStrcmp(lcur->name, (const xmlChar *)"HOST"))
			{
				vh->addHost((char*)lcur->children->content);
			}
			if(!xmlStrcmp(lcur->name, (const xmlChar *)"NAME"))
			{
				strcpy(vh->name,((char*)lcur->children->content));
			}
			if(!xmlStrcmp(lcur->name, (const xmlChar *)"SSL_PRIVATEKEY"))
			{
				strcpy(vh->sslContext.privateKeyFile,((char*)lcur->children->content));
			}
			if(!xmlStrcmp(lcur->name, (const xmlChar *)"SSL_CERTIFICATE"))
			{
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
				vh->addIP((char*)lcur->children->content);
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
				else
				{
						vh->protocol=PROTOCOL_UNKNOWN;
				}
				strncpy(vh->protocol_name,(char*)lcur->children->content,16);
				
			}
			if(!xmlStrcmp(lcur->name, (const xmlChar *)"DOCROOT"))
			{
				strcpy(vh->documentRoot,(char*)lcur->children->content);
			}
			if(!xmlStrcmp(lcur->name, (const xmlChar *)"SYSFOLDER"))
			{
				strcpy(vh->systemRoot,(char*)lcur->children->content);
			}
			if(!xmlStrcmp(lcur->name, (const xmlChar *)"ACCESSESLOG"))
			{
				strcpy(vh->accessesLogFileName,(char*)lcur->children->content);
			}
			if(!xmlStrcmp(lcur->name, (const xmlChar *)"WARNINGLOG"))
			{
				strcpy(vh->warningsLogFileName,(char*)lcur->children->content);
			}
			
			lcur=lcur->next;
		}
		MYSERVER_FILE *accesses=vh->getAccessesLogFile();
		accesses->openFile(vh->accessesLogFileName,MYSERVER_FILE_OPEN_APPEND|MYSERVER_FILE_OPEN_ALWAYS|MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_NO_INHERIT);
		MYSERVER_FILE * warnings=vh->getWarningsLogFile();
		warnings->openFile(vh->warningsLogFileName,MYSERVER_FILE_OPEN_APPEND|MYSERVER_FILE_OPEN_ALWAYS|MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_NO_INHERIT);
		vh->setMaxLogSize(maxlogSize);
		vh->initializeSSL();
		addvHost(vh);
	}
	parser.close();
}
/*!
*Save the virtual hosts to a XML configuration file
*/
void vhostmanager::saveXMLConfigurationFile(char *filename)
{
	MYSERVER_FILE out;
	u_long nbw;
	out.openFile(filename,MYSERVER_FILE_CREATE_ALWAYS|MYSERVER_FILE_OPEN_WRITE);
	out.writeToFile("<?xml version=\"1.0\"?>\r\n<VHOSTS>\r\n",33,&nbw);
	sVhostList *list=this->getvHostList();
	while(list)
	{
		out.writeToFile("<VHOST>\r\n",9,&nbw);

		out.writeToFile("<NAME>",6,&nbw);
		out.writeToFile(list->host->name,(u_long)strlen(list->host->name),&nbw);
		out.writeToFile("</NAME>\r\n",9,&nbw);

		vhost::sIpList *ipList = list->host->ipList;
		while(ipList)
		{
			out.writeToFile("<IP>",4,&nbw);
			out.writeToFile(ipList->hostIp,(u_long)strlen(ipList->hostIp),&nbw);
			out.writeToFile("</IP>\r\n",7,&nbw);
			ipList=ipList->next;
		}
		vhost::sHostList *hostList = list->host->hostList;
		while(hostList)
		{
			out.writeToFile("<HOST>",6,&nbw);
			out.writeToFile(hostList->hostName,(u_long)strlen(hostList->hostName),&nbw);
			out.writeToFile("</HOST>\r\n",9,&nbw);
			hostList=hostList->next;
		}
		out.writeToFile("<PORT>",6,&nbw);
		char port[6];
		sprintf(port,"%i",list->host->port);
		out.writeToFile(port,(u_long)strlen(port),&nbw);
		out.writeToFile("</PORT>\r\n",9,&nbw);

		if(list->host->sslContext.privateKeyFile[0])
		{
			out.writeToFile("<SSL_PRIVATEKEY>",16,&nbw);
			out.writeToFile(list->host->sslContext.privateKeyFile,(u_long)strlen(list->host->sslContext.privateKeyFile),&nbw);
			out.writeToFile("</SSL_PRIVATEKEY>\r\n",19,&nbw);
		}

		if(list->host->sslContext.certificateFile[0])
		{
			out.writeToFile("<SSL_CERTIFICATE>",17,&nbw);
			out.writeToFile(list->host->sslContext.certificateFile,(u_long)strlen(list->host->sslContext.certificateFile),&nbw);
			out.writeToFile("</SSL_CERTIFICATE>\r\n",20,&nbw);
		}

		if(list->host->sslContext.password[0])
		{
			out.writeToFile("<SSL_PASSWORD>",15,&nbw);
			out.writeToFile(list->host->sslContext.password,(u_long)strlen(list->host->sslContext.password),&nbw);
			out.writeToFile("</SSL_PASSWORD>\r\n",18,&nbw);
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
			default:			
				out.writeToFile(list->host->protocol_name,3,&nbw);
				break;
		}
		out.writeToFile("</PROTOCOL>\r\n",13,&nbw);

		out.writeToFile("<DOCROOT>",9,&nbw);
		out.writeToFile(list->host->documentRoot,(u_long)strlen(list->host->documentRoot),&nbw);
		out.writeToFile("</DOCROOT>\r\n",12,&nbw);

		out.writeToFile("<SYSFOLDER>",11,&nbw);
		out.writeToFile(list->host->systemRoot,(u_long)strlen(list->host->systemRoot),&nbw);
		out.writeToFile("</SYSFOLDER>\r\n",14,&nbw);

		out.writeToFile("<ACCESSESLOG>",13,&nbw);
		out.writeToFile(list->host->accessesLogFileName,(u_long)strlen(list->host->accessesLogFileName),&nbw);
		out.writeToFile("</ACCESSESLOG>\r\n",16,&nbw);

		out.writeToFile("<WARNINGLOG>",12,&nbw);
		out.writeToFile(list->host->warningsLogFileName,(u_long)strlen(list->host->warningsLogFileName),&nbw);
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
int vhost::initializeSSL()
{
#ifndef DO_NOT_USE_SSL
	if(this->protocol!=PROTOCOL_HTTPS)
		return -2;
	sslContext.method = SSLv23_method();
	sslContext.context = SSL_CTX_new(sslContext.method);
	if(!(SSL_CTX_use_certificate_chain_file(sslContext.context,sslContext.certificateFile)))
		return -1;
	SSL_CTX_set_default_passwd_cb_userdata(sslContext.context, sslContext.password);
	SSL_CTX_set_default_passwd_cb(sslContext.context, password_cb);
	if(!(SSL_CTX_use_PrivateKey_file(sslContext.context,sslContext.privateKeyFile,SSL_FILETYPE_PEM)))
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
void vhost::generateRsaKey()
{
#ifndef DO_NOT_USE_SSL
	RSA *rsa;

	rsa=RSA_generate_key(512,RSA_F4,NULL,NULL);

	if (!SSL_CTX_set_tmp_rsa(sslContext.context,rsa))
		return;

	RSA_free(rsa);
#endif
}
/*!
*Get the SSL context.
*/
SSL_CTX* vhost::getSSLContext()
{
	return sslContext.context;
}
/*!
*Clean the memory used by the SSL context.
*/
int vhost::freeSSL()
{
#ifndef DO_NOT_USE_SSL
	if(sslContext.context)
	{
		SSL_CTX_free(sslContext.context);
		return 1;
	}
	else 
		return 0;
#else
	return 1;
#endif
}
/*!
*Get a virtual host by its position in the list.
*Zero based list.
*/
vhost* vhostmanager::getVHostByNumber(int n)
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
int vhostmanager::removeVHost(int n)
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
