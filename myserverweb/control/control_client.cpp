/*
 *MyServer
 *Copyright (C) 2002,2003,2004 The MyServer Team
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "control_client.h"

//#define DEBUG
/* Max read of header before failure */
#define MAXHEADERLEN  2048
/* Max wait time before failure */
#define TIMEOUT       30000
/* Buffer size.  Make smaller for more dramatic effect */
#define BUFFSIZE      16

ControlClient::ControlClient()
{
   clearCallback();
   Connected = false;
   Buffer.free();
   UserName[0] = '\0';
   memset(UserPass, 0, 64);
   memset(LastCode, 0, 4);
}

ControlClient::~ControlClient()
{
   clearCallback();
   memset(UserPass, 0, 64); // no memory snoops here
   Buffer.free();
   if(Connected)
     socket.closesocket();
}

void ControlClient::setCallback(ControlClientCallback callback, void * parm)
{
   Progress = callback;
   Object = parm;
}

void ControlClient::clearCallback()
{
   Progress = NULL;
   Object = NULL;
}

int ControlClient::Login(const char * address, const int port,
			 const char * name, const char * pass)
{
   if(address == NULL || port == 0 || name == NULL || pass == NULL)
     return -1;

   if(Connected)
     Logout();

   MYSERVER_HOSTENT *hp = Socket::gethostbyname(address);
   if(hp == NULL)
     return -2;

   int ret;

   struct sockaddr_in sockAddr;
   int sockLen = sizeof(sockAddr);
   memset(&sockAddr, 0, sizeof(sockAddr));
   sockAddr.sin_family = AF_INET;
   memcpy(&sockAddr.sin_addr, hp->h_addr, hp->h_length);
   sockAddr.sin_port = htons(port);
   
   /*! Try to create the socket. */
   if(socket.socket(AF_INET, SOCK_STREAM, 0, 1) == -1)
     return -2;

   /*! If the socket was created try to connect. */
   if(socket.connect((MYSERVER_SOCKADDR*)&sockAddr, sockLen) == -1)
     {
       socket.closesocket();
       return -2;
     }

   Connected = true;

   // Socket connected, test if we are authorized
   strncpy(UserName, name, 64);
   strncpy(UserPass, pass, 64);
   ret = sendRequest("VERSION", "");
   if(ret) // bad thing happend
     {
	Logout();
	return -3;
     }
   ret = getResponse();
   if(ret)
     {
	Logout();
	if(ret == -1)
	  return -3;
	return ret;
     }

   Buffer.setLength(0);
   return 0;
}

int ControlClient::Logout()
{
   memset(UserPass, 0, 64); // no memory snoops here
   Buffer.setLength(0);
   if(Connected)
     socket.closesocket();
   Connected = false;
   return 0;
}

int ControlClient::getVersion(MemBuf & data)
{
   int ret;
   ret = sendRequest("VERSION", "");
   if(ret)
     return -1;
   ret = getResponse();
   if(ret)
     return ret;
   if(DataPos == -1)
     return -1;
   Buffer.getPart(DataPos, Buffer.getLength(), data);
   Buffer.setLength(0);
   return 0;
}

int ControlClient::sendReboot()
{
   int ret;
   ret = sendRequest("REBOOT", "");
   if(ret)
     return -1;
   return 0;
}

int ControlClient::getMyserverConf(MemBuf & data)
{
   int ret;
   ret = sendRequest("GETFILE", "myserver.xml");
   if(ret)
     return -1;
   ret = getResponse();
   if(ret)
     return ret;
   if(DataPos == -1)
     return -1;
   Buffer.getPart(DataPos, Buffer.getLength(), data);
   Buffer.setLength(0);
   return 0;
}

int ControlClient::getVhostsConf(MemBuf & data)
{
   int ret;
   ret = sendRequest("GETFILE", "virtualhosts.xml");
   if(ret)
     return -1;
   ret = getResponse();
   if(ret)
     return ret;
   if(DataPos == -1)
     return -1;
   Buffer.getPart(DataPos, Buffer.getLength(), data);
   Buffer.setLength(0);
   return 0;
}

int ControlClient::getMIMEtypesConf(MemBuf & data)
{
   int ret;
   ret = sendRequest("GETFILE", "MIMEtypes.xml");
   if(ret)
     return -1;
   ret = getResponse();
   if(ret)
     return ret;
   if(DataPos == -1)
     return -1;
   Buffer.getPart(DataPos, Buffer.getLength(), data);
   Buffer.setLength(0);
   return 0;
}

int ControlClient::sendMyserverConf(MemBuf & data)
{
   int ret;
   ret = sendRequest("PUTFILE", "myserver.xml", data);
   if(ret)
     return -1;
   ret = getResponse();
   if(ret)
     return ret;
   Buffer.setLength(0);
   return 0;
}

int ControlClient::sendVhostsConf(MemBuf & data)
{
   int ret;
   ret = sendRequest("PUTFILE", "virtualhosts.xml", data);
   if(ret)
     return -1;
   ret = getResponse();
   if(ret)
     return ret;
   Buffer.setLength(0);
   return 0;
}

