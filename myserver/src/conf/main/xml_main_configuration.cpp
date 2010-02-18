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

#include <include/conf/main/xml_main_configuration.h>
#include <include/conf/xml_conf.h>

XmlMainConfiguration::XmlMainConfiguration ()
{

}

XmlMainConfiguration::~XmlMainConfiguration ()
{

}

void XmlMainConfiguration::readData (list<NodeTree<string>*> *hashedDataTrees,
                                 HashMap<string, NodeTree<string>*> *hashedData)
{
  xmlNodePtr root = xmlDocGetRootElement (xmlParser.getDoc ())->xmlChildrenNode;
  XmlConf::build (root, hashedDataTrees, hashedData);
}

/*!
 * Return the last value defined as <DEFINE name="FIELD" value="VALUE" /> in the
 * xml file.
 */
const char *XmlMainConfiguration::getValue (const char* field)
{
  const char *ret = NULL;
  xmlNodePtr lcur = xmlDocGetRootElement (xmlParser.getDoc ())->xmlChildrenNode;
  xmlAttr *attrs;
  for (; lcur; lcur = lcur->next)
    if (lcur->name && !xmlStrcmp (lcur->name, (const xmlChar *) "DEFINE"))
      {
        NodeTree<string> *node = new NodeTree<string> ();
        const char *name = NULL;
        const char *value = NULL;

        for (attrs = lcur->properties; attrs; attrs = attrs->next)
          {
            if (!xmlStrcmp (attrs->name, (const xmlChar *) "name") &&
                attrs->children && attrs->children->content)
              name = (const char*)attrs->children->content;
            else if (!xmlStrcmp (attrs->name, (const xmlChar *) "value") &&
                attrs->children && attrs->children->content)
              value = (const char*)attrs->children->content;
          }
        if (! strcmp (name, field))
          ret = value;
      }

  return ret;
}
