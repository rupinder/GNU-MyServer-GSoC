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

#ifdef NOT_WIN
#include <unistd.h>
#include <sys/types.h>
#endif

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

#include <include/log/log_manager.h>
#include <include/filter/filters_factory.h>
#include <include/base/file/files_utility.h>
#include <include/base/file/file.h>
#include <include/base/utility.h>
#include <include/filter/gzip/gzip.h>
#include <include/filter/gzip/gzip_decompress.h>

class TestLogManager : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE (TestLogManager);
  CPPUNIT_TEST (testEmpty);
  CPPUNIT_TEST (testContains);
  CPPUNIT_TEST (testSuccessfulAdd);
  CPPUNIT_TEST (testRemove);
  CPPUNIT_TEST (testLog);
  CPPUNIT_TEST (testClose);
  CPPUNIT_TEST (testCycle);
  CPPUNIT_TEST (testLevel);
  CPPUNIT_TEST (testClear);
  CPPUNIT_TEST (testLogThroughGzip);
  CPPUNIT_TEST (testCycleWithGzipChain);
  CPPUNIT_TEST (testCount);
  CPPUNIT_TEST (testGet);
  CPPUNIT_TEST (testReOpen);
  CPPUNIT_TEST (testNotExistingFilterAdd);
  CPPUNIT_TEST (testFailureAdd);
  CPPUNIT_TEST (testSharedAdd);
  CPPUNIT_TEST (testSharedGet);
  CPPUNIT_TEST (testSharedRemove);
  CPPUNIT_TEST (testFailureLog);
  CPPUNIT_TEST (testChown);
  CPPUNIT_TEST_SUITE_END ();
  
  class AnObject
  {
  };

