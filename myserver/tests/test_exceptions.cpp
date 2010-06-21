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
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

#include "../include/base/exceptions/exceptions.h"
#include "../include/base/exceptions/checked.h"

#include <errno.h>

using namespace std;

class TestExceptions : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestExceptions );
  CPPUNIT_TEST (testWrongCwd);
  CPPUNIT_TEST (testErrno);
  CPPUNIT_TEST (testGetBacktrace);
  CPPUNIT_TEST_SUITE_END ();

  DIR *foodir;
  char *buf;

public:
  void setUp ()
  {
    foodir = NULL;
    buf = NULL;
  }

  void tearDown () {}

  void testGetBacktrace ()
  {
    try
      {
        errno = EINVAL;
        checked::raiseException ();

        CPPUNIT_FAIL ("Exception not raised");
      }
    catch (AbstractServerException & e)
      {
        char **bt = e.getBacktrace ();
#ifdef HAVE_BACKTRACE_SYMBOLS
        CPPUNIT_ASSERT (bt);
#endif
      }
  }

  void testErrno ()
  {
    bool success = false;
    errno = EINVAL;
    try
      {
        checked::raiseException ();
      }
    catch (AbstractServerException & ase)
      {
        success = ase.getErrno () == EINVAL;
      }

    CPPUNIT_ASSERT (success);
  }

  void testWrongCwd ()
  {
    bool success = false;
    try
      {
        checked::getcwd (buf, 1);
        CPPUNIT_FAIL ("The exception in testWrongCwd wasn't thrown or caught");
      }
    catch (...)
      {
	success = true;
      }

    if (! success)
      CPPUNIT_FAIL ("No exception was thrown!");
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION (TestExceptions);
