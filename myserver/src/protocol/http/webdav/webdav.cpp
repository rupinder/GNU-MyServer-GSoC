/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010
  Free Software Foundation, Inc.
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
#include <fts.h>
#include <include/protocol/http/http.h>


#include <string>

using namespace std;


/*!
 *Retrieve the elements from the XML Tree in request and
 *and push them into properties request vector (prop_req).
 *\param a_node Pointer to root node.
 */
void WebDAV::get_elements (xmlNode* a_node)
{
  xmlNode *cur_node = NULL;
  
  for (cur_node = a_node; cur_node; cur_node = cur_node->next)
  {
    if (cur_node->type == XML_ELEMENT_NODE)
      prop_req.push_back (reinterpret_cast <const char*> (cur_node->name));
      
    get_elements (cur_node->children);
  }
}

/*!
 *Retrieve the value of the property given the name 
 *of the property as a string.
 *\param prop The name of the Property.
 *\param path The path to the resource.
 */
char* WebDAV::get_prop_value (const char* prop, const char* path)
{
  time_t value;
  
  if (!strcmp (prop, "creationtime"))
    value = FilesUtility::getCreationTime (path);
  else if (!strcmp (prop, "lastmodifiedtime"))
    value = FilesUtility::getLastModTime (path);
  else if (!strcmp (prop, "lastaccesstime"))
    value = FilesUtility::getLastAccTime (path);
    
  return ctime (&value);
}

/*!
 *Generate the response tag for a single resource.
 *\param path The path to the resource.
 */
xmlNodePtr WebDAV::generate (const char* path)
{
  xmlNodePtr response = xmlNewNode (NULL, BAD_CAST "D:response");
  xmlNewChild (response, NULL, BAD_CAST "D:href", BAD_CAST (path));
  xmlNodePtr propstat = xmlNewNode (NULL, BAD_CAST "D:propstat");
  
  xmlNodePtr prop = xmlNewNode (NULL, BAD_CAST "D:prop");
  xmlNewProp (prop, BAD_CAST "xmlns:R", BAD_CAST "http://www.myserverproject.net");
  
  for (int i = 2; i < num_prop_req; i++)
  {
    xmlNewChild (prop, NULL, BAD_CAST ((char*) prop_req[i]), BAD_CAST (get_prop_value (prop_req[i], path)));
    xmlAddChild (propstat, prop);
  }
  
  xmlAddChild (response, propstat);
  return response;
}

/*!
 *Generate the complete response.
 *\param path The path to the resource.
 */
xmlDocPtr WebDAV::generate_response (const char* path)
{
  xmlDocPtr doc = xmlNewDoc ( BAD_CAST "1.0");
  xmlNodePtr root_node = xmlNewNode (NULL, BAD_CAST "D:multistatus");

  xmlNewProp (root_node, BAD_CAST "xmlns:D", BAD_CAST "DAV:");
  xmlDocSetRootElement (doc, root_node);
  
  FTSENT *e;
  char *argv[2] = {(char *) path, NULL};
    
  FTS* mytrav = fts_open (argv, FTS_LOGICAL, NULL);

  while (e = fts_read (mytrav))
    {
      if (e->fts_level == 6)
        continue;
      
      xmlNodePtr response = generate (e->fts_path);
      xmlAddChild (root_node, response);
    }
    
  return doc;
}

/*!
 *Execute PROPFIND command.
 *\param td Http Thread Context.
 */
void WebDAV::propfind (HttpThreadContext* td)
{
  prop_req.clear ();
  
  XmlParser p;
  p.open (td->inputData.getFilename (), 0);
  
  get_elements (xmlDocGetRootElement (p.getDoc ()));
  
  num_prop_req = prop_req.size ();
 
  xmlDocPtr doc = generate_response (td->request.uri.c_str ());
  xmlSaveFormatFileEnc ("test.xml", doc, "UTF-8", 1);
/*  
  MemBuf resp;
  int* LEN;
  *LEN = 4096;
  
  p.open ("test.xml", 0);
  p.saveMemBuf (resp, LEN);
  
  *td->auxiliaryBuffer << resp << "\r\n";
      
  td->connection->socket->send (td->auxiliaryBuffer->getBuffer (),
                                td->auxiliaryBuffer->getLength (), 0);
*/
}

/*!
 *Execute MKCOL command.
 *\param td Http Thread Context.
 */
int WebDAV::mkcol (HttpThreadContext* td)
{
  /* Not allowed in a directory super to Vhost Document Root.  */
  if (FilesUtility::getPathRecursionLevel (td->request.uri.c_str ()) <= 0)
    return td->http->raiseHTTPError (403);

  string loc = string (td->getVhostDir ()) + "/" + td->request.uri;

  /* Conflict if Collection exists already.  */
  if (FilesUtility::nodeExists (loc.c_str ()))
    return td->http->raiseHTTPError (409);
  else
    {
      try
        {
          FilesUtility::mkdir (loc);
          /* 201 Created.  */
          td->http->raiseHTTPError (201);
          return HttpDataHandler::RET_OK;
        }
      catch (exception & e)
        {
          td->connection->host->warningsLogWrite ( _E ("WebDAV: Internal Error"), &e);
          return td->http->raiseHTTPError (500);
        }
    }
}
