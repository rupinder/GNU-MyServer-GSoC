/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
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
#include "../include/log_manager.h"
#include "../include/utility.h"

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
  max_size = 0;
	mutex.init();
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

  opt = FILE_OPEN_APPEND | FILE_OPEN_ALWAYS |
            FILE_OPEN_WRITE | FILE_NO_INHERIT;

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
  u_long oldMax = max_size;
  max_size = nMax;
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
 *Write the string to the log.
 *Returns 0 on success.
 */
int LogManager::write(const char *str, int len)
{
  int ret;
  if(type == TYPE_CONSOLE)
  {
    printf("%s", str);
  }
  else if(type == TYPE_FILE)
  {
    /*!
    *File was not loaded correctly.
    */
    if(!loaded)
    {
       return 1;
    }

    /*!
     *We reached the max file size.
     *Don't use this limitation if max_size is equal to zero.
     */
    if(max_size && (file.getFileSize() > max_size))
    {
      return 1;
    }

    u_long nbw;

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
  return max_size;
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
