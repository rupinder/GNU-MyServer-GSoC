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


#include "../stdafx.h"
#include "../include/utility.h"
#include "../include/stringutils.h"
extern int mustEndServer; 
static MYSERVER_FILE_HANDLE warningsLogFile=0;
static MYSERVER_FILE_HANDLE accessesLogFile=0;

#ifndef WIN32
extern "C" {
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
}
#endif

/*
*Return the recursion of the path.
*/
int MYSERVER_FILE::getPathRecursionLevel(char* path)
{
	static char lpath[MAX_PATH];
	strcpy(lpath,path);
	int rec=0;
	char *token = strtok( lpath, "\\/" );
	do
	{
		if(token != NULL) 
		{
			if( strcmp(token,"..") && strcmp(token,".") )
				rec++;
			else
				rec--;
		}
		token = strtok( NULL, "\\/" );
	}
	while( token != NULL );
	return rec;
}
/*
*Costructor of the class.
*/
MYSERVER_FILE::MYSERVER_FILE()
{
	filename[0]='\0';
	handle=0;
}
/*
*Write data to a file.
*buffer is the pointer to the data to write
*buffersize is the number of byte to write
*nbw is a pointer to an unsigned long that receive the number of the
*bytes written correctly.
*/
int MYSERVER_FILE::writeToFile(char* buffer,u_long buffersize,u_long* nbw)
{
	if(buffersize==0)
	{
		*nbw=0;
		return -1;
	}
#ifdef WIN32
	return WriteFile((HANDLE)handle,buffer,buffersize,nbw,NULL);
#endif
#ifdef __linux__
	*nbw = write((int)handle, buffer, buffersize);
	return 0;
#endif
}
/*
*Open(or create if not exists) a file.
*If the function have success the return value is different from 0 and -1.
*filename is the name of the file to open
*opt is a bit-field containing the options on how open it
*/
int MYSERVER_FILE::openFile(char* filename,u_long opt)
{
	strcpy(MYSERVER_FILE::filename,filename);
#ifdef WIN32
	SECURITY_ATTRIBUTES sa = {0};  
    sa.nLength = sizeof(sa);
	if(opt & MYSERVER_FILE_NO_INHERIT)
		sa.bInheritHandle = FALSE;
	else
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

	if(attributeFlag == 0)
		attributeFlag = FILE_ATTRIBUTE_NORMAL;
	handle=(MYSERVER_FILE_HANDLE)CreateFile(filename, openFlag,FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,creationFlag,attributeFlag, NULL);
	if(handle==INVALID_HANDLE_VALUE)/*If exists an error*/
	{
		if(GetLastError()==ERROR_ACCESS_DENIED)/*returns -1 if the file is not accessible*/
			return -1;
		else
			return 0;/*returns 0 for any other error*/
	}
	else/*If no error exist in open the file*/
	{
		if(opt & MYSERVER_FILE_OPEN_APPEND)
			setFilePointer(getFileSize());
		else
			setFilePointer(0);
	}

#endif
#ifdef __linux__
	struct stat F_Stats;
	int F_Flags;
	
	if(opt && MYSERVER_FILE_OPEN_READ && MYSERVER_FILE_OPEN_WRITE)
		F_Flags = O_RDWR;
	else if(opt & MYSERVER_FILE_OPEN_READ)
		F_Flags = O_RDONLY;
	else if(opt & MYSERVER_FILE_OPEN_WRITE)
		F_Flags = O_WRONLY;
		
	char Buffer[strlen(filename)+1];
		
	if(opt & MYSERVER_FILE_OPEN_HIDDEN)
	{
		int index;
		Buffer[0] = '\0';
		for(index = strlen(filename); index >= 0; index--)
			if(filename[index] == '/')
			{
				index++;
				break;
			}
		if(index > 0)
			strncat(Buffer, filename, index);
		strcat(Buffer, ".");
		strcat(Buffer, filename + index);
	}
	else
		strcpy(Buffer, filename);
		
	if(opt & MYSERVER_FILE_OPEN_IFEXISTS)
	{
		if(stat(filename, &F_Stats) < 0)
		{
			return 0;
		}
		else
			handle = (MYSERVER_FILE_HANDLE)open(Buffer,F_Flags);
	}
	else if(opt & MYSERVER_FILE_OPEN_APPEND)
	{
		if(stat(filename, &F_Stats) < 0)
			handle = (MYSERVER_FILE_HANDLE)open(Buffer,O_CREAT | F_Flags, S_IRUSR | S_IWUSR);
		else
			handle = (MYSERVER_FILE_HANDLE)open(Buffer,O_APPEND | F_Flags);
	}
	else if(opt & MYSERVER_FILE_CREATE_ALWAYS)
	{
		if(stat(filename, &F_Stats))
			remove(filename);

		handle = (MYSERVER_FILE_HANDLE)open(Buffer,O_CREAT | F_Flags, S_IRUSR | S_IWUSR);
	}
	else if(opt & MYSERVER_FILE_OPEN_ALWAYS)
	{
		if(stat(filename, &F_Stats) < 0)
			handle = (MYSERVER_FILE_HANDLE)open(Buffer,O_CREAT | F_Flags, S_IRUSR | S_IWUSR);
		else
			handle = (MYSERVER_FILE_HANDLE)open(Buffer,F_Flags);
	}
	
	if(opt & MYSERVER_FILE_OPEN_TEMPORARY)
		unlink(Buffer); // Remove File on close
	
	if((int)handle < 0)
		handle = (MYSERVER_FILE_HANDLE)0;
		
#endif
	
	return handle?1:0;
}
/*
*Returns the file handle.
*/
MYSERVER_FILE_HANDLE MYSERVER_FILE::getHandle()
{
	return handle;
}
/*
*Set the file handle.
*/
int MYSERVER_FILE::setHandle(MYSERVER_FILE_HANDLE hl)
{
	handle=hl;
	return 1;
}
/*
*define the operator =
*/
int MYSERVER_FILE::operator =(MYSERVER_FILE f)
{
	setHandle(f.getHandle());
	strcpy(filename,f.filename);
	return 1;
}
/*
*Returns the file path.
*/
char *MYSERVER_FILE::getFilename()
{
	return filename;
}
/*
*Read data from a file to a buffer.
*/
int	MYSERVER_FILE::readFromFile(char* buffer,u_long buffersize,u_long* nbr)
{
#ifdef WIN32
	ReadFile((HANDLE)handle,buffer,buffersize,nbr,NULL);
	/*
	*Return 1 if we don't reach the ond of the file.
	*Return 0 if the end of the file is reached.
	*/
	return (*nbr<=buffersize)? 1 : 0 ;
#endif
#ifdef __linux__
	*nbr = read((int)handle, buffer, buffersize);
	
	return (*nbr<=buffersize)? 1 : 0 ;
#endif
}
/*
*Create a temporary file.
*/
int MYSERVER_FILE::createTemporaryFile(char* filename)
{ 
	return openFile(filename,MYSERVER_FILE_OPEN_READ|MYSERVER_FILE_OPEN_WRITE|MYSERVER_FILE_CREATE_ALWAYS|MYSERVER_FILE_OPEN_HIDDEN|MYSERVER_FILE_OPEN_TEMPORARY);
}
/*
*Close an open file handle.
*/
int MYSERVER_FILE::closeFile()
{
	int ret=0;
	if(handle)
	{
#ifdef WIN32
		FlushFileBuffers((HANDLE)handle);
		ret=CloseHandle((HANDLE)handle);
#endif
#ifdef __linux__
		fsync((int)handle);
		ret=close((int)handle);
#endif
	}
	handle=0;
	return ret;
}
/*
*Delete an existing file passing the path.
*/
int MYSERVER_FILE::deleteFile(char *filename)
{
#ifdef WIN32
	DeleteFile(filename);
#endif
#ifdef __linux__
	remove(filename);
#endif
	return 0;
}
/*
*Returns the file size in bytes.
*/
u_long MYSERVER_FILE::getFileSize()
{
	u_long size;
#ifdef WIN32
	size=GetFileSize((HANDLE)handle,NULL);
#endif
#ifdef __linux__
	struct stat F_Stats;
	fstat((int)handle, &F_Stats);
	size = F_Stats.st_size;
#endif
	
	return size;
}
/*
*Change the position of the pointer to the file.
*Returns a non-null value if failed.
*/
int MYSERVER_FILE::setFilePointer(u_long initialByte)
{
#ifdef WIN32
	return (SetFilePointer((HANDLE)handle,initialByte,NULL,FILE_BEGIN)==INVALID_SET_FILE_POINTER)?1:0;
#endif
#ifdef __linux__
	return (lseek((int)handle, initialByte, SEEK_SET))?1:0;
#endif
}
/*
*Returns a non-null value if the path is a folder.
*/
int MYSERVER_FILE::isFolder(char *filename)
{
#ifdef WIN32
	u_long fa=GetFileAttributes(filename);
	if(fa!=INVALID_FILE_ATTRIBUTES)
		return(fa & FILE_ATTRIBUTE_DIRECTORY)?1:0;
	else
		return 0;
#endif
#ifdef __linux__
	//sprintf("in isFolder filename = %s\n", filename);
	struct stat F_Stats;
	if(stat(filename, &F_Stats) < 0)
		return 0;

	return (S_ISDIR(F_Stats.st_mode))? 1 : 0;
#endif
	
}

