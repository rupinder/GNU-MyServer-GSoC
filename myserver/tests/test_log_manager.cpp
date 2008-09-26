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

#include <include/log/log_manager.h>
#include <include/filter/filters_factory.h>
#include <include/log/stream/log_stream_factory.h>
#include <include/base/file/files_utility.h>
#include <include/base/file/file.h>
#include <include/base/utility.h>
#include <include/filter/gzip/gzip.h>
#include <include/filter/gzip/gzip_decompress.h>

class TestLogManager : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE (TestLogManager);
  CPPUNIT_TEST (testEmpty);
  CPPUNIT_TEST (testSize);
  CPPUNIT_TEST (testContains);
  CPPUNIT_TEST (testAddLogStream);
  CPPUNIT_TEST (testRemoveLogStream);
  CPPUNIT_TEST (testLog);
  CPPUNIT_TEST (testClose);
  CPPUNIT_TEST (testCycleLog);
  CPPUNIT_TEST (testLoggingLevel);
  CPPUNIT_TEST (testClear);
  CPPUNIT_TEST (testLogThroughGzip);
  CPPUNIT_TEST (testCycleLogWithGzipChain);
  CPPUNIT_TEST_SUITE_END ();
public:
  void setUp ()
  {
    lsf = new LogStreamFactory ();
    ff = new FiltersFactory ();
    ff->insert ("gzip", Gzip::factory);
    ff->insert ("gunzip", GzipDecompress::factory);
    lm = new LogManager (ff, lsf);
    setcwdBuffer ();
  }

  void testEmpty ()
  {
    CPPUNIT_ASSERT (lm->empty ());
  }

  void testSize ()
  {
    CPPUNIT_ASSERT (lm->size () == 0);
  }

  void testContains ()
  {
    CPPUNIT_ASSERT (!lm->contains ("foo"));
  }

  void testAddLogStream ()
  {
    list<string> filters;
    CPPUNIT_ASSERT (lm->addLogStream ("foo", filters, 0));
    CPPUNIT_ASSERT (lm->addLogStream ("file", filters, 0));
    CPPUNIT_ASSERT (!lm->addLogStream ("file://foo", filters, 0));
    CPPUNIT_ASSERT (lm->addLogStream ("file://foo", filters, 0));
  }

  void testRemoveLogStream ()
  {
    list<string> filters;
    CPPUNIT_ASSERT (lm->removeLogStream ("foo"));
    lm->addLogStream ("file://foo", filters, 0);
    CPPUNIT_ASSERT (!lm->removeLogStream ("file://foo"));
  }

  void testLog ()
  {
    string message ("A message");
    list<string> filters;
    lm->addLogStream ("file://foo", filters, 0);
    lm->log (message);
    lm->close ();
    File f;
    f.openFile ("foo", FileStream::defaultFileMask);
    char buf[message.size () + 1];
    u_long nbr;
    f.read (buf, message.size () + 1, &nbr);
    buf[nbr] = '\0';
    CPPUNIT_ASSERT (!message.compare (buf));
    f.close ();
  }
  
  void testClose ()
  {
    list<string> filters;
    lm->addLogStream ("file://foo", filters, 0);
    lm->close ();
    CPPUNIT_ASSERT (!lm->getLogStream ("file://foo")->getIsOpened ());
  }

  void testCycleLog ()
  {
    string message ("A message\n");
    string message1 ("Another message\n");
    list<string> filters;
    lm->addLogStream ("file://foo", filters, 10);
    lm->log (message);
    CPPUNIT_ASSERT (!lm->log (message1));
    lm->close ();
    File f;
    f.openFile ("foo", FileStream::defaultFileMask);
    char buf[64];
    u_long nbr;
    f.read (buf, 64, &nbr);
    buf[nbr] = '\0';
    f.close ();
    CPPUNIT_ASSERT (!message1.compare (buf));
    list<string> cs = lm->getLogStream ("file://foo")->getCycledStreams ();
    list<string>::iterator it;
    for (it = cs.begin (); it != cs.end (); it++)
      {
        f.openFile (*it, FileStream::defaultFileMask);
        f.read (buf, 64, &nbr);
        buf[nbr] = '\0';
        f.close ();
        CPPUNIT_ASSERT (!message.compare (buf));
        CPPUNIT_ASSERT (!FilesUtility::deleteFile (*it));
      }
  }

  void testLoggingLevel ()
  {
    list<string> filters;
    lm->addLogStream ("file://foo", filters, 0);
    CPPUNIT_ASSERT (lm->log ("a message", INFO));
    CPPUNIT_ASSERT (!lm->log ("a message", ERROR));
    CPPUNIT_ASSERT (!lm->log ("a message", WARNING));
    lm->setLoggingLevel (INFO);
    CPPUNIT_ASSERT (!lm->log ("a message", INFO));
  }

  void testClear ()
  {
    list<string> filters;
    lm->addLogStream ("file://foo", filters, 0);
    lm->addLogStream ("console://", filters, 0);
    CPPUNIT_ASSERT (!lm->empty ());
    lm->clear ();
    CPPUNIT_ASSERT (lm->empty ());
  }
  
  void testLogThroughGzip ()
  {
    list<string> filters;
    string message ("a message");
    filters.push_back ("gzip");
    lm->addLogStream ("file://foo", filters, 0);
    lm->log (message);
    lm->close ();
    File f;
    f.openFile ("foo", FileStream::defaultFileMask);
    char gzipChainComp[64];
    char gzipChainDecomp[64];
    u_long nbr;
    f.read (gzipChainComp, 64, &nbr);
    f.close ();
    GzipDecompress gzdc;
    gzdc.decompress (&gzipChainComp[gzdc.headerSize ()], 
                     nbr - gzdc.headerSize (),
                     gzipChainDecomp, 64);
    gzipChainDecomp[message.size ()] = '\0';
    CPPUNIT_ASSERT (!message.compare (gzipChainDecomp));
  }

  void testCycleLogWithGzipChain ()
  {
    u_long cycleLog = 40;
    list<string> filters;
    string message ("The quick brown fox jumped over the lazy dog.");
    string message1 ("Yet another short log message.");
    File f;
    char gzipComp[128];
    char gzipDecomp[128];
    GzipDecompress gzdc;
    u_long nbr = 0;
    list<string> cs;
    list<string>::iterator it;
    
    filters.push_back ("gzip");
    lm->addLogStream ("file://foo", filters, cycleLog);
    lm->log (message);
    CPPUNIT_ASSERT (!lm->log (message1));
    f.openFile ("foo", FileStream::defaultFileMask);
    f.read (gzipComp, 128, &nbr);
    f.close ();
    gzdc.decompress (&gzipComp[gzdc.headerSize ()], 
                     nbr - gzdc.headerSize (),
                     gzipDecomp, 128);
    gzipDecomp[message1.size ()] = '\0';
    CPPUNIT_ASSERT (!message1.compare (gzipDecomp));
    cs = lm->getLogStream ("file://foo")->getCycledStreams ();
    for (it = cs.begin (); it != cs.end (); it++)
      {
        f.openFile (*it, FileStream::defaultFileMask);
        f.read (gzipComp, 128, &nbr);
        f.close ();
        gzdc.decompress (&gzipComp[gzdc.headerSize ()], 
                         nbr - gzdc.headerSize (),
                         gzipDecomp, 128);
        gzipDecomp[message.size ()] = '\0';
        CPPUNIT_ASSERT (!message.compare (gzipDecomp));
        CPPUNIT_ASSERT (!FilesUtility::deleteFile (*it));
      }
  }

  void tearDown ()
  {
    delete lm;
    delete lsf;
    delete ff;
    FilesUtility::deleteFile ("foo");
  }

private:
  LogManager* lm;
  LogStreamFactory* lsf;
  FiltersFactory* ff;
};

CPPUNIT_TEST_SUITE_REGISTRATION (TestLogManager);
