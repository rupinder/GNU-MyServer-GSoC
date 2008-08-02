/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006, 2007, 2008 Free Software Foundation, Inc.
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

#include "../stdafx.h"
#include "../include/clients_thread.h"
#include "../include/server.h"
#include "../include/security.h"
#include "../include/http_request.h"
#include "../include/http_response.h"
#include "../include/socket.h"
#include "../include/stringutils.h"
#include "../include/mem_buff.h"
#include "../include/ftp.h"

#ifdef NOT_WIN
extern "C" {
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include <sys/types.h>
#include <sys/wait.h>

}
#endif

/*!
 *Construct the object.
 */
ClientsThread::ClientsThread()
{
  busy = 0;
  initialized = 0;
  toDestroy = 0;
  staticThread = 0;
  nBytesToRead = 0;
  httpParser = 0;
  httpsParser = 0;
  controlProtocolParser = 0;
  ftpParser = 0;
}

/*!
 *Destroy the ClientsThread object.
 */
ClientsThread::~ClientsThread()
{
  if(initialized == 0)
    return;
  threadIsRunning = 0;

  if(httpParser)
    delete httpParser;

  if(httpsParser)
    delete httpsParser;

  if(controlProtocolParser)
    delete controlProtocolParser;

  if ( ftpParser != NULL )
    delete ftpParser;

  httpParser = 0;
  httpsParser = 0;
  controlProtocolParser = 0;

  buffer.free();
  buffer2.free();
}

/*!
 *Get the timeout value.
 */
int ClientsThread::getTimeout()
{
  return timeout;
}

/*!
 *Set the timeout value for the thread.
 *\param newTimeout The new timeout value.
 */
void ClientsThread::setTimeout(int newTimeout)
{
  timeout = newTimeout;
}

/*!
 *This function starts a new thread controlled by a ClientsThread 
 *class instance.
 *\param pParam Params to pass to the new thread.
 */
#ifdef WIN32
#define ClientsThread_TYPE int
unsigned int __stdcall clients_thread(void* pParam)
#endif

#ifdef HAVE_PTHREAD
#define ClientsThread_TYPE void*
void* clients_thread(void* pParam)
#endif

{
  Server* server = Server::getInstance();

#ifdef NOT_WIN
  /* Block SigTerm, SigInt, and SigPipe in threads.  */
  sigset_t sigmask;
  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGPIPE);
  sigaddset(&sigmask, SIGINT);
  sigaddset(&sigmask, SIGTERM);
  sigprocmask(SIG_SETMASK, &sigmask, NULL);
#endif
  ClientsThread *ct = (ClientsThread*)pParam;

  /* Return an error if the thread is initialized.  */
  if(ct->initialized)
#ifdef WIN32
    return 1;
#endif
#ifdef HAVE_PTHREAD
  return (void*)1;
#endif

  ct->threadIsRunning = 1;
  ct->threadIsStopped = 0;
  ct->buffersize = server->getInstance()->getBuffersize();
  ct->buffersize2 = server->getInstance()->getBuffersize2();
  
  ct->buffer.setLength(ct->buffersize);
  ct->buffer.m_nSizeLimit = ct->buffersize;
  ct->buffer2.setLength(ct->buffersize2);
  ct->buffer2.m_nSizeLimit = ct->buffersize2;

  /* Built-in protocols will be initialized at the first use.  */
  ct->httpParser = 0;
  ct->httpsParser = 0;
  ct->controlProtocolParser = 0;

  ct->initialized = 1;

  Server::getInstance()->increaseFreeThread();


  /* Wait that the server is ready before go in the running loop.  */
  while(!server->isServerReady() && ct->threadIsRunning)
  {
    Thread::wait(500);
  }

  /*
   *This function when is alive only call the controlConnections(...) function
   *of the ClientsThread class instance used for control the thread.
   */
  while(ct->threadIsRunning) 
  {
    int ret;
    try
    {
      /*
       *If the thread can be destroyed don't use it.
       */
      if((!ct->isStatic()) && ct->isToDestroy())
      {
        Thread::wait(1);
        continue;
      }

      ret = ct->controlConnections();
      Server::getInstance()->increaseFreeThread();
      ct->busy = 0;

      /*
       *The thread served the connection, so update the timeout value.
       */
      if(ret != 1)
      {
        ct->setTimeout(getTicks());
      }
    }
    catch( bad_alloc &ba)
    {
      ostringstream s;
      s << "Bad alloc: " << ba.what();
      
      Server::getInstance()->logPreparePrintError();
      Server::getInstance()->logWriteln(s.str().c_str());
      Server::getInstance()->logEndPrintError();
    }
    catch( exception &e)
    {
      ostringstream s;
      s << "Error: " << e.what();

      Server::getInstance()->logPreparePrintError();
      Server::getInstance()->logWriteln(s.str().c_str());
      Server::getInstance()->logEndPrintError();
    };
    
  }
  Server::getInstance()->decreaseFreeThread();

  delete ct;

  Thread::terminate();
  return 0;
}

/*!
 *Join the thread.
 *
 */
