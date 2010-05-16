/*
  MyServer
  Copyright (C) 2009, 2010 Free Software Foundation, Inc.
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

#include "myserver.h"

#include <include/conf/main/xml_main_configuration.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

#include <include/base/file/files_utility.h>

#define KEY "foo.bar"
#define VALUE "value"
#define XML_FILE "foo.xml"
#define XML_CONTENT "<?xml version=\"1.0\"?>\n<MYSERVER>"   \
  "<DEFINE name=\"" KEY "\" value=\"garbage\" />"             \
  "<DEFINE name=\"" KEY "\" value=\"" VALUE "\" />"             \
  "</MYSERVER>\n"

class TestXmlMainConfiguration : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE (TestXmlMainConfiguration);
  CPPUNIT_TEST (testOpen);
  CPPUNIT_TEST (testGetDoc);
  CPPUNIT_TEST (testGetValue);
  CPPUNIT_TEST (testClose);
  CPPUNIT_TEST_SUITE_END ();

  File xmlFile;
  XmlMainConfiguration xmlConf;
public:
  void setUp ()
  {
    u_long nbw;
    xmlFile.openFile (XML_FILE, File::WRITE | File::READ
                      | File::FILE_OPEN_ALWAYS);
    CPPUNIT_ASSERT_EQUAL (xmlFile.write (XML_CONTENT, strlen (XML_CONTENT),
                                           &nbw), 0);
    CPPUNIT_ASSERT_EQUAL (nbw, static_cast<u_long> (strlen (XML_CONTENT)));

    CPPUNIT_ASSERT_EQUAL (xmlConf.open (XML_FILE), 0);
  }
  void tearDown ()
  {
    CPPUNIT_ASSERT_EQUAL (xmlConf.close (), 0);
    FilesUtility::deleteFile (XML_FILE);
  }

  void testOpen ()
  {
    XmlMainConfiguration tmpXmlConf;
    int exceptions = 0;
    try
      {
        tmpXmlConf.open ("baz.xml");
      }
    catch (exception & e)
      {
        exceptions++;
      }
    try
      {
        tmpXmlConf.open ("foo/bar/baz.xml");
      }
    catch (exception & e)
      {
        exceptions++;
      }

    CPPUNIT_ASSERT_EQUAL (exceptions, 2);

    CPPUNIT_ASSERT_EQUAL (tmpXmlConf.open (XML_FILE), 0);
    CPPUNIT_ASSERT_EQUAL (tmpXmlConf.close (), 0);
  }

  void testGetDoc ()
  {
    XmlMainConfiguration tmpXmlConf;
    CPPUNIT_ASSERT_EQUAL (tmpXmlConf.getDoc (), static_cast <xmlDocPtr> (NULL));
    CPPUNIT_ASSERT (xmlConf.getDoc ());
  }

  void testGetValue ()
  {
    const char *ret = xmlConf.getValue (KEY);
    CPPUNIT_ASSERT (ret);
    CPPUNIT_ASSERT (! strcmp (ret, VALUE));
    CPPUNIT_ASSERT_EQUAL (strcmp (ret, VALUE), 0);
  }

  void testClose ()
  {
    XmlMainConfiguration tmpXmlConf;

    CPPUNIT_ASSERT (tmpXmlConf.close ());
    CPPUNIT_ASSERT_EQUAL (tmpXmlConf.open (XML_FILE), 0);
    CPPUNIT_ASSERT_EQUAL (tmpXmlConf.close (), 0);
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION (TestXmlMainConfiguration);
