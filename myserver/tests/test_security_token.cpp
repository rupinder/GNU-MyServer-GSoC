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

#include <include/conf/security/security_manager.h>
#include <include/conf/security/security_token.h>
#include <include/conf/vhost/vhost.h>
#include <include/server/server.h>

#include <string.h>

#include <iostream>
using namespace std;


class TestSecurityToken : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( TestSecurityToken );
  CPPUNIT_TEST ( testUser);
  CPPUNIT_TEST ( testPassword);
	CPPUNIT_TEST ( testValues );
  CPPUNIT_TEST ( testDirectory );
  CPPUNIT_TEST ( testSysDirectory );
  CPPUNIT_TEST ( testResource );
  CPPUNIT_TEST ( testMask );
  CPPUNIT_TEST ( testProvidedMask );
  CPPUNIT_TEST ( testNeededPassword );
  CPPUNIT_TEST ( testDone );
  CPPUNIT_TEST ( testAuthenticated );
  CPPUNIT_TEST ( testServer );
  CPPUNIT_TEST ( testVhost );
  CPPUNIT_TEST_SUITE_END();

  SecurityToken* securityToken;
public:
  void setUp()
  {
    securityToken = new SecurityToken();
  }

  void tearDown()
  {
    delete securityToken;
  }


  void testUser()
  {
    string user ("foo");
    securityToken->setUser (user);
    CPPUNIT_ASSERT_EQUAL (securityToken->getUser ().compare (user), 0);
  }

  void testPassword()
  {
    string password ("foo");
    securityToken->setPassword (password);
    CPPUNIT_ASSERT_EQUAL (securityToken->getPassword ().compare (password), 0);
  }

	void testValues ()
  {
    CPPUNIT_ASSERT (securityToken->getValues ());
  }

  void testDirectory ()
  {
    string dir ("foo");
    securityToken->setDirectory (&dir);
    CPPUNIT_ASSERT_EQUAL (securityToken->getDirectory (), &dir);
  }

  void testSysDirectory ()
  {
    string sysDir ("foo");
    securityToken->setSysDirectory (&sysDir);
    CPPUNIT_ASSERT_EQUAL (securityToken->getSysDirectory (), &sysDir);
  }

  void testResource ()
  {
    string resource ("foo");
    securityToken->setResource (&resource);
    CPPUNIT_ASSERT_EQUAL (securityToken->getResource (), &resource);
  }

  void testMask ()
  {
    int mask = MYSERVER_PERMISSION_READ;
    securityToken->setMask (mask);
    CPPUNIT_ASSERT_EQUAL (securityToken->getMask (), mask);
  }

  void testProvidedMask ()
  {
    int mask = MYSERVER_PERMISSION_READ;
    securityToken->setProvidedMask (mask);
    CPPUNIT_ASSERT_EQUAL (securityToken->getProvidedMask (), mask);
  }
  
  void testNeededPassword ()
  {
    string password ("foo");
    securityToken->setNeededPassword (password);
    CPPUNIT_ASSERT_EQUAL (securityToken->getNeededPassword ().compare (password), 0);
  }

  void testDone ()
  {
    securityToken->setDone (true);
    CPPUNIT_ASSERT_EQUAL (securityToken->isDone (), true);

    securityToken->setDone (false);
    CPPUNIT_ASSERT_EQUAL (securityToken->isDone (), false);
  }

  void testAuthenticated ()
  {
    securityToken->setAuthenticated (true);
    CPPUNIT_ASSERT_EQUAL (securityToken->isAuthenticated (), true);

    securityToken->setAuthenticated (false);
    CPPUNIT_ASSERT_EQUAL (securityToken->isAuthenticated (), false);
  }

  void testServer ()
  {
    Server *s = (Server*)0x10; //XXX: Dirty but it avoids to use the Singleton instance.
    securityToken->setServer (s);
    CPPUNIT_ASSERT_EQUAL (securityToken->getServer (), s);
  }
  
  void testVhost ()
  {
    Vhost *v = (Vhost*)0x100; //XXX: Dirty.
    securityToken->setVhost (v);
    CPPUNIT_ASSERT_EQUAL (securityToken->getVhost (), v);
  }

};


CPPUNIT_TEST_SUITE_REGISTRATION( TestSecurityToken );
