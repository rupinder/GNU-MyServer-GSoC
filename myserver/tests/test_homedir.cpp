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
#include <include/base/home_dir/home_dir.h>
#include <include/base/file/files_utility.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

#include <errno.h>

class TestHomeDir : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestHomeDir );
  CPPUNIT_TEST ( testLoadClear );
  CPPUNIT_TEST ( testGetAdmin );
  CPPUNIT_TEST_SUITE_END ();
  HomeDir *homeDir;
public:
  void setUp () {homeDir = new HomeDir ();}
  void tearDown () {delete homeDir;}

  void testLoadClear ()
  {
    CPPUNIT_ASSERT (!homeDir->isLoaded ());

    homeDir->load ();

    CPPUNIT_ASSERT (homeDir->isLoaded ());

    homeDir->clear ();

    CPPUNIT_ASSERT (!homeDir->isLoaded ());
  }

  void testGetAdmin ()
  {
    homeDir->load ();
    string username;
#ifdef WIN32
    /* Try to get home dir for Administrator under Windows.  */
    username.assign ("Administrator");
#else
    /* Under systems different than Windows, "root" should be present,
     * if it doesn't handle this differently.  */
    username.assign ("root");
#endif
    string dir;

#if !WIN32
    errno = 0;
#endif

    int ret = homeDir->getHomeDir (username, dir);

    /* FIXME: handle this case in homeDir::getHomeDir.  */
#if GETPWNAM
    /* If the user could not be found, silently return.  */
    if (ret && (errno == ENOENT || errno == ESRCH || errno == EBADF
                || errno == EPERM))
      return;
#elif !WIN32
    if (! FilesUtility::fileExists ("/etc/passwd"))
      return;
#endif

    CPPUNIT_ASSERT_EQUAL (ret, 0);

    CPPUNIT_ASSERT (dir.length ());

    homeDir->clear ();
  }

};
CPPUNIT_TEST_SUITE_REGISTRATION ( TestHomeDir );
