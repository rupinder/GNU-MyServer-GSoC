/*
 *  MyServer
 *  Copyright (C) 2002, 2003, 2004 The MyServer Team
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef VHOST_H_CONF
#define VHOST_H_CONF

#include <FL/Fl.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Choice.H>

#include "vector.h"
#include "../include/mem_buff.h"

enum 
{
   PROTOCOL_HTTP = 0,
   PROTOCOL_HTTPS,
   PROTOCOL_FTP,
   PROTOCOL_CONTROL,
   PROTOCOL_DYNAMIC
};

#define ALLOW_CGI               0x001
#define ALLOW_ISAPI             0x002
#define ALLOW_MSCGI             0x004
#define ALLOW_WINCGI            0x008
#define ALLOW_FASTCGI           0x010
#define ALLOW_SEND_LINK         0x020
#define ALLOW_EXTERNAL_COMMANDS 0x040
#define ALLOW_SEND_FILE         0x080
#define ALLOW_SCGI              0x100
#define ALLOW_ALL               0xFFF

extern const char * EMPTY;

struct VHostNode
{
   Vector Host;
   Vector Ip;
   int Port;
   int Protocol;
   int Service;
   char * Ssl_Privatekey;
   char * Ssl_Certificate;
   char * Ssl_Password;
   char * Docroot;
   char * Sysfolder;
   char * Accesseslog;
   char * Warninglog;
};

class VHostXML
{
 public:
   ~VHostXML();
   void clear();
   int load(const char *);
   int loadMemBuf(MemBuf &);
   int load_core(XmlParser &);
   int save(const char *);
   int saveMemBuf(MemBuf &);
   int save_core(XmlParser &);
   void populateName(Fl_Choice *);
   void populateHost(int, Fl_Browser *);
   void populateIp(int, Fl_Browser *);
   int addName(const char *);
   int addHost(int, const char *, bool);
   int addIp(int, const char *, bool);
   void removeName(int);
   void removeHost(int, int);
   void removeIp(int, int);
   void setPort(int, int);
   void setProtocol(int, int);
   void setService(int, int, bool);
   void setSsl_Privatekey(int, const char *);
   void setSsl_Certificate(int, const char *);
   void setSsl_Password(int, const char *);
   void setDocroot(int, const char *);
   void setSysfolder(int, const char *);
   void setAccesseslog(int, const char *);
   void setWarninglog(int, const char *);
   int getPort(int);
   int getProtocol(int);
   bool getService(int, int);
   const char * getSsl_Privatekey(int);
   const char * getSsl_Certificate(int);
   const char * getSsl_Password(int);
   const char * getDocroot(int);
   const char * getSysfolder(int);
   const char * getAccesseslog(int);
   const char * getWarninglog(int);
   /* Dynamic protocals */
   void populateProtocol(Fl_Choice *);
   void loadProtocols(Vector &);
 private :
   void DeleteVHostNode(VHostNode *);
   Vector VHosts;
   Vector Dynamic;
};

#endif
