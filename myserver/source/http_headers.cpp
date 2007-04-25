/*
MyServer
Copyright (C) 2002, 2003, 2004, 2007 The MyServer Team
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
#include "../include/mime_utils.h"
#include "../include/file.h"
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
#endif

/*!
 *Builds an HTTP header string starting from an HttpResponseHeader structure.
 *\param str The buffer where write the HTTP header.
 *\param response the HttpResponseHeader where the HTTP data is.
 */
void HttpHeaders::buildHTTPResponseHeader(char *str, 
																					HttpResponseHeader* response)
{
	/*
   *Here we build the HTTP response header.
   *Passing a HttpResponseHeader struct this builds an header string.
   *Every directive ends with a \r\n sequence.
   */
  char *pos = str;
  const int MAX = MYSERVER_KB(8);
	if(response->httpStatus != 200)
	{
		if(response->errorType.length() == 0)
		{
			HttpErrors::getErrorMessage(response->httpStatus, response->errorType);
		}
		pos += sprintf(str, "%s %i %s\r\nStatus: %s\r\n", response->ver.c_str(),
            response->httpStatus, response->errorType.c_str(), 
            response->errorType.c_str() );
	}
	else
    pos += sprintf(str,"%s 200 OK\r\n",response->ver.c_str());
	
	if(response->serverName.length())
	{
		pos += myserver_strlcpy(pos, "Server: ", MAX-(long)(pos-str));
		pos += myserver_strlcpy(pos, response->serverName.c_str(), MAX-(long)(pos-str));
		pos += myserver_strlcpy(pos, "\r\n", MAX-(long)(pos-str));
	}
	else
	{
		pos += myserver_strlcpy(pos, "Server: MyServer ", MAX-(long)(pos-str));
		pos += myserver_strlcpy(pos, versionOfSoftware,	MAX-(long)(pos-str));
		pos += myserver_strlcpy(pos, "\r\n", MAX-(long)(pos-str));
	}

	if(response->lastModified.length())
	{
		pos += myserver_strlcpy(pos,"Last-Modified: ", MAX-(long)(pos-str));
		pos += myserver_strlcpy(pos, response->lastModified.c_str(), 
														MAX-(long)(pos-str));
		pos += myserver_strlcpy(pos,"\r\n", MAX-(long)(pos-str));
	}
	if(response->connection.length())
	{
		pos += myserver_strlcpy(pos,"Connection: ", MAX-(long)(pos-str));
		pos += myserver_strlcpy(pos, response->connection.c_str(), 
														MAX-(long)(pos-str));
		pos += myserver_strlcpy(pos, "\r\n", MAX-(long)(pos-str));
	}
	else
	{
		pos += myserver_strlcpy(pos, "Connection: Close\r\n", MAX-(long)(pos-str));
	}
	if(response->contentLength.length())
	{
		/*
		*Do not specify the Content-Length field if it is used
		*the chunked Transfer-Encoding.
		*/
		HttpResponseHeader::Entry *e = response->other.get("Transfer-Encoding");

		if(!e || (e && e->value->find("chunked",0) == string::npos ))
		{
			pos += myserver_strlcpy(pos, "Content-Length: ", MAX-(long)(pos-str));
			pos += myserver_strlcpy(pos, response->contentLength.c_str(), MAX-(long)(pos-str));
			pos += myserver_strlcpy(pos, "\r\n", MAX-(long)(pos-str));
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
      pos += myserver_strlcpy(pos, cookie.c_str(), MAX-(long)(pos-str));	
			token+=len+1;
		}
	}
	if(response->mimeVer.length())
	{
		pos += myserver_strlcpy(pos, "MIME-Version: ", MAX-(long)(pos-str));
		pos += myserver_strlcpy(pos, response->mimeVer.c_str(), MAX-(long)(pos-str));
		pos += myserver_strlcpy(pos, "\r\n", MAX-(long)(pos-str));
	}
	if(response->contentType.length())
	{
		pos += myserver_strlcpy(pos, "Content-Type: ", MAX-(long)(pos-str));
		pos += myserver_strlcpy(pos, response->contentType.c_str(), MAX-(long)(pos-str));
		pos += myserver_strlcpy(pos, "\r\n", MAX-(long)(pos-str));
	}
	if(response->date.length())
	{
		pos += myserver_strlcpy(pos, "Date: ", MAX-(long)(pos-str));
		pos += myserver_strlcpy(pos, response->date.c_str(), MAX-(long)(pos-str));
		pos += myserver_strlcpy(pos, "\r\n", MAX-(long)(pos-str));
	}
	if(response->dateExp.length())
	{
		pos += myserver_strlcpy(pos, "Expires: ", MAX-(long)(pos-str));
		pos += myserver_strlcpy(pos, response->dateExp.c_str(), MAX-(long)(pos-str));
		pos += myserver_strlcpy(pos, "\r\n", MAX-(long)(pos-str));
	}
	if(response->auth.length())
	{
		pos += myserver_strlcpy(pos, "WWW-Authenticate: ", MAX-(long)(pos-str));
		pos += myserver_strlcpy(pos, response->auth.c_str(), MAX-(long)(pos-str));
		pos += myserver_strlcpy(pos, "\r\n", MAX-(long)(pos-str));
	}
	
	if(response->location.length())
	{
		pos += myserver_strlcpy(pos, "Location: ", MAX - (long)(pos - str));
		pos += myserver_strlcpy(pos, response->location.c_str(), 
														MAX - (long)(pos-str));
		pos += myserver_strlcpy(pos, "\r\n", MAX - (long)(pos - str));
	}

	if(response->other.size())
	{
		HashMap<string, HttpResponseHeader::Entry*>::Iterator it = 
			response->other.begin();
    for(; it != response->other.end(); it++)
    {
      HttpResponseHeader::Entry *e = *it;
      if(e)
      {
        pos += myserver_strlcpy(pos, e->name->c_str(), MAX-(long)(pos-str));
        pos += myserver_strlcpy(pos, ": ", MAX-(long)(pos-str));
        pos += myserver_strlcpy(pos, e->value->c_str(), MAX-(long)(pos-str));
        pos += myserver_strlcpy(pos, "\r\n", MAX-(long)(pos-str));
      }
      
    }
	}

	/*
	 *MyServer supports the bytes range.
	 */
	pos += myserver_strlcpy(pos, "Accept-Ranges: bytes\r\n", MAX-(long)(pos-str));
  
	/*
	 *The HTTP header ends with a \r\n sequence.
	 */
	pos += myserver_strlcpy(pos, "\r\n\0\0\0\0\0", MAX-(long)(pos-str));
}
/*!
 *Set the defaults value for a HttpResponseHeader structure.
 *\param response The HTTP response header structure to fullfill with
 *the default data.
 */