int ClientsThread::join()
{
  return Thread::join(tid);
}


/*!
 *Create the new thread.
 */
int ClientsThread::run()
{
  tid = 0;
  return Thread::create(&tid, &::clients_thread,
                        (void *)this);
}

/*!
 *Returns if the thread can be destroyed.
 */
int ClientsThread::isToDestroy()
{
  return toDestroy;
}

/*!
 *Check if the thread is a static one.
 */
int ClientsThread::isStatic()
{
  return staticThread;
}

/*!
 *Set the thread to be static.
 *\param value The new static value.
 */
void ClientsThread::setStatic(int value)
{
  staticThread = value;
}

/*!
 *Set if the thread can be destroyed.
 *\param value The new destroy value.
 */
void ClientsThread::setToDestroy(int value)
{
  toDestroy = value;
}

/*!
 *This is the main loop of the thread.
 *Here are controlled all the connections that belongs to the 
 *ClientsThread class instance.
 *Every connection is controlled by its protocol.
 *Return 1 if no connections to serve are available.
 *Return 0 in all other cases.
 */
int ClientsThread::controlConnections()
{
  /*
   *Control the protocol used by the connection.
   */
  int retcode = 0;
  int err = 0;
  ConnectionPtr c;
  Protocol* protocol = 0;
  u_long dataRead = 0;

  c = Server::getInstance()->getConnection(this->id);
  Server::getInstance()->decreaseFreeThread();


  /*
   *Check if c is a valid connection structure.
   */
  if(!c)
    return 1;

  busy = 1;
  dataRead = c->connectionBuffer.getLength();

  err = c->socket->recv(&((char*)(buffer.getBuffer()))[dataRead],
                        MYSERVER_KB(8) - dataRead - 1, 0);

  if(err == -1 && !Server::getInstance()->deleteConnection(c, this->id))
    return 0;

  buffer.setLength(dataRead + err);    

  c->setForceControl(0);


  /* Refresh with the right value.  */
  nBytesToRead = dataRead + err;

  if((dataRead + err) < MYSERVER_KB(8))
  {
    ((char*)buffer.getBuffer())[dataRead + err] = '\0';
  }
  else
  {
    Server::getInstance()->deleteConnection(c, this->id);
    return 0;
  }


  if(getTicks() - c->getTimeout() > 5000)
    c->setnTries(0);

  if(dataRead)
  {
    memcpy((char*)buffer.getBuffer(), c->connectionBuffer, dataRead);
  }

  c->setActiveThread(this);
  try
  {
    protocol = Server::getInstance()->getProtocol(c->host->getProtocolName());
    if(protocol)
    {
      retcode = protocol->controlConnection(c, (char*)buffer.getBuffer(), 
                                            (char*)buffer2.getBuffer(), buffer.getRealLength(), 
                                            buffer2.getRealLength(), nBytesToRead, id);
    }
    else
    {
      retcode = DELETE_CONNECTION;
    }

  }
  catch(...)
  {
    retcode = DELETE_CONNECTION;
  };

  /*! Delete the connection.  */
  if(retcode == DELETE_CONNECTION)
  {
    Server::getInstance()->deleteConnection(c, this->id);
    return 0;
  }
  /*! Keep the connection.  */
  else if(retcode == KEEP_CONNECTION)
  {
    c->connectionBuffer.setLength(0);
    Server::getInstance()->getConnectionsScheduler()->addWaitingConnection(c);
  }
  /*! Incomplete request to buffer.  */
  else if(retcode == INCOMPLETE_REQUEST)
  {
    /*
     *If the header is incomplete save the current received
     *data in the connection buffer.
     *Save the header in the connection buffer.
     */
    c->connectionBuffer.setBuffer(buffer.getBuffer(), nBytesToRead);
    Server::getInstance()->getConnectionsScheduler()->addWaitingConnection(c);
  }
  /* Incomplete request to check before new data is available.  */
  else if(retcode == INCOMPLETE_REQUEST_NO_WAIT)
  {
    c->setForceControl(1);
    Server::getInstance()->getConnectionsScheduler()->addReadyConnection(c);
  }    

  c->setTimeout( getTicks() );

  return 0;
}

/*!
 *Stop the thread.
 */
void ClientsThread::stop()
{
  /*
   *Set the run flag to False.
   *When the current thread find the threadIsRunning
   *flag setted to 0 automatically destroy the
   *thread.
   */
  threadIsRunning = 0;
}

/*!
 *Returns a non-null value if the thread is active.
 */
int ClientsThread::isRunning()
{
  return threadIsRunning;
}

/*!
 *Returns 1 if the thread is stopped.
 */
int ClientsThread::isStopped()
{
  return threadIsStopped;
}

/*!
 *Get a pointer to the buffer.
 */
MemBuf* ClientsThread::getBuffer()
{
  return &buffer;
}
/*!
 *Get a pointer to the buffer2.
 */
MemBuf *ClientsThread::getBuffer2()
{
  return &buffer2;
}

/*!
 *Check if the thread is working.
 */
int ClientsThread::isBusy()
{
  return busy;
}
