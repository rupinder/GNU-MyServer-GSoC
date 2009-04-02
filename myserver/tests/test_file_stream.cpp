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
#include <include/filter/gzip/gzip.h>
#include <include/filter/gzip/gzip_decompress.h>

class TestFileStream : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE (TestFileStream);
  CPPUNIT_TEST (testCycleLog);
  CPPUNIT_TEST (testReOpen);
  CPPUNIT_TEST_SUITE_END ();
public:
  void setUp ()
  {
    fsc = new FileStreamCreator ();
    ff = new FiltersFactory ();
    ff->insert ("gzip", Gzip::factory);
    setcwdBuffer ();
  }

  void testCycleLog ()
  {
    list<string> filters;
    string message;
    string message2;
    LogStream* ls;

#ifdef WIN32
    message.assign ("thisisaverylongmessage\r\n");
    message2.assign ("thisisanothermessage\r\n");
#else
    message.assign ("thisisaverylongmessage\n");
    message2.assign ("thisisanothermessage\n");
#endif
    ls = fsc->create (ff, "foo", filters, 10);
    CPPUNIT_ASSERT (ls);
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

  void testReOpen ()
  {
    list<string> filters;
    LogStream* ls;
    File f;
    char buf[128];
    u_long nbr;
    string message1;
    string message2;
    
    ls = fsc->create (ff, "foo", filters, 0);
    CPPUNIT_ASSERT (ls);
#ifdef WIN32
    message1.assign ("message1\r\n");
    message2.assign ("message2\r\n");
#endif
#ifdef NOT_WIN
    message1.assign ("message1\n");
    message2.assign ("message2\n");
#endif
    ls->log (message1);
    delete ls;
    ls = fsc->create (ff, "foo", filters, 0);
    CPPUNIT_ASSERT (ls);
    ls->log (message2);
    delete ls;
    f.openFile ("foo", FileStream::defaultFileMask);
    f.read (buf, 128, &nbr);
    f.close ();
    buf[nbr] = '\0';
    CPPUNIT_ASSERT (!string (buf).compare (message1.append (message2)));
  }
  
  void tearDown ()
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
