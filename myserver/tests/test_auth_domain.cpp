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

#include "myserver.h"
#include <ctype.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <include/conf/security/security_manager.h>
#include <include/conf/security/auth_domain.h>

#include <string.h>

#include <iostream>
using namespace std;

class TestAuthDomain : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestAuthDomain );
  CPPUNIT_TEST ( testGetName );
  CPPUNIT_TEST_SUITE_END ();

  SecurityToken *secToken;
  AuthDomain* authDomain;
public:
  void setUp ()
  {
    secToken = new SecurityToken ();
    authDomain = new AuthDomain (secToken);
  }

  void tearDown ()
  {
    delete authDomain;
    delete secToken;
  }

  void testGetName ()
  {
    CPPUNIT_ASSERT_EQUAL (authDomain->getName ().compare (""), 0);
  }

};


CPPUNIT_TEST_SUITE_REGISTRATION ( TestAuthDomain );
