/*
MyServer
Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009 Free Software Foundation, Inc.
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "stdafx.h"
#include <include/base/file/files_utility.h>
#include <include/base/utility.h>
#include <include/base/string/stringutils.h>

extern "C"
{
#include <sys/stat.h>
}

#ifndef WIN32
extern "C"
{
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
}
#endif

#include <string>
#include <sstream>
#include <memory>

using namespace std;

string FilesUtility::tmpPath;

/*!
 *Constructor body.
 */
FilesUtility::FilesUtility()
{

}

/*!
 *This funtion iterates through every character of the path
 *The first IF tries to clear out the bars, if it finds one,
 *just advances one character and starts the cycle again
 *The second IF tries to find at least two dots.
 *if it finds only 1, does nothing,
 *WIN32 if it finds 2 or more rec=rec-(number of dots -1)
 * *NIX if it finds 2, decrements rec, if it finds more,
 *increments it considering it's a name if it ends with
 *something else other then a bar of a NULL,
 *then it's a path of the form "/...qwerty/" that should be considered rec++
 *The last ELSE, catches the rest and advances to the next bar.
 *Return the recursion of the path.
 *\param path The file name.
 */
int FilesUtility::getPathRecursionLevel(const char* path)
{
  const char *lpath = path;
  int rec = 0;
#ifdef WIN32
  int temp;
#endif
  while(*lpath != 0)
  {
  /* ".." decreases the recursion level.  */
    if( (*lpath == '\\') || (*lpath == '/') )
    {
      lpath++;
      continue;
    }

    if(*lpath=='.')
    {
      lpath++;
#ifdef WIN32//--------------------------------
      temp = 0;
      while(*lpath == '.')
      {
        lpath++;
        temp++;
      }
      if( (*lpath == '\\') || (*lpath == '/') || (*lpath == 0))
        rec -= temp;
      else
      {
        lpath++;
        while( (*lpath != '\\') && (*lpath != '/') && (*lpath != 0))
          lpath++;
        rec++;
      }
#else    //--------------------------------
      if(*lpath == '.')
      {
        lpath++;
        if( (*lpath == '\\') || (*lpath == '/') || (*lpath == 0))
          rec--;
        else
        {
          while( (*lpath != '\\') && (*lpath != '/') && (*lpath != 0))
            lpath++;
          rec++;
        }
      }
#endif    //--------------------------------
    }
    else
    {
      while( (*lpath != '\\') && (*lpath != '/') && (*lpath != 0))
        lpath++;
      rec++;
    }
  }
  return rec;
}


/*!
 *Rename the file [BEFORE] to [AFTER]. Returns 0 on success.
 *\param before The old file name.
 *\param after The new file name.
 */
int FilesUtility::renameFile(const char* before, const char* after)
{
#ifdef WIN32
  return MoveFile(before, after) ? 0 : 1;
#else
  return rename(before, after);
#endif
}

/*!
 *Copy the file from [SRC] to [DEST]. Returns 0 on success.
 *\param src The source file name.
 *\param dest The destination file name.
 *\param overwrite Overwrite the dest file if already exists?
 */
int FilesUtility::copyFile (const char* src, const char* dest, int overwrite)
{
#ifdef WIN32
  return CopyFile (src, dest, overwrite ? FALSE : TRUE) ? 0 : 1;
#else
  File srcFile;
  File destFile;
  char buffer[4096];
  size_t dim;

  if (srcFile.openFile (src, File::MYSERVER_OPEN_READ))
    return -1;

  if (destFile.openFile (dest, File::MYSERVER_OPEN_WRITE | (overwrite ? File::MYSERVER_CREATE_ALWAYS : 0)))
    {
      srcFile.close ();
      return -1;
    }

  int ret = copyFile (srcFile, destFile);

  srcFile.close ();
  destFile.close ();

  return ret;
#endif
}

/*!
 *Copy the file from [SRC] to [DEST]. Returns 0 on success.
 *\param src The source File.
 *\param dest The destination File.
 */
int FilesUtility::copyFile (File& src, File& dest)
{
  char buffer[4096];
  u_long nbr, nbw;
  int ret;

  for (;;) 
  {
    ret = src.read (buffer, 4096, &nbr);
    if (ret)
      return -1;
    
    if (!nbr)
      break;
    
    ret = dest.writeToFile (buffer, nbr, &nbw);
    if (ret)
      return -1;
  }
  return 0;
}

