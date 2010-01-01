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
#include <ctype.h>

#include <string.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

#include <include/protocol/http/http_headers.h>
#include <include/connection/connection.h>
#include <include/protocol/http/http_response.h>

#include <iostream>
using namespace std;

class TestHttpResponse : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE (TestHttpResponse);
  CPPUNIT_TEST (testSimpleHeader);
  CPPUNIT_TEST (testInvalidResponse);
  CPPUNIT_TEST (testJoinField);
  CPPUNIT_TEST (testValidResponse);
  CPPUNIT_TEST (testReset);
  CPPUNIT_TEST (testResponseLength);
  CPPUNIT_TEST_SUITE_END ();

public:
  void setUp ()
  {

  }

  void tearDown ()
  {

  }

  void testSimpleHeader ()
  {
    HttpResponseHeader header;
    const char * responseStr = "HTTP/1.1 200 Success\r\nContent-Length: 0\r\n\r\n";
    u_long nbtr;
    int ret = HttpHeaders::buildHTTPResponseHeaderStruct (responseStr,
                                                         &header,
                                                         &nbtr);

    CPPUNIT_ASSERT_EQUAL (header.httpStatus, (int)200);
    CPPUNIT_ASSERT (header.contentLength.compare ("0") == 0);
    CPPUNIT_ASSERT (ret != 0);
  }

  void testJoinField ()
  {
    HttpResponseHeader header;
    u_long nbtr;
    const char * responseStr =
      "HTTP/1.1 200 Success\r\n\
Content-Length: 0\r\n\
ToJoin: a\r\n\
ToJoin: b\r\n";

    int ret = HttpHeaders::buildHTTPResponseHeaderStruct (responseStr, &header, &nbtr);
    CPPUNIT_ASSERT (ret != 0);

    string *v = header.getValue ("ToJoin", NULL);
    CPPUNIT_ASSERT (v);

    CPPUNIT_ASSERT (v->find ("a") != string::npos);
    CPPUNIT_ASSERT (v->find ("b") != string::npos);
    CPPUNIT_ASSERT (v->find (",") != string::npos);
    CPPUNIT_ASSERT (v->find ("x") == string::npos);
  }

  void testValidResponse ()
  {
    HttpResponseHeader header;
    const char * responseStr = "HTTP/1.1 200 Success\r\nContent-Length: 0\r\n\r\n";
    u_long nLinesptr;
    u_long ncharsptr;

    int ret = HttpHeaders::validHTTPResponse (responseStr,
                                             &nLinesptr,
                                             &ncharsptr);

    CPPUNIT_ASSERT (ret != 0);
  }

  void testInvalidResponse ()
  {
    HttpResponseHeader header;
    const char * responseStr = "Not really HTTP response";
    u_long nLinesptr;
    u_long ncharsptr;
    int ret;

    ret = HttpHeaders::validHTTPResponse (responseStr,
                                         &nLinesptr,
                                         &ncharsptr);
    CPPUNIT_ASSERT_EQUAL (ret, 0);

    ret = HttpHeaders::validHTTPResponse (NULL,
                                         &nLinesptr,
                                         &ncharsptr);
    CPPUNIT_ASSERT_EQUAL (ret, 0);
  }

  void testReset ()
  {
    HttpResponseHeader header;

    HttpHeaders::resetHTTPResponse (&header);

    CPPUNIT_ASSERT (header.ver.length () == 0);
    CPPUNIT_ASSERT (header.contentLength.length () == 0);
    CPPUNIT_ASSERT (header.errorType.length () == 0);
  }

  void testStatusType ()
  {
    HttpResponseHeader header;

    header.httpStatus = 100;
    CPPUNIT_ASSERT_EQUAL (header.getStatusType (), HttpResponseHeader::INFORMATIONAL);

    header.httpStatus = 200;
    CPPUNIT_ASSERT_EQUAL (header.getStatusType (), HttpResponseHeader::SUCCESSFUL);

    header.httpStatus = 300;
    CPPUNIT_ASSERT_EQUAL (header.getStatusType (), HttpResponseHeader::REDIRECTION);

    header.httpStatus = 400;
    CPPUNIT_ASSERT_EQUAL (header.getStatusType (), HttpResponseHeader::CLIENT_ERROR);

    header.httpStatus = 500;
    CPPUNIT_ASSERT_EQUAL (header.getStatusType (), HttpResponseHeader::SERVER_ERROR);
  }

  void testResponseLength ()
  {
    char buffer[512];
    HttpResponseHeader response;
    response.setValue ("foo", "bar");
    response.setValue ("baz", "foo");
    u_long len = HttpHeaders::buildHTTPResponseHeader (buffer, &response);
    CPPUNIT_ASSERT_EQUAL (len, (u_long) strlen (buffer));
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION ( TestHttpResponse );
