/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2009 Free Software Foundation, Inc.
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

#ifndef XML_CONF_H
# define XML_CONF_H

# include "stdafx.h"

# include <include/conf/nodetree.h>
# include <include/base/xml/xml_parser.h>
# include <include/base/hash_map/hash_map.h>

# include <list>

using namespace std;

/*!
 *Utility class for various operations on XML files.
 */
class XmlConf
{
 public:
  static int build (xmlNodePtr root,
                    list<NodeTree<string>*> *rootNodes,
                    HashMap<string, NodeTree<string>*> *hashedData);

 protected:
  static void readNode (xmlNodePtr lcur,
                        NodeTree<string> *root,
                        list<NodeTree<string>*> *rootNodes,
                        HashMap<string, NodeTree<string>*> *hashedData);
};

#endif
