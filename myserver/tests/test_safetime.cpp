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
#include <include/base/safetime/safetime.h>

#include <string.h>

#include <iostream>
using namespace std;

class TestSafetime : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( TestSafetime );
  CPPUNIT_TEST( testLocalTime );
  CPPUNIT_TEST( testGmTime );
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp()
  {
    myserver_safetime_init();
  }

  void tearDown()
  {
    myserver_safetime_destroy();
  }

  void testLocalTime()
  {
    tm res;
    time_t timep = 10;
    tm *outRes = myserver_localtime(&timep, &res);

    CPPUNIT_ASSERT(outRes);
    CPPUNIT_ASSERT_EQUAL(res.tm_sec, outRes->tm_sec);
    CPPUNIT_ASSERT_EQUAL(res.tm_min, outRes->tm_min);
    CPPUNIT_ASSERT_EQUAL(res.tm_hour, outRes->tm_hour);
    CPPUNIT_ASSERT_EQUAL(res.tm_mday, outRes->tm_mday);
    CPPUNIT_ASSERT_EQUAL(res.tm_mon, outRes->tm_mon);
    CPPUNIT_ASSERT_EQUAL(res.tm_year, outRes->tm_year);
    CPPUNIT_ASSERT_EQUAL(res.tm_wday, outRes->tm_wday);
    CPPUNIT_ASSERT_EQUAL(res.tm_yday, outRes->tm_yday);
    CPPUNIT_ASSERT_EQUAL(res.tm_isdst, outRes->tm_isdst);


    CPPUNIT_ASSERT_EQUAL(res.tm_sec, 10);
    CPPUNIT_ASSERT_EQUAL(res.tm_min, 0);
    CPPUNIT_ASSERT_EQUAL(res.tm_year, 70);
  }

  void testGmTime()
  {
    tm res;
    time_t timep = 10;
    tm *outRes = myserver_gmtime(&timep, &res);

    CPPUNIT_ASSERT(outRes);
    CPPUNIT_ASSERT_EQUAL(res.tm_sec, outRes->tm_sec);
    CPPUNIT_ASSERT_EQUAL(res.tm_min, outRes->tm_min);
    CPPUNIT_ASSERT_EQUAL(res.tm_hour, outRes->tm_hour);
    CPPUNIT_ASSERT_EQUAL(res.tm_mday, outRes->tm_mday);
    CPPUNIT_ASSERT_EQUAL(res.tm_mon, outRes->tm_mon);
    CPPUNIT_ASSERT_EQUAL(res.tm_year, outRes->tm_year);
    CPPUNIT_ASSERT_EQUAL(res.tm_wday, outRes->tm_wday);
    CPPUNIT_ASSERT_EQUAL(res.tm_yday, outRes->tm_yday);
    CPPUNIT_ASSERT_EQUAL(res.tm_isdst, outRes->tm_isdst);


    CPPUNIT_ASSERT_EQUAL(res.tm_sec, 10);
    CPPUNIT_ASSERT_EQUAL(res.tm_min, 0);
    CPPUNIT_ASSERT_EQUAL(res.tm_hour, 0);
    CPPUNIT_ASSERT_EQUAL(res.tm_year, 70);
  }
};


CPPUNIT_TEST_SUITE_REGISTRATION( TestSafetime );
