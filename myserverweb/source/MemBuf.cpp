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

#include "../include/MemBuf.h"
#include "../include/md5.h"

#ifndef DONT_MATCH_LENGTH
#include <math.h> // for the log10 function
#endif

CMemBuf::CMemBuf()
{
	m_buffer = NULL;
	m_nSize = 0;
	m_nRealSize = 0;
	m_nSizeLimit = 0;
	m_nBlockLength = 1024;
	m_bCanDelete = 1;
}

CMemBuf::CMemBuf(const void* pAdr, u_int size)
{
	m_buffer = NULL;
	m_nSize = 0;
	m_nRealSize = 0;
	m_nSizeLimit = 0;
	m_nBlockLength = 1024;
	SetBuffer(pAdr, size);
	m_bCanDelete = 1;
}

CMemBuf::CMemBuf(const CMemBuf& srcBuf)
{
	m_buffer = srcBuf.m_buffer;
	m_nSize = srcBuf.m_nSize;
	m_nRealSize = srcBuf.m_nRealSize;
	m_nSizeLimit = 0;
	m_nBlockLength = srcBuf.m_nBlockLength;
	m_bCanDelete = 1;
}

CMemBuf::CMemBuf(CMemBuf &srcBuf, int)
{
	m_buffer = NULL;
	m_nSize = 0;
	m_nRealSize = 0;
	m_nBlockLength = 1024;
	m_nSizeLimit = 0;
	SetBuffer(srcBuf.m_buffer, srcBuf.m_nSize);
	m_bCanDelete = 1;
}

u_int CMemBuf::Find(char c, u_int start)
{
#ifdef ASSERT
	ASSERT(m_buffer != NULL);
#endif
	if (start >= m_nSize)
		return (u_int) -1;
	void* pFound = memchr(m_buffer + start, c, m_nSize - start);

	return (pFound == NULL) ? (u_int) -1 : ((char*) pFound - m_buffer);
}

u_int CMemBuf::Find(const void* pAdr, u_int size, u_int start)
{
	if (start >= m_nSize)
		return (u_int)-1;

	char first = *((const char*) pAdr);
	char* pLast;
	const char* pPrev = m_buffer + start;
	u_int size_buf = m_nSize - start;
	const char* pEnd = m_buffer + m_nSize;
	while ((pLast = (char*) memchr(pPrev, first, size_buf)) != NULL)
	{
		size_buf -= (pLast - pPrev);
		pPrev = pLast + 1;

		if (pLast + size >= pEnd)
			return (u_int)-1;
		if (memcmp(pLast, pAdr, size) == 0)
			return (pLast - m_buffer);
	}

	return (u_int)-1;
}

void CMemBuf::AddBuffer(const void* pAdr, u_int size)
{
	if (size == 0)
		return;
#ifdef DONT_MATCH_LENGTH
	u_int nNewSize = m_nSize + size;
	if (nNewSize > m_nRealSize)
	{
		if (nNewSize - m_nRealSize < m_nBlockLength)
			SetLength(m_nRealSize + m_nBlockLength);
		else
			SetLength(nNewSize);
	}
	const u_int nAllowedSize = m_nRealSize - m_nSize;
	if (nAllowedSize == 0)
		return;
	if (size > nAllowedSize)
	{
		size = nAllowedSize;
		memcpy(m_buffer + m_nSize, pAdr, size);
		m_nSize = m_nRealSize;
	}
	else
	{
		memcpy(m_buffer + m_nSize, pAdr, size);
		m_nSize = nNewSize;
	}
#else
	u_int fs = m_nSize + size;
	if (m_nSizeLimit != 0 && fs > m_nSizeLimit)
	{
		size = m_nSizeLimit - m_nSize;
		fs = m_nSizeLimit;
	}

	char* temp = mem_alloc(fs);
	memcpy(temp, m_buffer, m_nSize);
	memcpy(temp + m_nRealSize, pAdr, size);
	mem_free(m_buffer);
	m_buffer = temp;
	m_nRealSize = fs;
#endif
	return;
}