void HttpHeaders::buildDefaultHTTPResponseHeader(HttpResponseHeader* response)
{
  string date;
  ostringstream stream;
	resetHTTPResponse(response);
	/*!
	 *By default use:
	 *-# the MIME type of the page equal to text/html.
	 *-# the version of the HTTP protocol to 1.1.
	 *-# the date of the page and the expire date to the current time.
	 *-# set the name of the server.
	 *-# set the page that it is not an error page.
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
 *Get the value for name in the hash dictionary.
 *If the key is not present in the hash map then the request
 *is propagated to the virtual host, if it is defined.
 *\param name The key name to look for in the hash map.
 */
const char* HttpThreadContext::getHashedData(const char *name)
{
	Vhost *vh = (Vhost*)connection->host;
	string *ret = other.get(string(name));
	if(ret)
		return ret->c_str();
	else
		return vh ? vh->getHashedData(name) : 0;
}

/*!
 *Get the current vhost doc directory for the environvment.
 */
const char *HttpThreadContext::getVhostDir()
{
	if(vhostDir.length() > 1)
		return vhostDir.c_str();
	if(connection && connection->host)
		return connection->host->getDocumentRoot();
	return "";
}

/*!
 *Get the current vhost sys directory for the environvment.
 */
const char *HttpThreadContext::getVhostSys()
{
	if(vhostSys.length() > 1)
		return vhostSys.c_str();
	if(connection && connection->host)
		return connection->host->getSystemRoot();
	return "";
}

/*!
 *Reset all the HTTP_REQUEST_HEADER structure members.
 *\param request the HTTP request header to free.
 */
void HttpHeaders::resetHTTPRequest(HttpRequestHeader *request)
{
	request->free();
}

/*!
 *Reset all the HttpResponseHeader structure members.
 *\param response the HTTP response header to free.
 */
void HttpHeaders::resetHTTPResponse(HttpResponseHeader *response)
{
	response->free();
}


/*!
 *Controls if the req string is a valid HTTP response header.
 *Returns 0 if req is an invalid header, a non-zero value if is a valid header.
 *\param res the buffer with the HTTP header.
 *\param td the thread context for the current executing thread.
 *\param nLinesptr is a value of the lines number in the HEADER.
 *\param ncharsptr is a value of the characters number in the HEADER.
 */
int HttpHeaders::validHTTPResponse(char *res, HttpThreadContext* td, 
                                    u_long* nLinesptr, u_long* ncharsptr)
{
	u_long i;
	u_long nLinechars = 0;
	u_long nLines = 0;

	if(res == 0)
		return 0;
	/*
	 *Count the number of lines in the header.
	 */
	for(i=0;;i++)
	{
		if(res[i]=='\n')
		{
			if((res[i+2] == '\n') || (res[i+1] == '\0') || (res[i+1] == '\n'))
			{
				if(i + 3 > td->buffersize)
					return 0;
				break;
			}
			nLines++;
		}
		else
		{
			/*
			 *We set a maximal theorical number of characters in a line.
			 *If a line contains more than 4160 characters we consider the 
			 *header invalid.
			 */
			if(nLinechars >= 4160)
	    		return 0;
	    	nLinechars++;
		}
	}
	
	/*
	 *Set the output variables.
	 */
	*nLinesptr = nLines;
	*ncharsptr = i+3;
	
	/*
	 *Return if is a valid request header.
	 */
	return 1;
}


/*!
 *Build the HTTP REQUEST HEADER string.
 *If no input is setted the input is the main buffer of the 
 *HttpThreadContext structure.
 *Returns 200 if is a valid request.
 *Returns -1 if the request is incomplete.
 *Any other returned value is the HTTP error.
 *\param request HTTP request structure to fullfill with data.
 *\param td the current executing thread context.
 *\param input buffer with the HTTP header.
 */
int HttpHeaders::buildHTTPRequestHeaderStruct(HttpRequestHeader *request, 
                                              HttpThreadContext* td, char* input)
{
	/*!
   *In this function there is the HTTP protocol parse.
   *The request is mapped into a HttpRequestHeader structure
   *And at the end of this each command is treated
   *differently. We use this mode to parse the HTTP
   *cause especially in the CGI is requested a continous
   *access to HTTP header data.
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

	/*
   *TokenOff is the length of the token starting from 
   *the location token.  
   */
	int tokenOff;

	/* If input was not specified use the buffer. */
	if(input==0)
	{
		token=input=td->buffer->getBuffer();
	}
	else
		token=input;

	/*
   *Control if the HTTP header is a valid header.
   */
	validRequest=validHTTPRequest(input, td, &nLines, &maxTotchars);

	/* Invalid header.  */
	if(validRequest!=200)
  {
    /* Incomplete header.  */
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

  /* Get the first token, this is the HTTP command.*/
	tokenOff = getCharInString(token, cmdSeps, HTTP_REQUEST_CMD_DIM);
  
  if(tokenOff == -1)
  {
    /* Keep trace of first line for logging. */
    tokenOff = getEndLine(token, HTTP_REQUEST_URI_DIM);
    if(tokenOff > 0)
      request->uri.assign(input, min(HTTP_REQUEST_URI_DIM, tokenOff) );
    else
      request->uri.assign(input, HTTP_REQUEST_URI_DIM );
  }
	
  do
	{
		if(tokenOff== -1 )
			return 400;
		
		/* Copy the HTTP field(this is the command if we are on the first line). */
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
			/*
       *The first line has the form:
       *GET /index.html HTTP/1.1
       */
			lineControlled=1;
			
			/* Copy the method type. */
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
				return 400;
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
				return 400;
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

      /*
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
        return 400;
      }  

      /* If the uri contains some query data determine how long it is. */
			if(containOpts)
			{
        j = getEndLine(&token[i], HTTP_REQUEST_URI_DIM);
				for(j=0;((int)(i+j+1)<max) && (j<HTTP_REQUEST_URI_OPTS_DIM-1);j++)
				{
					++j;
				}
			}

      /*
       *Save the query data and seek the cursor at the end of it. Start copying
       *from the second byte(do not store the  ? character).
       */
      request->uriOpts.assign(&token[i+1], j);
      i+=j+1;

      /*
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
      /*
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
        return 400;
      }

      /* Count how long the version token is. */
      j=i;
      while((i-j < HTTP_REQUEST_VER_DIM) && (token[i]!='\r') 
            && (token[i]!='\n'))
        i++;

      /* Save the HTTP version. */
      if(i-j)
        request->ver.assign(&token[j], i-j);

      /*
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
        return 400;
      }
			
      /* Store if the requested uri terminates with a slash character. */
      request->uriEndsWithSlash = request->uri[(request->uri.length())-1]=='/';

      /*
       *Do not maintain any slash character if the uri has them at 
       *the begin or at the end.
       */
      request->uri=trimRight(request->uri, " /");
			request->uriOpts=trim(request->uriOpts, " ");

		}else
		/* Authorization.  */
		if(!lstrcmpi(command,"Authorization"))
		{
			while(*token==' ')
				token++;
			tokenOff = getCharInString(token," ",HTTP_REQUEST_AUTH_DIM);
						
			if(tokenOff==-1)return 400;
		
			lineControlled=1;
	
			request->auth.assign(token,tokenOff);
      td->connection->setLogin("");
      td->connection->setPassword("");
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
				
		    if(len == -1)
					return 400;		

				login[0] = password[0] = '\0';

				while (len > 0 && (*tmp == '\r' || *tmp == '\n'))
				{
					tmp--;
					len--;
				}
				if (len <= 1)
					return 400;
				lbuffer2 = base64Utils.Decode(base64,&len);
				keep_lbuffer2 = lbuffer2;
   
				for(i = 0; (*lbuffer2 != ':') && (i < 32);i++)
				{
					login[i] = *lbuffer2++;
          login[i+1] = '\0';
				}
        lbuffer2++;
				for(i = 0; (*lbuffer2) && (i < 31); i++)
				{
					password[i] = *lbuffer2++;
					password[i+1] = '\0';
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
				token += tokenOff;
				while(*token == ' ')
					token++;
				tokenOff = getEndLine(token, 1024);
			  if(tokenOff==-1)
					return 400;		
				digestBuff=new char[tokenOff+1];
				if(!digestBuff)
					return 400;
				memcpy(digestBuff,token,tokenOff);
				digestBuff[tokenOff]='\0';
				digestToken = strtok( digestBuff, "=" );
				if(!digestToken)
					return 400;
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
					/* Update digestToken.  */
					digestToken = strtok( NULL, "=" );
				}while(digestToken);
				delete  [] digestBuff;
			}
		}else
		/* Content-Length.  */
		if(!lstrcmpi(command,"Content-Length"))
		{
			tokenOff = getEndLine(token, HTTP_REQUEST_CONTENT_LENGTH_DIM);
			if(tokenOff==-1)
        return 400;
			lineControlled=1;
			request->contentLength.assign(token,tokenOff);
		}else
		/* Range.  */
		if(!lstrcmpi(command,"Range"))
		{
      char rangeByteBegin[13];
      char rangeByteEnd[13];
      char *localToken = token;
			int i=0;
			rangeByteBegin[0] = '\0';
			rangeByteEnd[0] = '\0';
			lineControlled = 1;
			tokenOff = getEndLine(token, HTTP_REQUEST_RANGE_TYPE_DIM+30);
			if(tokenOff ==-1)
        return 400;
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
				rangeByteBegin[i++] = *localToken;
				rangeByteBegin[i] = '\0';
			}
			while((*(++localToken) != '-') && (i<12) && (*localToken != '\r'));
			i = 0;
      localToken++;
			do
			{
				rangeByteEnd[i++] = *localToken;
				rangeByteEnd[i] = '\0';
			}
			while((*(++localToken) != '\r' )&&(i<12));

      for(i=0;i < static_cast<int>(request->rangeType.length()); i++)
        if(request->rangeType[i] == '=')
          request->rangeType[i] = '\0';

      for(i = 0; i < static_cast<int>(strlen(rangeByteBegin)); i++)
        if(rangeByteBegin[i] == '=')
          rangeByteBegin[i] = '\0';

      for(i = 0; i < static_cast<int>(strlen(rangeByteEnd)); i++)
        if(rangeByteEnd[i]== '=')
          rangeByteEnd[i]='\0';
			
			if(rangeByteBegin[0] == 0)
      {
				request->rangeByteBegin=0;
      }			
      else
      {
        request->rangeByteBegin = (u_long)atol(rangeByteBegin); 
      }
      if(rangeByteEnd[0] == '\r')
      {
        request->rangeByteEnd = 0;
      }
      else
      {
        request->rangeByteEnd = (u_long)atol(rangeByteEnd);
        if(request->rangeByteEnd < request->rangeByteBegin)
          return 400;
      }
		}else
    if(!lineControlled)
    {
      tokenOff = getEndLine(token, maxTotchars);
      if(tokenOff==-1)
        return 400;
      
			{
				string cmdStr(command);
				HttpRequestHeader::Entry *old = request->other.get(cmdStr);
				if(old)
				{
					old->value->append(", ");
					old->value->append(token, 
														 std::min(static_cast<int>(HTTP_RESPONSE_OTHER_DIM
																											 - old->value->length()),
																			static_cast<int>(tokenOff)));
				}
				else
				{
					HttpRequestHeader::Entry *e = new HttpRequestHeader::Entry(); 
					if(e)
					{
						e->name->assign(command, HTTP_RESPONSE_OTHER_DIM);
						e->value->assign(token, 
														 std::min(HTTP_RESPONSE_OTHER_DIM, tokenOff));
						request->other.put(cmdStr, e);
					}
				}
			}
		}
    token+= tokenOff + 2;
		tokenOff = getCharInString(token,":",maxTotchars);
	}while(((u_long)(token-input)<maxTotchars) && token[0]!='\r');

	/*
   *END REQUEST STRUCTURE BUILD.
   */
	td->nHeaderChars=maxTotchars;
	return 200;
}

