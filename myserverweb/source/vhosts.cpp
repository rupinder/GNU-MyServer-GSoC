/*
*myServer
*Copyright (C) 2002 Giuseppe Scrivano
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
#include "../include/utility.h"
#include "../include/cserver.h"
#include "../include/connectionstruct.h"/*Used for protocols IDs*/
#include "../include/stringutils.h"


/*
*vhost
*/
vhost::vhost()
{
	ipList=0;
	hostList=0;
}
vhost::~vhost()
{
	clearHostList();
	clearIPList();
	if(accessesLogFile.ms_GetHandle())
		accessesLogFile.ms_CloseFile();
	if(warningsLogFile.ms_GetHandle())
		warningsLogFile.ms_CloseFile();
	memset(this,0,sizeof(vhost));
}
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
void vhost::addIP(char *ip)
{
	sIpList* il=new sIpList();
	strcpy(il->hostIp,ip);
	if(ipList)
	{
		il->next=ipList;
	}
	else
	{
		il->next=0;
	}
	ipList=il;
}

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

void vhost::addHost(char *host)
{
	sHostList* hl=new sHostList();
	strcpy(hl->hostName,host);
	if(hostList)
	{
		hl->next=hostList;
	}
	else
	{
		hl->next=0;
	}
	hostList=hl;
}

u_long vhost::ms_accessesLogWrite(char* str)
{
	u_long nbw;
	accessesLogFile.ms_WriteToFile(str,lstrlen(str),&nbw);
	return nbw;
}
MYSERVER_FILE* vhost::ms_getAccessesLogFile()
{
	return &accessesLogFile;
}

u_long vhost::ms_warningsLogWrite(char* str)
{
	u_long nbw;
	warningsLogFile.ms_WriteToFile(str,lstrlen(str),&nbw);
	return nbw;
}
MYSERVER_FILE* vhost::ms_getWarningsLogFile()
{
	return &warningsLogFile;
}


/*
*vhostmanager
*/
void vhostmanager::addvHost(vhost* vHost)
{
	if(vhostList==0)
	{
		vhostList=new sVhostList();	
		vhostList->host=vHost;
		vhostList->next=0;
	}
	else
	{
		sVhostList* hostl=vhostList;
		for(;;)/*Append the new host to the end of the linked list*/
		{
			if(hostl->next)
				hostl=hostl->next;
			else
				break;
		}
		hostl->next=new sVhostList();	

		hostl->next->next=0;/*Make sure that next is null*/
		hostl->next->host=vHost;
	}
	
}
vhost* vhostmanager::getvHost(char* host,char* ip,u_short port)
{
	sVhostList* vhl;
	for(vhl=vhostList;vhl;vhl=vhl->next)
	{
		if(vhl->host->port!=port)/*control if the host port is the correct one*/
			continue;
		if(ip && !vhl->host->isIPAllowed(ip)) /*If ip is defined check that it is allowed to connect to the host*/
			continue;
		if(host && !vhl->host->isHostAllowed(host))/*If host is defined check that it is allowed to connect to the host*/
			continue;
		return vhl->host;/*we find a valid host*/
	}
	return 0;
}

vhostmanager::vhostmanager()
{
	vhostList=0;
}
void vhostmanager::clean()
{
	sVhostList* shl=vhostList;
	sVhostList* prevshl=0;
	while(shl)
	{
		if(prevshl)
			delete prevshl;
		prevshl=shl;
		shl=shl->next;
	}
	if(prevshl)
		delete prevshl;
	memset(this,0,sizeof(vhostmanager));
}
vhostmanager::~vhostmanager()
{
	clean();
}


