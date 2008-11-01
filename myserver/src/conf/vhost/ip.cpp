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

#include <include/conf/vhost/ip.h>
#include <sstream>

/*!
 * comment here
 */
IpRange *IpRange::RangeFactory(const std::string &ipRange)
{
  Ipv4Range *pV4 = new Ipv4Range();
  if ( pV4->SetRange(ipRange) )
    return pV4;

  /* when will be implemented
  Ipv6Range *pV6 = new Ipv6Range();
  if ( pV6->SetRange(ipRange) )
    return pV6;
  */

  return NULL;
}

/*!
 * Ipv4Range c-tor
 */
Ipv4Range::Ipv4Range()
{
  Init();
}

/*!
 * Ipv4Range c-tor
 */
Ipv4Range::Ipv4Range(const std::string &sRange)
{
  SetRange(sRange);
}

/*!
 * range given as x.x.x.x-y.y.y.y or x.x.x.x(/y)
 */
bool Ipv4Range::SetRange(const std::string &sRange)
{
  if ( !Init() )
    return false;
  if ( sRange.empty() )
    return true;//just init
  std::string::size_type nPos = sRange.find('-');
  if ( nPos != std::string::npos )// x.x.x.x-y.y.y.y form
    {
      std::string start(sRange.substr(0, nPos));
      std::string end(sRange.substr(nPos + 1));
      return SetRange(start, end);
    }
  else// x.x.x.x(/y) form
    {
      std::istringstream istream(sRange);
      char nSep = 0;
      unsigned char nAddr[4];
      int nTemp = 0;
      for ( int i = 0; i < 4 && !istream.eof(); i++ )
	{
	  istream >> nTemp;
	  nAddr[i] = nTemp;
	  istream >> nSep;
	}

      nTemp = 32;
      if ( nSep == '/' )
	  istream >> nTemp;

      if ( !istream.eof() )
	return false;

      for ( int i = 0, nByte = 0; i < nTemp && nByte < 4; i++ )
	{
	  m_nMask[nByte] += (1 << (i+1)%8);
	  if ( (i+1)%8 == 0 )
	    nByte++;
	}
      for ( int i = 0; i < 4; i++ )
	{
	  if ( m_nMask[i] != 0 )
	    {
	      m_nStart[i] = nAddr[i];
	      m_nEnd[i] = nAddr[i];
	    }
	}
    }
  return true;
}

bool Ipv4Range::SetRange(const std::string &sStartHost, const std::string &sEndHost)
{
  if ( !Init() )
    return false;

  std::istringstream start(sStartHost), end(sEndHost);
  char nSep = 0;
  int nTemp = 0;
  for ( int i = 0; i < 4 && !start.eof() && !end.eof(); i++ )
    {
      start >> nTemp;
      m_nStart[i] = nTemp;
      start >> nSep;
      end >> nTemp;
      m_nEnd[i] = nTemp;
      end >> nSep;
    }
  if ( !start.eof() || !end.eof() )
    return false;

  // get mask lenght(max common addr part)
  char bs = 0;
  for ( bs = 0; bs < 32; bs++ )
    {
      if ( (m_nStart[bs/8] & (1 << bs%8)) != (m_nEnd[bs/8] & (1 << bs%8)) )
	break;
    }
  for ( int i = 0, nByte = 0; i < bs && nByte < 4; i++ )
    {
      m_nMask[nByte] += (1 << (i+1)%8);
      if ( (i+1)%8 == 0 )
	nByte++;
    }
  return true;
}

/*!
 * Ipv4Range initializer
 * return false on error
 */
bool Ipv4Range::Init()
{
  for ( int i = 0; i < 4; i++ )
    {
      m_nStart[i] = 0;
      m_nEnd[i] = 255;
      m_nMask[i] = 0;
    }
  return true;
}

/*!
 *d-tor
 */
Ipv4Range::~Ipv4Range()
{
}

/*!
 *checks if addr from param belongs the same network address
 */
bool Ipv4Range::InRange(const unsigned char addr[4])
{
  unsigned char hostMask[4];
  for ( int i = 0; i < 4; i++ )
    {
      if ( (addr[i] & m_nMask[i]) != (m_nStart[i] & m_nMask[i]) )
	return false;//networks differ
    }
  for ( int i = 0; i < 4; i++ )
    {
      hostMask[i] = ~m_nMask[i];
      if ( (m_nStart[i] & hostMask[i]) < (addr[i] & hostMask[i]) )
	  break;
      if ( (m_nStart[i] & hostMask[i]) > (addr[i] & hostMask[i]) )
	  return false;
    }
  for ( int i = 0; i < 4; i++ )
    {
      hostMask[i] = ~m_nMask[i];
      if ( (m_nEnd[i] & hostMask[i]) > (addr[i] & hostMask[i]) )
	return true;
      if ( (m_nEnd[i] & hostMask[i]) < (addr[i] & hostMask[i]) )
	return false;
    }
  return true;//equal to start or end
}

bool Ipv4Range::InRange(const std::string &ip)
{
  unsigned char addr[4];
  std::istringstream stream(ip);
  char nSep = 0;
  int nTemp = 0;
  for ( int i = 0; i < 4 && !stream.eof(); i++ )
    {
      stream >> nTemp;
      addr[i] = nTemp;
      if ( (addr[i] & m_nMask[i]) != (m_nStart[i] & m_nMask[i]) )
	return false;//networks differ
      stream >> nSep;
    }

  return InRange(addr);
}

bool Ipv4Range::InRange(const IpRange *pRange)
{
  if ( pRange == NULL )
    return false;
  const Ipv4Range *pLocal = static_cast<const Ipv4Range *>(pRange);
  return InRange(pLocal->GetStart()) && InRange(pLocal->GetEnd());
}
