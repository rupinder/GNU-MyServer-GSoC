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
#include "..\stdafx.h"
#include "..\include\http.h"

extern int err;

class  ClientsTHREAD
{
	friend class cserver;
	friend  unsigned int WINAPI startClientsTHREAD(void* pParam);
	friend LRESULT CALLBACK MainWndProc(HWND hwnd,UINT uMsg,WPARAM 
wParam,LPARAM lParam);
private:
	BOOL initialized;
	HANDLE hImpersonation;
	DWORD id;
	int err;
	BOOL threadIsRunning;
	DWORD nConnections;
	DWORD buffersize;
	DWORD buffersize2;
	LPCONNECTION addConnection(SOCKET,CONNECTION_PROTOCOL=PROTOCOL_HTTP);
	LPCONNECTION findConnection(SOCKET);
	char *buffer;
	char *buffer2;
	void clearAllConnections();
	BOOL deleteConnection(LPCONNECTION);
	HANDLE threadHandle;
	void controlConnections();
	HANDLE connectionMutex;
	LPCONNECTION connections;
	DWORD nBytesToRead;
public:
	ClientsTHREAD();
	~ClientsTHREAD();
	void stop();
	void clean();	
};
unsigned int WINAPI startClientsTHREAD(void* pParam); 