void vhostmanager::loadConfigurationFile(char* filename)
{
	/*
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

	char buffer[KB(10)];/*Exists a line greater than 10 KB?!?*/
	char buffer2[256];
	u_long nbr;/*Number of bytes read from the file*/
	MYSERVER_FILE fh;
	int ret=fh.ms_OpenFile(filename,MYSERVER_FILE_OPEN_IFEXISTS|MYSERVER_FILE_OPEN_READ);
	if((ret==0)||(ret==-1))/*If the file cannot be opened simply do nothing*/
		return;
	char c;
	for(;;)
	{
		buffer[0]='\0';
		for(;;)/*Save a line in the buffer. A line ends with a diesis.*/
		{
			fh.ms_ReadFromFile(&c,1,&nbr);
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
						oldc =c;
						fh.ms_ReadFromFile(&c,1,&nbr);
					}while(c!='/' && oldc!='*');
					buffer[0]='\0';
					continue;
				}
			}
			else
				break;
		}
		if(buffer[0]=='\0')/*If the buffer is only a point, we reached the file end*/
			break;		
		vhost *vh=new vhost();
		/*Parse the line*/

		int cc=0;
		for(;;)/*Get the hosts list*/
		{
			buffer2[0]='\0';
			while((buffer[cc]!=',')&&(buffer[cc]!=';'))
			{
				buffer2[strlen(buffer2)+1]='\0';
				buffer2[strlen(buffer2)]=buffer[cc];
				cc++;
			}
			if(buffer2[0]&&buffer2[0]!='0')/*If the host list is equal to 0 don't add anything to the list*/
				vh->addHost(buffer2);
			if(buffer[cc]==';')
				break;
			cc++;
		}
		cc++;
		for(;;)/*Get the ip list*/
		{
			buffer2[0]='\0';

			while((buffer[cc]!=',')&&(buffer[cc]!=';'))
			{
				buffer2[strlen(buffer2)+1]='\0';
				buffer2[strlen(buffer2)]=buffer[cc];
				cc++;
			}
			if(buffer2[0]&&buffer2[0]!='0')/*If the ip list is equal to 0 don't add anything to the list*/
				vh->addIP(buffer2);
			if(buffer[cc]==';')
				break;
			cc++;
		}
		cc++;
		/*Get the port used by the virtual host*/
		buffer2[0]='\0';
		while(buffer[cc]!=';')
		{
			buffer2[strlen(buffer2)+1]='\0';
			buffer2[strlen(buffer2)]=buffer[cc];
			cc++;
		}
		vh->port=(u_short)atoi(buffer2);
		cc++;		
		/*Get the protocol used by the virtual host*/
		buffer2[0]='\0';
		while(buffer[cc]!=';')
		{
			buffer2[strlen(buffer2)+1]='\0';
			buffer2[strlen(buffer2)]=buffer[cc];
			cc++;
		}
		if(!strcmp(buffer2,"HTTP"))
			vh->protocol=PROTOCOL_HTTP;
		if(!strcmp(buffer2,"FTP"))
			vh->protocol=PROTOCOL_FTP;
		cc++;
		/*Get the document root used by the virtual host*/
		buffer2[0]='\0';
		while(buffer[cc]!=';')
		{
			buffer2[strlen(buffer2)+1]='\0';
			buffer2[strlen(buffer2)]=buffer[cc];
			cc++;
		}	
		strcpy(vh->documentRootOriginal,buffer2);
		if(buffer2[0]!='|')
			sprintf(vh->documentRoot,"%s/%s",lserver->getPath(),buffer2);
		else
			strcpy(vh->documentRoot,&buffer2[1]);
		cc++;
		/*Get the system folder used by the virtual host*/
		buffer2[0]='\0';
		while(buffer[cc]!=';')
		{
			buffer2[strlen(buffer2)+1]='\0';
			buffer2[strlen(buffer2)]=buffer[cc];
			cc++;
		}	
		strcpy(vh->systemRootOriginal,buffer2);
		if(buffer2[0]!='|')
			sprintf(vh->systemRoot,"%s/%s",lserver->getPath(),buffer2);
		else
			strcpy(vh->systemRoot,&buffer2[1]);
		cc++;
		/*Get the accesses log file used by the virtual host*/
		buffer2[0]='\0';
		while(buffer[cc]!=';')
		{
			buffer2[strlen(buffer2)+1]='\0';
			buffer2[strlen(buffer2)]=buffer[cc];
			cc++;
		}	
		strcpy(vh->accessesLogFileName,buffer2);
		MYSERVER_FILE *accesses=vh->ms_getAccessesLogFile();
		accesses->ms_OpenFile(buffer2,MYSERVER_FILE_OPEN_ALWAYS|MYSERVER_FILE_OPEN_WRITE);
		cc++;
		/*Get the warnings log file used by the virtual host*/
		buffer2[0]='\0';
		while(buffer[cc]!=';' && buffer[cc])
		{
			buffer2[strlen(buffer2)+1]='\0';
			buffer2[strlen(buffer2)]=buffer[cc];
			cc++;
		}	
		strcpy(vh->warningsLogFileName,buffer2);
		MYSERVER_FILE * warnings=vh->ms_getWarningsLogFile();
		warnings->ms_OpenFile(buffer2,MYSERVER_FILE_OPEN_ALWAYS|MYSERVER_FILE_OPEN_WRITE);
		cc++;
		addvHost(vh);
	}
	fh.ms_CloseFile();
}
void vhostmanager::saveConfigurationFile(char *filename)
{
	char buffer[1024];
	if(vhostList==0)
		return;
	sVhostList*vhl=vhostList;
	u_long nbw;
	MYSERVER_FILE fh;
	fh.ms_OpenFile(filename,MYSERVER_FILE_CREATE_ALWAYS|MYSERVER_FILE_OPEN_WRITE);
	for(;vhl;vhl=vhl->next)
	{
		vhost*vh=vhl->host;
		vhost::sHostList* hl=vh->hostList;
		if(hl)
		{
			while(hl)
			{ 
				fh.ms_WriteToFile(hl->hostName,(u_long)strlen(hl->hostName),&nbw);
				strcpy(buffer,",");
				fh.ms_WriteToFile(buffer,(u_long)strlen(buffer),&nbw);
				if(hl->next)
					hl=hl->next;
			}
		}
		else
		{
			strcpy(buffer,"0");
			fh.ms_WriteToFile(buffer,(u_long)strlen(buffer),&nbw);
		}
		strcpy(buffer,";");
		fh.ms_WriteToFile(buffer,(u_long)strlen(buffer),&nbw);
		vhost::sIpList* il=vh->ipList;
		if(il)
		{
			while(il)
			{ 
				fh.ms_WriteToFile(il->hostIp,(u_long)strlen(il->hostIp),&nbw);
				if(il->next)
				{
					strcpy(buffer,",");
					fh.ms_WriteToFile(buffer,(u_long)strlen(buffer),&nbw);
				}

				il=il->next;
			}
		}
		else
		{
			strcpy(buffer,"0");
			fh.ms_WriteToFile(buffer,(u_long)strlen(buffer),&nbw);
		}
		sprintf(buffer,";%u;",vh->port);
		fh.ms_WriteToFile(buffer,(u_long)strlen(buffer),&nbw);

		if(vh->protocol==PROTOCOL_HTTP)
		{
			strcpy(buffer,"HTTP;");
			fh.ms_WriteToFile(buffer,(u_long)strlen(buffer),&nbw);
		}
		if(vh->protocol==PROTOCOL_FTP)
		{
			strcpy(buffer,"FTP;");
			fh.ms_WriteToFile(buffer,(u_long)strlen(buffer),&nbw);
		}

		fh.ms_WriteToFile(vh->documentRootOriginal,(u_long)strlen(vh->documentRootOriginal),&nbw);
		strcpy(buffer,";");
		fh.ms_WriteToFile(buffer,(u_long)strlen(buffer),&nbw);

		fh.ms_WriteToFile(vh->systemRootOriginal,(u_long)strlen(vh->systemRootOriginal),&nbw);
		strcpy(buffer,";");
		fh.ms_WriteToFile(buffer,(u_long)strlen(buffer),&nbw);

		fh.ms_WriteToFile(vh->accessesLogFileName,(u_long)strlen(vh->accessesLogFileName),&nbw);
		strcpy(buffer,";");
		fh.ms_WriteToFile(buffer,(u_long)strlen(buffer),&nbw);

		fh.ms_WriteToFile(vh->warningsLogFileName,(u_long)strlen(vh->warningsLogFileName),&nbw);
		if(vhl->next)
			strcpy(buffer,";#\r\n");
		else
			strcpy(buffer,";##\r\n\0");
		fh.ms_WriteToFile(buffer,(u_long)strlen(buffer),&nbw);
	}
	fh.ms_CloseFile();
}

vhostmanager::sVhostList* vhostmanager::getvHostList()
{
	return this->vhostList;
}

