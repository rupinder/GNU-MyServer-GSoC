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

#include <include/conf/security/auth_method.h>

#include <ctype.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <string.h>

#include <typeinfo>
using namespace std;


class TestAuthMethodImpl : public AuthMethod
{
public:
  bool exposeComparePassword (const char *password, const char *savedPassword,
                              const char *algorithm)
  {
    return AuthMethod::comparePassword (password, savedPassword, algorithm);
  }
};


class TestAuthMethod : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestAuthMethod );
  CPPUNIT_TEST (testCryptAlgoManager);
  CPPUNIT_TEST (testGetPermissionMask);
  CPPUNIT_TEST (testComparePassword);
  CPPUNIT_TEST (testNoException);
  CPPUNIT_TEST_SUITE_END ();

public:
  void setUp ()
  {

  }

  void tearDown ()
  {
  }

  void testCryptAlgoManager ()
  {
    CryptAlgoManager cam;
    TestAuthMethodImpl tam;

    tam.setCryptAlgoManager (&cam);

    CPPUNIT_ASSERT_EQUAL (&cam, tam.getCryptAlgoManager ());
  }

  void testGetPermissionMask ()
  {
    TestAuthMethodImpl tam;
    SecurityToken st;
    CPPUNIT_ASSERT (tam.getPermissionMask (&st) >= 0);
  }

  void testNoException ()
  {
    TestAuthMethodImpl tam;
    CryptAlgoManager cam;
    Md5::initialize (&cam);

    try
      {
        tam.exposeComparePassword ("d5aa1729c8c253e5d917a5264855eab8",
                                   "freedom",
                                   "md5555");
      }
    catch (...)
      {
        CPPUNIT_FAIL ("exception raised!");
      }
  }

  void testComparePassword ()
  {
    TestAuthMethodImpl tam;
    CryptAlgoManager cam;
    Md5::initialize (&cam);

    CPPUNIT_ASSERT (!tam.exposeComparePassword
                    ("d5aa1729c8c253e5d917a5264855eab8", "freedom",
                     "md5"));

    tam.setCryptAlgoManager (&cam);


    CPPUNIT_ASSERT (tam.exposeComparePassword
                    ("d5aa1729c8c253e5d917a5264855eab8", "freedom",
                     "md5"));


    CPPUNIT_ASSERT (!tam.exposeComparePassword
                    ("d5aa1729c8c253e5d917a5264855ea8b", "freedom",
                     "md5"));
  }
};


CPPUNIT_TEST_SUITE_REGISTRATION (TestAuthMethod);
