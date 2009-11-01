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

#include <list>
#include <string>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

#include <include/log/stream/socket_stream_creator.h>
#include <include/filter/filters_factory.h>
#include <include/base/socket/socket.h>

class TestSocketStreamCreator : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE (TestSocketStreamCreator);
  CPPUNIT_TEST (testCreation);
  CPPUNIT_TEST (testGetPort);
  CPPUNIT_TEST (testGetHost);
  CPPUNIT_TEST_SUITE_END ();
public:
  void setUp ()
  {
    ssc = new SocketStreamCreator ();
    ff = new FiltersFactory ();
  }

  void testCreation ()
  {
    list<string> filters;
    LogStream* ls = ssc->create (ff, "127.0.0.1:0", filters, 0);
    CPPUNIT_ASSERT (!ls);
  }

  void testGetPort ()
  {
    CPPUNIT_ASSERT (ssc->getPort ("foohost:8081") == 8081);
  }

  void testGetHost ()
  {
    CPPUNIT_ASSERT (!ssc->getHost ("foohost:8081").compare ("foohost"));
  }

  void tearDown ()
  {
    delete ssc;
    delete ff;
  }
private:
  SocketStreamCreator* ssc;
  FiltersFactory* ff;
};

CPPUNIT_TEST_SUITE_REGISTRATION (TestSocketStreamCreator);
