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

#include "../include/hash_map.h"
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

class TestHashmap : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( TestHashmap );
  CPPUNIT_TEST( testContainsKey );
  CPPUNIT_TEST( testClear );
  CPPUNIT_TEST( testSize );
  CPPUNIT_TEST( testEmpty );
  CPPUNIT_TEST( testPut );
  CPPUNIT_TEST( testRemove );
  CPPUNIT_TEST( testIterator );
  CPPUNIT_TEST_SUITE_END();

  HashMap<char*, int> *map;
public:
  void setUp()
  {
    map = new HashMap<char*, int>();
  }

  void tearDown()
  {
    delete map;
  }

  void testContainsKey()
  {
    map->put((char*)"one", 1);
    CPPUNIT_ASSERT_EQUAL(map->containsKey("one"), true); 
    CPPUNIT_ASSERT_EQUAL(map->containsKey("two"), false); 
  }

  void testClear()
  {
    map->put((char*)"one", 1);

    CPPUNIT_ASSERT_EQUAL(map->empty(), false); 

    map->clear();

    CPPUNIT_ASSERT_EQUAL(map->empty(), true); 
  }

  void testSize()
  {
    CPPUNIT_ASSERT_EQUAL(map->size(), 0); 
    map->put((char*)"one", 1);
    CPPUNIT_ASSERT_EQUAL(map->size(), 1); 
    map->put((char*)"two", 2);
    CPPUNIT_ASSERT_EQUAL(map->size(), 2); 
  }

  void testEmpty()
  {
    CPPUNIT_ASSERT_EQUAL(map->empty(), true); 
    map->put((char*)"key", 0);
    CPPUNIT_ASSERT_EQUAL(map->empty(), false); 
    
  }


  void testPut()
  {
    map->put((char*)"one", 1);
    CPPUNIT_ASSERT_EQUAL(map->get((char*)"one"), 1); 
  }

  void testRemove()
  {
    map->put((char*)"one", 1);
    map->remove((char*)"one");

    CPPUNIT_ASSERT_EQUAL(map->get((char*)"one"), 0);
  }


  void testIterator()
  {
    map->put((char*)"zero", 0);
    map->put((char*)"one", 1);
    map->put((char*)"two", 2);
    map->put((char*)"three", 3);
    map->put((char*)"four", 4);
    map->put((char*)"five", 5);
    map->put((char*)"six", 6);
    map->put((char*)"seven", 7);
    map->put((char*)"eight", 8);
    map->put((char*)"nine", 9);
    map->put((char*)"ten", 10);
    
    int values[11];

    for(int i = 0; i < 11; i++)
      values[i] = 0;

    for(HashMap<char*, int>::Iterator it = map->begin(); it != map->end(); it++)
    {
      values[*it] = 1;
    }
    
    for(int i = 0; i < 11; i++)
      CPPUNIT_ASSERT_EQUAL(values[i], 1);
  }

};
CPPUNIT_TEST_SUITE_REGISTRATION( TestHashmap );
