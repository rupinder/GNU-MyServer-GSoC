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
#pragma once
#ifndef HTTP_RESPONSE_REQUEST_HEADER_IN
#define HTTP_RESPONSE_REQUEST_HEADER_IN
/*
*Structure to describe an HTTP response
*/
struct HTTP_RESPONSE_HEADER
{
	char VER[10];	
	char SERVER_NAME[16];
	char MIME[12];		
	char CONTENTS_DIM[8];
	char ERROR_TYPE[20];
	char LOCATION[MAX_PATH];
	char DATE[30];		
	char DATEEXP[30];	
	int isError;		
	char OTHER[256];	

};

/*
*Structure to describe an HTTP request.
*/
struct HTTP_REQUEST_HEADER
{
	char CMD[16];		
	char VER[10];		
	char ACCEPT[128];
	char AUTH[32];
	char ACCEPTENC[64];	
	char ACCEPTLAN[64];	
	char ACCEPTCHARSET[64];
	char CONNECTION[32];
	char USER_AGENT[192];
	char CONTENTS_TYPE[12];
	char CONTENTS_DIM[8];
	char DATE[30];		
	char DATEEXP[30];	
	char MODIFIED_SINCE[30];
	char LAST_MODIFIED[30];	
	char URI[1024];			
	char URIOPTS[1024];		
	char *URIOPTSPTR;		
	char REFERER[MAX_PATH];	
	char HOST[128];			
	char OTHER[256];
	char RANGETYPE[12];		
	char RANGEBYTEBEGIN[10];
	char RANGEBYTEEND[10];	
}; 
#endif