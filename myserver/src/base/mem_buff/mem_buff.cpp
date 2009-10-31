/*
MyServer
Copyright (C) 2002, 2003, 2004, 2008, 2009 Free Software Foundation, Inc.
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

#include <include/base/mem_buff/mem_buff.h>
#include <include/base/crypt/md5.h>

#include <string.h>

#ifndef DONT_MATCH_LENGTH
# include <math.h> // for the log10 function
#endif

// Used by the CRC algorithm

u_int  MemBuf::crc32Table[256] =
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

/*! Default constructor */
MemBuf::MemBuf ()
{
  m_buffer = NULL;
  m_nSize = 0;
  m_nRealSize = 0;
  m_nSizeLimit = 0;
  m_nBlockLength = 1024;
  m_bCanDelete = 1;
}

/*!
Copy constructor
pAdr : pointer to the buffer to copy
size : size of pAdr
*/
MemBuf::MemBuf (const void* pAdr, u_int size)
{
  m_buffer = NULL;
  m_nSize = 0;
  m_nRealSize = 0;
  m_nSizeLimit = 0;
  m_nBlockLength = 1024;
  setBuffer (pAdr, size);
  m_bCanDelete = 1;
}

/*! Direct copy constructor (should only be used by the operators to avoid reallocations) */
MemBuf::MemBuf (const MemBuf& srcBuf)
{
  m_buffer = srcBuf.m_buffer;
  m_nSize = srcBuf.m_nSize;
  m_nRealSize = srcBuf.m_nRealSize;
  m_nSizeLimit = 0;
  m_nBlockLength = srcBuf.m_nBlockLength;
  m_bCanDelete = 0;
}

/*! Copy constructor */
MemBuf::MemBuf (MemBuf &srcBuf, int)
{
  m_buffer = NULL;
  m_nSize = 0;
  m_nRealSize = 0;
  m_nBlockLength = 1024;
  m_nSizeLimit = 0;
  setBuffer (srcBuf.m_buffer, srcBuf.m_nSize);
  m_bCanDelete = 1;
}

/*! find a specific character in the internal buffer from a specific location */
u_int MemBuf::find (char c, u_int start)
{
#ifdef ASSERT
  ASSERT (m_buffer != NULL);
#endif
  if (start >= m_nSize)
    return (u_int) -1;
  void* pFound = memchr (m_buffer + start, c, m_nSize - start);

  return (pFound == NULL) ? (u_int) -1 : ((char*) pFound - m_buffer);
}

/*! find a specific buffer in the internal buffer from a specific location */
u_int MemBuf::find (const void* pAdr, u_int size, u_int start)
{
  if (start >= m_nSize)
    return (u_int)-1;

  char first = *((const char*) pAdr);
  char* pLast;
  const char* pPrev = m_buffer + start;
  u_int size_buf = m_nSize - start;
  const char* pEnd = m_buffer + m_nSize + 1;
  while ((pLast = (char*) memchr (pPrev, first, size_buf)) != NULL)
  {
    size_buf -= (u_int)(pLast - pPrev);
    pPrev = pLast + 1;

    if (pLast + size >= pEnd)
      return (u_int)-1;
    if (memcmp (pLast, pAdr, size) == 0)
      return (u_int)(pLast - m_buffer);
  }

  return (u_int)-1;
}

/*! replace a character by another */

void MemBuf::replace (char what, char by)
{
  const char* pEnd = m_buffer + m_nSize;
  char *buf_c = m_buffer;
  for ( ; buf_c < pEnd; buf_c++)
  {
    if (*buf_c == what)
      *buf_c = by;
  }
}

/*!
Add a buffer at the end of the internal buffer.
If the internal buffer isn't large enough, a reallocation is done until m_nSizeLimit is reached
If m_nSizeLimit is equal to 0, reallocation are always done.
*/
void MemBuf::addBuffer (const void* pAdr, u_int size)
{
  if (size == 0)
    return;
#ifdef DONT_MATCH_LENGTH
  u_int nNewSize = m_nSize + size;
  if (nNewSize > m_nRealSize)
  {
    if (nNewSize - m_nRealSize < m_nBlockLength)
      setLength (m_nRealSize + m_nBlockLength);
    else
      setLength (nNewSize);
  }
  const u_int nAllowedSize = m_nRealSize - m_nSize;
  if (nAllowedSize == 0)
    return;
  if (size > nAllowedSize)
  {
    size = nAllowedSize;
    memcpy (m_buffer + m_nSize, pAdr, size);
    m_nSize = m_nRealSize;
  }
  else
  {
    memcpy (m_buffer + m_nSize, pAdr, size);
    m_nSize = nNewSize;
  }
#else
  u_int fs = m_nSize + size;
  if (m_nSizeLimit != 0 && fs > m_nSizeLimit)
  {
    size = m_nSizeLimit - m_nSize;
    fs = m_nSizeLimit;
  }

  char* temp = mem_alloc (fs);
  memcpy (temp, m_buffer, m_nSize);
  memcpy (temp + m_nRealSize, pAdr, size);
  mem_free (m_buffer);
  m_buffer = temp;
  m_nRealSize = fs;
#endif
  return;
}

