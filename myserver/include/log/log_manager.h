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
#include <include/filter/gzip/gzip.h>

using namespace std;


/*!
 * Defines different types of logging level.
 */
enum LoggingLevel
  {
    INFO,
    WARNING,
    ERROR
  };


class LogManager
{
public:


  /*!
   * Default constructor. It is only used to support the old
   * interface.
   */
  LogManager ();
  

  LogManager (FiltersFactory* filtersFactory,
	      LogStreamFactory* logStreamFactory,
	      LoggingLevel level = WARNING);


  /*!
   * Deallocates all LogStream owned by the LogManager and
   * other objects (like the Mutex and the LogStreamFactory)
   * allocated by the LogManager.
   */
  ~LogManager ();


  /*!
   * Add a new LogStream to the LogManager. Prevents duplicate
   * LogStream being added.
   * \param location The location where the new LogStream will point.
   * \param filters A list of filters used within the new LogStream.
   * \param cycleLog A non-zero value establishes the threshold for
   * the LogStream's growth before cycling it.
   */
  void addLogStream (string& location, 
		     list<string>& filters, 
		     u_long cycleLog);


  /*!
   * Remove the LogStream that point to location if it exists.
   * \param location The location where the LogStream which has to
   * be removed points to.
   */
  void removeLogStream (string& location);


  LogStream* getLogStream (string& location);
  

  /*!
   * Write `message' to the LogStream that points to `location'. If
   * no location is provided, it writes the message on all LogStreams
   * owned by this LogManager.
   * \param message The message string to write.
   * \param level The message logging level. If it is lower than the
   * LogManager's logging level, the LogManager discards it.
   * \param location The location where `message' will be written.
   * \return 0 on success.
   */
  int log (string& message, 
	   LoggingLevel level = WARNING,
	   string location = "all");


  /*!
   * Close the LogStream that points to `location'. If no location
   * is provided, it closes all LogStreams owned by this LogManager.
   * \return 0 on success.
   */
  int close (string location = "all");


  /*!
   * \return The cycleLog value for the LogStream that points to
   * location.
   */
  u_long getCycleLog (string& location);


  /*!
   * Set the cycleLog field for the LogStream that points to `location'.
   * If no location is provided, it will set the cycleLog to the same value
   * for all LogStreams.
   * \param cycleLog The new cycleLog value.
   * \param location The LogStream whose cycleLog will be changed.
   */
  void setCycleLog (u_long cycleLog, 
		    string location);


  /*!
   * Set the default logging level.
   * \param level The new logging level value.
   * \return The old logging level.
   */
  LoggingLevel setLoggingLevel (LoggingLevel level);


  LoggingLevel getLoggingLevel ();


  void setLogStreamFactory (LogStreamFactory* logStreamFactory);


  LogStreamFactory* getLogStreamFactory ();


  void setFiltersFactory (FiltersFactory* filtersFactory);

  
  FiltersFactory* getFiltersFactory ();


  int size ();


  bool empty ();


  bool contains (string& location);

  
  /*!
   * Delete all LogStream objects owned by this LogManager.
   */
  void clear ();


  /*!
   * Deprecated methods and fields.
   */
  int getLogSize ();
  int storeFile ();
  int load (char const*);
  void setGzip (int);
  int write (char const*, int len = 0);
  int writeln (char const*);
  File* getFile ();
  u_long setMaxSize (u_long);
  u_long getMaxSize ();
  int getType ();
  int getGzip ();
  void setType (int);
  int preparePrintError ();
  int endPrintError ();
  int requestAccess ();
  int terminateAccess ();
  void setCycleLog (u_long);
  int const static TYPE_CONSOLE;
  int const static TYPE_FILE;
  int type;
  int gzipLog;
  int cycleLog;
  u_long maxSize;
protected:


  /*!
   * Send events to LogStreams.
   * \param evt The event that should be notified to LogStreams.
   * \param message The message to deliver.
   * \param reply The receiver will put its data here.
   * \return 0 on success.
   */
  int notifyLogStreams (LogStreamEvent evt,
			void* message = 0,
			void* reply = 0);
private:


  /*!
   * Hold the default logging level for the LogManager.
   */
  LoggingLevel level;


  /*!
   * Ensure atomic operations to prevent interleaving.
   */
  Mutex* mutex;


  /*!
   * The LogStream creator.
   */
  LogStreamFactory* logStreamFactory;


  /*!
   * The Filter creator.
   */
  FiltersFactory* filtersFactory;


  /*!
   * Hold all LogStream owned by this LogManager.
   */
  map<string, LogStream*> logStreams;
};

#endif
