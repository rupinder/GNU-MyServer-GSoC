/*
MyServer
Copyright (C) 2002, 2003, 2004, 2006, 2007 The MyServer Team
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

#include "../include/vhosts.h"
#include "../include/file.h"
#include "../include/files_utility.h"
#include "../include/server.h"

#include "../include/connection.h"
#include "../include/stringutils.h"
#include "../include/securestr.h"

#ifdef HAVE_IDN
#include <stringprep.h>
#include <punycode.h>
#include <idna.h>
#endif

/*!
 *vhost costructor
 */
Vhost::Vhost()
{
	ipList.clear();
	hostList.clear();
	refMutex.init();
  documentRoot.assign("");
	systemRoot.assign("");
  accessesLogFileName.assign("");
  warningsLogFileName.assign("");
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
  refMutex.destroy();
  accessesLogFileName.assign("");
  
  warningsLogFileName.assign("");
 
  warningsLogFile.close();
  accessesLogFile.close();
  
  documentRoot.assign("");
	systemRoot.assign("");

  mimeManager.clean();
}

/*! 
 *Clear the data dictionary. 
 *Returns zero on success.
 */
int Vhost::freeHashedData()
{
  try
  {
		HashMap<string, string*>::Iterator it = hashedData.begin();
		for (;it != hashedData.end(); it++)
		{
			delete (*it);
		}
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
  list<StringRegex*>::iterator i = ipList.begin();
	while(i != ipList.end())
	{
    StringRegex* sr = *i;
    delete sr;
    i++;
	}
	hostList.clear();
}


/*!
 *Open the log files for the virtual hosts.
 *\param maxLogSize Define a max log size for the opened files.
 */
int Vhost::openLogFiles(u_long maxlogSize)
{
	const char* accessesLogFileName = getAccessesLogFileName();
	const char* warningsLogFileName = getWarningsLogFileName();
	if(accessesLogFileName)
	{
		accessesLogFile.load(accessesLogFileName);
    
		if(strstr(getAccessLogOpt(), "cycle=yes"))
		{
			accessesLogFile.setCycleLog(1);
		}
		if(strstr(getAccessLogOpt(), "cycle_gzip=no"))
		{
			accessesLogFile.setGzip(0);
		}
		else
		{
			accessesLogFile.setGzip(1);
		}  
	}

	if(warningsLogFileName)
	{
		warningsLogFile.load(warningsLogFileName);
		if(strstr(getWarningLogOpt(), "cycle=yes"))
		{
			warningsLogFile.setCycleLog(1);
		}
	
		if(strstr(getWarningLogOpt(), "cycle_gzip=no"))
		{
			warningsLogFile.setGzip(0);
		}
		else
		{
			warningsLogFile.setGzip(1);
		}
	}

	setMaxLogSize(maxlogSize);

	return 0;
}

/*!
 *Add an IP address to the list.
 *\param ip The ip to add.
 */
void Vhost::addIP(const char *ip, int isRegex)
{
	StringRegex* sr = new StringRegex();
  if(sr == 0)
    return;
	sr->name.assign(ip);
  /* If is a regular expression, the ip string is a pattern.  */
  if(isRegex)
    sr->regex.compile(ip, REG_EXTENDED);
  ipList.push_back(sr);
}
/*!
 *Remove the IP address to the list.
 *\param ip The ip to remove.
 */
void Vhost::removeIP(const char *ip)
{
  list<StringRegex*>::iterator i = ipList.begin();

	while(i != ipList.end())
	{
    StringRegex* sr = *i;
		/*
     *If this is the virtual host with the right IP.
     */
		if(!stringcmp(sr->name,ip))
		{
		  ipList.erase(i);
		  return;
		}
		
		i++;
	}
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
	if(ipList.size() == 0)
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
 /* If no IPs are specified then every host is allowed to connect here.  */
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
 *Here threads get the permission to use the access log file.
 *\param id The caller thread ID.
 */
u_long Vhost::accessesLogRequestAccess(int id)
{
	accessesLogFile.requestAccess();
  return 0;
}

/*!
 *Here threads get the permission to use the warnings log file.
 *\param id The caller thread ID.
 */
u_long Vhost::warningsLogRequestAccess(int id)
{
	warningsLogFile.requestAccess();
  return 0;
}

/*!
 *Here threads release the permission to use the access log file.
 *\param id The caller thread ID.
 */
u_long Vhost::accessesLogTerminateAccess(int id)
{
	accessesLogFile.terminateAccess();
  return 0;
}

/*!
 *Here threads release the permission to use the warnings log file.
 *\param id The caller thread ID.
 */
u_long Vhost::warningsLogTerminateAccess(int id)
{
	warningsLogFile.terminateAccess();
  return 0;
}

/*!
 *Write to the accesses log file.
 *\param str The line to log.
 */
int Vhost::accessesLogWrite(const char* str)
{
	return accessesLogFile.write(str);
}

/*!
 *Return a pointer to the file used by the accesses log.
 */
File* Vhost::getAccessesLogFile()
{
	return accessesLogFile.getFile();
}

/*!
 *Get the log object for the warnings.
 */
LogManager* Vhost::getWarningsLog()
{
  return &warningsLogFile;
}

/*!
 *Get the log object for the accesses.
 */
LogManager* Vhost::getAccessesLog()
{
  return &accessesLogFile;
}

/*!
 *Write a line to the warnings log file.
 *\param str The line to log.
 */
int Vhost::warningsLogWrite(const char* str)
{
  string msg;
  getLocalLogFormatDate(msg, 100);
  msg.append(" -- ");
  msg.append(str);
#ifdef WIN32
  msg.append("\r\n");
#else
  msg.append("\n");
#endif  
	return warningsLogFile.write(msg.c_str());
}

/*!
 *Return a pointer to the file used by the warnings log.
 */
File* Vhost::getWarningsLogFile()
{
	return warningsLogFile.getFile();
}

/*!
 *Set the max size of the log files.
 *\param newSize The new max dimension to use for the warning log files.
 */
void Vhost::setMaxLogSize(int newSize)
{
  warningsLogFile.setMaxSize(newSize);
  accessesLogFile.setMaxSize(newSize);  
}

/*!
 *Get the max size of the log files. Return 0 on success.
 */
int Vhost::getMaxLogSize()
{
  /*
   *warningsLogFile max log size is equal to the  
   *accessesLogFile one.
   */
	return warningsLogFile.getMaxSize( );
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
 *Get the value for name in the hash dictionary. If the data is not present
 *it tries to get it from the Server class.
 *\param name The key of the hashed entry.
 */
const char* Vhost::getHashedData(const char* name)
{
	
  string *s;
	if(name == 0)
		return 0;
	s = hashedData.get(name);
  if(s)
    return s->c_str();

  return Server::getInstance() ? Server::getInstance()->getHashedData(name) 
		: 0 ;
}
  
/*!
 *Initialize SSL on the virtual host.
 */
int Vhost::initializeSSL()
{
	DynamicProtocol* dp;

	if(this->protocol < 1000 && this->protocol)
		return 0;

	if(!this->protocol)
	{
		dp = Server::getInstance()->getDynProtocol(protocolName.c_str());
		if(!dp)
			return 0;
		
		if(!(dp->getOptions() & PROTOCOL_USES_SSL))
			return 0;
	}
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
