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

#include <string.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

#include <include/protocol/http/http_req_security_domain.h>

#include <iostream>
using namespace std;

class TestHttpReqSecurityDomain : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( TestHttpReqSecurityDomain );
  CPPUNIT_TEST( testConstructor );
  CPPUNIT_TEST( testGetValue );
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp()
  {

  }

  void tearDown()
  {

  }


  void testConstructor ()
  {
    HttpRequestHeader req;
    HttpReqSecurityDomain *dom = new  HttpReqSecurityDomain (&req);

    CPPUNIT_ASSERT_EQUAL (&req, dom->getRequest ());

    delete dom;
  }


  void testGetValue ()
  {
    HttpRequestHeader req;
    const char *name = "My-Header";
    const char *value = "foo-bar";
    string nameStr (name);
    HttpReqSecurityDomain *dom = new  HttpReqSecurityDomain (&req);

    req.setValue (name, value);

    string *ret = dom->getValue (nameStr);

    CPPUNIT_ASSERT_EQUAL (ret->compare (value), 0);

    delete dom;
  }

};


CPPUNIT_TEST_SUITE_REGISTRATION( TestHttpReqSecurityDomain );
