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
#ifndef VHOST_IN
#define VHOST_IN

#include "../stdafx.h"
#include "../include/filemanager.h"
class vhost
{
	MYSERVER_FILE_HANDLE warningsLogFile;
	MYSERVER_FILE_HANDLE accessesLogFile;
public:
	struct sHostList
	{
		char hostName[MAX_COMPUTERNAME_LENGTH+1];
		sHostList *next;
	}*hostList;/*List of hosts allowed by the vhost*/

	struct sIpList
	{
		char hostIp[32];
		sIpList *next; 
	}*ipList;/*List of IPs allowed by the vhost*/

	u_short port;/*Port to listen on*/

	char documentRoot[MAX_PATH];/*Path to the document root*/
	vhost();
	void addIP(char *);
	void addHost(char *);
	void clearIPList();
	void clearHostList();
	~vhost();
	/*
	*Functions to manage the logs file.
	*Derived directly from the filemanager utilities.
	*/
	u_long ms_accessesLogWrite(char*);
	void ms_setAccessesLogFile(MYSERVER_FILE_HANDLE);

	u_long ms_warningsLogWrite(char*);
	void ms_setWarningsLogFile(MYSERVER_FILE_HANDLE);
};


class vhostmanager
{
private:
	struct sVhostList
	{
		vhost* host;
		sVhostList* next;
	}*vhostList;/*List of virtual hosts*/
public:
	vhostmanager();
	~vhostmanager();
	void getvHost(vhost* ,char*,char*,u_short);/*Get a pointer to a vhost*/
	void addvHost(vhost*);/*Add an element to the vhost list*/
	void loadConfigurationFile(char *);/*Load the virtual hosts list from a configuration file*/
};

#endif