/*
*MyServer
*Copyright (C) 2002 The MyServer Team
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

#ifndef u_int
#define u_int unsigned int
#endif

#ifndef NULL
#define NULL 0
#endif



#define DONT_MATCH_LENGTH // Comment this line to always make the buffer
						  // have the exact length of his data
						  // (involves reallocations => unrecommended)
#ifdef USE_NEW
#define mem_alloc(size) (new char[size])
#define mem_free(pAdr) (delete [] pAdr)
#else
#include <malloc.h>
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
	CMemBuf(CMemBuf& srcBuf);
	CMemBuf(CMemBuf& srcBuf, int bCopy);
	~CMemBuf() {if (m_buffer != NULL && m_bCanDelete) delete [] m_buffer;};
public:
	int SetBuffer(const void* pAdr, u_int size);
	void SetLength(u_int newSize);

	void AddBuffer(const void* pAdr, u_int size);
	void AddBuffer(CMemBuf *nmb) {AddBuffer(nmb->m_buffer, nmb->m_nSize);};

	int Free() {if (m_buffer != NULL && m_bCanDelete) mem_free(m_buffer); m_buffer = NULL; m_nSize = m_nRealSize = 0; return true;};
public:
	u_int Find(char c, u_int start = 0);
	u_int Find(CMemBuf *smb, u_int start = 0) {return Find(smb->m_buffer, smb->m_nSize, start);};
	u_int Find(const void* pAdr, u_int size, u_int start = 0);
public:
	char& GetAt(u_int nIndex) {ASSERT(m_buffer != NULL); ASSERT(nIndex <= m_nSize); return *(m_buffer + nIndex);};
	char& operator[](u_int nIndex) {return GetAt(nIndex);};

	int GetPart(u_int nStart, u_int nEnd, CMemBuf& result);
	int GetPartAsString(u_int nStart, u_int nEnd, CMemBuf& result);

	void* GetBufferSetLength(u_int newSize);

	u_int GetLength() {return m_nSize;};

	int IsValid() {return ((m_nSize != 0) || (m_buffer != NULL));};

	const void* GetBuffer() {return (const void*) m_buffer;};
	operator const void*() {return (const void*) m_buffer;};
public:
	CMemBuf operator+ (CMemBuf& src) {CMemBuf temp(*this); temp.AddBuffer(&src); return temp;};
	CMemBuf operator+ (const char* src) {CMemBuf temp(*this); temp.AddBuffer((const void*) src, strlen(src)); return temp;};
	const CMemBuf& operator+= (CMemBuf& add) {AddBuffer(&add); return *this;};
	const CMemBuf& operator+= (const char* pStr) {AddBuffer(pStr, strlen(pStr)); return *this;};
	const CMemBuf& operator+= (char c) {AddBuffer(&c, 1); return *this;};
public:
	CMemBuf& operator<< (const char* pSrc) {AddBuffer(pSrc, strlen(pSrc)); return *this;};
	CMemBuf& operator<< (int i) {AddBuffer(&i, 4); return *this;};
	CMemBuf& operator<< (unsigned int i) {AddBuffer(&i, 4); return *this;};
	CMemBuf& operator<< (long i) {AddBuffer(&i, 4); return *this;};
	CMemBuf& operator<< (unsigned long i) {AddBuffer(&i, 4); return *this;};
	CMemBuf& operator<< (char c) {AddBuffer(&c, 1); return *this;};
	CMemBuf& operator<< (CMemBuf &src) {AddBuffer(src.m_buffer, src.m_nSize); return *this;};
public:
	CMemBuf& operator=(CMemBuf& src) {SetBuffer(src.m_buffer, src.m_nRealSize); return *this;};
	CMemBuf& operator=(const char* src) {SetBuffer((const void*) src, strlen(src) + 1); return* this;};
public:
	u_int m_nSizeLimit; // The maximun size that the buffer can reached ; 0 if none
	u_int m_nBlockLength; // Minimun size of new allocated blocks during addings
						 // We assume that m_nBlockLength < m_nSizeLimit
public: // Static conversion functions (hex, CRC...)
	static CMemBuf Hash_MD5(const void* pAdr, u_int nSize);
	static CMemBuf Hash_CRC(const void* pAdr, u_int nSize);
	static CMemBuf Hex(const void* pAdr, u_int nSize);
	static CMemBuf u_intToStr(u_int i) {return CMemBuf::XIntToStr(i, false);};
	static CMemBuf IntToStr(int i) {return CMemBuf::XIntToStr((u_int) (int) (-i), i < 0);};

	static CMemBuf Hex(CMemBuf& membuf) {return Hex(membuf.m_buffer, membuf.m_nSize);};
	static CMemBuf Hash_MD5(CMemBuf& membuf) {return Hash_MD5(membuf.m_buffer, membuf.m_nSize);};
	static CMemBuf Hash_CRC(CMemBuf& membuf) {return Hash_CRC(membuf.m_buffer, membuf.m_nSize);};
protected:
	static CMemBuf XIntToStr(u_int i, int bNegative);
	void AllocBuffer(u_int size) {Free(); m_buffer = mem_alloc(size); m_nRealSize = size;};
	char* m_buffer; // Using of char* instead of void* because the C++ Compilator doesn't know the size of a void* !!!!!
	u_int m_nSize;
	u_int m_nRealSize;
	int m_bCanDelete;
};

#endif