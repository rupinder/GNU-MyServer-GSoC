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
#include <include/conf/security/validator_factory.h>
#include <include/conf/security/validator.h>

#include <string.h>

#include <iostream>
using namespace std;


class TestValidatorFactory : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestValidatorFactory );
  CPPUNIT_TEST ( testGetValidator );
  CPPUNIT_TEST ( testAddValidator );
  CPPUNIT_TEST ( testIsValidatorPresent );
  CPPUNIT_TEST_SUITE_END ();

  ValidatorFactory* factory;
public:
  void setUp ()
  {
    factory = new ValidatorFactory ();
  }

  void tearDown ()
  {
    delete factory;
  }

  void testGetValidator ()
  {
    string val ("foo");
    CPPUNIT_ASSERT_EQUAL (factory->getValidator (val), (Validator*)NULL);

  }

  void testAddValidator ()
  {
    string val ("bar");
    Validator *validator = new Validator;

    Validator* old = factory->addValidator (val, validator);

    CPPUNIT_ASSERT_EQUAL (old, (Validator*)NULL);
    CPPUNIT_ASSERT (factory->getValidator (val));
  }

  void testIsValidatorPresent ()
  {
    string val ("bar");
    Validator *validator = new Validator;


    CPPUNIT_ASSERT_EQUAL (factory->isValidatorPresent (val), false);

    factory->addValidator (val, validator);

    CPPUNIT_ASSERT_EQUAL (factory->isValidatorPresent (val), true);
  }
};


CPPUNIT_TEST_SUITE_REGISTRATION ( TestValidatorFactory );
