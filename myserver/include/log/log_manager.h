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
# define LOG_MANAGER_H

# include <algorithm>
# include <map>
# include <string>

# include <include/base/sync/mutex.h>
# include <include/filter/filters_factory.h>
# include <include/log/stream/log_stream.h>
# include <include/log/stream/log_stream_factory.h>
# include <cstdarg>


using namespace std;

class LogManager
{
public:
  LogManager (FiltersFactory* ff, LoggingLevel level = MYSERVER_LOG_MSG_INFO);
  ~LogManager ();
  int add (const void *owner, string type, string location,
           list<string>& filters, u_long cycle);
  int remove (const void *owner);

  int log (const void* owner, string & message, bool appendNL = false,
           LoggingLevel level = MYSERVER_LOG_MSG_INFO);
  int log (const void* owner, const string & type, string & message,
           bool appendNL = false, LoggingLevel level = MYSERVER_LOG_MSG_INFO);
  int log (const void* owner, const string & type, const string & location,
           string & message, bool appendNL = false,
           LoggingLevel level = MYSERVER_LOG_MSG_INFO);
  int log (const void* owner, const string & type, LoggingLevel level,
           bool ts, bool appendNL, const char *fmt, ...);
  int log (const void* owner, const string & type, LoggingLevel level,
           bool ts, bool appendNL, const char *fmt, va_list args);

  int close (const void *owner);
  int close (const void *owner, string type);
  int close (const void *owner, string type, string location);
  int chown (const void *owner, string &uid, string &gid);
  int chown (const void *owner, string type, string &uid, string &gid);
  int chown (const void *owner, string type, string location, string &uid, string &gid);
  int get (const void *owner, list<string>* l);
  int get (const void *owner, string type, list<string>* l);
  int get (const void *owner, string type, string location, LogStream** ls);
  int setCycle (const void *owner, u_long cycle);
  int setCycle (const void *owner, string type, u_long cycle);
  int setCycle (const void *owner, string type, string location, u_long cycle);
  int getCycle (string location, u_long* cycle);
  int getFilters (string location, list<string>* l);
  LoggingLevel setLevel (LoggingLevel level);
  LoggingLevel getLevel ();
  void setFiltersFactory (FiltersFactory* ff);
  FiltersFactory* getFiltersFactory ();
  bool empty ();
  bool containsOwner (const void* owner);
  bool contains (const string & location);
  bool contains (const void* owner, const string & type);
  bool contains (const void* owner, const string & type, const string &
                 location);
  int count (const void* owner);
  int count (const void* owner, const string & type);
  int count (const void* owner, const string & type, const string & location);
  int clear ();
  int getOwnersList (string, list<const void*>*);
  list<string> getLoggingLevelsByNames ();
  map<LoggingLevel, string>& getLoggingLevels () { return loggingLevels; }
private:
  int notify (const void* owner, const string & type, const string & location,
              LogStreamEvent evt, void* msg = NULL, void* reply = NULL);
  int notify (const void* owner, const string & type, LogStreamEvent evt,
              void* msg = NULL, void* reply = NULL);
  int notify (const void* owner, LogStreamEvent evt, void* msg = 0,
              void* reply = NULL);
  int add (const void *owner);
  int add (const void *owner, string type);
  int add (const void *owner, string type, string location, LogStream* ls);
  int computeNewLine ();
  void associateLoggingLevelsWithNames ();

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
  map<string, list<const void*> > logStreamOwners;

  /*!
   * For each owner, store the LogStream objects that it owns.
   */
  map<const void*, map<string, map<string, LogStream*> > > owners;

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
