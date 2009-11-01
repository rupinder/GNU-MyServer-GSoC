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

#include <stdafx.h>
#include <ctype.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <include/plugin/plugin_info.h>

#include <string.h>

#include <iostream>
using namespace std;

class TestPluginInfo : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestPluginInfo );
  CPPUNIT_TEST ( testIsNotEnabled );
  CPPUNIT_TEST ( testIsEnabled );
  CPPUNIT_TEST ( testIsNotGlobal );
  CPPUNIT_TEST ( testIsGlobal );
  CPPUNIT_TEST ( testGetVersion );
  CPPUNIT_TEST ( testGetMyServerMinVersion );
  CPPUNIT_TEST ( testGetMyServerMaxVersion );
  CPPUNIT_TEST ( testGetName );
  CPPUNIT_TEST ( testDependencies );
  CPPUNIT_TEST ( testVersionConversionStringInt );
  CPPUNIT_TEST_SUITE_END ();
private:
  PluginInfo* pluginfo;

public:
  void setUp ()
  {
  	string name ("test");
	pluginfo = new PluginInfo (name,false,false);
  }

  void tearDown ()
  {
	delete (pluginfo);
  }

  void testIsNotEnabled ()
  {
  	CPPUNIT_ASSERT (!pluginfo->isEnabled ());
  }

  void testIsEnabled ()
  {
  	delete (pluginfo);
  	string name ("test");
  	pluginfo = new PluginInfo (name,true,false);
  	CPPUNIT_ASSERT (pluginfo->isEnabled ());
  }

  void testIsNotGlobal ()
  {
  	CPPUNIT_ASSERT (!pluginfo->isGlobal ());
  }

  void testIsGlobal ()
  {
  	delete (pluginfo);
  	string name ("test");
  	pluginfo = new PluginInfo (name,false,true);
  	CPPUNIT_ASSERT (pluginfo->isGlobal ());
  }

  void testGetVersion ()
  {
  	CPPUNIT_ASSERT_EQUAL (pluginfo->getVersion (),0);
  }

  void testGetMyServerMinVersion ()
  {
  	CPPUNIT_ASSERT_EQUAL (pluginfo->getMyServerMinVersion (),0);
  }

  void testGetMyServerMaxVersion ()
  {
  	CPPUNIT_ASSERT_EQUAL (pluginfo->getMyServerMaxVersion (),0);
  }

  void testGetName ()
  {
  	CPPUNIT_ASSERT_EQUAL (pluginfo->getName ().compare ("test"),0);
  }

  void testDependencies ()
  {
  	string test ("test-dep");
  	pluginfo->addDependence (test ,0,1);
  	CPPUNIT_ASSERT_EQUAL ( (*(pluginfo->begin ()))->first , 0);
  	CPPUNIT_ASSERT_EQUAL ( (*(pluginfo->begin ()))->second , 1);
  	CPPUNIT_ASSERT_EQUAL ( pluginfo->begin ().getKey ().compare ("test-dep"),0);
  }



  void testVersionConversionStringInt ()
  {
  	int v = PluginInfo::convertVersion (new string ("1"));
  	CPPUNIT_ASSERT_EQUAL (1<<24,v);

  	v = PluginInfo::convertVersion (new string ("255"));
  	CPPUNIT_ASSERT_EQUAL (255<<24,v);

  	v = PluginInfo::convertVersion (new string ("1.2"));
  	CPPUNIT_ASSERT_EQUAL ((1<<24) + (2<<16),v);

  	v = PluginInfo::convertVersion (new string ("1.2.3"));
  	CPPUNIT_ASSERT_EQUAL ((1<<24) + (2<<16) + (3<<8),v);

  	v = PluginInfo::convertVersion (new string ("1.2.3.4"));
  	CPPUNIT_ASSERT_EQUAL ((1<<24) + (2<<16) + (3<<8) + 4,v);

  	v = PluginInfo::convertVersion (new string ("1."));
  	CPPUNIT_ASSERT_EQUAL (-1,v);

  	v = PluginInfo::convertVersion (new string ("1.2."));
  	CPPUNIT_ASSERT_EQUAL (-1,v);

  	v = PluginInfo::convertVersion (new string ("1.2.3."));
  	CPPUNIT_ASSERT_EQUAL (-1,v);

  	v = PluginInfo::convertVersion (new string ("1.2.3.4."));
  	CPPUNIT_ASSERT_EQUAL (-1,v);

  	v = PluginInfo::convertVersion (new string ("1.2.3.4...."));
  	CPPUNIT_ASSERT_EQUAL (-1,v);

  	v = PluginInfo::convertVersion (new string ("......1.2.3.4"));
  	CPPUNIT_ASSERT_EQUAL (-1,v);

  	v = PluginInfo::convertVersion (new string ("1..2.3.4"));
  	CPPUNIT_ASSERT_EQUAL (-1,v);

  	v = PluginInfo::convertVersion (new string ("1.2...3.4"));
  	CPPUNIT_ASSERT_EQUAL (-1,v);

  	v = PluginInfo::convertVersion (new string ("1.2.3....4"));
  	CPPUNIT_ASSERT_EQUAL (-1,v);

  	v = PluginInfo::convertVersion (new string ("1.300.3.4"));
  	CPPUNIT_ASSERT_EQUAL (-1,v);

  	v = PluginInfo::convertVersion (new string ("1.3.299.4"));
  	CPPUNIT_ASSERT_EQUAL (-1,v);
  }
};


CPPUNIT_TEST_SUITE_REGISTRATION ( TestPluginInfo );