/*!
 *Delete an existing file passing the path.
 *Return a non-null value on errors.
 *\param filename The file to delete.
 */
int FilesUtility::deleteFile(const char *filename)
{
  int ret;
#ifdef WIN32
  ret = DeleteFile(filename);
  if(ret)
    return 0;
#else
  ret = remove(filename);
#endif
  return ret;
}

/*!
 *Returns a non-null value if the path is a directory.
 *\param filename The path to check.
 */
int FilesUtility::isDirectory(const char *filename)
{
#ifdef WIN32
  u_long fa = GetFileAttributes(filename);
  if(fa != INVALID_FILE_ATTRIBUTES)
    return(fa & FILE_ATTRIBUTE_DIRECTORY)?1:0;
  else
    return 0;
#else
  struct stat F_Stats;
  int ret = stat(filename, &F_Stats);
  if(ret < 0)
    return 0;

  return (S_ISDIR(F_Stats.st_mode))? 1 : 0;
#endif
}

/*!
 *Returns a non-null value if the given path is a link.
 *\param filename The path to check.
 */
int FilesUtility::isLink(const char* filename)
{
#ifdef WIN32
  return 0;
#else
  struct stat F_Stats;
  int ret = lstat(filename, &F_Stats);
  if(ret < 0)
    return 0;

  return (S_ISLNK(F_Stats.st_mode))? 1 : 0;
#endif

}

/*!
 *Returns a non-null value if the given path is a valid file.
 *\param filename The path to check.
 */
int FilesUtility::fileExists(const char* filename)
{
#ifdef WIN32
  /*! OpenFile is obsolete
  OFSTRUCT of;
  int ret = OpenFile(filename, &of, OF_EXIST);
  return (ret != HFILE_ERROR)?1:0;*/
  HANDLE hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ,
                       NULL, OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                       NULL);

  int nRet = hFile != INVALID_HANDLE_VALUE ? 1 : 0;
  CloseHandle(hFile);
  return nRet;
#else
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
 *\param filename The path to check.
 */
time_t FilesUtility::getLastModTime(const char *filename)
{
  int res;
#ifdef WIN32
  struct _stat sf;
  res = _stat(filename,&sf);
#else
  struct stat sf;
  res = stat(filename,&sf);
#endif

  if(res == 0)
    return sf.st_mtime;
  else
    return (-1);
}

/*!
 *Returns the time of the file creation.
 *Returns -1 on errors.
 *\param filename The path to check.
 */
time_t FilesUtility::getCreationTime(const char *filename)
{
  int res;
#ifdef WIN32
  struct _stat sf;
  res = _stat(filename, &sf);
#else
  struct stat sf;
  res = stat(filename, &sf);
#endif
  if(res == 0)
    return sf.st_ctime;
  else
    return (-1);
}

/*!
 *Returns the time of the last access to the file.
 *Returns -1 on errors.
 *\param filename The path to check.
 */
time_t FilesUtility::getLastAccTime(const char *filename)
{
  int res;
#ifdef WIN32
  struct _stat sf;
  res = _stat(filename, &sf);
#else
  struct stat sf;
  res = stat(filename, &sf);
#endif
  if(res == 0)
    return sf.st_atime;
  else
    return (-1);
}

/*!
 *Change the owner of the current file, use the value -1 for uid or gid
 *to do not change the value. Return 0 on success.
 *\param filename The path to the file to chown.
 *\param uid The user id.
 *\param gid the group id.
 */
int FilesUtility::chown(const char* filename, int uid, int gid)
{
#ifndef WIN32
  return ::chown(filename, uid, gid) ? 1 : 0;
#endif
  return 0;
}

/*!
 *Get the length of the file in the path.
 *\param path The full path where get the filename length.
 *\param filename A pointer to the start of the file name.
 */
int FilesUtility::getFilenameLength(const char *path, int *filename)
{
  int splitpoint, i, j;
  i = 0;
  j = 0;
  splitpoint = static_cast<int>(strlen(path) - 1);
  while ((splitpoint > 0) && (path[splitpoint] != '/'))
    splitpoint--;
  *filename = splitpoint + 1;
  return *filename;

}

/*!
 *Get the filename from a path.
 *Be sure that the filename buffer is at least getFilenameLength(...) bytes
 *before call this function.
 *\param path The full path to the file.
 *\param filename A buffer to fullfill with the file name.
 */