public:
  void setUp ()
  {
    ff = new FiltersFactory ();
    ff->insert ("gzip", Gzip::factory);
    ff->insert ("gunzip", GzipDecompress::factory);
    lm = new LogManager (ff);
    setcwdBuffer ();
  }

  void testEmpty ()
  {
    CPPUNIT_ASSERT (lm->empty ());
  }

  void testContains ()
  {
    CPPUNIT_ASSERT (!lm->contains ("foo"));
  }

  void testSuccessfulAdd ()
  {
    list<string> filters;

    CPPUNIT_ASSERT (!lm->add (this, "test", "file://foo", filters, 0));
  }

  void testRemove ()
  {
    list<string> filters;

    CPPUNIT_ASSERT (lm->remove (this));
    lm->add (this, "test", "file://foo", filters, 0);
    CPPUNIT_ASSERT (!lm->remove (this));
    CPPUNIT_ASSERT (!lm->contains (this));
  }

  void testLog ()
  {
    string message ("A message");
    list<string> filters;
    File f;
    char buf[message.size () + 1];
    u_long nbr;

    lm->add (this, "test", "file://foo", filters, 0);
    lm->log (this, "test", "file://foo", message);
    lm->close (this);
    f.openFile ("foo", FileStream::defaultFileMask);
    f.read (buf, message.size () + 1, &nbr);
    buf[nbr] = '\0';
    CPPUNIT_ASSERT (!message.compare (buf));
    f.close ();
  }
  
  void testClose ()
  {
    list<string> filters;

    lm->add (this, "test", "file://foo", filters, 0);
    CPPUNIT_ASSERT (lm->close (this, "test", "file://bar"));
    CPPUNIT_ASSERT (!lm->close (this, "test", "file://foo"));
    CPPUNIT_ASSERT (lm->close (this, "test", "file://foo"));
  }

  void testCycle ()
  {
    string message ("A message\n");
    string message1 ("Another message\n");
    list<string> filters;
    File f;
    char buf[64];
    u_long nbr;
    LogStream* ls;
    list<string> cs;
    list<string>::iterator it;

    lm->add (this, "test", "file://foo", filters, 10);
    lm->log (this, "test", "file://foo", message);
    CPPUNIT_ASSERT (!lm->log (this, "test", "file://foo", message1));
    lm->close (this, "test", "file://foo");
    f.openFile ("foo", FileStream::defaultFileMask);
    f.read (buf, 64, &nbr);
    buf[nbr] = '\0';
    f.close ();
    CPPUNIT_ASSERT (!message1.compare (buf));
    CPPUNIT_ASSERT (!lm->get (this, "test", "file://foo", &ls));
    cs = ls->getCycledStreams ();
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

  void testLevel ()
  {
    list<string> filters;

    lm->add (this, "test", "file://foo", filters, 0);
    lm->setLevel (MYSERVER_LOG_MSG_WARNING);
    CPPUNIT_ASSERT (lm->log (this, "test", "a message", false, MYSERVER_LOG_MSG_INFO));
    CPPUNIT_ASSERT (!lm->log (this, "test", "a message", false, MYSERVER_LOG_MSG_ERROR));
    CPPUNIT_ASSERT (!lm->log (this, "test", "a message", false, MYSERVER_LOG_MSG_WARNING));
    lm->setLevel (MYSERVER_LOG_MSG_INFO);
    CPPUNIT_ASSERT (!lm->log (this, "test", "a message", false, MYSERVER_LOG_MSG_INFO));
  }

  void testClear ()
  {
    list<string> filters;

    lm->add (this, "test", "file://foo", filters, 0);
    lm->add (this, "test", "console://stdout", filters, 0);
    CPPUNIT_ASSERT (!lm->empty ());
    lm->clear ();
    CPPUNIT_ASSERT (lm->empty ());
  }
  
  void testLogThroughGzip ()
  {
    list<string> filters;
    string message ("a message");
    File f;
    char gzipChainComp[64];
    char gzipChainDecomp[64];
    u_long nbr;
    GzipDecompress gzdc;

    filters.push_back ("gzip");
    lm->add (this, "test", "file://foo", filters, 0);
    lm->log (this, "test", "file://foo", message);
    lm->close (this, "test", "file://foo");
    f.openFile ("foo", FileStream::defaultFileMask);
    f.read (gzipChainComp, 64, &nbr);
    f.close ();
    gzdc.decompress (&gzipChainComp[gzdc.headerSize ()], 
                     nbr - gzdc.headerSize (),
                     gzipChainDecomp, 64);
    gzipChainDecomp[message.size ()] = '\0';
    CPPUNIT_ASSERT (!message.compare (gzipChainDecomp));
  }

  void testCycleWithGzipChain ()
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
    LogStream* ls;
    
    filters.push_back ("gzip");
    lm->add (this, "test", "file://foo", filters, cycleLog);
    lm->log (this, "test", "file://foo", message);
    CPPUNIT_ASSERT (!lm->log (this, "test", "file://foo", message1));
    f.openFile ("foo", FileStream::defaultFileMask);
    f.read (gzipComp, 128, &nbr);
    f.close ();
    gzdc.decompress (&gzipComp[gzdc.headerSize ()], 
                     nbr - gzdc.headerSize (),
                     gzipDecomp, 128);
    gzipDecomp[message1.size ()] = '\0';
    CPPUNIT_ASSERT (!message1.compare (gzipDecomp));
    CPPUNIT_ASSERT (!lm->get (this, "test", "file://foo", &ls));
    cs = ls->getCycledStreams ();
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

  void testCount ()
  {
    list<string> filters;

    lm->add (this, "test", "console://stdout", filters, 0);
    lm->add (this, "test_1", "console://stderr", filters, 0);
    lm->add (this, "test_1", "file://foo", filters, 0);
    CPPUNIT_ASSERT_EQUAL (3, lm->count (this));
    CPPUNIT_ASSERT_EQUAL (1, lm->count (this, "test"));
    CPPUNIT_ASSERT_EQUAL (2, lm->count (this, "test_1"));
    CPPUNIT_ASSERT_EQUAL (1, lm->count (this, "test", "console://stdout"));
    CPPUNIT_ASSERT_EQUAL (1, lm->count (this, "test_1", "console://stderr"));
    CPPUNIT_ASSERT_EQUAL (1, lm->count (this, "test_1", "file://foo"));
    CPPUNIT_ASSERT_EQUAL (0, lm->count (this, "foo"));
    CPPUNIT_ASSERT_EQUAL (0, lm->count (this, "test", "foo"));
    CPPUNIT_ASSERT_EQUAL (0, lm->count (0));
    CPPUNIT_ASSERT_EQUAL (0, lm->count (0, "foo"));
  }

  void testGet ()
  {
    list<string> filters;
    list<string> tmp;
    list<string> l;
    LogStream* ls;
    
    CPPUNIT_ASSERT (lm->get (this, &l));
    CPPUNIT_ASSERT (lm->get (this, "test", &l));
    lm->add (this, "test", "file://foo", filters, 0);
    lm->add (this, "test", "console://stdout", filters, 0);
    lm->add (this, "test_1", "console://stderr", filters, 0);
    tmp.push_back ("file://foo");
    tmp.push_back ("console://stdout");
    tmp.push_back ("console://stderr");
    CPPUNIT_ASSERT (!lm->get (this, &l));
    tmp.sort (); l.sort ();
    CPPUNIT_ASSERT (tmp == l);
    CPPUNIT_ASSERT (!lm->get (this, "test", &l));
    tmp.clear ();
    tmp.push_back ("file://foo");
    tmp.push_back ("console://stdout");
    tmp.sort (); l.sort ();
    CPPUNIT_ASSERT (tmp == l);
    CPPUNIT_ASSERT (!lm->get (this, "test", "console://stdout", &ls));
  }

  void testReOpen ()
  {
    list<string> filters;
    File f;
    string message1;
    string message2;
    char buf[128];
    u_long nbr;
    ostringstream oss;

    oss << "message1" << endl;
    message1.assign (oss.str ());
    oss << "message2" << endl;
    message2.assign (oss.str ());
    lm->add (this, "test", "file://foo", filters, 0);
    lm->log (this, "test", "file://foo", message1);
    lm->clear ();
    CPPUNIT_ASSERT (!lm->add (this, "test", "file://foo", filters, 0));
    CPPUNIT_ASSERT (!lm->log (this, "test", "file://foo", message2));
    lm->clear ();
    f.openFile ("foo", FileStream::defaultFileMask);
    f.read (buf, 128, &nbr);
    f.close ();
    buf[nbr] = '\0';
    CPPUNIT_ASSERT (!string (buf).compare (message1.append (message2)));
  }

  void testNotExistingFilterAdd ()
  {
    list<string> filters;

    filters.push_back ("not_existing_filter");
    CPPUNIT_ASSERT (lm->add (this, "test", "file://foo", filters, 0));
    CPPUNIT_ASSERT (lm->add (this, "test", "socket://127.0.0.1:6666", filters, 0));
    CPPUNIT_ASSERT (lm->add (this, "test", "console://stdout", filters, 0));
  }

  void testFailureAdd ()
  {
    list<string> filters;

    CPPUNIT_ASSERT (lm->add (this, "test", "foo", filters, 0));
    CPPUNIT_ASSERT (lm->add (this, "test", "file", filters, 0));
    CPPUNIT_ASSERT (!lm->add (this, "test", "file://foo", filters, 0));
    CPPUNIT_ASSERT (lm->add (this, "test", "file://foo", filters, 0));
  }

  void testSharedAdd ()
  {
    list<string> filters;
    AnObject anObject;

    CPPUNIT_ASSERT (!lm->add (this, "test", "file://foo", filters, 0));
    CPPUNIT_ASSERT (!lm->add (&anObject, "test", "file://foo", filters, 0));
    CPPUNIT_ASSERT (!lm->add (this, "test1", "file://foo", filters, 0));
  }

  void testSharedGet ()
  {
    list<string> filters;
    AnObject anObject;
    list<void*> l;
    LogStream* ls;
    LogStream* ls1;

    CPPUNIT_ASSERT (!lm->add (this, "test", "file://foo", filters, 0));
    CPPUNIT_ASSERT (!lm->add (&anObject, "test", "file://foo", filters, 0));
    CPPUNIT_ASSERT (!lm->getOwnersList ("file://foo", &l));
    CPPUNIT_ASSERT (l.size () == 2);
    CPPUNIT_ASSERT (!lm->get (this, "test", "file://foo", &ls));
    CPPUNIT_ASSERT (!lm->get (this, "test", "file://foo", &ls1));
    CPPUNIT_ASSERT (ls1 == ls);
    CPPUNIT_ASSERT (!lm->add (this, "test1", "file://foo", filters, 0));
    CPPUNIT_ASSERT (!lm->getOwnersList ("file://foo", &l));
    CPPUNIT_ASSERT (l.size () == 2);
  }

  void testSharedRemove ()
  {
    list<string> filters;
    AnObject anObject;
    AnObject anotherObject;
    list<void*> l;

    CPPUNIT_ASSERT (!lm->add (this, "test", "file://foo", filters, 0));
    CPPUNIT_ASSERT (!lm->add (&anObject, "test1", "file://foo", filters, 0));
    CPPUNIT_ASSERT (!lm->add (&anotherObject, "test2", "file://foo", filters, 0));
    lm->remove (this);
    CPPUNIT_ASSERT (lm->contains ("file://foo"));
    lm->getOwnersList ("file://foo", &l);
    CPPUNIT_ASSERT (l.size () == 2);
    lm->remove (&anObject);
    CPPUNIT_ASSERT (lm->contains ("file://foo"));
    lm->getOwnersList ("file://foo", &l);
    CPPUNIT_ASSERT (l.size () == 1);
    lm->remove (&anotherObject);
    CPPUNIT_ASSERT (lm->getOwnersList ("file://foo", &l));
  }
  
  void testFailureLog ()
  {
    list<string> filters;

    lm->add (this, "test", "file://foo", filters, 0);
    lm->close (this, "test", "file://foo");
    CPPUNIT_ASSERT (lm->log (this, "test", "file://foo", "message"));
  }

  void testChown ()
  {
#ifdef NOT_WIN
    list<string> filters;

    lm->add (this, "test", "file://foo", filters, 0);
    CPPUNIT_ASSERT (!lm->chown (this, "test", "file://foo", getuid (), getgid ()));
#endif
  }

  void tearDown ()
  {
    delete lm;
    delete ff;
    FilesUtility::deleteFile ("foo");
  }

private:
  LogManager* lm;
  FiltersFactory* ff;
};

CPPUNIT_TEST_SUITE_REGISTRATION (TestLogManager);
