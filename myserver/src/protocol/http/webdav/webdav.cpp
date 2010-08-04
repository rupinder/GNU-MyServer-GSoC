/*
  Myserver
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
#include <include/http_handler/http_file/http_file.h>


#include <string>

using namespace std;


/*!
  Constructor for WebDAV class.
 */
WebDAV::WebDAV ()
{
  const char* properties[3] = {"creationtime", "lastmodifiedtime", "lastaccesstime"};
  numPropReq = 0;
  propReq.clear ();
  available.clear ();

  for (int i = 0; i < 3; i++)
    available.push_back (properties[i]);

  numPropAvail = available.size ();
}

/*!
  Retrieve the elements from the XML Tree in request and
  and push them into properties request vector (propReq).
  \param aNode Pointer to root node.
 */
void WebDAV::getElements (xmlNode* aNode)
{
  xmlNode *curNode = NULL;

  for (curNode = aNode; curNode; curNode = curNode->next)
  {
    if (curNode->type == XML_ELEMENT_NODE)
      propReq.push_back (reinterpret_cast <const char*> (curNode->name));

    getElements (curNode->children);
  }
}

/*!
  Retrieve the value of the property given the name
  of the property as a string.
  \param prop The name of the Property.
  \param path The path to the resource.
 */
char* WebDAV::getPropValue (const char* prop, const char* path)
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
  Generate the response tag for a single resource.
  \param path The path to the resource.
 */
xmlNodePtr WebDAV::generate (const char* path)
{
  xmlNodePtr response = xmlNewNode (NULL, BAD_CAST "D:response");
  xmlNewChild (response, NULL, BAD_CAST "D:href", BAD_CAST (path));
  xmlNodePtr propstat = xmlNewNode (NULL, BAD_CAST "D:propstat");

  xmlNodePtr prop = xmlNewNode (NULL, BAD_CAST "D:prop");
  xmlNewProp (prop, BAD_CAST "xmlns:R", BAD_CAST "http://www.myserverproject.net");

  if (!strcmp (propReq[1], "propname"))
    {
      for (int i = 0; i < numPropAvail; i++)
        xmlNewChild (prop, NULL, BAD_CAST (available[i]), NULL);

      xmlAddChild (propstat, prop);
    }
  else if (!strcmp (propReq[1], "allprop"))
    {
      for (int i = 0; i < numPropAvail; i++)
      {
        xmlNewChild (prop, NULL, BAD_CAST ((char*) available[i]), BAD_CAST (getPropValue (available[i], path)));
        xmlAddChild (propstat, prop);
      }
    }
  else
    {
      for (int i = 2; i < numPropReq; i++)
      {
        xmlNewChild (prop, NULL, BAD_CAST ((char*) propReq[i]), BAD_CAST (getPropValue (propReq[i], path)));
        xmlAddChild (propstat, prop);
      }
    }

  xmlNewChild(propstat, NULL, BAD_CAST "D:status", BAD_CAST "HTTP/1.1 200 OK");
  xmlAddChild (response, propstat);
  return response;
}

/*!
  Generate the complete response.
  \param path The path to the resource.
 */
xmlDocPtr WebDAV::generateResponse (const char* path)
{
  xmlDocPtr doc = xmlNewDoc ( BAD_CAST "1.0");
  xmlNodePtr rootNode = xmlNewNode (NULL, BAD_CAST "D:multistatus");

  xmlNewProp (rootNode, BAD_CAST "xmlns:D", BAD_CAST "DAV:");
  xmlDocSetRootElement (doc, rootNode);

  RecReadDirectory recTree;

  recTree.clearTree ();
  recTree.fileTreeGenerate (path);

  while (recTree.nextMember ())
    {
      if (recTree.getInfo () == 6)
        continue;

      xmlNodePtr response = generate (recTree.getPath ());
      xmlAddChild (rootNode, response);
    }

  return doc;
}

/*!
  Execute PROPFIND command.
  \param td Http Thread Context.
 */