/*!
 *Build the HTTP RESPONSE HEADER string.
 *If no input is specified the input is the main buffer of the 
 *HttpThreadContext structure.
 *Return 0 on invalid input or internal errors.
 *\param response the HTTP response structure to fullfill.
 *\param td the current executing thread context.
 *\param input the buffer with the HTTP header data.
 */
int HttpHeaders::buildHTTPResponseHeaderStruct(HttpResponseHeader *response, 
																							 HttpThreadContext *td,
																							 char *input)
{
	/*!
	 *Brief description.
   *In this function there is the HTTP protocol parse.
   *The request is mapped into a HttpRequestHeader structure
   *And at the end of this every command is treated
   *differently. We use this mode for parse the HTTP
   *cause especially in the CGI is requested a continous
   *HTTP header access.
   *Before mapping the header in the structure 
   *control if this is a regular response.
   */
	char *newInput;
	u_long nLines,maxTotchars;
	u_long validResponse;
	const char cmdSeps[]   = ": ,\t\n\r\0";

	int containStatusLine=0;
	char *token = 0;
	char command[96];

	int lineControlled = 0;
	int nLineControlled = 0;

	if(input == 0)
	{
		input = td->buffer2->getBuffer();
	}
	/* Control if the HTTP header is a valid header.  */
	if(input[0] == 0)
		return 0;
	validResponse = validHTTPResponse(input, td, &nLines, &maxTotchars);

	if(validResponse)
	{
		newInput = new char[maxTotchars + 1];
		if(!newInput)
			return 0;
		/*
		 * FIXME: 
		 * Don't alloc new memory but simply use a no-destructive parsing.  
		 */
		memcpy(newInput, input, maxTotchars);
		newInput[maxTotchars] = '\0';
		input = newInput;
	}
	else
		return 0;

	token = input;
	
	/* Check if is specified the first line containing the HTTP status.  */
	if((input[0] == 'H') && (input[1] == 'T') && (input[2] == 'T')
     &&(input[3] == 'P') && (input[4] == ' '))
	{
		containStatusLine = 1;
		token = strtok( token, " " );
	}
	else
		token = strtok( token, ": ,\t\n\r" );
	do
	{
		if(!token)
			break;
		/*
     *Reset the flag lineControlled.
     */
		lineControlled = 0;

		/*
     *Copy the HTTP command.
     */
		myserver_strlcpy(command, token, 96);
		
		nLineControlled++;
		if((nLineControlled == 1) && containStatusLine)
		{
			lineControlled = 1;
			/* Copy the HTTP version.  */
      response->ver.assign(command);
		
			token = strtok( NULL, " ,\t\n\r" );
			if(token)
				response->httpStatus = atoi(token);
			
			token = strtok( NULL, "\r\n\0" );
			if(token)
				response->errorType.assign(token);

		}else
		/* Server.  */
		if(!lstrcmpi(command,"Server"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
      if(token)
        response->serverName.assign(token);
		}else
		/* Location  */
		if(!lstrcmpi(command,"Location"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
      if(token)
        response->location.assign(token);
		}else
		/* Last-Modified.  */
		if(!lstrcmpi(command,"Last-Modified"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
      if(token)
        response->lastModified.assign(token);
		}else
		/* Status.  */
		if(!lstrcmpi(command,"Status"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
      /*! If the response status is different from 200 don't modify it. */
      if(response->httpStatus == 200)
        if(token)
          response->httpStatus=atoi(token);
		}else
		/* Date.  */
		if(!lstrcmpi(command,"Date"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
      if(token)
        response->date.assign(token);
		}else
		/* Content-Type.  */
		if(!lstrcmpi(command,"Content-Type"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
      if(token)
        response->contentType.assign(token);
		}else
		/* MIME-Version.  */
		if(!lstrcmpi(command,"MIME-Version"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
      if(token)
        response->mimeVer.assign(token);
		}else
		/* Set-Cookie.  */
		if(!lstrcmpi(command,"Set-Cookie"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
      if(token)
      {
				/* Divide multiple cookies.  */
        response->cookie.append(token );
        response->cookie.append("\n");
      }
		}else
		/* Content-Length.  */
		if(!lstrcmpi(command,"Content-Length"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled=1;
			response->contentLength.assign(token);
		}else
		/* Connection.  */
		if(!lstrcmpi(command,"Connection"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled = 1;

      if(token)
        response->connection.assign(token);
		}else
		/* Expires.  */
		if(!lstrcmpi(command,"Expires"))
		{
			token = strtok( NULL, "\r\n\0" );
			lineControlled = 1;
      if(token)
        response->dateExp.assign(token);
		}
		/*
     *If the line is not controlled arrive with the token
     *at the end of the line.
     */
		if( (!lineControlled) &&  ((!containStatusLine) || (nLineControlled!=1)) )
		{
			token = strtok(NULL, "\r\n");
			if(token)
			{
				HttpResponseHeader::Entry *e;
				if(strlen(command) > HTTP_RESPONSE_OTHER_DIM || 
					 strlen(token) > HTTP_RESPONSE_OTHER_DIM)
					return 0;

				e = new HttpResponseHeader::Entry(); 
				if(e)
        {
					e->name->assign(command);
					e->value->assign(token);
					{
						HttpResponseHeader::Entry *old = 0;
						string cmdString(command);
						old = response->other.put(cmdString, e);
						if(old)
							delete old;
					}
				}
			}
		}
		token = strtok(NULL, cmdSeps);
	}while(token && ((u_long)(token - input) < maxTotchars));

	/*
   *END REQUEST STRUCTURE BUILD.
   */
	td->nBytesToRead = maxTotchars;
	delete [] input;
	return validResponse;
}

/*!
 *Controls if the req string is a valid HTTP request header.
 *Returns 200 if is a valid request.
 *Returns -1 if the request is incomplete.
 *Any other returned value is the HTTP error.
 *\param req the buffer with the HTTP request.
 *\param td the current executing thread context.
 *\param nLinesptr is a value of the lines number in the HEADER.
 *\param ncharsptr is a value of the characters number in the HEADER.
 */
int HttpHeaders::validHTTPRequest(char *req, HttpThreadContext* td,
                                   u_long* nLinesptr,u_long* ncharsptr)
{
	u_long i=0;
	u_long nLinechars = 0;
	nLinechars = 0;
	u_long nLines = 0;
	
	if(req == 0)
		return 400;
	
	for(;(i < MYSERVER_KB(8)); i++)
	{
		if(req[i]=='\n')
		{
			if(req[i + 2]=='\n')
			{
				if((i + 3) > td->buffer->getRealLength())
					return 400;
				break;
			}
			/* 
			 *If the lines number is greater than 25 we consider 
			 *the header invalid.
			*/
			if(nLines >= 25)
				return 400;
			nLinechars = 0;
			nLines++;
		}
		else if(req[i]=='\0')
      return -1;
		/*
		*We set a maximal theorical number of characters in a line to 2048.
		*If a line contains more than N characters we consider the header invalid.
		*/
		if(nLinechars >= 2048)
    {
      if(nLines == 0)
        return 414;
			return 400;
    }
		nLinechars++;
	}

	/* Set the output variables.  */
	*nLinesptr = nLines;
	*ncharsptr = i+3;
	
	/* Return if is a valid request header.  */
	return 200;
}
