/*
MyServer
Copyright (C) 2005, 2006, 2007, 2008 Free Software Foundation, Inc.
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
#include <include/server/server.h>
#include <include/protocol/http/http.h>
#include <include/protocol/http/http_headers.h>
#include <include/http_handler/http_dir/http_dir.h>
#include <include/filter/filters_chain.h>
#include <include/base/file/files_utility.h>

extern "C" 
{
#ifdef WIN32
#include <direct.h>
#include <errno.h>
#endif

#ifdef NOT_WIN
#include <string.h>
#include <errno.h>
#endif
}

#include <include/base/find_data/find_data.h>
#include <include/base/string/stringutils.h>
#ifdef NOT_WIN
#endif

#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iomanip>

using namespace std;

/*!
 * Compare two HttpDir::FileStruct by their filename.
 */
bool HttpDir::compareFileStructByName (HttpDir::FileStruct i, HttpDir::FileStruct j)
{
  return stringcmpi (i.name, j.name) < 0 ? true : false;
}

/*!
 * Compare two HttpDir::FileStruct by their last modified file time.
 */

bool HttpDir::compareFileStructByTime (HttpDir::FileStruct i, HttpDir::FileStruct j)
{
  return i.time_write < j.time_write;
}

/*!
 * Compare two HttpDir::FileStruct by their file size.
 */
bool HttpDir::compareFileStructBySize (HttpDir::FileStruct i, HttpDir::FileStruct j)
{
  return i.size < j.size;
}

/*!
 *Constructor for the class.
 */
HttpDir::HttpDir()
{

}

/*!
 *Destroy the object.
 */
HttpDir::~HttpDir()
{

}

/*!
 *Load the static elements.
 */
int HttpDir::load(XmlParser* /*confFile*/)
{
  return 0;
}

/*!
 *Unload the static elements.
 */
int HttpDir::unLoad()
{
  return 0;
}

/*!
 *Get a formatted dobule representation of BYTES right shifted by POWER.
 *\param bytes Number of bytes.
 *\param power Power of 2.
 *\return the double representation for BYTES>>POWER.
 */
double HttpDir::formatBytes(u_long bytes, u_int power)
{
  /* u_long can be 32 bits.  Don't do a right shift that is bigger than
   * its size.  */
  const u_long quotient = (bytes >> (power / 2)) >> (power / 2);

  if (quotient == 0)
    return -1;

  const u_long unit = 1 << power;
  const u_long remainder = bytes & (unit - 1);
  const double result = quotient + static_cast<double>(remainder) / unit;
  return result;
}


/*!
 *Fullfill the string out with a formatted representation for bytes.
 *\param bytes Size to format.
 *\param out Out string. 
 */
void HttpDir::getFormattedSize(u_long bytes, string & out)
{
  const string symbols[] = {"TB", "GB", "MB", "KB", "bytes"};
  const u_int powers[] = {40, 30, 20, 10, 0};
  double result;
  size_t i;
  ostringstream osstr;

  if (bytes == 0)
  {
    out = "0";
    return;
  }

  for (i = 0; i < sizeof (powers); i++)
  {
    result = formatBytes (bytes, powers[i]);
    if (result != -1)
      break;
  }

  if((result - floor(result)) < 0.01)
    osstr << std::fixed << setprecision(0) << result << " " << symbols[i];
  else
    osstr << std::fixed << setprecision(2) << result << " " << symbols[i];
  
  out = osstr.str();
}

