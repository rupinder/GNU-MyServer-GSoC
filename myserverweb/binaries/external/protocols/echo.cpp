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

/*
*Include the needed MyServer files to compile the new protocol.
*/
#include "../../../include/connectionstruct.h"
#include "../../../source/sockets.cpp"
#include "../../../source/utility.cpp"

#ifdef WIN32
#define EXPORTABLE _declspec(dllexport)
#endif

#ifdef WIN32
int EXPORTABLE loadProtocol(void* languageParser,char* confFile,void* lserver)
#else
extern "C" int loadProtocol(void* languageParser,char* confFile,void* lserver)
#endif
{
	/*Do nothing for load this protocol*/
	return 1;
	
}
#ifdef WIN32
int EXPORTABLE unloadProtocol(void* languageParser)
#else
extern "C" int unloadProtocol(void* languageParser)
#endif
{
	/*Do nothing for unload*/
	return 1;
}

#ifdef WIN32
char * EXPORTABLE registerName(char* out,int len)
#else
extern "C" char * registerName(char* out,int len)
#endif
{
	if(out)
	{
		strncpy(out,"ECHO",len);
		return out;
	}
	else
		return "ECHO";
	
}

#ifdef WIN32
int EXPORTABLE controlConnection(void * a,char* b1,char* b2,int bs1,int bs2,u_long nbtr,u_long id)
#else
extern "C" int controlConnection(void * a,char* b1,char* b2,int bs1,int bs2,u_long nbtr,u_long id)
#endif
{
	LPCONNECTION connection=(LPCONNECTION)a;
	strcpy(b1,"HTTP/1.1 200 OK\r\nConnection: Closed\r\nContent-type: text/html\r\n\r\nHello World!!!");
	connection->socket.send(b1,(int)strlen(b1),0);	
	/*!
	*This function MUST return:
	*0 to delete the connection from the active connections list
	*1 to keep the connection active and clear the connectionBuffer
	*2 if the header is incomplete and to save it in a temporary buffer
	*3 if the header is incomplete without save it in a temporary buffer
	*/
	return 0;

}



#ifdef WIN32
BOOL APIENTRY DllMain( HANDLE,DWORD ul_reason_for_call,LPVOID)
{
	switch (ul_reason_for_call)	
	{
		case DLL_PROCESS_ATTACH:	
		case DLL_THREAD_ATTACH:	
		case DLL_THREAD_DETACH:	
		case DLL_PROCESS_DETACH:
		break;	
	}   
	return TRUE;
}
#endif


/*
*To configure MyServer to use this protocol add the following lines in your virtual hosts
*configuration file:
*<VHOST>
*<NAME>HTTP ECHO TEST</NAME>
*<PORT>1111</PORT>
*<PROTOCOL>ECHO</PROTOCOL>
*<DOCROOT>web</DOCROOT>
*<SYSFOLDER>system</SYSFOLDER>
*<ACCESSESLOG>logs/myServerECHO.log</ACCESSESLOG>
*<WARNINGLOG>logs/myServerECHO.err</WARNINGLOG>
*</VHOST>
*/
