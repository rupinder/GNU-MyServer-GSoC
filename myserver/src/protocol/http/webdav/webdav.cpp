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
#include <include/protocol/http/http.h>
#include <include/protocol/http/http_errors.h>
#include <include/http_handler/http_file/http_file.h>

#include <fts_.h>

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
const char *WebDAV::getPropValue (const char* prop, const char* path)
{
  time_t value;

  if (!strcmp (prop, "creationtime"))
    value = FilesUtility::getCreationTime (path);
  else if (!strcmp (prop, "getlastmodified"))
    value = FilesUtility::getLastModTime (path);
  else if (!strcmp (prop, "lastaccesstime"))
    value = FilesUtility::getLastAccTime (path);
  else if (!strcmp (prop, "getcontentlength"))
      return "280";
  else if (!strcmp (prop, "executable"))
      return "no";
  else if (!strcmp (prop, "checked-in"))
      return "";
  else if (!strcmp (prop, "checked-out"))
      return "";

  /* FIXME: this is not reentrant, use ctime_r instead.  */
  char *out = new char[32];
  return getRFC822GMTTime (value, out, NULL);
}

/*!
  Generate the response tag for a single resource.
  \param path The path to the resource.
 */
xmlNodePtr WebDAV::generate (const char *path)
{
  xmlNodePtr response = xmlNewNode (NULL, BAD_CAST "D:response");
  xmlNewProp (response, BAD_CAST "xmlns:g0", BAD_CAST "DAV:");

  xmlNewChild (response, NULL, BAD_CAST "D:href", BAD_CAST (path));
  xmlNodePtr propstat = xmlNewNode (NULL, BAD_CAST "D:propstat");

  xmlNodePtr prop = xmlNewNode (NULL, BAD_CAST "D:prop");

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
          if (strcmp (available[i], "resourcetype"))
            {
              char buffer[32];
              sprintf (buffer, "g0:%s", available[i]);
              xmlNewChild (prop, NULL, BAD_CAST (buffer),
                           BAD_CAST (getPropValue (available[i], path)));
            }
          else
            {
              xmlNodePtr res = xmlNewNode (NULL, BAD_CAST "D:collection");
              xmlAddChild (prop, res);
            }
        }

      xmlAddChild (propstat, prop);
    }
  else
    {
      for (int i = 2; i < numPropReq; i++)
        {
          if (strcmp (propReq[i], "resourcetype"))
            {
              char buffer[32];
              sprintf (buffer, "g0:%s", propReq[i]);

              xmlNewChild (prop, NULL, BAD_CAST (buffer),
                           BAD_CAST (getPropValue (propReq[i], path)));
            }
          else
            {
              xmlNodePtr content = xmlNewNode (NULL, BAD_CAST "D:collection");
              xmlNodePtr res = xmlNewNode (NULL, BAD_CAST "g0:resourcetype");
              xmlAddChild (res, content);
              xmlAddChild (prop, res);
            }
        }

      xmlAddChild (propstat, prop);
    }

  xmlNewChild (propstat, NULL, BAD_CAST "D:status", BAD_CAST "HTTP/1.1 200 OK");
  xmlAddChild (response, propstat);
  return response;
}

/*!
  Generate the complete response.
  \param path The path to the resource.
 */
