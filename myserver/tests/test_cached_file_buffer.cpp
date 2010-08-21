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
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <include/base/files_cache/cached_file_buffer.h>
#include <include/base/files_cache/cached_file_factory.h>
#include <string.h>

#include <string>

using namespace std;

class TestCachedFileBuffer : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestCachedFileBuffer );

  CPPUNIT_TEST (testGetFileSize);
  CPPUNIT_TEST (testFactoryToNotify);
  CPPUNIT_TEST (testRefCounter);
  CPPUNIT_TEST_SUITE_END ();

  CachedFileBuffer *cfb;
public:
  void setUp ()
  {
    const char* buffer = "hello world!!!";

    cfb = new CachedFileBuffer (buffer, strlen (buffer));
  }

  void tearDown ()
  {
    delete cfb;
  }

  void testGetFileSize ()
  {
    CPPUNIT_ASSERT (cfb->getFileSize ());
  }


  void testFactoryToNotify ()
  {
    CachedFileFactory cff;

    cfb->setFactoryToNotify (&cff);
    CPPUNIT_ASSERT_EQUAL (cfb->getFactoryToNotify (), &cff);

    cfb->setFactoryToNotify (NULL);
    CPPUNIT_ASSERT_EQUAL (cfb->getFactoryToNotify (), (CachedFileFactory*)NULL);
  }

  void testRefCounter ()
  {
    for (u_long i = 1; i <= 20; i++)
    {
      cfb->addRef ();
      CPPUNIT_ASSERT_EQUAL (cfb->getReferenceCounter (), i);
    }

    for (u_long i = 1; i <= 20; i++)
    {
      cfb->decRef ();
      CPPUNIT_ASSERT_EQUAL (cfb->getReferenceCounter (), 20 - i);
    }

  }
};

CPPUNIT_TEST_SUITE_REGISTRATION ( TestCachedFileBuffer );
