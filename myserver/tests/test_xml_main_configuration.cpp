/*
 MyServer
 Copyright (C) 2009 Free Software Foundation, Inc.
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

#include <stdafx.h>

#include <include/conf/main/xml_main_configuration.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

class TestXmlMainConfiguration : public CppUnit::TestFixture
{
  /* FIXME: Actually these tests don't really check if things work, simply they
     check that they don't work when they shouldn't.  */
	CPPUNIT_TEST_SUITE (TestXmlMainConfiguration);
	CPPUNIT_TEST (testOpen);
	CPPUNIT_TEST (testGetDoc);
	CPPUNIT_TEST (testClose);
	CPPUNIT_TEST_SUITE_END ();

public:
	void setUp () {}
	void tearDown () {}
	void testOpen ()
	{
    XmlMainConfiguration xmlConf;
		CPPUNIT_ASSERT (xmlConf.open ("bla/bla/bla"));
	}

	void testGetDoc ()
	{
    XmlMainConfiguration xmlConf;
		CPPUNIT_ASSERT_EQUAL (xmlConf.getDoc (), (xmlDocPtr)NULL);
	}

	void testClose ()
	{
    XmlMainConfiguration xmlConf;
		CPPUNIT_ASSERT (xmlConf.close ());
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION (TestXmlMainConfiguration);
