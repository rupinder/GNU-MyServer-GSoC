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

#include "stdafx.h"
#include <include/base/read_directory/read_directory.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

class TestReadDirectory : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestReadDirectory );
  CPPUNIT_TEST ( testLoadDot );
  CPPUNIT_TEST_SUITE_END ();

public:
  void setUp () {}
  void tearDown () {}

  /* Read the content of the "." directory.
   */
  void testLoadDot ()
  {
    ReadDirectory rd;
    int ret = rd.findfirst (".");
    CPPUNIT_ASSERT_EQUAL (ret, 0);

    int counter = 0;
    /* We assume that the directory has less entries than `counterMax'.  */
    const int counterMax = 50000;

    while (!(ret = rd.findnext ()) && counter++ < counterMax)
      {
        CPPUNIT_ASSERT (rd.name.length ());
        CPPUNIT_ASSERT (rd.attrib >= 0);
        CPPUNIT_ASSERT (rd.st_nlink >= 1);
        CPPUNIT_ASSERT (rd.size >= 0);
        CPPUNIT_ASSERT (rd.time_write);
      }

    CPPUNIT_ASSERT (ret > 0);
    CPPUNIT_ASSERT (counter < counterMax);

    ret = rd.findclose ();
  }

};

CPPUNIT_TEST_SUITE_REGISTRATION ( TestReadDirectory );
