/*
*MyServer
*Copyright (C) 2002,2003,2004,2005 The MyServer Team
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

#ifdef NOT_WIN
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

extern int mustEndServer; 

/*!
 *Return the recursion of the path.
 *Return -1 on errors.
 */
int File::getPathRecursionLevel(char* path)
{
	char *lpath;
  int lpath_len = strlen(path) + 1;
	int rec=0;
	char *token;
  lpath = new char[lpath_len];
  if(lpath == 0)
    return -1;
	strcpy(lpath,path);
  token = strtok( lpath, "\\/" );
	do
	{
		if(token != NULL) 
		{
			/*! ".." decreases the recursion level. */
			if( !strcmp(token,"..")  )
				rec--;
			/*! "." keeps recursion level equal to the previous. */
			else if(strcmp(token,".") )
			/*! Everything else increases it. */
				rec++;
		}
		token = strtok( NULL, "\\/" );
	}
	while( token != NULL );
  delete [] lpath;
	return rec;
}
/*!
 *Costructor of the class.
 */
File::File()
{
	filename=0;
	handle=0;
}
/*!
 *Write data to a file.
 *buffer is the pointer to the data to write
 *buffersize is the number of byte to write
 *nbw is a pointer to an unsigned long that receive the number of the
 *bytes written correctly.
 */
int File::writeToFile(const char* buffer, u_long buffersize,u_long* nbw)
{
	if(buffersize==0)
	{
		*nbw=0;
		return 1;
	}
#ifdef WIN32
	int ret = WriteFile((HANDLE)handle,buffer,buffersize,nbw,NULL);
	return (!ret);
#endif
#ifdef NOT_WIN
	*nbw =  write((long)handle, buffer, buffersize);
	return (*nbw==buffersize)? 0 : 1 ;
#endif
}

/*!
 *Constructor for the class.
 */
File::File(char *nfilename, int opt) 
  : handle(0)
{
  filename = 0;
  openFile(nfilename, opt);
}

/*!
 *Open(or create if not exists) a file.
 *If the function have success the return value is nonzero.
 *filename is the name of the file to open.
 *opt is a bit-field containing the options on how open it.
 *openFile returns 0 if the call was successfull, any other value on errors.
 */
