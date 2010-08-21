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

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>


#include "../include/base/slab/slab.h"

struct TestSlabRecord
{
  int a;
  int b;
};

class TestSlab : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestSlab );

  CPPUNIT_TEST ( testGet );
  CPPUNIT_TEST ( testForcedGet );
  CPPUNIT_TEST ( testCapacity );
  CPPUNIT_TEST ( testInit );

  CPPUNIT_TEST_SUITE_END ();

public:
  void setUp ()
  {
  }

  void tearDown ()
  {
  }

  void testInit ()
  {
    size_t N = 10;
    Slab<TestSlabRecord> slabs (N);
    CPPUNIT_ASSERT_EQUAL (slabs.getCapacity (), N);

    for (size_t i = 1; i < 100; i += 10)
      {
        slabs.init (i);
        CPPUNIT_ASSERT_EQUAL (slabs.getCapacity (), i);
      }
  }

  void testCapacity ()
  {
    size_t N = 100;
    Slab<TestSlabRecord> slabs (N);

    CPPUNIT_ASSERT_EQUAL (slabs.getCapacity (), N);
  }

  void testGet ()
  {
    size_t N = 100;
    Slab<TestSlabRecord> slabs (N);
    TestSlabRecord *recs[N];

    for (u_long j = 0; j < N; j++)
      {
        recs[j] = slabs.get ();
        CPPUNIT_ASSERT (recs[j]);
      }

    /* No more free instances.  */
    CPPUNIT_ASSERT_EQUAL (slabs.get (), (TestSlabRecord*)NULL);

    for (u_long j = 0; j < N; j++)
      slabs.put (recs[j]);


    CPPUNIT_ASSERT (slabs.get ());
  }

  void testForcedGet ()
  {
    size_t N = 100;
    Slab<TestSlabRecord> slabs (N);
    TestSlabRecord *recs[N * 2];

    for (u_long j = 0; j < N * 2; j++)
      {
        recs[j] = slabs.forcedGet ();
        CPPUNIT_ASSERT (recs[j]);
      }

    for (u_long j = 0; j < N * 2; j++)
      slabs.put (recs[j]);
  }

};

CPPUNIT_TEST_SUITE_REGISTRATION (TestSlab);
