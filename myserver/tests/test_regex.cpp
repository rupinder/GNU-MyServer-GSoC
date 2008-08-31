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

#include <include/base/regex/myserver_regex.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

class TestRegex : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( TestRegex );
  CPPUNIT_TEST( testStarMatch );
  CPPUNIT_TEST( testPlusMatch );
  CPPUNIT_TEST( testGroup );
  CPPUNIT_TEST( testIsCompiled );
  CPPUNIT_TEST( testClone );
  CPPUNIT_TEST( testFree );
  CPPUNIT_TEST_SUITE_END();

  Regex *regex;
public:
  void setUp()
  {
    regex = new Regex();
  }

  void tearDown()
  {
    delete regex;
  }

  void testGroup()
  {
    int ret = regex->compile("hello (world)!*", REG_EXTENDED);

    CPPUNIT_ASSERT_EQUAL(ret, 0);

    regmatch_t pm[2];
    ret = regex->exec("hello world!!!", 2, pm, 0);
    CPPUNIT_ASSERT_EQUAL(ret, 0);


    CPPUNIT_ASSERT_EQUAL(pm[1].rm_so, 6);
    CPPUNIT_ASSERT_EQUAL(pm[1].rm_eo, 11);
  }

  void testStarMatch()
  {
    int ret = regex->compile("hello world!*", REG_EXTENDED);

    CPPUNIT_ASSERT_EQUAL(ret, 0);

    regmatch_t pm;
    ret = regex->exec("hello world!!!", 1, &pm, 0);
    CPPUNIT_ASSERT_EQUAL(ret, 0);

    ret = regex->exec("hell0 world!!!", 1, &pm, 0);
    CPPUNIT_ASSERT(ret);

    ret = regex->exec("hello world", 1, &pm, 0);
    CPPUNIT_ASSERT_EQUAL(ret, 0);
  }

  void testPlusMatch()
  {
    int ret = regex->compile("hello world!+", REG_EXTENDED);

    CPPUNIT_ASSERT_EQUAL(ret, 0);

    regmatch_t pm;
    ret = regex->exec("hello world!!!", 1, &pm, 0);
    CPPUNIT_ASSERT_EQUAL(ret, 0);

    ret = regex->exec("hell0 world!!!", 1, &pm, 0);
    CPPUNIT_ASSERT(ret);

    ret = regex->exec("hello world", 1, &pm, 0);
    CPPUNIT_ASSERT(ret);
  }

  void testIsCompiled()
  {
    CPPUNIT_ASSERT_EQUAL(regex->isCompiled(), 0);

    regex->compile("test", REG_EXTENDED);

    CPPUNIT_ASSERT(regex->isCompiled());
  }

  void testClone()
  {
    Regex cloned;
    regmatch_t pm;

    regex->compile("test", REG_EXTENDED);

    cloned.clone(*regex);

    int ret = regex->exec("test", 1, &pm, 0);
    int retCloned = cloned.exec("test", 1, &pm, 0);

    CPPUNIT_ASSERT_EQUAL(ret, retCloned);
  }


  void testFree()
  {
    regmatch_t pm;
    regex->compile("test", REG_EXTENDED);

    regex->free();

    int ret = regex->exec("test", 1, &pm, 0);

    CPPUNIT_ASSERT(ret);
  }

};
CPPUNIT_TEST_SUITE_REGISTRATION( TestRegex );
