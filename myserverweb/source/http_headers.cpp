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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "../stdafx.h"
#include <sstream>
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
 *Builds an HTTP header string starting from an HttpResponseHeader structure.
 */
void HttpHeaders::buildHTTPResponseHeader(char *str, HttpResponseHeader* response)
{
	/*!
   *Here is builded the HEADER of a HTTP response.
   *Passing a HttpResponseHeader struct this builds an header string.
   *Every directive ends with a \r\n sequence.
   */
	if(response->httpStatus!=200)
	{
		if(response->errorType.length() == 0)
		{
			int errID = getErrorIDfromHTTPStatusCode(response->httpStatus);
			if(errID!=-1)
				response->errorType.assign(HTTP_ERROR_MSGS[errID], 
                                    HTTP_RESPONSE_ERROR_TYPE_DIM);
		}
		sprintf(str,"%s %i %s\r\nStatus: %s\r\n",response->ver.c_str(),
            response->httpStatus, response->errorType.c_str(), 
            response->errorType.c_str() );
	}
	else
    sprintf(str,"%s 200 OK\r\n",response->ver.c_str());
	
	if(response->serverName.length())
	{
		strcat(str,"Server: ");
		strcat(str, response->serverName.c_str());
		strcat(str,"\r\n");
	}
	if(response->cacheControl.length())
	{
		strcat(str,"Cache-Control: ");
		strcat(str, response->cacheControl.c_str());
		strcat(str,"\r\n");
	}
	if(response->lastModified.length())
	{
		strcat(str,"Last-Modified: ");
		strcat(str, response->lastModified.c_str());
		strcat(str,"\r\n");
	}
	if(response->connection.length())
	{
		strcat(str,"Connection: ");
		strcat(str, response->connection.c_str());
		strcat(str, "\r\n");
	}
	else
	{
		strcat(str, "Connection: Close\r\n");
	}
	if(response->transferEncoding.length())
	{
		strcat(str, "Transfer-Encoding: ");
		strcat(str, response->transferEncoding.c_str());
		strcat(str, "\r\n");
	}
	if(response->contentEncoding.length())
	{
		strcat(str, "Content-Encoding: ");
		strcat(str, response->contentEncoding.c_str());
		strcat(str, "\r\n");
	}
	if(response->contentRange.length())
	{
		strcat(str, "Content-Range: ");
		strcat(str, response->contentRange.c_str());
		strcat(str, "\r\n");
	}
	if(response->contentLength.length())
	{
		/*!
		*Do not specify the Content-Length field if it is used
		*the chunked Transfer-Encoding.
		*/
		if(response->transferEncoding.find("chunked",0) == string::npos )
		{
			strcat(str, "Content-Length: ");
			strcat(str, response->contentLength.c_str());
			strcat(str, "\r\n");
		}
	}
	if(response->cookie.length())
	{
    string cookie;
		const char *token = response->cookie.c_str();
    int max = response->cookie.length();
		while(token)
		{
      int len = getCharInString(token, "\n", max);
      if(len == -1 || *token=='\n')
        break;
      cookie.assign("Set-Cookie: ");
			cookie.append(token, len);
			cookie.append("\r\n");	
      strcat(str, cookie.c_str());	
			token+=len+1;
		}
	}
	if(response->p3p.length())
	{
		strcat(str, "p3p: ");
		strcat(str, response->p3p.c_str());
		strcat(str, "\r\n");
	}
	if(response->mimeVer.length())
	{
		strcat(str, "MIME-Version: ");
		strcat(str, response->mimeVer.c_str());
		strcat(str, "\r\n");
	}
	if(response->contentType.length())
	{
		strcat(str, "Content-Type: ");
		strcat(str, response->contentType.c_str());
		strcat(str, "\r\n");
	}
	if(response->date.length())
	{
		strcat(str, "Date: ");
		strcat(str, response->date.c_str());
		strcat(str, "\r\n");
	}
	if(response->dateExp.length())
	{
		strcat(str, "Expires: ");
		strcat(str, response->dateExp.c_str());
		strcat(str, "\r\n");
	}
	if(response->auth.length())
	{
		strcat(str, "WWW-Authenticate: ");
		strcat(str, response->auth.c_str());
		strcat(str, "\r\n");
	}
	
	if(response->location.length())
	{
		strcat(str, "Location: ");
		strcat(str, response->location.c_str());
		strcat(str, "\r\n");
	}

	if(response->other.size())
	{
    int i;
    for(i=0; i < response->other.size(); i++)
    {
      HttpResponseHeader::Entry *e = response->other.getData(i);
      if(e)
      {
        strcat(str, e->name.c_str());
        strcat(str, ": ");
        strcat(str, e->value.c_str());
        strcat(str, "\r\n");
      }
      
    }
	}

	/*!
	*MyServer supports the bytes range.
	*/
	strcat(str, "Accept-Ranges: bytes\r\n");
  
	/*!
	*The HTTP header ends with a \r\n sequence.
	*/
	strcat(str, "\r\n\0\0\0\0\0");
}
/*!
 *Set the defaults value for a HttpResponseHeader structure.
 */
