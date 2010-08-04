/*
  MyServer
  Copyright (C) 2009, 2010 Free Software Foundation, Inc.
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
#include <include/base/read_directory/rec_read_directory.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

class TestRecReadDirectory : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestRecReadDirectory );
  CPPUNIT_TEST ( testTreeGen );
  CPPUNIT_TEST ( testFileInfo );
  CPPUNIT_TEST ( testFilePath );
  CPPUNIT_TEST_SUITE_END ();

public:
  void setUp () {}
  void tearDown () {}

  /* Generate a tree.
   */
  void testTreeGen ()
  {
    RecReadDirectory rec;
    rec.fileTreeGenerate ("web/cgi-bin");

    CPPUNIT_ASSERT (rec.nextMember () != NULL);
  }

  /* Check file info.
   */
  void testFileInfo ()
  {
    RecReadDirectory rec;
    rec.fileTreeGenerate ("web/cgi-bin");

    CPPUNIT_ASSERT (rec.getInfo () <= 14 && rec.getInfo () >= 0);
  }

  /* Check file path.
   */
  void testFilePath ()
  {
    RecReadDirectory rec;
    rec.fileTreeGenerate ("web/cgi-bin");

    CPPUNIT_ASSERT (strlen (rec.getPath ()) > 0);
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION ( TestRecReadDirectory );
