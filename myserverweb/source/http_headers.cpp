/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
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

#include "../stdafx.h"
#include "../include/http_headers.h"
#include "../include/security.h"
#include "../include/http.h"
#include "../include/AMMimeUtils.h"
#include "../include/filemanager.h"
#include "../include/utility.h"
#include "../include/stringutils.h"
#include "../include/securestr.h"
extern "C" {
#ifdef WIN32
#include <direct.h>
#include <errno.h>
#endif
#ifdef NOT_WIN
#include <string.h>
#include <errno.h>
#endif
}

#ifdef NOT_WIN
#include "../include/lfind.h"

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

// Bloodshed Dev-C++ Helper
#ifndef intptr_t
#define intptr_t int
#endif

/*!
 *Builds an HTTP header string starting from an HTTP_RESPONSE_HEADER structure.
 */
void HttpHeaders::buildHTTPResponseHeader(char *str,HTTP_RESPONSE_HEADER* response)
{
	/*!
   *Here is builded the HEADER of a HTTP response.
   *Passing a HTTP_RESPONSE_HEADER struct this builds an header string.
   *Every directive ends with a \r\n sequence.
   */
	if(response->httpStatus!=200)
	{
		if(response->ERROR_TYPE[0]=='\0')
		{
			int errID = getErrorIDfromHTTPStatusCode(response->httpStatus);
			if(errID!=-1)
				strncpy(response->ERROR_TYPE, HTTP_ERROR_MSGS[errID], 
                HTTP_RESPONSE_ERROR_TYPE_DIM);
		}
		sprintf(str,"%s %i %s\r\nStatus: %s\r\n",response->VER,response->httpStatus, 
            response->ERROR_TYPE,response->ERROR_TYPE);
	}
	else
		sprintf(str,"%s 200 OK\r\n",response->VER);
	
	if(response->SERVER_NAME[0])
	{
		strcat(str,"Server: ");
		strcat(str,response->SERVER_NAME);
		strcat(str,"\r\n");
	}
	if(response->CACHE_CONTROL[0])
	{
		strcat(str,"Cache-Control: ");
		strcat(str,response->CACHE_CONTROL);
		strcat(str,"\r\n");
	}
	if(response->LAST_MODIFIED[0])
	{
		strcat(str,"Last-Modified: ");
		strcat(str,response->LAST_MODIFIED);
		strcat(str,"\r\n");
	}
	if(response->CONNECTION[0])
	{
		strcat(str,"Connection: ");
		strcat(str,response->CONNECTION);
		strcat(str,"\r\n");
	}
	else
	{
		strcat(str,"Connection: Close\r\n");
	}
	if(response->TRANSFER_ENCODING[0])
	{
		strcat(str,"Transfer-Encoding: ");
		strcat(str,response->TRANSFER_ENCODING);
		strcat(str,"\r\n");
	}
	if(response->CONTENT_ENCODING[0])
	{
		strcat(str,"Content-Encoding: ");
		strcat(str,response->CONTENT_ENCODING);
		strcat(str,"\r\n");
	}
	if(response->CONTENT_RANGE[0])
	{
		strcat(str,"Content-Range: ");
		strcat(str,response->CONTENT_RANGE);
		strcat(str,"\r\n");
	}
	if(response->CONTENT_LENGTH[0])
	{
		/*!
     *Do not specify the Content-Length field if it is used
     *the chunked Transfer-Encoding.
     */
		if(!strstr(response->TRANSFER_ENCODING,"chunked"))
		{
			strcat(str,"Content-Length: ");
			strcat(str,response->CONTENT_LENGTH);
			strcat(str,"\r\n");
		}
	}
	if(response->COOKIE[0])
	{
		char *token = strtok(response->COOKIE,"\n");
		do
		{
			strcat(str,"Set-Cookie: ");
			strcat(str,token);
			strcat(str,"\r\n");		
			token=strtok(NULL,"\n");
		}while(token);
	}
	if(response->P3P[0])
	{
		strcat(str,"P3P: ");
		strcat(str,response->P3P);
		strcat(str,"\r\n");
	}
	if(response->MIMEVER[0])
	{
		strcat(str,"MIME-Version: ");
		strcat(str,response->MIMEVER);
		strcat(str,"\r\n");
	}
	if(response->CONTENT_TYPE[0])
	{
		strcat(str,"Content-Type: ");
		strcat(str,response->CONTENT_TYPE);
		strcat(str,"\r\n");
	}
	if(response->DATE[0])
	{
		strcat(str,"Date: ");
		strcat(str,response->DATE);
		strcat(str,"\r\n");
	}
	if(response->DATEEXP[0])
	{
		strcat(str,"Expires: ");
		strcat(str,response->DATEEXP);
		strcat(str,"\r\n");
	}
	if(response->AUTH[0])
	{
		strcat(str,"WWW-Authenticate: ");
		strcat(str,response->AUTH);
		strcat(str,"\r\n");
	}
	
	if(response->LOCATION[0])
	{
		strcat(str,"Location: ");
		strcat(str,response->LOCATION);
		strcat(str,"\r\n");
	}

	if(response->OTHER[0])
	{
		strcat(str,response->OTHER);
	}

	/*!
   *MyServer supports the bytes range.
   */
	strcat(str,"Accept-Ranges: bytes\r\n");
  
	/*!
   *The HTTP header ends with a \r\n sequence.
   */
	strcat(str,"\r\n\0\0\0\0\0");
}
/*!
 *Set the defaults value for a HTTP_RESPONSE_HEADER structure.
 */