int File::openFile(char* nfilename,u_long opt)
{
	long ret=0;
  int filename_len;
  if(filename)
  {
    delete [] filename;
  }
  filename_len = strlen(nfilename) + 1;
  filename = new char[filename_len];
  if(filename == 0)
    return -1;
	strcpy(filename, nfilename);
#ifdef WIN32
	SECURITY_ATTRIBUTES sa = {0};
	sa.nLength = sizeof(sa);
	if(opt & FILE_NO_INHERIT)
		sa.bInheritHandle = FALSE;
	else
		sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;
	u_long creationFlag=0;
	u_long openFlag=0;
	u_long attributeFlag=0;

	if(opt & FILE_OPEN_ALWAYS)
		creationFlag|=OPEN_ALWAYS;
	if(opt & FILE_OPEN_IFEXISTS)
		creationFlag|=OPEN_EXISTING;
	if(opt & FILE_CREATE_ALWAYS)
		creationFlag|=CREATE_ALWAYS;

	if(opt & FILE_OPEN_READ)
		openFlag|=GENERIC_READ;
	if(opt & FILE_OPEN_WRITE)
		openFlag|=GENERIC_WRITE;

	if(opt & FILE_OPEN_TEMPORARY)
	{
		openFlag|=FILE_ATTRIBUTE_TEMPORARY; 
		attributeFlag|=FILE_FLAG_DELETE_ON_CLOSE;
	}
	if(opt & FILE_OPEN_HIDDEN)
		openFlag|=FILE_ATTRIBUTE_HIDDEN;

	if(attributeFlag == 0)
		attributeFlag = FILE_ATTRIBUTE_NORMAL;
	handle=(FileHandle)CreateFile(filename, openFlag, 
                                          FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                          &sa, creationFlag, attributeFlag, NULL);
	/*! Return 1 if an error happens.  */
  if(handle==INVALID_HANDLE_VALUE)
  {
    if(filename)
    {
      delete [] filename;
    }
    filename = 0;
		return 1;
  }
	else/*! Open the file. */
	{
		if(opt & FILE_OPEN_APPEND)
			ret=setFilePointer(getFileSize());
		else
			ret=setFilePointer(0);
  		if(ret)
      {
        closeFile();
        if(filename)
        {
          delete [] filename;
        }
        filename = 0;
        return 1;
      }
	}

#endif
#ifdef NOT_WIN
	struct stat F_Stats;
	int F_Flags;
  char Buffer[strlen(filename)+1];
	if(opt && FILE_OPEN_READ && FILE_OPEN_WRITE)
		F_Flags = O_RDWR;
	else if(opt & FILE_OPEN_READ)
		F_Flags = O_RDONLY;
	else if(opt & FILE_OPEN_WRITE)
		F_Flags = O_WRONLY;
		
	if(opt & FILE_OPEN_HIDDEN)
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
		
	if(opt & FILE_OPEN_IFEXISTS)
	{
		ret = stat(filename, &F_Stats);
		if(ret  < 0)
		{
      if(filename)
        delete [] filename;
      filename = 0;
			return 1;
		}
		ret = open(Buffer,F_Flags);
		if(ret == -1)
    {
      if(filename)
        delete [] filename;
      filename = 0;
			return 1;
    }
		handle= (FileHandle)ret;
	}
	else if(opt & FILE_OPEN_APPEND)
	{
		ret = stat(filename, &F_Stats);
		if(ret < 0)
			ret = open(Buffer,O_CREAT | F_Flags, S_IRUSR | S_IWUSR);
		else
			ret = open(Buffer,O_APPEND | F_Flags);
		if(ret == -1)
     {
      if(filename)
        delete [] filename;
      filename = 0;
			return 1;
    }
		else
			handle=(FileHandle)ret;
	}
	else if(opt & FILE_CREATE_ALWAYS)
	{
		stat(filename, &F_Stats);
		if(ret)
			remove(filename);

		ret = open(Buffer,O_CREAT | F_Flags, S_IRUSR | S_IWUSR);
		if(ret == -1)
    {
      if(filename)
        delete [] filename;
	  filename = 0;
			return 1;
    }
		else
			handle=(FileHandle)ret;
	}
	else if(opt & FILE_OPEN_ALWAYS)
	{
		ret = stat(filename, &F_Stats);
		if(ret < 0)
			ret =open(Buffer,O_CREAT | F_Flags, S_IRUSR | S_IWUSR);
		else
			ret = open(Buffer,F_Flags);
		if(ret == -1)
    {
      if(filename)
        delete [] filename;
      filename = 0;
			return 1;
    }
		else
			 handle=(FileHandle)ret;
	}
	
	if(opt & FILE_OPEN_TEMPORARY)
		unlink(Buffer); // Remove File on close
	
	if((long)handle < 0)
  {
		handle = (FileHandle)0;
    if(filename)
      delete [] filename;
    filename = 0;
  }
#endif
	
	return 0;
}
/*!
 *Returns the file handle.
 */
FileHandle File::getHandle()
{
	return handle;
}
/*!
 *Set the file handle.
 *Return a non null-value on errors.
 */
int File::setHandle(FileHandle hl)
{
	handle=hl;
	return 0;
}
/*!
 *define the operator =
 */
int File::operator =(File f)
{
  if(filename)
    delete [] filename;
  filename = 0;
  setHandle(f.getHandle());
  if(f.filename)
  {
    int filename_len = strlen(f.filename)+1;
    filename = new char[filename_len];
    if(filename == 0)
      return -1;
    strcpy(filename,f.filename);
  }
  else
  {
    filename = 0;
    handle = 0;
  }
	return 0;
}
/*!
 *Set the name of the file
 *Return Non-zero on errors.
 */
