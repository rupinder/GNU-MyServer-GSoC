/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2006, 2007, 2008, 2009 Free Software Foundation, Inc.
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <include/conf/vhost/vhost.h>
#include <include/base/file/file.h>
#include <include/base/file/files_utility.h>
#include <include/server/server.h>

#include <include/connection/connection.h>
#include <include/base/string/stringutils.h>
#include <include/base/string/securestr.h>
#include <include/conf/vhost/ip.h>

#ifdef HAVE_IDN
#include <stringprep.h>
#include <punycode.h>
#include <idna.h>
#endif


/*!
 *vhost costructor
 */
Vhost::Vhost(LogManager* lm)
{
  //ipList.clear();
  hostList.clear();
  refMutex.init();
  documentRoot.assign("");
  systemRoot.assign("");
  hashedData.clear();
  protocolData = 0;

  /*
   *By default use a non specified value for the throttling rate. -1 means 
   *that the throttling rate was not specified, while 0 means it was 
   *specified but there is not a limit.
   */
  throttlingRate = (u_long) -1;
  refCount = 0;
  nullReferenceCb = 0;
  defaultPriority = 0;
  logManager = lm;
}


/*!
 *Destroy the vhost. 
 */
Vhost::~Vhost()
{
  if(protocolData)
    delete protocolData;
  clearHostList();
  clearIPList();
  freeSSL();
  freeHashedData();

  HashMap<string, MimeRecord*>::Iterator it = locationsMime.begin ();

  while (it != locationsMime.end ())
    {
      delete *it;
      it++;
    }


  refMutex.destroy();
  documentRoot.assign("");
  systemRoot.assign("");
  mimeManager.clean();
  logManager->remove (this);
}

/*! 
 *Clear the data dictionary. 
 *Returns zero on success.
 */
int Vhost::freeHashedData()
{
  try
    {
      list<NodeTree<string>*>::iterator it = hashedDataTrees.begin ();
      while (it != hashedDataTrees.end ())
        {
          delete *it;
          it++;
        }

      hashedDataTrees.clear ();
      hashedData.clear();
    }
  catch(...)
    {
      return 1;
    }
  return 0;
}

/*!
 *Check if a MIME type file is defined for the virtual host.
 */
int Vhost::isMIME()
{
  return mimeManager.isLoaded();
}

/*!
 *Get the MIME manager for the virtual host.
 */
MimeManager* Vhost::getMIME()
{
  return &mimeManager;
}

/*!
 *Clear the list of the hosts.
 */
void Vhost::clearHostList()
{
  list<StringRegex*>::iterator i = hostList.begin();
  while(i != hostList.end())
    {
      StringRegex* sr = *i;
      delete sr;
      i++;
    }
  hostList.clear();
}

/*!
 *Clear the list of IPs.
 */
void Vhost::clearIPList()
{
  list<IpRange *>::iterator it = ipListAllow.begin();
  while(it != ipListAllow.end())
    {
      delete (*it);
      it++;
    }
  ipListAllow.clear();

  it = ipListDeny.begin();
  while(it != ipListDeny.end())
    {
      delete (*it);
      it++;
    }
  ipListDeny.clear();

  /*
  list<StringRegex*>::iterator i = ipList.begin();
  while(i != ipList.end())
    {
      StringRegex* sr = *i;
      delete sr;
      i++;
    }
  hostList.clear();
  */
}

int
Vhost::openAccessLog (string location, list<string>& filters, u_long cycle)
{
  return logManager->add (this, "ACCESSLOG", location, filters, cycle);
}

int
Vhost::openWarningLog (string location, list<string>& filters, u_long cycle)
{
  return logManager->add (this, "WARNINGLOG", location, filters, cycle);
}

int
Vhost::openLogFiles ()
{ 
  return logManager->count (this, "ACCESSLOG") == 0 ||
    logManager->count (this, "WARNINGLOG") == 0;
}