int MemBuf::setBuffer (const void* pAdr, u_int size)
{
#ifdef DONT_MATCH_LENGTH
  if (size <= m_nRealSize)
  {
    memcpy (m_buffer, pAdr, size);
    m_nSize = size;
  }
  else
  {
    if (m_nSizeLimit != 0 && size > m_nSizeLimit)
      size = m_nSizeLimit;
    allocBuffer (size);
    memcpy (m_buffer, pAdr, size);
    m_nSize = m_nRealSize = size;
  }
#else
  ASSERT (size != 0);
  allocBuffer (size);
  memcpy (m_buffer, pAdr, size);
#endif
  return 1;
}

/*!
Set an external buffer as the internal buffer.
No memory copy are done.
This can be useful to use variables from the stack.
Ex.:
unsigned char stackBuffer[10];
MemBuf buffer;
buffer.setExternalBuffer (stackBuffer, 10); // here, you'll use the memory from stackBuffer
*/
void MemBuf::setExternalBuffer (const void* pAdr, u_int size)
{
  free ();
  m_bCanDelete = false;
  m_buffer = (char*) pAdr;
  m_nRealSize = size;
  m_nSize = 0;
}

/*! Return the internal buffer by setting previously a specific length */
char* MemBuf::getBuffersetLength (u_int newSize)
{
  setLength (newSize);
  return (char*)m_buffer;
}

/*!
Copy a part of the internal buffer in "result".
Here, "result" is completly removed and replaced by this part.
*/
int MemBuf::getPart (u_int nStart, u_int nEnd, MemBuf& result)
{
  if (nEnd > m_nSize)
    nEnd = m_nSize;

  if (nStart == nEnd)
  {
#ifdef DONT_MATCH_LENGTH
    result.m_nSize = 0;
#else
    result.free ();
#endif
    return 1;
  }

  result.setBuffer (m_buffer + nStart, nEnd - nStart);
  return 1;
}

/*!
Copy a part of the internal buffer in "result".
Here, "result" is completly removed and replaced by this part.
*/
int MemBuf::getPartAsString (u_int nStart, u_int nEnd, MemBuf& result)
{
  if (nEnd > m_nRealSize)
    nEnd = m_nRealSize;

  if (nStart == nEnd)
  {
#ifdef DONT_MATCH_LENGTH
    result.m_nSize = 0;
    result[0] = '\0';
#else
    result.free ();
#endif
    return 1;
  }

  const u_int lg = nEnd - nStart;
  char* buf = (char*) result.getBuffersetLength (lg + 1);
  memcpy (buf, m_buffer + nStart,  lg);
  buf[lg] = '\0';

  return 1;
}

/*!
Set the length of the internal buffer.
If the length is smallest than the existing one, no reallocation is done.
if it's biggest, a reallocation is done until m_nSizeLimit is reached.
*/
void MemBuf::setLength (u_int newSize)
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
    ASSERT (m_buffer != NULL);
    char* temp = mem_alloc (newSize);
    memcpy (temp, m_buffer, newSize);
    if (m_bCanDelete)
      mem_free (m_buffer);
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
      allocBuffer (newSize);
      return;
    }

    char* temp = mem_alloc (newSize);
    memcpy (temp, m_buffer, m_nRealSize);
    if (m_bCanDelete)
      mem_free (m_buffer);
    m_buffer = temp;
    m_nRealSize = newSize;
#ifndef DONT_MATCH_LENGTH
    m_nSize = newSize;
#endif
    return;
  }
}

// hex <-> Data conversion functions

/*! Write to the MemBuf the hex representation of pAdr */
void MemBuf::hex (const void* pAdr, u_int nSize)
{
  const u_int nFinalSize = nSize * 2;
  setLength (nFinalSize + 1);
  const char* hex_chars = "0123456789abcdef";
  for (u_int i = 0; i < nSize; i++)
  {
    const unsigned char c = *((unsigned char*) pAdr + i);
    *this << hex_chars[c >> 4] << hex_chars[c & 15];
  }
  *this << '\0';
  m_nSize = nFinalSize;
}

