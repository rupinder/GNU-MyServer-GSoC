/*
  MyServer
  Copyright (C) 2009, 2010 Free Software Foundation, Inc.
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
#include <ctype.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

#include <include/conf/nodetree.h>

class TestNodeTree : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestNodeTree );
  CPPUNIT_TEST ( testCreateDestroy );
  CPPUNIT_TEST ( testAttributes );
  CPPUNIT_TEST_SUITE_END ();


public:

  NodeTree<string> *node;

  void setUp ()
  {
    node = new NodeTree<string> ();
  }

  void tearDown ()
  {
    delete node;
  }

  void testAttributes ()
  {
    string name ("my_attrib");
    string value ("foo.bar");
    string *value2;

    value2 = node->getAttr (name);
    CPPUNIT_ASSERT_EQUAL (value2, (string*)NULL);

    node->addAttr (name, value);

    value2 = node->getAttr (name);
    CPPUNIT_ASSERT (value2);

    CPPUNIT_ASSERT_EQUAL (value2->compare (value), 0);
  }

  void testCreateDestroy ()
  {
    for (int j = 0; j < 5; j++)
      {
        NodeTree<string> *child = new NodeTree<string> ();
        node->addChild (child);
        for (int i = 0; i < 10; i++)
          {
            string s = "just a string";
            child->addChild (new NodeTree<string> (s));
          }
      }

    CPPUNIT_ASSERT_EQUAL (countLeafNodes (node), 50);
  }

protected:
  int countLeafNodes (NodeTree<string> *c)
  {
    int count = 0;
    for (list<NodeTree<string>*>::iterator it = c->getChildren ()->begin ();
         it != c->getChildren ()->end ();
         it++)
      {
        NodeTree<string> *n = *it;
        if (n->isLeaf ())
          count++;
        else
          count += countLeafNodes (n);
      }

    return count;
  }

};


CPPUNIT_TEST_SUITE_REGISTRATION ( TestNodeTree );