int File::setFilename(char* nfilename)
{
  if(filename)
    delete [] filename;
  filename = 0;
  int filename_len = strlen(nfilename)+1;
  filename = new char[filename_len];
  if(filename == 0)
    return -1;
	strcpy(filename,nfilename);
  return 0;
}
/*!
 *Returns the file path.
 */
char *File::getFilename()
{
	return filename;
}
/*!
 *Read data from a file to a buffer.
 *Return 1 or errors.
 *Return 0 on success.
 */
int File::readFromFile(char* buffer,u_long buffersize,u_long* nbr)
{
#ifdef WIN32
	int ret = ReadFile((HANDLE)handle, buffer, buffersize, nbr, NULL);
	return (!ret);
#endif
#ifdef NOT_WIN
	int ret  = read((long)handle, buffer, buffersize);
  *nbr = (u_long)ret;
	return (ret == -1) ;
#endif
}

/*!
 *Create a temporary file.
 */
int File::createTemporaryFile(char* filename)
{ 
	return openFile(filename,FILE_OPEN_READ|FILE_OPEN_WRITE
                  |FILE_CREATE_ALWAYS|FILE_OPEN_TEMPORARY);

}
/*!
 *Close an open file handle.
 */
int File::closeFile()
{
	int ret=0;
	if(handle)
    {
#ifdef WIN32
      if(handle)
      {
        FlushFileBuffers((HANDLE)handle);
        ret=CloseHandle((HANDLE)handle);
      }
#endif
#ifdef NOT_WIN
      if(handle)
      {
        fsync((long)handle);
        ret=close((long)handle);
      }
#endif
	}
  if(filename)
    delete [] filename;
	filename = 0;
	handle=0;
	return ret;
}
/*!
 *Delete an existing file passing the path.
 *Return a non-null value on errors.
 */
int File::deleteFile(char *filename)
{
	int ret;
#ifdef WIN32
	ret = DeleteFile(filename);
	if(ret)
		return 0;
#endif
#ifdef NOT_WIN
	ret = remove(filename);
#endif
	return ret;
}
/*!
 *Returns the file size in bytes.
 *Returns -1 on errors.
 */
u_long File::getFileSize()
{
	u_long ret;
#ifdef WIN32
	ret=GetFileSize((HANDLE)handle,NULL);
	if(ret!=INVALID_FILE_SIZE)
	{
		return ret;
	}
	else
		return (u_long)-1;
#endif
#ifdef NOT_WIN
	struct stat F_Stats;
	ret = fstat((long)handle, &F_Stats);
	if(ret)
		return (u_long)(-1);
	else
		return F_Stats.st_size;
#endif
}
/*!
 *Change the position of the pointer to the file.
 */
int File::setFilePointer(u_long initialByte)
{
	u_long ret;
#ifdef WIN32
	ret=SetFilePointer((HANDLE)handle,initialByte,NULL,FILE_BEGIN);
  /*! SetFilePointer returns INVALID_SET_FILE_POINTER on an error.  */
	return (ret==INVALID_SET_FILE_POINTER)?1:0;
#endif
#ifdef NOT_WIN
	ret = lseek((long)handle, initialByte, SEEK_SET);
	return (ret!=initialByte)?1:0;
#endif
}
/*!
 *Returns a non-null value if the path is a directory.
 */
int File::isDirectory(char *filename)
{
#ifdef WIN32
	u_long fa=GetFileAttributes(filename);
	if(fa!=INVALID_FILE_ATTRIBUTES)
		return(fa & FILE_ATTRIBUTE_DIRECTORY)?1:0;
	else
		return 0;
#endif
#ifdef NOT_WIN
	struct stat F_Stats;
	int ret = stat(filename, &F_Stats);
	if(ret < 0)
		return 0;

	return (S_ISDIR(F_Stats.st_mode))? 1 : 0;
#endif
}

/*!
 *Returns a non-null value if the given path is a valid file.
 */
