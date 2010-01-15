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

extern "C"
{
#include <string.h>
#ifndef WIN32
# include <errno.h>
# include <arpa/inet.h>
#endif
}
#include <iostream>

using namespace std;

static DEFINE_THREAD (testRecvClient, pParam);

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
    char host[] = "localhost";
    int port = 6543;
    MYSERVER_SOCKADDRIN sockIn = { 0 };
    int status;

    ((sockaddr_in*) (&sockIn))->sin_family = AF_INET;
    ((sockaddr_in*) (&sockIn))->sin_addr.s_addr = inet_addr ("127.0.0.1");
    ((sockaddr_in*) (&sockIn))->sin_port = htons (port);

    socklen_t sockInLen = sizeof (sockaddr_in);

    CPPUNIT_ASSERT (obj->socket (AF_INET, SOCK_STREAM, 0) != -1);

    CPPUNIT_ASSERT (obj->setsockopt (SOL_SOCKET, SO_REUSEADDR,
                                      (const char*) &optvalReuseAddr,
                                      sizeof (optvalReuseAddr)) != -1);

    /* If the port is used by another program, try a few others.  */
    if ((status = obj->bind (&sockIn, sockInLen)) != 0)
      while (++port < 28000)
        {
          ((sockaddr_in*) (&sockIn))->sin_port = htons (port);
          if ((status = obj->bind (&sockIn, sockInLen)) == 0)
            break;
        }

    CPPUNIT_ASSERT (status != -1);

    CPPUNIT_ASSERT (obj->listen (1) != -1);

    CPPUNIT_ASSERT_EQUAL (Thread::create (&tid, testRecvClient, &port), 0);

    CPPUNIT_ASSERT (obj->dataOnRead (5));

    Socket s = obj->accept (&sockIn, &sockInLen);

    status = int (s.getHandle ());

    if (status < 0)
      CPPUNIT_ASSERT (status != -1);

    CPPUNIT_ASSERT (s.bytesToRead () >= 0);

    int bufLen = 8;
    char buf[bufLen];
    memset (buf, 0, bufLen);

    status = s.recv (buf, bufLen, 0);

    s.send ("a", 1, 0);

    CPPUNIT_ASSERT (!strcmp (buf, "ehlo"));
    CPPUNIT_ASSERT (status >= 0 || status == -2);

    Thread::join (tid);

    CPPUNIT_ASSERT (obj->close () != -1);
  }

  void testGetLocalIPsList ()
  {
    int status = obj->socket (AF_INET, SOCK_STREAM, 0);

    CPPUNIT_ASSERT (status != -1);

    string out;

    status = obj->getLocalIPsList (out);

    CPPUNIT_ASSERT (status != -1);

    status = obj->close ();

    CPPUNIT_ASSERT (status != -1);
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION (TestSocket);

static DEFINE_THREAD (testRecvClient, pParam)
{
  Socket *obj2 = new Socket;

  int optvalReuseAddr = 1;
  char host[] = "localhost";
  int port = *((int*)pParam);
  int status;

  CPPUNIT_ASSERT (obj2->socket (AF_INET, SOCK_STREAM, 0) != -1);

  CPPUNIT_ASSERT (obj2->connect (host, port) != -1);

  int bufLen = 8;
  char buf[bufLen];
  memset (buf, 0, bufLen);
  strcpy (buf, "ehlo");

  CPPUNIT_ASSERT (obj2->send (buf, strlen (buf), 0) != -1);

  /* To sync.  */
  CPPUNIT_ASSERT (obj2->recv (buf, bufLen, 0, MYSERVER_SEC (5)) != -1);

  CPPUNIT_ASSERT (obj2->shutdown (SD_BOTH) != -1);

  CPPUNIT_ASSERT (obj2->close () != -1);

  delete obj2;
  obj2 = NULL;
}
