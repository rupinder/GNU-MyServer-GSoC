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
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef CONTROL_HEADER_H
#define CONTROL_HEADER_H
#include "../stdafx.h"
#include "../include/protocol.h"

class control_header
{
  char connection[32];
  char command[32];
  char cmdOptions[64];
  char auth[64];
  char version[12];
  int length;
public:
  void reset();
  char *getOptions();
  char *getVersion();
  char *getConnection();
  int getLength();
  char *getAuthName();
  char *getCommand();
  control_header();
  virtual ~control_header();
  int parse_header(char *buffer, int bufferlen, int*);
};

#endif
