/*
 MyServer
 Copyright (C) 2008 The MyServer Team
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

#include "../include/memory_stream.h"
#include "../include/gzip.h"
#include "../include/gzip_decompress.h"
#include "../include/filters_chain.h"
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

class TestFilterChain : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( TestFilterChain );
  CPPUNIT_TEST( testGzipChain );
  CPPUNIT_TEST_SUITE_END();
  
public:
  void setUp() {}
  void tearDown() {}
  void testGzipChain()
  {
    char szTest[64], szTemp[64], szExpected[64];
    strcpy(szTest, "The quick brown fox jumps over the lazy dog");
    int nLength = strlen(szTest);

    u_long nbw = 0;
    MemBuf mb;
    MemoryStream ms(&mb);
    ms.write(szTest, 64, &nbw);

    FiltersChain fc;
    fc.setStream(&ms);
    fc.addFilter(Gzip::factory("GzipFilter"), &nbw);
    fc.addFilter(GzipDecompress::factory("GzipDecompressFilter"), &nbw);

    char *pBuff = szExpected;
    int i = nLength;
    while ( !fc.read(szTemp, 64, &nbw) && i > 0 )
    {
      strncpy(pBuff, szTemp, nbw>i?i:nbw);
      pBuff += nbw;
      i -= nbw;
    }
    szExpected[nLength] = '\0';
    fc.clearAllFilters();

    CPPUNIT_ASSERT(strncmp(szTest, szExpected, nbw) == 0);
  }
};
CPPUNIT_TEST_SUITE_REGISTRATION( TestFilterChain );
