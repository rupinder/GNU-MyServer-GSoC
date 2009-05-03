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

#include <list>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <include/conf/xml_conf.h>


/*!
 *Read a forest of trees (structured objects) from XML.
 *
 *\param root The XML document root.
 *\param rootNodes All the stored top-level objects.
 *\param hashedData Access node by a name.
 *\return 0 on success.
 */
int XmlConf::build (xmlNodePtr root,
                    list<NodeTree<string>*> *rootNodes,
                    HashMap<string, NodeTree<string>*> *hashedData)
{
  readNode (root, NULL, rootNodes, hashedData);
  return 0;
}

void XmlConf::readNode (xmlNodePtr lcur,
                        NodeTree<string>* root,
                        list<NodeTree<string>*> *rootNodes,
                        HashMap<string, NodeTree<string>*> *hashedData)
{
  xmlAttr *attrs;
  for(; lcur; lcur = lcur->next)
    if (lcur->name && !xmlStrcmp (lcur->name, (const xmlChar *)"DEFINE"))
      {
        const char *name = NULL;
        const char *value = NULL;

        for (attrs = lcur->properties; attrs; attrs = attrs->next)
          {
            if (!xmlStrcmp (attrs->name, (const xmlChar *)"name") && 
                attrs->children && attrs->children->content)
              name = (const char*)attrs->children->content;
            
            if (!xmlStrcmp (attrs->name, (const xmlChar *)"value") && 
                attrs->children && attrs->children->content)
              value = (const char*)attrs->children->content;
          }

        string *v = value ? new string((const char*)value) : NULL;
        NodeTree<string> *node = new NodeTree<string> ();
        node->setValue (v);
 
        if(name)
          {
            string key((const char*)name);
            hashedData->put(key, node);
          }

        if (root)
          root->addChild (node);

        if (rootNodes)
          rootNodes->push_back (node);

        if (lcur->children)
          readNode (lcur->children,
                    node,
                    NULL,
                    hashedData);
      }
}