/*
*Returns a non-null value if the given path is a valid file.
*/
int MYSERVER_FILE::fileExists(char* filename)
{
#ifdef WIN32
	SECURITY_ATTRIBUTES sa = {0}; 
	MYSERVER_FILE_HANDLE fhandle=(MYSERVER_FILE_HANDLE)CreateFile(filename, GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL);
	if(fhandle==INVALID_HANDLE_VALUE)
	{
		return 0;
	}
	else
	{
		CloseHandle(fhandle);
		return 1;
	}
#endif
#ifdef __linux__
	struct stat F_Stats;
	if(stat(filename, &F_Stats) < 0)
		return 0;
		
	return (S_ISREG(F_Stats.st_mode))? 1 : 0;
#endif
}

/*
*Returns the time of the last modify to the file.
*/
time_t MYSERVER_FILE::getLastModTime(char *filename)
{
	int res;
#ifdef WIN32
	struct _stat sf;
	res=_stat(filename,&sf);
#endif
#ifdef __linux__
	struct stat sf;
	res=stat(filename,&sf);
#endif
	if(res==0)
		return sf.st_mtime;
	else
		return (-1);
}
time_t MYSERVER_FILE::getLastModTime()
{
	return getLastModTime(filename);
}

/*
*Returns the time of the file creation.
*/
time_t MYSERVER_FILE::getCreationTime(char *filename)
{
	int res;
#ifdef WIN32
	struct _stat sf;
	res=_stat(filename,&sf);
#endif
#ifdef __linux__
	struct stat sf;
	res=stat(filename,&sf);
#endif
	if(res==0)
		return sf.st_ctime;
	else
		return (-1);
}
time_t MYSERVER_FILE::getCreationTime()
{
	return getCreationTime(filename);
}
/*
*Returns the time of the last access to the file.
*/
time_t MYSERVER_FILE::getLastAccTime(char *filename)
{
	int res;
#ifdef WIN32
	struct _stat sf;
	res=_stat(filename,&sf);
#endif
#ifdef __linux__
	struct stat sf;
	res=stat(filename,&sf);
#endif
	if(res==0)
		return sf.st_atime;
	else
		return (-1);
}
time_t MYSERVER_FILE::getLastAccTime()
{
	return getLastAccTime(filename);
}


