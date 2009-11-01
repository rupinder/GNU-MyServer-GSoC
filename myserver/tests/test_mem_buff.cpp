/*
 MyServer
 Copyright (C) 2008, 2009 Free Software Foundation, Inc.
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

#include <stdafx.h>
#include <ctype.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <include/base/mem_buff/mem_buff.h>
#include <string.h>

class TestMemBuffer : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestMemBuffer );
  CPPUNIT_TEST ( testLength );
  CPPUNIT_TEST ( testFind );
  CPPUNIT_TEST ( testReplace );
  CPPUNIT_TEST ( testUintToStr );
  CPPUNIT_TEST ( testIntToStr );
  CPPUNIT_TEST ( testHexCharToNumber );
  CPPUNIT_TEST ( testGetPart );
  CPPUNIT_TEST ( testIsValid );
  CPPUNIT_TEST ( testExternalBuffer );
  CPPUNIT_TEST ( testGetAt );
  CPPUNIT_TEST ( testGetBuffer );
  CPPUNIT_TEST_SUITE_END ();

  MemBuf *memBuff;

public:
  void setUp ()
  {
    memBuff = new MemBuf ();
  }

  void tearDown ()
  {
    delete memBuff;
  }


  void testLength ()
  {
    u_int len = 10;
    memBuff->setLength (len);
    CPPUNIT_ASSERT_EQUAL (memBuff->getRealLength (), len);

    CPPUNIT_ASSERT_EQUAL (memBuff->getLength (), 0u);

    *memBuff << (const char*)"1234567890";

    CPPUNIT_ASSERT_EQUAL (memBuff->getLength (), 10u);
  }

  void testFind ()
  {
    *memBuff << (const char*)"1234567890";
    CPPUNIT_ASSERT_EQUAL (memBuff->find ('5'), 4u);

    CPPUNIT_ASSERT_EQUAL (memBuff->find ('5', 4), 4u);

    CPPUNIT_ASSERT_EQUAL (memBuff->find ('5', 6), (u_int)-1);


  }

  void testReplace ()
  {
    *memBuff << (const char*)"1234567890";
    CPPUNIT_ASSERT_EQUAL (memBuff->find ('5'), 4u);

    memBuff->replace ('5', '6');

    CPPUNIT_ASSERT_EQUAL (memBuff->find ('5'), (u_int)-1);
  }

  void testUintToStr ()
  {
    memBuff->uintToStr (0);

    CPPUNIT_ASSERT (strcmp (memBuff->getBuffer (), "0") == 0);

    memBuff->setLength (0);

    memBuff->uintToStr (10);
    CPPUNIT_ASSERT (strcmp (memBuff->getBuffer (), "10") == 0);

    memBuff->setLength (0);

    memBuff->uintToStr (100);
    CPPUNIT_ASSERT (strcmp (memBuff->getBuffer (), "100") == 0);
  }


  void testIntToStr ()
  {
    memBuff->intToStr (0);
    CPPUNIT_ASSERT (strcmp (memBuff->getBuffer (), "0") == 0);

    memBuff->setLength (0);

    memBuff->intToStr (10);
    CPPUNIT_ASSERT (strcmp (memBuff->getBuffer (), "10") == 0);

    memBuff->setLength (0);

    memBuff->intToStr (-100);
    CPPUNIT_ASSERT (strcmp (memBuff->getBuffer (), "-100") == 0);
  }

  void testHexCharToNumber ()
  {
    CPPUNIT_ASSERT_EQUAL (memBuff->hexCharToNumber ('1'), (unsigned char) 1);
    CPPUNIT_ASSERT_EQUAL (memBuff->hexCharToNumber ('a'), (unsigned char)  10);
  }

  void testGetPart ()
  {
    *memBuff << "0123456789abcdef";

    MemBuf dest;

    memBuff->getPart (3, 10, dest);

    CPPUNIT_ASSERT (memcmp (dest.getBuffer (), "3456789", 7) == 0);
  }

  void testIsValid ()
  {
    CPPUNIT_ASSERT_EQUAL (memBuff->isValid (), 0);

    *memBuff << "123";

    CPPUNIT_ASSERT_EQUAL (memBuff->isValid (), 1);
  }

  void testExternalBuffer ()
  {
    char buffer[256];

    memBuff->setExternalBuffer (buffer, 256);

    *memBuff << "foo";


    CPPUNIT_ASSERT (memcmp (buffer, "foo", 3) == 0);

  }

  void testGetAt ()
  {
    *memBuff << "foo";
    char val = memBuff->getAt (1);

    CPPUNIT_ASSERT_EQUAL (val, 'o');

    val = (*memBuff)[1];
    CPPUNIT_ASSERT_EQUAL (val, 'o');
  }

  void testGetBuffer ()
  {
	*memBuff << "MyServer is a powerful and easy to configure web server.";
	char szExpected[128];
	memset (szExpected, 0, 128);
	strcpy (szExpected, "MyServer is a powerful and easy to configure web server.");
	CPPUNIT_ASSERT (memcmp (memBuff->getBuffer (), szExpected, strlen (szExpected)) == 0);
  }
};


CPPUNIT_TEST_SUITE_REGISTRATION ( TestMemBuffer );