/*!
 *Browse a directory printing its contents in an HTML file.
 *\param td The current thread context.
 *\param s The current connection structure.
 *\param directory Directory to show.
 *\param cgi non used.
 *\param onlyHeader Specify if send only the HTTP header.
*/
int HttpDir::send(HttpThreadContext* td, ConnectionPtr s, 
                  const char* directory, const char* /*cgi*/, 
                  int /*execute*/, int onlyHeader)
{
  u_long nbw;
  string filename;
  int ret;
  FindData fd;
  FiltersChain chain;
  int lastSlash = 0;
  bool useChunks = false;
  u_long sentData = 0;
  int i;
  char fileTime[32];
  char* bufferloop;
  const char* browseDirCSSpath;
  bool keepalive = false;
  vector<HttpDir::FileStruct> files;
  size_t sortIndex;
  char sortType;
  bool sortReverse = false;
  HttpRequestHeader::Entry *host = td->request.other.get("Host");


  chain.setProtocol(td->http);
  chain.setProtocolData(td);
  chain.setStream(td->connection->socket);
  if(td->mime && Server::getInstance()->getFiltersFactory()->chain(&chain, 
                                                        td->mime->filters, 
                                             td->connection->socket, &nbw, 1))
  {
    td->connection->host->warningsLogRequestAccess(td->id);
    td->connection->host->warningsLogWrite("HttpDir: Error loading filters");
    td->connection->host->warningsLogTerminateAccess(td->id);
    chain.clearAllFilters(); 
    return td->http->raiseHTTPError(500);
  }

  lastSlash = td->request.uri.rfind('/') + 1;

  checkDataChunks(td, &keepalive, &useChunks);

  td->response.contentType.assign("text/html");

  if(!td->appendOutputs)
  {

    HttpHeaders::buildHTTPResponseHeader(td->buffer->getBuffer(), 
                                         &(td->response));

    if(s->socket->send(td->buffer->getBuffer(), 
                       (u_long)strlen(td->buffer->getBuffer()), 0) 
       == SOCKET_ERROR)
    {
      /* Remove the connection.  */
      return 0;
    }  
  }    

  if(onlyHeader)
    return 1;


  sortIndex = td->request.uriOpts.find("sort=");

  if(sortIndex != string::npos && sortIndex + 5 < td->request.uriOpts.length())
  {
    sortType = td->request.uriOpts.at(sortIndex + 5);
  }

  if(sortIndex != string::npos && sortIndex + 6 < td->request.uriOpts.length())
  {
     sortReverse = td->request.uriOpts.at(sortIndex + 6) == 'I';
  }

  /* Make sortType always lowercase.  */
  switch(sortType)
  {
    case 's':
    case 'S':
      sortType = 's';
      break;
    case 't':
    case 'T':
      sortType = 't';
      break;
    default:
      sort (files.begin(), files.end(), compareFileStructByName);
  }

  browseDirCSSpath = td->http->getBrowseDirCSSFile();

  td->buffer2->setLength(0);
  *td->buffer2 << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
    "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\"\r\n"
    "\"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\r\n"
    "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\">"
    "\r\n<head>\r\n<title>" ;
  *td->buffer2 << td->request.uri.c_str() ;
  *td->buffer2 << "</title>\r\n";

  /*
   *If it is defined a CSS file for the graphic layout of 
   *the browse directory insert it in the page.  
   */
  if(browseDirCSSpath != 0)
  {
    *td->buffer2 << "<link rel=\"stylesheet\" href=\""
                 << browseDirCSSpath 
                 << "\" type=\"text/css\" media=\"all\"/>\r\n";
  }


  *td->buffer2 << "</head>\r\n"; 

  ret = appendDataToHTTPChannel(td, td->buffer2->getBuffer(),
                                td->buffer2->getLength(),
                                &(td->outputData), &chain,
                                td->appendOutputs, useChunks);
  if(ret)
  {
    /* Return an internal server error. */
    td->outputData.closeFile();
    chain.clearAllFilters(); 
    return td->http->raiseHTTPError(500);
  }

  sentData = td->buffer2->getLength();
              
  browseDirCSSpath = td->http->getBrowseDirCSSFile();

  filename = directory;
  td->buffer2->setLength(0);
  *td->buffer2 << "<body>\r\n<h1>Contents of directory ";
  *td->buffer2 <<  &td->request.uri[lastSlash] ;
  *td->buffer2 << "</h1>\r\n<hr />\r\n";

  ret = appendDataToHTTPChannel(td, td->buffer2->getBuffer(),
                                td->buffer2->getLength(),
                                &(td->outputData), &chain,
                                td->appendOutputs, useChunks);

  if(ret)
  {
    td->outputData.closeFile();
    /* Return an internal server error.  */
    return td->http->raiseHTTPError(500);
  }
  sentData += td->buffer2->getLength();

  ret = fd.findfirst(filename.c_str());

  if(ret == -1)
  {
    chain.clearAllFilters(); 
    return td->http->raiseHTTPError(404);
  }

  /*
   *With the current code we build the HTML TABLE to indicize the
   *files in the directory.
   */
  td->buffer2->setLength(0);
  *td->buffer2 << "<table width=\"100%\">\r\n<tr>\r\n" ;

  if(sortType == 'f' && !sortReverse)
    *td->buffer2 << "<th><a href=\"?sort=fI\">File</a></th>\r\n";
  else
    *td->buffer2 << "<th><a href=\"?sort=f\">File</a></th>\r\n";

  if(sortType == 't' && !sortReverse)
    *td->buffer2 << "<th><a href=\"?sort=tI\">Last Modified</a></th>\r\n";
  else
    *td->buffer2 << "<th><a href=\"?sort=t\">Last Modified</a></th>\r\n";

  if(sortType == 's' && !sortReverse)
    *td->buffer2 << "<th><a href=\"?sort=sI\">Size</a></th>\r\n</tr>\r\n";
  else
    *td->buffer2 << "<th><a href=\"?sort=s\">Size</a></th>\r\n</tr>\r\n";

  ret = appendDataToHTTPChannel(td, td->buffer2->getBuffer(),
                                td->buffer2->getLength(),
                                &(td->outputData), &chain,
                                td->appendOutputs, useChunks);

  if(ret)
  {
    td->outputData.closeFile();
    chain.clearAllFilters(); 
    /* Return an internal server error.  */
    return td->http->raiseHTTPError(500);
  }

  sentData += td->buffer2->getLength();

  td->buffer2->setLength(0);

  if(FilesUtility::getPathRecursionLevel(td->request.uri) >= 1)
  {
    string file;
    file.assign(td->request.uri);
    file.append("/../");
    
    *td->buffer2 << "<tr>\r\n<td colspan=\"2\">"
                 << "<a href=\""
                 << (td->request.uriEndsWithSlash ? ".." : ".")
           << "\">[ .. ]</a></td>\n"
                 << "<td>[directory]</td></tr>\r\n";
    
    ret = appendDataToHTTPChannel(td, td->buffer2->getBuffer(),
                                  td->buffer2->getLength(),
                                  &(td->outputData), &chain,
                                  td->appendOutputs, useChunks);
    if(ret)
    {
      fd.findclose();
      td->outputData.closeFile();
      chain.clearAllFilters(); 
      /* Return an internal server error.  */
      return td->http->raiseHTTPError(500);
    }
    sentData += td->buffer2->getLength();
  }

  /* Put all files in a vector.  */
  do
  {
    if(fd.name[0] == '.')
      continue;
    /* Do not show the security file.  */
    if(!strcmp(fd.name, "security"))
      continue;

    FileStruct file;
    file.name.assign(fd.name);
    file.time_write = fd.time_write;
    file.attrib = fd.attrib;
    file.size = fd.size;
    files.push_back(file);

  }
  while(!fd.findnext());

  fd.findclose();

  /* Sort the vector.  */
  switch(sortType)
  {
    case 's':
      sort (files.begin(), files.end(), compareFileStructBySize);
      break;
    case 't':
      sort (files.begin(), files.end(), compareFileStructByTime);
      break;
    case 'f':
      sort (files.begin(), files.end(), compareFileStructByName);
  }

  if(sortReverse)
    reverse (files.begin(), files.end());

  /* Build the files table and send it.  */
  for(vector<FileStruct>::iterator it = files.begin();
      it != files.end(); it++)
  {  
    string formattedName;

    FileStruct& file = *it;

    td->buffer2->setLength(0);

    *td->buffer2 << "<tr>\r\n<td><a href=\"";
    if(!td->request.uriEndsWithSlash)
    {
      *td->buffer2 << &td->request.uri[lastSlash];
      *td->buffer2 << "/" ;
    }
    formattedName.assign(file.name);

    formatHtml(file.name, formattedName);

    *td->buffer2 << formattedName ;
    *td->buffer2 << "\">" ;
    *td->buffer2 << formattedName;
    *td->buffer2 << "</a></td>\r\n<td>";
  
    getRFC822GMTTime(file.time_write, fileTime, 32);

    *td->buffer2 << fileTime ;
    *td->buffer2 << "</td>\r\n<td>";
    
    if(file.attrib & FILE_ATTRIBUTE_DIRECTORY)
    {
      *td->buffer2 << "[directory]";
    }
    else
    {
      string out;
      getFormattedSize(file.size, out);
       *td->buffer2 << out;
    }

    *td->buffer2 << "</td>\r\n</tr>\r\n";
    ret = appendDataToHTTPChannel(td, td->buffer2->getBuffer(),
                                  td->buffer2->getLength(),
                                  &(td->outputData), &chain,
                                  td->appendOutputs, useChunks);
    if(ret)
    {
      td->outputData.closeFile();
      chain.clearAllFilters(); 
      /* Return an internal server error.  */
      return td->http->raiseHTTPError(500);
    }

    sentData += td->buffer2->getLength();

  }

  td->buffer2->setLength(0);
  *td->buffer2 << "</table>\r\n<hr />\r\n<address>"
               << MYSERVER_VERSION;
              
  if(host && host->value->length())
  {    
    ostringstream portBuff;
    size_t portSeparator = host->value->find(':');
    *td->buffer2 << " on ";
    if(portSeparator != string::npos)
      *td->buffer2 << host->value->substr(0, portSeparator).c_str() ;
    else
      *td->buffer2 << host->value->c_str() ;
    
    *td->buffer2 << " Port ";
    portBuff << td->connection->getLocalPort();
    *td->buffer2 << portBuff.str();
  }
  *td->buffer2 << "</address>\r\n</body>\r\n</html>\r\n";
  ret = appendDataToHTTPChannel(td, td->buffer2->getBuffer(),
                                td->buffer2->getLength(),
                                &(td->outputData), &chain,
                                td->appendOutputs, useChunks);

  if(ret)
  {
    td->outputData.closeFile();
    /* Return an internal server error.  */
    return td->http->raiseHTTPError(500);
  }  
  sentData += td->buffer2->getLength();

  *td->buffer2 << end_str;
  /* Changes the \ character in the / character.  */
  bufferloop = td->buffer2->getBuffer();
  while(*bufferloop++)
    if(*bufferloop == '\\')
      *bufferloop = '/';

  if(!td->appendOutputs && useChunks)
  {
    if(chain.write("0\r\n\r\n", 5, &nbw))
      return 1;
  }

  /* For logging activity.  */  
  td->sentData += sentData;

  chain.clearAllFilters(); 
  return 1;

}

/*!
 *Format a string to html.
 *\param name String to convert.
 *\param out HTML converted string.
 */
void HttpDir::formatHtml(string& in, string& out)
{
  string::size_type pos = 0;
  out.assign(in);
  /*
   *Replace characters in the ranges 32-65 91-96 123-126 160-255
   *with "&#CODE;".
   */
  for(pos = 0; out[pos] != '\0'; pos++)
  {
    if(((u_char)out[pos] >= 32 && 
        (u_char)out[pos] <= 65)   ||
       ((u_char)out[pos] >= 91 && 
        (u_char)out[pos] <= 96)   ||
       ((u_char)out[pos] >= 123 && 
        (u_char)out[pos] <= 126) ||
       ((u_char)out[pos] >= 160 && 
        (u_char)out[pos] < 255))
    {
      ostringstream os;
      os << "&#" << (int)((unsigned char)out[pos]) << ";";
      out.replace(pos, 1, os.str());
      pos += os.str().length() - 1;
    }
  }
}
