/*
*MyServer
*Copyright (C) 2002 The MyServer Team
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
#include "../include/cserver.h"
#include "../include/security.h"
#include "../include/http.h"
#include "../include/AMMimeUtils.h"
#include "../include/filemanager.h"
#include "../include/sockets.h"
#include "../include/utility.h"
#include "../include/stringutils.h"
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
void http_headers::buildHTTPResponseHeader(char *str,HTTP_RESPONSE_HEADER* response)
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
			int errID=getErrorIDfromHTTPStatusCode(response->httpStatus);
			if(errID!=-1)
				strncpy(response->ERROR_TYPE,HTTP_ERROR_MSGS[errID],HTTP_RESPONSE_ERROR_TYPE_DIM);
		}
		sprintf(str,"%s %i %s\r\nStatus: %s\r\n",response->VER,response->httpStatus,response->ERROR_TYPE,response->ERROR_TYPE);
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
		char *token=strtok(response->COOKIE,"\n");
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
	strcat(str,"\r\n");
}
/*!
*Set the defaults value for a HTTP_RESPONSE_HEADER structure.
*/
void http_headers::buildDefaultHTTPResponseHeader(HTTP_RESPONSE_HEADER* response)
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
void http_headers::resetHTTPRequest(HTTP_REQUEST_HEADER *request)
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
	request->RANGEBYTEBEGIN[0]='\0';
	request->RANGEBYTEEND[0]='\0';
	request->uriEndsWithSlash=0;
	request->digest_realm[0]='\0';
	request->digest_opaque[0]='\0';
	request->digest_nonce[0]='\0';
	request->digest_cnonce[0]='\0';
	request->digest_username[0]='\0';
	request->digest_response[0]='\0';
	request->digest_qop[0]='\0';
	request->digest_nc[0]='\0';
}
/*!
*Reset all the HTTP_RESPONSE_HEADER structure members.
*/
void http_headers::resetHTTPResponse(HTTP_RESPONSE_HEADER *response)
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
}


