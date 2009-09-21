/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2008 Free Software Foundation, Inc.
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

#ifndef IP_H
# define IP_H

# include "stdafx.h"
# include <string>

class IpRange
{
 public:
  virtual bool InRange(const std::string &ip) = 0;
  virtual bool InRange(const IpRange *pRange) = 0;

  static IpRange *RangeFactory(const std::string &ipRange);
};

class Ipv4Range : public IpRange
{
public:
  Ipv4Range();
  Ipv4Range(const std::string &sRange);
  Ipv4Range(const std::string &sStartHost, const std::string &sEndHost);

  bool SetRange(const std::string &sRange);
  bool SetRange(const std::string &sStartHost, const std::string &sEndHost);

  virtual ~Ipv4Range();
  virtual bool InRange(const std::string &ip);
  virtual bool InRange(const IpRange *pRange);

  const unsigned char* GetStart() const { return m_nStart; }
  const unsigned char* GetEnd() const { return m_nEnd; }
protected:
  bool Init();
  bool InRange(const unsigned char addr[4]);
  unsigned char m_nStart[4], m_nEnd[4], m_nMask[4];
};

//TODO: implement
class Ipv6Range : public IpRange
{
};

#endif //IP_H
