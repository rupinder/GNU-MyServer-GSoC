/* classes: h_files */

#ifndef RXALLH
#define RXALLH
/*	Copyright (C) 1995, 1996 Tom Lord
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, 59 Temple Place - Suite 330, 
 * Boston, MA 02111-1307, USA. 
 */
#ifndef __STDC__
#define __STDC__
#endif


#ifdef WIN32

#include <stdlib.h>
#include <string.h>
#include "rx.h"
#include "malloc.h"
extern int sscanf(const char *buffer, const char *format,...);

#endif


#ifdef __STDC__

#else /* STDC */

#endif /* STDC */


#ifdef WIN32

#ifndef isprint
#define 	_Z   0x00 /* nothing but 0 */
#define 	_U   0x01 /* upper */
#define 	_L   0x02 /* lower */
#define 	_D   0x04 /* digit */
#define 	_C   0x08 /* cntrl */
#define 	_P   0x10 /* punct */
#define 	_S   0x20 /* white space */
#define 	_X   0x40 /* hex digit */
#define 	_SP   0x80 /* hard space x20 */

#define isalnum(c) ((__ctype+1)[c]&(_U|_L|_D))
#define isalpha(c) ((__ctype+1)[c]&(_U|_L))
#define iscntrl(c) ((__ctype+1)[c]&(_C))
#define isdigit(c) ((__ctype+1)[c]&(_D))
#define isgraph(c) ((__ctype+1)[c]&(_P|_U|_L|_D))
#define islower(c) ((__ctype+1)[c]&(_L))
#define isprint(c) ((__ctype+1)[c]&(_P|_U|_L|_D|_SP))
#define ispunct(c) ((__ctype+1)[c]&(_P))
#define isspace(c) ((__ctype+1)[c]&(_S))
#define isupper(c) ((__ctype+1)[c]&(_U))
#define isxdigit(c) ((__ctype+1)[c]&(_D|_X))

#define isascii(c) (((unsigned) c)<=0x7f)
#define toascii(c) (((unsigned) c)&0x7f)

#define tolower(c) (__ctmp=c,isupper(__ctmp)?__ctmp-('A'-'a'):__ctmp)
#define toupper(c) (__ctmp=c,islower(__ctmp)?__ctmp-('a'-'A'):__ctmp)
/*
 *Defined in rx.c
 */
extern unsigned char __ctype[];
extern char __ctmp;

#endif

#endif


#endif  /* RXALLH */
