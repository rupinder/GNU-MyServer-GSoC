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

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include "../include/files_utility.h"

class TestFilesUtility : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( TestFilesUtility );
  CPPUNIT_TEST( testGetPathRecursionLevel );
  CPPUNIT_TEST_SUITE_END();
  
public:
  void setUp() {}
  void tearDown() {}
  void testGetPathRecursionLevel()
  {
    CPPUNIT_ASSERT(FilesUtility::getPathRecursionLevel("/foo/bar") > 0);
    CPPUNIT_ASSERT(FilesUtility::getPathRecursionLevel("/foo/././././bar") > 0);
    CPPUNIT_ASSERT(FilesUtility::getPathRecursionLevel("/home/.././../../bar/") < 0);
    CPPUNIT_ASSERT(FilesUtility::getPathRecursionLevel("./.././../../bar/") < 0);
    CPPUNIT_ASSERT(FilesUtility::getPathRecursionLevel("") == 0);
    CPPUNIT_ASSERT(FilesUtility::getPathRecursionLevel(".") == 0);
    CPPUNIT_ASSERT(FilesUtility::getPathRecursionLevel("/./././..") < 0);
  }
};
CPPUNIT_TEST_SUITE_REGISTRATION( TestFilesUtility );