/*! Return the decimal number of an hexadecimal character */
unsigned char MemBuf::hexCharToNumber (unsigned char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 87; // 'a' + 10
  if (c >= 'A' && c <= 'F')
    return c - 55; // 'A' + 10
  return 0;
}

/*! Return a MemBuf with the decoded hexadicmal string pointed at pAdr */
MemBuf MemBuf::hexToData (const void* pAdr, u_int nSize)
{
  if ((nSize & 1) == 1) // nSize impair
    return MemBuf ("", 1);
  MemBuf memFinal;
  memFinal.m_bCanDelete = false;
  memFinal.setLength (nSize >> 1);
  const char* pTmp = (const char*) pAdr;
  const char* pEnd = pTmp + nSize;
  for ( ; pTmp < pEnd; pTmp += 2)
    memFinal << (unsigned char) ((hexCharToNumber (*pTmp) << 4) + hexCharToNumber (*(pTmp + 1)));
  return memFinal;
}

/*! MD5 hashing function */
void MemBuf::hashMD5(const void* pAdr, u_int nSize)
{
  Md5 md5;
  setLength (16);

  md5.init ();
  md5.update ((char*) pAdr, nSize);
  md5.end ((char*) getBuffer ());
  m_nSize = 16;
}

/*! CRC hashing function */
void MemBuf::hashCRC (const void* pAdr, u_int nSize)
{
  setLength (4);
  u_int* nCrc32 = (u_int*) getBuffer ();
  *nCrc32 = 0xFFFFFFFF;
  for (u_int i = 0; i < nSize; i++)
  {
    const unsigned char byte = *((unsigned char*) pAdr + i);
    *nCrc32 = ((*nCrc32) >> 8) ^ crc32Table[(byte) ^ ((*nCrc32) & 0x000000FF)];
  }
  *nCrc32 = ~(*nCrc32);
  m_nSize = 4;
}

// Int <-> Str conversion functions

/*! Return a MemBuf with the string representation of "i" */
void MemBuf::xIntToStr (u_int i, int bNegative)
{
  if (i == 0) // log10(0) won't work !!
  {
    setBuffer ("0", 2);
    m_nSize = 1;
    return;
  }
#ifdef DONT_MATCH_LENGTH
  setLength (12);
#else
  setLength ((u_int) log10(i) + 3);
#endif
  do
  {
    *this << (char) ('0' + i % 10);
    i /= 10;
  }
  while (i > 0);
  if (bNegative)
    *this << '-';
  // Here, we got the string but in reverse order.
  // So, we reverse it :)

  char *pFirst = m_buffer;
  char *pLast = m_buffer + m_nSize - 1;
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
  *this << end_str;
  m_nSize--;
}

/*! Return a MemBuf with the string representation of "i" using an external buffer. */
void MemBuf::xIntToStr (u_int i, int bNegative, char* pBufToUse, u_int nBufSize)
{
  m_nSizeLimit = nBufSize;
  setExternalBuffer (pBufToUse, nBufSize);
  if (i == 0) // log10(0) won't work !!
  {
    setBuffer ("0", 2);
    m_nSize = 1;
    return;
  }

  do
  {
    *this << (char) ('0' + i % 10);
    i /= 10;
  }
  while (i > 0);
  if (bNegative)
    *this << '-';
  // Here, we got the string but in reverse order.
  // So, we reverse it :)

  char *pFirst = pBufToUse;
  char *pLast = pBufToUse + m_nSize - 1;
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
  *this << end_str;
  m_nSize--;
}

/* Convert a string into an unsigned number */
u_int MemBuf::strToUint (const char* pAdr)
{
  int nSize = (int)strlen (pAdr);
  if (nSize > 10) // a (signed/unsigned) 32-bit number as a maximun of 10 digit
    nSize = 10;
  u_int nRes = 0;
  u_int pow10[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};
  while (nSize-- > 0)
  {
    nRes += (u_int) ((*pAdr - '0')) * pow10[nSize];
    pAdr++;
  }
  return nRes;
}

/* Convert a string into a signed number */
int MemBuf::strToInt (const char* pAdr)
{
  if (*pAdr == '-')
    return (int) (-(int) (strToUint (++pAdr)));
  return (int) strToUint (pAdr);
}

/*! Destructor */
MemBuf::~MemBuf ()
{
  if (m_buffer != NULL && m_bCanDelete)
    mem_free (m_buffer);
}

void MemBuf::addBuffer (MemBuf *nmb)
{
  addBuffer (nmb->m_buffer, nmb->m_nSize);
}

