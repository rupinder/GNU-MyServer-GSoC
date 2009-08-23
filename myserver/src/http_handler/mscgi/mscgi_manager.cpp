/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006, 2007, 2008 Free Software Foundation, Inc.
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include "stdafx.h"
#include <include/base/xml/xml_parser.h>
#include <include/http_handler/mscgi/mscgi_manager.h>
#include <include/protocol/http/http.h>
#include <include/http_handler/mscgi/mscgi.h>
#include <include/base/string/securestr.h>

#include <algorithm>

using namespace std;


/*!
 *Write to the stdout.
 */
int MscgiManager::write(const char* str)
{
  return cgidata->mscgi->write(str, (u_long)strlen(str), cgidata);
}

/*!
 *Write binary to the stdout.
 */
int MscgiManager::write(const void* data, int len)
{
  return cgidata->mscgi->write((const char*)data, len, cgidata);
}

Server* MscgiManager::getServer()
{
  return cgidata->server;
}

/*!
 *Start the execution of the CGI.
 */
int MscgiManager::start(MsCgiData* data)
{
  cgidata = data;
  td = data->td;
  return 1;
}

/*!
 *Clean the memory allocated by the CGI.
 */
int MscgiManager::clean()
{
  return 1;
}

/*!
 *Set the HTTP error identifier.
 */
int MscgiManager::setPageError(int ID)
{
  td->response.httpStatus = ID;
  return 1;
}

/*!
 *Raise an HTTP error
 */
int MscgiManager::raiseError(int ID)
{
  cgidata->errorPage = ID;
  return 1;
}

/*!
 *Constructor of the class
 */
MscgiManager::MscgiManager(MsCgiData* data)
{
  start(data);
}

/*!
 *Destructor of the class
 */
MscgiManager::~MscgiManager(void)
{
  clean();
}

/*!
 *Returns the value of a param passed through the URL.
 */
char* MscgiManager::getParam(const char* param)
{
  const char* c;
  u_long len = 0;
  if(td->request.uriOpts.length() == 0)
    return 0;
  localbuffer[0]='\0';
  c = td->request.uriOpts.c_str();
  for(;;)
  {
    while((*c) && strncmp(c, param, std::min(strlen(param), strlen(c))))
      c++;

    if(*c == '\0')
    {
       return &localbuffer[0];
    }

    c += strlen(param);

    if(*c == '=')
    {
      c++;
      break;
    }
  }
  while((c[len]) && (c[len] != '&') && (len < LOCAL_BUFFER_DIM-1 ))
  {
    localbuffer[len] = c[len];
    localbuffer[len+1] = '\0';
    len++;
  }
  return localbuffer;
}

/*!
 *Returns the value of a param passed through a POST request.
 */
char* MscgiManager::postParam(const char* param)
{
  char buffer[LOCAL_BUFFER_DIM + 50];
  u_long nbr = 0;
  char c;

  u_long toRead = td->inputData.getFileSize();

  buffer[0] = '\0';
  localbuffer[0] = '\0';

  if( (toRead == 0) || (toRead == (u_long)-1) )
    return 0;
  do
  {
    cgidata->td->inputData.read(&c, 1, &nbr);
    if(nbr == 0)
      break;
    if((c == '&') |(toRead == 1))
    {
      if(!strncmp(param, buffer, strlen(param)))
      {
        myserver_strlcpy(localbuffer, &buffer[strlen(param)], LOCAL_BUFFER_DIM);
        break;
      }
      buffer[0] = '\0';
    }
    else
    {
      size_t len = strlen(buffer);
      if(len + 1 < LOCAL_BUFFER_DIM + 50)
      {
        buffer[len] = c;
        buffer[len + 1] = '\0';
      }
    }
  }while(--toRead);

  return localbuffer;

}

/*!
 *Write to stdout.
 */
int MscgiManager::operator <<(const char* str)
{
  return write(str);
}

/*!
 *Read from the stdin.
 */
char *MscgiManager::operator >>(const char* str)
{
  /*!
   *If it is a POST request return a param from the POST values
   *else return a GET param.
   */
  if(td->request.uriOptsPtr)
    return postParam(str);
  else
    return getParam(str);
}

/*!
 *Get the value of an environment variable.
 */
void MscgiManager::getenv(const char* lpszVariableName, char *lpvBuffer,
                        u_long* lpdwSize)
{
  ((char*)lpvBuffer)[0] = '\0';
  char *localEnv = cgidata->envString;
  size_t variableNameLen = strlen(lpszVariableName);
  for(u_long i = 0; ; i += (u_long)strlen(&localEnv[i]) + 1)
  {
    if(((localEnv[i+variableNameLen])== '=') &&
       (!strncmp(&localEnv[i], lpszVariableName, variableNameLen)))
    {
      u_long j = 0;
      u_long min_v = std::min((u_long)strlen(&localEnv[i + variableNameLen+1]),
                              (u_long)(*lpvBuffer)-1);
      for(j = 0; j < min_v; j++)
        lpvBuffer[j] = localEnv[i + variableNameLen + j + 1];
      lpvBuffer[j] = '\0';
      break;
    }
    else if((localEnv[i] == '\0') && (localEnv[i + 1] == '\0'))
    {
      break;
    }
  }
  *lpdwSize = (u_int)strlen((char*)lpvBuffer);
}

/*!
 *Returns the CGI data structure.
 *This structure is shared with the MyServer core so use it carefully!
 */
MsCgiData* MscgiManager::getCgiData()
{
  return cgidata;
}

/*!
 *Specify the MIME type for the data.
 */
void MscgiManager::setContentType(const char * type)
{
  td->response.contentType.assign(type, HTTP_RESPONSE_CONTENT_TYPE_DIM);
}

/*!
 *Add an HTTP header to the response.
 */
void MscgiManager::addHeader(const char* name,  const char *value)
{
  td->response.setValue(name, value);

}
