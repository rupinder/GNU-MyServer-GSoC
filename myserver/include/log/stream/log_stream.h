/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2006, 2008, 2009, 2010 Free Software Foundation, Inc.
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

#ifndef LOG_STREAM_H
# define LOG_STREAM_H

# include "stdafx.h"

# include <list>
# include <string>

# include <include/filter/filters_chain.h>
# include <include/filter/filters_factory.h>
# include <include/filter/stream.h>
# include <include/base/sync/mutex.h>

using namespace std;

enum LoggingLevel
  {
    /* The MYSERVER_LOG_MSG_PLAIN is only used within the
     * LogManager class to print new lines with normal text
     * attributes over the ConsoleStream.
     */
    MYSERVER_LOG_MSG_PLAIN,
    MYSERVER_LOG_MSG_INFO,
    MYSERVER_LOG_MSG_WARNING,
    MYSERVER_LOG_MSG_ERROR
  };

enum LogStreamEvent
  {
    MYSERVER_LOG_EVT_SET_CYCLE,
    MYSERVER_LOG_EVT_LOG,
    MYSERVER_LOG_EVT_CLOSE,
    MYSERVER_LOG_EVT_ADD_FILTER,
    MYSERVER_LOG_EVT_CHOWN,
    MYSERVER_LOG_EVT_SET_MODE
  };

class LogStream
{
public:
  LogStream (FiltersFactory* ff, u_long cycle, Stream* out, FiltersChain* fc);
  int addFilter (Filter* filter);
  int removeFilter (Filter* filter);
  int update (LogStreamEvent evt, void* message = 0, void* reply = 0);
  int close ();
  int getIsOpened ();
  u_long getLatestWrittenBytes ();
  u_long getCycle ();
  Stream* getOutStream ();
  FiltersFactory const* getFiltersFactory ();
  FiltersChain* getFiltersChain ();
  int setCycle (u_long cycle);
  int needToCycle ();
  list<string>& getCycledStreams ();
  virtual int log (const string & message);
  virtual u_long streamSize ();
  virtual int chown (int uid, int gid);
  virtual int setMode (LoggingLevel level);
  virtual ~LogStream ();
protected:
  virtual int streamCycle ();
  int doCycle ();
  int write (const string &message);
  int resetFilters ();
  u_long nbw;
  u_long cycle;
  int isOpened;
  FiltersChain* fc;
  Stream* out;
  FiltersFactory* ff;
  list<string> cycledStreams;
  Mutex* mutex;
};

#endif
