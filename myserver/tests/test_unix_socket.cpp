/*
 MyServer
 Copyright (C) 2009, 2010 Free Software Foundation, Inc.
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

#include <include/base/socket_pair/socket_pair.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

#include <include/base/utility.h>
#include <include/base/file/files_utility.h>
#include <include/base/unix_socket/unix_socket.h>
#include <include/base/thread/thread.h>
#include <include/base/pipe/pipe.h>
#include <string.h>

#ifdef AF_UNIX

# define TEST_STRING "Hello World!"

struct UnixSocketServerType
{
  bool result;
  UnixSocket *socket;
};

void* test_unix_socket_server (void* pParam)
{
  UnixSocketServerType *server = (UnixSocketServerType*)pParam;
  server->result = false;

  Socket client = server->socket->accept ();

  if (client.getHandle () == 0)
    return NULL;

  char buffer [256];
  if (client.recv (buffer, 256, 0, MYSERVER_SEC (1)) == 0)
    {
      client.close ();
      return NULL;
    }


  server->result = ! strcasecmp (buffer, TEST_STRING);

  client.close ();
  return NULL;
}



class TestUnixSocket : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestUnixSocket );
  CPPUNIT_TEST ( testBind );
  CPPUNIT_TEST ( testClient );
  CPPUNIT_TEST ( testReadWriteHandle );
  CPPUNIT_TEST_SUITE_END ();

  UnixSocket *sock;
public:

  void setUp ()
  {
    sock = new UnixSocket ();
  }

  void tearDown ()
  {
    delete sock;
  }

  void testBind ()
  {
    int ret;
    string path;
    FilesUtility::temporaryFileName (0, path);

    CPPUNIT_ASSERT (sock->socket ());

    ret = FilesUtility::nodeExists (path.c_str ());
    CPPUNIT_ASSERT_EQUAL (ret, 0);

    CPPUNIT_ASSERT_EQUAL (sock->bind (path.c_str ()), 0);

    ret = FilesUtility::nodeExists (path.c_str ());
    CPPUNIT_ASSERT (ret);

    CPPUNIT_ASSERT_EQUAL (sock->shutdown (), 0);

    CPPUNIT_ASSERT_EQUAL (sock->close (), 0);

    ret = FilesUtility::nodeExists (path.c_str ());
    CPPUNIT_ASSERT_EQUAL (ret, 0);
  }

  void testClient ()
  {
    ThreadID tid;
    UnixSocketServerType data;
    data.socket = sock;
    data.result = false;
    string path;
    FilesUtility::temporaryFileName (0, path);
    sock->socket ();
    sock->bind (path.c_str ());
    CPPUNIT_ASSERT_EQUAL (sock->listen (1), 0);

    Thread::create (&tid, test_unix_socket_server, &data);

    UnixSocket client;
    client.socket ();
    CPPUNIT_ASSERT_EQUAL (client.connect (path.c_str ()), 0);
    int length = strlen (TEST_STRING) + 1;
    CPPUNIT_ASSERT_EQUAL (client.send (TEST_STRING, length, 0), length);

    Thread::join (tid);

    client.shutdown ();
    client.close ();

    CPPUNIT_ASSERT_EQUAL (data.result, true);
  }


  void testReadWriteHandle ()
  {
# ifndef WIN32
    const char *msg = "hello world";
    ThreadID tid;
    Pipe pipe;
    u_long nbw;
    char buffer[64];
    string path;
    FilesUtility::temporaryFileName (0, path);
    sock->socket ();
    sock->bind (path.c_str ());
    CPPUNIT_ASSERT_EQUAL (sock->listen (1), 0);

    pipe.create ();
    CPPUNIT_ASSERT (! pipe.write (msg, strlen (msg), &nbw));

    UnixSocket client;
    client.socket ();
    CPPUNIT_ASSERT_EQUAL (client.connect (path.c_str ()), 0);

    Socket clientRecv = sock->accept ();
    Handle fd;

    CPPUNIT_ASSERT (writeFileHandle (client.getHandle (), pipe.getReadHandle ()) >= 0);
    CPPUNIT_ASSERT (readFileHandle (clientRecv.getHandle (), &fd) >= 0);

    CPPUNIT_ASSERT (read (fd, buffer, 64) > 0);

    CPPUNIT_ASSERT_EQUAL (strcmp (buffer, msg), 0);

    clientRecv.close ();
    client.shutdown ();
    client.close ();
# endif
  }

};

CPPUNIT_TEST_SUITE_REGISTRATION ( TestUnixSocket );

#endif
