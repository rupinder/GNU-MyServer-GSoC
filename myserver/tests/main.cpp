#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

char *versionOfSoftware = "MyServer";

int main(int argc, char* argv[])
{
  CppUnit::Test *suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();
  CppUnit::TextUi::TestRunner runner;
  runner.addTest( suite );
  CppUnit::Outputter * out = new CppUnit::CompilerOutputter( &runner.result(), std::cerr );

  runner.setOutputter( out );

  bool wasSucessful = runner.run();

  return wasSucessful ? 0 : 1;
}
