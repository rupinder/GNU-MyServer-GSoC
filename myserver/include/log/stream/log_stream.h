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


/*!
 * Defines some events of interest for LogStream objects.
 */
enum LogStreamEvent
{
  EVT_SET_CYCLE_LOG,
  EVT_LOG,
  EVT_CLOSE,
  EVT_ADD_FILTER
};


using namespace std;


class LogStream
{
 public:
  

  /*!
   * The constructor.
   * \param filtersFactory An instance of FiltersFactory.
   * \param cycleLog The cycleLog value.
   * \param outStream The Stream where this LogStream will
   * write the log messages.
   * \param filtersChain The messages will pass through a 
   * FiltersChain before being written to the Stream.
   */
  LogStream (FiltersFactory* filtersFactory,
             u_long cycleLog, 
             Stream* outStream,
             FiltersChain* filtersChain);


  /*!
   * \return 0 on success.
   */
  int addFilter (Filter* filter);
  

  /*!
   * \return 0 on success.
   */
  int removeFilter (Filter* filter);


  /*!
   * \return 0 on success.
   */
  int update (LogStreamEvent evt, void* message = 0, void* reply = 0);


  /*!
   * \return 0 on success.
   */
  int close ();


  /*!
   * \return 0 the close method was called.
   */
  int getIsOpened ();


  u_long getLatestWrittenBytes ();


  virtual u_long streamSize ();


  u_long getCycleLog ();


  Stream* getOutStream ();


  FiltersFactory const* getFiltersFactory ();


  FiltersChain* getFiltersChain ();


  void setCycleLog (u_long cycleLog);


  /*!
   * Check if we have reached the max allowed size for the log.
   * \return 0 if there is no need to cycle the log yet.
   */
  int needToCycle ();
  

  /*!
   * \return 0 on success.
   */
  virtual int log (string message);


  virtual ~LogStream ();


  list<string>& getCycledStreams ();


 protected:


  /*!
   * \return 0 on success.
   */
  virtual int streamCycle ();


  /*!
   * Flushes any remaining data in the FiltersChain, cycle the stream
   * and recreates all filters.
   * \return 0 on success.
   */
  int doCycle ();
  

  /*!
   * \return 0 on success.
   */
  int write (string message);


  /*!
   * \return 0 on success.
   */
  int resetFilters ();


  /*!
   * The latest number of bytes written to the Stream.
   */
  u_long nbw;
  

  /*!
   * Will be zero after a successful call to the close method.
   */
  int isOpened;
  

  /*!
   * A zero value means `never cycle'. A non-zero value establishes
   * the maximum size allowed for the LogStream's growth before 
   * cycling it.
   */
  u_long cycleLog;


  FiltersChain* filtersChain;


  Stream* outStream;


  FiltersFactory* filtersFactory;


  list<string> cycledStreams;
};

#endif
