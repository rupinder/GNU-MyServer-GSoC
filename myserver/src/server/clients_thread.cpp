/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006, 2007, 2008, 2009, 2010 Free
Software Foundation, Inc.
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

#include "stdafx.h"
#include <include/server/clients_thread.h>

#include <include/base/thread/thread.h>
#include <include/server/server.h>
#include <include/base/socket/socket.h>
#include <include/base/string/stringutils.h>

#include <include/protocol/http/http.h>
#include <include/base/mem_buff/mem_buff.h>
#include <include/protocol/https/https.h>
#include <include/protocol/control/control_protocol.h>
#include <include/protocol/ftp/ftp.h>

#ifndef WIN32
extern "C"
{
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <string.h>
# include <signal.h>
# include <unistd.h>
# include <time.h>
# ifdef HAVE_PTHREAD
#  include <pthread.h>
# endif
# include <sys/types.h>
# include <sys/wait.h>
}
#endif

/*!
 * Construct the object.
 */
ClientsThread::ClientsThread (Server* server)
{
  this->server = server;
  busy = false;
  initialized = false;
  toDestroy = false;
  staticThread = 0;
  nBytesToRead = 0;
}

/*!
 *Destroy the ClientsThread object.
 */
ClientsThread::~ClientsThread ()
{
  if (!initialized)
    return;

  threadIsRunning = false;

  buffer.free ();
  auxiliaryBuffer.free ();
}

/*!
 * Get the timeout value.
 */
int ClientsThread::getTimeout ()
{
  return timeout;
}

/*!
 * Set the timeout value for the thread.
 * \param newTimeout The new timeout value.
 */
void ClientsThread::setTimeout (int newTimeout)
{
  timeout = newTimeout;
}

/*!
 * This function starts a new thread controlled by a ClientsThread
 * class instance.
 * \param pParam Params to pass to the new thread.
 */
DEFINE_THREAD (clients_thread, pParam)
{
#ifndef WIN32
  /* Block SigTerm, SigInt, and SigPipe in threads.  */
  sigset_t sigmask;
  sigemptyset (&sigmask);
  sigaddset (&sigmask, SIGPIPE);
  sigaddset (&sigmask, SIGINT);
  sigaddset (&sigmask, SIGTERM);
  sigprocmask (SIG_SETMASK, &sigmask, NULL);
#endif
  ClientsThread *ct = (ClientsThread*)pParam;

  /* Return an error if the thread is initialized.  */
  if (ct->initialized)
#ifdef WIN32
    return 1;
#endif
#ifdef HAVE_PTHREAD
  return (void*)1;
#endif

  ct->threadIsRunning = true;
  ct->threadIsStopped = false;
  ct->bufferSize = ct->server->getBuffersize ();

  ct->buffer.setRealLength (ct->bufferSize);
  ct->buffer.setSizeLimit (ct->bufferSize);

  ct->auxiliaryBuffer.setRealLength (ct->bufferSize);
  ct->auxiliaryBuffer.setSizeLimit (ct->bufferSize);

  /* Built-in protocols will be initialized at the first use.  */
  ct->initialized = true;

  ct->server->increaseFreeThread ();

  /* Wait that the server is ready before go in the running loop.  */
  while (!ct->server->isServerReady () && ct->threadIsRunning)
    Thread::wait (500);

  /*
   * This function when is alive only call the controlConnections (...) function
   * of the ClientsThread class instance used for control the thread.
   */
  while (ct->threadIsRunning)
    {
      int ret;
      try
        {
          /*
           *If the thread can be destroyed don't use it.
           */
          if ((!ct->isStatic ()) && ct->isToDestroy ())
            {
              Thread::wait (1000);
              break;
            }

          ret = ct->controlConnections ();
          ct->server->increaseFreeThread ();
          ct->busy = false;

          /*
           *The thread served the connection, so update the timeout value.
           */
          if (ret != 1)
            ct->setTimeout (getTicks ());
        }
      catch (bad_alloc &ba)
        {
          ct->server->log (MYSERVER_LOG_MSG_ERROR, _("Bad alloc: %s"),
                                  ba.what ());
        }
      catch (exception &e)
        {
          ct->server->log (MYSERVER_LOG_MSG_ERROR, _("Error : %s"),
                                  e.what ());
        };
  }

  ct->server->decreaseFreeThread ();
  ct->threadIsStopped = true;

  Thread::terminate ();
  return 0;
}

/*!
 * Join the thread.
 */
int ClientsThread::join ()
{
  return Thread::join (tid);
}


/*!
 * Create the new thread.
 */
int ClientsThread::run ()
{
  tid = 0;
  return Thread::create (&tid, &::clients_thread,
                         (void *)this);
}

/*!
 * Returns if the thread can be destroyed.
 */
bool ClientsThread::isToDestroy ()
{
  return toDestroy;
}

/*!
 * Check if the thread is a static one.
 */