int CMemBuf::SetBuffer(const void* pAdr, u_int size)
{
#ifdef DONT_MATCH_LENGTH
	if (size <= m_nRealSize)
	{
		memcpy(m_buffer, pAdr, size);
		m_nSize = size;
	}
	else
	{
		if (size > m_nSizeLimit)
			size = m_nSizeLimit;
		AllocBuffer(size);
		memcpy(m_buffer, pAdr, size);
		m_nSize = m_nRealSize = size;
	}
#else
	ASSERT(size != 0);
	AllocBuffer(size);
	memcpy(m_buffer, pAdr, size);
#endif
	return 1;
}

void* CMemBuf::GetBufferSetLength(u_int newSize)
{
	SetLength(newSize);
	return m_buffer;
}

int CMemBuf::GetPart(u_int nStart, u_int nEnd, CMemBuf& result)
{
	if (nEnd > m_nSize)
		nEnd = m_nSize;

	if (nStart == nEnd)
	{
#ifdef DONT_MATCH_LENGTH
		result.m_nSize = 0;
#else
		result.Free();
#endif
		return 1;
	}

	result.SetBuffer(m_buffer + nStart, nEnd - nStart);
	return 1;
}

int CMemBuf::GetPartAsString(u_int nStart, u_int nEnd, CMemBuf& result)
{
	if (nEnd > m_nRealSize)
		nEnd = m_nRealSize;

	if (nStart == nEnd)
	{
#ifdef DONT_MATCH_LENGTH
		result.m_nSize = 0;
		result[0] = '\0';
#else
		result.Free();
#endif
		return 1;
	}

	const u_int lg = nEnd - nStart;
	char* buf = (char*) result.GetBufferSetLength(lg + 1);
	memcpy(buf, m_buffer + nStart,  lg);
	buf[lg] = '\0';

	return 1;
}

void CMemBuf::SetLength(u_int newSize)
{
#ifndef DONT_MATCH_LENGTH
	if (newSize == 0)
		return;
#endif

	if (newSize == m_nRealSize)
		return;

	if (newSize < m_nRealSize)
	{
#ifdef DONT_MATCH_LENGTH
		m_nSize = newSize;
#else
		ASSERT(m_buffer != NULL);
		char* temp = mem_alloc(newSize);
		memcpy(temp, m_buffer, newSize);
		if (m_bCanDelete)
			mem_free(m_buffer);
		m_buffer = temp;
		m_nRealSize = m_nSize = newSize;
#endif
		return;
	}

	if (newSize > m_nRealSize)
	{
		if (m_nSizeLimit != 0 && newSize > m_nSizeLimit)
		{
			newSize = m_nSizeLimit;
			if (newSize == m_nRealSize)
				return;
		}

		if (m_buffer == NULL || m_nRealSize == 0)
		{
			AllocBuffer(newSize);
			return;
		}

		char* temp = mem_alloc(newSize);
		memcpy(temp, m_buffer, m_nRealSize);
		if (m_bCanDelete)
			mem_free(m_buffer);
		m_buffer = temp;
		m_nRealSize = newSize;
#ifndef DONT_MATCH_LENGTH
		m_nSize = newSize;
#endif
		return;
	}
}

// Static conversion functions

CMemBuf CMemBuf::Hex(const void* pAdr, u_int nSize)
{
	CMemBuf hexFinal;
	hexFinal.m_bCanDelete = 0;
	const u_int nFinalSize = nSize * 2;
	hexFinal.SetLength(nFinalSize + 1);
	const char* hex_chars = "0123456789abcdef";
	for (u_int i = 0; i < nSize; i++)
	{
		const unsigned char c = *((unsigned char*) pAdr + i);
		hexFinal << hex_chars[c >> 4] << hex_chars[c & 15];
	}
	hexFinal << '\0';
	hexFinal.m_nSize = nFinalSize;
	return hexFinal;
}

