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
#include <include/base/crypt/crypt_algo_manager.h>
#include <include/base/crypt/md5.h>

#include <ctype.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <string.h>

#include <typeinfo>
using namespace std;

class TestCryptAlgoManager : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestCryptAlgoManager );
  CPPUNIT_TEST (testRegister);
  CPPUNIT_TEST (testCheck);
  CPPUNIT_TEST (testNoExists);
  CPPUNIT_TEST_SUITE_END ();

public:
  void setUp ()
  {

  }

  void tearDown ()
  {
  }

  void testRegister ()
  {
    string name ("md5");
    CryptAlgoManager cam;
    CryptAlgo *cal;

    cal = cam.make (name);
    CPPUNIT_ASSERT_EQUAL (cal, (CryptAlgo*) NULL);

    Md5::initialize (&cam);

    cal = cam.make (name);
    CPPUNIT_ASSERT (cal);
    CPPUNIT_ASSERT (typeid (*cal) == typeid (Md5));
    delete cal;

    cam.registerAlgorithm (name, NULL);
    cal = cam.make (name);
    CPPUNIT_ASSERT_EQUAL (cal, (CryptAlgo*) NULL);
  }

  void testCheck ()
  {
    string name ("md5");
    CryptAlgoManager cam;
    string value ("freedom");
    string result ("d5aa1729c8c253e5d917a5264855eab8");
    string wrong  ("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    Md5::initialize (&cam);

    CPPUNIT_ASSERT (cam.check (value, result, name));
    CPPUNIT_ASSERT (!cam.check (value, wrong, name));


    CPPUNIT_ASSERT (cam.check (value.c_str (), value.length (),
                               result.c_str (), name.c_str ()));

    CPPUNIT_ASSERT (!cam.check (value.c_str (), value.length (),
                               wrong.c_str (), name.c_str ()));
  }

  void testNoExists ()
  {
    CryptAlgoManager cam;
    bool raised = false;
    Md5::initialize (&cam);

    string value ("freedom");
    string result ("d5aa1729c8c253e5d917a5264855eab8");
    string name  ("it_does_not_exist");

    try
      {
        /* Using an algorithm that is not registered causes an
           exception.  */
        cam.check (value, result, name);
      }
    catch (...)
      {
        raised = true;
      }

    CPPUNIT_ASSERT (raised);
  }

};


CPPUNIT_TEST_SUITE_REGISTRATION (TestCryptAlgoManager);