xmlDocPtr WebDAV::generateResponse (const char* path, unsigned int reqDepth)
{
  xmlDocPtr doc = xmlNewDoc ( BAD_CAST "1.0");
  xmlNodePtr rootNode = xmlNewNode (NULL, BAD_CAST "D:multistatus");

  xmlNewProp (rootNode, BAD_CAST "xmlns:D", BAD_CAST "DAV:");
  xmlNewProp (rootNode, BAD_CAST "xmlns:ns0", BAD_CAST "DAV:");
  xmlNewProp (rootNode, BAD_CAST "xmlns:ns1", BAD_CAST "http://apache.org/dav/props/");

  xmlDocSetRootElement (doc, rootNode);

  bool f = 0;
  RecReadDirectory recTree;
  size_t relOffset = strlen (path) - 1;
  recTree.clearTree ();
  recTree.fileTreeGenerate (path);
  while (recTree.nextMember ())
    {
      if (recTree.getInfo () == FTS_DP)
        continue;

      if (recTree.getLevel () > reqDepth)
        recTree.skip ();
      else
        {
          const char *relPath = recTree.getPath () + relOffset;
          xmlNodePtr response = generate (relPath);
          xmlAddChild (rootNode, response);
        }
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
      bool keepalive, useChunks;
      u_long nbw, nbw2;
      FiltersChain chain;
      list<string> filters;
      FiltersFactory *ff = Server::getInstance ()->getFiltersFactory ();

      /* Obtain the payload.  */
      XmlParser p;
      p.open (td->inputData.getFilename (), 0);

      /* Obtain xml entities in the payload.  */
      getElements (xmlDocGetRootElement (p.getDoc ()));

      /* Number of entities (properties requested).  */
      numPropReq = propReq.size ();

      ff->chain (&chain, filters, td->connection->socket, &nbw, 1);


      HttpHeaders::buildDefaultHTTPResponseHeader (&(td->response));
      HttpDataHandler::checkDataChunks (td, &keepalive, &useChunks);
      td->response.httpStatus = 207;
      if (keepalive)
        td->response.setValue ("connection", "keep-alive");
      else
        td->response.setValue ("connection", "close");
      HttpHeaders::sendHeader (td->response, *chain.getStream (), *td->buffer, td);

      /* Determine the Depth.  */
      MemBuf tmp;
      string* depth = td->request.getValue ("Depth", NULL);
      unsigned int reqDepth = UINT_MAX;
      if (depth->size () > 0)
        reqDepth = tmp.strToUint ((*depth).c_str ());

      xmlDocPtr doc = generateResponse (loc.c_str (), reqDepth);
      xmlSaveFormatFileEnc ("test.xml", doc, "UTF-8", 1);

      File f;
      f.openFile ("test.xml", File::READ);
      for (;;)
        {
          u_long nbr;
          f.read (td->buffer->getBuffer (), td->buffer->getRealLength (), &nbr);
          if (nbr == 0)
            break;

          HttpDataHandler::appendDataToHTTPChannel (td, td->buffer->getBuffer (),
                                                    nbr, &(td->outputData),
                                                    &chain, td->appendOutputs,
                                                    useChunks);
          if (nbr != td->buffer->getRealLength ())
            break;
        }

      if (useChunks && chain.getStream ()->write ("0\r\n\r\n", 5, &nbw2))
        return HttpDataHandler::RET_FAILURE;

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
  string destination = *td->request.getValue ("Destination", NULL);
  string* over = td->request.getValue ("Overwrite", NULL);
  string source = string (td->getVhostDir ()) + td->request.uri;

  string file, directory, filenamepath;
  int permissions;

  string temp = "http://" + *td->request.getValue ("Host", NULL) + "/";
  u_long endPos = temp.size ();

  string target = string (td->getVhostDir ()) + "/" + destination.substr (endPos);


  /* Raise 423 if Locked.  */
  if (isLocked (td, target))
    return td->http->raiseHTTPError (423);


  /* Determine Overwrite flag.  */
  if (over != NULL)
  {
    if (over->size () == 1 && (*over)[0] == 'F')
      overwrite = 0;
  }

  /* Check the permissions for destination.  */
  ret = td->http->getFilePermissions (target, directory, file,
                                      filenamepath, false, &permissions);
  if (ret != 200)
    return ret;
  else if (! (permissions & MYSERVER_PERMISSION_WRITE))
    return td->http->sendAuth ();

  /* If a file is to be copied.  */
  if (!FilesUtility::isDirectory (source.c_str ()))
    {
      int pos = target.rfind ("/");

      /* Conflict if Ancestor doesn't exist.  */
      if (!FilesUtility::nodeExists (target.substr (0, pos).c_str ()))
        return td->http->raiseHTTPError (409);

      /* Check if already exists.  */
      if (!overwrite && FilesUtility::nodeExists (target.c_str ()))
        return td->http->raiseHTTPError (412);

      FilesUtility::copyFile (source.c_str (), target.c_str (), 1);
      return td->http->raiseHTTPError (201);
    }
  else
    {
      /* Remove trailing slash.  */
      if (target[target.size () - 1] == '/')
        target.resize (target.size () - 1);

      /* Conflict if Ancestor doesn't exist.  */
      if (!FilesUtility::nodeExists (target.c_str ()))
        return td->http->raiseHTTPError (409);

      ret = FilesUtility::copyDir (source, target, 1);
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
  string destination = *td->request.getValue ("Destination", NULL);
  string* over = td->request.getValue ("Overwrite", NULL);
  string source = string (td->getVhostDir ()) + "/" + td->request.uri;

  string file, directory, filenamepath;
  int permissions;

  string temp = "http://" + *td->request.getValue ("Host", NULL) + "/";
  u_long endPos = temp.size ();

  string target = string (td->getVhostDir ()) + "/" + destination.substr (endPos);

  /* Raise 423 if Locked.  */
  if (isLocked (td, target))
    return td->http->raiseHTTPError (423);

  /* Determing Overwrite flag.  */
  if (over != NULL)
  {
    if (over->size() == 1 && (*over)[0] == 'F')
      overwrite = 0;
  }

  /* Check the permissions for target.  */
  ret = td->http->getFilePermissions (target, directory, file,
                                      filenamepath, false, &permissions);
  if (ret != 200)
    return ret;

  /* Check if allowed.  */
  else if (! (permissions & MYSERVER_PERMISSION_WRITE))
    return td->http->sendAuth ();

  /* Conflict if Ancestor doesn't exist.  */
  if (!FilesUtility::nodeExists (target.substr (0, target.rfind ("/")).c_str ()))
    return td->http->raiseHTTPError (409);

  /* Check if already exists.  */
  if (!overwrite && FilesUtility::nodeExists (target.c_str ()))
    return td->http->raiseHTTPError (412);

  FilesUtility::renameFile (source.c_str (), target.c_str ());
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
