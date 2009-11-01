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
#include <include/filter/gzip/gzip.h>
#include <include/filter/gzip/gzip_decompress.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

class TestGzip : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestGzip );
  CPPUNIT_TEST ( testGzipCompression );
  CPPUNIT_TEST_SUITE_END ();

public:
  void setUp () {}
  void tearDown () {}
  void testGzipCompression ()
  {
    Gzip gzip;
    char szTest[64] = "The quick brown fox jumps over the lazy dog";
    char szCompressed[64];
    char szDecompressed[64];

    gzip.compress (szTest, 64, szCompressed, 64);

    GzipDecompress gzipdc;
    gzipdc.decompress (szCompressed, 64, szDecompressed, 64);

    CPPUNIT_ASSERT (strcmp (szTest, szDecompressed) == 0);
  }
};
CPPUNIT_TEST_SUITE_REGISTRATION ( TestGzip );
