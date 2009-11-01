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

#include <ctype.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

#include <include/conf/security/security_manager.h>
#include <include/conf/security/xml_validator.h>

#include <string.h>

#include <iostream>
using namespace std;

class TestXmlValidator : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestXmlValidator );
  CPPUNIT_TEST ( testGetPermissionMask );
  CPPUNIT_TEST ( testGetPermissionMaskImpl );
  CPPUNIT_TEST ( testGetValue );
  CPPUNIT_TEST_SUITE_END ();

  XmlValidator* xmlValidator;
public:
  void setUp ()
  {
    xmlValidator = new XmlValidator ();
  }

  void tearDown ()
  {
    delete xmlValidator;
  }

  void testGetValue ()
  {
    string val ("value");
    HashMap<string, SecurityDomain*> hashedDomains;

    CPPUNIT_ASSERT_EQUAL (xmlValidator->getValue (&hashedDomains, val), (string*)NULL);

  }

  void testGetPermissionMaskImpl ()
  {
    string val ("value");
    SecurityToken secToken;
    secToken.setResource (&val);
    secToken.setResource (&val);
    secToken.setDirectory (&val);
    secToken.setSysDirectory (&val);
    CPPUNIT_ASSERT_EQUAL (xmlValidator->getPermissionMaskImpl (&secToken, NULL, NULL), 0);

  }

  void testGetPermissionMask ()
  {
    SecurityToken secToken;
    string val ("value");
    secToken.setResource (&val);
    secToken.setDirectory (&val);
    secToken.setSysDirectory (&val);

    CPPUNIT_ASSERT_EQUAL (xmlValidator->getPermissionMask (&secToken, (SecurityDomain**) NULL, NULL), 0);
    CPPUNIT_ASSERT_EQUAL (xmlValidator->getPermissionMask (&secToken, (list<SecurityDomain*>*) NULL, NULL), 0);
  }
};


CPPUNIT_TEST_SUITE_REGISTRATION ( TestXmlValidator );
