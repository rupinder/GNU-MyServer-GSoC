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

#include "..\include\mime_manager.h"
#include "..\include\filemanager.h"

/*
*Source code to manage the MIME types in myServer.
*MIME types are recorded in a static buffer "data", that is a strings array.
*Every MIME type is described by three strings:
html,text/html,NONE;
*1)its extension(for example HTML)
*2)its MIME description(for example text/html)
*3)if the file type is used by a CGI this is the path to the CGI manager(for example c:/php/php.exe),
*	if the file isn't registered by a CGI then this is "NONE".
*The file is ended by a '#' character
*/
HRESULT MIME_Manager::load(char *filename)
{
	numMimeTypesLoaded=0;
	char buffer[MAX_MIME_TYPES*2*32];
	ZeroMemory(&(data[0][0][0]),sizeof(buffer));
	ZeroMemory(buffer,sizeof(buffer));
	MYSERVER_FILE_HANDLE f=ms_OpenFile(filename,MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_IFEXISTS);
	if(f==0)
		return 1;
	DWORD nbw;
	ms_ReadFromFile(f,buffer,sizeof(buffer),&nbw);
	ms_CloseFile(f);
	DWORD nc=0;
	for(DWORD i=0;i<MAX_MIME_TYPES;i++)
	{
		while(buffer[nc]==' ')
			nc++;
		if(buffer[nc]=='#')
			break;
		while(buffer[nc]!=',')
		{
			if((buffer[nc]!='\n')&&(buffer[nc]!='\r')&&(buffer[nc]!=' '))
				data[i][0][lstrlen(data[i][0])]=buffer[nc];
			nc++;
		}
		nc++;
		while(buffer[nc]!=',')
		{
			if((buffer[nc]!='\n')&&(buffer[nc]!='\r')&&(buffer[nc]!=' '))
				data[i][1][lstrlen(data[i][1])]=buffer[nc];
			nc++;
		}
		nc++;
		while(buffer[nc]!=';')
		{
			if((buffer[nc]!='\n')&&(buffer[nc]!='\r'))
				data[i][2][lstrlen(data[i][2])]=buffer[nc];
			nc++;
			if(!lstrcmpi(data[i][2],"NONE"))
				data[i][2][0]='\0';
			
		}
		numMimeTypesLoaded++;
		nc++;
	}
	return 0;

}
/*
*This function returns true if the passed ext is registered by a CGI.
*Passing a file extension ext this function fills the strings dest and dest2
*respectly with the MIME type description and if there are the path to the CGI manager.
*/
BOOL MIME_Manager::getMIME(char* ext,char *dest,char *dest2)
{
	DWORD i;
	for(i=0;i<numMimeTypesLoaded;i++)
	{
		if(!lstrcmpi(ext,data[i][0]	))
		{
			lstrcpy(dest,data[i][1]);
			if(data[i][2][0])
			{
				if(dest2)
					lstrcpy(dest2,data[i][2]);
				return TRUE;
			}
			else
			{
				return FALSE;
			}
		}
	}
	return FALSE;
}
/*
*Dump the content of the data buffer to a file
*/
VOID MIME_Manager::dumpToFILE(char *file)
{
	MYSERVER_FILE_HANDLE f=ms_OpenFile(file,MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_OPEN_ALWAYS);
	DWORD nbw;
	ms_WriteToFile(f,&data[0][0][0],sizeof(data),&nbw);
	ms_CloseFile(f);
}

VOID MIME_Manager::clean()
{

}
/*
*Returns the number of MIME types loaded
*/
DWORD MIME_Manager::getNumMIMELoaded()
{
	return numMimeTypesLoaded;
}
