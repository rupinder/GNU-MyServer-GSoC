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
  CPPUNIT_TEST (testCatch1);
  CPPUNIT_TEST (testCatch2);
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

  void testCatch1 ()
  {
    try
      {
        checked::closedir (foodir);
        CPPUNIT_FAIL ("The exception in testCatch1 wasn't thrown or caught");
      }
    catch (ArgumentListException& e)
      {
        CPPUNIT_ASSERT (e.getErrno () == EINVAL);
      }
    catch (...)
      {
        CPPUNIT_FAIL ("The wrong exception in testCatch1 was thrown");
      }
  }

  void testCatch2 ()
  {
    try
      {
        char *p = checked::getcwd (buf, 1);
        CPPUNIT_FAIL ("The exception in testCatch2 wasn't thrown or caught");
      }
    catch (OverflowException& e)
      {
        CPPUNIT_ASSERT (e.getErrno () == ERANGE);
      }
    catch (...)
      {
        CPPUNIT_FAIL ("The wrong exception in testCatch2 was thrown");
      }
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION (TestExceptions);