/*! free used memory */
int MemBuf::free ()
{
  if (m_buffer != NULL && m_bCanDelete)
  {
    mem_free (m_buffer);
    m_buffer = NULL;
    m_nSize = m_nRealSize = 0;
    return 0;
  }
  return 1;
}
/*!
*Get the real allocated size.
*/
u_int MemBuf::getRealLength ()
{
  return m_nRealSize;
}

u_int MemBuf::find (MemBuf *smb, u_int start)
{
  return find (smb->m_buffer, smb->m_nSize, start);
}
char& MemBuf::getAt (u_int nIndex)
{
#ifdef ASSERT
  ASSERT (m_buffer != NULL);
  ASSERT (nIndex <= m_nSize);
#endif
  return *(m_buffer + nIndex);
}
char& MemBuf::operator[](u_int nIndex)
{
  return getAt (nIndex);
}

u_int MemBuf::getLength ()
{
  return m_nSize;
}

int MemBuf::isValid ()
{
  return ((m_nSize != 0) || (m_buffer != NULL))?1:0;
}

char* MemBuf::getBuffer ()
{
  return ( char*) m_buffer;
}

MemBuf::operator const void*()
{
  return (const void*) m_buffer;
}
MemBuf MemBuf::operator+ (MemBuf& src)
{
  MemBuf temp (*this);
  temp.addBuffer (&src);
  return temp;
}
MemBuf MemBuf::operator+ (const char* src)
{
  MemBuf temp (*this);
  temp.addBuffer ((const void*) src, (u_int)strlen (src));
  return temp;
}
const MemBuf& MemBuf::operator+= (MemBuf& add)
{
  addBuffer (&add);
  return *this;
}
const MemBuf& MemBuf::operator+= (const char* pStr)
{
  addBuffer (pStr, (u_int)strlen (pStr));
  return *this;
}
const MemBuf& MemBuf::operator+= (char c)
{
  addBuffer (&c, 1);
  return *this;
}
MemBuf& MemBuf::operator<< (const char* pSrc)
{
  addBuffer (pSrc, (u_int)strlen (pSrc));
  return *this;
}
MemBuf& MemBuf::operator<< (int i)
{
  addBuffer (&i, 4);
  return *this;
}
MemBuf& MemBuf::operator<< (unsigned int i)
{
  addBuffer (&i, 4);
  return *this;
}
MemBuf& MemBuf::operator<< (long i)
{
  addBuffer (&i, 4);
  return *this;
}
MemBuf& MemBuf::operator<< (unsigned long i)
{
  addBuffer (&i, 4);
  return *this;
}
MemBuf& MemBuf::operator<< (char c)
{
  addBuffer (&c, 1);
  return *this;
}
MemBuf& MemBuf::operator<< (unsigned char c)
{
  addBuffer (&c, 1);
  return *this;
}
MemBuf& MemBuf::operator<< (const  MemBuf &src)
{
  addBuffer (src.m_buffer, src.m_nSize);
  return *this;
}
MemBuf& MemBuf::operator<< (const string &src)
{
  addBuffer ((const void*) src.c_str (), src.length ());
  return *this;
}
MemBuf& MemBuf::operator=(const MemBuf& src)
{
  setBuffer (src.m_buffer, src.m_nRealSize);
  return *this;
}
MemBuf& MemBuf::operator=(const char* src)
{
  setBuffer ((const void*) src, (u_int)strlen (src) + 1);
  return* this;
}

void MemBuf::uintToStr (u_int i)
{
  xIntToStr (i, 0);
}

void MemBuf::uintToStr (u_int i, char* pBufToUse, u_int nBufSize)
{
  xIntToStr (i, 0, pBufToUse, nBufSize);
}

void MemBuf::intToStr (int i)
{
  if (i < 0)
    xIntToStr ( (u_int)(-i), 1);
  else
    xIntToStr ( (u_int) i, 0);
}

void MemBuf::intToStr (int i, char* pBufToUse, u_int nBufSize)
{
  if (i < 0)
    xIntToStr ((u_int)(-i), 1, pBufToUse, nBufSize);
  else
    xIntToStr ((u_int) i, 0, pBufToUse, nBufSize);
}

void MemBuf::hex (MemBuf& membuf)
{
  hex (membuf.m_buffer, membuf.m_nSize);
}
void MemBuf::hashMD5(MemBuf& membuf)
{
  hashMD5(membuf.m_buffer, membuf.m_nSize);
}
void MemBuf::hashCRC (MemBuf& membuf)
{
  hashCRC (membuf.m_buffer, membuf.m_nSize);
}

void MemBuf::allocBuffer (u_int size)
{
  if (size > m_nRealSize || m_buffer == NULL)
  {
    free ();
    m_buffer = mem_alloc (size);
    m_nRealSize = size;
  }
}
