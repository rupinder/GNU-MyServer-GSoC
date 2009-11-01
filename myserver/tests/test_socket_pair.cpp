/*
 MyServer
 Copyright (C) 2009 Free Software Foundation, Inc.
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

#include <include/base/socket_pair/socket_pair.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

#include <string.h>

class TestSocketPair : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestSocketPair );
  CPPUNIT_TEST ( testCreateClose );
  CPPUNIT_TEST ( testInverted );
  CPPUNIT_TEST ( testWriteRead );
  CPPUNIT_TEST_SUITE_END ();

public:

  void setUp ()
  {
  }

  void tearDown ()
  {
  }

  void testInverted ()
  {
    SocketPair sp;
    SocketPair inverted;
    sp.create ();
    sp.inverted (inverted);

    CPPUNIT_ASSERT_EQUAL (sp.getFirstHandle (), inverted.getSecondHandle ());
    CPPUNIT_ASSERT_EQUAL (sp.getSecondHandle (), inverted.getFirstHandle ());

    sp.close ();
  }

  void testCreateClose ()
  {
    SocketPair sp;
    int ret = sp.create ();

    CPPUNIT_ASSERT_EQUAL (ret, 0);

    ret = sp.close ();

    CPPUNIT_ASSERT_EQUAL (ret, 0);
  }

  void testWriteRead ()
  {
    char inBuffer[] = "Hello World!";
    char outBuffer[256];
    u_long nbw, nbr;
    int ret;
    SocketPair writeSock;
    SocketPair readSock;

    writeSock.create ();
    writeSock.inverted (readSock);


    ret = writeSock.write (inBuffer, strlen (inBuffer) + 1, &nbw);

    CPPUNIT_ASSERT_EQUAL (ret, 0);

    ret = readSock.read (outBuffer, 256, &nbr);

    CPPUNIT_ASSERT_EQUAL (ret, 0);


    CPPUNIT_ASSERT_EQUAL (strcmp (inBuffer, outBuffer), 0);

    writeSock.close ();
  }

};

CPPUNIT_TEST_SUITE_REGISTRATION ( TestSocketPair );