/*!
 *Add an IP address to the list.
 *\param ip The ip to add.
 *\param isRegex Specify if the ip is specified as a regex.
 */
void Vhost::addIP(const char *ip, int isRegex)
{
  std::string sTempIp(ip);
  IpRange *pNewRange = IpRange::RangeFactory(sTempIp);
  if ( pNewRange != NULL )
    ipListAllow.push_back(pNewRange);
}

/*!
 *Remove the IP address to the list.
 *\param ip The ip to remove.
 */
void Vhost::removeIP(const char *ip)
{
  std::string sTempIp(ip);
  ipListDeny.push_back(IpRange::RangeFactory(sTempIp));

  /*
  list<StringRegex*>::iterator i = ipList.begin();

  while(i != ipList.end())
    {
      StringRegex* sr = *i;
      / *
       *If this is the virtual host with the right IP.
       * /
      if(!stringcmp(sr->name,ip))
        {
          ipList.erase(i);
          return;
        }
    
      i++;
    }
  */
}

/*!
 *Remove the host address to the list.
 *\param host The hostname to remove.
 */
void Vhost::removeHost(const char *host)
{
  list<StringRegex*>::iterator i = hostList.begin();

  while(i != hostList.end())
    {
      StringRegex* sr = *i;
      /*
       *If this is the virtual host with the right IP.
       */
      if(!stringcmp(sr->name, host))
        {
          hostList.erase(i);
          return;
        }
    
      i++;
    }
}
/*!
 *Check if an host is allowed to the connection
 *\param host hostname to check.
 */
int Vhost::isHostAllowed(const char* host)
{
  /* If no hosts are specified then every host is allowed to connect here.  */
  if(!hostList.size() || !host)
    return 1;
    
  list<StringRegex*>::iterator i = hostList.begin();
  while(i != hostList.end())
    {
      StringRegex* sr = *i;
      regmatch_t pm;
      if(sr->regex.isCompiled())
        {
          if (!sr->regex.exec(host, 1, &pm, REG_NOTBOL))
            {
              return 1;
            }
        }
    
      if(!stringcmp(sr->name, host))
        return 1;
      
      i++;
    }
  return 0;
}

/*!
 *Check if all the host are allowed to the connection.
 */
int Vhost::areAllHostAllowed()
{
  if(hostList.size() == 0)
    return 1;
  return 0;
}

/*!
 *Check if all the IPs are allowed to the connection.
 */
int Vhost::areAllIPAllowed()
{
  /*
  if(ipList.size() == 0)
    return 1;
  */
  if ( ipListDeny.empty() && ipListAllow.empty() )
    return 1;
  return 0;
}

/*!
 *Check if the network is allowed to the connection(control the network 
 *by the local IP).
 *\param ip The IP to check.
 */
int Vhost::isIPAllowed(const char* ip)
{
  if ( areAllIPAllowed() )
    return 1;

  std::string sTempIp(ip);
  IpRange *pTempIp = IpRange::RangeFactory(sTempIp);
  list<IpRange *>::const_iterator it = ipListDeny.begin();
  while ( it != ipListDeny.end() )
    {
      if ( (*it)->InRange(pTempIp) )
	{
	  delete pTempIp;
	  return 0;
	}
      it++;
    }
  it = ipListAllow.begin();
  while ( it != ipListAllow.end() )
    {
      if ( (*it)->InRange(pTempIp) )
	{
	  delete pTempIp;
	  return 1;
	}
      it++;
    }
  delete pTempIp;
  return 0;
  /*
  / * If no IPs are specified then every host is allowed to connect here.  * /
  if(!ipList.size() || !ip)
    return 1;
    
  list<StringRegex*>::iterator i = ipList.begin();
  while(i != ipList.end())
    {
      StringRegex* sr = *i;
      regmatch_t pm;
      if(sr->regex.isCompiled())
        {
          if (!sr->regex.exec(ip ,1, &pm, REG_NOTBOL))
            {
              return 1;
            }
        }
    
      if(!stringcmp(sr->name, ip))
        return 1;
      
      i++;
    }
  */
  return 0;
}