void FilesUtility::getFilename(const char *path, char *filename)
{
  int splitpoint, i, j;
  i = 0;
  j = 0;
  splitpoint = static_cast<int>(strlen(path) - 1);
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
 *Get the filename from a path.
 *\param path The full path to the file.
 *\param filename A buffer to fullfill with the file name.
 */
void FilesUtility::getFilename(string const &path, string& filename)
{
  u_long splitpoint = path.find_last_of("\\/");
  if(splitpoint != string::npos)
  {
    filename = path.substr(splitpoint+1, path.length()-1);
  }
  else
    filename.assign("");

}

/*!
 *Use this function before call splitPath to be sure that the buffers
 *dir and filename are bigger enough to contain the data.
 *\param path The full path to the file.
 *\param dir The directory part length of the path.
 *\param filename The length of the buffer needed to contain the file name.
 */
void FilesUtility::splitPathLength(const char *path, int *dir, int *filename)
{
  int splitpoint = 0;
  int i = 0;

  for(i = 0; path[i]; i++)
    if(path[i] == '/' || path[i] == '\\' )
      splitpoint = i;

  if(dir)
  {
    *dir = splitpoint + 2;
  }

  if(filename)
  {
    *filename = i - splitpoint + 2;
  }
}

/*!
 *Splits a file path into a directory and filename.
 *Path is an input value while dir and filename are the output values.
 *\param path The full path to the file.
 *\param dir The directory part of the path.
 *\param filename A buffer to fullfill with the file name.
 */
void FilesUtility::splitPath(const char *path, char *dir, char *filename)
{
  int splitpoint = 0;
  int i = 0;

  for(i = 0; path[i]; i++)
    if(path[i] == '/' || path[i] == '\\' )
      splitpoint = i;

  if(dir)
  {
    if(splitpoint)
    {
      memcpy(dir, path, splitpoint);
      dir[splitpoint] = '/';
      dir[splitpoint + 1] = '\0';
    }
    else
      dir[0] = '\0';
  }

  if(filename)
  {
    memcpy(filename, path + splitpoint + (splitpoint ? 1 : 0), i - splitpoint - (splitpoint ? 1 : 0));
    filename[i - splitpoint - (splitpoint ? 1 : 0)] = '\0';
  }
}

/*!
 *Split a path in a dir and a filename.
 *\param path The full path to the file.
 *\param dir The directory part of the path.
 *\param filename A buffer to fullfill with the file name.
 */
void FilesUtility::splitPath(string const &path, string& dir, string& filename)
{
  u_long splitpoint;
  u_long len = path.length();
  splitpoint = path.find_last_of("\\/");
  if(splitpoint != string::npos)
  {
    char lastDirChar;
    dir  = path.substr(0, splitpoint);

    lastDirChar = dir[dir.length() - 1];

    if(lastDirChar != '\\' && lastDirChar != '/')
      dir.append("/");


    filename = path.substr(splitpoint+1, len-1);
  }
  else
  {
    filename  = path;
    dir.assign("");
  }
}

/*!
 *Get the file extension passing its path.
 *Save in ext all the bytes afer the last dot(.) in filename.
 *\param ext The buffer to fullfill with the file extension.
 *\param filename The path to the file.
 */
void FilesUtility::getFileExt(char* ext, const char* filename)
{
  int nDot;
  nDot = static_cast<int>(strlen(filename) - 1);

  while ((nDot > 0) && (filename[nDot] != '.'))
    nDot--;

  if (nDot > 0)
    strcpy(ext, filename + nDot + 1);
  else
    ext[0] = 0;
}

/*!
 *Get the file extension passing its path.
 *Save in ext all the bytes afer the last dot(.) in filename.
 *\param ext The buffer to fullfill with the file extension.
 *\param filename The path to the file.
 */
void FilesUtility::getFileExt(string& ext, string const &filename)
{
  u_long pos = filename.find_last_of('.');
  if(pos != string::npos)
  {
    ext = filename.substr(pos+1, filename.length()-1);
  }
  else
  {
    ext.assign("");
  }

}

/*!
 *Get the file path in the short form of the specified file
 *Return -1 on errors.
 *\param filePath The path to use.
 *\param out The buffer where write.
 *\param buffersize The buffer length.
 */
int FilesUtility::getShortFileName(char *filePath, char *out, int buffersize)
{
#ifdef WIN32
  int ret = GetShortPathName(filePath, out, buffersize);
  if(!ret)
    return -1;
  return 0;
#else
  strncpy(out, filePath, buffersize);
  return 0;
#endif
}

/*!
 *Complete the path of the file.
 *Return non-zero on errors.
 *\param fileName The buffer to use.
 *\param size The new buffer size.
 *\param dontRealloc Don't realloc a new buffer.
 */
int FilesUtility::completePath(char **fileName,int *size, int dontRealloc)
{
  if((fileName == 0) ||( *fileName==0))
    return -1;
#ifdef WIN32
  char *buffer;
  int bufferLen = strlen(*fileName) + 1;
  int bufferNewLen;
  auto_ptr<char> bufferAutoPtr( new char[bufferLen] );

  buffer = bufferAutoPtr.get();

  strcpy(buffer, *fileName);
  bufferNewLen = GetFullPathName(buffer, 0, *fileName, 0) + 1;
  if(bufferNewLen == 0)
  {
    return -1;
  }
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
      return -1;
    }
    *size = bufferNewLen;
  }
  if(GetFullPathName(buffer, bufferNewLen, *fileName, 0) == 0)
    return -1;

  return 0;
