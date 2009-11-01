/*
 MyServer
 Copyright (C) 2008, 2009 Free Software Foundation, Inc.
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
#include <ctype.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <include/connection/connection.h>
#include <include/server/clients_thread.h>

class TestConnection : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestConnection );
  CPPUNIT_TEST ( testID );
  CPPUNIT_TEST ( testTimeout );
  CPPUNIT_TEST ( testLocalPort );
  CPPUNIT_TEST ( testIpAddress );
  CPPUNIT_TEST ( testActiveThread );
  CPPUNIT_TEST ( testLocalIpAddress );
  CPPUNIT_TEST ( testTries );
  CPPUNIT_TEST ( testPort );
  CPPUNIT_TEST ( testLogin );
  CPPUNIT_TEST ( testPassword );
  CPPUNIT_TEST ( testForceControl );
  CPPUNIT_TEST ( testToRemove );
  CPPUNIT_TEST ( testScheduled );
  CPPUNIT_TEST ( testPriority );
  CPPUNIT_TEST_SUITE_END ();

  ConnectionPtr connection;

public:
  void setUp ()
  {
    connection = new Connection;
  }

  void tearDown ()
  {
    delete connection;
  }


  void testID ()
  {
    for (u_long i = 0; i < 100; i += 10)
    {
      connection->setID (i);
      CPPUNIT_ASSERT_EQUAL (connection->getID (), i);
    }
  }

  void testTimeout ()
  {
    for (u_long i = 0; i < 100; i += 10)
    {
      connection->setTimeout (i);
      CPPUNIT_ASSERT_EQUAL (connection->getTimeout (), i);
    }
  }

  void testLocalPort ()
  {
    for (u_short i = 0; i < 100; i += 10)
    {
      connection->setLocalPort (i);
      CPPUNIT_ASSERT_EQUAL (connection->getLocalPort (), i);
    }
  }

  void testIpAddress ()
  {
    connection->setIpAddr ("127.0.0.1");
    CPPUNIT_ASSERT (strcmp (connection->getIpAddr (), "127.0.0.1") == 0);
  }

  void testActiveThread ()
  {
    ClientsThread *ct = new ClientsThread (NULL);

    connection->setActiveThread (ct);

    CPPUNIT_ASSERT_EQUAL (connection->getActiveThread (), ct);

    delete ct;
  }

  void testLocalIpAddress ()
  {
    connection->setLocalIpAddr ("127.0.0.1");
    CPPUNIT_ASSERT (strcmp (connection->getLocalIpAddr (), "127.0.0.1") == 0);
  }

  void testTries ()
  {
    char tries = 0;
    connection->setnTries (tries);
    CPPUNIT_ASSERT_EQUAL (connection->getnTries (), tries);

    for (int i = 0; i < 10; i++)
    {
      tries++;
      connection->incnTries ();
      CPPUNIT_ASSERT_EQUAL (connection->getnTries (), tries);

    }
  }

  void testPort ()
  {
    for (u_short i = 0; i < 100; i += 10)
    {
      connection->setPort (i);
      CPPUNIT_ASSERT_EQUAL (connection->getPort (), i);
    }
  }


  void testLogin ()
  {
    connection->setLogin ("username");
    CPPUNIT_ASSERT (strcmp (connection->getLogin (), "username") == 0);
  }

  void testPassword ()
  {
    connection->setPassword ("password");
    CPPUNIT_ASSERT (strcmp (connection->getPassword (), "password") == 0);
  }


  void testForceControl ()
  {
    connection->setForceControl (0);
    CPPUNIT_ASSERT_EQUAL (connection->isForceControl (), 0);

    connection->setForceControl (1);
    CPPUNIT_ASSERT_EQUAL (connection->isForceControl (), 1);
  }

  void testToRemove ()
  {
    connection->setToRemove (0);
    CPPUNIT_ASSERT_EQUAL (connection->getToRemove (), 0);

    connection->setToRemove (1);
    CPPUNIT_ASSERT_EQUAL (connection->getToRemove (), 1);
  }

  void testScheduled ()
  {
    connection->setScheduled (0);
    CPPUNIT_ASSERT_EQUAL (connection->isScheduled (), 0);

    connection->setScheduled (1);
    CPPUNIT_ASSERT_EQUAL (connection->isScheduled (), 1);
  }


  void testPriority ()
  {
    for (int i = 0; i < 100; i += 10)
    {
      connection->setPriority (i);
      CPPUNIT_ASSERT_EQUAL (connection->getPriority (), i);
    }
  }

  void testContinuation ()
  {
    continuationPROC continuation = (continuationPROC) 100;

    CPPUNIT_ASSERT (connection->getContinuation () == NULL);
    connection->setContinuation (continuation);

    CPPUNIT_ASSERT (connection->getContinuation () == continuation);
  }



};


CPPUNIT_TEST_SUITE_REGISTRATION ( TestConnection );
