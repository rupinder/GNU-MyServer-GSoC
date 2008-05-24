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

#include <string.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

#include "../include/http_headers.h"
#include "../include/connection.h"
#include "../include/http_response.h"

#include <iostream>
using namespace std;

class TestHttpResponse : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( TestHttpResponse );
  CPPUNIT_TEST( testSimpleHeader );
  CPPUNIT_TEST( testValidResponse );
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp()
  {

  }

  void tearDown()
  {

  }

  void testSimpleHeader()
  {
    HttpResponseHeader header;
    const char * responseStr = "HTTP/1.1 200 Success\r\nContent-Length: 0\r\n\r\n";
    u_long nbtr;
    int ret = HttpHeaders::buildHTTPResponseHeaderStruct(responseStr,
                                                         &header,
                                                         &nbtr);

    CPPUNIT_ASSERT_EQUAL(header.httpStatus, (int)200);
    CPPUNIT_ASSERT(header.contentLength.compare("0") == 0);
    CPPUNIT_ASSERT(ret != 0);
  }

  void testValidResponse()
  {
    HttpResponseHeader header;
    const char * responseStr = "HTTP/1.1 200 Success\r\nContent-Length: 0\r\n\r\n";
    u_long nLinesptr;
    u_long ncharsptr;

    int ret = HttpHeaders::validHTTPResponse(responseStr,
                                             &nLinesptr,
                                             &ncharsptr);

    CPPUNIT_ASSERT(ret != 0);
  }
  


  
};


CPPUNIT_TEST_SUITE_REGISTRATION( TestHttpResponse );
