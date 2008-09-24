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

#include <include/filter/filters_factory.h>
#include <include/log/stream/file_stream.h>
#include <include/log/stream/file_stream_creator.h>
#include <include/base/file/files_utility.h>
#include <include/base/file/file.h>
#include <include/base/utility.h>

class TestFileStream : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE (TestFileStream);
  CPPUNIT_TEST (testCycleLog);
  CPPUNIT_TEST (testMakeNewFileName);
  CPPUNIT_TEST_SUITE_END ();
public:
  void setUp()
  {
    fsc = new FileStreamCreator ();
    ff = new FiltersFactory ();
    setcwdBuffer ();
  }

  void testMakeNewFileName ()
  {
    list<string> filters;
    LogStream* ls = fsc->create (ff, "foo", filters, 0);
    FileStream* fs = dynamic_cast<FileStream*>(ls);
    File* outStream = dynamic_cast<File*>(fs->getOutStream ());
    string newFileName = fs->makeNewFileName (outStream->getFilename ());
    string wd;
    getdefaultwd (wd);
    wd.append ("/foo");
    CPPUNIT_ASSERT (newFileName.find (wd) != string::npos);
    delete fs;
  }

  void testCycleLog ()
  {
    list<string> filters;
    string message ("thisisaverylongmessage\n");
    string message2 ("thisisanothermessage\n");
    LogStream* ls = fsc->create (ff, "foo", filters, 10);
    File* outStream = dynamic_cast<File*>(ls->getOutStream ());
    CPPUNIT_ASSERT (!ls->log (message));
    CPPUNIT_ASSERT (!ls->log (message2));
    ls->close ();
    File f;
    f.openFile ("foo", FileStream::defaultFileMask);
    char buf[64];
    u_long nbr;
    f.read (buf, 64, &nbr);
    buf[nbr] = '\0';
    CPPUNIT_ASSERT (!message2.compare (buf));
    f.close ();
    list<string> cs = ls->getCycledStreams ();
    CPPUNIT_ASSERT (cs.size ());
    list<string>::iterator it;
    for (it = cs.begin (); it != cs.end (); it++)
      {
        f.openFile (*it, FileStream::defaultFileMask);
        f.read (buf, 64, &nbr);
        buf[nbr] = '\0';
        CPPUNIT_ASSERT (!message.compare (buf));
        f.close ();
        CPPUNIT_ASSERT (!FilesUtility::deleteFile (*it));
      }
    delete ls;
  }

  void tearDown()
  {
    delete ff;
    delete fsc;
    FilesUtility::deleteFile ("foo");
  }

private:
  FiltersFactory* ff;
  FileStreamCreator* fsc;
};

CPPUNIT_TEST_SUITE_REGISTRATION (TestFileStream);
