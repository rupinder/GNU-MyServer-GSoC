/*
 MyServer
 Copyright (C) 2008 Free Software Foundation, Inc.
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

#include <ctype.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include "../include/mutex.h"
#include "../include/thread.h"

#include <string.h>

#include <iostream>
using namespace std;


struct TestMutexThreadArg
{
  Mutex *mutex;
  int value;
  int max;
};

#ifdef WIN32
unsigned int __stdcall test_mutex_incremented(void* pParam)
#endif
#ifdef HAVE_PTHREAD
void* test_mutex_incrementer(void* pParam)
#endif
{
  TestMutexThreadArg *arg = (TestMutexThreadArg*) pParam;

  for(int i = 0; i < arg->max; i++)
  {
    arg->mutex->lock();
    arg->value++;
    arg->mutex->unlock();
  }
}


class TestMutex : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( TestMutex );
  CPPUNIT_TEST( testLockUnlock );
  CPPUNIT_TEST( testSynchronizedAccess );
  CPPUNIT_TEST_SUITE_END();

  Mutex *mutex;
public:
  void setUp()
  {
    mutex = new Mutex();

  }

  void tearDown()
  {
    delete mutex;
  }

  void testLockUnlock()
  {
    mutex->lock();
    CPPUNIT_ASSERT_EQUAL(mutex->isLocked(), true);
    mutex->unlock();
    CPPUNIT_ASSERT_EQUAL(mutex->isLocked(), false);
  }

  void testSynchronizedAccess()
  {
    const int N_THREADS = 10;
    ThreadID tid[N_THREADS];
    Mutex mutex;
    TestMutexThreadArg arg;

    arg.value = 0;
    arg.mutex = &mutex;
    arg.max = 100;

    for(int i = 0; i < N_THREADS; i++)
    {
      int res = Thread::create(&(tid[i]), test_mutex_incrementer, &arg);
      CPPUNIT_ASSERT_EQUAL(res, 0);
    }

    for(int i = 0; i < N_THREADS; i++)
      Thread::join(tid[i]);

    CPPUNIT_ASSERT_EQUAL(arg.value, N_THREADS * arg.max);

  }
};


CPPUNIT_TEST_SUITE_REGISTRATION( TestMutex );