int ControlClient::sendMIMEtypesConf(MemBuf & data)
{
   int ret;
   ret = sendRequest("PUTFILE", "MIMEtypes.xml", data);
   if(ret)
     return -1;
   ret = getResponse();
   if(ret)
     return ret;
   Buffer.setLength(0);
   return 0;
}

int ControlClient::getLanguages(Vector & list)
{
   int ret;
   ret = sendRequest("SHOWLANGUAGEFILES", "");
   if(ret)
     return -1;
   ret = getResponse();
   if(ret)
     return ret;
   if(DataPos == -1)
     return -1;
   /* Warning: this is destructive to Buffer */
   list.clear();
   int i, len;
   char * chrptr = &Buffer[DataPos];
   len = Buffer.getLength();
   for(i = DataPos; i < len; i++) // slow but not critical
     {
	if(Buffer[i] == '\r')
	  {
	     Buffer[i] = '\0';
	     list.add(chrptr);
	     chrptr = &Buffer[i] + 2; // avoid error checking
	  } // if
     } // for
   Buffer.setLength(0);
   return 0;
}

int ControlClient::getDynamicProtocols(Vector & list)
{
   int ret;
   ret = sendRequest("SHOWDYNAMICPROTOCOLS", "");
   if(ret)
     return -1;
   ret = getResponse();
   if(ret)
     return ret;
   list.clear();
   if(DataPos == -1)
     return 0; // empty list
   /* Warning: this is destructive to Buffer */
   int i, len;
   char * chrptr = &Buffer[DataPos];
   len = Buffer.getLength();
   for(i = DataPos; i < len; i++) // slow but not critical
     {
	if(Buffer[i] == '\r')
	  {
	     Buffer[i] = '\0';
	     list.add(chrptr);
	     chrptr = &Buffer[i] + 2; // avoid error checking
	  } // if
     } // for
   Buffer.setLength(0);
   return 0;
}

int ControlClient::getConnections(Vector & list)
{
   int ret;
   ret = sendRequest("SHOWCONNECTIONS", "");
   if(ret)
     return -1;
   ret = getResponse();
   if(ret)
     return ret;
   list.clear();
   if(DataPos == -1)
     return 0; // empty list
   /* Warning: this is destructive to Buffer */
   int i, len;
   char * chrptr = &Buffer[DataPos];
   len = Buffer.getLength();
   for(i = DataPos; i < len; i++) // slow but not critical
     {
	if(Buffer[i] == '\r')
	  {
	     Buffer[i] = '\0';
	     list.add(chrptr);
	     chrptr = &Buffer[i] + 2; // avoid error checking
	  } // if
     } // for
   Buffer.setLength(0);
   return 0;
}

int ControlClient::sendKillConnection(VectorNode * node)
{
   char * chrptr = (char *)memchr(node->Text, '-', strlen(node->Text));
   if(chrptr == NULL)
     return -1;
   int len = (int)(chrptr - node->Text);
   char id[len];
   memcpy(id, node->Text, len);
   id[len] = '\0';
   int ret;
   ret = sendRequest("KILLCONNECTION", id);
   if(ret)
     return -1;
   ret = getResponse();
   if(ret)
     return ret;
   Buffer.setLength(0);
   return 0;
}

int ControlClient::sendDisableReboot()
{
   int ret;
   ret = sendRequest("DISABLEREBOOT", "");
   if(ret)
     return -1;
   ret = getResponse();
   if(ret)
     return ret;
   return 0;
}

int ControlClient::sendEnableReboot()
{
   int ret;
   ret = sendRequest("ENABLEREBOOT", "");
   if(ret)
     return -1;
   ret = getResponse();
   if(ret)
     return ret;
   return 0;
}

int ControlClient::sendRequest(const char * cmd, const char * opt)
{
   memset(LastCode, 0, 4);
   if(!Connected)
     return -1;

   int ret;
   Buffer.setLength(0);
   Buffer << "/" << cmd << " CONTROL/1.0 " << opt << "\r\n";
   Buffer << "/CONNECTION Keep-Alive\r\n";
   Buffer << "/LEN 0\r\n";
   Buffer << "/AUTH " << UserName << ":" << UserPass << "\r\n";
   Buffer << "\r\n";
   ret = socket.send((const char *)Buffer.getBuffer(), Buffer.getLength(), 0);
#ifdef DEBUG
   write(1, (const char *)Buffer.getBuffer(), Buffer.getLength());
#endif   
   Buffer.setLength(0);
   return (ret == -1 ? -1 : 0);
}

