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
  CPPUNIT_TEST (testCreateTemporaryDelayedFile);
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
                            | File::NO_CACHE_STAT
                            | File::FILE_OPEN_ALWAYS);
  }
public:
  void setUp ()
  {
    FilesUtility::temporaryFileName (0, fname);
    tfile = new File;
  }

  void tearDown ()
  {
    try
      {
        FilesUtility::deleteFile (tfile->getFilename ());
      }
    catch (...)
      {
      }
    delete tfile;
  }

  void testCreateTemporaryDelayedFile ()
  {
    int ret = tfile->createTemporaryFile (fname.c_str (), false);
    CPPUNIT_ASSERT_EQUAL (ret, 0);

    /* The unlink is done just before the close, the file can be stat'ed. */
    ret = FilesUtility::nodeExists (tfile->getFilename ());
    CPPUNIT_ASSERT (ret);

    ret = tfile->close ();
    CPPUNIT_ASSERT_EQUAL (ret, 0);

    ret = FilesUtility::nodeExists (tfile->getFilename ());
    CPPUNIT_ASSERT_EQUAL (ret, 0);
  }

  void testCreateTemporaryFile ()
  {
    int ret = tfile->createTemporaryFile (fname.c_str (), true);
    CPPUNIT_ASSERT_EQUAL (ret, 0);

    /* Using unlink the file can't be stat'ed at this point.  */
    ret = FilesUtility::nodeExists (tfile->getFilename ());
    CPPUNIT_ASSERT_EQUAL (ret, 0);

    ret = tfile->close ();
    CPPUNIT_ASSERT_EQUAL (ret, 0);

    ret = FilesUtility::nodeExists (tfile->getFilename ());
    CPPUNIT_ASSERT_EQUAL (ret, 0);
  }

  void testOnFile ()
  {
    const char *buf = "HELLO myWORLD";
    char readbuf[512];
    size_t bufLen = strlen (buf);
    size_t nbw;
    size_t nbr;

    CPPUNIT_ASSERT_EQUAL (openHelper (), 0);

    CPPUNIT_ASSERT_EQUAL (tfile->writeToFile (buf, bufLen, &nbw), 0);

    memset (readbuf, 0, bufLen);

    CPPUNIT_ASSERT_EQUAL (tfile->seek (0), 0);

    CPPUNIT_ASSERT_EQUAL (tfile->read (readbuf, bufLen, &nbr), 0);

    CPPUNIT_ASSERT (nbr > 0);

    for (size_t i = 0; i < bufLen; i++)
      CPPUNIT_ASSERT_EQUAL (buf[i], readbuf[i]);

    CPPUNIT_ASSERT_EQUAL ((size_t) tfile->getFileSize (), nbr);
    CPPUNIT_ASSERT_EQUAL (tfile->close (), 0);

    try
      {
        FilesUtility::deleteFile (fname.c_str ());
      }
    catch (...)
      {
      }
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
    CPPUNIT_ASSERT_EQUAL (tfile->getSeek (), (off_t) 1);
  }

  void testBinary ()
  {
    const char *data = "this\0text\0is\0NUL\0separed\0\0";
    char buffer[32];
    size_t nbw, nbr;
    CPPUNIT_ASSERT_EQUAL (openHelper (), 0);
    CPPUNIT_ASSERT_EQUAL (tfile->write (data, 26, &nbw), 0);
    CPPUNIT_ASSERT_EQUAL (nbw, (size_t) 26);

    CPPUNIT_ASSERT_EQUAL (tfile->seek (0), 0);

    CPPUNIT_ASSERT_EQUAL (tfile->read (buffer, 26, &nbr), 0);
    CPPUNIT_ASSERT_EQUAL (nbr, (size_t) 26);
    CPPUNIT_ASSERT_EQUAL (memcmp (data, buffer, 26), 0);
  }

  void testTruncate ()
  {
    size_t nbw;

    try
      {
        FilesUtility::deleteFile (fname.c_str ());
      }
    catch (...)
      {
      }

    CPPUNIT_ASSERT_EQUAL (openHelper (), 0);

    CPPUNIT_ASSERT_EQUAL (tfile->getFileSize (), (off_t) 0);

    const char *data = "Hello World!"; /* Something very original.  */
    const size_t dataLen = strlen (data);

    CPPUNIT_ASSERT_EQUAL (tfile->writeToFile (data, dataLen, &nbw), 0);

    CPPUNIT_ASSERT_EQUAL (tfile->getFileSize (), (off_t) dataLen);

    CPPUNIT_ASSERT_EQUAL (tfile->truncate (dataLen / 2), 0);

    CPPUNIT_ASSERT_EQUAL (tfile->getFileSize (), (off_t) dataLen / 2);

    CPPUNIT_ASSERT_EQUAL (tfile->writeToFile (data, dataLen, &nbw), 0);

    CPPUNIT_ASSERT_EQUAL (tfile->getFileSize (), (off_t) (dataLen + dataLen / 2));

    try
      {
        FilesUtility::deleteFile (fname.c_str ());
      }
    catch (...)
      {
      }
  }

};

CPPUNIT_TEST_SUITE_REGISTRATION (TestFile);
