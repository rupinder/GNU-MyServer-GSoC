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
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>


#include "../include/base/utility.h"
#include "../include/base/file/file.h"
#include "../include/base/file/files_utility.h"

#include <string.h>
#include <unistd.h>
#include <string>

using namespace std;

class TestFile : public CppUnit::TestFixture
{
  File *tfile;
  string fname;
  
  CPPUNIT_TEST_SUITE( TestFile );
  
  CPPUNIT_TEST( testCreateTemporaryFile );
  CPPUNIT_TEST( testOnFile );
  
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp ( )
  {
    setcwdBuffer ();/* Be sure the cwd buffer is initialized.  */
    FilesUtility::temporaryFileName ( 0, fname );
    tfile = new File;
  }
  
  void tearDown ( )
  {
    delete tfile;
  }
  
  void testCreateTemporaryFile ( )
  {
    CPPUNIT_ASSERT_EQUAL( tfile->createTemporaryFile ( fname.c_str() ), 0 );
    
    CPPUNIT_ASSERT_EQUAL( tfile->close ( ), 0 );
  }
  
  void testOnFile ( )
  {
    char buf[] = { 'H', 'e', 'l', 'l', 'o', ',', ' ', 'm', 'y', 'W', 'o', 'r', 'l', 'd', 0 };
    const int bufLen = sizeof (buf) / sizeof (char);
    u_long nbw;
    u_long nbr;
    
    CPPUNIT_ASSERT_EQUAL( tfile->openFile( fname.c_str(), File::MYSERVER_OPEN_WRITE | File::MYSERVER_OPEN_READ | File::MYSERVER_CREATE_ALWAYS ), 0 );
    
    CPPUNIT_ASSERT_EQUAL( tfile->writeToFile ( buf, bufLen, &nbw ), 0 );
    
    memset ( buf, 0, bufLen );
    
    CPPUNIT_ASSERT_EQUAL( tfile->seek ( 0 ), 0 );
    
    CPPUNIT_ASSERT_EQUAL( tfile->read ( buf, bufLen, &nbr ), 0 );
    
    CPPUNIT_ASSERT( nbr > 0 );
    
    CPPUNIT_ASSERT( tfile->getCreationTime ( ) != -1 );
    
    CPPUNIT_ASSERT( tfile->getLastAccTime ( ) != -1 );
    
    CPPUNIT_ASSERT( tfile->getLastModTime ( ) != -1 );
    
    CPPUNIT_ASSERT( tfile->getFileSize ( ) != -1 );
    
    CPPUNIT_ASSERT_EQUAL( tfile->close ( ), 0 );
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION( TestFile );
