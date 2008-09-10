/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006, 2007, 2008 Free Software Foundation, Inc.
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
#include <include/log/log_manager.h>
#include <include/base/utility.h>
#include <include/filter/gzip/gzip.h>
#include <include/base/file/files_utility.h>

#include <string>
#include <sstream>
#include <iostream>

using namespace std;

const int LogManager::TYPE_CONSOLE = 1;
const int LogManager::TYPE_FILE = 2;

/*!
 *Initialize the object.
 */
LogManager::LogManager()
{
  loaded = 0;
  /*!
   *By default put everything on the console.
   */
  type = TYPE_CONSOLE;
  gzipLog = 1;
  maxSize = 0;
  mutex.init();
  cycleLog = 0;
}

/*!
 *Destroy the object.
 */
LogManager::~LogManager()
{
  /*!
   *Try to close the file.
   */
  mutex.destroy();
  close();
}

/*!
 *Load and use the file to save logs.
 *Return zero on sucess.
 */
int LogManager::load(const char *filename)
{
  int opt, ret;
  /*!
   *If the file is still loaded close it before load again.
   */
  if(loaded)
    close();

  opt = File::MYSERVER_OPEN_APPEND | 
    File::MYSERVER_OPEN_ALWAYS |
    File::MYSERVER_OPEN_WRITE | 
    File::MYSERVER_OPEN_READ | 
    File::MYSERVER_NO_INHERIT;

  ret = file.openFile(filename, opt);

  if(ret)
  {
    return 1;
  }
  setType(TYPE_FILE);
  loaded = 1;
  return 0;
    
}

/*!
 *Close the file.
 */
int LogManager::close()
{
  if(loaded)
    file.closeFile();
  return 0;
}

/*!
 *Set the type for the log.
 */
void LogManager::setType(int nType)
{
  type = nType;
}

/*!
 *Set the max size for the log.
 *Returns the old limit.
 *Using a size of zero means that this limit is not used.
 */
u_long LogManager::setMaxSize(u_long nMax)
{
  u_long oldMax = maxSize;
  maxSize = nMax;
  return oldMax;
}


/*!
 *Write the string to the log plus termination character[s].
 *Returns 0 on success.
 */
int LogManager::writeln(const char *str)
{
  int ret = write(str);
#ifdef WIN32
  if(ret == 0)
    ret = write("\r\n");
#else
  if(ret == 0)
    ret = write("\n");
#endif
  return ret;
}

/*!
 *Set the log to save results on a new file when the max size is reached.
 */
void LogManager::setCycleLog(int l)
{
  cycleLog = l;
}

/*!
 *Get if the log store log data on a new file when the max size is reached.
 */
int LogManager::getCycleLog()
{
  return cycleLog;
}

/*!
 *Write the string to the log.
 *Returns 0 on success.
 */
int LogManager::write(const char *str, int len)
{
  int ret;
  if(type == TYPE_CONSOLE)
  {
    cout << str;
  }
  else if(type == TYPE_FILE)
  {
    u_long nbw;

    /*!
     *File was not loaded correctly.
     */
    if(!loaded)
    {
       return 1;
    }

    /*!
     *We reached the max file size.
     *Don't use this limitation if maxSize is equal to zero.
     */
    if(maxSize && (file.getFileSize() > maxSize))
    {
      if(storeFile())
        return 1;
    }

    /*!
     *If the len specified is equal to zero write the string as
     *a null character terminated one.
     */
    ret = file.writeToFile(str, len ? len : (u_long)strlen(str), &nbw);
    return ret;
  }
  return 0;
}

/*!
 *Set if the log will cycle log files using gzip.
 */
void LogManager::setGzip(bool useGzip)
{
  gzipLog = useGzip;
}

/*!
 *Returns nonzero if the log is using gzip for cycled logs.
 */
int LogManager::getGzip()
{
  return gzipLog;
}

/*!
 *Store the log manager in another file and reload the file.
 */
