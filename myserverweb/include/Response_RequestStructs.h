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
#ifndef RESPONSE_REQUESTSTRUCTS_H
#define RESPONSE_REQUESTSTRUCTS_H
/*
*Structure to describe an HTTP response
*/
struct HTTP_RESPONSE_HEADER
{
	int httpStatus;
	char VER[10];	
	char SERVER_NAME[32];
	char CONTENTS_TYPE[30];
	char CONNECTION[32];
	char MIMEVER[8];
	char P3P[200];
	char COOKIE[2048];
	char CONTENTS_DIM[8];
	char ERROR_TYPE[20];
	char LOCATION[MAX_PATH];
	char DATE[30];		
	char DATEEXP[30];	

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
	char USER_AGENT[128];
	char COOKIE[2048];
	char CONTENTS_TYPE[30];
	char CONTENTS_DIM[8];
	char DATE[30];		
	char DATEEXP[30];	
	char MODIFIED_SINCE[30];
	char LAST_MODIFIED[30];	
	char URI[1024];
	char PRAGMA[200];
	char URIOPTS[1024];		
	char *URIOPTSPTR;		
	char REFERER[MAX_PATH];	
	char FROM[MAX_PATH];
	char HOST[128];			
	char OTHER[256];
	char RANGETYPE[12];		
	char RANGEBYTEBEGIN[10];
	char RANGEBYTEEND[10];
	int uriEndsWithSlash;
}; 
#endif
