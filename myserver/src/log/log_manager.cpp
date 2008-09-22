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

void
LogManager::addLogStream (string& location, 
			  list<string>& filters, 
			  u_long cycleLog)
{
  mutex->lock ();
  if (!contains (location))
    {
      logStreams[location] = 
	logStreamFactory->createLogStream (filtersFactory,
					   location,
					   filters,
					   cycleLog);
    }
  mutex->unlock ();
}

void
LogManager::removeLogStream (string& location)
{
  mutex->lock ();
  if (contains (location))
    {
      delete logStreams[location];
      logStreams.erase (location);
    }
  mutex->unlock ();
}

LogStream*
LogManager::getLogStream (string& location)
{
  if (contains (location))
    return logStreams[location];
  return 0;
}

int
LogManager::notifyLogStreams (LogStreamEvent evt, 
			      void* message, 
			      void* reply)
{
  mutex->lock ();
  map<string, LogStream*>::iterator it;
  int retVal = 0;
  for (it = logStreams.begin (); it != logStreams.end (); it++)
    {
      retVal |= it->second->update (evt, message, reply);
    }
  mutex->unlock ();
  return retVal;
}

int
LogManager::log (string& message, LoggingLevel level, string location)
{
  if (level >= this->level)
    {
      if (contains (location))
	{
	  return logStreams[location]->log (message);
	}
      else
	{
	  return notifyLogStreams (EVT_LOG, static_cast<void*>(&message));
	}
    }
  return 1;
}

int
LogManager::close (string location)
{
  if (contains (location))
    {
      return logStreams[location]->close ();
    }
  return notifyLogStreams (EVT_CLOSE);
}

void
LogManager::setCycleLog (u_long cycleLog, string location)
{
  if (contains (location))
    {
      logStreams[location]->setCycleLog (cycleLog);
    }
  else
    {
      notifyLogStreams (EVT_SET_CYCLE_LOG, static_cast<void*>(&cycleLog));
    }
}

LoggingLevel
LogManager::setLoggingLevel (LoggingLevel level)
{
  LoggingLevel oldLevel = level;
  this->level = level;
  return oldLevel;
}

u_long
LogManager::getCycleLog (string& location)
{
  if (contains (location))
    return logStreams[location]->getCycleLog ();
}

LoggingLevel
LogManager::getLoggingLevel ()
{
  return level;
}

void
LogManager::setLogStreamFactory (LogStreamFactory* logStreamFactory)
{
  this->logStreamFactory = logStreamFactory;
}

LogStreamFactory* 
LogManager::getLogStreamFactory ()
{
  return logStreamFactory;
}

void
LogManager::setFiltersFactory (FiltersFactory* filtersFactory)
{
  this->filtersFactory = filtersFactory;
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
  map<string, LogStream*>::iterator it;
  for (it = logStreams.begin (); it != logStreams.end (); it++)
    delete it->second;
  logStreams.clear ();
}

bool
LogManager::empty ()
{
  return size () == 0;
}

bool
LogManager::contains (string& location)
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
  string location ("console://");
  list<string> filters;
  addLogStream (location, filters, maxSize);
}

int
LogManager::load (const char *filename)
{
  setType (TYPE_FILE);
  clear ();
  string location ("file://");
  location.append (filename);
  list<string> filters;
  addLogStream (location, filters, maxSize);
  return 0;
}

int
LogManager::write (const char *str, int len)
{
  string message (str);
  return notifyLogStreams (EVT_LOG, static_cast<void*>(&message));
}

int LogManager::getLogSize ()
{
  if (getFile ())
    return getFile ()->getFileSize ();
  return 0;
}

void
LogManager::setCycleLog (u_long cycleLog)
{
  cycleLog = 1;
  notifyLogStreams (EVT_SET_CYCLE_LOG, static_cast<void*>(&maxSize));
}

File*
LogManager::getFile ()
{
  map<string, LogStream*>::iterator it;
  for (it = logStreams.begin (); it != logStreams.end (); it++)
    {
      if (it->first.find ("file://") != string::npos)
	{
	  return dynamic_cast<File*>(it->second->getOutStream ());
	}
    }
  return 0;
}

int 
LogManager::writeln (const char *str)
{
  int ret = write (str);
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
  gzipLog = useGzip;
  if (gzipLog)
    notifyLogStreams (EVT_ADD_FILTER, new Gzip ());
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
