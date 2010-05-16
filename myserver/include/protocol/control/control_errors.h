/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2004, 2005, 2009, 2010 Free Software Foundation, Inc.
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

#ifndef CONTROL_ERRORS_H
# define CONTROL_ERRORS_H

# include "myserver.h"

/*!
  These are all the errors that the control server can return to a client.
  Use these definitions instead of hardly-code the value.
 */

enum
  {
    /*! The request was accepted and served. */
    CONTROL_OK = 100,

    /*! A generic error was encountered. */
    CONTROL_ERROR = 200,

    /*! An internal server error happened. */
    CONTROL_INTERNAL = 201,

    /*! The Authorization was not accepted. */
    CONTROL_AUTH = 202,

    /*! A malformed request was sent. */
    CONTROL_MALFORMED = 203,

    /*! A bad command was specified. */
    CONTROL_CMD_NOT_FOUND = 204,

    /*! A bad length field was specified. */
    CONTROL_BAD_LEN = 205,

    /*! The server is too busy to handle the request. */
    CONTROL_SERVER_BUSY = 206,

    /*! The client uses a version of this protocol that we cannot understand. */
    CONTROL_BAD_VERSION = 207,

    /*! The requested file doesn't exist on the local FS. */
    CONTROL_FILE_NOT_FOUND = 208
  };

#endif