#else
  char *buffer;
  int bufferLen;
  int bufferNewLen;
  /* We assume that path starting with / are yet completed.  */
  if((*fileName)[0]=='/')
    return 0;
  bufferLen = strlen(*fileName) + 1;

  auto_ptr<char> bufferAutoPtr( new char[bufferLen] );

  buffer = bufferAutoPtr.get();


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
      return -1;
    }
    *size = bufferNewLen;
  }
  sprintf(*fileName, "%s/%s", getdefaultwd(0, 0), buffer );

  return 0;
#endif
}

/*!
 *Complete the path of the file.
 *Return non-zero on errors.
 *\param fileName The file name to complete.
 */
int FilesUtility::completePath(string &fileName)
{
#ifdef WIN32
  char *buffer;
  int bufferLen;

  bufferLen = GetFullPathName(fileName.c_str(), 0, buffer, 0) + 1;
  if(bufferLen == 0)
    return -1;

  auto_ptr<char> bufferAutoPtr( new char[bufferLen] );

  buffer = bufferAutoPtr.get();

  if(GetFullPathName(fileName.c_str(), bufferLen, buffer, 0) == 0)
    return -1;

  fileName.assign(buffer);

  return 0;
#else
  ostringstream stream;
  /* We assume that path starting with / are yet completed. */
  if(fileName[0] != '/')
  {
    stream << getdefaultwd(0, 0) << "/" <<  fileName.c_str();
    fileName.assign(stream.str());
  }

  return 0;
#endif
}

/*!
 *Create a new directory.
 *Return non-zero on errors.
 *\param path The path to the directory to create.
 */
int FilesUtility::mkdir (const char *path)
{
#ifdef WIN32
  return CreateDirectory(path, NULL)?0:-1;
#else
  return ::mkdir(path, S_IRUSR | S_IWUSR);
#endif
}


/*!
 *Delete a directory.
 *Return non-zero on errors.
 *\param path The directory to remove.
 */
int FilesUtility::rmdir (const char *path)
{
#ifdef WIN32
  return RemoveDirectory(path)?0:-1;
#else
  return ::rmdir(path);
#endif
}

/*!
 *Set the temporary path to a default one.
 */
void FilesUtility::resetTmpPath()
{
#ifdef WIN32
  const char *tmpDir = getenv ("TMPDIR");
  if (tmpDir)
    tmpPath.assign (tmpDir);
  else
  {
    TCHAR lpPathBuffer[MAX_PATH];
    GetTempPath (MAX_PATH, lpPathBuffer);
    tmpPath.assign (lpPathBuffer);
  }
#else
  const char *tmpDir = getenv ("TMPDIR");

#ifdef P_tmpdir
  if (tmpDir == NULL)
    tmpDir = P_tmpdir;
#endif

  if (tmpDir == NULL)
    tmpDir = "/tmp";

  tmpPath.assign (tmpDir);
#endif
}


/*!
 *Create an unique temporary file name.  This function doesn't create
 *or open the file but generates only its name.
 *\param tid Caller thread id.
 *\param out Output string.
 */
void FilesUtility::temporaryFileName (u_long tid, string &out)
{
  ostringstream stream;
  static u_long counter = 1;
  counter++;

  if (tmpPath.length () == 0)
    tmpPath.assign (getdefaultwd (0, 0));

  stream << tmpPath << "/myserver_" << counter  << "_" << tid << ".tmp";
  out.assign (stream.str ());
}
