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
#include "..\include\stringutils.h"
extern int mustEndServer; 
static MYSERVER_FILE_HANDLE warningsLogFile=0;
static MYSERVER_FILE_HANDLE accessesLogFile=0;

/*
*Write to the log file.
*/
u_long warningsLogWrite(char* str)
{
	u_long nbw;
	ms_WriteToFile(warningsLogFile,str,lstrlen(str),&nbw);
	return nbw;
}
/*
*Set the log file handle.
*/
void setWarningsLogFile(MYSERVER_FILE_HANDLE nlg)
{
	warningsLogFile=nlg;
}
/*
*Write to the accesses file.
*/
u_long accessesLogWrite(char* str)
{
	u_long nbw;
	ms_WriteToFile(accessesLogFile,str,lstrlen(str),&nbw);
	return nbw;
}
/*
*Set the accesses file handle.
*/
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
INT	ms_WriteToFile(MYSERVER_FILE_HANDLE f,char* buffer,u_long buffersize,u_long* nbw)
{
#ifdef WIN32
	return WriteFile((HANDLE)f,buffer,buffersize,nbw,NULL);
#endif
}
/*
*Open(or create if not exists) a file.
*If the function have success the return value is different from 0 and -1.
*/
MYSERVER_FILE_HANDLE ms_OpenFile(char* filename,u_long opt)
{
	MYSERVER_FILE_HANDLE ret=0;
#ifdef WIN32
	SECURITY_ATTRIBUTES sa = {0};  
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;
	u_long creationFlag=0;
	u_long openFlag=0;
	u_long attributeFlag=0;

	if(opt & MYSERVER_FILE_OPEN_ALWAYS)
		creationFlag|=OPEN_ALWAYS;
	if(opt & MYSERVER_FILE_OPEN_IFEXISTS)
		creationFlag|=OPEN_EXISTING;
	if(opt & MYSERVER_FILE_CREATE_ALWAYS)
		creationFlag|=CREATE_ALWAYS;

	if(opt & MYSERVER_FILE_OPEN_READ)
		openFlag|=GENERIC_READ;
	if(opt & MYSERVER_FILE_OPEN_WRITE)
		openFlag|=GENERIC_WRITE;

	if(opt & MYSERVER_FILE_OPEN_TEMPORARY)
	{
		openFlag|=FILE_ATTRIBUTE_TEMPORARY;
		attributeFlag|=FILE_FLAG_DELETE_ON_CLOSE;
	}

	if(opt & MYSERVER_FILE_OPEN_HIDDEN)
		openFlag|=FILE_ATTRIBUTE_HIDDEN;

	ret=(MYSERVER_FILE_HANDLE)CreateFile(filename, openFlag,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,creationFlag,attributeFlag, NULL);
	if(ret==INVALID_HANDLE_VALUE)/*If exists an error*/
	{
		if(GetLastError()==ERROR_ACCESS_DENIED)/*returns -1 if the file is not accessible*/
			return (MYSERVER_FILE_HANDLE)-1;
		else
			ret=0;/*returns 0 for any other error*/
	}
	else/*If no error exist in open the file*/
	{
		if(ret && (opt & MYSERVER_FILE_OPEN_APPEND))
			setFilePointer((MYSERVER_FILE_HANDLE)ret,getFileSize((MYSERVER_FILE_HANDLE)ret));
		else
			setFilePointer((MYSERVER_FILE_HANDLE)ret,0);
	}

#endif
	return ret;
}
/*
*Read data from a file to a buffer.
*/
INT	ms_ReadFromFile(MYSERVER_FILE_HANDLE f,char* buffer,u_long buffersize,u_long* nbr)
{
#ifdef WIN32
	ReadFile((HANDLE)f,buffer,buffersize,nbr,NULL);
	/*
	*Return 1 if we don't reach the ond of the file.
	*Return 0 if the end of the file is reached.
	*/
	return (*nbr<=buffersize)? 1 : 0 ;
#endif
}
/*
*Create a temporary file.
*/
MYSERVER_FILE_HANDLE ms_CreateTemporaryFile(char* filename)
{
#ifdef WIN32
	return ms_OpenFile(filename,MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_OPEN_HIDDEN|MYSERVER_FILE_OPEN_TEMPORARY|MYSERVER_FILE_CREATE_ALWAYS);
#endif
}
/*
*Close an open file handle.
*/
INT ms_CloseFile(MYSERVER_FILE_HANDLE fh)
{
#ifdef WIN32
	FlushFileBuffers((HANDLE)fh);
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
u_long getFileSize(MYSERVER_FILE_HANDLE f)
{
	u_long size;
#ifdef WIN32
	size=GetFileSize((HANDLE)f,NULL);
#endif
	return size;
}
/*
*Change the position of the pointer to the file.
*Returns a non-null value if failed.
*/
int setFilePointer(MYSERVER_FILE_HANDLE h,u_long initialByte)
{
#ifdef WIN32
	return (SetFilePointer((HANDLE)h,initialByte,NULL,FILE_BEGIN)==INVALID_SET_FILE_POINTER)?1:0;
#endif
}
/*
*Returns a non-null value if the path is a folder.
*/
INT	ms_IsFolder(char *filename)
{
#ifdef WIN32
	u_long fa=GetFileAttributes(filename);
	if(fa!=INVALID_FILE_ATTRIBUTES)
		return(fa & FILE_ATTRIBUTE_DIRECTORY)?1:0;
	else
		return 0;
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