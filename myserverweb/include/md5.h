/*
 * This code implements the MD5 message-digest algorithm.
 * The algorithm is due to Ron Rivest.  This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 *
 * To compute the message digest of a chunk of bytes, declare an
 * MD5Context structure, pass it to MD5Init, call MD5Update as
 * needed on buffers full of bytes, and then call MD5Final, which
 * will fill a supplied 16-byte array with the digest.
 */
#ifndef MD5_H
#define MD5_H

#define MD5_BYTES 16

#ifdef __alpha
typedef unsigned int uint32;
#else
typedef unsigned long uint32;
#endif

struct MYSERVER_MD5Context {
	uint32 buf[4];
	uint32 bits[2];
	unsigned char in[64];
};

void MYSERVER_MD5Init(MYSERVER_MD5Context *context);
void MYSERVER_MD5Update(struct MYSERVER_MD5Context *context, unsigned char const *buf,unsigned len);
void MYSERVER_MD5Final(unsigned char digest[MD5_BYTES], struct MYSERVER_MD5Context *context);
void MYSERVER_MD5Transform(uint32 buf[4], uint32 const in[16]);
char * MYSERVER_MD5End(MYSERVER_MD5Context *ctx, char *buf);
/*!
 * This is needed to make RSAREF happy on some MS-DOS compilers.
 */
#ifndef MD5_CTX
#ifdef DO_NOT_USE_SSL
typedef struct MYSERVER_MD5Context MD5_CTX;
#endif
#endif

#endif /*! !MD5_H */
