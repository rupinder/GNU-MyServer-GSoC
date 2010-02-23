/*
  MyServer
  Copyright (C) 2008, 2009, 2010 Free Software Foundation, Inc.
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
#include <ctype.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

#include <include/plugin/plugins_manager.h>
#include <include/server/server.h>
#include <include/plugin/plugin_info.h>

#include <string.h>

#include <list>
#include <string>
#include <iostream>

using namespace std;

class MockPlugin : public Plugin
{
public:
  virtual int load (Server* server)
  {
    return 0;
  }

  virtual int preLoad (string& file, bool global)
  {
    return 0;
  }

  virtual int postLoad (Server* server)
  {
    return 0;
  }

  virtual int unLoad ()
  {
    return 0;
  }

  virtual const char* getName (char* buffer, u_long len)
  {
    return 0;
  }

  virtual void* getDirectMethod (char* name)
  {
    return 0;
  }

};

class MockPluginsManager : public PluginsManager
{
public:
  vector<string> names;
  vector<string> files;
  vector<PluginInfo*> pinfos;
  void clear ()
  {
    names.clear ();
    files.clear ();
    pinfos.clear ();
  }

protected:
  virtual int loadFile (Server* server, string &name, string &file,
                        PluginInfo* pinfo)
  {
    names.push_back (name);
    files.push_back (file);
    pinfos.push_back (pinfo);
    return 0;
  }

  virtual Plugin* preLoadPlugin (string &file, Server* server, bool global)
  {
    new MockPlugin ();
  }
};

class TestPluginsManager : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE (TestPluginsManager );
  CPPUNIT_TEST ( testQuickLoad );
  CPPUNIT_TEST_SUITE_END ();
private:
  static int addPlugin (Server* server, string &name,
                        string &file, PluginInfo* pinfo)
  {

  }

public:
  void setUp ()
  {
  }

  void tearDown ()
  {
  }

  void testQuickLoad ()
  {
    MockPluginsManager manager;
    int ret = manager.quickLoad (Server::getInstance (),
                                 "name1:plugin1.so,name2:plugin2.so");

    CPPUNIT_ASSERT_EQUAL (ret, 0);
    CPPUNIT_ASSERT_EQUAL (manager.names[0].compare ("name1"), 0);
    CPPUNIT_ASSERT_EQUAL (manager.names[1].compare ("name2"), 0);

    CPPUNIT_ASSERT_EQUAL (manager.files[0].compare ("plugin1.so"), 0);
    CPPUNIT_ASSERT_EQUAL (manager.files[1].compare ("plugin2.so"), 0);
  }
};


CPPUNIT_TEST_SUITE_REGISTRATION (TestPluginsManager);
