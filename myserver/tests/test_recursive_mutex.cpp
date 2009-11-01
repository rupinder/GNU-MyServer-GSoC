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
#include <include/base/sync/recursive_mutex.h>


class TestRecursiveMutex : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestRecursiveMutex );
  CPPUNIT_TEST ( testLockUnlock );
  CPPUNIT_TEST_SUITE_END ();

  Mutex *mutex;
public:
  void setUp ()
  {
    mutex = new RecursiveMutex ();

  }

  void tearDown ()
  {
    delete mutex;
  }

  void testLockUnlock ()
  {
    const int N_TIMES = 10;
    //This shouldn't deadlock.  If it does, then it is bug.
    for (int i = 0; i < N_TIMES; i++)
    {
      mutex->lock ();
      CPPUNIT_ASSERT_EQUAL (mutex->isLocked (), true);
    }

    for (int i = 0; i < N_TIMES; i++)
    {
      mutex->unlock ();
      CPPUNIT_ASSERT_EQUAL (mutex->isLocked (), false);
    }
  }
};


CPPUNIT_TEST_SUITE_REGISTRATION ( TestRecursiveMutex );
