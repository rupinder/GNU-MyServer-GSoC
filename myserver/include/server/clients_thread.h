/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009 Free Software Foundation, Inc.
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

#ifndef CLIENTS_THREAD_H
# define CLIENTS_THREAD_H
# include "stdafx.h"
# include <include/base/utility.h>
# include <include/connection/connection.h>
# include <include/base/mem_buff/mem_buff.h>

class Server;

class  ClientsThread
{
  friend class Server;

  friend DEFINE_THREAD (clients_thread, pParam);

public:
  enum RETURN_CODE
  {
    /*!
     *Delete the current connection from the connections pool.
     */
    DELETE_CONNECTION = 0,
    /*!
     *Keep the connection in the connections pool waiting for new data.
     */
    KEEP_CONNECTION = 1,
    /*!
     *The request present in the connection buffer is not complete, keep
     *data in the buffer and append to it.
     */
    INCOMPLETE_REQUEST = 2,
    /*!
    *The request present in the buffer is not complete, append to the buffer
    *and check before new data is present.
    */
    INCOMPLETE_REQUEST_NO_WAIT = 3
  };
  MemBuf *getBuffer ();
  MemBuf *getSecondaryBuffer ();
  ClientsThread (Server* server);
  ~ClientsThread ();
  void stop ();
  int getTimeout ();
  void setTimeout (int);
  bool isToDestroy ();
  void setToDestroy (bool);
  bool isStatic ();
  bool isBusy ();
  void setStatic (bool);
  int run ();
  ThreadID getThreadId (){return tid;}
  int join ();
  u_long getBufferSize () {return bufferSize;}

private:
  Server* server;
  ThreadID tid;
  bool toDestroy;
  int timeout;
  bool initialized;
  int staticThread;
  u_long id;
  bool busy;
  bool threadIsStopped;
  bool threadIsRunning;
  u_long bufferSize;
  bool isRunning ();
  bool isStopped ();
  MemBuf buffer;
  MemBuf secondaryBuffer;
  int controlConnections ();
  u_long nBytesToRead;
};

DEFINE_THREAD (clients_thread, pParam);

#endif