CMemBuf CMemBuf::Hash_MD5(const void* pAdr, u_int nSize)
{
	CMemBuf mem_MD5;
	mem_MD5.m_bCanDelete = 0;
	mem_MD5.SetLength(16);
	MYSERVER_MD5Context ctx;
	MYSERVER_MD5Init(&ctx);
	MYSERVER_MD5Update(&ctx, (unsigned char*) pAdr, nSize);
	MYSERVER_MD5Final((unsigned char*) mem_MD5.GetBuffer(), &ctx);
	mem_MD5.m_nSize = 16;
	return mem_MD5;
}

// Used by the CRC algorithm

u_int  CMemBuf::crc32Table[256] =
{
	0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
	0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
	0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
	0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
	0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
	0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
	0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
	0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
	0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
	0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
	0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
	0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
	0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
	0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
	0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
	0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,

	0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
	0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
	0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
	0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
	0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
	0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
	0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
	0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
	0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
	0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
	0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
	0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
	0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
	0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
	0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
	0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,

	0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
	0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
	0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
	0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
	0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
	0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
	0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
	0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
	0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
	0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
	0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
	0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
	0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
	0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
	0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
	0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,

	0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
	0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
	0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
	0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
	0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
	0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
	0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
	0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
	0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
	0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
	0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
	0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
	0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
	0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
	0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
	0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};
		 
CMemBuf CMemBuf::Hash_CRC(const void* pAdr, u_int nSize)
{
	CMemBuf membuf;
	membuf.m_bCanDelete = 0;
	membuf.SetLength(4);
	u_int* nCrc32 = (u_int*) membuf.GetBuffer();
	*nCrc32 = 0xFFFFFFFF;
	for (u_int i = 0; i < nSize; i++)
	{
		const unsigned char byte = *((unsigned char*) pAdr + i);
		*nCrc32 = ((*nCrc32) >> 8) ^ crc32Table[(byte) ^ ((*nCrc32) & 0x000000FF)];
	}
	*nCrc32 = ~(*nCrc32);
	membuf.m_nSize = 4;
	return membuf;
}

CMemBuf CMemBuf::XIntToStr(u_int i, int bNegative)
{
	CMemBuf strFinal;
	strFinal.m_bCanDelete = 0;
	if (i == 0) // log10(0) won't work !!
	{
		strFinal.SetBuffer("0", 2);
		strFinal.m_nSize = 1;
		return strFinal;
	}
#ifdef DONT_MATCH_LENGTH
	strFinal.SetLength(12);
#else
	strFinal.SetLength((u_int) log10(i) + 3);
#endif
	do
	{
		strFinal << (char) ('0' + i % 10);
		i /= 10;
	}
	while (i > 0);
	if (bNegative)
		strFinal << '-';
	// Here, we got the string but in reverse order.
	// So, we reverse it :)

	char *pFirst = strFinal.m_buffer;
	char *pLast = strFinal.m_buffer + strFinal.m_nSize - 1;
	char temp;
	do
	{
		temp = *pLast;
		*pLast = *pFirst;
		*pFirst = temp;
		pFirst++;
		pLast--;
	}
	while (pFirst < pLast);
	strFinal << end_str;
	strFinal.m_nSize--;
	return strFinal;
}




CMemBuf::~CMemBuf() 
{
	if (m_buffer != NULL && m_bCanDelete) 
		delete [] m_buffer;
	};
	
void CMemBuf::AddBuffer(CMemBuf *nmb) 
{
	AddBuffer(nmb->m_buffer, nmb->m_nSize);
}

int CMemBuf::Free() 
{
	if(m_buffer != NULL && m_bCanDelete) 
		mem_free(m_buffer); 
	m_buffer = NULL; 
	m_nSize = m_nRealSize = 0; 
	return 1;
};
u_int CMemBuf::Find(CMemBuf *smb, u_int start) 
{
	return Find(smb->m_buffer, smb->m_nSize, start);
};
char& CMemBuf::GetAt(u_int nIndex) 
{
#ifdef ASSERT
	ASSERT(m_buffer != NULL); 
	ASSERT(nIndex <= m_nSize); 
#endif
	return *(m_buffer + nIndex);
};
char& CMemBuf::operator[](u_int nIndex) 
{
	return GetAt(nIndex);
};

