#include <ctype.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

#include <include/conf/security/security_manager.h>
#include <include/conf/security/validator.h>

#include <string.h>

#include <iostream>
using namespace std;

class TestValidator : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( TestValidator );
  CPPUNIT_TEST( testGetPermissionMask );
  CPPUNIT_TEST( testGetPermissionMaskImpl );
  CPPUNIT_TEST( testGetValue );
  CPPUNIT_TEST_SUITE_END();

  Validator* validator;
public:
  void setUp()
  {
    validator = new Validator ();
  }

  void tearDown()
  {
    delete validator;
  }

  void testGetValue ()
  {
    string val ("value");
    HashMap<string, SecurityDomain*> hashedDomains;
    CPPUNIT_ASSERT_EQUAL (validator->getValue (&hashedDomains, val), (string*)NULL);
  }

  void testGetPermissionMaskImpl ()
  {
    string val ("value");
    SecurityToken secToken;
    CPPUNIT_ASSERT_EQUAL (validator->getPermissionMaskImpl (&secToken, NULL, NULL), 0);

  }

  void testGetPermissionMask ()
  {
    string val ("value");
    SecurityToken secToken;
    CPPUNIT_ASSERT_EQUAL (validator->getPermissionMask (&secToken, (SecurityDomain**) NULL, NULL), 0);
    CPPUNIT_ASSERT_EQUAL (validator->getPermissionMask (&secToken, (list<SecurityDomain*>*) NULL, NULL), 0);
   }
};


CPPUNIT_TEST_SUITE_REGISTRATION( TestValidator );
