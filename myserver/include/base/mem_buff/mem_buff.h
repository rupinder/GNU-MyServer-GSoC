/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2002, 2003, 2004 Free Software Foundation, Inc.
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

# include "stdafx.h"
# include <iostream>
# include <string>
using namespace std;

# define USE_NEW


# define DONT_MATCH_LENGTH // Comment this line to always make the buffer
			  // have the exact length of his data
			  // (involves reallocations => unrecommended)
# ifdef USE_NEW
#  define mem_alloc(size) (new char[size])
#  define mem_free(pAdr) (delete [] pAdr)
# else
#  include <stdlib.h>
#  define mem_alloc(size) ((char*) malloc(size))
#  define mem_free(pAdr) (free(pAdr))
# endif

# define end_str '\0'

class MemBuf
{
public:
	MemBuf();
	MemBuf(const void* pAdr, u_int size);
	MemBuf(const MemBuf& srcBuf);
	MemBuf(MemBuf& srcBuf, int bCopy);
	~MemBuf();

	void setExternalBuffer(const void* pAdr, u_int size);
	int setBuffer(const void* pAdr, u_int size);
	void setLength(u_int newSize);

	void addBuffer(const void* pAdr, u_int size);
	void addBuffer(MemBuf *nmb);

	int free();

	u_int find(char c, u_int start = 0);
	u_int find(MemBuf *smb, u_int start = 0);
	u_int find(const void* pAdr, u_int size, u_int start = 0);
	void replace(char what, char by);
	char& getAt(u_int nIndex);
	char& operator[](u_int nIndex);

	int getPart(u_int nStart, u_int nEnd, MemBuf& result);
	int getPartAsString(u_int nStart, u_int nEnd, MemBuf& result);

	char* getBuffersetLength(u_int newSize);

	u_int getLength();
	u_int getRealLength();

	int isValid();

	char* getBuffer();
	operator const void*() ;
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
	MemBuf& operator=(const MemBuf& src) ;
	MemBuf& operator=(const char* src);

	u_int m_nSizeLimit; // The maximun size that the buffer can reached ; 0 if none
	u_int m_nBlockLength; // Minimun size of new allocated blocks during addings
						 // We assume that m_nBlockLength < m_nSizeLimit

	void hashMD5(const void* pAdr, u_int nSize);
	void hashCRC(const void* pAdr, u_int nSize);
	void hex(const void* pAdr, u_int nSize);
	void uintToStr(u_int i);
	void intToStr( int i);
	u_int strToUint(const char* pAdr);
	unsigned char hexCharToNumber(unsigned char c);
	MemBuf hexToData(const void* pAdr, u_int nSize);
	int strToInt(const char* pAdr);
	void hex(MemBuf& membuf) ;
	void hashMD5(MemBuf& membuf);
	void hashCRC(MemBuf& membuf);
	void uintToStr(u_int i, char* pBufToUse, u_int nBufSize) ;
	void xIntToStr(u_int i, int bNegative, char* pBufToUse, u_int nBufSize);
	void intToStr(int i, char* pBufToUse, u_int nBufSize);

protected:
	void xIntToStr(u_int i, int bNegative);
	void allocBuffer(u_int size);
	char* m_buffer; // Using of char* instead of void* because the C++ Compilator doesn't know the size of a void* !!!!!
	u_int m_nSize;
	u_int m_nRealSize;
	int m_bCanDelete;
	static u_int crc32Table[256];
};

#endif