/*
*Get the filename from a path.
*/
void MYSERVER_FILE::getFilename(const char *path, char *filename)
{
	int splitpoint, i, j;
	i = 0;
	j = 0;
	splitpoint =(int)(strlen(path) - 1);
	while ((splitpoint > 0) && (path[splitpoint] != '/'))
	splitpoint--;
	if ((splitpoint == 0) && (path[splitpoint] != '/'))
	{
		strcpy(filename, path);
	}
	else
	{
		splitpoint++;
		i=splitpoint;
		while (path[i] != 0)
		{
			filename[j] = path[i];
			j++;
			i++;
		}
		filename[j] = 0;
	}
}
/*
*Splits a file path into a directory and filename.
*Path is an input value while dir and filename are the output values.
*/
void MYSERVER_FILE::splitPath(const char *path, char *dir, char *filename)
{
	int splitpoint, i, j;
	i = 0;
	j = 0;
	splitpoint =(int)( strlen(path) - 1);
	while ((splitpoint > 0) && ((path[splitpoint] != '/')&&(path[splitpoint] != '\\')))
		splitpoint--;
	if ((splitpoint == 0) && (path[splitpoint] != '/'))
	{
		dir[0] = 0;
		strcpy(filename, path);
	}
	else
	{
		splitpoint++;
		while (i < splitpoint)
		{
			dir[i] = path[i];
			i++;
		}
		dir[i] = 0;
		while (path[i] != 0)
		{
			filename[j] = path[i];
			j++;
			i++;
		}
		filename[j] = 0;
	}
}
/*
*Get the file extension passing its path.
*Save in ext all the bytes afer the last dot(.) if filename.
*/
void MYSERVER_FILE::getFileExt(char* ext,const char* filename)
{
	int nDot, nPathLen;
	nPathLen =(int)(strlen(filename) - 1);
	nDot = nPathLen;
	while ((nDot > 0) && (filename[nDot] != '.'))
		nDot--;
	if (nDot > 0)
		strcpy(ext, filename + nDot + 1);
	else
		ext[0] = 0;
}
/*
*Complete the path of the file.
*/
void MYSERVER_FILE::completePath(char *fileName)
{
#ifdef WIN32
	GetFullPathName(fileName,MAX_PATH,fileName,0);
#endif
#ifdef __linux__
	if(fileName[0]=='/')
		return;
	char buffer[MAX_PATH];
	strcpy(buffer,fileName);
	getdefaultwd(fileName,MAX_PATH);
	strcat(fileName,"/");
	strcat(fileName,buffer);
#endif

}
