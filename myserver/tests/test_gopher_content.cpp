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
#include <include/protocol/gopher/gopher_content.h>
#include "memory_socket.h"

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>


class TestGopherContent : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE (TestGopherContent);
  CPPUNIT_TEST (testGopherImageContent);
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

  void testGopherImageContent ()
  {
    string fname = "test.jpg";
    string path = "root/subdir/gopher";
    string hostname = "localhost";
    string port = "70";
    string element  = "Itest.jpg\troot/subdir/gopher\tlocalhost\t70\t\n";
    GopherImage i (fname.c_str (), path.c_str (), hostname.c_str (),
                   port.c_str ());
    i.toProtocol(s);

    const char *buffer = *s;
    CPPUNIT_ASSERT_EQUAL (strncmp (element.c_str (), buffer, element.length ()),
                          0);
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION (TestGopherContent);