void HttpHeaders::buildDefaultHTTPResponseHeader(HttpResponseHeader* response)
{
  string date;
  ostringstream stream;
	resetHTTPResponse(response);
	/*!
	*By default use:
	*1) the MIME type of the page equal to text/html.
	*2) the version of the HTTP protocol to 1.1.
	*3) the date of the page and the expire date to the current time.
	*4) set the name of the server.
	*5) set the page that it is not an error page.
	*/
	response->contentType.assign("text/html");
	response->ver.assign("HTTP/1.1");
	getRFC822GMTTime(date,HTTP_RESPONSE_DATE_DIM);
  response->date.assign(date);
	response->dateExp.assign(date);
  stream << "MyServer " << versionOfSoftware;
	response->serverName.assign(stream.str());
}


/*!
 *Reset all the HTTP_REQUEST_HEADER structure members.
 */
void HttpHeaders::resetHTTPRequest(HttpRequestHeader *request)
{
	request->free();
}

/*!
 *Reset all the HttpResponseHeader structure members.
 */
void HttpHeaders::resetHTTPResponse(HttpResponseHeader *response)
{
	response->free();
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
	u_long nLinechars=0;
	u_long nLines=0;

	if(res==0)
		return 0;
	/*!
	*Count the number of lines in the header.
	*/
	for(i=0;;i++)
	{
		if(res[i]=='\n')
		{
			if((res[i+2]=='\n') || (res[i+1]=='\0') || (res[i+1]=='\n'))
			{
				if((i+3)>td->buffersize)
					return 0;
				break;
			}
			nLines++;
		}
		else
		{
			/*!
			*We set a maximal theorical number of characters in a line.
			*If a line contains more than 4160 characters we consider the 
      *header invalid.
			*/
			if(nLinechars>=4160)
	    		return 0;
	    	nLinechars++;
		}
	}
	
	/*!
	*Set the output variables.
	*/
	*nLinesptr=nLines;
	*ncharsptr=i+3;
	
	/*!
	*Return if is a valid request header.
	*/
	return 1;
}


/*!
 *Build the HTTP REQUEST HEADER string.
 *If no input is setted the input is the main buffer of the 
 *HttpThreadContext structure.
 *Returns e_200 if is a valid request.
 *Returns -1 if the request is incomplete.
 *Any other returned value is the HTTP error.
 */
