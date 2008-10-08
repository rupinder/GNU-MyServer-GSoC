/* -*- mode: c++ -*- */
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

#ifndef LOG_STREAM_H
#define LOG_STREAM_H

#include <list>
#include <string>

#include <include/filter/filters_chain.h>
#include <include/filter/filters_factory.h>
#include <include/filter/stream.h>
#include <include/base/sync/mutex.h>

using namespace std;

enum LogStreamEvent
  {
    EVT_SET_CYCLE,
    EVT_LOG,
    EVT_CLOSE,
    EVT_ADD_FILTER,
    EVT_CHOWN,
    EVT_ENTER_ERROR_MODE,
    EVT_EXIT_ERROR_MODE
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
  virtual int log (string message);
  virtual u_long streamSize ();
  virtual int chown (int uid, int gid);
  virtual int enterErrorMode ();
  virtual int exitErrorMode ();
  virtual ~LogStream ();
protected:
  virtual int streamCycle ();
  int doCycle ();
  int write (string message);
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
