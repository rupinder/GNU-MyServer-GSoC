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

#ifndef LOG_STREAM_FACTORY_H
#define LOG_STREAM_FACTORY_H

#include <list>
#include <map>
#include <string>

#include <include/log/stream/log_stream.h>
#include <include/log/stream/console_stream_creator.h>
#include <include/log/stream/file_stream_creator.h>
#include <include/log/stream/socket_stream_creator.h>
#include <include/log/stream/log_stream_creator.h>

using namespace std;

class LogStreamFactory
{
public:


  /*!
   * This constructor initializes the logStreamCreators map. To
   * add more LogStreams, you have to create a new class implementing
   * the LogStreamCreator interface and then add it here providing
   * a valid protocol string (in the form protocol://) as key.
   */
  LogStreamFactory ();

  
  /*!
   * The destructor. Ensures that the memory allocated for
   * all the LogStreamCreator objects gets freed.
   */
  ~LogStreamFactory ();


  /*!
   * Factory method. Creates a new concrete LogStream.
   * \param filtersFactory Self explicating.
   * \param location This is the path where the LogStream will point to.
   * The location string must follow the syntax `protocol://path/to/log'.
   * The protocol:// part can't be missing, since it is required to get
   * the proper constructor for the new object that will be created.
   * \param filters A list of strings representing filters to be used within
   * the new LogStream.
   * \param cycleLog Whether the log should be cycled or not. 0 means never
   * cycle.
   * \return A new instance of one of LogStream subclasses.
   */
  LogStream* createLogStream (FiltersFactory* filtersFactory, 
			      string& location, 
			      list<string>& filters,
			      u_long cycleLog);


  /*!
   * A helper method useful to get the protocol part of a location string.
   * \param location The string representing a valid location as explained
   * in createLogStream method description.
   * \return A new string with the protocol part of the location.
   */
  string getProtocol (string& location);


  /*!
   * A helper method useful to get the path part of a location string.
   * \param location The string representing a valid location as explained
   * in createLogStream method description.
   * \return A new string with the path part of the location.
   */
  string getPath (string& location);

private:


  /*!
   * Holds all the constructors known by the LogStreamFactory.
   */
  map<string, LogStreamCreator*> logStreamCreators;
};

#endif
