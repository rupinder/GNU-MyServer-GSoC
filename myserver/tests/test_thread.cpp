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
#include <include/base/thread/thread.h>

#include <iostream>
using namespace std;

static DEFINE_THREAD (test_thread,pParam)
{
  int *arg = (int*) pParam;

  *arg *= *arg;

  return NULL;
}

static DEFINE_THREAD (test_terminate_thread, pParam)
{
  int *arg = (int*) pParam;

  *arg = 0;

  Thread::terminate ();

  //Should never be here.
  *arg = 1;

  return NULL;
}

class TestThread : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestThread );
  CPPUNIT_TEST ( testThreadCreate );
  CPPUNIT_TEST ( testThreadTerminate );
  CPPUNIT_TEST_SUITE_END ();

public:
  void setUp ()
  {

  }

  void tearDown ()
  {

  }

  void testThreadCreate ()
  {
    ThreadID tid;
    int data = 5;
    int expected = data * data;

    int res = Thread::create (&tid, test_thread, &data);
    Thread::join (tid);


    CPPUNIT_ASSERT_EQUAL (res, 0);
    CPPUNIT_ASSERT_EQUAL (data, expected);
  }

  void testThreadTerminate ()
  {
    ThreadID tid;
    int data = 1;
    int res = Thread::create (&tid, test_terminate_thread, &data);
    Thread::join (tid);


    CPPUNIT_ASSERT_EQUAL (res, 0);
    CPPUNIT_ASSERT_EQUAL (data, 0);
  }


};


CPPUNIT_TEST_SUITE_REGISTRATION ( TestThread );
