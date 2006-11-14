/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006 The MyServer Team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "../stdafx.h"
#include "../include/clients_thread.h"
#include "../include/server.h"
#include "../include/security.h"
#include "../include/http_request.h"
#include "../include/http_response.h"
#include "../include/sockets.h"
#include "../include/stringutils.h"
#include "../include/mem_buff.h"

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
  parsing = 0;
	err = 0;
	initialized = 0;
  next = 0;
  toDestroy = 0;
  staticThread = 0;
  nBytesToRead = 0;
}

/*!
 *Destroy the ClientsThread object.
 */
ClientsThread::~ClientsThread()
{
	clean();
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
unsigned int __stdcall startClientsThread(void* pParam)
#endif

#ifdef HAVE_PTHREAD
#define ClientsThread_TYPE void*
void * startClientsThread(void* pParam)
#endif

{
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

  if(Server::getInstance()->getUid() && 
		 Process::setuid(Server::getInstance()->getUid()))
  {
    ostringstream out;
    out << Server::getInstance()->getLanguageParser()->getValue("ERR_ERROR") 
        << ": setuid " << Server::getInstance()->getUid();
    Server::getInstance()->logPreparePrintError();
    Server::getInstance()->logWriteln(out.str().c_str());
    Server::getInstance()->logEndPrintError();
    Thread::terminate();
    return 0;

  }	
  if(Server::getInstance()->getGid() && 
		 Process::setgid(Server::getInstance()->getGid()))
  {
    ostringstream out;
    out << Server::getInstance()->getLanguageParser()->getValue("ERR_ERROR")
        << ": setgid "  << Server::getInstance()->getGid();
    Server::getInstance()->logPreparePrintError();
    Server::getInstance()->logWriteln(out.str().c_str());
    Server::getInstance()->logEndPrintError();
    Thread::terminate();
    return 0;
  }	

	ct->threadIsRunning = 1;
	ct->threadIsStopped = 0;
	ct->buffersize = Server::getInstance()->getBuffersize();
	ct->buffersize2 = Server::getInstance()->getBuffersize2();
	
	ct->buffer.setLength(ct->buffersize);
	ct->buffer.m_nSizeLimit = ct->buffersize;
	ct->buffer2.setLength(ct->buffersize2);
	ct->buffer2.m_nSizeLimit = ct->buffersize2;

  /* Built-in protocols will be initialized at the first use.  */
  ct->httpParser = 0;
  ct->httpsParser = 0;
  ct->controlProtocolParser = 0;

	ct->initialized = 1;

  /* Reset first 1024 bytes for thread buffers.  */
	memset((char*)ct->buffer.getBuffer(), 0, 
         1024 > ct->buffer.getRealLength() ? 1024 
				                                   : ct->buffer.getRealLength());
	memset((char*)ct->buffer2.getBuffer(), 0, 
         1024 > ct->buffer2.getRealLength() ? 1024 
				                                    : ct->buffer2.getRealLength());

	/* Wait that the server is ready before go in the running loop.  */
  while(!Server::getInstance()->isServerReady())
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
      Thread::wait(1);
      /*
       *If the thread can be destroyed don't use it.
       */
      if((!ct->isStatic()) && ct->isToDestroy() )
      {
        continue;
      }
      
      ct->parsing = 1;
      ret = ct->controlConnections();

      /*
       *The thread served the connection, so update the timeout value.
       */
      if(ret != 1)
      {
        ct->setTimeout( getTicks() );
      }
      else
      {
        /*
         *Long inactive non static thread... Maybe we don't need it.
         */
        if(!ct->isStatic())
          if(getTicks() - ct->getTimeout() > MYSERVER_SEC(15) )
            ct->setToDestroy(1);
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
    
    ct->parsing = 0;
  }
  ct->threadIsStopped = 1;
  
  Thread::terminate();
  return 0;
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
  ConnectionPtr c;
  DynamicProtocol* dp = 0;
	
	/*
   *Get the access to the connections list.
   */
	Server::getInstance()->connectionsMutexLock();
	c = Server::getInstance()->getConnection(this->id);
	
	/*
   *Check if c exists.
   *Check if c is a valid connection structure.
   *Do not parse a connection that is going to be parsed by another thread.
   */
  if(!c)
  {
		Server::getInstance()->connectionsMutexUnlock();
    return 1;
  }

	if(c->isParsing())
	{
		Server::getInstance()->connectionsMutexUnlock();
		return 0;
	}

	/*
   *Unlock connections list access after setting parsing flag.
   */
	Server::getInstance()->connectionsMutexUnlock();

  /* Number of bytes waiting to be read.  */
  if(c->socket.dataOnRead())
    nBytesToRead = c->socket.bytesToRead();
  else
    nBytesToRead = 0;

	if(nBytesToRead || c->getForceParsing())
	{
		c->setForceParsing(0);
		if(nBytesToRead)
			err = c->socket.recv(&((char*)(buffer.getBuffer()))[c->getDataRead()],
													 MYSERVER_KB(8) - c->getDataRead(), 0);

    /* Refresh with the right value.  */
    nBytesToRead = c->getDataRead() + err;

		if(err == -1)
		{
			Server::getInstance()->deleteConnection(c, this->id);
			return 0;
		}
 		if((c->getDataRead() + err) < MYSERVER_KB(8))
		{
			((char*)buffer.getBuffer())[c->getDataRead() + err] = '\0';
		}
		else
		{
			Server::getInstance()->deleteConnection(c, this->id);
			return 0;
		}

		buffer.setBuffer(c->connectionBuffer, c->getDataRead());

		c->thread = this;
    try
    {
      switch(c->host->getProtocol())
      {
        /*
         *controlHTTPConnection returns 0 if the connection must 
         *be removed from the active connections list.
         */
			case PROTOCOL_HTTP:
        if(httpParser == 0)
        {
          httpParser = new Http();
          if(!httpParser)
            return 0;
        }
				retcode = httpParser->controlConnection(c, (char*)buffer.getBuffer(), 
													 (char*)buffer2.getBuffer(), buffer.getRealLength(), 
																	 buffer2.getRealLength(), nBytesToRead, id);
 				break;
        /*
         *Parse an HTTPS connection request.
         */
			  case PROTOCOL_HTTPS:
          if(!httpsParser)
          {
            httpsParser = new Https();
            if(!httpsParser)
              return 0;
          }
          retcode = httpsParser->controlConnection(c, 
																										(char*)buffer.getBuffer(), 
																									(char*)buffer2.getBuffer(), 
																										buffer.getRealLength(), 
																										buffer2.getRealLength(), 
																										nBytesToRead, id);
          break;
			  case PROTOCOL_CONTROL:
          if(!controlProtocolParser)
          {
            controlProtocolParser = new ControlProtocol();
            if(!controlProtocolParser)
              return 0;
          }
          retcode = controlProtocolParser->controlConnection(c, 
                       (char*)buffer.getBuffer(), (char*)buffer2.getBuffer(), 
                       buffer.getRealLength(), buffer2.getRealLength(), 
																												nBytesToRead, id);
          break;
		  	default:
          dp = Server::getInstance()->getDynProtocol(
																							c->host->getProtocolName());
			  	if(!dp)
			  	{
			  		retcode = 0;
			  	}
			  	else
				  {
				  	retcode = dp->controlConnection(c, (char*)buffer.getBuffer(), 
											(char*)buffer2.getBuffer(), buffer.getRealLength(), 
                    buffer2.getRealLength(), nBytesToRead, id);
				  }
          break;
      }
    }
    catch(...)
    {
      retcode = 0;
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
			c->setDataRead(0);
			c->connectionBuffer[0] = '\0';
		}
		/*! Incomplete request to buffer.  */
		else if(retcode == INCOMPLETE_REQUEST)
    {
			/*
       *If the header is incomplete save the current received
       *data in the connection buffer.
       *Save the header in the connection buffer.
       */
			memcpy(c->connectionBuffer, (char*)buffer.getBuffer(), 
						 c->getDataRead() + err);

			c->setDataRead(c->getDataRead() + err);
		}
		/* Incomplete request to check before new data is available.  */
		else if(retcode == INCOMPLETE_REQUEST_NO_WAIT)
		{
			c->setForceParsing(1);
		}		
		c->setTimeout( getTicks() );
	}
	else
	{
		/* Reset nTries after 5 seconds.  */
		if(getTicks() - c->getTimeout() > 5000)
			c->setnTries( 0 );

		/*
     *If the connection is inactive for a time greater that the value
     *configured remove the connection from the connections pool
     */
		if((getTicks()- c->getTimeout()) > Server::getInstance()->getTimeout() )
		{
			Server::getInstance()->deleteConnection(c, this->id);
			return 0;
		}
	}

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
 *Clean the memory used by the thread.
 */
void ClientsThread::clean()
{
	/* If the thread was not initialized return from the clean function.  */
	if(initialized == 0)
		return;
  /* If the thread is parsing wait.  */
  while(parsing)
  {
    Thread::wait(100);
  }
	threadIsRunning = 0;
  if(httpParser)
    delete httpParser;
  if(httpsParser)
    delete httpsParser;
  if(controlProtocolParser)
    delete controlProtocolParser;

	buffer.free();
	buffer2.free();

	initialized = 0;
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
int ClientsThread::isParsing()
{
  return parsing;
}
