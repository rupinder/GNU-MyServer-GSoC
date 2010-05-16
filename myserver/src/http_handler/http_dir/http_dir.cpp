/*
MyServer
Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010 Free Software
Foundation, Inc.
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
#include <include/server/server.h>
#include <include/protocol/http/http.h>
#include <include/protocol/http/http_headers.h>
#include <include/http_handler/http_dir/http_dir.h>
#include <include/filter/filters_chain.h>
#include <include/base/file/files_utility.h>

#include <errno.h>

#ifdef WIN32
# include <direct.h>
#else
# include <string.h>
#endif


#include <include/base/read_directory/read_directory.h>
#include <include/base/string/stringutils.h>

#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iomanip>

using namespace std;

/*!
  Compare two HttpDir::FileStruct by their filename.
 */
bool HttpDir::compareFileStructByName (HttpDir::FileStruct i,
                                       HttpDir::FileStruct j)
{
  return stringcmpi (i.name, j.name) < 0 ? true : false;
}

/*!
  Compare two HttpDir::FileStruct by their last modified file time.
 */

bool HttpDir::compareFileStructByTime (HttpDir::FileStruct i,
                                       HttpDir::FileStruct j)
{
  return i.time_write < j.time_write;
}

/*!
  Compare two HttpDir::FileStruct by their file size.
 */
bool HttpDir::compareFileStructBySize (HttpDir::FileStruct i,
                                       HttpDir::FileStruct j)
{
  return i.size < j.size;
}

/*!
  Constructor for the class.
 */
HttpDir::HttpDir ()
{

}

/*!
  Destroy the object.
 */
HttpDir::~HttpDir ()
{

}

/*!
  Load the static elements.
 */
int HttpDir::load ()
{
  return 0;
}

/*!
  Unload the static elements.
 */
int HttpDir::unLoad ()
{
  return 0;
}

/*!
  Get a formatted dobule representation of BYTES right shifted by POWER.
  \param bytes Number of bytes.
  \param power Power of 2.
  \return the double representation for BYTES>>POWER.
 */
double HttpDir::formatBytes (u_long bytes, u_int power)
{
  /* u_long can be 32 bits.  Don't do a right shift that is bigger than
    its size.  */
  const u_long quotient = (bytes >> (power / 2)) >> (power / 2);

  if (quotient == 0)
    return -1;

  const u_long unit = 1 << power;
  const u_long remainder = bytes & (unit - 1);
  const double result = quotient + static_cast<double>(remainder) / unit;
  return result;
}


/*!
  Fullfill the string out with a formatted representation for bytes.
  \param bytes Size to format.
  \param out Out string.
 */
void HttpDir::getFormattedSize (u_long bytes, string & out)
{
  const string symbols[] = {"TB", "GB", "MB", "KB", "bytes"};
  const u_int powers[] = {40, 30, 20, 10, 0};
  double result;
  size_t i;
  ostringstream osstr;

  if (bytes == 0)
    {
      out = "0 bytes";
      return;
    }

  for (i = 0; i < sizeof (powers); i++)
    {
      result = formatBytes (bytes, powers[i]);
      if (result != -1)
        break;
    }

  if ((result - floor (result)) < 0.01)
    osstr << std::fixed << setprecision (0) << result << " " << symbols[i];
  else
    osstr << std::fixed << setprecision (2) << result << " " << symbols[i];

  out = osstr.str ();
}

/*!
  Generate the HTML header for the results table.
  \param out Buffer where write it.
  \param sortType How elements are sorted.
  \param sortReverse True if element are in reverse order.
  \param formatString Decide which fields show.
 */
void HttpDir::generateHeader (MemBuf &out, char sortType, bool sortReverse,
                              const char* formatString)
{
  const char* cur = formatString;

  out << "<tr>\r\n";

  for (;;)
    {
      while (*cur && ((*cur == '%' ) || (*cur == ' ' )))
        cur++;

      if (!(*cur))
        break;

      switch (*cur)
        {
        case 'f':
          if (sortType == 'f' && !sortReverse)
            out << "<th><a href=\"?sort=fI\">File</a></th>\r\n";
          else
            out << "<th><a href=\"?sort=f\">File</a></th>\r\n";
          break;

        case 't':
          if (sortType == 't' && !sortReverse)
            out << "<th><a href=\"?sort=tI\">Last Modified</a></th>\r\n";
          else
            out << "<th><a href=\"?sort=t\">Last Modified</a></th>\r\n";
          break;

        case 's':
          if (sortType == 's' && !sortReverse)
            out << "<th><a href=\"?sort=sI\">Size</a></th>\r\n";
          else
            out << "<th><a href=\"?sort=s\">Size</a></th>\r\n";
          break;
        }
      cur++;
    }

  out << "</tr>\r\n";
}

