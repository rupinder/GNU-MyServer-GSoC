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

#include "..\stdafx.h"
#include "..\include\utility.h"
#include <string.h>
extern BOOL mustEndServer; 
static MYSERVER_FILE_HANDLE warningsLogFile=0;
static MYSERVER_FILE_HANDLE accessesLogFile=0;

/*
*Functions to manage the log files.
*/
DWORD warningsLogWrite(char* str)
{
	DWORD nbw;
	ms_WriteToFile(warningsLogFile,str,lstrlen(str),&nbw);
	return nbw;
}
void setWarningsLogFile(MYSERVER_FILE_HANDLE nlg)
{
	warningsLogFile=nlg;
}
DWORD accessesLogWrite(char* str)
{
	DWORD nbw;
	ms_WriteToFile(accessesLogFile,str,lstrlen(str),&nbw);
	return nbw;
}
void setAccessesLogFile(MYSERVER_FILE_HANDLE nlg)
{
	accessesLogFile=nlg;
}


/*
*Return the recursion of the path.
*/
int getPathRecursionLevel(char* path)
{
	static char lpath[MAX_PATH];
	lstrcpy(lpath,path);
	int rec=0;
	char *token = strtok( lpath, "\\/" );
	do
	{
		if(lstrcmpi(token,".."))
			rec++;
		else
			rec--;
		token = strtok( NULL, "\\/" );
	}
	while( token != NULL );
	return rec;
}
/*
*Write data to a file.
*/
INT	ms_WriteToFile(MYSERVER_FILE_HANDLE f,char* buffer,DWORD buffersize,DWORD* nbw)
{
#ifdef WIN32
	return WriteFile((HANDLE)f,buffer,buffersize,nbw,NULL);
#endif
}
/*
*Read data from a file to a buffer.
*/
INT	ms_ReadFromFile(MYSERVER_FILE_HANDLE f,char* buffer,DWORD buffersize,DWORD* nbr)
{
#ifdef WIN32
	ReadFile((HANDLE)f,buffer,buffersize,nbr,NULL);
	/*
	*Return 1 if we don't have problems with the buffersize.
	*/
	return (*nbr<=buffersize)? 1 : 0 ;
#endif
}

/*
*Open(or create if not exists) a file.
*/
MYSERVER_FILE_HANDLE ms_OpenFile(char* filename,DWORD opt)
{
	MYSERVER_FILE_HANDLE ret;
#ifdef WIN32
	SECURITY_ATTRIBUTES sa = {0};  
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;
	DWORD creationFlag=0;
	DWORD openFlag=0;
	DWORD attributeFlag=0;

	if(opt & MYSERVER_FILE_OPEN_ALWAYS)
		creationFlag|=OPEN_ALWAYS;
	if(opt & MYSERVER_FILE_OPEN_IFEXISTS)
		creationFlag|=OPEN_EXISTING;

	if(opt & MYSERVER_FILE_OPEN_READ)
		openFlag|=GENERIC_READ;
	if(opt & MYSERVER_FILE_OPEN_WRITE)
		openFlag|=GENERIC_WRITE;

	if(opt & MYSERVER_FILE_OPEN_TEMPORARY)
		openFlag|=FILE_ATTRIBUTE_TEMPORARY;
	if(opt & MYSERVER_FILE_OPEN_HIDDEN)
		openFlag|=FILE_ATTRIBUTE_HIDDEN;

	ret=(MYSERVER_FILE_HANDLE)CreateFile(filename, openFlag,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,creationFlag,attributeFlag, NULL);
	if(ret==INVALID_HANDLE_VALUE)
		ret=0;

	if(ret && (opt & MYSERVER_FILE_OPEN_APPEND))
		setFilePointer((MYSERVER_FILE_HANDLE)ret,getFileSize((MYSERVER_FILE_HANDLE)ret));
	else
		setFilePointer((MYSERVER_FILE_HANDLE)ret,0);

#endif
	return ret;
}
/*
*Create a temporary file.
*/
MYSERVER_FILE_HANDLE ms_CreateTemporaryFile(char* filename)
{
#ifdef WIN32
	return ms_OpenFile(filename,MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_OPEN_TEMPORARY|MYSERVER_FILE_OPEN_HIDDEN|MYSERVER_FILE_OPEN_ALWAYS);
#endif
}
/*
*Close an open file handle.
*/
INT ms_CloseFile(MYSERVER_FILE_HANDLE fh)
{
#ifdef WIN32
	CloseHandle((HANDLE)fh);
#endif
	return 0;
}
/*
*Delete an existing file passing its path.
*/
INT ms_DeleteFile(char *filename)
{
#ifdef WIN32
	DeleteFile(filename);
#endif
	return 0;
}
/*
*Returns the file size in bytes.
*/
DWORD getFileSize(MYSERVER_FILE_HANDLE f)
{
	DWORD size;
#ifdef WIN32
	size=GetFileSize((HANDLE)f,NULL);
#endif
	return size;
}
/*
*Change the position of the pointer to the file.
*Returns a non-null value if failed.
*/
BOOL setFilePointer(MYSERVER_FILE_HANDLE h,DWORD initialByte)
{
#ifdef WIN32
	return (SetFilePointer((HANDLE)h,initialByte,0,FILE_BEGIN)==INVALID_SET_FILE_POINTER)?1:0;
#endif
}
/*
*Returns a non-null value if the handle is a folder.
*/
INT	ms_IsFolder(char *filename)
{
#ifdef WIN32
	return(GetFileAttributes(filename)&FILE_ATTRIBUTE_DIRECTORY)?1:0;
#endif
}

/*
*Returns a non-null value if the given path is a valid file.
*/
INT ms_FileExists(char* filename)
{
#ifdef WIN32
	MYSERVER_FILE_HANDLE fh=ms_OpenFile(filename,MYSERVER_FILE_OPEN_IFEXISTS|MYSERVER_FILE_OPEN_READ);
	if(fh)
	{
		ms_CloseFile(fh);
		return 1;
	}
	else
		return 0;
#endif
}