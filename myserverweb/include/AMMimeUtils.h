/*
*myServer
*Copyright (C) Rocky_10_Balboa
*This library is free software; you can redistribute it and/or
*modify it under the terms of the GNU Library General Public
*License as published by the Free Software Foundation; either
*version 2 of the License, or (at your option) any later version.

*This library is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*Library General Public License for more details.

*You should have received a copy of the GNU Library General Public
*License along with this library; if not, write to the
*Free Software Foundation, Inc., 59 Temple Place - Suite 330,
*Boston, MA  02111-1307, USA.
*/

#pragma once
char* MimeDecodeMailHeaderField(char *s);
class CBase64Utils
{
private:
	int ErrorCode;
public:
	int GetLastError() {return ErrorCode;};
	CBase64Utils();
	~CBase64Utils();
	char* Decode(char *input, int *bufsize);
	char* Encode(char *input, int bufsize);
};
class CQPUtils
{
private:
	char* ExpandBuffer(char *buffer, int UsedSize, int *BufSize, bool SingleChar = true);
	int ErrorCode;
public:
	int GetLastError() {return ErrorCode;};
	char* Decode(char *input);
	char* Encode(char *input);
	CQPUtils();
	~CQPUtils();
}; 

