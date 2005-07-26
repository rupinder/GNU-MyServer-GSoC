/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "../include/protocol.h"
#include "../include/securestr.h"
#include "../include/control_header.h"
#include "../include/cXMLParser.h"
#include "../include/control_errors.h"
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

#ifdef NOT_WIN
#include "../include/lfind.h"

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

// Bloodshed Dev-C++ Helper
#ifndef intptr_t
#define intptr_t int
#endif

/*!
 *Return a string containing options specified by the client.
 */
char* ControlHeader::getOptions()
{
  return cmdOptions;
}

/*!
 *Return a string containing the auth login name in the MD5 format.
 */
char* ControlHeader::getAuthLogin()
{
  return authLogin;
}

/*!
 *Return a string containing the auth login password in the MD5 format.
 */
char* ControlHeader::getAuthPassword()
{
  return authPassword;
}

/*!
 *Return a string containing the command specified in the request.
 */
char* ControlHeader::getCommand()
{
  return command;
}

/*!
 *Return a string containing the connection type.
 *Connection can be Closed or Keep-Alive.
 */
char* ControlHeader::getConnection()
{
  return connection;
}

/*!
 *Return the version of protocol used.
 */
char *ControlHeader::getVersion()
{
  return version;
}

/*!
 *Return the data length specified in the client header.
 */
int ControlHeader::getLength()
{
  return length;
}


/*!
 *Costructor for the class.
 */
ControlHeader::ControlHeader()
{
  reset();
}

/*!
 *Reset everything.
 */
void ControlHeader::reset()
{
  /*! Reset everything. */
  command[0]='\0';
  cmdOptions[0]='\0';
  authLogin[0]='\0';
  authPassword[0]='\0';
  version[0]='\0';
  length=0;
}

/*!
 *Destructor for the ControlHeader class.
 */
ControlHeader::~ControlHeader()
{

}

/*!
 *Parse the header in buffer. Put the effective length of the header in len.
 *Return a control_error. See control_errors.h to have a list of all errors.
 *Return -1 on an incomplete header.
 *The header has this form:
 *_/CMD VERSION more_options\r\n
 *_/AUTH md5(name:password)\r\n
 *_/LEN length of data\r\n
 *_***
 *_Additional fields.(Not used now)
 *_***
 *_\r\n        -> Header ends with a \r\n.
 *_Data...
 */
int ControlHeader::parse_header(char *buffer, int bufferlen, int *len)
{
  /*! Do a reset before the parsing. */
  reset();

  /*! Do we find a \r\n\r\n sequence? */
  int end_reached = 0;

  /*! No buffer was specified. */
  if(buffer == 0)
    return CONTROL_INTERNAL;

  /*! Nothing to parse. */
  if(bufferlen == 0)
    return CONTROL_INTERNAL;

  /*! buffer offset that we are parsing. */
  char *offset = buffer;

  /*! # of the line we are parsing. */
  int nLine = 1;

  for( ; ; )
  {
    char *field = (char*)offset ;

    /*! Get the length of the field name. */
    int fieldLen = 0;

    if(nLine == 1)
    {
      /*!
       *Do not copy initial /.
       */
      field++ ;
      offset++ ;

      /*! Get the length of the field name. */
      fieldLen = getCharInString(field, " ", 32);
      
      /*! Return nonzero on errors. */
      if(fieldLen == -1)
        return CONTROL_MALFORMED;

      /*! 
       *For first line field name is the command itself. 
       *Do not copy initial /.
       */
      myserver_strlcpy(command, field, min(fieldLen + 1 , 32) );
      
      /*! Update the offset. */
      offset += fieldLen + 1;
      
      int versionLen = getCharInString(offset, " \r", 32);
      if(versionLen == -1)
        return CONTROL_MALFORMED;

      myserver_strlcpy(version, offset, min(versionLen + 1, 12) );
      offset += versionLen + 1;

      int optionsLen = getCharInString(offset, "\r", 32);
      if(optionsLen == -1)
        cmdOptions[0]='\0';
      else
        myserver_strlcpy(cmdOptions, offset, min(optionsLen + 1, 64) );
      /*! Put the offset at the end of \r\n. */
      offset += ((optionsLen!= -1)?optionsLen:0) + 2 ;
    }
    else/*! Handle other lines other than the first. */
    {
      /*! We reach the end of the request. */
      if(field[0]=='\r' && field[1]=='\n')
      {
        end_reached = 1;
        offset += 2 ;
        break;
      }

      /*! Get the length of the field name. */
      fieldLen = getCharInString(field, " ", 32);

      if(fieldLen == -1)
      {
        return -1;
      }

      if(!strncmp(field, "/auth ", 6))
      {
        offset += 6;
        int len = getCharInString(offset, ":", 64);
        if(len == -1)
          return CONTROL_MALFORMED;
        myserver_strlcpy(authLogin, offset, min(len + 1, 64));
        offset+=len + 1;
        len = getCharInString(offset, "\r", 64);
        if(len == -1)
          return CONTROL_MALFORMED;
        myserver_strlcpy(authPassword, offset, min(len + 1, 64));
        offset += len + 2;
      }
      else if(!strncmp(field, "/connection ", 12))
      {
        offset += 12;
        int len = getCharInString(offset, "\r", 32);
        if(len == -1)
          return CONTROL_MALFORMED;
        myserver_strlcpy(connection, offset, min(len + 1, 32));
        offset += len + 2;
      }
      else if(!strncmp(field, "/LEN ", 5))
      {
        offset += 5;
        int len = getCharInString(offset, "\r", 32);
        if(len == -1)
          return CONTROL_MALFORMED;
        char tmp_buff[12];
        myserver_strlcpy(tmp_buff, offset, min(len + 1 , 12));
        length = atoi(tmp_buff);
        offset += len + 2;
      }   
      else
      {
        int len = getCharInString(offset, "\r", 32);
        if(len == -1)
          return CONTROL_MALFORMED;
        offset += len + 2;
      }   
    }
    /*! Increment the nLine value for the next line parsing. */
    nLine++;
    
    /*! Do we need more than 20 lines? */
    if(nLine > 20)
      return CONTROL_MALFORMED;
  }
  /*! Save the effective request length. */
  if(len)
    *len = static_cast<int>(offset - buffer);

  if(end_reached)
    return CONTROL_OK;
  else
    return -1;
}
