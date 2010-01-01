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

#include "stdafx.h"

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

#include <../include/protocol/url.h>

#include <string.h>
#include <unistd.h>
#include <string>

using namespace std;

class TestUrl : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestUrl );
  CPPUNIT_TEST ( testUrl );
  CPPUNIT_TEST ( testUrlWithCredentials );
  CPPUNIT_TEST ( testUrlWithQuery );
  CPPUNIT_TEST ( testUrlWithoutPort );
  CPPUNIT_TEST_SUITE_END ();

public:
  void setUp ( )
  {

  }

  void tearDown ( )
  {

  }

  void testUrlWithCredentials ()
  {
    const char *urlStr = "http://root@foo.bar:8080/what/i/want";
    Url url (urlStr, 80);

    CPPUNIT_ASSERT (!url.getCredentials ().compare ("root"));
    CPPUNIT_ASSERT (!url.getHost ().compare ("foo.bar"));
    CPPUNIT_ASSERT (!url.getResource ().compare ("what/i/want"));
    CPPUNIT_ASSERT (!url.getProtocol ().compare ("http"));
    CPPUNIT_ASSERT (!url.getQuery ().compare (""));
    CPPUNIT_ASSERT_EQUAL (url.getPort (), (u_short)8080);
  }

  void testUrlWithQuery ()
  {
    const char *urlStr = "http://foo.bar:8080/what/i/want?my_query";
    Url url (urlStr, 80);

    CPPUNIT_ASSERT (!url.getCredentials ().compare (""));
    CPPUNIT_ASSERT (!url.getHost ().compare ("foo.bar"));
    CPPUNIT_ASSERT (!url.getResource ().compare ("what/i/want"));
    CPPUNIT_ASSERT (!url.getProtocol ().compare ("http"));
    CPPUNIT_ASSERT (!url.getQuery ().compare ("my_query"));
    CPPUNIT_ASSERT_EQUAL (url.getPort (), (u_short)8080);
  }

  void testUrl ()
  {
    const char *urlStr = "http://foo.bar:8080/what/i/want";
    Url url (urlStr, 80);

    CPPUNIT_ASSERT (!url.getCredentials ().compare (""));
    CPPUNIT_ASSERT (!url.getHost ().compare ("foo.bar"));
    CPPUNIT_ASSERT (!url.getResource ().compare ("what/i/want"));
    CPPUNIT_ASSERT (!url.getProtocol ().compare ("http"));
    CPPUNIT_ASSERT (!url.getQuery ().compare (""));
    CPPUNIT_ASSERT_EQUAL (url.getPort (), (u_short)8080);
  }

  void testUrlWithoutPort ()
  {
    const char *urlStr = "http://foo.bar/what/i/want";
    Url url (urlStr, 80);

    CPPUNIT_ASSERT (!url.getCredentials ().compare (""));
    CPPUNIT_ASSERT (!url.getHost ().compare ("foo.bar"));
    CPPUNIT_ASSERT (!url.getResource ().compare ("what/i/want"));
    CPPUNIT_ASSERT (!url.getProtocol ().compare ("http"));
    CPPUNIT_ASSERT_EQUAL (url.getPort (), (u_short)80);
  }

};

CPPUNIT_TEST_SUITE_REGISTRATION ( TestUrl );
