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
#include <include/base/md5/md5.h>

class TestMd5 : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( TestMd5 );
  CPPUNIT_TEST( testHash );
  CPPUNIT_TEST_SUITE_END();

  Md5* md5;
public:
  void setUp()
  {
    md5 = new Md5();

  }

  void tearDown()
  {
    delete md5;
  }

  void testHash()
  {
    unsigned char out[33];
    const unsigned char* msg = (unsigned char* ) "hello world!\n";

    char *expected = (char*) "c897d1410af8f2c74fba11b1db511e9e";

    md5->init();
    md5->update(msg, 13);

    md5->end((char*)out);

    for(int i = 0; i < 32; i++)
    {
      CPPUNIT_ASSERT_EQUAL(tolower(expected[i]), tolower(out[i]));
    }

  }
};


CPPUNIT_TEST_SUITE_REGISTRATION( TestMd5 );
