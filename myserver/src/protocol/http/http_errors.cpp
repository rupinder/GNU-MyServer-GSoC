/*
MyServer
Copyright (C) 2002, 2003, 2004, 2007, 2008, 2009 Free Software Foundation, Inc.
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


#include "stdafx.h"
#include <include/protocol/http/http_errors.h>

#include <sstream>


HashMap<int, const char*> HttpErrors::messagesMap;

bool HttpErrors::loaded = false;


/*!
 * Get an error page from its error code.
 * \param statusCode The HTTP error.
 * \param out Output string where write the error page name.
 */
void HttpErrors::getErrorPage (int statusCode, string& out)
{
  ostringstream os;
  os << "/errors/" << statusCode << ".html";
  out.assign (os.str ());
}

/*!
 *Get an error message from its error code.
 *\param statusCode The HTTP error.
 *\param out Output string where write the error message.
 */
void HttpErrors::getErrorMessage (int statusCode, string& out)
{
  const char* msg = messagesMap.get (statusCode);

  if (msg)
    out.assign (msg);
  else
    out.assign ("");
}

/*!
 * Unload the allocated resources.
 */
void HttpErrors::unLoad ()
{
  messagesMap.clear ();
}

/*!
 * Put a new message in the map.
 * \param id The HTTP error code.
 * \param msg The message associated to the error code.
 */
void HttpErrors::putMessage (int id, const char* msg)
{
  messagesMap.put (id, msg);
}

/*!
 * Load the HTTP errors.
 */
void HttpErrors::load ()
{
  if (loaded)
    return;

  loaded = true;

  /* INFORMATIONAL.  */
  putMessage (100, "Continue");
  putMessage (101, "Switching Protocols");
  putMessage (102, "Processing");

  /* SUCCESS.  */
  putMessage (200, "OK");
  putMessage (201, "Created");
  putMessage (202, "Accepted");
  putMessage (203, "Non-Authoritative Information");
  putMessage (204, "No Content");
  putMessage (205, "Reset Content");
  putMessage (206, "Partial Content");
  putMessage (207, "Multi-Status");

  /* REDIRECTION.  */
  putMessage (300, "Multiple Choices");
  putMessage (301, "Moved Permanently");
  putMessage (302, "Found");
  putMessage (303, "See Other");
  putMessage (304, "Not Modified");
  putMessage (305, "Use proxy");
  putMessage (306, "Switch proxy");
  putMessage (307, "Temporary redirect");

  /* CLIENT ERROR.  */
  putMessage (400, "Bad Request");
  putMessage (401, "Unauthorized");
  putMessage (402, "Payment required");
  putMessage (403, "Forbidden");
  putMessage (404, "Not Found");
  putMessage (405, "Method Not Allowed");
  putMessage (406, "Not Acceptable");
  putMessage (407, "Proxy Authentication Required");
  putMessage (408, "Request timeout");
  putMessage (409, "Conflict");
  putMessage (411, "Length Required");
  putMessage (412, "Precondition Failed");
  putMessage (413, "Request Entity Too Large");
  putMessage (414, "Request URI Too Long");
  putMessage (415, "Unsupported Media Type");
  putMessage (416, "Requested Range Not Satisfiable");
  putMessage (417, "Expectation Failed");
  putMessage (422, "Unprocessable Entity");
  putMessage (423, "Locked");
  putMessage (424, "Failed Dependency");
  putMessage (425, "Unordered Collection");
  putMessage (426, "Upgrade Required");
  putMessage (412, "Precondition Failed");

  /* SERVER ERROR.  */
  putMessage (500, "Internal Server Error");
  putMessage (501, "Not Implemented");
  putMessage (502, "Bad Gateway");
  putMessage (503, "Service Unavailable");
  putMessage (504, "Gateway Timeout");
  putMessage (505, "HTTP Version Not Supported");
  putMessage (506, "Variant Also Negotiates");
  putMessage (507, "Insufficient Storage");
  putMessage (509, "Bandwidth Limit Exceeded");
  putMessage (510, "Not Extended");
}