bool ClientsThread::isStatic ()
{
  return staticThread;
}

/*!
 * Set the thread to be static.
 * \param value The new static value.
 */
void ClientsThread::setStatic (bool value)
{
  staticThread = value;
}

/*!
 * Set if the thread can be destroyed.
 * \param value The new destroy value.
 */
void ClientsThread::setToDestroy (bool value)
{
  toDestroy = value;
}

/*!
 * This is the main loop of the thread.
 * Here are controlled all the connections that belongs to the
 * ClientsThread class instance.
 * Every connection is controlled by its protocol.
 *\return 1 if no connections to serve are available.
 *\return 0 in all other cases.
 */
int ClientsThread::controlConnections ()
{
  /* Control the protocol used by the connection.  */
  int retcode = 0;
  int err = 0;
  ConnectionPtr c;
  Protocol* protocol = 0;
  u_long dataRead = 0;

  c = server->getConnection (this->id);

  server->decreaseFreeThread ();

  if (!c)
    return 1;

  busy = true;
  dataRead = c->getConnectionBuffer ()->getLength ();

  err = c->socket->recv (&((char*)(buffer.getBuffer ()))[dataRead],
                        MYSERVER_KB (8) - dataRead - 1, 0);

  if (err == -1 && !server->deleteConnection (c))
    return 0;

  buffer.setRealLength (dataRead + err);
  c->setForceControl (0);

  /* Refresh with the right value.  */
  nBytesToRead = dataRead + err;

  if ((dataRead + err) < MYSERVER_KB (8))
    ((char*)buffer.getBuffer ())[dataRead + err] = '\0';
  else
    {
      server->deleteConnection (c);
      return 0;
    }

  if (getTicks () - c->getTimeout () > 5000)
    c->setnTries (0);

  if (dataRead)
    memcpy ((char*)buffer.getBuffer (), c->getConnectionBuffer ()->getBuffer (),
            dataRead);

  c->setActiveThread (this);
  try
  {
    if (c->hasContinuation ())
      {
        retcode = c->getContinuation ()(c, (char*)buffer.getBuffer (),
                                   (char*)auxiliaryBuffer.getBuffer (),
                                   buffer.getRealLength (),
                                   auxiliaryBuffer.getRealLength (),
                                   nBytesToRead, id);
        c->setContinuation (NULL);
      }
    else
      {
        protocol = server->getProtocol (c->host->getProtocolName ());
        if (protocol)
          retcode = protocol->controlConnection (c, (char*)buffer.getBuffer (),
                                           (char*)auxiliaryBuffer.getBuffer (),
                                           buffer.getRealLength (),
                                           auxiliaryBuffer.getRealLength (),
                                           nBytesToRead, id);
        else
          retcode = ClientsThread::DELETE_CONNECTION;
      }
  }
  catch (...)
    {
      retcode = DELETE_CONNECTION;
    };

  c->setTimeout (getTicks ());

  /* Delete the connection.  */
  if (retcode == DELETE_CONNECTION)
    {
      server->deleteConnection (c);
      return 0;
    }
  /* Keep the connection.  */
  else if (retcode == KEEP_CONNECTION)
    {
      c->getConnectionBuffer ()->setLength (0);
      server->getConnectionsScheduler ()->addWaitingConnection (c);
    }
  /* Incomplete request to buffer.  */
  else if (retcode == INCOMPLETE_REQUEST)
    {
      /*
       * If the header is incomplete save the current received
       * data in the connection buffer.
       * Save the header in the connection buffer.
       */
      c->getConnectionBuffer ()->setBuffer (buffer.getBuffer (), nBytesToRead);
      server->getConnectionsScheduler ()->addWaitingConnection (c);
    }
  /* Incomplete request to check before new data is available.  */
  else if (retcode == INCOMPLETE_REQUEST_NO_WAIT)
    {
      c->setForceControl (1);
      server->getConnectionsScheduler ()->addReadyConnection (c);
    }

  return 0;
}

/*!
 * Stop the thread.
 */
void ClientsThread::stop ()
{
  /*
   * Set the run flag to False.
   * When the current thread find the threadIsRunning
   * flag setted to 0 automatically destroy the
   * thread.
   */
  threadIsRunning = false;
}

/*!
 * Returns a non-null value if the thread is active.
 */
bool ClientsThread::isRunning ()
{
  return threadIsRunning;
}

/*!
 * Returns if the thread is stopped.
 */
bool ClientsThread::isStopped ()
{
  return threadIsStopped;
}

/*!
 * Get a pointer to the buffer.
 */
MemBuf* ClientsThread::getBuffer ()
{
  return &buffer;
}
/*!
 * Get a pointer to the secondary buffer.
 */
MemBuf *ClientsThread::getAuxiliaryBuffer ()
{
  return &auxiliaryBuffer;
}

/*!
 * Check if the thread is working.
 */
bool ClientsThread::isBusy ()
{
  return busy;
}
