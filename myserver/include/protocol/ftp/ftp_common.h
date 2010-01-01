/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2008, 2009, 2010 Free Software Foundation, Inc.
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

#ifndef FTP_COMMON_H
# define FTP_COMMON_H

# include "stdafx.h"

# include <string>

# define PARSER_STR_LEN 512

struct FtpHost
{
  int h1, h2, h3, h4;
  int p1, p2;
};

void setFtpHost (FtpHost &out, const FtpHost &in);
void setFtpHost (FtpHost &out, const char *szIn);
void getIpAddr (const FtpHost &host, char *pOut, const int &nBuffSize);
int getPortNo (const FtpHost &host);
std::string getPortNo (unsigned int nport);
std::string getHost (const FtpHost &host);

#endif //__FTP_COMMON_H__
