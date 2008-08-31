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
#include <include/base/xml/xml_parser.h>
#include <string.h>
#include <string>

using namespace std;


class TestXml : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( TestXml );
  CPPUNIT_TEST( testOpenMemBuf );
  CPPUNIT_TEST( testXPath );
  CPPUNIT_TEST_SUITE_END();

  MemBuf* getXmlMemBuf()
  {
    MemBuf* memBuf = new MemBuf();

    *memBuf << "<?xml version=\"1.0\"?>"
            << "<ROOT>"
            << "<NODE><ELEMENT name=\"a\">a</ELEMENT></NODE>"
            << "<NODE><ELEMENT>b</ELEMENT><ELEMENT>c</ELEMENT></NODE>"
            << "<NODE><ELEMENT>d</ELEMENT><ELEMENT>e</ELEMENT></NODE>"
            << "<NODE><ELEMENT>f</ELEMENT></NODE>"
            << "</ROOT>";
    
    return memBuf;
  }

public:

  void setUp()
  {
 
  }

  void tearDown()
  {

  }
  
  void testOpenMemBuf()
  {
    MemBuf* memBuf = getXmlMemBuf();

    XmlParser *xml = new XmlParser();
    int ret = xml->openMemBuf(*memBuf);

    CPPUNIT_ASSERT_EQUAL(ret, 0);

    delete memBuf;
    delete xml;
  }

  void testXPath()
  {
    MemBuf* memBuf = getXmlMemBuf();

    XmlParser *xml = new XmlParser();
    xml->openMemBuf(*memBuf, true);


    XmlXPathResult* xpathRes = xml->evaluateXpath("/ROOT/NODE/ELEMENT[@name='a']/text()");

    CPPUNIT_ASSERT(xpathRes);

    xmlXPathObjectPtr obj = xpathRes->getObject();

    xmlNodeSetPtr nodes = xpathRes->getNodeSet();

    CPPUNIT_ASSERT(obj);

    CPPUNIT_ASSERT_EQUAL(nodes->nodeNr, 1);
    
    CPPUNIT_ASSERT(!strcmp((const char*)nodes->nodeTab[0]->content, "a"));

    delete xpathRes;
    delete memBuf;
    delete xml;
  }

};


CPPUNIT_TEST_SUITE_REGISTRATION( TestXml );
