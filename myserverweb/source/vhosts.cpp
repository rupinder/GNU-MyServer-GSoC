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

vhost::vhost()
{
	ipList=0;
	hostList=0;
}
vhost::~vhost()
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

}

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