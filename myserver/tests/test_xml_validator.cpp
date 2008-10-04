#include <ctype.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

#include <include/conf/security/security_manager.h>
#include <include/conf/security/xml_validator.h>

#include <string.h>

#include <iostream>
using namespace std;

class TestXmlValidator : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( TestXmlValidator );
  CPPUNIT_TEST( testGetPermissionMask );
  CPPUNIT_TEST( testGetPermissionMaskImpl );
  CPPUNIT_TEST( testGetValue );
  CPPUNIT_TEST_SUITE_END();
  
  XmlValidator* xmlValidator;
public:
  void setUp()
  {
    xmlValidator = new XmlValidator();
  }

  void tearDown()
  {
    delete xmlValidator;
  }

  void testGetValue()
  {
    string val("value");
    HashMap<string, SecurityDomain*> hashedDomains;

    CPPUNIT_ASSERT_EQUAL(xmlValidator->getValue(&hashedDomains, val), (string*)NULL);
 
  }
 
  void testGetPermissionMaskImpl()
  {
    string val("value");
    SecurityToken secToken;
    CPPUNIT_ASSERT_EQUAL(xmlValidator->getPermissionMaskImpl(&secToken, NULL, NULL), 0);
 
  }

  void testGetPermissionMask()
  {
    SecurityToken secToken;
    CPPUNIT_ASSERT_EQUAL (xmlValidator->getPermissionMask (&secToken, (SecurityDomain**) NULL, NULL), 0);
    CPPUNIT_ASSERT_EQUAL (xmlValidator->getPermissionMask (&secToken, (list<SecurityDomain*>*) NULL, NULL), 0);
  }
};


CPPUNIT_TEST_SUITE_REGISTRATION( TestXmlValidator );
