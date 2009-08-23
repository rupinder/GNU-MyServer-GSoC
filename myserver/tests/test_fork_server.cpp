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

#include <include/base/process/fork_server.h>
#include <include/base/process/process.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <include/base/pipe/pipe.h>

#include <exception>
#include <string.h>

using namespace std;

class TestForkServer : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( TestForkServer );
  CPPUNIT_TEST( testExecuteProcess );
  CPPUNIT_TEST (testStartKillLoop);
  CPPUNIT_TEST_SUITE_END();

  ForkServer *fs;

public:
  void setUp()
  {
    fs = new ForkServer;
  }

  void tearDown()
  {
    delete fs;
  }

  void testStartKillLoop ()
  {
#ifndef WIN32
    CPPUNIT_ASSERT_EQUAL (fs->isInitialized (), false);

    int ret = fs->startForkServer ();

    CPPUNIT_ASSERT_EQUAL (ret, 0);

    CPPUNIT_ASSERT_EQUAL (fs->isInitialized (), true);

    fs->killServer ();
#endif
  }

  void testExecuteProcess()
  {
    try
      {
#ifndef WIN32
        int pid = 0;
        int port = 0;
        StartProcInfo spi;
        char buffer [32] = {'\0'};
        const char *msg = "ForkServer";
        u_long nbr;
        int ret = fs->startForkServer ();

        Pipe pipe;
        pipe.create();

        spi.stdIn = -1;
        spi.stdError = -1;
        spi.stdOut =  pipe.getWriteHandle();

        spi.cmd.assign ("/bin/echo");
        spi.arg.assign (msg);
        spi.cwd.assign ("");
        spi.envString = NULL;

        ret = fs->executeProcess (&spi, ForkServer::FLAG_USE_OUT, &pid, &port);
        pipe.closeWrite ();

        CPPUNIT_ASSERT_EQUAL (ret, 0);

        ret = pipe.read (buffer, 32, &nbr);

        CPPUNIT_ASSERT_EQUAL (ret, 0);

        CPPUNIT_ASSERT_EQUAL (strncmp (buffer, msg, strlen (msg)), 0);

        pipe.closeRead ();

        fs->killServer ();
#endif
      }
    catch (exception &e)
      {
        CPPUNIT_FAIL (e.what ());
      }
  }

};
CPPUNIT_TEST_SUITE_REGISTRATION( TestForkServer );
