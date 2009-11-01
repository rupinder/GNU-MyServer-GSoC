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
#include <include/conf/security/security_manager.h>
#include <include/conf/security/security_domain.h>

#include <string.h>

#include <iostream>
using namespace std;


class TestSecurityDomain : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestSecurityDomain );
  CPPUNIT_TEST ( testGetValue );
  CPPUNIT_TEST ( testGetName );
  CPPUNIT_TEST_SUITE_END ();

  SecurityDomain* securityDomain;
public:
  void setUp ()
  {
    securityDomain = new SecurityDomain ();
  }

  void tearDown ()
  {
    delete securityDomain;
  }

  void testGetName ()
  {
    string val ("value");
    CPPUNIT_ASSERT_EQUAL (securityDomain->getName ().compare (""), 0);
  }

  void testGetValue ()
  {
    string val ("value");
    CPPUNIT_ASSERT_EQUAL (securityDomain->getValue (val), (string*)NULL);
  }

};


CPPUNIT_TEST_SUITE_REGISTRATION ( TestSecurityDomain );