/*!
  Generate the HTML code for an element in the result table.
  \param out Buffer where write the HTML.
  \param file Structure with information on the element.
  \param linkPrefix Prefix to use for the generated links.
  \param formatString Specify which element show.
 */
void HttpDir::generateElement (MemBuf &out,
                               FileStruct &file,
                               string &linkPrefix,
                               const char *formatString)
{
  char fileTime[32];
  string name;

  formatHtml (file.name, name);

  out << "<tr>\r\n";

  const char* cur = formatString;

  for (;;)
  {
    while (*cur && ((*cur == '%' ) || (*cur == ' ' )))
      cur++;

    if (!(*cur))
      break;

    switch (*cur)
    {
    case 'f':
      out << "<td><a href=\"";
      out << linkPrefix << name;
      out << "\">" ;
      out << name;
      out << "</a></td>\r\n";
      break;

    case 't':
      getRFC822GMTTime (file.time_write, fileTime, 32);
      out << "<td>";
      out << fileTime ;
      out << "</td>\r\n";
      break;

    case 's':
      out << "<td>";
      if (file.attrib & FILE_ATTRIBUTE_DIRECTORY)
        out << "[directory]";
      else
        {
          string tmp;
          getFormattedSize (file.size, tmp);
          out << tmp;
        }
      out << "</td>";
      break;
    }

    cur++;
  }
  out << "</tr>\r\n";
}


