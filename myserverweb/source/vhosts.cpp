/*
*myServer
*Copyright (C) Rocky_10_Balboa
*This library is free software; you can redistribute it and/or
*modify it under the terms of the GNU Library General Public
*License as published by the Free Software Foundation; either
*version 2 of the License, or (at your option) any later version.

*This library is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*Library General Public License for more details.

*You should have received a copy of the GNU Library General Public
*License along with this library; if not, write to the
*Free Software Foundation, Inc., 59 Temple Place - Suite 330,
*Boston, MA  02111-1307, USA.
*/
#include "../include/vhosts.h"

#ifndef WIN32
extern "C" {
#include <string.h>
}

#define lstrcmpi strcmp
#define lstrcpy strcpy
#define lstrcat strcat
#define lstrlen strlen
#endif

/*
*vhost
*/
vhost::vhost()
{
	ipList=0;
	hostList=0;
	warningsLogFile=0;
	accessesLogFile=0;
}
vhost::~vhost()
{
	clearHostList();
	clearIPList();
	if(accessesLogFile)
		ms_CloseFile(accessesLogFile);
	if(warningsLogFile)
		ms_CloseFile(warningsLogFile);
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
	ms_WriteToFile(accessesLogFile,str,lstrlen(str),&nbw);
	return nbw;
}
void vhost::ms_setAccessesLogFile(MYSERVER_FILE_HANDLE nlg)
{
	accessesLogFile=nlg;
}

u_long vhost::ms_warningsLogWrite(char* str)
{
	u_long nbw;
	ms_WriteToFile(warningsLogFile,str,lstrlen(str),&nbw);
	return nbw;
}
void vhost::ms_setWarningsLogFile(MYSERVER_FILE_HANDLE nlg)
{
	warningsLogFile=nlg;
}


/*
*vhostmanager
*/

void vhostmanager::addvHost(vhost* vHost)
{
	sVhostList* hostl=vhostList;
	if(hostl==0)
	{
		hostl=new sVhostList();	
		hostl->host=vHost;
		hostl->next=0;
	}
	else
	{
		while(hostl->next)
		{
			hostl->next;
		}
		hostl=new sVhostList();	
		hostl->next->next=0;/*Make sure that next is null*/
		hostl->next->host=vHost;
	}
	
}
void vhostmanager::getvHost(vhost* out,char*,char*,u_short)
{

}

vhostmanager::vhostmanager()
{

}
vhostmanager::~vhostmanager()
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
}


void vhostmanager::loadConfigurationFile(char* /*filename*/)
{
	/*
	*JUST AN IDEA OF WHAT THE FILE WOULD BE
	*
	FILE STRUCTURE:
	hosts;IPs;port;protocol;documentRoot;systemFolder;accessesLog;warningLog.
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
	QUESTIONS:
	1)How determine the IP that an incoming socket is connected to.
	2)Compare two IPs(IPv6 safe)?
	*/
}
