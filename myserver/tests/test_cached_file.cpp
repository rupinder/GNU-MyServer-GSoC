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

#include <cppunit/CompilerOutputter.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <include/base/files_cache/cached_file.h>
#include <string.h>

#include <string>

using namespace std;

class TestCachedFile : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestCachedFile );

  CPPUNIT_TEST (testFilePointer);
  CPPUNIT_TEST (testGetHandle);
  CPPUNIT_TEST (testOpenFile);
  CPPUNIT_TEST (testCreateTemporaryFile);
  CPPUNIT_TEST (testWrite);
  CPPUNIT_TEST (testRead);

  CPPUNIT_TEST_SUITE_END ();

  CachedFileBuffer *cfb;
  CachedFile *cf;

public:
  void setUp ()
  {
    const char* buffer = "hello world!!!";

    cfb = new CachedFileBuffer (buffer, strlen (buffer));

    cf = new CachedFile (cfb);
  }

  void tearDown ()
  {
    delete cf;
    delete cfb;
  }


  void testFilePointer ()
  {
    CPPUNIT_ASSERT_EQUAL (cf->seek (3), 0);
    CPPUNIT_ASSERT (cf->seek (100000));
  }


  void testGetHandle ()
  {
    CPPUNIT_ASSERT_EQUAL (cf->getHandle (), (Handle)-1);
  }

  void testOpenFile ()
  {
    string filename;
    CPPUNIT_ASSERT (cf->openFile (NULL, 0));
    CPPUNIT_ASSERT (cf->openFile (filename, 0));
  }

  void testCreateTemporaryFile ()
  {
    CPPUNIT_ASSERT (cf->createTemporaryFile (NULL));
  }

  void testWrite ()
  {
    CPPUNIT_ASSERT (cf->writeToFile (NULL, 0, NULL));
  }

  void testRead ()
  {
    u_long nbr = 0;
    u_long size = cf->getFileSize ();

    CPPUNIT_ASSERT (size > 0);

    char *buffer =new char[size];

    int ret = cf->read (buffer, size, &nbr);

    CPPUNIT_ASSERT_EQUAL (ret, 0);
    CPPUNIT_ASSERT (nbr > 0);

    delete [] buffer;
  }



};

CPPUNIT_TEST_SUITE_REGISTRATION ( TestCachedFile );
