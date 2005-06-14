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
#ifndef CONTROL_CLIENT_H
#define CONTROL_CLIENT_H

#include "../include/sockets.h"
#include "vector.h"
#include "../include/filemanager.h"
#include "../include/control_errors.h"
#include "../include/stringutils.h"
#include "../include/MemBuf.h"

typedef void (*ControlClientCallback)(void *, unsigned int, unsigned int);

class ControlClient
{
 public:
   ControlClient();
   ~ControlClient();
   
   void setCallback(ControlClientCallback, void *);
   void clearCallback();
   
   int Login(const char *, const int, const char *, const char *);
   int Logout();
   
   int getVersion(CMemBuf &);
   int sendReboot();
   
   int getMyserverConf(CMemBuf &);
   int getVhostsConf(CMemBuf &);
   int getMIMEtypesConf(CMemBuf &);
   
   int sendMyserverConf(CMemBuf &);
   int sendVhostsConf(CMemBuf &);
   int sendMIMEtypesConf(CMemBuf &);
   
   int getLanguages(Vector &);
   int getDynamicProtocols(Vector &);
   
   int getConnections(Vector &);
   int sendKillConnection(VectorNode *);

   int sendDisableReboot();
   int sendEnableReboot();
   
   char LastCode[4];
 protected:
   int sendRequest(const char *, const char *);
   int sendRequest(const char *, const char *, CMemBuf &);
   int getResponse();
   Socket socket;
   bool Connected;
   CMemBuf Buffer;
   int DataPos;
 private:
   int HeaderGetReturn();
   int HeaderGetLEN();
   ControlClientCallback Progress;
   void * Object;
   char UserName[64];
   char UserPass[64];
};

#endif
