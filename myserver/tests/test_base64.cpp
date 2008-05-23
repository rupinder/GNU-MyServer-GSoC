/*
 MyServer
 Copyright (C) 2008 The MyServer Team
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
#include "../include/mime_utils.h"

#include <string.h>

#include <iostream>
using namespace std;

class TestBase64 : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( TestBase64 );
  CPPUNIT_TEST( testEncodeDecode );
  CPPUNIT_TEST_SUITE_END();

  CBase64Utils *base64;
public:
  void setUp()
  {
    base64 = new CBase64Utils();

  }

  void tearDown()
  {
    delete base64;
  }

  void testEncodeDecode()
  {
    int len = 0;
    char* input = (char*) "Hello world!\n";
    char* encoded = base64->Encode(input, 13);

    len = strlen(encoded);

    char* decoded = base64->Decode(encoded, &len);

    CPPUNIT_ASSERT_EQUAL(strlen(input), strlen(decoded));

    for (int i = 0; i < strlen(decoded); i++)
      CPPUNIT_ASSERT_EQUAL(input[i], decoded[i]);

  }
};


CPPUNIT_TEST_SUITE_REGISTRATION( TestBase64 );