int HttpHeaders::buildHTTPRequestHeaderStruct(HttpRequestHeader *request, 
                                              HttpThreadContext* td, char* input)
{
	/*!
   *In this function there is the HTTP protocol parse.
   *The request is mapped into a HttpRequestHeader structure
   *And at the end of this every command is treated
   *differently. We use this mode for parse the HTTP
   *cause especially in the CGI is requested a continous
   *HTTP header access.
   *Before mapping the header in the structure 
   *control if this is a regular request.
   *The HTTP header ends with a \r\n\r\n sequence.
   */
  
	u_long i=0,j=0;
	int max=0;
	u_long nLines, maxTotchars;
	int validRequest;
	const int maxUri = HTTP_REQUEST_URI_DIM + 200 ;
	const char cmdSeps[]   = ": ,\t\n\r";

	char *token=0;
	char command[96];

	int nLineControlled = 0;
	int lineControlled = 0;

	/*! 
   *TokenOff is the length of the token starting from 
   *the location token.  
   */
	int tokenOff;

	/*! If input was not specified use the buffer. */
	if(input==0)
	{
		token=input=td->buffer->getBuffer();
	}
	else
		token=input;

	/*!
   *Control if the HTTP header is a valid header.
   */
	validRequest=validHTTPRequest(input, td, &nLines, &maxTotchars);

	/*! Invalid header.  */
	if(validRequest!=e_200)
  {
    /*! Incomplete header.  */
    if(validRequest==-1)
			return -1;
		/* Keep trace of first line for logging. */
		tokenOff = getEndLine(input, HTTP_REQUEST_URI_DIM);
		if(tokenOff > 0)
			request->uri.assign( input, min(HTTP_REQUEST_URI_DIM, tokenOff) );
		else
      request->uri.assign(input, HTTP_REQUEST_URI_DIM );
		return validRequest;
	}

  /*! Get the first token, this is the HTTP command.*/
	tokenOff = getCharInString(token, cmdSeps, HTTP_REQUEST_CMD_DIM);
  
  if(tokenOff == -1)
  {
    /*! Keep trace of first line for logging. */
    tokenOff = getEndLine(token, HTTP_REQUEST_URI_DIM);
    if(tokenOff > 0)
      request->uri.assign(input, min(HTTP_REQUEST_URI_DIM, tokenOff) );
    else
      request->uri.assign(input, HTTP_REQUEST_URI_DIM );
  }
	
  do
	{
		if(tokenOff== -1 )
			return e_400;
		
		/*! Copy the HTTP field(this is the command if we are on the first line). */
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
			u_long len_token =tokenOff;
			/*!
       *The first line has the form:
       *GET /index.html HTTP/1.1
       */
			lineControlled=1;
			/*! Copy the method type. */
			request->cmd.assign(command, tokenOff);
			tokenOff = getEndLine(token, 
                            HTTP_REQUEST_VER_DIM + HTTP_REQUEST_URI_DIM+10);
			len_token = tokenOff;
			if(tokenOff==-1)
        {
          request->ver.clear();
          request->cmd.clear();
          tokenOff = getEndLine(input, HTTP_REQUEST_URI_DIM);
          if(tokenOff > 0)
            request->uri.assign(input, min(HTTP_REQUEST_URI_DIM, tokenOff) );
          else
            request->uri.assign(input, HTTP_REQUEST_URI_DIM );
          return e_400;
        }
      if(tokenOff > maxUri)
        {
          request->ver.clear();
          request->cmd.clear();
          tokenOff = getEndLine(input, HTTP_REQUEST_URI_DIM);
          if(tokenOff > 0)
            request->uri.assign(input, min(HTTP_REQUEST_URI_DIM, tokenOff) );
          else
            request->uri.assign(input, HTTP_REQUEST_URI_DIM );
          return e_400;
        }
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
        else if(token[i]==' ')
				{
					break;
				}
			}

      /*! 
       *If a uri was specified store it. If it wasn't specified 
       *return an invalid header value.
       */
      if(i)
        request->uri.assign(token, i);
      else
      {
        request->ver.clear();
        request->cmd.clear();
        tokenOff = getEndLine(input, HTTP_REQUEST_URI_DIM);
        if(tokenOff > 0)
          request->uri.assign(input, min(HTTP_REQUEST_URI_DIM, tokenOff) );
        else
          request->uri.assign(input, HTTP_REQUEST_URI_DIM );
        return e_400;
      }  

      /*! If the uri contains some query data determine how long it is. */
			if(containOpts)
			{
        j = getEndLine(&token[i], HTTP_REQUEST_URI_DIM);
				for(j=0;((int)(i+j+1)<max) && (j<HTTP_REQUEST_URI_OPTS_DIM-1);j++)
				{
					++j;
				}
			}

      /*! 
       *Save the query data and seek the cursor at the end of it. Start copying
       *from the second byte(do not store the  ? character).
       */
      request->uriOpts.assign(&token[i+1], j);
      i+=j+1;

      /*! 
       *Seek the cursor at the end of the spaces. Do not allow more than 
       *10 spaces character between the uri token and the HTTP version. 
       */
      for(j=0; j<10; j++)
      {
        if(token[i]==' ')
          i++;
        else 
          break;
      }
      /*! 
       *If there are more than 10 black spaces store the entire line 
       *for logging then return an invalid header value.
       */
      if(j == 10)
      {
        request->ver.clear();
        request->cmd.clear();
        tokenOff = getEndLine(input, HTTP_REQUEST_URI_DIM);
        if(tokenOff > 0)
          request->uri.assign(input, min(HTTP_REQUEST_URI_DIM, tokenOff) );
        else
          request->uri.assign(input, HTTP_REQUEST_URI_DIM );
        return e_400;
      }

      /*! Count how long the version token is. */
      j=i;
      while((i-j < HTTP_REQUEST_VER_DIM) && (token[i]!='\r') 
            && (token[i]!='\n'))
        i++;

      /*! Save the HTTP version. */
      if(i-j)
        request->ver.assign(&token[j], i-j);

      /*! 
       *If the version is not specified or it is too long store 
       *some information for logging then return an invalid header value. 
       */
      if((!j) || ( (i-j) == HTTP_REQUEST_VER_DIM ))
      {
        request->ver.clear();
        request->cmd.clear();   
        tokenOff = getEndLine(input, HTTP_REQUEST_URI_DIM);
        if(tokenOff > 0)
          request->uri.assign(input, min(HTTP_REQUEST_URI_DIM, tokenOff) );
        else
          request->uri.assign(input, HTTP_REQUEST_URI_DIM );
        return e_400;
      }
			
      /*! Store if the requested uri terminates with a slash character. */
      if(request->uri[(request->uri.length())-1]=='/')
      {
				request->uriEndsWithSlash=1;
 			}
      else
      {
				request->uriEndsWithSlash=0;
      }
      /*! 
       *Do not maintain any slash character if the uri has them at 
       *the begin or at the end.
       */
      request->uri=trimRight(request->uri, " /");
			request->uriOpts=trim(request->uriOpts, " ");

		}else
		/*!User-Agent*/
		if(!lstrcmpi(command,"User-Agent"))
		{
			tokenOff = getEndLine(token, HTTP_REQUEST_USER_AGENT_DIM);
      if(tokenOff==-1)return e_400;
			lineControlled=1;
			request->userAgent.assign(token,tokenOff+1);
		}else
		/*!Authorization*/
		if(!lstrcmpi(command,"Authorization"))
		{
			while(*token==' ')
				token++;
			tokenOff = getCharInString(token," ",HTTP_REQUEST_AUTH_DIM);
						
			if(tokenOff==-1)return e_400;
		
			lineControlled=1;
	
			request->auth.assign(token,tokenOff);
      td->connection->setLogin("");
      td->connection->setPassword("");
      td->identity[0]='\0';
			if(!request->auth.compare("Basic"))
			{
				u_long i;
				char *base64 = &token[strlen("Basic ")];
				int len = getEndLine(base64, 64);
				char *tmp = base64 + len - 1;
				char* lbuffer2;
				char* keep_lbuffer2;
        char login[32];
        char password[32];
		    if(len==-1)
					return e_400;		
				while (len > 0 && (*tmp == '\r' || *tmp == '\n'))
				{
					tmp--;
					len--;
				}
				if (len <= 1)
					return e_400;
				lbuffer2=base64Utils.Decode(base64,&len);
				keep_lbuffer2=lbuffer2;
   
				for(i=0;(*lbuffer2!=':') && (i<32);i++)
				{
					login[i]=*lbuffer2++;
          login[i+1]='\0';
				}
				myserver_strlcpy(td->identity,td->connection->getLogin(),32+1);
        lbuffer2++;
				for(i=0;(*lbuffer2)&&(i<31);i++)
				{
					password[i]=*lbuffer2++;
					password[i+1]='\0';
				}
        td->connection->setLogin(login);
        td->connection->setPassword(password);
        tokenOff = getEndLine(token, 100);
        delete keep_lbuffer2;
			}
			else if(!request->auth.compare("Digest"))
			{
				char *digestBuff;
				char *digestToken;
				token+=tokenOff;
				while(*token==' ')
					token++;
				tokenOff = getEndLine(token, 1024);
			  if(tokenOff==-1)
					return e_400;		
				digestBuff=new char[tokenOff+1];
				if(!digestBuff)
					return e_400;
				memcpy(digestBuff,token,tokenOff);
				digestBuff[tokenOff]='\0';
				digestToken = strtok( digestBuff, "=" );
				if(!digestToken)
					return e_400;
				do
				{
					StrTrim(digestToken," ");
					if(!lstrcmpi(digestToken,"nonce"))
					{
						digestToken = strtok( NULL, "," );
            if(digestToken)
            {
              StrTrim(digestToken,"\" ");
              myserver_strlcpy(td->request.digestNonce,digestToken,48+1);
            }
					}
					else if(!lstrcmpi(digestToken,"opaque"))
					{
						digestToken = strtok( NULL, "," );
            if(digestToken)
            {
              StrTrim(digestToken,"\" ");
              myserver_strlcpy(td->request.digestOpaque,digestToken,48+1);
            }
					}
					else if(!lstrcmpi(digestToken,"uri"))
					{
						digestToken = strtok( NULL, "\r\n," );
            if(digestToken)
            {
              StrTrim(digestToken,"\" ");
              myserver_strlcpy(td->request.digestUri,digestToken,1024+1);
            }
					}
					else if(!lstrcmpi(digestToken,"method"))
					{
						digestToken = strtok( NULL, "\r\n," );
            if(digestToken)
            {
              StrTrim(digestToken,"\" ");
              myserver_strlcpy(td->request.digestMethod,digestToken,16+1);
            }
					}	
					else if(!lstrcmpi(digestToken,"qop"))
					{
						digestToken = strtok( NULL, "\r\n," );
            if(digestToken)
            {
              StrTrim(digestToken,"\" ");
              myserver_strlcpy(td->request.digestQop,digestToken,16+1);
            }
					}					
					else if(!lstrcmpi(digestToken,"realm"))
					{
						digestToken = strtok( NULL, "\r\n," );
            if(digestToken)
            {
              StrTrim(digestToken,"\" ");
              myserver_strlcpy(td->request.digestRealm,digestToken,48+1);
            }
					}
					else if(!lstrcmpi(digestToken,"cnonce"))
					{
						digestToken = strtok( NULL, "\r\n," );
            if(digestToken)
            {
              StrTrim(digestToken," \"");
              myserver_strlcpy(td->request.digestCnonce, digestToken, 48+1);
            }
					}
					else if(!lstrcmpi(digestToken, "username"))
					{
						digestToken = strtok( NULL, "\r\n," );
            if(digestToken)
            {
              StrTrim(digestToken, "\" ");
              myserver_strlcpy(td->request.digestUsername, digestToken, 48+1);
              td->connection->setLogin(digestToken);
            }
					}
					else if(!lstrcmpi(digestToken,"response"))
					{
						digestToken = strtok( NULL, "\r\n," );
            if(digestToken)
             {
               StrTrim(digestToken,"\" ");
               myserver_strlcpy(td->request.digestResponse,digestToken,48+1);
             }
					}
					else if(!lstrcmpi(digestToken,"nc"))
					{
						digestToken = strtok( NULL, "\r\n," );
            if(digestToken)
            {
              StrTrim(digestToken,"\" ");
              myserver_strlcpy(td->request.digestNc,digestToken,10+1);
            }
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
			tokenOff = getEndLine(token, HTTP_REQUEST_HOST_DIM);
			if(tokenOff==-1)
        return e_400;
			lineControlled=1;
			request->host.assign(token,tokenOff);
      /*!
       *Do not save the port if specified.
       */
      while(request->host[cur])
      {
        if(request->host[cur]==':')
        {
          request->host[cur]='\0';
          break;
        }
        cur++;
      }
		}else
		/*!Content-Encoding*/
		if(!lstrcmpi(command,"Content-Encoding"))
		{
			tokenOff = getEndLine(token, HTTP_REQUEST_CONTENT_ENCODING_DIM);
			if(tokenOff==-1)return e_400;
			lineControlled=1;
			request->contentEncoding.assign(token,tokenOff);
		}else
		/*!Transfer-Encoding*/
		if(!lstrcmpi(command,"Transfer-Encoding"))
		{
			tokenOff = getEndLine(token, HTTP_REQUEST_TRANSFER_ENCODING_DIM);
			if(tokenOff==-1)return e_400;
			lineControlled=1;
			request->transferEncoding.assign(token,tokenOff);
		}else
		/*!Content-Type*/
		if(!lstrcmpi(command,"Content-Type"))
		{
			tokenOff = getEndLine(token, HTTP_REQUEST_CONTENT_TYPE_DIM);
			if(tokenOff==-1)
        return e_400;
			lineControlled=1;
			request->contentType.assign(token,tokenOff);
		}else
		/*!If-Modified-Since*/
		if(!lstrcmpi(command,"If-Modified-Since"))
		{
			tokenOff = getEndLine(token, HTTP_REQUEST_IF_MODIFIED_SINCE_DIM);
			if(tokenOff==-1)
        return e_400;
			lineControlled=1;
			request->ifModifiedSince.assign(token,tokenOff);
		}else
		/*!Accept*/
		if(!lstrcmpi(command,"Accept"))
		{
			int max = HTTP_REQUEST_ACCEPT_DIM-(int)request->accept.length();
			if(max < 0)
				return e_400;
			tokenOff = getEndLine(token, max);
			if(tokenOff == -1)
        return e_400;
			lineControlled=1;
			request->accept.append(token,tokenOff+1);
		}else
		/*!Accept-Language*/
		if(!lstrcmpi(command,"Accept-Language"))
		{
			tokenOff = getEndLine(token, HTTP_REQUEST_ACCEPT_LANGUAGE_DIM);
			if(tokenOff==-1)
        return e_400;
			lineControlled=1;
			request->acceptLanguage.assign(token,tokenOff);
		}else
		/*!Accept-Charset*/
		if(!lstrcmpi(command,"Accept-Charset"))
		{
			tokenOff = getEndLine(token, HTTP_REQUEST_ACCEPT_CHARSET_DIM);
			if(tokenOff==-1)
        return e_400;
			lineControlled=1;
			request->acceptCharset.assign(token,tokenOff);
		}else
		/*!Accept-Encoding*/
		if(!lstrcmpi(command,"Accept-Encoding"))
		{
			tokenOff = getEndLine(token, HTTP_REQUEST_ACCEPT_ENCODING_DIM);
			if(tokenOff==-1)
        return e_400;
			lineControlled=1;
			request->acceptEncoding.assign(token,tokenOff);
		}else
		/*!Connection*/
		if(!lstrcmpi(command,"Connection"))
		{
			tokenOff = getEndLine(token, HTTP_REQUEST_CONNECTION_DIM);
			if(tokenOff==-1)
        return e_400;
			lineControlled=1;
			request->connection.assign(token,tokenOff);
		}else
		/*!Cookie*/
		if(!lstrcmpi(command,"Cookie"))
		{
			tokenOff = getEndLine(token, HTTP_REQUEST_COOKIE_DIM);
			if(tokenOff==-1)
        return e_400;
			lineControlled=1;
			request->cookie.assign(token,tokenOff);
		}else
		/*!From*/
		if(!lstrcmpi(command,"From"))
		{
			tokenOff = getEndLine(token, HTTP_REQUEST_FROM_DIM);
			if(tokenOff==-1)
        return e_400;
			lineControlled=1;
			request->from.assign(token,tokenOff);
		}else
		/*!Content-Length*/
		if(!lstrcmpi(command,"Content-Length"))
		{
			tokenOff = getEndLine(token, HTTP_REQUEST_CONTENT_LENGTH_DIM);
			if(tokenOff==-1)
        return e_400;
			lineControlled=1;
			request->contentLength.assign(token,tokenOff);
		}else
		/*!Cache-Control*/
		if(!lstrcmpi(command,"Cache-Control"))
		{
			tokenOff = getEndLine(token, HTTP_REQUEST_CACHE_CONTROL_DIM);
			if(tokenOff==-1)
        return e_400;
			lineControlled=1;
			request->cacheControl.assign(token,tokenOff);
		}else
		/*!Range*/
		if(!lstrcmpi(command,"Range"))
		{
      char rangeByteBegin[13];
      char rangeByteEnd[13];
      char *localToken = token;
			int i=0;
			rangeByteBegin[0]='\0';
			rangeByteEnd[0]='\0';
			lineControlled=1;
			tokenOff = getEndLine(token, HTTP_REQUEST_RANGE_TYPE_DIM+30);
			if(tokenOff==-1)
        return e_400;
			do
			{
        i++;
			}
			while((*(++localToken) != '=')&&(i<HTTP_REQUEST_RANGE_TYPE_DIM));

      request->rangeType.assign(localToken, i);

			i=0;
      localToken++;
			do
			{
				rangeByteBegin[i++]=*localToken;
				rangeByteBegin[i]='\0';
			}
			while((*(++localToken) != '-')&&(i<12) && (*localToken != '\r'));
			i=0;
      localToken++;
			do
			{
				rangeByteEnd[i++]=*localToken;
				rangeByteEnd[i]='\0';
			}
			while((*(++localToken) != '\r' )&&(i<12));

      for(i=0;i<static_cast<int>(request->rangeType.length());i++)
        if(request->rangeType[i]== '=')
          request->rangeType[i]='\0';

      for(i=0; i<static_cast<int>(strlen(rangeByteBegin)); i++)
        if(rangeByteBegin[i]== '=')
          rangeByteBegin[i]='\0';

      for(i=0; i<static_cast<int>(strlen(rangeByteEnd)); i++)
        if(rangeByteEnd[i]== '=')
          rangeByteEnd[i]='\0';
			
			if(rangeByteBegin[0] == 0)
      {
				request->rangeByteBegin=0;
      }			
      else
      {
        request->rangeByteBegin=(u_long)atol(rangeByteBegin); 
      }
      if(rangeByteEnd[0]=='\r')
      {
        request->rangeByteEnd=0;
      }
      else
      {
        request->rangeByteEnd=(u_long)atol(rangeByteEnd);
        if(request->rangeByteEnd < request->rangeByteBegin)
          return e_400;
      }
		}else
		/*!Referer*/
		if(!lstrcmpi(command,"Referer"))
		{
			tokenOff = getEndLine(token, HTTP_REQUEST_REFERER_DIM);
			if(tokenOff==-1)return e_400;
			lineControlled=1;
			request->referer.assign(token,tokenOff);
		}else
		/*!Pragma*/
		if(!lstrcmpi(command,"Pragma"))
		{
			tokenOff = getEndLine(token, HTTP_REQUEST_PRAGMA_DIM);
			if(tokenOff==-1)return e_400;
			lineControlled=1;
			request->pragma.assign(token,tokenOff);
		}
    if(!lineControlled)
    {
      tokenOff = getEndLine(token, maxTotchars);
      if(tokenOff==-1)
        return e_400;
      if(!request->other.getData(command))
      {
        HttpRequestHeader::Entry *e = new HttpRequestHeader::Entry(); 
        if(e)
        {
          e->name.assign(command);
          e->value.assign(token, tokenOff);
          if(request->other.insert(command, e))
            delete e;
        }

      }
		}
    token+= tokenOff + 2;
		tokenOff = getCharInString(token,":",maxTotchars);
	}while(((u_long)(token-input)<maxTotchars) && token[0]!='\r');
	/*!
   *END REQUEST STRUCTURE BUILD.
   */
	td->nHeaderChars=maxTotchars;
	return e_200;
}

/*!
 *Build the HTTP RESPONSE HEADER string.
 *If no input is specified the input is the main buffer of the 
 *HttpThreadContext structure.
 *Return 0 on invalid input or internal errors.
 */
int HttpHeaders::buildHTTPResponseHeaderStruct(HttpResponseHeader *response, 
                                                HttpThreadContext *td,char *input)
{
	/*!
   *In this function there is the HTTP protocol parse.
   *The request is mapped into a HttpRequestHeader structure
   *And at the end of this every command is treated
   *differently. We use this mode for parse the HTTP
   *cause especially in the CGI is requested a continous
   *HTTP header access.
   *Before mapping the header in the structure 
   *control if this is a regular request.
   *The HTTP header ends with a \r\n\r\n sequence.
   */
	char *newInput;
	u_long nLines,maxTotchars;
	u_long validResponse;
	const char cmdSeps[]   = ": ,\t\n\r\0";

	int containStatusLine=0;
	char *token=0;
	char command[96];

	int lineControlled = 0;
	int nLineControlled = 0;

	if(input==0)
	{
		input=td->buffer2->getBuffer();
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
		token = strtok( token, ": ,\t\n\r" );
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
      response->ver.assign(command);
		
			token = strtok( NULL, " ,\t\n\r" );
			if(token)
				response->httpStatus=atoi(token);
			
			token = strtok( NULL, "\r\n\0" );
			if(token)
				response->errorType.assign(token);

		}else
		/*!Server*/
		if(!lstrcmpi(command,"Server"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
      if(token)
        response->serverName.assign(token);
		}else
		/*!Location*/
		if(!lstrcmpi(command,"Location"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
      if(token)
        response->location.assign(token);
		}else
		/*!Last-Modified*/
		if(!lstrcmpi(command,"Last-Modified"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
      if(token)
        response->lastModified.assign(token);
		}else
		/*!Status*/
		if(!lstrcmpi(command,"Status"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
      /*! If the response status is different from 200 don't modify it. */
      if(response->httpStatus == 200)
        if(token)
          response->httpStatus=atoi(token);
		}else
		/*!Content-Encoding*/
		if(!lstrcmpi(command,"Content-Encoding"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
      if(token)
        response->contentEncoding.assign(token);
		}else
		/*!Cache-Control*/
		if(!lstrcmpi(command,"Cache-Control"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
      if(token)
        response->cacheControl.assign(token);
		}else
		/*!Date*/
		if(!lstrcmpi(command,"Date"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
      if(token)
        response->date.assign(token);
		}else
		/*!Content-Type*/
		if(!lstrcmpi(command,"Content-Type"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
      if(token)
        response->contentType.assign(token);
		}else
		/*!MIME-Version*/
		if(!lstrcmpi(command,"MIME-Version"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
      if(token)
        response->mimeVer.assign(token);
		}else
		/*!Set-Cookie*/
		if(!lstrcmpi(command,"Set-Cookie"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
      if(token)
      {
        response->cookie.append(token );
        response->cookie.append("\n");/*! Divide multiple cookies. */
      }
		}else
		/*!Content-Length*/
		if(!lstrcmpi(command,"Content-Length"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			response->contentLength.assign(token);
		}else
		/*!Content-Range*/
		if(!lstrcmpi(command,"Content-Range"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
      if(token)
        response->contentRange.assign(token);
		}else
		/*!p3p*/
		if(!lstrcmpi(command,"p3p"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
      if(token)
			response->p3p.assign(token);
		}else
		/*!Connection*/
		if(!lstrcmpi(command,"Connection"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
      if(token)
        response->connection.assign(token);
		}else
		/*!Expires*/
		if(!lstrcmpi(command,"Expires"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
      if(token)
        response->dateExp.assign(token);
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
        if(!response->other.getData(command))
        {
          HttpResponseHeader::Entry *e = new HttpResponseHeader::Entry(); 
          if(e)
          {
            e->name.assign(command);
            e->value.assign(token);
            if(response->other.insert(command, e))
              delete e;
          }

        }
			}
		}
		token = strtok( NULL, cmdSeps );
	}while(token && ((u_long)(token-input)<maxTotchars));
	/*!
   *END REQUEST STRUCTURE BUILD.
   */
	td->nBytesToRead=maxTotchars;
	delete [] input;
	return validResponse;
}

/*!
 *Controls if the req string is a valid HTTP request header.
 *Returns e_200 if is a valid request.
 *Returns -1 if the request is incomplete.
 *Any other returned value is the HTTP error.
 *nLinesptr is a value of the lines number in the HEADER.
 *ncharsptr is a value of the characters number in the HEADER.
 */
int HttpHeaders::validHTTPRequest(char *req, HttpThreadContext* td,
                                   u_long* nLinesptr,u_long* ncharsptr)
{
	u_long i=0;
	u_long nLinechars=0;
	nLinechars=0;
	u_long nLines=0;
	
	if(req==0)
		return e_400;
	
	for(;(i<MYSERVER_KB(8));i++)
	{
		if(req[i]=='\n')
		{
			if(req[i+2]=='\n')
			{
				if((i+3) > td->buffer->getRealLength())
					return e_400;
				break;
			}
			/*! 
			*If the lines number is greater than 25 we consider 
			*the header invalid.
			*/
			if(nLines>=25)
				return e_400;
			nLinechars=0;
			nLines++;
		}
		else if(req[i]=='\0')
      return -1;
		/*!
		*We set a maximal theorical number of characters in a line to 2048.
		*If a line contains more than N characters we consider the header invalid.
		*/
		if(nLinechars>=2048)
    {
      if(nLines == 0)
        return e_414;
			return e_400;
    }
		nLinechars++;
	}

	/*! Set the output variables.  */
	*nLinesptr=nLines;
	*ncharsptr=i+3;
	
	/*! Return if is a valid request header.  */
	return e_200;
}
