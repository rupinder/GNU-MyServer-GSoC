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

#include <string.h>
#include <unistd.h>
#include <string>

using namespace std;

class TestFile : public CppUnit::TestFixture
{
  File *tfile;
  string fname;

  CPPUNIT_TEST_SUITE (TestFile);

  CPPUNIT_TEST (testCreateTemporaryFile);
  CPPUNIT_TEST (testOnFile);
  CPPUNIT_TEST (testBinary);
  CPPUNIT_TEST (testTruncate);
  CPPUNIT_TEST (testCreationTime);
  CPPUNIT_TEST (testLastAccessTime);
  CPPUNIT_TEST (testLastModTime);
  CPPUNIT_TEST (testSeek);
  CPPUNIT_TEST_SUITE_END ();

  int openHelper ()
  {
   return  tfile->openFile (fname.c_str (), File::WRITE
                            | File::READ
                            | File::FILE_CREATE_ALWAYS);
  }
public:
  void setUp ()
  {
    FilesUtility::temporaryFileName (0, fname);
    tfile = new File;
  }

  void tearDown ()
  {
    delete tfile;
    FilesUtility::deleteFile (fname);
  }

  void testCreateTemporaryFile ()
  {
    CPPUNIT_ASSERT_EQUAL (tfile->createTemporaryFile (fname.c_str ()), 0);

    CPPUNIT_ASSERT_EQUAL (tfile->close (), 0);
  }

  void testOnFile ()
  {
    const char *buf = "HELLO myWORLD";
    char readbuf[512];
    size_t bufLen = strlen (buf);
    u_long nbw;
    u_long nbr;

    CPPUNIT_ASSERT_EQUAL (openHelper (), 0);

    CPPUNIT_ASSERT_EQUAL (tfile->writeToFile (buf, bufLen, &nbw), 0);

    memset (readbuf, 0, bufLen);

    CPPUNIT_ASSERT_EQUAL (tfile->seek (0), 0);

    CPPUNIT_ASSERT_EQUAL (tfile->read (readbuf, bufLen, &nbr), 0);

    CPPUNIT_ASSERT (nbr > 0);

    for (size_t i = 0; i < bufLen; i++)
      CPPUNIT_ASSERT_EQUAL (buf[i], readbuf[i]);

    CPPUNIT_ASSERT_EQUAL (tfile->getFileSize (), nbr);
    CPPUNIT_ASSERT_EQUAL (tfile->close (), 0);

    FilesUtility::deleteFile (fname.c_str ());
  }

  void testCreationTime ()
  {
    CPPUNIT_ASSERT_EQUAL (openHelper (), 0);
    CPPUNIT_ASSERT (tfile->getCreationTime () != -1);
  }

  void testLastAccessTime ()
  {
    CPPUNIT_ASSERT_EQUAL (openHelper (), 0);
    CPPUNIT_ASSERT (tfile->getCreationTime () != -1);
  }

  void testLastModTime ()
  {
    CPPUNIT_ASSERT_EQUAL (openHelper (), 0);
    CPPUNIT_ASSERT (tfile->getCreationTime () != -1);
  }

  void testSeek ()
  {
    CPPUNIT_ASSERT_EQUAL (openHelper (), 0);
    CPPUNIT_ASSERT_EQUAL (tfile->seek (1), 0);
    CPPUNIT_ASSERT_EQUAL (tfile->getSeek (), 1ul);
  }

  void testBinary ()
  {
    const char *data = "this\0text\0is\0NUL\0separed\0\0";
    char buffer[32];
    u_long nbw, nbr;
    CPPUNIT_ASSERT_EQUAL (openHelper (), 0);
    CPPUNIT_ASSERT_EQUAL (tfile->write (data, 26, &nbw), 0);
    CPPUNIT_ASSERT_EQUAL (nbw, 26ul);

    CPPUNIT_ASSERT_EQUAL (tfile->seek (0), 0);

    CPPUNIT_ASSERT_EQUAL (tfile->read (buffer, 26, &nbr), 0);
    CPPUNIT_ASSERT_EQUAL (nbr, 26ul);
    CPPUNIT_ASSERT_EQUAL (memcmp (data, buffer, 26), 0);
  }

  void testTruncate ()
  {
    u_long nbw;

    CPPUNIT_ASSERT_EQUAL (FilesUtility::deleteFile (fname.c_str ()), 0);

    CPPUNIT_ASSERT_EQUAL (openHelper (), 0);

    CPPUNIT_ASSERT_EQUAL (tfile->getFileSize (), 0ul);

    const char *data = "Hello World!"; /* Something very original.  */
    const u_long dataLen = strlen (data);

    CPPUNIT_ASSERT_EQUAL (tfile->writeToFile (data, dataLen, &nbw), 0);

    CPPUNIT_ASSERT_EQUAL (tfile->getFileSize (), dataLen);

    CPPUNIT_ASSERT_EQUAL (tfile->truncate (dataLen / 2), 0);

    CPPUNIT_ASSERT_EQUAL (tfile->getFileSize (), dataLen / 2);

    CPPUNIT_ASSERT_EQUAL (tfile->writeToFile (data, dataLen, &nbw), 0);

    CPPUNIT_ASSERT_EQUAL (tfile->getFileSize (), dataLen + dataLen / 2);

    FilesUtility::deleteFile (fname.c_str ());
  }

};

CPPUNIT_TEST_SUITE_REGISTRATION (TestFile);