void HttpHeaders::buildDefaultHTTPResponseHeader(HTTP_RESPONSE_HEADER* response)
{
	resetHTTPResponse(response);
	/*!
   *By default use:
   *1) the MIME type of the page equal to text/html.
   *2) the version of the HTTP protocol to 1.1.
   *3) the date of the page and the expire date to the current time.
   *4) set the name of the server.
   *5) set the page that it is not an error page.
   */
	strcpy(response->CONTENT_TYPE,"text/html");
	strcpy(response->VER,"HTTP/1.1");
	response->httpStatus=200;
	getRFC822GMTTime(response->DATE,HTTP_RESPONSE_DATE_DIM);
	strncpy(response->DATEEXP,response->DATE,HTTP_RESPONSE_DATEEXP_DIM);
	sprintf(response->SERVER_NAME,"MyServer %s",versionOfSoftware);
}


/*!
 *Reset all the HTTP_REQUEST_HEADER structure members.
 */
void HttpHeaders::resetHTTPRequest(HTTP_REQUEST_HEADER *request)
{
	request->TRANSFER_ENCODING[0]='\0';	
	request->CONTENT_ENCODING[0]='\0';	
	request->CMD[0]='\0';		
	request->VER[0]='\0';		
	request->ACCEPT[0]='\0';
	request->AUTH[0]='\0';
	request->ACCEPTENC[0]='\0';	
	request->ACCEPTLAN[0]='\0';	
	request->ACCEPTCHARSET[0]='\0';
	request->CONNECTION[0]='\0';
	request->USER_AGENT[0]='\0';
	request->COOKIE[0]='\0';
	request->CONTENT_TYPE[0]='\0';
	request->CONTENT_LENGTH[0]='\0';
	request->DATE[0]='\0';
	request->FROM[0]='\0';
	request->DATEEXP[0]='\0';	
	request->MODIFIED_SINCE[0]='\0';
	request->LAST_MODIFIED[0]='\0';	
	request->URI[0]='\0';
	request->URIOPTS[0]='\0';
	request->URIOPTSPTR=NULL;
	request->REFERER[0]='\0';
	request->HOST[0]='\0';
	request->CACHE_CONTROL[0]='\0';
	request->IF_MODIFIED_SINCE[0]='\0';
	request->OTHER[0]='\0';
	request->PRAGMA[0]='\0';
	request->RANGETYPE[0]='\0';
	request->RANGEBYTEBEGIN=0;
	request->RANGEBYTEEND=0;
	request->uriEndsWithSlash=0;
	request->digest_realm[0]='\0';
	request->digest_opaque[0]='\0';
	request->digest_nonce[0]='\0';
	request->digest_cnonce[0]='\0';
	request->digest_method[0]='\0';
	request->digest_username[0]='\0';
	request->digest_response[0]='\0';
	request->digest_qop[0]='\0';
	request->digest_nc[0]='\0';
}

/*!
 *Reset all the HTTP_RESPONSE_HEADER structure members.
 */
void HttpHeaders::resetHTTPResponse(HTTP_RESPONSE_HEADER *response)
{
	response->httpStatus=200;
	response->VER[0]='\0';	
	response->SERVER_NAME[0]='\0';
	response->CONTENT_TYPE[0]='\0';
	response->CONNECTION[0]='\0';
	response->MIMEVER[0]='\0';
	response->P3P[0]='\0';
	response->COOKIE[0]='\0';
	response->CONTENT_LENGTH[0]='\0';
	response->ERROR_TYPE[0]='\0';
	response->CONTENT_ENCODING[0]='\0';
	response->TRANSFER_ENCODING[0]='\0';
	response->LOCATION[0]='\0';
	response->DATE[0]='\0';		
	response->AUTH[0]='\0';
	response->DATEEXP[0]='\0';	
	response->OTHER[0]='\0';
	response->LAST_MODIFIED[0]='\0';
	response->CACHE_CONTROL[0]='\0';
	response->CONTENT_RANGE[0]='\0';
}


/*!
 *Controls if the req string is a valid HTTP response header.
 *Returns 0 if req is an invalid header, a non-zero value if is a valid header.
 *nLinesptr is a value of the lines number in the HEADER.
 *ncharsptr is a value of the characters number in the HEADER.
 */
