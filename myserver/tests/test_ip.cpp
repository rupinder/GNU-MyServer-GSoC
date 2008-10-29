/*
 MyServer
 Copyright (C) 2008 Free Software Foundation, Inc.
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

#include <include/conf/vhost/ip.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

class TestIpRange : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( TestIpRange );
  CPPUNIT_TEST( testRangeInclusion );
  CPPUNIT_TEST( testIpInRange );
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp() {}
  void tearDown() {}
  void testRangeInclusion()
  {
    Ipv4Range LargeRange("192.168.0.0/24"), InsideRange("192.168.0.100-192.168.0.200");
    CPPUNIT_ASSERT( LargeRange.InRange(&InsideRange) );
  }
  void testIpInRange()
  {
    Ipv4Range testRange("192.168.0.0/24");
    CPPUNIT_ASSERT( testRange.InRange("192.168.0.127") );
  }
};
