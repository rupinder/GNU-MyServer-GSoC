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


#include "../include/base/utility.h"
#include "../include/base/file/file.h"
#include "../include/base/file/files_utility.h"
#include "include/base/files_cache/membuf_file.h"

#include <string.h>
#include <unistd.h>
#include <string>

using namespace std;

class TestMembufFile : public CppUnit::TestFixture
{
  MemBuf *memBuf;
  MemBufFile *tfile;

  CPPUNIT_TEST_SUITE ( TestMembufFile );
  CPPUNIT_TEST (testCreateTemporaryMembufFile);
  CPPUNIT_TEST (testClose);
  CPPUNIT_TEST (testOpenFile);
  CPPUNIT_TEST (testWrite);
  CPPUNIT_TEST (testRead);
  CPPUNIT_TEST (testHandle);
  CPPUNIT_TEST (testSeek);
  CPPUNIT_TEST_SUITE_END ();

public:
  void setUp ()
  {
    memBuf = new MemBuf ();
    tfile = new MemBufFile (memBuf);
  }

  void tearDown ()
  {
    delete tfile;
    delete memBuf;
  }

  void testHandle ()
  {
    Handle handle = (Handle) 1;
    CPPUNIT_ASSERT_EQUAL (tfile->getHandle (), (Handle)-1);
    CPPUNIT_ASSERT_EQUAL (tfile->setHandle (handle), -1);
    CPPUNIT_ASSERT_EQUAL (tfile->getHandle (), (Handle)-1);
  }

  void testCreateTemporaryMembufFile ()
  {
    CPPUNIT_ASSERT_EQUAL (tfile->createTemporaryFile ("foo.dat"), -1);
  }

  void testClose ()
  {
    CPPUNIT_ASSERT_EQUAL (tfile->close (), 0);
  }

  void testOpenFile ()
  {
    const char *file = "tmp.dat";
    CPPUNIT_ASSERT_EQUAL (tfile->openFile (file, File::WRITE
                                           | File::READ
                                           | File::FILE_OPEN_ALWAYS), -1);
  }

  void testWrite ()
  {
    const char *data = "hello dudes";
    const size_t len = strlen (data);
    size_t nbw;

    CPPUNIT_ASSERT_EQUAL (tfile->getSeek (), (size_t) 0);
    CPPUNIT_ASSERT_EQUAL (tfile->write (data, len, &nbw), 0);
    CPPUNIT_ASSERT_EQUAL (len, nbw);
    CPPUNIT_ASSERT_EQUAL (tfile->getSeek (), nbw);
  }

  void testRead ()
  {
    char buffer[512];
    const char *data = "hello dudes";
    u_long len = strlen (data);
    size_t nbw, nbr;

    tfile->read (buffer, len, &nbr);
    CPPUNIT_ASSERT_EQUAL (nbr, (size_t) 0);
    CPPUNIT_ASSERT_EQUAL (tfile->write (data, len, &nbw), 0);
    CPPUNIT_ASSERT_EQUAL (tfile->getSeek (), nbw);

    CPPUNIT_ASSERT_EQUAL (tfile->seek (0), 0);
    CPPUNIT_ASSERT_EQUAL (tfile->read (buffer, 512, &nbr), 0);
    CPPUNIT_ASSERT_EQUAL (nbr, nbw);
  }

  void testSeek ()
  {
    size_t seek = 12L;
    CPPUNIT_ASSERT_EQUAL (tfile->getSeek (), (size_t) 0);
    CPPUNIT_ASSERT_EQUAL (tfile->seek (seek), 0);
    CPPUNIT_ASSERT_EQUAL (tfile->getSeek (), seek);
  }

};

CPPUNIT_TEST_SUITE_REGISTRATION (TestMembufFile);
