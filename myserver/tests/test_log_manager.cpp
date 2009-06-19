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

#ifndef WIN32
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
  CPPUNIT_TEST (testNotEmpty);
  CPPUNIT_TEST (testNotContains);
  CPPUNIT_TEST (testContainsOwner);
  CPPUNIT_TEST (testFileStreamAdd);
  CPPUNIT_TEST (testEmptyRemove);
  CPPUNIT_TEST (testFileStreamRemove);
  CPPUNIT_TEST (testFileStreamLog);
  CPPUNIT_TEST (testFileStreamClose);
  CPPUNIT_TEST (testCloseAnAlreadyClosedFileStream);
  CPPUNIT_TEST (testFileStreamCycle);
  CPPUNIT_TEST (testMessageLevelGreaterThanLogManagerOne);
  CPPUNIT_TEST (testMessageLevelLessThanLogManagerOne);
  CPPUNIT_TEST (testMessageLevelEqualToLogManagerOne);
  CPPUNIT_TEST (testClear);
  CPPUNIT_TEST (testLogThroughGzip);
  CPPUNIT_TEST (testCycleWithGzipChain);
  CPPUNIT_TEST (testCountSingleLogStream);
  CPPUNIT_TEST (testCountSameTypeLogStreams);
  CPPUNIT_TEST (testCountAllLogStreamsOwnedByAnObject);
  CPPUNIT_TEST (testGetWhenNotOwningAnyLogStream);
  CPPUNIT_TEST (testGetAllOwnedLogStreamsList);
  CPPUNIT_TEST (testGetAllLogStreamsOfTheSameType);
  CPPUNIT_TEST (testGetASingleExistingLogStream);
  CPPUNIT_TEST (testGetASingleNotExistingLogStream);
  CPPUNIT_TEST (testReOpen);
  CPPUNIT_TEST (testFileStreamAddWithNotExistingFilter);
  CPPUNIT_TEST (testSocketStreamAddWithNotExistingFilter);
  CPPUNIT_TEST (testConsoleStreamAddWithNotExistingFilter);
  CPPUNIT_TEST (testAddMalformedLocation);
  CPPUNIT_TEST (testAddMultipleTimesTheSameKey);
  CPPUNIT_TEST (testAddLogStreamSharedByDistinctObjects);
  CPPUNIT_TEST (testAddLogStreamSharedByTheSameObject);
  CPPUNIT_TEST (testGetLogStreamSharedByDistinctObjects);
  CPPUNIT_TEST (testGetLogStreamSharedByTheSameObject);
  CPPUNIT_TEST (testStillKeepSharedLogStreamIfAnyOfItsOwnersGetRemoved);
  CPPUNIT_TEST (testLogOnAClosedLogStream);
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
  
  void testNotEmpty ()
  {
    list<string> filters;

    lm->add (this, "test", "file://foo", filters, 0);

    CPPUNIT_ASSERT (!lm->empty ());
  }

  void testNotContains ()
  {
    CPPUNIT_ASSERT (!lm->contains ("file://foo"));
  }

  void testContainsOwner ()
  {
    list<string> filters;
    
    lm->add (this, "test", "file://foo", filters, 0);

    CPPUNIT_ASSERT (lm->contains (this));
  }

  void testFileStreamAdd ()
  {
    list<string> filters;

    CPPUNIT_ASSERT (!lm->add (this, "test", "file://foo", filters, 0));
  }

  void testEmptyRemove ()
  {
    CPPUNIT_ASSERT (lm->remove (this));
  }

  void testFileStreamRemove ()
  {
    list<string> filters;

    lm->add (this, "test", "file://foo", filters, 0);
    lm->remove (this);

    CPPUNIT_ASSERT (!lm->contains (this));
  }

  void testFileStreamLog ()
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
    f.close ();

    CPPUNIT_ASSERT (!message.compare (buf));
  }
  
  void testFileStreamClose ()
  {
    list<string> filters;

    lm->add (this, "test", "file://foo", filters, 0);

    CPPUNIT_ASSERT (!lm->close (this, "test", "file://foo"));
  }

  void testCloseAnAlreadyClosedFileStream ()
  {
    list<string> filters;

    lm->add (this, "test", "file://foo", filters, 0);
    lm->close (this, "test", "file://foo");

    CPPUNIT_ASSERT (lm->close (this, "test", "file://foo"));
  }

  void testFileStreamCycle ()
  {
    string message ("A message\n");
    string message1 ("Another message\n");
    string gotMessage;
    string gotMessage1;
    list<string> filters;
    File f;
    char buf[64];
    u_long nbr;
    LogStream* ls;
    list<string> cs;
    list<string>::iterator it;

    lm->add (this, "test", "file://foo", filters, 10);
    lm->log (this, "test", "file://foo", message);
    lm->log (this, "test", "file://foo", message1);
    lm->close (this, "test", "file://foo");
    f.openFile ("foo", FileStream::defaultFileMask);
    f.read (buf, 64, &nbr);
    buf[nbr] = '\0';
    f.close ();
    gotMessage1.assign (buf);
    lm->get (this, "test", "file://foo", &ls);
    cs = ls->getCycledStreams ();
    for (it = cs.begin (); it != cs.end (); it++)
      {
        f.openFile (*it, FileStream::defaultFileMask);
        f.read (buf, 64, &nbr);
        buf[nbr] = '\0';
        f.close ();
        gotMessage.assign (buf);
        FilesUtility::deleteFile (*it);
      }

    CPPUNIT_ASSERT (!message1.compare (gotMessage1));
    CPPUNIT_ASSERT (!message.compare (gotMessage));
  }

  void testMessageLevelGreaterThanLogManagerOne ()
  {
    list<string> filters;

    lm->add (this, "test", "file://foo", filters, 0);
    lm->setLevel (MYSERVER_LOG_MSG_WARNING);

    CPPUNIT_ASSERT (!lm->log (this, "test", "a message", false, MYSERVER_LOG_MSG_ERROR));
  }

  void testMessageLevelLessThanLogManagerOne ()
  {
    list<string> filters;

    lm->add (this, "test", "file://foo", filters, 0);
    lm->setLevel (MYSERVER_LOG_MSG_WARNING);

    CPPUNIT_ASSERT (lm->log (this, "test", "a message", false, MYSERVER_LOG_MSG_INFO));
  }

  void testMessageLevelEqualToLogManagerOne ()
  {
    list<string> filters;

    lm->add (this, "test", "file://foo", filters, 0);
    lm->setLevel (MYSERVER_LOG_MSG_WARNING);

    CPPUNIT_ASSERT (!lm->log (this, "test", "a message", false, MYSERVER_LOG_MSG_WARNING));    
  }

  void testClear ()
  {
    list<string> filters;

    lm->add (this, "test", "file://foo", filters, 0);
    lm->add (this, "test", "console://stdout", filters, 0);
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
    string gotMessage;
    string gotMessage1;
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
    lm->log (this, "test", "file://foo", message1);
    f.openFile ("foo", FileStream::defaultFileMask);
    f.read (gzipComp, 128, &nbr);
    f.close ();
    gzdc.decompress (&gzipComp[gzdc.headerSize ()], 
                     nbr - gzdc.headerSize (),
                     gzipDecomp, 128);
    gzipDecomp[message1.size ()] = '\0';
    gotMessage1.assign (gzipDecomp);
    lm->get (this, "test", "file://foo", &ls);
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
        gotMessage.assign (gzipDecomp);
        FilesUtility::deleteFile (*it);
      }

    CPPUNIT_ASSERT (!message1.compare (gotMessage1));
    CPPUNIT_ASSERT (!message.compare (gotMessage));
  }

  void testCountSingleLogStream ()
  {
    list<string> filters;

    lm->add (this, "test", "console://stdout", filters, 0);
    lm->add (this, "test_1", "console://stderr", filters, 0);
    lm->add (this, "test_1", "file://foo", filters, 0);

    CPPUNIT_ASSERT_EQUAL (lm->count (this, "test_1", "file://foo"), 1);
  }

  void testCountSameTypeLogStreams ()
  {
    list<string> filters;

    lm->add (this, "test", "console://stdout", filters, 0);
    lm->add (this, "test_1", "console://stderr", filters, 0);
    lm->add (this, "test_1", "file://foo", filters, 0);

    CPPUNIT_ASSERT_EQUAL (lm->count (this, "test_1"), 2);
  }

  void testCountAllLogStreamsOwnedByAnObject ()
  {
    list<string> filters;

    lm->add (this, "test", "console://stdout", filters, 0);
    lm->add (this, "test_1", "console://stderr", filters, 0);
    lm->add (this, "test_1", "file://foo", filters, 0);

    CPPUNIT_ASSERT_EQUAL (lm->count (this), 3);
  }
  
  void testGetWhenNotOwningAnyLogStream ()
  {
    list<string> l;

    CPPUNIT_ASSERT (lm->get (this, &l));
  }

  void testGetAllOwnedLogStreamsList ()
  {
    list<string> filters;
    list<string> tmp;
    list<string> l;
    
    lm->add (this, "test", "file://foo", filters, 0);
    lm->add (this, "test", "console://stdout", filters, 0);
    lm->add (this, "test_1", "console://stderr", filters, 0);
    tmp.push_back ("file://foo");
    tmp.push_back ("console://stdout");
    tmp.push_back ("console://stderr");
    lm->get (this, &l);
    tmp.sort (); 
    l.sort ();

    CPPUNIT_ASSERT (tmp == l);
  }

  void testGetAllLogStreamsOfTheSameType ()
  {
    list<string> filters;
    list<string> tmp;
    list<string> l;

    lm->add (this, "test", "file://foo", filters, 0);
    lm->add (this, "test", "console://stdout", filters, 0);
    lm->add (this, "test_1", "console://stderr", filters, 0);
    tmp.push_back ("file://foo");
    tmp.push_back ("console://stdout");
    lm->get (this, "test", &l);
    tmp.sort (); 
    l.sort ();

    CPPUNIT_ASSERT (tmp == l);
  }
  
  void testGetASingleExistingLogStream ()
  {
    list<string> filters;
    LogStream* ls;

    lm->add (this, "test", "file://foo", filters, 0);

    CPPUNIT_ASSERT (!lm->get (this, "test", "file://foo", &ls));
  }

  void testGetASingleNotExistingLogStream ()
  {
    list<string> filters;
    LogStream* ls;
    
    lm->add (this, "test", "file://foo", filters, 0);

    CPPUNIT_ASSERT (lm->get (this, "test", "file://bar", &ls));
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
    lm->add (this, "test", "file://foo", filters, 0);
    lm->log (this, "test", "file://foo", message2);
    lm->clear ();
    f.openFile ("foo", FileStream::defaultFileMask);
    f.read (buf, 128, &nbr);
    f.close ();
    buf[nbr] = '\0';

    CPPUNIT_ASSERT (!string (buf).compare (message1.append (message2)));
  }

  void testFileStreamAddWithNotExistingFilter ()
  {
    list<string> filters;

    filters.push_back ("not_existing_filter");

    CPPUNIT_ASSERT (lm->add (this, "test", "file://foo", filters, 0));
  }

  void testSocketStreamAddWithNotExistingFilter ()
  {
    list<string> filters;

    filters.push_back ("not_existing_filter");

    CPPUNIT_ASSERT (lm->add (this, "test", "socket://127.0.0.1:6666", filters, 0));
  }

  void testConsoleStreamAddWithNotExistingFilter ()
  {
    list<string> filters;

    filters.push_back ("not_existing_filter");

    CPPUNIT_ASSERT (lm->add (this, "test", "console://stdout", filters, 0));
  }

  void testAddMalformedLocation ()
  {
    list<string> filters;

    CPPUNIT_ASSERT (lm->add (this, "test", "file", filters, 0));
  }

  void testAddMultipleTimesTheSameKey ()
  {
    list<string> filters;

    lm->add (this, "test", "file://foo", filters, 0);

    CPPUNIT_ASSERT (lm->add (this, "test", "file://foo", filters, 0));
  }

  void testAddLogStreamSharedByDistinctObjects ()
  {
    list<string> filters;
    AnObject anObject;

    lm->add (this, "test", "file://foo", filters, 0);

    CPPUNIT_ASSERT (!lm->add (&anObject, "test", "file://foo", filters, 0));
  }

  void testAddLogStreamSharedByTheSameObject ()
  {
    list<string> filters;

    lm->add (this, "test", "file://foo", filters, 0);

    CPPUNIT_ASSERT (!lm->add (this, "test1", "file://foo", filters, 0));
  }

  void testGetLogStreamSharedByDistinctObjects ()
  {
    list<string> filters;
    AnObject anObject;
    list<void*> l;
    LogStream* ls;
    LogStream* ls1;

    lm->add (this, "test", "file://foo", filters, 0);
    lm->add (&anObject, "test", "file://foo", filters, 0);
    lm->getOwnersList ("file://foo", &l);
    lm->get (this, "test", "file://foo", &ls);
    lm->get (&anObject, "test", "file://foo", &ls1);

    CPPUNIT_ASSERT (ls == ls1);
  }

  void testGetLogStreamSharedByTheSameObject ()
  {
    list<string> filters;
    list<void*> l;
    LogStream* ls;
    LogStream* ls1;

    lm->add (this, "test", "file://foo", filters, 0);
    lm->add (this, "test1", "file://foo", filters, 0);
    lm->getOwnersList ("file://foo", &l);
    lm->get (this, "test", "file://foo", &ls);
    lm->get (this, "test1", "file://foo", &ls1);

    CPPUNIT_ASSERT (ls == ls1);
  }

  void testStillKeepSharedLogStreamIfAnyOfItsOwnersGetRemoved ()
  {
    list<string> filters;
    AnObject anObject;
    AnObject anotherObject;
    list<void*> l;

    lm->add (this, "test", "file://foo", filters, 0);
    lm->add (&anObject, "test1", "file://foo", filters, 0);
    lm->add (&anotherObject, "test2", "file://foo", filters, 0);
    lm->remove (this);
    lm->remove (&anObject);

    CPPUNIT_ASSERT (lm->contains ("file://foo"));
  }

  void testRemoveALogStreamWhenAllItsOwnersGetRemoved ()
  {
    list<string> filters;
    AnObject anObject;
    AnObject anotherObject;
    list<void*> l;
    
    lm->add (this, "test", "file://foo", filters, 0);
    lm->add (&anObject, "test1", "file://foo", filters, 0);
    lm->add (&anotherObject, "test2", "file://foo", filters, 0);
    lm->remove (this);
    lm->remove (&anObject);
    lm->remove (&anotherObject);
    
    CPPUNIT_ASSERT (!lm->contains ("file://foo"));
  }

  void testLogOnAClosedLogStream ()
  {
    list<string> filters;

    lm->add (this, "test", "file://foo", filters, 0);
    lm->close (this, "test", "file://foo");

    CPPUNIT_ASSERT (lm->log (this, "test", "file://foo", "message"));
  }

  void testChown ()
  {
#ifndef WIN32
    list<string> filters;

    lm->add (this, "test", "file://foo", filters, 0);
    ostringstream uidOss;
    uidOss << ::getuid ();
    string uid (uidOss.str ());

    ostringstream gidOss;
    gidOss << ::getuid ();
    string gid (gidOss.str ());

    CPPUNIT_ASSERT (!lm->chown (this, "test", "file://foo", uid, gid));
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
