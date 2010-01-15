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
#include <ctype.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <include/connections_scheduler/connections_scheduler.h>


class MockSocket : public Socket
{
  FileHandle handle;
public:

  /*
   * BE SURE TO USE VALID HANDLES.
   * 0, 1, 2 are valid ones.
   */
  MockSocket (FileHandle handle)
  {
    this->handle = handle;
  }

  virtual int connect (MYSERVER_SOCKADDR*, int)
  {
    return 0;
  }
	virtual int closesocket ()
  {
    return 0;
  }

	virtual int shutdown (int how)
  {
    return 0;
  }
	virtual int recv (char*, int, int, u_long)
  {
    return 0;
  }
	virtual int recv (char*, int, int)
  {
    return 0;
  }
	virtual u_long bytesToRead ()
  {
    return 0;
  }

	virtual Handle getHandle ()
  {
    return (Handle) handle;
  }

};


class TestSchedulerVisitor : public ConnectionsSchedulerVisitor
{
public:
  TestSchedulerVisitor ()
  {
    arg = NULL;
  }

  void *getArg ()
  {
    return arg;
  }

  virtual int visitConnection (ConnectionPtr conn, void* param)
  {
    arg = param;
    return 0;
  }
private:
  void *arg;
};


class TestConnectionsScheduler : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestConnectionsScheduler );
  CPPUNIT_TEST ( testAddNewConnection );
  CPPUNIT_TEST ( testGetNumTotalConnections );
  CPPUNIT_TEST ( testAddNewReadyConnection );
  CPPUNIT_TEST ( testGetConnections );
  CPPUNIT_TEST ( testVisitor );
  CPPUNIT_TEST_SUITE_END ();

  ConnectionsScheduler* scheduler;
public:

  void setUp ()
  {
    CPPUNIT_ASSERT_EQUAL ( Socket::startupSocketLib ( ), 0 );
    scheduler = new ConnectionsScheduler ();
    scheduler->initialize ();
  }

  void tearDown ()
  {
    scheduler->release ();
    delete scheduler;
  }

  void testVisitor ()
  {
    TestSchedulerVisitor visitor;

    void* arg = this;
    u_long max = 3;

    for (u_long i = 0; i < max; i++)
      {
        ConnectionPtr conn = new Connection;
        conn->socket = new MockSocket ((FileHandle) i);
        scheduler->addWaitingConnection (conn);
      }

    scheduler->accept (&visitor, arg);

    CPPUNIT_ASSERT_EQUAL (arg, visitor.getArg ());

  }

  void testGetConnections ()
  {
    ConnectionPtr conn = new Connection;
    conn->socket = new MockSocket ((FileHandle) 1);

    list<ConnectionPtr> out;

    scheduler->getConnections (out);

    CPPUNIT_ASSERT_EQUAL (out.size (), (size_t)0);

    conn = new Connection;
    conn->socket = new MockSocket ((FileHandle) 2);

    scheduler->addWaitingConnection (conn);

    scheduler->getConnections (out);

    CPPUNIT_ASSERT_EQUAL (out.size (), (size_t)1);
  }

  void testGetNumTotalConnections ()
  {
    ConnectionPtr conn = new Connection;
    conn->socket = new MockSocket ((FileHandle) 1);

    CPPUNIT_ASSERT_EQUAL (scheduler->getNumTotalConnections (), 0ul);

    scheduler->registerConnectionID (conn);

    CPPUNIT_ASSERT_EQUAL (scheduler->getNumTotalConnections (), 1ul);

    scheduler->registerConnectionID (conn);

    CPPUNIT_ASSERT_EQUAL (scheduler->getNumTotalConnections (), 2ul);

    scheduler->registerConnectionID (conn);

    CPPUNIT_ASSERT_EQUAL (scheduler->getNumTotalConnections (), 3ul);

    delete conn;
  }

  void testAddNewReadyConnection ()
  {
    ConnectionPtr gotConn;

    ConnectionPtr conn = new Connection;
    conn->socket = new MockSocket ((FileHandle) 1);

    CPPUNIT_ASSERT_EQUAL (scheduler->getNumTotalConnections (), 0ul);


    scheduler->addReadyConnection (conn);

    CPPUNIT_ASSERT_EQUAL (scheduler->getNumTotalConnections (), 0ul);

    gotConn = scheduler->getConnection ();

    CPPUNIT_ASSERT_EQUAL (scheduler->getNumTotalConnections (), 0ul);
    CPPUNIT_ASSERT_EQUAL (conn, gotConn);

    delete conn;
  }

  void testAddNewConnection ()
  {
    ConnectionPtr conn = new Connection;
    conn->socket = new MockSocket ((FileHandle) 1);

    CPPUNIT_ASSERT_EQUAL (scheduler->getNumTotalConnections (), 0ul);

    scheduler->addWaitingConnection (conn);

    CPPUNIT_ASSERT_EQUAL (scheduler->getNumAliveConnections (), 1ul);

    scheduler->removeConnection (conn);

    CPPUNIT_ASSERT_EQUAL (scheduler->getNumTotalConnections (), 0ul);
    /* The connection is freed by the scheduler.  */
  }




};


CPPUNIT_TEST_SUITE_REGISTRATION ( TestConnectionsScheduler );
