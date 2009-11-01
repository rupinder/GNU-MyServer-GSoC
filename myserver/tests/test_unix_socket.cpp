/*
 MyServer
 Copyright (C) 2009 Free Software Foundation, Inc.
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

#include <stdafx.h>

#include <include/base/socket_pair/socket_pair.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

#include <include/base/utility.h>
#include <include/base/file/files_utility.h>
#include <include/base/unix_socket/unix_socket.h>
#include <include/base/thread/thread.h>
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


  server->result = !strcmpi (buffer, TEST_STRING);

  client.close ();
  return NULL;
}



class TestUnixSocket : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestUnixSocket );
  CPPUNIT_TEST ( testBind );
  CPPUNIT_TEST ( testClient );
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

    //FIXME: find a better place inside FilesUtility.
    struct stat F_Stats;
    ret = stat (path.c_str (), &F_Stats);
    CPPUNIT_ASSERT (ret);
    /////////////////////////////////////////////////

    CPPUNIT_ASSERT_EQUAL (sock->bind (path.c_str ()), 0);

    ret = stat (path.c_str (), &F_Stats);
    CPPUNIT_ASSERT_EQUAL (ret, 0);

    CPPUNIT_ASSERT_EQUAL (sock->shutdown (), 0);

    CPPUNIT_ASSERT_EQUAL (sock->close (), 0);

    ret = stat (path.c_str (), &F_Stats);
    CPPUNIT_ASSERT (ret);
  }

  void testClient ()
  {
    ThreadID tid;
    UnixSocketServerType data;
    data.socket = sock;
    data.result = false;

    int ret;
    string path;
    FilesUtility::temporaryFileName (0, path);
    sock->socket ();
    sock->bind (path.c_str ());
    CPPUNIT_ASSERT_EQUAL (sock->listen (1), 0);

    int res = Thread::create (&tid, test_unix_socket_server, &data);

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
};

CPPUNIT_TEST_SUITE_REGISTRATION ( TestUnixSocket );

#endif
