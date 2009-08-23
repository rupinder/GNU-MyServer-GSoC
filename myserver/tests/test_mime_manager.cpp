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

#include <ctype.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

#include <include/conf/mime/mime_manager.h>

#include <string.h>

#include <iostream>
using namespace std;


class TestMimeManager : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestMimeManager );
  CPPUNIT_TEST ( testLoadXML );
  CPPUNIT_TEST_SUITE_END ();

  MimeManager *mm;
public:
  void setUp ()
  {
    mm = new MimeManager;
  }

  void tearDown ()
  {
    delete mm;
  }

  void testLoadXML ()
  {
    XmlParser *parser = getTestParser ();

    CPPUNIT_ASSERT_EQUAL (mm->isLoaded(), false);

    CPPUNIT_ASSERT_EQUAL (mm->loadXML (parser), 2ul);
    CPPUNIT_ASSERT_EQUAL (mm->getNumMIMELoaded(), 2ul);

    CPPUNIT_ASSERT_EQUAL (mm->isLoaded(), true);

    delete parser;
  }

  void testAddRecord ()
  {
    XmlParser *parser = getTestParser ();
    MimeRecord mr;
    mm->loadXML (parser);

    CPPUNIT_ASSERT_EQUAL (mm->getNumMIMELoaded(), 2ul);

    mm->addRecord (&mr);

    CPPUNIT_ASSERT_EQUAL (mm->getNumMIMELoaded(), 3ul);

    delete parser;
  }

private:

  //Returns a XML object with 2 MIME.
  XmlParser *getTestParser ()
  {
    MemBuf mb;
    XmlParser *parser = new XmlParser ();

    const char *buffer = "<?xml version=\"1.0\"?>\n<MIMES>\n\
       <MIME mime=\"text/html\" handler=\"SEND\" param=\"\">\n                 \
       <EXTENSION value=\"htm\"/>\n<EXTENSION value=\"html\"/>\n\
       </MIME>\n<MIME mime=\"text/html\" handler=\"SEND\" param=\"\">\n\
       <EXTENSION value=\"txt\"/>\n</MIME>\n</MIMES>\n";

    mb.addBuffer (buffer, strlen (buffer));
    parser->openMemBuf(mb);

    return parser;
  }
};


CPPUNIT_TEST_SUITE_REGISTRATION( TestMimeManager );