/*!
 *Add an host to the allowed host list.
 *\param host hostname to add.
 *\param isRegex Is the host a regex?
 */
void Vhost::addHost(const char *host, int isRegex)
{
  StringRegex* hl = new StringRegex();
  if(hl == 0)
    return;

#ifdef HAVE_IDN
  size_t len;
  int ret;
  char* ascii = 0;
  uint32_t* ucs4 = stringprep_utf8_to_ucs4 (host, -1, &len);

  if(!ucs4)
    {
      delete hl;
      return;
    }

  ret = idna_to_ascii_4z (ucs4, &ascii, 0);

  free(ucs4);
  
  if (ret != IDNA_SUCCESS)
    {   
      delete hl;
      return;
    }

  host = ascii;
#endif

  hl->name.assign(host);

  if(isRegex)
    hl->regex.compile(host, REG_EXTENDED);
  hostList.push_back(hl);

#ifdef HAVE_IDN
  free(ascii);
#endif

}

/*!
 * Write to the accesses log.
 * \param str The line to log.
 */
int 
Vhost::accessesLogWrite (const char* str)
{
  return logManager->log (this, "ACCESSLOG", string (str));
}

/*!
 * Write a line to the warnings log.
 * \param str The line to log.
 */
int
Vhost::warningsLogWrite (const char* str)
{
  string msg;
  getLocalLogFormatDate (msg, 100);
  msg.append (" -- ");
  msg.append (str);
  return logManager->log (this, "WARNINGLOG", msg, true);
}

/*!
 *Set the null reference callback function. It is called when the reference
 *counter for the virtual host is zero. 
 *\param cb The null references callback function.
 */
void Vhost::setNullRefCB(NULL_REFERENCECB cb)
{
  nullReferenceCb = cb;
}

/*!
 *Get the null reference callback function used by the virtual host.
 */
NULL_REFERENCECB Vhost::getNullRefCB()
{
  return nullReferenceCb;               
}

/*!
 *Increment current references counter by 1.
 */
void Vhost::addRef()
{
  refMutex.lock(0);
  refCount++;
  refMutex.unlock(0);
}

/*!
 *Decrement current references counter by 1.
 */
void Vhost::removeRef()
{
  refMutex.lock(0);
  refCount--;
  /*! 
   *If the reference count reaches zero and a callback 
   *function is defined call it.
   */
  if(refCount == 0 && nullReferenceCb)
    nullReferenceCb(this);
  refMutex.unlock(0);
}

/*!
 *Get the references counter.
 */
int Vhost::getRef()
{
  return refCount;
}

/*!
 *Set the reference counter for the virtual host.
 *\param n The new reference counter.
 */
void Vhost::setRef(int n)
{
  refCount = n;
}

/*! 
 *Get the value for name in the hash dictionary.
 *\param name The hashed entry key.
 */
const char* Vhost::getHashedData(const char* name)
{
  NodeTree<string> *s = hashedData.get(name);

  return s ? s->getValue ()->c_str() : 0;
}
  
/*!
 *Initialize SSL on the virtual host.
 */
int Vhost::initializeSSL()
{
  Protocol* protocol;

  protocol = Server::getInstance()->getProtocol(protocolName.c_str());
  if(!protocol)
    return 0;
    
  if(!(protocol->getProtocolOptions() & PROTOCOL_USES_SSL))
    return 0;

  return sslContext.initialize();
}  


/*!
 *Get the SSL context.
 */
SSL_CTX* Vhost::getSSLContext()
{
  return sslContext.getContext();
}


/*!
 *Clean the memory used by the SSL context.
 */
int Vhost::freeSSL()
{
  return sslContext.free();
}