int File::fileExists(char* filename)
{
#ifdef WIN32
	OFSTRUCT of;
	/*! OpenFile is now a wrapper for CreateFile.  */
	int ret = OpenFile(filename, &of, OF_EXIST);
	return (ret != HFILE_ERROR)?1:0;
#endif
#ifdef NOT_WIN
	struct stat F_Stats;
	int ret = stat(filename, &F_Stats);
	if(ret < 0)
		return 0;
	/*! Return 1 if it is a regular file or a directory.  */
	return (S_ISREG(F_Stats.st_mode) | S_ISDIR(F_Stats.st_mode))? 1 : 0;
#endif
}

/*!
 *Returns the time of the last modify to the file.
 *Returns -1 on errors.
 */
time_t File::getLastModTime(char *filename)
{
	int res;
#ifdef WIN32
	struct _stat sf;
	res=_stat(filename,&sf);
#endif
#ifdef NOT_WIN
	struct stat sf;
	res=stat(filename,&sf);
#endif
	if(res==0)
		return sf.st_mtime;
	else
		return (-1);
}
/*!
 *Get the time of the last modifify did to the file.
 */
time_t File::getLastModTime()
{
	return getLastModTime(filename);
}

/*!
 *Returns the time of the file creation.
 *Returns -1 on errors.
 */
time_t File::getCreationTime(char *filename)
{
	int res;
#ifdef WIN32
	struct _stat sf;
	res=_stat(filename,&sf);
#endif
#ifdef NOT_WIN
	struct stat sf;
	res=stat(filename,&sf);
#endif
	if(res==0)
		return sf.st_ctime;
	else
		return (-1);
}
/*!
 *This function returns the creation time of the file.
 */
time_t File::getCreationTime()
{
	return getCreationTime(filename);
}
/*!
 *Returns the time of the last access to the file.
 *Returns -1 on errors.
 */
time_t File::getLastAccTime(char *filename)
{
	int res;
#ifdef WIN32
	struct _stat sf;
	res=_stat(filename,&sf);
#endif
#ifdef NOT_WIN
	struct stat sf;
	res=stat(filename,&sf);
#endif
	if(res==0)
		return sf.st_atime;
	else
		return (-1);
}
/*!
 *Returns the time of the last access to the file.
 */
time_t File::getLastAccTime()
{
	return getLastAccTime(filename);
}

/*!
 *Get the length of the file in the path.
 */
int File::getFilenameLength(const char *path, int *filename)
{
	int splitpoint, i, j;
	i = 0;
	j = 0;
	splitpoint =(int)(strlen(path) - 1);
	while ((splitpoint > 0) && (path[splitpoint] != '/'))
    splitpoint--;
  *filename = splitpoint + 1;
  return *filename;

}

/*!
 *Get the filename from a path.
 *Be sure that the filename buffer is at least getFilenameLength(...) bytes
 *before call this function.
 */
void File::getFilename(const char *path, char *filename)
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
		while(path[i] != 0)
		{
			filename[j] = path[i];
			j++;
			i++;
		}
		filename[j] = 0;
	}
}

/*!
 *Use this function before call splitPath to be sure that the buffers
 *dir and filename are bigger enough to contain the data.
 */
void File::splitPathLength(const char *path, int *dir, int *filename)
{
	int splitpoint, i, j, len;
	if(path == 0)
	{
		*dir=0;
		*filename=0;
		return;
	}
  len = strlen(path);
	i = 0;
	j = 0;
	splitpoint =(int)(len-1);
	while ((splitpoint > 0) && ((path[splitpoint] != '/')&&(path[splitpoint] != '\\')))
		splitpoint--;

  if(dir)
    *dir = splitpoint + 2;

  if(filename)
    *filename = len - splitpoint + 2;
}
/*!
 *Splits a file path into a directory and filename.
 *Path is an input value while dir and filename are the output values.
 */