int LogManager::storeFile()
{
  char *buffer = 0;
  char *buffer2= 0;
  const u_long bufferSize = MYSERVER_KB(64);
  try
  {
    string filepath;
    string filedir;
    string filename;
    ostringstream newfilename;
    string time;
    Gzip gzip;

    /*! Do nothing if we haven't to cycle log files. */
    if(!cycleLog)
      return 1;

#ifdef DO_NOT_USE_GZIP
    gzipLog = 0;
#endif

    filepath.assign(getFile()->getFilename());
    FilesUtility::completePath(filepath);
    FilesUtility::splitPath(filepath, filedir, filename);

    getRFC822LocalTime(time, 32);
    time = trim(time.substr(5,32));
    
    for(int i=0;i< static_cast<int>(time.length()); i++)
      if((time[i] == ' ') || (time[i] == ':'))
        time[i]= '.';

    newfilename << filedir << "/" << filename << "." << time;
    
    if(gzipLog)
      newfilename << ".gz";
    
    {
      char gzipData[16];
      File newFile;
      File *currentFile = getFile();

      buffer = new char[bufferSize];
      if(buffer == 0)
        return 1;
      buffer2 = new char[bufferSize];
      if(buffer == 0)
        {
          delete [] buffer;
          return 1;
        }
     
      if(newFile.openFile(newfilename.str().c_str(), 
              File::MYSERVER_OPEN_WRITE | File::MYSERVER_NO_INHERIT | File::MYSERVER_CREATE_ALWAYS ))
      {
        delete [] buffer;
        delete [] buffer2;
        return 1;
      }
      if(currentFile->setFilePointer(0))
      {
        delete [] buffer;
        delete [] buffer2;
        newFile.closeFile();
        return 1;
      }
      if(gzipLog)
      {    
        u_long nbw;
        u_long len = gzip.getHeader(gzipData, 16);
        if(newFile.writeToFile(gzipData, len,  &nbw))
        {
          delete [] buffer;
          delete [] buffer2;
          newFile.closeFile();
          return 1;
        }  
        gzip.initialize();
      }
      
      for(;;)
      {
        u_long nbr;
        u_long nbw;
        if(currentFile->readFromFile(buffer,gzipLog ? bufferSize/2 : bufferSize, &nbr))
        {
          delete [] buffer;
          delete [] buffer2;      
          newFile.closeFile();
          return 1;
        }    
        if(nbr == 0)
          break;
        if(gzipLog)
        {
          u_long nbw;
          u_long size=gzip.compress(buffer,nbr, buffer2, bufferSize/2);
          if(newFile.writeToFile(buffer2, size, &nbw))
          {
            delete [] buffer;
            delete [] buffer2;
            newFile.closeFile();
            return 1;
          } 
        }
        else if(newFile.writeToFile(buffer, nbr, &nbw))
        {
          delete [] buffer;
          delete [] buffer2;
          newFile.closeFile();
          return 1;
        } 
      }
      if(gzipLog)
      {
        u_long nbw;
        u_long len = gzip.flush(buffer2, bufferSize/2);
        if(newFile.writeToFile(buffer2, len, &nbw))
        {
          delete [] buffer;
          delete [] buffer2;
          newFile.closeFile();
          return 1;
        }  
        len=gzip.getFooter(gzipData, 16);
        if(newFile.writeToFile(gzipData, len, &nbw))
        {
          delete [] buffer;
          delete [] buffer2;
          newFile.closeFile();
          return 1;
        }   
        gzip.free();
      }
      newFile.closeFile();
      currentFile->closeFile();
      FilesUtility::deleteFile(filepath.c_str());
      if(currentFile->openFile(filepath.c_str(), File::MYSERVER_OPEN_APPEND|
                               File::MYSERVER_OPEN_ALWAYS | File::MYSERVER_OPEN_WRITE |
                               File::MYSERVER_OPEN_READ | File::MYSERVER_NO_INHERIT))
      {
        delete [] buffer;
        delete [] buffer2;
        return 1;
      }
    }

    delete [] buffer;
    delete [] buffer2;
    return 0;
  }
  catch(...)
  {
    if(buffer)
      delete [] buffer;
    if(buffer2)
      delete [] buffer2;
    throw;
  };
}

/*!
 *Get the type of log.
 */
int LogManager::getType()
{
  return type;
}

/*!
 *Switch in the error output mode.
 */
int LogManager::preparePrintError()
{

  if(type == TYPE_CONSOLE)
  {
    ::preparePrintError();
  }

  return 0;
}

/*!
 *Exit from printing errors.
 */
int LogManager::endPrintError()
{
  if(type == TYPE_CONSOLE)
  {
    ::endPrintError();
  }
  return 0;
}

/*!
 *Get a pointer to the file object.
 */
File *LogManager::getFile()
{
  return &file;
}

/*!
 *Request access for the thread.
 */
int LogManager::requestAccess()
{
  mutex.lock();
  return 0;
}

/*!
 *Terminate the access for the thread.
 */
int LogManager::terminateAccess()
{
  mutex.unlock();
  return 0;
}

/*!
 *Return the max size for the log.
 */
u_long LogManager::getMaxSize()
{
  return maxSize;
}

/*!
 *Return the actual size for the log file.
 */
int LogManager::getLogSize()
{
  switch(type)
  {
    case TYPE_FILE:
      return file.getFileSize();

    case TYPE_CONSOLE:
      return 0;
  }

  return 0;

}
