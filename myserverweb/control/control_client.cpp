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

ControlClient::ControlClient()
{
   Connected = false;
   Buffer.Free();
   UserName[0] = '\0';
   memset(UserPass, 0, 64);
   memset(LastCode, 0, 4);
}

ControlClient::~ControlClient()
{
   memset(UserPass, 0, 64); // no memory snoops here
   Buffer.Free();
   if(Connected)
     socket.closesocket();
}

int ControlClient::Login(const char * address, const int port,
			 const char * name, const char * pass)
{
   if(address == NULL || port == 0 || name == NULL || pass == NULL)
     return -1;

   if(Connected)
     Logout();

   MYSERVER_HOSTENT *hp = MYSERVER_SOCKET::gethostbyname(address);
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
   if(socket.socket(AF_INET, SOCK_STREAM, 0) == -1)
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

   Buffer.Free();
   return 0;
}

int ControlClient::Logout()
{
   memset(UserPass, 0, 64); // no memory snoops here
   Buffer.Free();
   if(Connected)
     socket.closesocket();
   Connected = false;
   return 0;
}

int ControlClient::getVersion(CMemBuf & data)
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
   Buffer.GetPart(DataPos, Buffer.GetLength(), data);
   Buffer.Free();
   return 0;
}

int ControlClient::sendReboot()
{
   int ret;
   ret = sendRequest("REBOOT", "");
   if(ret)
     return -1;
   ret = getResponse();
   if(ret)
     return ret;
   Buffer.Free();
   return 0;
}

int ControlClient::getMyserverConf(CMemBuf & data)
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
   Buffer.GetPart(DataPos, Buffer.GetLength(), data);
   Buffer.Free();
   return 0;
}

int ControlClient::getVhostsConf(CMemBuf & data)
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
   Buffer.GetPart(DataPos, Buffer.GetLength(), data);
   Buffer.Free();
   return 0;
}

int ControlClient::getMIMEtypesConf(CMemBuf & data)
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
   Buffer.GetPart(DataPos, Buffer.GetLength(), data);
   Buffer.Free();
   return 0;
}

int ControlClient::sendMyserverConf(CMemBuf & data)
{
   int ret;
   ret = sendRequest("PUTFILE", "myserver.xml", data);
   if(ret)
     return -1;
   ret = getResponse();
   if(ret)
     return ret;
   Buffer.Free();
   return 0;
}

int ControlClient::sendVhostsConf(CMemBuf & data)
{
   int ret;
   ret = sendRequest("PUTFILE", "virtualhosts.xml", data);
   if(ret)
     return -1;
   ret = getResponse();
   if(ret)
     return ret;
   Buffer.Free();
   return 0;
}

int ControlClient::sendMIMEtypesConf(CMemBuf & data)
{
   int ret;
   ret = sendRequest("PUTFILE", "MIMEtypes.xml", data);
   if(ret)
     return -1;
   ret = getResponse();
   if(ret)
     return ret;
   Buffer.Free();
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
   len = Buffer.GetLength();
   for(i = DataPos; i < len; i++) // slow but not critical
     {
	if(Buffer[i] == '\r')
	  {
	     Buffer[i] = '\0';
	     list.add(chrptr);
	     chrptr = &Buffer[i] + 2; // avoid error checking
	  } // if
     } // for
   Buffer.Free();
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
   if(DataPos == -1)
     return -1;
   /* Warning: this is destructive to Buffer */
   list.clear();
   int i, len;
   char * chrptr = &Buffer[DataPos];
   len = Buffer.GetLength();
   for(i = DataPos; i < len; i++) // slow but not critical
     {
	if(Buffer[i] == '\r')
	  {
	     Buffer[i] = '\0';
	     list.add(chrptr);
	     chrptr = &Buffer[i] + 2; // avoid error checking
	  } // if
     } // for
   Buffer.Free();
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
   if(DataPos == -1)
     return -1;
   /* Warning: this is destructive to Buffer */
   list.clear();
   int i, len;
   char * chrptr = &Buffer[DataPos];
   len = Buffer.GetLength();
   for(i = DataPos; i < len; i++) // slow but not critical
     {
	if(Buffer[i] == '\r')
	  {
	     Buffer[i] = '\0';
	     list.add(chrptr);
	     chrptr = &Buffer[i] + 2; // avoid error checking
	  } // if
     } // for
   Buffer.Free();
   return 0;
}

