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

const int MYSERVER_LOG_MANAGER::TYPE_CONSOLE = 1;
const int MYSERVER_LOG_MANAGER::TYPE_FILE = 2;

/*!
 *Initialize the object.
 */
MYSERVER_LOG_MANAGER::MYSERVER_LOG_MANAGER()
{
  loaded = 0;
  /*!
   *By default put everything on the console.
   */
  type = TYPE_CONSOLE;
  max_size = 0;
	mutex.myserver_mutex_init();
}

/*!
 *Destroy the object.
 */
MYSERVER_LOG_MANAGER::~MYSERVER_LOG_MANAGER()
{
  /*!
   *Try to close the file.
   */
	mutex.myserver_mutex_destroy();
  close();
}

/*!
 *Load and use the file to save logs.
 *Return zero on sucess.
 */
int MYSERVER_LOG_MANAGER::load(char *filename)
{
  int opt, ret;
  /*!
   *If the file is still loaded close it before load again.
   */
  if(loaded)
    close();

  opt = MYSERVER_FILE_OPEN_APPEND | MYSERVER_FILE_OPEN_ALWAYS |
            MYSERVER_FILE_OPEN_WRITE | MYSERVER_FILE_NO_INHERIT;

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
int MYSERVER_LOG_MANAGER::close()
{
  if(loaded)
    file.closeFile();
  return 0;
}

/*!
 *Set the type for the log.
 */
void MYSERVER_LOG_MANAGER::setType(int nType)
{
  type = nType;
}

/*!
 *Set the max size for the log.
 *Returns the old limit.
 *Using a size of zero means that this limit is not used.
 */
int MYSERVER_LOG_MANAGER::setMaxSize(int nMax)
{
  int oldMax = max_size;
  max_size = nMax;
  return oldMax;
}


/*!
 *Write the string to the log plus termination character[s].
 *Returns 0 on success.
 */
int MYSERVER_LOG_MANAGER::writeln(char *str)
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
int MYSERVER_LOG_MANAGER::write(char *str, int len)
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
int MYSERVER_LOG_MANAGER::getType()
{
  return type;
}

/*!
 *Switch in the error output mode.
 */
int MYSERVER_LOG_MANAGER::preparePrintError()
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
int MYSERVER_LOG_MANAGER::endPrintError()
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
MYSERVER_FILE *MYSERVER_LOG_MANAGER::getFile()
{
  return &file;
}

/*!
 *Request access for the thread.
 */
int MYSERVER_LOG_MANAGER::requestAccess()
{
  mutex.myserver_mutex_lock();
  return 0;
}

/*!
 *Terminate the access for the thread.
 */
int MYSERVER_LOG_MANAGER::terminateAccess()
{
  mutex.myserver_mutex_unlock();
  return 0;
}

/*!
 *Return the max size for the log.
 */
int MYSERVER_LOG_MANAGER::getMaxSize()
{
  return max_size;
}

/*!
 *Return the actual size for the log file.
 */
int MYSERVER_LOG_MANAGER::getLogSize()
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