int HttpHeaders::validHTTPResponse(char *res, HttpThreadContext* td, 
                                    u_long* nLinesptr, u_long* ncharsptr)
{
	u_long i;
	u_long buffersize=td->buffersize;
	u_long nLinechars;
	int isValidCommand=0;
	nLinechars=0;
	u_long nLines=0;
	u_long maxTotchars=0;
	if(res==0)
		return 0;
	/*!
   *Count the number of lines in the header.
   */
	for(i=nLines=0;;i++)
	{
		if(res[i]=='\n')
		{
			if((res[i+2]=='\n')|(res[i+1]=='\0')|(res[i+1]=='\n'))
			{
				maxTotchars=i+3;
				if(maxTotchars>buffersize)
				{
					isValidCommand=0;
					break;
				}
				isValidCommand=1;
				break;
			}
			nLines++;
		}
		else
		{
			nLinechars++;
		}
		/*!
     *We set a maximal theorical number of characters in a line.
     *If a line contains more than 4160 characters we consider the header invalid.
     */
		if(nLinechars>4160)
    {
      isValidCommand = 0;
			break;
    }
	}

	/*!
   *Set the output variables.
   */
	*nLinesptr=nLines;
	*ncharsptr=maxTotchars;
	
	/*!
   *Return if is a valid request header.
   */
	return((isValidCommand)?1:0);
}


/*!
 *Build the HTTP REQUEST HEADER string.
 *If no input is setted the input is the main buffer of the 
 *HttpThreadContext structure.
 */
