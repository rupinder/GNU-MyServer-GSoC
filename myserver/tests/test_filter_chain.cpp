/*
 MyServer
 Copyright (C) 2008 Free Software Foundation, Inc.
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

#include <include/filter/stream.h>
#include <include/filter/gzip/gzip.h>
#include <include/filter/gzip/gzip_decompress.h>
#include <include/filter/filters_chain.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <include/protocol/protocol.h>
#include <include/filter/memory_stream.h>

class MyProtocol : public Protocol
{

};

class TestFilterChain : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( TestFilterChain );
  CPPUNIT_TEST( testGzipChain );
  CPPUNIT_TEST( testProtocol );
  CPPUNIT_TEST( testIsEmpty );
  CPPUNIT_TEST( testProtocolData );
  CPPUNIT_TEST( testAcceptDuplicates );
  CPPUNIT_TEST( testStream );
  CPPUNIT_TEST( testIsFilterPresent );
  CPPUNIT_TEST( testRemoveFilter );
  CPPUNIT_TEST_SUITE_END();

  FiltersChain *fc;
  
public:
  void setUp()
  {
    fc = new FiltersChain;
  }

  void tearDown()
  {
    delete fc;
  }

  void testGzipChain()
  {
    char szTest[64], szTemp[64], szExpected[64];
    strcpy(szTest, "The quick brown fox jumps over the lazy dog");
    int nLength = strlen(szTest);

    u_long nbw = 0;
    MemBuf mb;
    MemoryStream ms(&mb);
    ms.write(szTest, 64, &nbw);

    fc->setStream(&ms);
    fc->addFilter(Gzip::factory("GzipFilter"), &nbw);
    fc->addFilter(GzipDecompress::factory("GzipDecompressFilter"), &nbw);

    char *pBuff = szExpected;
    int i = nLength;
    while ( !fc->read(szTemp, 64, &nbw) && i > 0 )
    {
      strncpy(pBuff, szTemp, (int)nbw > i ? i : nbw);
      pBuff += nbw;
      i -= nbw;
    }
    szExpected[nLength] = '\0';
    fc->clearAllFilters();

    CPPUNIT_ASSERT(strncmp(szTest, szExpected, nbw) == 0);
  }


  void testProtocol()
  {
    MyProtocol mp;
    fc->setProtocol(&mp);
    CPPUNIT_ASSERT_EQUAL((Protocol*)&mp, fc->getProtocol());
  }

  void testAcceptDuplicates()
  {
    fc->setAcceptDuplicates(0);
    CPPUNIT_ASSERT_EQUAL(fc->getAcceptDuplicates(), 0);

    fc->setAcceptDuplicates(1);
    CPPUNIT_ASSERT_EQUAL(fc->getAcceptDuplicates(), 1);
  }


  void testStream()
  {
    MemBuf mb;
    MemoryStream ms(&mb);

    fc->setStream(&ms);

    CPPUNIT_ASSERT_EQUAL((Stream*)&ms, fc->getStream());
  }

  void testProtocolData()
  {
    char *data = new char[2];
    fc->setProtocolData(data);
    CPPUNIT_ASSERT_EQUAL(data, (char*)fc->getProtocolData());
    delete [] data;
  }

  void testIsEmpty()
  {
    u_long nbw;
    MemBuf mb;
    MemoryStream ms(&mb);

    fc->setStream(&ms);

    CPPUNIT_ASSERT(fc->isEmpty() != 0);

    fc->addFilter(Gzip::factory("GzipFilter"), &nbw);

    CPPUNIT_ASSERT_EQUAL(fc->isEmpty(), 0);

    fc->clearAllFilters();

    CPPUNIT_ASSERT(fc->isEmpty() != 0);
  }
  
  void testIsFilterPresent()
  {
    u_long nbw;
    Filter* filter = Gzip::factory("GzipFilter");
    MemBuf mb;
    char name[32];
    MemoryStream ms(&mb);

    fc->setStream(&ms);

    CPPUNIT_ASSERT_EQUAL(fc->isFilterPresent(filter), 0);
    CPPUNIT_ASSERT_EQUAL(fc->isFilterPresent(filter->getName(name, 32)), 0);

    fc->addFilter(filter, &nbw);

    CPPUNIT_ASSERT(fc->isFilterPresent(filter) != 0);
    CPPUNIT_ASSERT(fc->isFilterPresent(filter->getName(name, 32)) != 0);
  }

  void testRemoveFilter()
  {
    char name[32];
    MemBuf mb;
    u_long nbw;
    Filter* filter = Gzip::factory("GzipFilter");
    MemoryStream ms(&mb);

    fc->setStream(&ms);

    fc->addFilter(filter, &nbw);

    CPPUNIT_ASSERT(fc->isFilterPresent(filter) != 0);
    CPPUNIT_ASSERT(fc->isFilterPresent(filter->getName(name, 32)) != 0);

    fc->removeFilter(filter);

    CPPUNIT_ASSERT_EQUAL(fc->isFilterPresent(filter), 0);
    CPPUNIT_ASSERT_EQUAL(fc->isFilterPresent(filter->getName(name, 32)), 0);
  }

};
CPPUNIT_TEST_SUITE_REGISTRATION( TestFilterChain );
