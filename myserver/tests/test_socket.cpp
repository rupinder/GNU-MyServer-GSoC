/*
  MyServer
  Copyright (C) 2008, 2009, 2010 Free Software Foundation, Inc.
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

#include "myserver.h"

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

#include "../include/base/socket/socket.h"
#include "../include/base/thread/thread.h"
#include "../include/base/utility.h"

#include <string.h>
#ifndef WIN32
# include <errno.h>
# include <arpa/inet.h>
#endif

#include <iostream>

using namespace std;

static DEFINE_THREAD (testRecvClient, pParam);

struct ThreadArg
{
  int port;
  bool success;

  /* This is valid only if !success.  */
  exception reason;
};

class TestSocket : public CppUnit::TestFixture
{
  Socket *obj;
  ThreadID tid;

  CPPUNIT_TEST_SUITE (TestSocket);

  CPPUNIT_TEST (testGethostname);
  CPPUNIT_TEST (testRecv);
  CPPUNIT_TEST (testGetLocalIPsList);

  CPPUNIT_TEST_SUITE_END ();

public:

  void setUp ()
  {
    CPPUNIT_ASSERT_EQUAL (Socket::startupSocketLib (), 0);

    obj = new Socket;
  }

  void tearDown ()
  {
    delete obj;
  }

  void testGethostname ()
  {
    int len = HOST_NAME_MAX;
    char host[HOST_NAME_MAX];
    int status = obj->gethostname (host, len);

    CPPUNIT_ASSERT_EQUAL (status, 0);
  }

  void testRecv ()
  {
    ThreadID tid;
    int optvalReuseAddr = 1;
    ThreadArg arg;
    arg.port = 6543;
    MYSERVER_SOCKADDRIN sockIn = { 0 };
    int status;

    ((sockaddr_in*) (&sockIn))->sin_family = AF_INET;
    ((sockaddr_in*) (&sockIn))->sin_addr.s_addr = inet_addr (LOCALHOST_ADDRESS);
    ((sockaddr_in*) (&sockIn))->sin_port = htons (arg.port);

    socklen_t sockInLen = sizeof (sockaddr_in);

    CPPUNIT_ASSERT (obj->socket (AF_INET, SOCK_STREAM, 0) != -1);

    CPPUNIT_ASSERT (obj->setsockopt (SOL_SOCKET, SO_REUSEADDR,
                                     (const char*) &optvalReuseAddr,
                                     sizeof (optvalReuseAddr)) != -1);

    /* If the port is used by another program, try a few others.  */
    if ((status = obj->bind (&sockIn, sockInLen)) != 0)
      while (++arg.port < 28000)
        {
          ((sockaddr_in*) (&sockIn))->sin_port = htons (arg.port);
          if ((status = obj->bind (&sockIn, sockInLen)) == 0)
            break;
        }

    CPPUNIT_ASSERT (status != -1);

    CPPUNIT_ASSERT (obj->listen (1) != -1);

    CPPUNIT_ASSERT_EQUAL (Thread::create (&tid, testRecvClient, &arg), 0);

    CPPUNIT_ASSERT (obj->dataAvailable (5));

    Socket s = obj->accept (&sockIn, &sockInLen);

    status = (int) s.getHandle ();

    if (status < 0)
      CPPUNIT_ASSERT (status);

    CPPUNIT_ASSERT (s.bytesToRead () >= 0);

    int bufLen = 8;
    char buf[bufLen];
    memset (buf, 0, bufLen);

    status = s.recv (buf, bufLen, 0);

    s.send ("a", 1, 0);

    CPPUNIT_ASSERT (!strcmp (buf, "ehlo"));
    CPPUNIT_ASSERT (status >= 0);

    Thread::join (tid);
    if (!arg.success)
      throw arg.reason;

    CPPUNIT_ASSERT (obj->close () != -1);
  }

  void testGetLocalIPsList ()
  {
    string out;
    try
      {
        obj->socket (AF_INET, SOCK_STREAM, 0);
        obj->getLocalIPsList (out);
        obj->close ();
      }
    catch (exception & e)
      {
        CPPUNIT_FAIL (e.what ());
        obj->close ();
      }
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION (TestSocket);

static DEFINE_THREAD (testRecvClient, pParam)
{
  ThreadArg *arg = (ThreadArg *) pParam;
  arg->success = true;
  try
    {
      int ret;
      Socket *obj2 = new Socket;
      char host[] = LOCALHOST_ADDRESS;

      ret = obj2->socket (AF_INET, SOCK_STREAM, 0);
      CPPUNIT_ASSERT (ret != -1);

      ret = obj2->connect (host, arg->port);
      CPPUNIT_ASSERT (ret != -1);

      int bufLen = 8;
      char buf[bufLen];
      memset (buf, 0, bufLen);
      strcpy (buf, "ehlo");

      ret = obj2->send (buf, strlen (buf), 0);
      CPPUNIT_ASSERT (ret != -1);

      /* To sync.  */
      ret = obj2->recv (buf, bufLen, 0, MYSERVER_SEC (5));
      CPPUNIT_ASSERT (ret != -1);

      ret = obj2->shutdown (SHUT_RDWR);
      CPPUNIT_ASSERT (ret != -1);

      ret = obj2->close ();
      CPPUNIT_ASSERT (ret != -1);

      delete obj2;
      obj2 = NULL;
      return 0;
    }
  catch (exception& e)
    {
      arg->success = false;
      arg->reason = e;
    }
  catch (...)
    {
    }
  return 0;
}