/*!
*Controls if the req string is a valid HTTP response header.
*Returns 0 if req is an invalid header, a non-zero value if is a valid header.
*nLinesptr is a value of the lines number in the HEADER.
*ncharsptr is a value of the characters number in the HEADER.
*/
u_long http_headers::validHTTPResponse(char *req,httpThreadContext* td,u_long* nLinesptr,u_long* ncharsptr)
{
	u_long i;
	u_long buffersize=td->buffersize;
	u_long nLinechars;
	int isValidCommand=0;
	nLinechars=0;
	u_long nLines=0;
	u_long maxTotchars=0;
	if(req==0)
		return 0;
	/*!
	*Count the number of lines in the header.
	*/
	for(i=nLines=0;;i++)
	{
		if(req[i]=='\n')
		{
			if((req[i+2]=='\n')|(req[i+1]=='\0')|(req[i+1]=='\n'))
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
		*If a line contains more than 4110 lines we consider the header invalid.
		*/
		if(nLinechars>4110)
			break;
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
*If no input is setted the input is the main buffer of the httpThreadContext structure.
*/
int http_headers::buildHTTPRequestHeaderStruct(HTTP_REQUEST_HEADER *request,httpThreadContext *td,char *input)
{
	/*!
	*In this function there is the HTTP protocol parse.
	*The request is mapped into a HTTP_REQUEST_HEADER structure
	*And at the end of this every command is treated
	*differently. We use this mode for parse the HTTP
	*cause especially in the CGI is requested a continous
	*HTTP header access.
	*Before mapping the header in the structure 
	*control if this is a regular request->
	*The HTTP header ends with a \r\n\r\n sequence.
	*/

	/*!
	*Control if the HTTP header is a valid header.
	*/
	u_long i=0,j=0,max=0;
	u_long nLines,maxTotchars;
	int noinputspecified=0;
	if(input==0)
	{
		noinputspecified=1;
		input=td->buffer;
	}
	u_long validRequest=validHTTPRequest(input,td,&nLines,&maxTotchars);
	if(validRequest==0)/*!Invalid header*/
		return 0;
	else if(validRequest==-1)/*!Incomplete header*/
		return -1;

	const int max_URI=MAX_PATH+200;
	const char seps[]   = "\t\n\r";
	const char cmdseps[]   = ": ,\t\n\r";

	static char *token=0;
	static char command[96];

	static int nLineControlled;
	nLineControlled=0;
	static int lineControlled;
	
	if(!noinputspecified)
		token=input;
	else
	{
		strncpy(td->buffer2,input,maxTotchars);
		input=token=td->buffer2;
	}
	while((token[i]=='\n')||(token[i]=='\r'))token++;
	token = strtok( token, cmdseps );
	if(!token)return 0;
	do
	{
		/*!
		*Reset the flag lineControlled.
		*/
		lineControlled=0;

		/*!
		*Copy the HTTP command.
		*/
		strncpy(command,token,96);
		
		nLineControlled++;
		if(nLineControlled==1)
		{
			/*!
			*The first line has the form:
			*GET /index.html HTTP/1.1
			*/
			lineControlled=1;
			/*!
			*Copy the method type.
			*/
			strncpy(request->CMD,command,HTTP_REQUEST_CMD_DIM);
			token = strtok( NULL, "\t\n\r" );
			if(token==0)
				return 0;
			u_long len_token=(u_long)strlen(token);
			max=len_token;
			while((token[max]!=' ')&(len_token-max<HTTP_REQUEST_VER_DIM))
				max--;
			int containOpts=0;
			for(i=0;(i<max)&&(i<HTTP_REQUEST_URI_DIM);i++)
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
				for(j=0;(i<max) && (j<HTTP_REQUEST_URIOPTS_DIM-1);j++)
				{
					request->URIOPTS[j]=token[++i];
					request->URIOPTS[j+1]='\0';
				}
			}
			strncpy(request->VER,&token[max],HTTP_REQUEST_VER_DIM);

			if(request->URI[strlen(request->URI)-1]=='/')
				request->uriEndsWithSlash=1;
			else
				request->uriEndsWithSlash=0;
			StrTrim(request->URI," /");
			StrTrim(request->URIOPTS," /");
			max=(u_long)strlen(request->URI);
			if(max>max_URI)
			{
				return 414;
			}
		}else
		/*!User-Agent*/
		if(!lstrcmpi(command,"User-Agent"))
		{
			token = strtok( NULL, "\r\n" );
			if(!token)return 0;
			lineControlled=1;
			strncpy(request->USER_AGENT,token,HTTP_REQUEST_USER_AGENT_DIM);
			StrTrim(request->USER_AGENT," ");
		}else
		/*!Authorization*/
		if(!lstrcmpi(command,"Authorization"))
		{
			token = strtok( NULL, " " );
			if(!token)return 0;
			lineControlled=1;
			
			strcpy(request->AUTH,token);
			if(!lstrcmpi(token,"Basic"))
			{
				u_long i;
				char *base64=&token[strlen("Basic ")];
				int len=(int)strlen(base64);
				char* lbuffer2=base64Utils.Decode(base64,&len);
				char* keep_lbuffer2=lbuffer2;
				for(i=0;(*lbuffer2!=':') && (i<19);i++)
				{
					td->connection->login[i]=*lbuffer2++;
					td->connection->login[i+1]='\0';
				}
				strncpy(td->identity,td->connection->login,32);
				lbuffer2++;
				for(i=0;(*lbuffer2)&&(i<31);i++)
				{
					td->connection->password[i]=*lbuffer2++;
					td->connection->password[i+1]='\0';
				}
				free(keep_lbuffer2);
				token = strtok( NULL, "\r\n");
			}
			else if(!lstrcmpi(token,"Digest"))
			{
				LPCONNECTION a = td->connection;
				if(a->protocolBuffer==0)
					return 0;
				while(token = strtok( NULL, "=" ))
				{
					StrTrim(token," ");
					if(!lstrcmpi(token,"nonce"))
					{
						token = strtok( NULL, "," );
						StrTrim(token,"\" ");
						strncpy(td->request.digest_nonce,token,48);
					}
					else if(!lstrcmpi(token,"opaque"))
					{
						token = strtok( NULL, "," );
						StrTrim(token,"\" ");
						strncpy(td->request.digest_opaque,token,48);
					}
					else if(!lstrcmpi(token,"uri"))
					{
						token = strtok( NULL, "\r\n," );
						StrTrim(token,"\" ");
						strncpy(td->request.digest_uri,token,1024);
					}
					else if(!lstrcmpi(token,"method"))
					{
						token = strtok( NULL, "\r\n," );
						StrTrim(token,"\" ");
						strncpy(td->request.digest_method,token,16);
					}										
					else if(!lstrcmpi(token,"qop"))
					{
						token = strtok( NULL, "\r\n," );
						StrTrim(token,"\" ");
						strncpy(td->request.digest_qop,token,16);
					}					
					else if(!lstrcmpi(token,"realm"))
					{
						token = strtok( NULL, "\r\n," );
						StrTrim(token,"\" ");
						strncpy(td->request.digest_realm,token,48);
					}
					else if(!lstrcmpi(token,"cnonce"))
					{
						token = strtok( NULL, "\r\n," );
						StrTrim(token," \"");
						strncpy(td->request.digest_cnonce,token,48);
					}
					else if(!lstrcmpi(token,"username"))
					{
						token = strtok( NULL, "\r\n," );
						StrTrim(token,"\" ");
						strncpy(td->request.digest_username,token,16);
						strncpy(td->connection->login,token,48);						
					}
					else if(!lstrcmpi(token,"response"))
					{
						token = strtok( NULL, "\r\n," );
						StrTrim(token,"\" ");
						strncpy(td->request.digest_response,token,48);
					}
					else if(!lstrcmpi(token,"nc"))
					{
						token = strtok( NULL, "\r\n," );
						StrTrim(token,"\" ");
						strncpy(td->request.digest_nc,token,10);
					}
					else 
					{
						token = strtok( NULL, "\r\n," );
					}
				}
			}
		}else
		/*!Host*/
		if(!lstrcmpi(command,"Host"))
		{
			token = strtok( NULL, seps );
			if(!token)return 0;
			lineControlled=1;
			strncpy(request->HOST,token,HTTP_REQUEST_HOST_DIM);
			StrTrim(request->HOST," ");
		}else
		/*!Content-Encoding*/
		if(!lstrcmpi(command,"Content-Encoding"))
		{
			token = strtok( NULL, seps );
			if(!token)return 0;
			lineControlled=1;
			strncpy(request->CONTENT_ENCODING,token,HTTP_REQUEST_CONTENT_ENCODING_DIM);
			StrTrim(request->CONTENT_ENCODING," ");
		}else
		/*!Content-Type*/
		if(!lstrcmpi(command,"Content-Type"))
		{
			token = strtok( NULL, seps );
			if(!token)return 0;
			lineControlled=1;
			strncpy(request->CONTENT_TYPE,token,HTTP_REQUEST_CONTENT_TYPE_DIM);
			StrTrim(request->CONTENT_TYPE," ");
		}else
		/*!If-Modified-Since*/
		if(!lstrcmpi(command,"If-Modified-Since"))
		{
			token = strtok( NULL, seps );
			if(!token)return 0;
			lineControlled=1;
			strncpy(request->IF_MODIFIED_SINCE,token,HTTP_REQUEST_IF_MODIFIED_SINCE_DIM);
			StrTrim(request->IF_MODIFIED_SINCE," ");
		}else
		/*!Accept*/
		if(!lstrcmpi(command,"Accept"))
		{
			token = strtok( NULL, "\n\r" );
			if(!token)return 0;
			lineControlled=1;
			strncat(request->ACCEPT,token,HTTP_REQUEST_ACCEPT_DIM-strlen(request->ACCEPT));
			StrTrim(request->ACCEPT," ");
		}else
		/*!Accept-Language*/
		if(!lstrcmpi(command,"Accept-Language"))
		{
			token = strtok( NULL, "\n\r" );
			if(!token)return 0;
			lineControlled=1;
			strncpy(request->ACCEPTLAN,token,HTTP_REQUEST_ACCEPTLAN_DIM);
			StrTrim(request->ACCEPTLAN," ");
		}else
		/*!Accept-Charset*/
		if(!lstrcmpi(command,"Accept-Charset"))
		{
			token = strtok( NULL, "\n\r" );
			if(!token)return 0;
			lineControlled=1;
			strncpy(request->ACCEPTCHARSET,token,HTTP_REQUEST_ACCEPTCHARSET_DIM);
			StrTrim(request->ACCEPTCHARSET," ");
		}else
		/*!Accept-Encoding*/
		if(!lstrcmpi(command,"Accept-Encoding"))
		{
			token = strtok( NULL, "\n\r" );
			if(!token)return 0;
			lineControlled=1;
			strncpy(request->ACCEPTENC,token,HTTP_REQUEST_ACCEPTENC_DIM);
			StrTrim(request->ACCEPTENC," ");
		}else
		/*!Connection*/
		if(!lstrcmpi(command,"Connection"))
		{
			token = strtok( NULL, "\n\r" );
			if(!token)return 0;
			lineControlled=1;
			strncpy(request->CONNECTION,token,HTTP_REQUEST_CONNECTION_DIM);
			StrTrim(request->CONNECTION," ");
		}else
		/*!Cookie*/
		if(!lstrcmpi(command,"Cookie"))
		{
			token = strtok( NULL, "\n\r" );
			if(!token)return 0;
			lineControlled=1;
			strncpy(request->COOKIE,token,HTTP_REQUEST_COOKIE_DIM);
		}else
		/*!From*/
		if(!lstrcmpi(command,"From"))
		{
			token = strtok( NULL, "\r\n" );
			if(!token)return 0;
			lineControlled=1;
			strncpy(request->FROM,token,HTTP_REQUEST_FROM_DIM);
			StrTrim(request->FROM," ");
		}else
		/*!Content-Length*/
		if(!lstrcmpi(command,"Content-Length"))
		{
			token = strtok( NULL, "\n\r" );
			if(!token)return 0;
			lineControlled=1;
			strncpy(request->CONTENT_LENGTH,token,HTTP_REQUEST_CONTENT_LENGTH_DIM);
		}else
		/*!Cache-Control*/
		if(!lstrcmpi(command,"Cache-Control"))
		{
			token = strtok( NULL, "\n\r" );
			if(!token)return 0;
			lineControlled=1;
			strncpy(request->CACHE_CONTROL,token,HTTP_REQUEST_CACHE_CONTROL_DIM);
		}else
		/*!Range*/
		if(!lstrcmpi(command,"Range"))
		{
			request->RANGETYPE[0]='\0';
			request->RANGEBYTEBEGIN[0]='\0';
			request->RANGEBYTEEND[0]='\0';
			lineControlled=1;
			token = strtok( NULL, "\r\n\t" );
			if(!token)return 0;
			int i=0;
			do
			{
				request->RANGETYPE[i++]=*token;
				request->RANGETYPE[i]='\0';
			}
			while((*token++ != '=')&&(i<HTTP_REQUEST_RANGETYPE_DIM));
			i=0;
			do
			{
				request->RANGEBYTEBEGIN[i++]=*token;
				request->RANGEBYTEBEGIN[i]='\0';
			}
			while((*token++ != '-')&&(i<HTTP_REQUEST_RANGEBYTEBEGIN_DIM));
			i=0;
			do
			{
				request->RANGEBYTEEND[i++]=*token;
				request->RANGEBYTEEND[i]='\0';
			}
			while((*token++)&&(i<HTTP_REQUEST_RANGEBYTEEND_DIM));
			StrTrim(request->RANGETYPE,"= ");
			StrTrim(request->RANGEBYTEBEGIN,"- ");
			StrTrim(request->RANGEBYTEEND,"- ");
			
			if(request->RANGEBYTEBEGIN[0]==0)
				strncpy(request->RANGEBYTEBEGIN,"0",HTTP_REQUEST_RANGEBYTEBEGIN_DIM);
			if(request->RANGEBYTEEND[0]==0)
				strncpy(request->RANGEBYTEEND,"-1",HTTP_REQUEST_RANGEBYTEEND_DIM);

		}else
		/*!Referer*/
		if(!lstrcmpi(command,"Referer"))
		{
			token = strtok( NULL, seps );
			if(!token)return 0;
			lineControlled=1;
			strncpy(request->REFERER,token,HTTP_REQUEST_REFERER_DIM);
			StrTrim(request->REFERER," ");
		}else
		/*!Pragma*/
		if(!lstrcmpi(command,"Pragma"))
		{
			token = strtok( NULL, seps );
			if(!token)return 0;
			lineControlled=1;
			strncpy(request->PRAGMA,token,HTTP_REQUEST_PRAGMA_DIM);
			StrTrim(request->PRAGMA," ");
		}
		
		/*!
		*If the line is not controlled arrive with the token
		*at the end of the line.
		*/
		if(!lineControlled)
		{
			token = strtok( NULL, "\n" );
			if(!token)return 0;
		}
		token = strtok( NULL, cmdseps );
	}while((u_long)(token-input)<maxTotchars);
	/*!
	*END REQUEST STRUCTURE BUILD.
	*/
	td->nHeaderChars=maxTotchars;
	return validRequest;
}

/*!
*Build the HTTP RESPONSE HEADER string.
*If no input is setted the input is the main buffer of the httpThreadContext structure.
*/
int http_headers::buildHTTPResponseHeaderStruct(HTTP_RESPONSE_HEADER *response,httpThreadContext *td,char *input)
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

	if(input==0)
	{
		input=td->buffer;
		noinputspecified=1;
	}
	/*!
	*Control if the HTTP header is a valid header.
	*/
	if(input[0]==0)
		return 0;
	u_long nLines,maxTotchars;
	u_long validRequest=validHTTPResponse(input,td,&nLines,&maxTotchars);

	const char cmdseps[]   = ": ,\t\n\r\0";

	static char *token=0;
	static char command[96];

	static int nLineControlled;
	nLineControlled=0;
	static int lineControlled;
	if(noinputspecified)
	{
		strncpy(td->buffer2,input,maxTotchars);
		input=token=td->buffer2;
	}
	else
	{
		token=input;
	}

	token = strtok( token, cmdseps );
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
		strncpy(command,token,96);
		
		
		nLineControlled++;
		if((nLineControlled==1)&&(token[0]=='H')&&(token[1]=='T')&&(token[2]=='T')&&(token[3]=='P'))
		{
			/*!
			*The first line has the form:
			*GET /index.html HTTP/1.1
			*/
			lineControlled=1;
			/*!
			*Copy the HTTP version.
			*/
			strncpy(response->VER,command,HTTP_RESPONSE_VER_DIM);
		
			token = strtok( NULL, " ,\t\n\r" );
			if(token)
				response->httpStatus=atoi(token);
			
			token = strtok( NULL, "\r\n\0" );
			if(token)
				strncpy(response->ERROR_TYPE,token,HTTP_RESPONSE_ERROR_TYPE_DIM);

		}else
		/*!Server*/
		if(!lstrcmpi(command,"Server"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			strncpy(response->SERVER_NAME,token,HTTP_RESPONSE_SERVER_NAME_DIM);
			StrTrim(response->SERVER_NAME," ");
		}else
		/*!Location*/
		if(!lstrcmpi(command,"Location"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			strncpy(response->LOCATION,token,HTTP_RESPONSE_LOCATION_DIM);
			StrTrim(response->LOCATION," ");
		}else
		/*!Last-Modified*/
		if(!lstrcmpi(command,"Last-Modified"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			strncpy(response->LAST_MODIFIED,token,HTTP_RESPONSE_LAST_MODIFIED_DIM);
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
			strncpy(response->CONTENT_ENCODING,token,HTTP_RESPONSE_CONTENT_ENCODING_DIM);
			StrTrim(response->CONTENT_ENCODING," ");
		}else
		/*!Cache-Control*/
		if(!lstrcmpi(command,"Cache-Control"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			strncpy(response->CACHE_CONTROL,token,HTTP_RESPONSE_CACHE_CONTROL_DIM);
		}else
		/*!Date*/
		if(!lstrcmpi(command,"Date"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			strncpy(response->DATE,token,HTTP_RESPONSE_DATE_DIM);
			StrTrim(response->DATE," ");
		}else
		/*!Content-Type*/
		if(!lstrcmpi(command,"Content-Type"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			strncpy(response->CONTENT_TYPE,token,HTTP_RESPONSE_CONTENT_TYPE_DIM);
			StrTrim(response->CONTENT_TYPE," ");
		}else
		/*!MIME-Version*/
		if(!lstrcmpi(command,"MIME-Version"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			strncpy(response->MIMEVER,token,HTTP_RESPONSE_MIMEVER_DIM);
			StrTrim(response->MIMEVER," ");
		}else
		/*!Set-Cookie*/
		if(!lstrcmpi(command,"Set-Cookie"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			strncat(response->COOKIE,token,HTTP_RESPONSE_COOKIE_DIM-strlen(response->COOKIE));
			strcat(response->COOKIE,"\n");/*!Divide multiple cookies*/
		}else
		/*!Content-Length*/
		if(!lstrcmpi(command,"Content-Length"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			strncpy(response->CONTENT_LENGTH,token,HTTP_RESPONSE_CONTENT_LENGTH_DIM);
			StrTrim(response->CONTENT_LENGTH," ");
		}else
		/*!P3P*/
		if(!lstrcmpi(command,"P3P"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			strncpy(response->P3P,token,HTTP_RESPONSE_P3P_DIM);
			StrTrim(response->P3P," ");
		}else
		/*!Connection*/
		if(!lstrcmpi(command,"Connection"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			strncpy(response->CONNECTION,token,HTTP_RESPONSE_CONNECTION_DIM);
			StrTrim(response->CONNECTION," ");
		}else
		/*!Expires*/
		if(!lstrcmpi(command,"Expires"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			strncpy(response->DATEEXP,token,HTTP_RESPONSE_DATEEXP_DIM);
			StrTrim(response->DATEEXP," ");
		}
		/*!
		*If the line is not controlled arrive with the token
		*at the end of the line.
		*/
		if(!lineControlled)
		{
			token = strtok( NULL, "\n" );
			if(token)
			{
				if(response->OTHER[0])
					strncat(response->OTHER,"\r\n",HTTP_RESPONSE_OTHER_DIM-strlen(response->OTHER));
				strncat(response->OTHER,command,HTTP_RESPONSE_OTHER_DIM-strlen(response->OTHER));
				strncat(response->OTHER,": ",HTTP_RESPONSE_OTHER_DIM-strlen(response->OTHER));
				strncat(response->OTHER,token,HTTP_RESPONSE_OTHER_DIM-strlen(response->OTHER));
			}
		}
		token = strtok( NULL, cmdseps );
	}while((u_long)(token-input)<maxTotchars);
	/*!
	*END REQUEST STRUCTURE BUILD.
	*/
	td->nBytesToRead=maxTotchars;
	return validRequest;
}

/*!
*Controls if the req string is a valid HTTP request header.
*Returns 0 if req is an invalid header, 
*Returns -1 if the header is incomplete,
*Returns another non-zero value if is a valid header.
*nLinesptr is a value of the lines number in the HEADER.
*ncharsptr is a value of the characters number in the HEADER.
*/
u_long http_headers::validHTTPRequest(char *req,httpThreadContext* td,u_long* nLinesptr,u_long* ncharsptr)
{
	u_long i=0;
	u_long buffersize=td->buffersize;
	u_long nLinechars=0;
	u_long isValidCommand=0;
	nLinechars=0;
	u_long nLines=0;
	u_long maxTotchars=0;
	nLines=0;
	if(req==0)
	{
		return 0;
	}
	
	for(;(i<KB(8));i++)
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
			*If the lines number is greater than 25 we consider the header invalid.
			*/
			if(nLines>25)
			{
				return 0;
			}
		}
		else if(req[i]==0)
			return ((u_long)-1);
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

	/*!
	*Set the output variables.
	*/
	*nLinesptr=nLines;
	*ncharsptr=maxTotchars;
	
	/*!
	*Return if is a valid request header.
	*/
	return isValidCommand;
}
