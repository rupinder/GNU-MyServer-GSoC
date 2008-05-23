/*
 MyServer
 Copyright (C) 2008 The MyServer Team
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

#include <ctype.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include "../include/multicast.h"

#include <iostream>
using namespace std;

class TestMulticastRegistry : public MulticastRegistry< int, int, int >
{
public:
  void notifyAll()
  {
    int val = 10;
    notifyMulticast (val, val);
  }

  int getHandlersSize(int msg)
  {
    vector<Multicast<int, int, int>*>* handlers = getHandlers(msg);

    if(handlers)
      return handlers->size();
    
    return 0;
  }

};

class TestMulticastObserver : public Multicast<int, int, int>
{
  bool right;
public:

  TestMulticastObserver() : right(false){}

  virtual int updateMulticast(MulticastRegistry<int, int, int>* reg, int& msg, int arg)
  {
    if(msg == 10 && arg == 10)
      right = true;

    return 0;
  }


  bool isRight()
  {
    return right;
  }

};

class TestMulticast : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( TestMulticast );
  CPPUNIT_TEST( testMulticast );
  CPPUNIT_TEST( testAddRemove );
  CPPUNIT_TEST_SUITE_END();

  TestMulticastRegistry* registry;
  TestMulticastObserver* observer;
public:
  void setUp()
  {
    registry = new TestMulticastRegistry();
    observer = new TestMulticastObserver();
  }

  void tearDown()
  {
    delete observer;
    delete registry;
  }

  void testMulticast()
  {
    int msg = 10;
    CPPUNIT_ASSERT_EQUAL(observer->isRight(), false);

    registry->addMulticast(msg, observer);
    registry->notifyAll();

    CPPUNIT_ASSERT_EQUAL(observer->isRight(), true);
  }


  void testAddRemove()
  {
    int msg = 10;

    CPPUNIT_ASSERT_EQUAL(registry->getHandlersSize(msg), 0);

    registry->addMulticast(msg, observer);


    CPPUNIT_ASSERT_EQUAL(registry->getHandlersSize(msg), 1);

    registry->removeMulticast(msg, observer);

    CPPUNIT_ASSERT_EQUAL(registry->getHandlersSize(msg), 0);

  }


};


CPPUNIT_TEST_SUITE_REGISTRATION( TestMulticast );
