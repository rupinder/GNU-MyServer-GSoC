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

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <include/base/file/files_utility.h>
#include <string.h>

#include <string>

using namespace std;

class TestFilesUtility : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( TestFilesUtility );
  CPPUNIT_TEST( testGetPathRecursionLevel );
  CPPUNIT_TEST( testSplitPath );
  CPPUNIT_TEST( testSplitPathString );
  CPPUNIT_TEST( testSplitPathLength );
  CPPUNIT_TEST( testGetFileExt );

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

  void testSplitPathString()
  {
    string path;
    string dir;
    string file;

    path.assign("/foo/bar/baz");

    FilesUtility::splitPath(path, dir, file);

    CPPUNIT_ASSERT(dir.compare("/foo/bar/") == 0);
    CPPUNIT_ASSERT(file.compare("baz") == 0);

    path.assign("/foo/bar/");

    FilesUtility::splitPath(path, dir, file);

    CPPUNIT_ASSERT(dir.compare("/foo/bar/") == 0);
    CPPUNIT_ASSERT(file.compare("") == 0);

    path.assign("baz");

    FilesUtility::splitPath(path, dir, file);

    CPPUNIT_ASSERT(dir.compare("") == 0);
    CPPUNIT_ASSERT(file.compare("baz") == 0);
  }


  void testSplitPath()
  {
    char *path;
    char dir[256];
    char file[256];

    FilesUtility::splitPath("/foo/bar/baz", dir, file);

    CPPUNIT_ASSERT(strcmp(dir, "/foo/bar/") == 0);
    CPPUNIT_ASSERT(strcmp(file, "baz") == 0);


    FilesUtility::splitPath("/foo/bar/", dir, file);

    CPPUNIT_ASSERT(strcmp(dir, "/foo/bar/") == 0);
    CPPUNIT_ASSERT(strcmp(file, "") == 0);


    FilesUtility::splitPath("baz", dir, file);

    CPPUNIT_ASSERT(strcmp(dir, "") == 0);
    CPPUNIT_ASSERT(strcmp(file, "baz") == 0);
  }

  void testSplitPathLength()
  {
    char *path;
    int dir;
    int file;

    FilesUtility::splitPathLength("/foo/bar/baz", &dir, &file);

    CPPUNIT_ASSERT(dir >= 10 && dir <= 12);
    CPPUNIT_ASSERT(file >= 4 && file <= 6);

    FilesUtility::splitPathLength("/foo/bar/", &dir, &file);

    CPPUNIT_ASSERT(dir >= 10 && dir <= 12);
    CPPUNIT_ASSERT(file >= 1 && file <= 3);

    FilesUtility::splitPathLength("baz", &dir, &file);

    CPPUNIT_ASSERT(dir >= 1 && dir <= 3);
    CPPUNIT_ASSERT(file >= 4 && file <= 6);
  }

  void testGetFileExt()
  {

    char ext[12];
    FilesUtility::getFileExt(ext, "myserver.exe");
    CPPUNIT_ASSERT(strcmp(ext, "exe") == 0);

    FilesUtility::getFileExt(ext, "myserver");
    CPPUNIT_ASSERT(strcmp(ext, "") == 0);

    FilesUtility::getFileExt(ext, "myserver.exe.sh");
    CPPUNIT_ASSERT(strcmp(ext, "sh") == 0);
  }


};
CPPUNIT_TEST_SUITE_REGISTRATION( TestFilesUtility );
