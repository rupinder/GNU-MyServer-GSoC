/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
*              GUINET Adrien (grainailleur) 2004
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

#ifndef MEMBUF_CLASS
#define MEMBUF_CLASS

#ifndef NULL
#define NULL 0
#endif

#include "../stdafx.h"


#define USE_NEW


#define DONT_MATCH_LENGTH // Comment this line to always make the buffer
			  // have the exact length of his data
			  // (involves reallocations => unrecommended)
#ifdef USE_NEW
#define mem_alloc(size) (new char[size])
#define mem_free(pAdr) (delete [] pAdr)
#else
#include <stdlib.h>
#define mem_alloc(size) ((char*) malloc(size))
#define mem_free(pAdr) (free(pAdr))
#endif

#define end_str '\0'

#include <string.h>

class CMemBuf
{
public:
	CMemBuf();
	CMemBuf(const void* pAdr, u_int size);
	CMemBuf(const CMemBuf& srcBuf);
	CMemBuf(CMemBuf& srcBuf, int bCopy);
	~CMemBuf();
public:
	void SetExternalBuffer(const void* pAdr, u_int size);
	int SetBuffer(const void* pAdr, u_int size);
	void SetLength(u_int newSize);

	void AddBuffer(const void* pAdr, u_int size);
	void AddBuffer(CMemBuf *nmb);

	int Free();
public:
	u_int Find(char c, u_int start = 0);
	u_int Find(CMemBuf *smb, u_int start = 0);
	u_int Find(const void* pAdr, u_int size, u_int start = 0);
	void Replace(char what, char by);
public:
	char& GetAt(u_int nIndex);
	char& operator[](u_int nIndex);

	int GetPart(u_int nStart, u_int nEnd, CMemBuf& result);
	int GetPartAsString(u_int nStart, u_int nEnd, CMemBuf& result);

	void* GetBufferSetLength(u_int newSize);

	u_int GetLength();
	u_int GetRealLength();

	int IsValid();

	const void* GetBuffer();
	operator const void*() ;
public:
	CMemBuf operator+ (CMemBuf& src);
	CMemBuf operator+ (const char* src);
	const CMemBuf& operator+= (CMemBuf& add);
	const CMemBuf& operator+= (const char* pStr);
	const CMemBuf& operator+= (char c) ;
public:
	CMemBuf& operator<< (const char* pSrc) ;
	CMemBuf& operator<< (int i) ;
	CMemBuf& operator<< (unsigned int i) ;
	CMemBuf& operator<< (long i) ;
	CMemBuf& operator<< (unsigned long i);
	CMemBuf& operator<< (char c) ;
	CMemBuf& operator<< (unsigned char c) ;
	CMemBuf& operator<< (const CMemBuf &src) ;
public:
	CMemBuf& operator=(const CMemBuf& src) ;
	CMemBuf& operator=(const char* src);
public:
	u_int m_nSizeLimit; // The maximun size that the buffer can reached ; 0 if none
	u_int m_nBlockLength; // Minimun size of new allocated blocks during addings
						 // We assume that m_nBlockLength < m_nSizeLimit
public: // Static conversion functions (hex, CRC...)
	static CMemBuf Hash_MD5(const void* pAdr, u_int nSize);
	static CMemBuf Hash_CRC(const void* pAdr, u_int nSize);
	static CMemBuf Hex(const void* pAdr, u_int nSize);
	static CMemBuf UIntToStr(u_int i);
	static CMemBuf IntToStr(int i);
	u_int StrToUint(const char* pAdr);
	unsigned char HexCharToNumber(unsigned char c);
	CMemBuf HexToData(const void* pAdr, u_int nSize);
	int StrToInt(const char* pAdr);
	static CMemBuf Hex(CMemBuf& membuf) ;
	static CMemBuf Hash_MD5(CMemBuf& membuf);
	static CMemBuf Hash_CRC(CMemBuf& membuf);
	static  CMemBuf UIntToStr(u_int i, char* pBufToUse, u_int nBufSize) ;
	static CMemBuf XIntToStr(u_int i, int bNegative, char* pBufToUse, u_int nBufSize);	
	static CMemBuf IntToStr(int i, char* pBufToUse, u_int nBufSize);
protected:
	static CMemBuf XIntToStr(u_int i, int bNegative);
	void AllocBuffer(u_int size);
	char* m_buffer; // Using of char* instead of void* because the C++ Compilator doesn't know the size of a void* !!!!!
	u_int m_nSize;
	u_int m_nRealSize;
	int m_bCanDelete;
	static u_int crc32Table[256];
};

#endif
