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
#include <include/server/server.h>

#undef open
#undef close

#include <cppunit/TextOutputter.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/XmlOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

#include <fstream>
#include <string.h>

using namespace std;

const char *program_name = NULL;

int main (int argc, char* argv[])
{
  bool xml = argc > 1 && !strcmp (argv[1], "xml");
  bool compiler = argc > 1 && !strcmp (argv[1], "compiler");
  char *filename = argc > 2 ? argv[2] : NULL;

  program_name = argv[0];

  std::ostream *str = &(std::cerr);

  ofstream ofile;

  if (filename)
    {
      ofile.open (filename);
      str = &ofile;
    }

  Server::createInstance ();
  Server::getInstance ()->loadLibraries ();

  CppUnit::Outputter * out;
  CppUnit::Test *suite = CppUnit::TestFactoryRegistry::getRegistry ().makeTest ();
  CppUnit::TextUi::TestRunner runner;
  runner.addTest ( suite );

  if (xml)
    out = new CppUnit::XmlOutputter ( &runner.result (), *str );
  else if (compiler)
    out = new CppUnit::CompilerOutputter ( &runner.result (), *str );
  else
    out = new CppUnit::TextOutputter ( &runner.result (), *str );

  runner.setOutputter ( out );

  int ret = runner.run () ? 0 : 1;

  if (filename)
    ofile.close ();

  return ret;
}
