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

#include <include/log/stream/log_stream.h>

LogStream::LogStream (FiltersFactory* filtersFactory, 
		      u_long cycleLog,
		      Stream* outStream,
		      FiltersChain* filtersChain) :
cycleLog (cycleLog), isOpened (1)
{
  this->filtersFactory = filtersFactory;
  this->outStream = outStream;
  this->filtersChain = filtersChain;
}

LogStream::~LogStream ()
{
  if (isOpened)
    close ();
  filtersChain->clearAllFilters ();
  delete outStream;
  delete filtersChain;
}

int
LogStream::resetFilters ()
{
  list<string> filters (filtersChain->getFilters ());
  filtersChain->clearAllFilters ();
  return filtersFactory->chain (filtersChain, filters, outStream, &nbw);
}

int
LogStream::log (string& message)
{
  if (needToCycle ())
    {
      return doCycle () || write (message);
    }
  return write (message);
}

int
LogStream::needToCycle ()
{
  return cycleLog && (streamSize () >= cycleLog);
}

int
LogStream::doCycle ()
{
  return filtersChain->flush (&nbw) || streamCycle () || resetFilters ();
}

int
LogStream::write (string& message)
{
  return filtersChain->write (message.c_str (), message.size (), &nbw);
}

void
LogStream::setCycleLog (u_long cycleLog)
{
  this->cycleLog = cycleLog;
}

u_long
LogStream::getCycleLog ()
{
  return cycleLog;
}

Stream*
LogStream::getOutStream ()
{
  return outStream;
}

FiltersChain*
LogStream::getFiltersChain ()
{
  return filtersChain;
}

int
LogStream::close ()
{
  return isOpened = (filtersChain->flush (&nbw) || outStream->close ());
}

int
LogStream::update (LogStreamEvent evt, void* message, void* reply)
{
  switch (evt)
    {
    case EVT_SET_CYCLE_LOG:
      {
	setCycleLog (*static_cast<u_long*>(message));
	return 0;
      }
      break;
    case EVT_LOG:
      {
	return log (*static_cast<string*>(message));
      }
      break;
    case EVT_CLOSE:
      {
	return close ();
      }
      break;
    case EVT_ADD_FILTER:
      {
	return addFilter (static_cast<Filter*>(message));
      }
      break;
    default:
      return 1;
    }
}

int
LogStream::addFilter (Filter* filter)
{
  return filtersChain->addFilter (filter, &nbw);
}

int
LogStream::removeFilter (Filter* filter)
{
  return filtersChain->removeFilter (filter);
}

FiltersFactory const*
LogStream::getFiltersFactory ()
{
  return filtersFactory;
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
