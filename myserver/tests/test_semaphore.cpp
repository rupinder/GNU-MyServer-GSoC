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

#include <stdafx.h>
#include <ctype.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <include/base/sync/semaphore.h>
#include <include/base/thread/thread.h>

#include <string.h>

#include <iostream>
using namespace std;


class TestSemaphore : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestSemaphore );
  CPPUNIT_TEST ( testUnlockLock );
  CPPUNIT_TEST ( testConstructor );
  CPPUNIT_TEST ( testIsInitialized );
  CPPUNIT_TEST ( testGetHandle );
  CPPUNIT_TEST_SUITE_END ();

  Semaphore *sem;
public:
  void setUp ()
  {
  }

  void tearDown ()
  {

  }

  void testGetHandle ()
  {
    sem = new Semaphore (0);

    CPPUNIT_ASSERT (sem->getHandle ());

    delete sem;
  }

  void testUnlockLock ()
  {
    sem = new Semaphore (0);

    sem->unlock ();

    sem->lock ();

    for (int i = 0; i < 10; i++)
      sem->unlock ();

    for (int i = 0; i < 10; i++)
      sem->lock ();

    delete sem;
  }


  void testIsInitialized ()
  {
    sem = new Semaphore (0);


    CPPUNIT_ASSERT (sem->isInitialized ());

    sem->destroy ();

    CPPUNIT_ASSERT_EQUAL (sem->isInitialized (), 0);

    delete sem;
  }

  void testConstructor ()
  {
    sem = new Semaphore (10);

    for (int i = 0; i < 10; i++)
      sem->lock ();
  }

};


CPPUNIT_TEST_SUITE_REGISTRATION ( TestSemaphore );
