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
#include <include/base/utility.h>
#include <include/base/thread/thread.h>
#include <string>

using namespace std;


class TestUtility : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( TestUtility );
  CPPUNIT_TEST( testGetTicks );
  CPPUNIT_TEST( testGetOsVersion );
  CPPUNIT_TEST( testCWD );
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp()
  {
 
  }

  void tearDown()
  {

  }

  void testGetTicks()
  {

    u_long ticks = getTicks();

    Thread::wait(1500);

    CPPUNIT_ASSERT(getTicks() > ticks);
  }

  void testGetOsVersion()
  {
    CPPUNIT_ASSERT(getOSVersion() > 0);
  }


  void testGetCPUCount()
  {
    CPPUNIT_ASSERT(getCPUCount() > 0);
  }

  void testCWD()
  {
    char *buffer;
    int bufferLen;

    setcwdBuffer();

    bufferLen = getdefaultwdlen();
    CPPUNIT_ASSERT(bufferLen > 0);

    buffer = new char[bufferLen + 1];


    getdefaultwd(buffer, bufferLen);

    CPPUNIT_ASSERT( strlen(buffer) > 0 );
    CPPUNIT_ASSERT(strlen(buffer) <= bufferLen);


    string strBuff;

    int ret = getdefaultwd(strBuff);

    CPPUNIT_ASSERT_EQUAL(ret, 0);

    CPPUNIT_ASSERT(strBuff.length() > 0);
    


    freecwdBuffer();

    delete [] buffer;
  }

};


CPPUNIT_TEST_SUITE_REGISTRATION( TestUtility );
