#include <ctype.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

#include <include/conf/security/security_manager.h>
#include <include/conf/security/validator_factory.h>
#include <include/conf/security/validator.h>

#include <string.h>

#include <iostream>
using namespace std;


class TestValidatorFactory : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( TestValidatorFactory );
  CPPUNIT_TEST( testGetValidator );
  CPPUNIT_TEST( testAddValidator );
  CPPUNIT_TEST( testIsValidatorPresent );
  CPPUNIT_TEST_SUITE_END();
  
  ValidatorFactory* factory;
public:
  void setUp()
  {
    factory = new ValidatorFactory();
  }

  void tearDown()
  {
    delete factory;
  }

  void testGetValidator()
  {
    string val("foo");
    CPPUNIT_ASSERT_EQUAL(factory->getValidator(val), (Validator*)NULL);
 
  }
 
  void testAddValidator()
  {
    string val("bar");
    Validator *validator = new Validator;
   
    Validator* old = factory->addValidator(val, validator);

    CPPUNIT_ASSERT_EQUAL(old, (Validator*)NULL);
    CPPUNIT_ASSERT(factory->getValidator(val));
  }

  void testIsValidatorPresent()
  {
    string val("bar");
    Validator *validator = new Validator;

 
    CPPUNIT_ASSERT_EQUAL(factory->isValidatorPresent(val), false);
   
    factory->addValidator(val, validator);
 
    CPPUNIT_ASSERT_EQUAL(factory->isValidatorPresent(val), true);
  }
};


CPPUNIT_TEST_SUITE_REGISTRATION( TestValidatorFactory );
