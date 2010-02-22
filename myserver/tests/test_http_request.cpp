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

#include <string.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

#include <include/protocol/http/http_headers.h>
#include <include/connection/connection.h>
#include <include/protocol/http/http_request.h>

#include <iostream>
using namespace std;

class TestHttpRequest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE (TestHttpRequest);
  CPPUNIT_TEST (testSimpleHeader);
  CPPUNIT_TEST (testAuthBasic);
  CPPUNIT_TEST (testAuthDigest);
  CPPUNIT_TEST (testRange);
  CPPUNIT_TEST (testIncompleteHeader);
  CPPUNIT_TEST (testDefaultHttpRequest);
  CPPUNIT_TEST (testValidRequest);
  CPPUNIT_TEST (testKeepAlive);
  CPPUNIT_TEST (testResetHttpRequest);
  CPPUNIT_TEST_SUITE_END ();

public:
  void setUp ()
  {

  }

  void tearDown ()
  {

  }


  void testKeepAlive ()
  {
    HttpRequestHeader header;
    Connection connection;
    u_long requestLength;
    const char * requestStr;
    int ret;
#define BUILD_KEEPALIVE_REQ(X) "GET / HTTP/1.1\r\n" \
      "Connection: " X "\r\n"                       \
      "Host: localhost\r\n\r\n"

    requestStr = BUILD_KEEPALIVE_REQ ("Keep-Alive");
    ret = HttpHeaders::buildHTTPRequestHeaderStruct (requestStr,
                                                     strlen (requestStr),
                                                     &requestLength,
                                                     &header,
                                                     &connection);

    CPPUNIT_ASSERT (header.isKeepAlive ());

    requestStr = BUILD_KEEPALIVE_REQ ("close");
    HttpHeaders::resetHTTPRequest (&header);
    ret = HttpHeaders::buildHTTPRequestHeaderStruct (requestStr,
                                                     strlen (requestStr),
                                                     &requestLength,
                                                     &header,
                                                     &connection);

    CPPUNIT_ASSERT (! header.isKeepAlive ());

    requestStr = BUILD_KEEPALIVE_REQ ("keep-Alive");
    HttpHeaders::resetHTTPRequest (&header);
    ret = HttpHeaders::buildHTTPRequestHeaderStruct (requestStr,
                                                     strlen (requestStr),
                                                     &requestLength,
                                                     &header,
                                                     &connection);

    CPPUNIT_ASSERT (header.isKeepAlive ());

#undef BUILD_KEEPALIVE_REQ
  }

  void testSimpleHeader ()
  {

    HttpRequestHeader header;
    Connection connection;
    const char * requestStr = "GET /resource?args HTTP/1.1\r\nHost: localhost\r\n\r\n";
    u_long  bufferLength = strlen (requestStr);
    u_long requestLength;
    int ret = HttpHeaders::buildHTTPRequestHeaderStruct (requestStr,
                                                        bufferLength,
                                                        &requestLength,
                                                        &header,
                                                        &connection);

    CPPUNIT_ASSERT_EQUAL (ret, 200);
    CPPUNIT_ASSERT (header.cmd.compare ("GET") == 0);
    CPPUNIT_ASSERT (header.ver.compare ("HTTP/1.1") == 0);
    CPPUNIT_ASSERT (header.uri.compare ("/resource") == 0);
    CPPUNIT_ASSERT (header.uriOpts.compare ("args") == 0);

    CPPUNIT_ASSERT (header.getValue ("Host", 0));
    CPPUNIT_ASSERT (header.getValue ("Host", 0)->compare ("localhost") == 0);
  }


  void testRange ()
  {
    HttpRequestHeader header;
    Connection connection;
    const char * requestStr;
    u_long  bufferLength;
    u_long requestLength;
    int ret;

    requestStr = "GET /resource HTTP/1.1\r\nHost: localhost\r\nRange: bytes=10-20\r\n\r\n";
    bufferLength = strlen (requestStr);

    ret = HttpHeaders::buildHTTPRequestHeaderStruct (requestStr,
                                                    bufferLength,
                                                    &requestLength,
                                                    &header,
                                                    &connection);


    CPPUNIT_ASSERT (ret == 200);
    CPPUNIT_ASSERT (header.rangeByteBegin == 10);
    CPPUNIT_ASSERT (header.rangeByteEnd == 20);
    CPPUNIT_ASSERT (header.rangeType.compare ("bytes") == 0);


    requestStr = "GET /resource HTTP/1.1\r\nHost: localhost\r\nRange: bytes=10-\r\n\r\n";
    bufferLength = strlen (requestStr);

    ret = HttpHeaders::buildHTTPRequestHeaderStruct (requestStr,
                                                    bufferLength,
                                                    &requestLength,
                                                    &header,
                                                    &connection);


    CPPUNIT_ASSERT (ret == 200);
    CPPUNIT_ASSERT (header.rangeByteBegin == 10);
    CPPUNIT_ASSERT (header.rangeByteEnd == 0);
    CPPUNIT_ASSERT (header.rangeType.compare ("bytes") == 0);
  }

  void testValidRequest ()
  {
    const char *request = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
    int ret;
    u_long nLines;
    u_long nChars;
    ret = HttpHeaders::validHTTPRequest (request,
                                        strlen (request),
                                        &nLines,
                                        &nChars);

    CPPUNIT_ASSERT_EQUAL (ret, 200);
  }

  void testIncompleteHeader ()
  {
    const char *incompleteRequest = "GET / HTTP/1.1\r\nHost: localhost";
    int ret;
    u_long nLines;
    u_long nChars;
    ret = HttpHeaders::validHTTPRequest (incompleteRequest,
                                        strlen (incompleteRequest),
                                        &nLines,
                                        &nChars);

    CPPUNIT_ASSERT_EQUAL (ret, -1);
  }


  void testAuthDigest ()
  {
    HttpRequestHeader header;
    Connection connection;
    const char *requestStr;
    u_long  bufferLength;
    u_long requestLength;
    int ret;

    requestStr = "GET /resource HTTP/1.1\r\nHost: localhost\r\n" \
      "Authorization: Digest username=\"user\", realm=\"localhost:8080\", "\
      "nonce=\"88b29c4b136e2efee059647dbac10427\", uri=\"/res/\", " \
      "algorithm=MD5, response=\"e979c020b57cc0e86e46d5c8cad8497e\", " \
      "opaque=\"8666683506aacd900bbd5a74ac4edf68\", qop=auth, nc=00000001, " \
      "cnonce=\"c40dc807904a2357\"\r\n\r\n";
    bufferLength = strlen (requestStr);

    ret = HttpHeaders::buildHTTPRequestHeaderStruct (requestStr,
                                                    bufferLength,
                                                    &requestLength,
                                                    &header,
                                                    &connection);
    CPPUNIT_ASSERT (ret == 200);
    CPPUNIT_ASSERT (!strcmp (header.digestRealm, "localhost:8080"));
    CPPUNIT_ASSERT (!strcmp (header.digestOpaque,
                             "8666683506aacd900bbd5a74ac4edf68"));
    CPPUNIT_ASSERT (!strcmp (header.digestNonce,
                             "88b29c4b136e2efee059647dbac10427"));
    CPPUNIT_ASSERT (!strcmp (header.digestUri, "/res/"));
    CPPUNIT_ASSERT (!strcmp (header.digestUsername, "user")); 
    CPPUNIT_ASSERT (!strcmp (header.digestNc, "00000001")); 
    CPPUNIT_ASSERT (!strcmp (header.digestQop, "auth")); 
    CPPUNIT_ASSERT (!strcmp (header.digestResponse,
                             "e979c020b57cc0e86e46d5c8cad8497e")); 
    CPPUNIT_ASSERT (!strcmp (header.digestCnonce, "c40dc807904a2357")); 

  }

  void testAuthBasic ()
  {
    HttpRequestHeader header;
    Connection connection;
    const char *requestStr;
    u_long  bufferLength;
    u_long requestLength;
    int ret;

    requestStr = "GET /resource HTTP/1.1\r\nHost: localhost\r\n" \
      "Authorization: Basic dXNlcjpmcmVlZG9t\r\n\r\n";

    bufferLength = strlen (requestStr);

    ret = HttpHeaders::buildHTTPRequestHeaderStruct (requestStr,
                                                    bufferLength,
                                                    &requestLength,
                                                    &header,
                                                    &connection);
    CPPUNIT_ASSERT (ret == 200);
  }

  void testDefaultHttpRequest ()
  {
    HttpRequestHeader header;
    HttpHeaders::buildDefaultHTTPRequestHeader (&header);


    CPPUNIT_ASSERT (header.cmd.compare ("GET") == 0);
    CPPUNIT_ASSERT (header.uri.compare ("/") == 0);
    CPPUNIT_ASSERT (header.uriOpts.compare ("") == 0);
    CPPUNIT_ASSERT (header.ver.compare ("HTTP/1.1") == 0);
    CPPUNIT_ASSERT (header.uriOptsPtr == NULL);
    CPPUNIT_ASSERT (header.getValue ("NotExists", 0) == NULL);
  }


  void testResetHttpRequest ()
  {
    HttpRequestHeader header;
    HttpHeaders::resetHTTPRequest (&header);

    CPPUNIT_ASSERT (header.cmd.compare ("") == 0);
    CPPUNIT_ASSERT (header.uri.compare ("") == 0);
    CPPUNIT_ASSERT (header.uriOpts.compare ("") == 0);
    CPPUNIT_ASSERT (header.ver.compare ("") == 0);
    CPPUNIT_ASSERT (header.uriOptsPtr == NULL);
    CPPUNIT_ASSERT (header.getValue ("NotExists", 0) == NULL);
  }

};


CPPUNIT_TEST_SUITE_REGISTRATION ( TestHttpRequest );
