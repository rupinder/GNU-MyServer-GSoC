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

struct MYSERVER_MD5Context {
	unsigned int buf[4];
	unsigned int bytes[2];
	unsigned int in[16];
};

void MYSERVER_MD5Init(MYSERVER_MD5Context *context);
void MYSERVER_MD5Update(struct MYSERVER_MD5Context *context, unsigned char const *buf, unsigned long len);
void MYSERVER_MD5Final(unsigned char digest[16], struct MYSERVER_MD5Context *context);
void MYSERVER_MD5Transform(unsigned int buf[4], unsigned int const in[16]);
char * MYSERVER_MD5End(MYSERVER_MD5Context *ctx, char *buf);


#endif /*! !MD5_H */
