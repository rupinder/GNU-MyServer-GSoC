/*
  MyServer
  Copyright (C) 2006, 2008 Free Software Foundation, Inc.
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

#include <include/log/log_manager.h>

int const LogManager::TYPE_CONSOLE = 1;
int const LogManager::TYPE_FILE = 2;

LogManager::LogManager (FiltersFactory* filtersFactory,
                        LogStreamFactory* logStreamFactory,
                        LoggingLevel level) : level (level)
{
  this->filtersFactory = filtersFactory;
  this->logStreamFactory = logStreamFactory;
  mutex = new Mutex ();
  mutex->init ();
}

LogManager::~LogManager ()
{
  if (!empty ())
    clear ();
  delete mutex;
}

int
LogManager::addLogStream (string location, 
                          list<string>& filters, 
                          u_long cycleLog)
{
  mutex->lock ();
  int retVal = 1;
  if (!contains (location))
    {
      LogStream* logStream = logStreamFactory->createLogStream (filtersFactory,
                                                                location,
                                                                filters,
                                                                cycleLog);
      if (logStream)
        {
          logStreams[location] = logStream;
          retVal = 0;
        }
    }
  mutex->unlock ();
  return retVal;
}

int
LogManager::removeLogStream (string location)
{
  mutex->lock ();
  int retVal = 1;
  if (contains (location))
    {
      delete logStreams[location];
      logStreams.erase (location);
      retVal = 0;
    }
  mutex->unlock ();
  return retVal;
}

LogStream*
LogManager::getLogStream (string location)
{
  LogStream* retVal = 0;
  if (contains (location))
    retVal = logStreams[location];
  return retVal;
}

int
LogManager::notifyLogStreams (LogStreamEvent evt, 
                              void* message, 
                              void* reply)
{
  map<string, LogStream*>::iterator it;
  int retVal = 0;
  for (it = logStreams.begin (); it != logStreams.end (); it++)
    {
      retVal |= it->second->update (evt, message, reply);
    }
  return retVal;
}

int
LogManager::log (string message, LoggingLevel level, string location)
{
  mutex->lock ();
  int retVal = 1;
  if (level >= this->level)
    {
      if (contains (location))
        {
          retVal = logStreams[location]->log (message);
        }
      else if (!location.compare ("all"))
        {
          retVal = notifyLogStreams (EVT_LOG, static_cast<void*>(&message));
        }
    }
  mutex->unlock ();
  return retVal;
}

int
LogManager::close (string location)
{
  mutex->lock ();
  int retVal = 1;
  if (contains (location))
    {
      retVal = logStreams[location]->close ();
    }
  else if (!location.compare ("all"))
    {
      retVal = notifyLogStreams (EVT_CLOSE);
    }
  mutex->unlock ();
  return retVal;
}

void
LogManager::setCycleLog (u_long cycleLog, string location)
{
  mutex->lock ();
  if (contains (location))
    {
      logStreams[location]->setCycleLog (cycleLog);
    }
  else if (!location.compare ("all"))
    {
      notifyLogStreams (EVT_SET_CYCLE_LOG, static_cast<void*>(&cycleLog));
    }
  mutex->unlock ();
}

LoggingLevel
LogManager::setLoggingLevel (LoggingLevel level)
{
  mutex->lock ();
  LoggingLevel oldLevel = level;
  this->level = level;
  mutex->unlock ();
  return oldLevel;
}

u_long
LogManager::getCycleLog (string location)
{
  u_long retVal = -1;
  if (contains (location))
    retVal = logStreams[location]->getCycleLog ();
  return retVal;
}

LoggingLevel
LogManager::getLoggingLevel ()
{
  return level;
}

void
LogManager::setLogStreamFactory (LogStreamFactory* logStreamFactory)
{
  mutex->lock ();
  this->logStreamFactory = logStreamFactory;
  mutex->unlock ();
}

LogStreamFactory* 
LogManager::getLogStreamFactory ()
{
  return logStreamFactory;
}

void
LogManager::setFiltersFactory (FiltersFactory* filtersFactory)
{
  mutex->lock ();
  this->filtersFactory = filtersFactory;
  mutex->unlock ();
}

FiltersFactory* 
LogManager::getFiltersFactory ()
{
  return filtersFactory;
}

int
LogManager::size ()
{
  return logStreams.size ();
}

void
LogManager::clear ()
{
  mutex->lock ();
  map<string, LogStream*>::iterator it;
  for (it = logStreams.begin (); it != logStreams.end (); it++)
    delete it->second;
  logStreams.clear ();
  mutex->unlock ();
}

bool
LogManager::empty ()
{
  return size () == 0;
}

bool
LogManager::contains (string location)
{
  return logStreams.count (location) > 0;
}

/*
 * *******************************************************************
 *
 *               D   E   P   R   E   C   A   T   E   D
 *
 * *******************************************************************
 */

LogManager::LogManager () : level (WARNING)
{
  mutex = new Mutex ();
  this->filtersFactory = new FiltersFactory ();
  this->filtersFactory->insert ("gzip", Gzip::factory);
  logStreamFactory = new LogStreamFactory ();
  mutex->init ();
  type = TYPE_CONSOLE;
  maxSize = 0;
  gzipLog = 1;
  cycleLog = 0;
  list<string> filters;
  addLogStream ("console://", filters, maxSize);
}

int
LogManager::load (const char *filename)
{
  setType (TYPE_FILE);
  removeLogStream ("console://");
  string location ("file://");
  location.append (filename);
  location.append (".gz");
  list<string> filters;
  /* 
     we use the gzip compression by default, for now.
  */
  filters.push_back ("gzip");
  addLogStream (location, filters, maxSize);
  return 0;
}

int
LogManager::write (string message, int len)
{
  return log (message);
}

int LogManager::getLogSize ()
{
  return 0;
}

void
LogManager::setCycleLog (u_long l)
{
  cycleLog = l;
  setCycleLog (maxSize, "all");
}

File*
LogManager::getFile ()
{
  return 0;
}

int 
LogManager::writeln (string message)
{
  int ret = write (message);
#ifdef WIN32
  if (ret == 0)
    ret = write ("\r\n");
#else
  if (ret == 0)
    ret = write ("\n");
#endif
  return ret;
}

u_long LogManager::getMaxSize()
{
  return maxSize;
}

u_long
LogManager::setMaxSize (u_long nMax)
{
  u_long old = maxSize;
  maxSize = nMax;
  setCycleLog (cycleLog);
  return old;
}

void 
LogManager::setGzip (int useGzip)
{
  
}

int
LogManager::getType ()
{
  return type;
}

void
LogManager::setType (int nType)
{
  type = nType;
}

int
LogManager::preparePrintError ()
{
  return 0;
}

int
LogManager::endPrintError ()
{
  return 0;
}

int LogManager::requestAccess ()
{
  return 0;
}

int LogManager::terminateAccess ()
{
  return 0;
}

int 
LogManager::getGzip ()
{
  return gzipLog;
}

int
LogManager::storeFile()
{
  return 0;
}
