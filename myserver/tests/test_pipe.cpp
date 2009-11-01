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
#include <ctype.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <include/base/pipe/pipe.h>
#include <string.h>

class TestPipe : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestPipe );
  CPPUNIT_TEST ( testCreateClose );
  CPPUNIT_TEST ( testWriteRead );
  CPPUNIT_TEST ( testInverted );
  CPPUNIT_TEST ( testWaitForData );
  CPPUNIT_TEST_SUITE_END ();

  Pipe *pipe;

public:
  void setUp ()
  {
    pipe = new Pipe ();
  }

  void tearDown ()
  {
    delete pipe;
  }


  void testCreateClose ()
  {
    int ret = pipe->create ();

    CPPUNIT_ASSERT_EQUAL (pipe->pipeTerminated (), false);

    CPPUNIT_ASSERT_EQUAL (ret, 0);

    pipe->close ();

    CPPUNIT_ASSERT_EQUAL (pipe->pipeTerminated (), true);
  }

  void testWriteRead ()
  {
    char outBuff[256];
    char inBuff[256];
    u_long nbw;
    u_long nbr;

    strcpy (outBuff, "MyServer is a powerful and easy to configure web server");

    int ret = pipe->create ();

    CPPUNIT_ASSERT_EQUAL (ret, 0);

    ret = pipe->write (outBuff, 256, &nbw);

    CPPUNIT_ASSERT_EQUAL (ret, 0);

    ret = pipe->read (inBuff, 256, &nbr);

    CPPUNIT_ASSERT_EQUAL (ret, 0);


    CPPUNIT_ASSERT_EQUAL (nbr, nbw);
    CPPUNIT_ASSERT (strcmp (outBuff, inBuff) == 0);

    pipe->close ();
  }

  void testWaitForData ()
  {
    char outBuff[256];
    u_long nbw;

    strcpy (outBuff, "MyServer is a powerful and easy to configure web server");

    int ret = pipe->create ();

    CPPUNIT_ASSERT_EQUAL (pipe->waitForData (0, 100), 0);

    ret = pipe->write (outBuff, 256, &nbw);

    CPPUNIT_ASSERT_EQUAL (pipe->waitForData (0, 100), 1);

    pipe->close ();
  }


  void testInverted ()
  {
    Pipe pipeInv;
    int ret = pipe->create ();

    CPPUNIT_ASSERT_EQUAL (ret, 0);

    pipe->inverted (pipeInv);

    CPPUNIT_ASSERT_EQUAL (pipeInv.getReadHandle (), pipe->getWriteHandle ());
    CPPUNIT_ASSERT_EQUAL (pipe->getReadHandle (), pipeInv.getWriteHandle ());

    pipe->close ();

  }
};


CPPUNIT_TEST_SUITE_REGISTRATION ( TestPipe );
