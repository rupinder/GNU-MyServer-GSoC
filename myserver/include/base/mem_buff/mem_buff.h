/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2009, 2010 Free Software Foundation,
  Inc.
  Copyright (C) 2004, Guinet Adrien (grainailleur)
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

#ifndef MEMBUF_H
# define MEMBUF_H

# ifndef NULL
#  define NULL 0
# endif

# include "myserver.h"
# include <iostream>
# include <string>
using namespace std;

# define end_str '\0'

class MemBuf
{
public:
  MemBuf ();
  MemBuf (const void* pAdr, size_t size);
  MemBuf (const MemBuf& srcBuf);
  MemBuf (MemBuf& srcBuf, int bCopy);
  ~MemBuf ();

  void setExternalBuffer (const void* pAdr, size_t size);
  int setBuffer (const void* pAdr, size_t size);
  void setLength (size_t newSize);
  void setRealLength (size_t newSize);

  void addBuffer (const void* pAdr, size_t size);
  void addBuffer (MemBuf *nmb);

  int free ();

  size_t find (char c, size_t start = 0);
  size_t find (MemBuf *smb, size_t start = 0);
  size_t find (const void* pAdr, size_t size, size_t start = 0);
  void replace (char what, char by);
  char& getAt (size_t nIndex);
  char& operator[](size_t nIndex);

  int getPart (size_t nStart, size_t nEnd, MemBuf& result);
  int getPartAsString (size_t nStart, size_t nEnd, MemBuf& result);

  char *getBuffersetLength (size_t newSize);

  size_t getLength ();
  size_t getRealLength ();

  int isValid ();

  char *getBuffer ();
  operator const void* ();
  MemBuf operator+ (MemBuf& src);
  MemBuf operator+ (const char* src);
  const MemBuf& operator+= (MemBuf& add);
  const MemBuf& operator+= (const char* pStr);
  const MemBuf& operator+= (char c) ;

  MemBuf& operator<< (const char* pSrc) ;
  MemBuf& operator<< (int i) ;
  MemBuf& operator<< (unsigned int i) ;
  MemBuf& operator<< (long i) ;
  MemBuf& operator<< (unsigned long i);
  MemBuf& operator<< (char c) ;
  MemBuf& operator<< (unsigned char c) ;
  MemBuf& operator<< (const MemBuf &src) ;
  MemBuf& operator<< (const string &src) ;
  MemBuf& operator= (const MemBuf& src) ;
  MemBuf& operator= (const char* src);

  void hashMD5(const void* pAdr, size_t nSize);
  void hashCRC (const void* pAdr, size_t nSize);
  void hex (const void* pAdr, size_t nSize);
  void uintToStr (size_t i);
  void intToStr (int i);
  size_t strToUint (const char* pAdr);
  unsigned char hexCharToNumber (unsigned char c);
  MemBuf hexToData (const void* pAdr, size_t nSize);
  int strToInt (const char* pAdr);
  void hex (MemBuf& membuf) ;
  void hashMD5 (MemBuf& membuf);
  void hashCRC (MemBuf& membuf);
  void uintToStr (size_t i, char* pBufToUse, size_t nBufSize) ;
  void xIntToStr (size_t i, int bNegative, char* pBufToUse, size_t nBufSize);
  void intToStr (int i, char* pBufToUse, size_t nBufSize);

  size_t getSizeLimit () {return nSizeLimit;}
  void setSizeLimit (size_t newSize) {nSizeLimit = newSize;}

protected:


  /* The maximun size that the buffer can reached ; 0 if none.  */
  size_t nSizeLimit;

  /* Minimum size of new allocated blocks during addings.
    We assume that nBlockLength < nSizeLimit.  */
  size_t nBlockLength;

  void xIntToStr (size_t i, int bNegative);
  void allocBuffer (size_t size);
  char *buffer;
  size_t nSize;
  size_t nRealSize;
  int bCanDelete;
  static u_int crc32Table[256];
};

#endif
