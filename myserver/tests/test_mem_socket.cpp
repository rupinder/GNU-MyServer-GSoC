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

#include "stdafx.h"
#include "stdafx.h"
#include "memory_socket.h"

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

/* Ensure the mock memory socket works.  */
class TestMemorySocket : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE (TestMemorySocket);
  CPPUNIT_TEST (testMemorySocketWrite);
  CPPUNIT_TEST (testMemorySocketRead);
  CPPUNIT_TEST_SUITE_END ();

  MemorySocket *s;

public:
  void setUp ()
  {
    s = new MemorySocket ();
    s->setThrottling (0);
  }
  void tearDown ()
  {
    delete s;
  }

  void testMemorySocketWrite ()
  {
    string temp = "foo bar foo bar foo bar";
    s->send (temp.c_str (), temp.length (), 0);
    const char* buffer = *s;
    CPPUNIT_ASSERT_EQUAL (strncmp (temp.c_str (), buffer, temp.length ()), 0);
    CPPUNIT_ASSERT_EQUAL (strncmp (temp.c_str (), buffer, s->getLength ()), 0);
  }

  void testMemorySocketRead ()
  {
    string temp = "foo bar foo bar foo bar";
    s->send (temp.c_str (), temp.length (), 0);
    CPPUNIT_ASSERT_EQUAL (strncmp (temp.c_str (), *s, temp.length ()), 0);
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION (TestMemorySocket);