u_int CMemBuf::GetLength() 
{
	return m_nSize;
}

int CMemBuf::IsValid() 
{
	return ((m_nSize != 0) || (m_buffer != NULL))?1:0;
}

const void* CMemBuf::GetBuffer() 
{
	return (const void*) m_buffer;
}

CMemBuf::operator const void*()
{
	return (const void*) m_buffer;
}
CMemBuf CMemBuf::operator+ (CMemBuf& src) 
{	
	CMemBuf temp(*this); 
	temp.AddBuffer(&src); 
	return temp;
}
CMemBuf CMemBuf::operator+ (const char* src) 
{
	CMemBuf temp(*this); 
	temp.AddBuffer((const void*) src, strlen(src)); 
	return temp;
}
const CMemBuf& CMemBuf::operator+= (CMemBuf& add) 
{
	AddBuffer(&add); 
	return *this;
}
const CMemBuf& CMemBuf::operator+= (const char* pStr) 
{
	AddBuffer(pStr, strlen(pStr)); 
	return *this;
}
const CMemBuf& CMemBuf::operator+= (char c) 
{
	AddBuffer(&c, 1); 
	return *this;
}
CMemBuf& CMemBuf::operator<< (const char* pSrc) 
{
	AddBuffer(pSrc, strlen(pSrc)); 
	return *this;
}
CMemBuf& CMemBuf::operator<< (int i) 
{
	AddBuffer(&i, 4); 
	return *this;
}
CMemBuf& CMemBuf::operator<< (unsigned int i) 
{
	AddBuffer(&i, 4); 
	return *this;
}
CMemBuf& CMemBuf::operator<< (long i) 
{
	AddBuffer(&i, 4); 
	return *this;
}
CMemBuf& CMemBuf::operator<< (unsigned long i) 
{
	AddBuffer(&i, 4); 
	return *this;
}
CMemBuf& CMemBuf::operator<< (char c) 
{
	AddBuffer(&c, 1); 
	return *this;
}
CMemBuf& CMemBuf::operator<< (const  CMemBuf &src) 
{
	AddBuffer(src.m_buffer, src.m_nSize); 
	return *this;
}
CMemBuf& CMemBuf::operator=(CMemBuf& src) 
{
	SetBuffer(src.m_buffer, src.m_nRealSize); 
	return *this;
}
CMemBuf& CMemBuf::operator=(const char* src) 
{
	SetBuffer((const void*) src, strlen(src) + 1); 
	return* this;
}
	
CMemBuf CMemBuf::u_intToStr(u_int i) 
{
	return XIntToStr(i, 0);
}
CMemBuf CMemBuf::IntToStr(int i) 
{
	if (i < 0) 
		return XIntToStr((u_int)(-i), 1); 
	else 
		return XIntToStr((u_int) i, 0);
}

CMemBuf CMemBuf::Hex(CMemBuf& membuf)
{
	return Hex(membuf.m_buffer, membuf.m_nSize);
}
CMemBuf CMemBuf::Hash_MD5(CMemBuf& membuf) 
{
	return Hash_MD5(membuf.m_buffer, membuf.m_nSize);
}
CMemBuf CMemBuf::Hash_CRC(CMemBuf& membuf) 
{
	return Hash_CRC(membuf.m_buffer, membuf.m_nSize);
}

void CMemBuf::SetExternalBuffer(const void* pAdr, u_int size)
{
	Free();
	m_bCanDelete = false;
	m_buffer = (char*) pAdr;
	m_nRealSize = m_nSize = size;
} 

void CMemBuf::AllocBuffer(u_int size)
{
	Free(); 
	m_buffer = mem_alloc(size); 
	m_nRealSize = size;
}