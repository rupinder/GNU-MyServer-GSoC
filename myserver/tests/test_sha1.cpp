/*
 MyServer Copyright (C) 2008, 2009 Free Software Foundation, Inc.
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or (at
 your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdafx.h>
#include <include/base/crypt/sha1.h>

#include <ctype.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <string.h>

class TestSha1 : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestSha1 );
  CPPUNIT_TEST ( testHash );
  CPPUNIT_TEST_SUITE_END ();

  Sha1* sha1;
public:
  void setUp ()
  {
    sha1 = new Sha1();

  }

  void tearDown ()
  {
    delete sha1;
  }

  void testHash ()
  {
    char out[45];
    const char* msg = "GNU is not UNIX";

    char *expected = (char*) "5ef99232e377af054b479eae19b1b06d688f2a7e";

    sha1->init ();
    sha1->update (msg, strlen (msg));
    char *ret = sha1->end (out);

    CPPUNIT_ASSERT_EQUAL (ret, &out[0]);

    for (int i = 0; i < 32; i++)
      CPPUNIT_ASSERT_EQUAL (tolower (expected[i]), tolower (out[i]));
  }
};


CPPUNIT_TEST_SUITE_REGISTRATION ( TestSha1 );
