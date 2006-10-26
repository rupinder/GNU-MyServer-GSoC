/*
MyServer
Copyright (C) 2004, 2005 The MyServer Team
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

#ifndef CONTROL_ERRORS_H
#define CONTROL_ERRORS_H
/*!
 *These are all the errors that the control server can return to a client.
 *Use these definitions instead of hardly-code the value.
 */

  /*! The request was accepted and served. */
#define CONTROL_OK            100    
  
  /*! A generic error was encountered. */
#define CONTROL_ERROR         200    

  /*! An internal server error happened. */
#define CONTROL_INTERNAL      201  

  /*! The Authorization was not accepted. */
#define CONTROL_AUTH          202    

  /*! A malformed request was sent. */
#define CONTROL_MALFORMED     203

  /*! A bad command was specified. */
#define CONTROL_CMD_NOT_FOUND    204

  /*! A bad length field was specified. */
#define CONTROL_BAD_LEN     205

  /*! The server is too busy to handle the request. */
#define CONTROL_SERVER_BUSY      206

  /*! The client uses a version of this protocol that we cannot understand. */
#define CONTROL_BAD_VERSION      207

  /*! The requested file doesn't exist on the local FS. */
#define CONTROL_FILE_NOT_FOUND      208

#endif