int WebDAV::propfind (HttpThreadContext* td)
{
  /* Not allowed in a directory super to Vhost Document Root.  */
  if (FilesUtility::getPathRecursionLevel (td->request.uri.c_str ()) < 0)
    return td->http->raiseHTTPError (403);

  string loc = string (td->getVhostDir ()) + "/" + td->request.uri;

  try
    {
      /* Obtain the payload.  */
      XmlParser p;
      p.open (td->inputData.getFilename (), 0);

      /* Obtain xml entities in the payload.  */
      getElements (xmlDocGetRootElement (p.getDoc ()));

      /* Number of entities (properties requested).  */
      numPropReq = propReq.size ();

      xmlDocPtr doc = generateResponse (loc.c_str ());
      xmlSaveFormatFileEnc ("test.xml", doc, "UTF-8", 1);

      MemBuf resp;
      int len;
      bool keepalive, useChunks;
      u_long nbw, nbw2;

      p.open ("test.xml", 0);
      p.saveMemBuf (resp, &len);

      FiltersChain chain;
      if (td->mime)
        {
          FiltersFactory *ff = Server::getInstance ()->getFiltersFactory ();
          ff->chain (&chain, td->mime->filters, td->connection->socket, &nbw, 1);
        }

      HttpDataHandler::checkDataChunks (td, &keepalive, &useChunks);

      HttpHeaders::sendHeader (td->response, *chain.getStream (), *td->buffer, td);

      HttpDataHandler::appendDataToHTTPChannel (td,
                                          resp.getBuffer (),
                                          len, &(td->outputData),
                                          &chain, td->appendOutputs,
                                          useChunks);

      if (useChunks && chain.getStream ()->write ("0\r\n\r\n", 5, &nbw2))
        return HttpDataHandler::RET_FAILURE;

      /* 200 Success.  */
      td->http->raiseHTTPError (200);
      return HttpDataHandler::RET_OK;
    }
  catch (exception & e)
    {
      td->connection->host->warningsLogWrite ( _E ("WebDAV: Internal Error"), &e);
      return td->http->raiseHTTPError (500);
    }

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
  Execute COPY command.
  \param td Http Thread Context.
 */
int WebDAV::copy (HttpThreadContext* td)
{
  int ret, overwrite = 1;
  string dest = *td->request.getValue ("Destination", NULL);
  string* over = td->request.getValue ("Overwrite", NULL);
  string source = string (td->getVhostDir ()) + "/" + td->request.uri;

  /* Not allowed in a directory super to Vhost Document Root.  */
  if (FilesUtility::getPathRecursionLevel (td->request.uri.c_str ()) <= 0 ||
      FilesUtility::getPathRecursionLevel (dest.c_str ()) <= 0)
    return td->http->raiseHTTPError (403);

  string destination = string (td->getVhostDir ()) + "/" + dest;
  string file, directory, filenamepath;
  int permissions;

  /* Raise 423 if Locked.  */
  if (isLocked (td, destination))
    return td->http->raiseHTTPError (423);


  /* Determine Overwrite flag.  */
  if (over != NULL)
  {
    if (over->size() == 1 && (*over)[0] == 'F')
      overwrite = 0;
  }

  /* Check the permissions for destination.  */
  ret = td->http->getFilePermissions (destination, directory, file,
                                      filenamepath, false, &permissions);
  if (ret != 200)
    return ret;
  else if (! (permissions & MYSERVER_PERMISSION_WRITE))
    return td->http->sendAuth ();

  /* If a file is to be copied.  */
  if (!FilesUtility::isDirectory (source.c_str ()))
    {
        /* Conflict if Ancestor doesn't exist.  */
      if (!FilesUtility::nodeExists (destination.substr (0, destination.rfind ("/")).c_str ()))
        return td->http->raiseHTTPError (409);

      /* Check if already exists.  */
      if (!overwrite && FilesUtility::nodeExists (destination.c_str ()))
        return td->http->raiseHTTPError (412);

      FilesUtility::copyFile (source.c_str (), destination.c_str (), 1);
      return td->http->raiseHTTPError (201);
    }
  else
    {
      /* Remove trailing slash.  */
      if (destination[destination.size () - 1] == '/')
        destination.resize (destination.size () - 1);

      /* Conflict if Ancestor doesn't exist.  */
      if (!FilesUtility::nodeExists (destination.c_str ()))
        return td->http->raiseHTTPError (409);

      ret = FilesUtility::copyDir (source, destination, 1);
      if (ret != 0)
        return td->http->raiseHTTPError (ret);

      return td->http->raiseHTTPError (201);
    }
}

/*!
  Execute DELETE command.
  \param td Http Thread Context.
 */
int WebDAV::davdelete (HttpThreadContext* td)
{
  /* Not allowed in a directory super to Vhost Document Root.  */
  if (FilesUtility::getPathRecursionLevel (td->request.uri.c_str ()) <= 0)
    return td->http->raiseHTTPError (403);

  int ret, permissions;
  string directory, file, filenamepath;
  string location = string (td->getVhostDir ()) + "/" + td->request.uri;

  /* Raise 423 if Locked.  */
  if (isLocked (td, location))
    return td->http->raiseHTTPError (423);

  /* Check the permissions for location.  */
  ret = td->http->getFilePermissions (location, directory, file,
                                      filenamepath, false, &permissions);
  if (ret != 200)
    return ret;
  else if (! (permissions & MYSERVER_PERMISSION_WRITE))
    return td->http->sendAuth ();

  if (!FilesUtility::isDirectory (location.c_str ()))
    {
      FilesUtility::deleteFile (location.c_str ());
      return td->http->raiseHTTPError (202);
    }
  else
    {
      FilesUtility::deleteDir (location.c_str ());
      return td->http->raiseHTTPError (202);
    }
}

/*!
  Execute MOVE command.
  \param td Http Thread Context.
 */
int WebDAV::move (HttpThreadContext* td)
{
  int ret, overwrite;
  string dest = *td->request.getValue ("Destination", NULL);
  string* over = td->request.getValue ("Overwrite", NULL);
  string source = string (td->getVhostDir ()) + "/" + td->request.uri;
  string destination = string (td->getVhostDir ()) + "/" + dest;
  string file, directory, filenamepath;
  int permissions;

  /* Raise 423 if Locked.  */
  if (isLocked (td, destination))
    return td->http->raiseHTTPError (423);

  /* Determing Overwrite flag.  */
  if (over != NULL)
  {
    if (over->size() == 1 && (*over)[0] == 'F')
      overwrite = 0;
  }

  /* Check the permissions for destination.  */
  ret = td->http->getFilePermissions (destination, directory, file,
                                      filenamepath, false, &permissions);
  if (ret != 200)
    return ret;

  /* Check if allowed.  */
  else if (! (permissions & MYSERVER_PERMISSION_WRITE))
    return td->http->sendAuth ();

  /* Conflict if Ancestor doesn't exist.  */
  if (!FilesUtility::nodeExists (destination.substr (0, destination.rfind ("/")).c_str ()))
    return td->http->raiseHTTPError (409);

  /* Check if already exists.  */
  if (!overwrite && FilesUtility::nodeExists (destination.c_str ()))
    return td->http->raiseHTTPError (412);

  FilesUtility::renameFile (source.c_str (), destination.c_str ());
  return td->http->raiseHTTPError (201);
}

/*!
  Execute LOCK command.
  \param td Http Thread Context.
 */
int WebDAV::lock (HttpThreadContext* td)
{
  if (FilesUtility::getPathRecursionLevel (td->request.uri.c_str ()) <= 0)
    return td->http->raiseHTTPError (403);

  string loc = string (td->getVhostDir ()) + "/" + td->request.uri;

  char urn[48];
  sha1.init ();
  td->auxiliaryBuffer->setLength (0);
  *td->auxiliaryBuffer << loc;

  sha1.update (*td->auxiliaryBuffer);
  sha1.end (urn);

  string lockLoc = string (td->getVhostSys ()) + "/webdav/locks/" + string (urn);

  if (!FilesUtility::nodeExists (lockLoc.substr (0, lockLoc.rfind ("/")).c_str ()))
    {
      string temp = string (td->getVhostSys ()) + "/webdav";
      FilesUtility::mkdir (temp.c_str());
      temp += "/locks";
      FilesUtility::mkdir (temp.c_str());
    }

  File resLock;
  return resLock.openFile (lockLoc.c_str (), File::WRITE | File::FILE_OPEN_ALWAYS);
}

/*!
  Execute UNLOCK command.
  \param td Http Thread Context.
 */
int WebDAV::unlock (HttpThreadContext* td)
{
  string loc = string (td->getVhostDir ()) + "/" + td->request.uri;

  char urn[48];
  sha1.init ();
  td->auxiliaryBuffer->setLength (0);
  *td->auxiliaryBuffer << loc;

  sha1.update (*td->auxiliaryBuffer);
  sha1.end (urn);

  string lockLoc = string (td->getVhostSys ()) + "/webdav/locks/" + string (urn);

  return FilesUtility::deleteFile (lockLoc.c_str ());
}

/*!
  Check if a resource is locked.
  \param path Path to the resource.
 */
bool WebDAV::isLocked (HttpThreadContext* td, string path)
{
  char urn[48];
  sha1.init ();
  td->auxiliaryBuffer->setLength (0);
  *td->auxiliaryBuffer << string (path);

  sha1.update (*td->auxiliaryBuffer);
  sha1.end (urn);

  string lockLoc = string (td->getVhostSys ()) + "/webdav/locks/" + string (urn);

  return FilesUtility::nodeExists (lockLoc.c_str ());
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

  /* Raise 423 if Locked.  */
  if (isLocked (td, loc))
    return td->http->raiseHTTPError (423);

  int pos = loc.rfind ("/");

  /* Conflict if Ancestor doesn't exist.  */
  if (!FilesUtility::nodeExists (loc.substr (0, pos).c_str ()))
    return td->http->raiseHTTPError (409);
  /* Conflict if Collection exists already.  */
  else if (FilesUtility::nodeExists (loc.c_str ()))
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
