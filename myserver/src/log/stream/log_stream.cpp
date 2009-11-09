/*
  MyServer
  Copyright (C) 2006, 2008, 2009 Free Software Foundation, Inc.
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
#include <include/log/stream/log_stream.h>

LogStream::LogStream (FiltersFactory* ff, u_long cycle, Stream* out,
                      FiltersChain* fc) : cycle (cycle), isOpened (1)
{
  this->ff = ff;
  this->out = out;
  this->fc = fc;
  mutex = new Mutex ();
  mutex->init ();
}

LogStream::~LogStream ()
{
  if (isOpened)
    close ();
  fc->clearAllFilters ();
  delete out;
  delete fc;
  delete mutex;
}

int
LogStream::resetFilters ()
{
  list<string> filters (fc->getFilters ());
  fc->clearAllFilters ();
  return ff->chain (fc, filters, out, &nbw);
}

int
LogStream::log (const string & message)
{
  mutex->lock ();
  int success = 0;
  if (needToCycle ())
    {
      success = doCycle () || write (message);
    }
  else
    {
      success = write (message);
    }
  mutex->unlock ();
  return success;
}

int
LogStream::needToCycle ()
{
  return cycle && (streamSize () >= cycle);
}

int
LogStream::doCycle ()
{
  return fc->flush (&nbw) || streamCycle () || resetFilters ();
}

int
LogStream::write (const string &message)
{
  return fc->write (message.c_str (), message.length (), &nbw);
}

int
LogStream::setCycle (u_long cycle)
{
  mutex->lock ();
  this->cycle = cycle;
  mutex->unlock ();
  return 0;
}

u_long
LogStream::getCycle ()
{
  return cycle;
}

Stream*
LogStream::getOutStream ()
{
  return out;
}

FiltersChain*
LogStream::getFiltersChain ()
{
  return fc;
}

int
LogStream::close ()
{
  mutex->lock ();
  int success = 1;
  isOpened = fc->flush (&nbw) || out->close ();
  if (!isOpened)
    {
      success = 0;
    }
  mutex->unlock ();
  return success;
}

int
LogStream::update (LogStreamEvent evt, void* message, void* reply)
{
  switch (evt)
    {
    case MYSERVER_LOG_EVT_SET_CYCLE:
      return !isOpened || setCycle (*static_cast<u_long*>(message));

    case MYSERVER_LOG_EVT_LOG:
      return !isOpened || log (*static_cast<string*>(message));

    case MYSERVER_LOG_EVT_CLOSE:
      return !isOpened || close ();

    case MYSERVER_LOG_EVT_ADD_FILTER:
      return !isOpened
        || addFilter (static_cast<Filter*>(message));

    case MYSERVER_LOG_EVT_CHOWN:
        return !isOpened || chown (static_cast<int*>(message)[0],
                                   static_cast<int*>(message)[1]);

    case MYSERVER_LOG_EVT_SET_MODE:
        return !isOpened
          || setMode (*static_cast<LoggingLevel*>(message));

    }

  return 1;
}

int
LogStream::addFilter (Filter* filter)
{
  mutex->lock ();
  int success = fc->addFilter (filter, &nbw);
  mutex->unlock ();
  return success;
}

int
LogStream::removeFilter (Filter* filter)
{
  mutex->lock ();
  int success = fc->removeFilter (filter);
  mutex->unlock ();
  return success;
}

FiltersFactory const*
LogStream::getFiltersFactory ()
{
  return ff;
}

u_long
LogStream::streamSize ()
{
  return 0;
}

int
LogStream::streamCycle ()
{
  return 0;
}

int
LogStream::getIsOpened ()
{
  return isOpened;
}

u_long
LogStream::getLatestWrittenBytes ()
{
  return nbw;
}

list<string>&
LogStream::getCycledStreams ()
{
  return cycledStreams;
}

int
LogStream::chown (int uid, int gid)
{
  return 0;
}

int
LogStream::setMode (LoggingLevel level)
{
  return 0;
}
