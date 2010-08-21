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

#include <include/filter/memory_stream.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <string.h>

class TestMemStream : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestMemStream );
  CPPUNIT_TEST ( testRead );
  CPPUNIT_TEST ( testWrite );
  CPPUNIT_TEST ( testFlush );
  CPPUNIT_TEST ( testRefresh );
  CPPUNIT_TEST ( testAvailableToRead );
  CPPUNIT_TEST_SUITE_END ();

  MemoryStream *stream;

public:
  void setUp ()
  {
    stream = new MemoryStream ();
  }

  void tearDown ()
  {
    delete stream;
  }


  /* Helper method.  */
  u_long addSomeData (MemoryStream* s)
  {
    size_t nbw;

    char data[512] = "This program is free software; you can redistribute it and/or modify\n\
                      it under the terms of the GNU General Public License as published by\n\
                      the Free Software Foundation; either version 3 of the License, or\n\
                      (at your option) any later version.\n";

    s->write (data, 512, &nbw);

    return nbw;
  }

  void testAvailableToRead ()
  {
    u_long size = addSomeData (stream);

    CPPUNIT_ASSERT_EQUAL (size, (u_long) stream->availableToRead ());
  }

  void testRead ()
  {
    size_t nbr;
    u_long size = addSomeData (stream);

    char buffer[20];

    CPPUNIT_ASSERT_EQUAL (size, (u_long) stream->availableToRead ());

    stream->read (buffer, 20, &nbr);

    CPPUNIT_ASSERT_EQUAL (size - 20u, (u_long) stream->availableToRead ());
  }


  void testRefresh ()
  {
    size_t nbr;
    u_long size = addSomeData (stream);

    char buffer[20];

    CPPUNIT_ASSERT_EQUAL (size, (u_long) stream->availableToRead ());

    stream->read (buffer, 20, &nbr);
    stream->refresh ();

    CPPUNIT_ASSERT_EQUAL ((u_long) stream->availableToRead (), 0ul);
  }


  void testWrite ()
  {
    size_t nbw;

    int ret = stream->write ("hello world!", 12, &nbw);

    CPPUNIT_ASSERT_EQUAL (ret, 0);

    CPPUNIT_ASSERT_EQUAL ((u_long) stream->availableToRead (), 12ul);
  }

  void testFlush ()
  {
    size_t nbw;
    addSomeData (stream);
    int ret = stream->flush (&nbw);

    CPPUNIT_ASSERT (nbw >= 0);
    CPPUNIT_ASSERT_EQUAL (ret, 0);
  }


};


CPPUNIT_TEST_SUITE_REGISTRATION ( TestMemStream );