int ControlClient::sendKillConnection(VectorNode * node)
{
   char * id = strdup(node->Text);
   char * chrptr = strchr(id, '-');
   chrptr = '\0';
   int ret;
   ret = sendRequest("KILLCONNECTION", id);
   free(id);
   if(ret)
     return -1;
   ret = getResponse();
   if(ret)
     return ret;
   Buffer.Free();
   return 0;
}

int ControlClient::sendRequest(const char * cmd, const char * opt)
{
   if(!Connected)
     return -1;

   int ret;
   Buffer.Free();
   Buffer << "/" << cmd << " CONTROL/1.0 " << opt << "\r\n";
   Buffer << "/CONNECTION Keep-Alive\r\n";
   Buffer << "/LEN 0\r\n";
   Buffer << "/AUTH " << UserName << ":" << UserPass << "\r\n";
   Buffer << "\r\n";
   ret = socket.send((const char *)Buffer.GetBuffer(), Buffer.GetLength(), 0);
#ifdef DEBUG
   write(1, (const char *)Buffer.GetBuffer(), Buffer.GetLength());
#endif   
   Buffer.Free();
   return (ret == -1 ? -1 : 0);
}

int ControlClient::sendRequest(const char * cmd, const char * opt, CMemBuf & data)
{
   if(!Connected)
     return -1;

   int ret1, ret2;
   Buffer.Free();
   Buffer << "/" << cmd << " CONTROL/1.0 " << opt << "\r\n";
   Buffer << "/CONNECTION Keep-Alive\r\n";
   Buffer << "/LEN " << CMemBuf::IntToStr(data.GetLength()) << "\r\n";
   Buffer << "/AUTH " << UserName << ":" << UserPass << "\r\n";
   Buffer << "\r\n";
   ret1 = socket.send((const char *)Buffer.GetBuffer(), Buffer.GetLength(), 0);
#ifdef DEBUG
   write(1, (const char *)Buffer.GetBuffer(), Buffer.GetLength());
#endif
   // TODO: change to send smaller chunks (for progress indication)
   ret2 = socket.send((const char *)data.GetBuffer(), data.GetLength(), 0);
   Buffer.Free();
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
   char cBuffer[1024];
   Buffer.Free();
#ifdef DEBUG
   write(1, "Find header:", strlen("Find header:"));
#endif
   while(Buffer.Find("\r\n\r\n", 4, 0) == -1) // a little costly, may change
     {
	ret = socket.recv(cBuffer, 1024, 0);

	if(ret == -1)
	  {
	     HeaderGetReturn(); // get the code if any
	     return -1;
	  }

	Buffer.AddBuffer((const void *)cBuffer, ret);
#ifdef DEBUG
	write(1, cBuffer, ret);
#endif
     }

   // get header len
   hLen = Buffer.Find("\r\n\r\n", 4, 0) + 4;

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
	while(Buffer.GetLength() < (hLen + returnLEN))
	  {
	     ret = socket.recv(cBuffer, 1024, 0);

	     if(ret == -1)
	       return -1;  // problem here
#ifdef DEBUG
	     write(1, cBuffer, ret);
#endif
	     Buffer.AddBuffer((const void *)cBuffer, ret);
	     // TODO: Add progress indication hooks here
	  } // while
     } // if
   else
     DataPos = -1; // no data

   return 0; // A-OK
}

int ControlClient::HeaderGetReturn()
{
   int pos = Buffer.Find('/');
   int end = pos;
   int len = Buffer.GetLength();
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
   memcpy(temp, &Buffer.GetAt(pos), end - pos);
   temp[end - pos] = '\0';
   
   ret = atoi(&temp[1]);
   
   strncpy(LastCode, &temp[1], 4);
   
   return ret;
}

int ControlClient::HeaderGetLEN()
{
   int pos = Buffer.Find((const void *)"/LEN", 4);
   int end = pos;
   int len = Buffer.GetLength();
   
   if(pos == -1)
     return -1;
   
   while(Buffer[end] != '\r')
     {  
	end++;
	if(end > len)
	  return -1;
     }
   
   char temp[end - pos];
   memcpy(temp, &Buffer.GetAt(pos), end - pos);
   temp[end - pos] = '\0';
   
   return atoi(&temp[4]);
}

   
