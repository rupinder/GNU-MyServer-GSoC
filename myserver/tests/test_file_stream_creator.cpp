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

#include <list>
#include <string>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

#include <include/log/stream/file_stream_creator.h>
#include <include/base/file/file.h>
#include <include/base/file/files_utility.h>
#include <include/filter/filters_factory.h>

class TestFileStreamCreator : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE (TestFileStreamCreator);
  CPPUNIT_TEST (testCreation);
  CPPUNIT_TEST_SUITE_END ();
public:
  void setUp ()
  {
    fsc = new FileStreamCreator ();
    ff = new FiltersFactory ();
  }
  
  void testCreation ()
  {
    list<string> filters;
    LogStream* ls = fsc->create (ff, "foo", filters, 0);
    CPPUNIT_ASSERT (ls);
  }
  
  void tearDown ()
  {
    delete fsc;
    delete ff;
    FilesUtility::deleteFile ("foo");
  }
private:
  FileStreamCreator* fsc;
  FiltersFactory* ff;
};

CPPUNIT_TEST_SUITE_REGISTRATION (TestFileStreamCreator);
