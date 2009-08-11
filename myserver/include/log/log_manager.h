/* -*- mode: c++ -*- */
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

#ifndef LOG_MANAGER_H
#define LOG_MANAGER_H

#include <algorithm>
#include <map>
#include <string>

#include <include/base/sync/mutex.h>
#include <include/filter/filters_factory.h>
#include <include/log/stream/log_stream.h>
#include <include/log/stream/log_stream_factory.h>
#include <cstdarg>


using namespace std;

class LogManager
{
public:
  LogManager (FiltersFactory* ff, LoggingLevel level = MYSERVER_LOG_MSG_INFO);
  ~LogManager ();
  int add (void* owner, string type, string location,
           list<string>& filters, u_long cycle);
  int remove (void* owner);
  int log (void* owner, string message, bool appendNL = false,
           LoggingLevel level = MYSERVER_LOG_MSG_INFO);
  int log (void* owner, string type, string message, bool appendNL = false,
           LoggingLevel level = MYSERVER_LOG_MSG_INFO);
  int log (void* owner, string type, string location, string message,
           bool appendNL = false, LoggingLevel level = MYSERVER_LOG_MSG_INFO);
  int log (void* owner, string type, string location, LoggingLevel level,
           bool appendNL, const char *fmt, ...);
  int log (void* owner, string type, string location, LoggingLevel level,
           bool appendNL, const char *fmt, va_list args);

  int close (void* owner);
  int close (void* owner, string type);
  int close (void* owner, string type, string location);
  int chown (void* owner, string &uid, string &gid);
  int chown (void* owner, string type, string &uid, string &gid);
  int chown (void* owner, string type, string location, string &uid, string &gid);
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
  int getOwnersList (string, list<void*>*);
  list<string> getLoggingLevelsByNames ();
  map<LoggingLevel, string>& getLoggingLevels () { return loggingLevels; }
private:
  int notify (void* owner, string type, string location, LogStreamEvent evt,
              void* msg = 0, void* reply = 0);
  int notify (void* owner, string type, LogStreamEvent evt, void* msg = 0,
              void* reply = 0);
  int notify (void* owner, LogStreamEvent evt, void* msg = 0, void* reply = 0);
  int add (void* owner);
  int add (void* owner, string type);
  int add (void* owner, string type, string location, LogStream* ls);
  int computeNewLine ();
  void associateLoggingLevelsWithNames ();
  int logWriteln (string, LoggingLevel);

  LoggingLevel level;
  Mutex* mutex;
  LogStreamFactory* lsf;
  FiltersFactory* ff;

  /*!
   * Store all the LogStream objects, each identified through its location string.
   * There can't be two LogStream objects pointing to the same location,
   * anyway it is possible to share the same LogStream between more owners.
   */
  map<string, LogStream*> logStreams;

  /*!
   * For each LogStream, store the list of objects that use it.
   */
  map<string, list<void*> > logStreamOwners;

  /*!
   * For each owner, store the LogStream objects that it owns.
   */
  map<void*, map<string, map<string, LogStream*> > > owners;

  /*!
   * Store the newline string for the host operating system.
   */
  string newline;

  /*!
   * Associate each LoggingLevel with its string representation.
   */
  map<LoggingLevel, string> loggingLevels;
};

#endif