int HttpHeaders::buildHTTPRequestHeaderStruct(HTTP_REQUEST_HEADER *request, 
                                               HttpThreadContext* td, char* input)
{
	/*!
   *In this function there is the HTTP protocol parse.
   *The request is mapped into a HTTP_REQUEST_HEADER structure
   *And at the end of this every command is treated
   *differently. We use this mode for parse the HTTP
   *cause especially in the CGI is requested a continous
   *HTTP header access.
   *Before mapping the header in the structure 
   *control if this is a regular request.
   *The HTTP header ends with a \r\n\r\n sequence.
   */
  
	/*!
   *Control if the HTTP header is a valid header.
   */
	u_long i=0,j=0;
	int max=0;
	u_long nLines, maxTotchars;
	int noinputspecified=0;
	int validRequest;
	const int max_URI = HTTP_REQUEST_URI_DIM + 200 ;
	const char seps[]   = "\n\r";
	const char cmdseps[]   = ": ,\t\n\r";

	char *token=0;
	char command[96];

	int nLineControlled = 0;
	int lineControlled = 0;

	/*! TokenOff is the length of the token starting from the location token.  */
	int tokenOff;
	if(input==0)
	{
		noinputspecified=1;
		input=(char*)td->buffer->GetBuffer();
	}
	validRequest=validHTTPRequest(input,td,&nLines,&maxTotchars);
	/*! Invalid header.  */
	if(validRequest==0)
		return 0;
	/*! Incomplete header.  */
	else if(validRequest==-1)
		return -1;

  token=0;

	if(!noinputspecified)
		token=input;
	else
	{
		input=token=(char*)td->buffer->GetBuffer();
	}
	tokenOff = getCharInString(token, cmdseps, HTTP_REQUEST_CMD_DIM);
	do
	{
		if(tokenOff== -1 )
			return 0;
		
		/*! Copy the HTTP command.  */
		myserver_strlcpy(command, token, min(96, tokenOff+1) );
	
		token+=tokenOff;
		command[tokenOff]='\0';
		if(*token==':')
			token++;
		while(*token ==' ')
			token++;
		nLineControlled++;
    lineControlled = 0;
		if(nLineControlled==1)
		{
			int containOpts=0;
			/*!
       *The first line has the form:
       *GET /index.html HTTP/1.1
       */
			lineControlled=1;
			/*! Copy the method type.  */
			strncpy(request->CMD, command, tokenOff);
			request->CMD[tokenOff] = '\0';
			tokenOff = getCharInString(token, "\t\n\r", 
                                 HTTP_REQUEST_VER_DIM + HTTP_REQUEST_URI_DIM+10);
			u_long len_token =tokenOff;
			if(tokenOff==-1)
				return 0;
			max=(int)tokenOff;
			while((token[max]!=' ') && (len_token-max<HTTP_REQUEST_VER_DIM))
				max--;
			for(i=0;((int)i<max)&&(i<HTTP_REQUEST_URI_DIM);i++)
			{
				if(token[i]=='?')
				{
					containOpts=1;
					break;
				}
				request->URI[i]=token[i];
			}
			request->URI[i]='\0';

			if(containOpts)
			{
				for(j=0;((int)i<max) && (j<HTTP_REQUEST_URIOPTS_DIM-1);j++)
				{
					request->URIOPTS[j]=token[++i];
					request->URIOPTS[j+1]='\0';
				}
			}
			myserver_strlcpy(request->VER,&token[max+1],HTTP_REQUEST_VER_DIM-1);
			if(request->URI[strlen(request->URI)-1]=='/')
				request->uriEndsWithSlash=1;
			else
				request->uriEndsWithSlash=0;
			StrTrim(request->URI," /");
			StrTrim(request->URIOPTS," /");
			max=strlen(request->URI);
			if(max>max_URI)
			{
				return 414;
			}
		}else
		/*!User-Agent*/
		if(!lstrcmpi(command,"User-Agent"))
		{
			tokenOff = getCharInString(token,"\n\r",HTTP_REQUEST_USER_AGENT_DIM);
						
			if(tokenOff==-1)return 0;
			lineControlled=1;
			myserver_strlcpy(request->USER_AGENT,token,tokenOff+1);
			request->USER_AGENT[tokenOff]='\0';
			StrTrim(request->USER_AGENT," ");
		}else
		/*!Authorization*/
		if(!lstrcmpi(command,"Authorization"))
		{
			while(*token==' ')
				token++;
			tokenOff = getCharInString(token," ",HTTP_REQUEST_AUTH_DIM);
						
			if(tokenOff==-1)return 0;
		
			lineControlled=1;
	
			myserver_strlcpy(request->AUTH,token,tokenOff+1);
			request->AUTH[tokenOff]='\0';
			if(!lstrcmpi(request->AUTH,"Basic"))
			{
				u_long i;
				char *base64=&token[strlen("Basic ")];
				int len=(int)strlen(base64);
				char *tmp = base64 + len - 1;
				char* lbuffer2;
				char* keep_lbuffer2;
        char *login;
        char* password;
				while (len > 0 && (*tmp == '\r' || *tmp == '\n'))
				{
					tmp--;
					len--;
				}
				if (len <= 1)
					return 0;
				lbuffer2=base64Utils.Decode(base64,&len);
				keep_lbuffer2=lbuffer2;
        login = td->connection->getLogin();
				for(i=0;(*lbuffer2!=':') && (i<19);i++)
				{
					login[i]=*lbuffer2++;
          login[i+1]='\0';
				}
				myserver_strlcpy(td->identity,td->connection->getLogin(),32+1);
        lbuffer2++;
        password = td->connection->getPassword();
				for(i=0;(*lbuffer2)&&(i<31);i++)
				{
					password[i]=*lbuffer2++;
					password[i+1]='\0';
				}
			}
			else if(!lstrcmpi(request->AUTH,"Digest"))
			{
				char *digestBuff;
				char *digestToken;
				token+=tokenOff;
				while(*token==' ')
					token++;
				tokenOff = getCharInString(token,"\r\n",0);
				digestBuff=new char[tokenOff];
				if(!digestBuff)
					return 0;
				memcpy(digestBuff,token,tokenOff);
				digestBuff[tokenOff]='\0';
				digestToken = strtok( digestBuff, "=" );
				if(!digestToken)
					return 0;
				do
				{
					StrTrim(digestToken," ");
					if(!lstrcmpi(digestToken,"nonce"))
					{
						digestToken = strtok( NULL, "," );
						StrTrim(digestToken,"\" ");
						myserver_strlcpy(td->request.digest_nonce,digestToken,48+1);
					}
					else if(!lstrcmpi(digestToken,"opaque"))
					{
						digestToken = strtok( NULL, "," );
						StrTrim(digestToken,"\" ");
						myserver_strlcpy(td->request.digest_opaque,digestToken,48+1);
					}
					else if(!lstrcmpi(digestToken,"uri"))
					{
						digestToken = strtok( NULL, "\r\n," );
						StrTrim(digestToken,"\" ");
						myserver_strlcpy(td->request.digest_uri,digestToken,1024+1);
					}
					else if(!lstrcmpi(digestToken,"method"))
					{
						digestToken = strtok( NULL, "\r\n," );
						StrTrim(digestToken,"\" ");
						myserver_strlcpy(td->request.digest_method,digestToken,16+1);
					}	
					else if(!lstrcmpi(digestToken,"qop"))
					{
						digestToken = strtok( NULL, "\r\n," );
						StrTrim(digestToken,"\" ");
						myserver_strlcpy(td->request.digest_qop,digestToken,16+1);
					}					
					else if(!lstrcmpi(digestToken,"realm"))
					{
						digestToken = strtok( NULL, "\r\n," );
						StrTrim(digestToken,"\" ");
						myserver_strlcpy(td->request.digest_realm,digestToken,48+1);
					}
					else if(!lstrcmpi(digestToken,"cnonce"))
					{
						digestToken = strtok( NULL, "\r\n," );
						StrTrim(digestToken," \"");
						myserver_strlcpy(td->request.digest_cnonce, digestToken, 48+1);
					}
					else if(!lstrcmpi(digestToken, "username"))
					{
						digestToken = strtok( NULL, "\r\n," );
						StrTrim(digestToken, "\" ");
						myserver_strlcpy(td->request.digest_username, digestToken, 48+1);
						myserver_strlcpy(td->connection->getLogin(), digestToken, 48+1);
					}
					else if(!lstrcmpi(digestToken,"response"))
					{
						digestToken = strtok( NULL, "\r\n," );
						StrTrim(digestToken,"\" ");
						myserver_strlcpy(td->request.digest_response,digestToken,48+1);
					}
					else if(!lstrcmpi(digestToken,"nc"))
					{
						digestToken = strtok( NULL, "\r\n," );
						StrTrim(digestToken,"\" ");
						myserver_strlcpy(td->request.digest_nc,digestToken,10+1);
					}
					else 
					{
						digestToken = strtok( NULL, "\r\n," );
					}
					/*! Update digestToken. */
					digestToken = strtok( NULL, "=" );
				}while(digestToken);
				delete  [] digestBuff;
			}
		}else
		/*!Host*/
		if(!lstrcmpi(command,"Host"))
		{
      int cur = 0;
			tokenOff = getCharInString(token,seps,HTTP_REQUEST_HOST_DIM);
			if(tokenOff==-1)
        return 0;
			lineControlled=1;
			myserver_strlcpy(request->HOST,token,tokenOff+1);
			request->HOST[tokenOff]='\0';
      /*!
       *Do not save the port if specified.
       */
      while(request->HOST[cur])
      {
        if(request->HOST[cur]==':')
        {
          request->HOST[cur]='\0';
          break;
        }
        cur++;
      }
			StrTrim(request->HOST," ");
		}else
		/*!Content-Encoding*/
		if(!lstrcmpi(command,"Content-Encoding"))
		{
			tokenOff = getCharInString(token,seps,HTTP_REQUEST_CONTENT_ENCODING_DIM);
			if(tokenOff==-1)return 0;
			lineControlled=1;
			myserver_strlcpy(request->CONTENT_ENCODING,token,tokenOff+1);
			request->CONTENT_ENCODING[tokenOff]='\0';
			StrTrim(request->CONTENT_ENCODING," ");
		}else
		/*!Transfer-Encoding*/
		if(!lstrcmpi(command,"Transfer-Encoding"))
		{
			tokenOff = getCharInString(token,seps,HTTP_REQUEST_TRANSFER_ENCODING_DIM);
			if(tokenOff==-1)return 0;
			lineControlled=1;
			myserver_strlcpy(request->TRANSFER_ENCODING,token,tokenOff+1);
			request->TRANSFER_ENCODING[tokenOff]='\0';
			StrTrim(request->TRANSFER_ENCODING," ");
		}else
		/*!Content-Type*/
		if(!lstrcmpi(command,"Content-Type"))
		{
			tokenOff = getCharInString(token,seps,HTTP_REQUEST_CONTENT_TYPE_DIM);
			if(tokenOff==-1)
        return 0;
			lineControlled=1;
			myserver_strlcpy(request->CONTENT_TYPE,token,tokenOff+1);
			request->CONTENT_TYPE[tokenOff]='\0';
			StrTrim(request->CONTENT_TYPE," ");
		}else
		/*!If-Modified-Since*/
		if(!lstrcmpi(command,"If-Modified-Since"))
		{
			tokenOff = getCharInString(token,seps,HTTP_REQUEST_IF_MODIFIED_SINCE_DIM);
			if(tokenOff==-1)
        return 0;
			lineControlled=1;
			myserver_strlcpy(request->IF_MODIFIED_SINCE,token,tokenOff+1);
			request->IF_MODIFIED_SINCE[tokenOff]='\0';
			StrTrim(request->IF_MODIFIED_SINCE," ");
		}else
		/*!Accept*/
		if(!lstrcmpi(command,"Accept"))
		{
			int max = HTTP_REQUEST_ACCEPT_DIM-(int)strlen(request->ACCEPT);
			int oldlen = (int)strlen(request->ACCEPT);
			if(max < 0)
				return 0;
			tokenOff = getCharInString(token,seps,max);
			if(tokenOff == -1)
        return 0;
			lineControlled=1;
			strncat(request->ACCEPT,token,tokenOff+1);
			request->ACCEPT[oldlen+tokenOff]='\0';
			StrTrim(request->ACCEPT," ");
		}else
		/*!Accept-Language*/
		if(!lstrcmpi(command,"Accept-Language"))
		{
			tokenOff = getCharInString(token,seps,HTTP_REQUEST_ACCEPTLAN_DIM);
			if(tokenOff==-1)
        return 0;
			lineControlled=1;
			myserver_strlcpy(request->ACCEPTLAN,token,tokenOff+1);
			request->ACCEPTLAN[tokenOff]='\0';
			StrTrim(request->ACCEPTLAN," ");
		}else
		/*!Accept-Charset*/
		if(!lstrcmpi(command,"Accept-Charset"))
		{
			tokenOff = getCharInString(token,seps,HTTP_REQUEST_ACCEPTCHARSET_DIM);
			if(tokenOff==-1)
        return 0;
			lineControlled=1;
			myserver_strlcpy(request->ACCEPTCHARSET,token,tokenOff+1);
			request->ACCEPTCHARSET[tokenOff]='\0';
			StrTrim(request->ACCEPTCHARSET," ");
		}else
		/*!Accept-Encoding*/
		if(!lstrcmpi(command,"Accept-Encoding"))
		{
			tokenOff = getCharInString(token,seps,HTTP_REQUEST_ACCEPTENC_DIM);
			if(tokenOff==-1)
        return 0;
			lineControlled=1;
			myserver_strlcpy(request->ACCEPTENC,token,tokenOff+1);
			request->ACCEPTENC[tokenOff]='\0';
			StrTrim(request->ACCEPTENC," ");
		}else
		/*!Connection*/
		if(!lstrcmpi(command,"Connection"))
		{
			tokenOff = getCharInString(token,seps,HTTP_REQUEST_CONNECTION_DIM);
			if(tokenOff==-1)
        return 0;
			lineControlled=1;
			myserver_strlcpy(request->CONNECTION,token,tokenOff+1);
			request->CONNECTION[tokenOff]='\0';
			StrTrim(request->CONNECTION," ");
		}else
		/*!Cookie*/
		if(!lstrcmpi(command,"Cookie"))
		{
			tokenOff = getCharInString(token,seps,HTTP_REQUEST_COOKIE_DIM);
			if(tokenOff==-1)
        return 0;
			lineControlled=1;
			myserver_strlcpy(request->COOKIE,token,tokenOff+1);
			request->COOKIE[tokenOff]='\0';
		}else
		/*!From*/
		if(!lstrcmpi(command,"From"))
		{
			tokenOff = getCharInString(token,seps,HTTP_REQUEST_FROM_DIM);
			if(tokenOff==-1)
        return 0;
			lineControlled=1;
			myserver_strlcpy(request->FROM,token,tokenOff+1);
			request->FROM[tokenOff]='\0';
			StrTrim(request->FROM," ");
		}else
		/*!Content-Length*/
		if(!lstrcmpi(command,"Content-Length"))
		{
			tokenOff = getCharInString(token,seps,HTTP_REQUEST_CONTENT_LENGTH_DIM);
			if(tokenOff==-1)
        return 0;
			lineControlled=1;
			myserver_strlcpy(request->CONTENT_LENGTH,token,tokenOff+1);
			request->CONTENT_LENGTH[tokenOff]='\0';
		}else
		/*!Cache-Control*/
		if(!lstrcmpi(command,"Cache-Control"))
		{
			tokenOff = getCharInString(token,seps,HTTP_REQUEST_CACHE_CONTROL_DIM);
			if(tokenOff==-1)
        return 0;
			lineControlled=1;
			myserver_strlcpy(request->CACHE_CONTROL,token,tokenOff+1);
			request->CACHE_CONTROL[tokenOff]='\0';
		}else
		/*!Range*/
		if(!lstrcmpi(command,"Range"))
		{
      char RANGEBYTEBEGIN[13];
      char RANGEBYTEEND[13];
      char *localToken = token;
			int i=0;
			request->RANGETYPE[0]='\0';
			RANGEBYTEBEGIN[0]='\0';
			RANGEBYTEEND[0]='\0';
			lineControlled=1;
			tokenOff = getCharInString(token,seps,HTTP_REQUEST_RANGETYPE_DIM+30);
			if(tokenOff==-1)
        return 0;
			do
			{
				request->RANGETYPE[i++]=*localToken;
				request->RANGETYPE[i]='\0';
			}
			while((*(++localToken) != '=')&&(i<HTTP_REQUEST_RANGETYPE_DIM));
			i=0;
      localToken++;
			do
			{
				RANGEBYTEBEGIN[i++]=*localToken;
				RANGEBYTEBEGIN[i]='\0';
			}
			while((*(++localToken) != '-')&&(i<12) && (*localToken != '\r'));
			i=0;
      localToken++;
			do
			{
				RANGEBYTEEND[i++]=*localToken;
				RANGEBYTEEND[i]='\0';
			}
			while((*(++localToken) != '\r' )&&(i<12));
			StrTrim(request->RANGETYPE,"= ");
			StrTrim(RANGEBYTEBEGIN,"- ");
			StrTrim(RANGEBYTEEND,"- ");
			
			if(RANGEBYTEBEGIN[0]==0)
      {
				request->RANGEBYTEBEGIN=0;
      }			
      else
      {
        request->RANGEBYTEBEGIN=(u_long)atol(RANGEBYTEBEGIN); 
      }
      if(RANGEBYTEEND[0]=='\r')
      {
        request->RANGEBYTEEND=0;
      }
      else
      {
        request->RANGEBYTEEND=(u_long)atol(RANGEBYTEEND);
        if(request->RANGEBYTEEND < request->RANGEBYTEBEGIN)
          return 0;
      }
		}else
		/*!Referer*/
		if(!lstrcmpi(command,"Referer"))
		{
			tokenOff = getCharInString(token,seps,HTTP_REQUEST_REFERER_DIM);
			if(tokenOff==-1)return 0;
			lineControlled=1;
			myserver_strlcpy(request->REFERER,token,tokenOff+1);
			request->REFERER[tokenOff]='\0';
			StrTrim(request->REFERER," ");
		}else
		/*!Pragma*/
		if(!lstrcmpi(command,"Pragma"))
		{
			tokenOff = getCharInString(token,seps,HTTP_REQUEST_PRAGMA_DIM);
			if(tokenOff==-1)return 0;
			lineControlled=1;
			myserver_strlcpy(request->PRAGMA,token,tokenOff+1);
			request->PRAGMA[tokenOff]='\0';
			StrTrim(request->PRAGMA," ");
		}
    if(!lineControlled)
    {
      tokenOff = getCharInString(token,seps,maxTotchars);
      if(tokenOff==-1)
        return 0;
		}
    token+= tokenOff + 2;
		tokenOff = getCharInString(token,":",maxTotchars);
	}while(((u_long)(token-input)<maxTotchars) && token[0]!='\r');
	/*!
   *END REQUEST STRUCTURE BUILD.
   */
	td->nHeaderChars=maxTotchars;
	return validRequest;
}

