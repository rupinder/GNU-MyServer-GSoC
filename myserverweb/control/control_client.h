#ifndef CONTROL_CLIENT_H
#define CONTROL_CLIENT_H

#include "../include/sockets.h"
#include "../include/vector.h"
#include "../include/filemanager.h"
#include "../include/control_errors.h"
#include "../include/stringutils.h"
#include "../include/MemBuf.h"

class ControlClient
{
 public:
   ControlClient();
   ~ControlClient();
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
 protected:
   int sendRequest(const char *, const char *);
   int sendRequest(const char *, const char *, CMemBuf &);
   int getResponse();
   MYSERVER_SOCKET socket;
   bool Connected;
   CMemBuf Buffer;
   int DataPos;
 private:
   int HeaderGetReturn();
   int HeaderGetLEN();
   char UserName[64];
   char UserPass[64];
};

#endif
