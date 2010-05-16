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
#include <include/base/files_cache/cached_file_factory.h>
#include <string.h>

#include <string>

using namespace std;

class TestCachedFileFactory : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestCachedFileFactory );

  CPPUNIT_TEST (testInitialize);
  CPPUNIT_TEST (testSize);
  CPPUNIT_TEST (testMaxSize);
  CPPUNIT_TEST (testMinSize);

  CPPUNIT_TEST_SUITE_END ();
  CachedFileFactory *cff;

public:
  void setUp ()
  {
    cff = new CachedFileFactory ();
  }

  void tearDown ()
  {
    delete cff;
  }

  void testInitialize ()
  {
    cff->initialize (20);

    CPPUNIT_ASSERT_EQUAL (cff->getSize (), 20ul);
    CPPUNIT_ASSERT_EQUAL (cff->getUsed (), 0ul);
    CPPUNIT_ASSERT_EQUAL (cff->getUsedSize (), 0ul);
    CPPUNIT_ASSERT_EQUAL (cff->getMaxSize (), 0ul);
    CPPUNIT_ASSERT_EQUAL (cff->getMinSize (), 0ul);
    CPPUNIT_ASSERT_EQUAL (cff->getMaxSize (), 0ul);
  }


  void testSize ()
  {
    for (u_long i = 0; i < 100; i += 10)
    {
      cff->setSize (i);
      CPPUNIT_ASSERT_EQUAL (cff->getSize (), i);
    }
  }

  void testMaxSize ()
  {
    for (u_long i = 0; i < 100; i += 10)
    {
      cff->setMaxSize (i);
      CPPUNIT_ASSERT_EQUAL (cff->getMaxSize (), i);
    }
  }

  void testMinSize ()
  {
    for (u_long i = 0; i < 100; i += 10)
    {
      cff->setMinSize (i);
      CPPUNIT_ASSERT_EQUAL (cff->getMinSize (), i);
    }
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION ( TestCachedFileFactory );