void File::splitPath(const char *path, char *dir, char *filename)
{
	int splitpoint, i, j;
	i = 0;
	j = 0;
	if(path==0)
		return;
	splitpoint =(int)( strlen(path) - 1);
	while ((splitpoint > 0) && ((path[splitpoint] != '/')&&(path[splitpoint] != '\\')))
		splitpoint--;
	if ((splitpoint == 0) && (path[splitpoint] != '/'))
	{
		dir[0] = 0;
    if(filename)
      strcpy(filename, path);
	}
	else
	{
    if(dir)
    {
		 splitpoint++;
		 while (i < splitpoint)
		  {
      
        dir[i] = path[i];
			  i++;
		  }
		  dir[i] = 0;
    }
    i = splitpoint;
    if(filename)
    {
      while (path[i] != 0)
      {
        filename[j] = path[i];
        j++;
        i++;
      }
      filename[j] = 0;
    }
	}
}
/*!
*Get the file extension passing its path.
*Save in ext all the bytes afer the last dot(.) if filename.
*/
void File::getFileExt(char* ext,const char* filename)
{
	int nDot, nPathLen;
	nPathLen = (int)(strlen(filename) - 1);
	nDot = nPathLen;
	while ((nDot > 0) && (filename[nDot] != '.'))
		nDot--;
	if (nDot > 0)
		strcpy(ext, filename + nDot + 1);
	else
		ext[0] = 0;
}
/*!
*Get the file path in the short form.
*/
int File::getShortFileName(char *out,int buffersize)
{
#ifdef WIN32
  if(filename)
    return GetShortPathName(filename,out,buffersize);
  else
    return 0;
#endif
#ifdef NOT_WIN
  int ret = 0;
  int filename_len = strlen(filename) + 1 ;
  if(filename_len < buffersize)
  {
    ret = 0;
    strncpy(out,filename,buffersize);
  }
  else
  {
    ret = filename_len;
  }
  return ret;
#endif
}
/*!
*Get the file path in the short form of the specified file
*Return -1 on errors.
*/
int File::getShortFileName(char *filePath,char *out,int buffersize)
{
#ifdef WIN32
	int ret = GetShortPathName(filePath,out,buffersize);
	if(!ret)
		return -1;
	return 0;
#endif
#ifdef NOT_WIN
	strncpy(out,filePath,buffersize);
	return 0;	
#endif
}

/*!
 *Complete the path of the file.
 *Return non-zero on errors.
 *If dontRealloc is selected don't realloc memory.
 */
int File::completePath(char **fileName,int *size, int dontRealloc)
{
  if((fileName == 0) ||( *fileName==0))
    return -1;
#ifdef WIN32
  char *buffer;
  int bufferLen = strlen(*fileName) + 1;
  int bufferNewLen;
  buffer = new char[bufferLen];
  if(buffer == 0)
  {
    return -1;
  }
  strcpy(buffer, *fileName);
  bufferNewLen = GetFullPathName(buffer, 0, *fileName, 0) + 1;
  if(dontRealloc)
  {
    if(*size < bufferNewLen )
      return -1;
  }
  else
  {
    delete [] (*fileName);
    *fileName = new char[bufferNewLen];
    if(*fileName == 0)
    {
      *size = 0;
      delete [] buffer;
      return -1;
    }
    *size = bufferNewLen;
  }
  GetFullPathName(buffer, bufferNewLen, *fileName, 0);
  delete [] buffer;
  return 0;

#endif
#ifdef NOT_WIN

	char *buffer;
  int bufferLen;
  int bufferNewLen;
	if((*fileName)[0]=='/')
		return 0;
  bufferLen = strlen(*fileName) + 1;
  buffer = new char[bufferLen];
  if(buffer == 0)
    return 0;
	strcpy(buffer, *fileName);
  bufferNewLen =  getdefaultwdlen() +  bufferLen + 1 ;
  if(dontRealloc)
  {
    if(*size < bufferNewLen )
      return -1;
  }
  else
  {
    delete [] (*fileName);
    *fileName = new char[bufferNewLen];
    if(*fileName == 0)
    {
      *size = 0;
      delete [] buffer;
      return -1;
    }
    *size = bufferNewLen;
  }
 
  sprintf(*fileName, "%s/%s", getdefaultwd(0, 0), buffer );
  delete [] buffer;
	return 0;
#endif
}
