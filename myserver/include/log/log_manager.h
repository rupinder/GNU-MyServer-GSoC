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

#ifndef LOG_MANAGER_H
#define LOG_MANAGER_H

#include <map>
#include <string>

#include <include/base/sync/mutex.h>
#include <include/filter/filters_factory.h>
#include <include/log/stream/log_stream.h>
#include <include/log/stream/log_stream_factory.h>

using namespace std;

enum LoggingLevel
  {
    MYSERVER_LOG_INFO,
    MYSERVER_LOG_WARNING,
    MYSERVER_LOG_ERROR
  };

class LogManager
{
public:
  LogManager (FiltersFactory* ff, LoggingLevel level = MYSERVER_LOG_WARNING);
  ~LogManager ();
  int add (void* owner, string type, string location, 
           list<string>& filters, u_long cycle);
  int remove (void* owner);
  int log (void* owner, string message, bool appendNL = false,
           LoggingLevel level = MYSERVER_LOG_WARNING);
  int log (void* owner, string type, string message, bool appendNL = false,
           LoggingLevel level = MYSERVER_LOG_WARNING);
  int log (void* owner, string type, string location, string message, 
           bool appendNL = false, LoggingLevel level = MYSERVER_LOG_WARNING);
  int close (void* owner);
  int close (void* owner, string type);
  int close (void* owner, string type, string location);
  int chown (void* owner, int uid, int gid);
  int chown (void* owner, string type, int uid, int gid);
  int chown (void* owner, string type, string location, int uid, int gid);
  int get (void* owner, list<string>* l);
  int get (void* owner, string type, list<string>* l);
  int get (void* owner, string type, string location, LogStream** ls);
  int setCycle (void* owner, u_long cycle);
  int setCycle (void* owner, string type, u_long cycle);
  int setCycle (void* owner, string type, string location, u_long cycle);
  int getCycle (string location, u_long* cycle);
  int getFilters (string location, list<string>* l);
  LoggingLevel setLevel (LoggingLevel level);
  LoggingLevel getLevel ();
  void setFiltersFactory (FiltersFactory* ff);
  FiltersFactory* getFiltersFactory ();
  bool empty ();
  bool contains (string location);
  bool contains (void* owner);
  bool contains (void* owner, string type);
  bool contains (void* owner, string type, string location);
  int count (void* owner);
  int count (void* owner, string type);
  int count (void* owner, string type, string location);
  int clear ();
protected:
  int notify (void* owner, string type, string location, LogStreamEvent evt, 
              void* msg = 0, void* reply = 0);
  int notify (void* owner, string type, LogStreamEvent evt, void* msg = 0, 
              void* reply = 0);
  int notify (void* owner, LogStreamEvent evt, void* msg = 0, void* reply = 0);
  int add (void* owner);
  int add (void* owner, string type);
  int add (void* owner, string type, string location, LogStream* ls);
private:
  LoggingLevel level;
  Mutex* mutex;
  LogStreamFactory* lsf;
  FiltersFactory* ff;
  map<string, LogStream*> logStreams;
  map<void*, map<string, map<string, LogStream*> > > owners;
};

#endif