/*!
 *Build the HTTP RESPONSE HEADER string.
 *If no input is setted the input is the main buffer of the 
 *HttpThreadContext structure.
 *Return 0 on invalid input or internal errors.
 */
int HttpHeaders::buildHTTPResponseHeaderStruct(HTTP_RESPONSE_HEADER *response, 
                                                HttpThreadContext *td,char *input)
{
	/*!
   *In this function there is the HTTP protocol parse.
   *The request is mapped into a HTTP_REQUEST_HEADER structure
   *And at the end of this every command is treated
   *differently. We use this mode for parse the HTTP
   *cause especially in the CGI is requested a continous
   *HTTP header access.
   *Before mapping the header in the structure 
   *control if this is a regular request.
   *The HTTP header ends with a \r\n\r\n sequence.
   */
	int noinputspecified=0;
	char *newInput;
	u_long nLines,maxTotchars;
	u_long validResponse;
	const char cmdseps[]   = ": ,\t\n\r\0";

	int containStatusLine=0;
	char *token=0;
	char command[96];

	int lineControlled = 0;
	int nLineControlled = 0;

	if(input==0)
	{
		input=(char*)td->buffer2->GetBuffer();
		noinputspecified=1;
	}
	/*! Control if the HTTP header is a valid header.  */
	if(input[0]==0)
		return 0;
	validResponse=validHTTPResponse(input, td, &nLines, &maxTotchars);

	if(validResponse)
	{
		newInput=new char[maxTotchars + 1];
		if(!newInput)
			return 0;
		/*! FIXME: Don't alloc new memory but simply use a no-destructive parsing.  */
		memcpy(newInput, input, maxTotchars);
		newInput[maxTotchars]='\0';
		input = newInput;
	}
	else
		return 0;

	token=input;
	
	/*! Check if is specified the first line containing the HTTP status.  */
	if((input[0]=='H')&&(input[1]=='T')&&(input[2]=='T')
     &&(input[3]=='P')&&(input[4]==' '))
	{
		containStatusLine=1;
		token = strtok( token, " " );
	}
	else
		token = strtok( token, " ,\t\n\r" );
	do
	{
		if(!token)
			break;
		/*!
		*Reset the flag lineControlled.
		*/
		lineControlled=0;

		/*!
		*Copy the HTTP command.
		*/
		myserver_strlcpy(command, token, 96);
		
		nLineControlled++;
		if((nLineControlled==1)&& containStatusLine)
		{
			lineControlled=1;
			/*! Copy the HTTP version. */
			myserver_strlcpy(response->VER, command, HTTP_RESPONSE_VER_DIM);
		
			token = strtok( NULL, " ,\t\n\r" );
			if(token)
				response->httpStatus=atoi(token);
			
			token = strtok( NULL, "\r\n\0" );
			if(token)
				myserver_strlcpy(response->ERROR_TYPE,token,HTTP_RESPONSE_ERROR_TYPE_DIM);

		}else
		/*!Server*/
		if(!lstrcmpi(command,"Server"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			myserver_strlcpy(response->SERVER_NAME,token,HTTP_RESPONSE_SERVER_NAME_DIM);
			StrTrim(response->SERVER_NAME," ");
		}else
		/*!Location*/
		if(!lstrcmpi(command,"Location"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			myserver_strlcpy(response->LOCATION,token,HTTP_RESPONSE_LOCATION_DIM);
			StrTrim(response->LOCATION," ");
		}else
		/*!Last-Modified*/
		if(!lstrcmpi(command,"Last-Modified"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			myserver_strlcpy(response->LAST_MODIFIED,token,HTTP_RESPONSE_LAST_MODIFIED_DIM);
			StrTrim(response->LAST_MODIFIED," ");
		}else
		/*!Status*/
		if(!lstrcmpi(command,"Status"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			response->httpStatus=atoi(token);
		}else
		/*!Content-Encoding*/
		if(!lstrcmpi(command,"Content-Encoding"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			myserver_strlcpy(response->CONTENT_ENCODING,token, 
                       HTTP_RESPONSE_CONTENT_ENCODING_DIM);
			StrTrim(response->CONTENT_ENCODING," ");
		}else
		/*!Cache-Control*/
		if(!lstrcmpi(command,"Cache-Control"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			myserver_strlcpy(response->CACHE_CONTROL,token,HTTP_RESPONSE_CACHE_CONTROL_DIM);
		}else
		/*!Date*/
		if(!lstrcmpi(command,"Date"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			myserver_strlcpy(response->DATE,token,HTTP_RESPONSE_DATE_DIM);
			StrTrim(response->DATE," ");
		}else
		/*!Content-Type*/
		if(!lstrcmpi(command,"Content-Type"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			myserver_strlcpy(response->CONTENT_TYPE,token,HTTP_RESPONSE_CONTENT_TYPE_DIM);
			StrTrim(response->CONTENT_TYPE," ");
		}else
		/*!MIME-Version*/
		if(!lstrcmpi(command,"MIME-Version"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			myserver_strlcpy(response->MIMEVER,token,HTTP_RESPONSE_MIMEVER_DIM);
			StrTrim(response->MIMEVER," ");
		}else
		/*!Set-Cookie*/
		if(!lstrcmpi(command,"Set-Cookie"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			strncat(response->COOKIE,token,
              HTTP_RESPONSE_COOKIE_DIM-strlen(response->COOKIE));
			strcat(response->COOKIE,"\n");/*!Divide multiple cookies*/
		}else
		/*!Content-Length*/
		if(!lstrcmpi(command,"Content-Length"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			myserver_strlcpy(response->CONTENT_LENGTH,token, 
                       HTTP_RESPONSE_CONTENT_LENGTH_DIM);
			StrTrim(response->CONTENT_LENGTH," ");
		}else
		/*!Content-Range*/
		if(!lstrcmpi(command,"Content-Range"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			myserver_strlcpy(response->CONTENT_RANGE,token, 
                       HTTP_RESPONSE_CONTENT_RANGE_DIM);
			StrTrim(response->CONTENT_RANGE," ");
		}else
		/*!P3P*/
		if(!lstrcmpi(command,"P3P"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			myserver_strlcpy(response->P3P,token,HTTP_RESPONSE_P3P_DIM);
			StrTrim(response->P3P," ");
		}else
		/*!Connection*/
		if(!lstrcmpi(command,"Connection"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			myserver_strlcpy(response->CONNECTION,token,HTTP_RESPONSE_CONNECTION_DIM);
			StrTrim(response->CONNECTION," ");
		}else
		/*!Expires*/
		if(!lstrcmpi(command,"Expires"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			myserver_strlcpy(response->DATEEXP,token,HTTP_RESPONSE_DATEEXP_DIM);
			StrTrim(response->DATEEXP," ");
		}
		/*!
     *If the line is not controlled arrive with the token
     *at the end of the line.
     */
		if( (!lineControlled)&&  ((!containStatusLine) || (nLineControlled!=1)) )
		{
			token = strtok( NULL, "\n" );
			if(token)
			{
				strncat(response->OTHER,command,
                HTTP_RESPONSE_OTHER_DIM-strlen(response->OTHER));
				strncat(response->OTHER, token, 
                HTTP_RESPONSE_OTHER_DIM-strlen(response->OTHER)-1);
        strncat(response->OTHER, "\n", 1);     
			}
		}
		token = strtok( NULL, cmdseps );
	}while((u_long)(token-input)<maxTotchars);
	/*!
   *END REQUEST STRUCTURE BUILD.
   */
	td->nBytesToRead=maxTotchars;
	delete [] input;
	return validResponse;
}

/*!
 *Controls if the req string is a valid HTTP request header.
 *Returns 0 if req is an invalid header, 
 *Returns -1 if the header is incomplete,
 *Returns another non-zero value if is a valid header.
 *nLinesptr is a value of the lines number in the HEADER.
 *ncharsptr is a value of the characters number in the HEADER.
 */
int HttpHeaders::validHTTPRequest(char *req, HttpThreadContext* td,
                                   u_long* nLinesptr,u_long* ncharsptr)
{
	u_long i=0;
	u_long buffersize=td->buffer->GetRealLength();
	u_long nLinechars=0;
	int isValidCommand=-1;
	nLinechars=0;
	u_long nLines=0;
	u_long maxTotchars=0;
	nLines=0;
	if(req==0)
	{
		return 0;
	}
	
	for(;(i<MYSERVER_KB(8));i++)
	{
		if(req[i]=='\n')
		{
			if(req[i+2]=='\n')
			{
				maxTotchars=i+3;
				if(maxTotchars>buffersize)
				{
					isValidCommand=0;
					break;				
				}
				isValidCommand=1;
				break;
			}
			nLinechars=0;
			nLines++;
			/*! 
       *If the lines number is greater than 25 we consider 
       *the header invalid.  
       */
			if(nLines>25)
			{
				return 0;
			}
		}
		else if(req[i]=='\0')
    {
      isValidCommand = -1;
      break;
    }
		else
		{
			nLinechars++;
		}
		/*!
     *We set a maximal theorical number of characters in a line to 1024.
     *If a line contains more than N characters we consider the header invalid.
     */
		if(nLinechars>2048)
		{
			isValidCommand=0;
			break;
		}
	}

	/*! Set the output variables.  */
	*nLinesptr=nLines;
	*ncharsptr=maxTotchars;
	
	/*! Return if is a valid request header.  */
	return isValidCommand;
}
