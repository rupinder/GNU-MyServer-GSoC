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
#include <include/conf/security/security_manager.h>
#include <include/conf/security/security_cache.h>

#include <string.h>

#include <iostream>
using namespace std;

class TestSecurityCache : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestSecurityCache );
  CPPUNIT_TEST ( testMaxNodes );
  CPPUNIT_TEST ( testGetParser );
  CPPUNIT_TEST ( testGetSecurityFile );
  CPPUNIT_TEST_SUITE_END ();

  SecurityCache *secCache;
public:
  void setUp ()
  {
    secCache = new SecurityCache ();
  }

  void tearDown ()
  {
    delete secCache;
  }

  void testMaxNodes ()
  {
    secCache->setMaxNodes (-10);
    CPPUNIT_ASSERT_EQUAL (secCache->getMaxNodes (), 1);

    secCache->setMaxNodes (0);
    CPPUNIT_ASSERT_EQUAL (secCache->getMaxNodes (), 1);

    secCache->setMaxNodes (1);
    CPPUNIT_ASSERT_EQUAL (secCache->getMaxNodes (), 1);

    secCache->setMaxNodes (10);
    CPPUNIT_ASSERT_EQUAL (secCache->getMaxNodes (), 10);

    secCache->setMaxNodes (100);
    CPPUNIT_ASSERT_EQUAL (secCache->getMaxNodes (), 100);
  }

  void testGetSecurityFile ()
  {
    string file ("foo/bar/not/exist/in/reality");
    string out;

    CPPUNIT_ASSERT (secCache->getSecurityFile (file, file, out));
  }


  void testGetParser ()
  {
    string dir ("foo/bar/not/exist/in/reality");
    string file ("baz");
    CPPUNIT_ASSERT_EQUAL (secCache->getParser (dir, dir, false),
                          (SecurityCache::CacheNode *) NULL);
  }


};


CPPUNIT_TEST_SUITE_REGISTRATION ( TestSecurityCache );