int ControlClient::sendRequest(const char * cmd, const char * opt, MemBuf & data)
{
   memset(LastCode, 0, 4);
   if(!Connected)
     return -1;
   MemBuf tmp;
   int ret1, ret2;
   int len, pos;
   int bytes;
   tmp.intToStr(data.getLength());
   Buffer.setLength(0);
   Buffer << "/" << cmd << " CONTROL/1.0 " << opt << "\r\n";
   Buffer << "/CONNECTION Keep-Alive\r\n";
   Buffer << "/LEN " << tmp << "\r\n";
   Buffer << "/AUTH " << UserName << ":" << UserPass << "\r\n";
   Buffer << "\r\n";
   ret1 = socket.send((const char *)Buffer.getBuffer(), Buffer.getLength(), 0);
#ifdef DEBUG
   write(1, (const char *)Buffer.getBuffer(), Buffer.getLength());
#endif
   // send small chuncks for some user feedback
   len = data.getLength();
   pos = 0;
   while(pos < len)
     {
	bytes = (len - pos < BUFFSIZE ? len - pos : BUFFSIZE);
	ret2 = socket.send((const char *)&data[pos], bytes, 0);
	if(ret2 == -1)
	  break;
	pos += ret2;
	// callback function
	if(Progress)
	  Progress(Object, len, pos);
     }
   Buffer.setLength(0);
#ifdef DEBUG
   if(ret1 == -1)
     write(1, "ret1 is -1\n", strlen("ret1 is -1\n"));
   if(ret2 == -1)
     write(1, "ret2 is -1\n", strlen("ret2 is -1\n"));
#endif
   return (ret1 == -1 || ret2 == -1 ? -1 : 0);
}

/*!
 * Try to receive data from the server.
 * Returns -1 on errors.
 * Returns 1 if there are not bytes to read.
 * Returns 2 if the header is incomplete.
 * Returns 3 on auth not valid.
 * Returns 0 on success.
 */
int ControlClient::getResponse()
{
   if(!Connected)
     return -1;
//   if(!socket.bytesToRead())
//     return  1;

   int ret = 0;
   int hLen = 0;
   int returnLEN = 0;
   char cBuffer[BUFFSIZE];
   Buffer.setLength(0);
#ifdef DEBUG
   write(1, "Find header:", strlen("Find header:"));
#endif
   while(Buffer.find("\r\n\r\n", 4, 0) == (u_int)-1) // a little costly, may change
     {
	ret = socket.recv(cBuffer, BUFFSIZE, 0, TIMEOUT);

	if(ret == -1)
	  {
	     HeaderGetReturn(); // get the code if any
	     return -1;
	  }

	Buffer.addBuffer((const void *)cBuffer, ret);
	
	if(Buffer.getLength() > MAXHEADERLEN)
	  {
	     HeaderGetReturn(); // get the code if any
	     return -1;
	  }
#ifdef DEBUG
	write(1, cBuffer, ret);
#endif
     }

#ifdef DEBUG
   write(1, "\nDone\n", strlen("\nDone\n"));
#endif
   
   // get header len
   hLen = Buffer.find("\r\n\r\n", 4, 0) + 4;

   // process the header
   ret = HeaderGetReturn();

   if(ret != CONTROL_OK)
     {
	if(ret == -1)
	  return 2;
	else if(ret == CONTROL_AUTH)
	  return 3;
	else
	  {
#ifdef DEBUG
	     write(1, "HeaderGetReturn is -1\n", strlen("HeaderGetReturn is -1\n"));
#endif
	     return -1;
	  }
     }

   returnLEN = HeaderGetLEN();
   
   if(returnLEN == -1)
     return -1;

   if(returnLEN > 0)  // we have data so get it
     {
#ifdef DEBUG
	write(1, "Get data:", strlen("Get data:"));
#endif
	DataPos = hLen;
	while(Buffer.getLength() < (u_int)(hLen + returnLEN))
	  {
	     ret = socket.recv(cBuffer, BUFFSIZE, 0, TIMEOUT);

	     if(ret == -1)
	       return -1;  // problem here
#ifdef DEBUG
	     write(1, cBuffer, ret);
#endif
	     Buffer.addBuffer((const void *)cBuffer, ret);
	     
	     // Callback function
	     if(Progress)
	       Progress(Object, returnLEN, Buffer.getLength() - hLen); 
	  } // while
#ifdef DEBUG
   write(1, "\nDone\n", strlen("\nDone\n"));
#endif
     } // if
   else
     DataPos = -1; // no data

   return 0; // A-OK
}

int ControlClient::HeaderGetReturn()
{
   int pos = Buffer.find('/');
   int end = pos;
   int len = Buffer.getLength();
   int ret;
   
   if(pos == -1)
     return -1;
   
   while(Buffer[end] != '\r')
     {
	end++;
	if(end > len)
	  return -1;
     }
   
   char temp[end - pos];
   memcpy(temp, &Buffer.getAt(pos), end - pos);
   temp[end - pos] = '\0';
   
   ret = atoi(&temp[1]);
   
   strncpy(LastCode, &temp[1], 4);
   
   return ret;
}

int ControlClient::HeaderGetLEN()
{
   int pos = Buffer.find((const void *)"/LEN", 4);
   int end = pos;
   int len = Buffer.getLength();
   
   if(pos == -1)
     return -1;
   
   while(Buffer[end] != '\r')
     {  
	end++;
	if(end > len)
	  return -1;
     }
   
   char temp[end - pos];
   memcpy(temp, &Buffer.getAt(pos), end - pos);
   temp[end - pos] = '\0';
   
   return atoi(&temp[4]);
}

   