/*!
  Browse a directory printing its contents in an HTML file.
  \param td The current thread context.
  \param directory Directory to show.
  \param cgi not used.
  \param execute not used.
  \param onlyHeader Specify if send only the HTTP header.
*/
int HttpDir::send (HttpThreadContext* td,
                   const char* directory, const char* cgi,
                   bool execute, bool onlyHeader)
{
  u_long nbw;
  string filename;
  int ret;
  ReadDirectory fd;
  FiltersChain chain;
  int lastSlash = 0;
  bool useChunks = false;
  u_long sentData = 0;
  char* bufferloop;
  const char* cssFile;
  bool keepalive = false;
  vector<HttpDir::FileStruct> files;
  size_t sortIndex;
  char sortType = 0;
  bool sortReverse = false;
  string linkPrefix;
  Regex ignRegex;
  const char *ignPattern;
  const char *formatString;
  HttpRequestHeader::Entry *host;

  try
    {
      host = td->request.other.get ("host");
      formatString = td->securityToken.getData ("http.dir.format",
                                                MYSERVER_SECURITY_CONF
                                                | MYSERVER_VHOST_CONF
                                                | MYSERVER_SERVER_CONF,
                                                "%f%t%s");


      host = td->request.other.get ("host");
      chain.setStream (td->connection->socket);
      if (! (td->permissions & MYSERVER_PERMISSION_BROWSE))
        return td->http->sendAuth ();

      if (td->mime)
        Server::getInstance ()->getFiltersFactory ()->chain (&chain,
                                                             td->mime->filters,
                                                             td->connection->socket,
                                                             &nbw, 1);

      lastSlash = td->request.uri.rfind ('/') + 1;

      checkDataChunks (td, &keepalive, &useChunks);

      td->response.setValue ("content-type", "text/html");

      ignPattern = td->securityToken.getData ("http.dir.ignore",
                                              MYSERVER_SECURITY_CONF
                                              | MYSERVER_VHOST_CONF
                                              | MYSERVER_SERVER_CONF, NULL);
      if (ignPattern)
        {
          int ret = ignRegex.compile (ignPattern, REG_EXTENDED);
          if (ret)
            {
              td->connection->host->warningsLogWrite (_("HttpDir: pattern `%s' "
                                                        "is not valid"), ignPattern);

              fd.findclose ();
              td->outputData.close ();
              chain.clearAllFilters ();
              return td->http->raiseHTTPError (500);
            }
        }

      HttpHeaders::sendHeader (td->response, *td->connection->socket,
                               *td->buffer, td);

      if (onlyHeader)
        return HttpDataHandler::RET_OK;

      sortIndex = td->request.uriOpts.find ("sort=");

      if (sortIndex != string::npos && sortIndex + 5 < td->request.uriOpts.length ())
        sortType = td->request.uriOpts.at (sortIndex + 5);

      if (sortIndex != string::npos && sortIndex + 6 < td->request.uriOpts.length ())
        sortReverse = td->request.uriOpts.at (sortIndex + 6) == 'I';

      /* Make sortType always lowercase.  */
      switch (sortType)
        {
        case 's':
        case 'S':
          sortType = 's';
          break;

        case 't':
        case 'T':
          sortType = 't';
          break;
        }

      cssFile = td->securityToken.getData ("http.dir.css", MYSERVER_SECURITY_CONF
                                           | MYSERVER_VHOST_CONF
                                           | MYSERVER_SERVER_CONF, NULL);

      td->auxiliaryBuffer->setLength (0);
      *td->auxiliaryBuffer << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
        "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\"\r\n"
        "\"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\r\n"
        "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\">"
        "\r\n<head>\r\n<title>" ;
      *td->auxiliaryBuffer << td->request.uri.c_str () ;
      *td->auxiliaryBuffer << "</title>\r\n";

      /*
        If it is defined a CSS file for the graphic layout of
        the browse directory insert it in the page.
       */
      if (cssFile)
        {
          *td->auxiliaryBuffer << "<link rel=\"stylesheet\" href=\""
                               << cssFile
                               << "\" type=\"text/css\" media=\"all\"/>\r\n";
        }

      *td->auxiliaryBuffer << "</head>\r\n";

      appendDataToHTTPChannel (td, td->auxiliaryBuffer->getBuffer (),
                               td->auxiliaryBuffer->getLength (),
                               &(td->outputData), &chain,
                               td->appendOutputs, useChunks);

      sentData = td->auxiliaryBuffer->getLength ();

      filename = directory;
      td->auxiliaryBuffer->setLength (0);
      *td->auxiliaryBuffer << "<body>\r\n<h1>Contents of directory ";
      *td->auxiliaryBuffer <<  &td->request.uri[lastSlash] ;
      *td->auxiliaryBuffer << "</h1>\r\n<hr />\r\n";

      appendDataToHTTPChannel (td, td->auxiliaryBuffer->getBuffer (),
                               td->auxiliaryBuffer->getLength (),
                               &(td->outputData), &chain,
                               td->appendOutputs, useChunks);

      sentData += td->auxiliaryBuffer->getLength ();

      fd.findfirst (filename.c_str ());

      /*
        With the current code we build the HTML TABLE to indicize the
        files in the directory.
       */
      td->auxiliaryBuffer->setLength (0);
      *td->auxiliaryBuffer << "<table width=\"100%\">\r\n" ;

      generateHeader (*td->auxiliaryBuffer, sortType, sortReverse, formatString);

      appendDataToHTTPChannel (td, td->auxiliaryBuffer->getBuffer (),
                               td->auxiliaryBuffer->getLength (),
                               &(td->outputData), &chain,
                               td->appendOutputs, useChunks);

      sentData += td->auxiliaryBuffer->getLength ();

      td->auxiliaryBuffer->setLength (0);

      if (FilesUtility::getPathRecursionLevel (td->request.uri) >= 1)
        {
          const char* cur = formatString;
          string file;
          file.assign (td->request.uri);
          file.append ("/../");

          *td->auxiliaryBuffer << "<tr>\r\n";

          for (;;)
            {
              while (*cur && ((*cur == '%' ) || (*cur == ' ' )))
                cur++;

              if (!(*cur))
                break;

              if (*cur == 'f')
                *td->auxiliaryBuffer << "<td>\n"
                                     << "<a href=\""
                                     << (td->request.uriEndsWithSlash ? ".."
                                         :  ".")
                                     << "\">[ .. ]</a></td>\n";
              else
                *td->auxiliaryBuffer << "<td></td>\n";

              cur++;
            }

          *td->auxiliaryBuffer << "</tr>\r\n";

          appendDataToHTTPChannel (td, td->auxiliaryBuffer->getBuffer (),
                                   td->auxiliaryBuffer->getLength (),
                                   &(td->outputData), &chain,
                                   td->appendOutputs, useChunks);

          sentData += td->auxiliaryBuffer->getLength ();
        }

      /* Put all files in a vector.  */
      do
        {
          if (fd.name[0] == '.')
            continue;

          if (ignPattern)
            {
              regmatch_t pm[1];
              if (! ignRegex.exec (fd.name, 1, pm, 0))
                continue;
            }

          FileStruct file;
          file.name = fd.name;
          file.time_write = fd.time_write;
          file.attrib = fd.attrib;
          file.size = fd.size;
          files.push_back (file);
        }
      while (! fd.findnext ());

      fd.findclose ();

      /* Sort the vector.  */
      switch (sortType)
        {
        case 's':
          sort (files.begin (), files.end (), compareFileStructBySize);
          break;
        case 't':
          sort (files.begin (), files.end (), compareFileStructByTime);
          break;
        case 'f':
          sort (files.begin (), files.end (), compareFileStructByName);
        }

      if (sortReverse)
        reverse (files.begin (), files.end ());

      if (!td->request.uriEndsWithSlash)
        {
          linkPrefix.assign (&td->request.uri[lastSlash]);
          linkPrefix.append ("/");
        }
      else
        linkPrefix.assign ("");


      /* Build the files table and send it.  */
      for (vector<FileStruct>::iterator it = files.begin ();
           it != files.end (); it++)
        {
          FileStruct& file = *it;
          td->auxiliaryBuffer->setLength (0);
          generateElement (*td->auxiliaryBuffer, file, linkPrefix, formatString);
          appendDataToHTTPChannel (td, td->auxiliaryBuffer->getBuffer (),
                                   td->auxiliaryBuffer->getLength (),
                                   &(td->outputData), &chain,
                                   td->appendOutputs, useChunks);

          sentData += td->auxiliaryBuffer->getLength ();
        }

      td->auxiliaryBuffer->setLength (0);
      *td->auxiliaryBuffer << "</table>\r\n<hr />\r\n<address>"
                           << MYSERVER_VERSION;

      if (host && host->value.length ())
        {
          ostringstream portBuff;
          size_t portSeparator = host->value.find (':');
          *td->auxiliaryBuffer << " on ";
          if (portSeparator != string::npos)
            *td->auxiliaryBuffer << host->value.substr (0, portSeparator).c_str () ;
          else
            *td->auxiliaryBuffer << host->value.c_str () ;

          *td->auxiliaryBuffer << " Port ";
          portBuff << td->connection->getLocalPort ();
          *td->auxiliaryBuffer << portBuff.str ();
        }
      *td->auxiliaryBuffer << "</address>\r\n</body>\r\n</html>\r\n";
      ret = appendDataToHTTPChannel (td, td->auxiliaryBuffer->getBuffer (),
                                     td->auxiliaryBuffer->getLength (),
                                     &(td->outputData), &chain,
                                     td->appendOutputs, useChunks);


      sentData += td->auxiliaryBuffer->getLength ();

      *td->auxiliaryBuffer << end_str;
      /* Changes the \ character in the / character.  */
      bufferloop = td->auxiliaryBuffer->getBuffer ();
      while (*bufferloop++)
        if (*bufferloop == '\\')
          *bufferloop = '/';

      if (!td->appendOutputs && useChunks)
        chain.getStream ()->write ("0\r\n\r\n", 5, &nbw);

      /* For logging activity.  */
      td->sentData += sentData;
    }
  catch (exception & e)
    {
      /* Return an internal server error. */
      td->outputData.close ();
      chain.clearAllFilters ();
      td->connection->host->warningsLogWrite (_E ("HttpDir: internal error"),
                                              &e);
      return td->http->raiseHTTPError (500);
    }
  chain.clearAllFilters ();
  return HttpDataHandler::RET_OK;
}

/*!
  Format a string to html.
  \param in String to convert.
  \param out HTML converted string.
 */
void HttpDir::formatHtml (string& in, string& out)
{
  string::size_type pos = 0;
  out.assign (in);
  /*
    Replace characters in the ranges 32-65 91-96 123-126 160-255
    with "&#CODE;".
   */
  for (pos = 0; out[pos] != '\0'; pos++)
    {
      if (((u_char)out[pos] >= 32
           && (u_char)out[pos] <= 65)
          || ((u_char)out[pos] >= 91
              && (u_char)out[pos] <= 96)
          || ((u_char)out[pos] >= 123
              && (u_char)out[pos] <= 126)
          || ((u_char)out[pos] >= 160
              && (u_char)out[pos] < 255))
        {
          ostringstream os;
          os << "&#" << (int)((unsigned char)out[pos]) << ";";
          out.replace (pos, 1, os.str ());
          pos += os.str ().length () - 1;
        }
    }
}
