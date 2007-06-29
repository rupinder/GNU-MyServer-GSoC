/*
MyServer
Copyright (C) 2007 The MyServer Team
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
#include "../include/vhost_manager.h"
#include "../include/vhosts.h"
#include "../include/server.h"
#include "../include/files_utility.h"

/*!
 *VhostManager add function.
 *\param vh The virtual host to add.
 */
int VhostManager::addVHost(Vhost* vh)
{
  Vhost* hostl;
  
  list<Vhost*>::iterator it;
  
  mutex.lock();

	/* Be sure there is a listening thread on the specified port.  */
	listenThreads->addListeningThread(vh->getPort());
	
	if(extSource)
	{
		int ret = extSource->addVHost(vh);
		mutex.unlock();
		return ret;
	}

  it = hostList.begin();

  try
  {
    for(;it != hostList.end(); it++)
    {
      hostl = *it;

      /* Do not do a case sensitive compare under windows.  */
#ifdef WIN32
      if(!stringcmpi(vh->getAccessesLogFileName(), 
                     hostl->getAccessesLogFileName()))
#else
        if(!stringcmp(vh->getAccessesLogFileName(), 
                      hostl->getAccessesLogFileName()))
#endif
        {
          string error;
          error.assign("Warning: multiple hosts use the same log file:" );
          error.append(vh->getAccessesLogFileName());
					Server::getInstance()->logLockAccess();
          Server::getInstance()->logPreparePrintError();
          Server::getInstance()->logWriteln(error.c_str());     
          Server::getInstance()->logEndPrintError();
					Server::getInstance()->logUnlockAccess();
        }

#ifdef WIN32
      if(!stringcmpi(vh->getWarningsLogFileName(), 
                     hostl->getWarningsLogFileName()))
#else
      if(!stringcmp(vh->getWarningsLogFileName(), 
										hostl->getWarningsLogFileName()))
#endif
      {
				string error;
				error.assign("Warning: multiple hosts use the same log file:" );
				error.append(vh->getWarningsLogFileName());
				Server::getInstance()->logLockAccess();
				Server::getInstance()->logPreparePrintError();
				Server::getInstance()->logWriteln(error.c_str());     
				Server::getInstance()->logEndPrintError();
				Server::getInstance()->logUnlockAccess();
			}
    }

		if(!vh->protocol)
		{
			string error;
			error.assign("Warning: protocol not defined for virtual host: " );
			error.append(vh->getName());
			error.append(", using HTTP by default");
			vh->setProtocol(PROTOCOL_HTTP);
			Server::getInstance()->logLockAccess();
			Server::getInstance()->logPreparePrintError();
			Server::getInstance()->logWriteln(error.c_str());     
			Server::getInstance()->logEndPrintError();
			Server::getInstance()->logUnlockAccess();
		}
    hostList.push_back(vh);
    mutex.unlock();
    return 0;
  }
  catch(...)
  {
    mutex.unlock();
    return -1;
  };
}

/*!
 *Get the vhost for the connection. A return value of 0 means that
 *a valid host was not found. 
 *\param host Hostname for the virtual host.
 *\param ip IP address for the virtual host.
 *\param port The port used by the client to connect to the server.
 */
Vhost* VhostManager::getVHost(const char* host, const char* ip, u_short port)
{
  list<Vhost*>::iterator it;

  mutex.lock();

  try
  {
    if(extSource)
    {
      Vhost* ret = extSource->getVHost(host, ip, port);
      mutex.unlock();
      return ret;
    }

		it = hostList.begin();

    /*Do a linear search here. We have to use the first full-matching 
		 *virtual host. 
		 */
    for(; it != hostList.end(); it++)
    {
      Vhost* vh = *it;
      /* Control if the host port is the correct one.  */
      if(vh->getPort() != port)
        continue;
      /* If ip is defined check that it is allowed to connect to the host.  */
      if(ip && !vh->isIPAllowed(ip))
        continue;
      /* If host is defined check if it is allowed to connect to the host.  */
      if(host && !vh->isHostAllowed(host))
        continue;
      /* We find a valid host.  */
      mutex.unlock();
      /* Add a reference.  */
      vh->addRef();
      return vh;
    }
    mutex.unlock();
    return 0;
  }
  catch(...)
  {
    mutex.unlock();
    return 0;   
  };
}

/*!
 *VhostManager costructor.
 *\param lt A ListenThreads object to use to create new threads.
 */
VhostManager::VhostManager(ListenThreads* lt)
{
	listenThreads = lt;
	hostList.clear();
  extSource = 0;
  mutex.init();
}

/*!
 *Clean the virtual hosts.
 */
void VhostManager::clean()
{
  list<Vhost*>::iterator it;

  mutex.lock();

  it = hostList.begin();  
  
  try
  {
    for(;it != hostList.end(); it++)
      delete *it;
      
    hostList.clear();
    
    mutex.unlock();
  }
  catch(...)
  {
    mutex.unlock();
    return;  
  };
}

/*!
 *vhostmanager destructor.
 */
VhostManager::~VhostManager()
{
	clean();
  mutex.destroy();
}

/*!
 *Returns the entire virtual hosts list.
 */
list<Vhost*>* VhostManager::getVHostList()
{
	return &(this->hostList);
}

/*!
 *Change the file owner for the log files.
 */
void VhostManager::changeFilesOwner()
{
  if(Server::getInstance()->getUid() | Server::getInstance()->getGid())
  {
    int uid = Server::getInstance()->getUid();
    int gid = Server::getInstance()->getGid();

    /*
     *Change the user and group identifier to -1
     *if they are not specified.
     */
    if(!uid)
      uid = -1;

    if(!gid)
      gid = -1;

    /*
     *Change the log files owner if a different user or group
     *identifier is specified.
     */
    for(int i = 0; ; i++)
    {
			int err;
      Vhost* vh = getVHostByNumber(i);
      /* Break if we reach the end of the list.  */
      if(!vh)
        break;

      /* Chown the log files.  */
      err = FilesUtility::chown(vh->getAccessesLogFileName(), uid, gid);
      if(err)
      {
        string str;
        str.assign("Error changing owner for: ");
        str.append(vh->getAccessesLogFileName());
        Server::getInstance()->logPreparePrintError();
        Server::getInstance()->logWriteln(str);
        Server::getInstance()->logEndPrintError();
      }

      err = FilesUtility::chown(vh->getWarningsLogFileName(), uid, gid);
      if(err)
      {
        string str;
        str.assign("Error changing owner for: ");
        str.append(vh->getWarningsLogFileName());
        Server::getInstance()->logPreparePrintError();
        Server::getInstance()->logWriteln(str);
        Server::getInstance()->logEndPrintError();
      }
    }
  }
}


/*!
 *Returns the number of hosts in the list
 */
int VhostManager::getHostsNumber()
{
	return hostList.size();
}

/*!
 *Load the virtual hosts from a XML configuration file
 *Returns non-null on errors.
 *\param filename The XML file to open.
 *\param maxlogSize The maximum dimension for the log file.
 */
int VhostManager::loadXMLConfigurationFile(const char *filename, 
																					 int maxlogSize)
{
	XmlParser parser;
	xmlDocPtr doc;
	xmlNodePtr node;
  string errMsg;
	if(parser.open(filename))
	{
    errMsg.assign("Error opening: ");
    errMsg.append(filename);
		Server::getInstance()->logLockAccess();
    Server::getInstance()->logPreparePrintError();
    Server::getInstance()->logWriteln(errMsg.c_str());
    Server::getInstance()->logEndPrintError();
		Server::getInstance()->logUnlockAccess();
		return -1;
	}
	doc = parser.getDoc();
	node = doc->children->children;
	for(;node;node = node->next )
	{
		xmlNodePtr lcur;
		Vhost *vh;
		if(xmlStrcmp(node->name, (const xmlChar *)"VHOST"))
			continue;
		lcur=node->children;
		vh=new Vhost();
    if(vh == 0)
    {
      parser.close();
      clean();
      errMsg.assign("Error: allocating memory");
			Server::getInstance()->logLockAccess();
      Server::getInstance()->logPreparePrintError();
      Server::getInstance()->logWriteln(errMsg.c_str());
      Server::getInstance()->logEndPrintError();
			Server::getInstance()->logUnlockAccess();
      return -1;
    }
      SslContext* sslContext = vh->getVhostSSLContext();
		while(lcur)
    {
			if(!xmlStrcmp(lcur->name, (const xmlChar *)"HOST"))
			{
        int useRegex = 0;
        xmlAttr *attrs = lcur->properties;
        while(attrs)
        {
            if(!xmlStrcmp(attrs->name, (const xmlChar *)"isRegex"))
            {
              if(attrs->children && attrs->children->content && 
                 (!xmlStrcmp(attrs->children->content, 
														 (const xmlChar *)"YES")))
              {
                useRegex = 1;
              }
            }
            attrs = attrs->next;
        }

			  vh->addHost((const char*)lcur->children->content, useRegex);
			}
			else if(!xmlStrcmp(lcur->name, (const xmlChar *)"NAME"))
			{
				vh->setName((char*)lcur->children->content);
			}
			else if(!xmlStrcmp(lcur->name, (const xmlChar *)"SSL_PRIVATEKEY"))
			{
				string pk((char*)lcur->children->content);
				sslContext->setPrivateKeyFile(pk);
			}
			else if(!xmlStrcmp(lcur->name, (const xmlChar *)"SSL_CERTIFICATE"))
			{
				string certificate((char*)lcur->children->content);
				sslContext->setCertificateFile(certificate);
			}
			else if(!xmlStrcmp(lcur->name, (const xmlChar *)"SSL_PASSWORD"))
			{
				string pw;
				if(lcur->children)
					pw.assign((char*)lcur->children->content);
				else
					pw.assign("");

					sslContext->setPassword(pw);

			}
			else if(!xmlStrcmp(lcur->name, (const xmlChar *)"IP"))
			{
        int useRegex = 0;
        xmlAttr *attrs =  lcur->properties;
        while(attrs)
        {
					if(!xmlStrcmp(attrs->name, (const xmlChar *)"isRegex"))
          {
						if(attrs->children && attrs->children->content && 
							 (!xmlStrcmp(attrs->children->content, 
													 (const xmlChar *)"YES")))
            {
							useRegex = 1;
						}
					}
					attrs = attrs->next;
        }
	  	 vh->addIP((char*)lcur->children->content, useRegex);
			}
			else if(!xmlStrcmp(lcur->name, (const xmlChar *)"PORT"))
			{
        int val = atoi((char*)lcur->children->content);
        if(val > (1 << 16) || strlen((const char*)lcur->children->content) > 6)
        {
          errMsg.assign("Error: specified port greater than 65536 or invalid: ");
          errMsg.append((char*)lcur->children->content);
					Server::getInstance()->logLockAccess();
					Server::getInstance()->logPreparePrintError();
					Server::getInstance()->logWriteln(errMsg.c_str());
					Server::getInstance()->logEndPrintError();
					Server::getInstance()->logUnlockAccess();

        }
				vh->setPort((u_short)val);
			}
			else if(!xmlStrcmp(lcur->name, (const xmlChar *)"PROTOCOL"))
			{
				if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"HTTP"))
					vh->setProtocol(PROTOCOL_HTTP);
				else if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"HTTPS"))
					vh->setProtocol(PROTOCOL_HTTPS);
				else if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"FTP"))
					vh->setProtocol(PROTOCOL_FTP);
				else if(!xmlStrcmp(lcur->children->content,(const xmlChar *)"CONTROL"))
					vh->setProtocol(PROTOCOL_CONTROL);
				else
				{
						vh->setProtocol(PROTOCOL_UNKNOWN);
				}
				vh->setProtocolName((char*)lcur->children->content);
				
			}
			else if(!xmlStrcmp(lcur->name, (const xmlChar *)"DOCROOT"))
			{
        if(lcur->children && lcur->children->content)
        {
          char* lastChar = (char*)lcur->children->content;
					while(*(lastChar+1) != '\0')
						lastChar++;

          if(*lastChar == '\\' || *lastChar == '/')
          {
						*lastChar = '\0';
          }
					vh->setDocumentRoot((const char*)lcur->children->content);
        }
        else
        {
          vh->setDocumentRoot("");
        }
			}
      else if(!xmlStrcmp(lcur->name, (const xmlChar *)"SYSFOLDER"))
			{
        if(lcur->children && lcur->children->content)
        {
          char* lastChar = (char*)lcur->children->content;
					while(*(lastChar+1) != '\0')
						lastChar++;

          if(*lastChar == '\\' || *lastChar == '/')
          {
						*lastChar = '\0';
          }
            vh->setSystemRoot((const char*)lcur->children->content);
        }
        else
        {
          vh->setSystemRoot("");
        }
      }
			else if(!xmlStrcmp(lcur->name, (const xmlChar *)"ACCESSLOG"))
			{
        xmlAttr *attr;
        string opt;
        opt.assign("");
				vh->setAccessLogOpt("");
				if(lcur->children && lcur->children->content)
					vh->setAccessesLogFileName((char*)lcur->children->content);
				else
				{
					errMsg.assign("Error: invalid accesses log file name");
					Server::getInstance()->logLockAccess();
					Server::getInstance()->logPreparePrintError();
					Server::getInstance()->logWriteln(errMsg.c_str());
					Server::getInstance()->logEndPrintError();
					Server::getInstance()->logUnlockAccess();
				}

				attr =  lcur->properties;
        while(attr)
        {
					opt.append((char*)attr->name);
					opt.append("=");
          opt.append((char*)attr->children->content);

					if(attr->next)
					{
						opt.append(",");
					}
					attr = attr->next;
        }
        vh->setAccessLogOpt(opt.c_str());
			}
			else if(!xmlStrcmp(lcur->name, (const xmlChar *)"WARNINGLOG"))
      {
				xmlAttr *attr;
        string opt;
        opt.assign("");

				if(lcur->children && lcur->children->content)
					vh->setWarningsLogFileName((char*)lcur->children->content);
				else
				{
					errMsg.assign("Error: invalid warnings log file name");
					Server::getInstance()->logLockAccess();
					Server::getInstance()->logPreparePrintError();
					Server::getInstance()->logWriteln(errMsg.c_str());
					Server::getInstance()->logEndPrintError();
					Server::getInstance()->logUnlockAccess();
				}
				vh->setWarningLogOpt("");                
				attr = lcur->properties;
				while(attr)
				{
					opt.append((char*)attr->name);
					opt.append("=");
					opt.append((char*)attr->children->content);
					if(attr->next)
          {
     
            opt.append(",");
          }

					attr=attr->next;
				}
        
        vh->setWarningLogOpt(opt.c_str());
      }
      else if(!xmlStrcmp(lcur->name, (const xmlChar *)"MIME_FILE"))
			{
				if(lcur->children)
          vh->getMIME()->loadXML((char*)lcur->children->content);
			}
      else if(!xmlStrcmp(lcur->name, (const xmlChar *)"THROTTLING_RATE"))
			{
				vh->setThrottlingRate((u_long)atoi((char*)lcur->children->content));
 			}
 			else if(lcur->children && lcur->children->content)
 			{
				string *old;
        string *s = new string((const char*)lcur->children->content);
        if(s == 0)
				{
					parser.close();
					clean();
          return -1;
				}
				string keyValue((const char*)lcur->name);
        old = vh->hashedData.put(keyValue, s);
				if(old)
        {
          delete old;
        }            
      }
      lcur = lcur->next;
    }

		if(vh->openLogFiles())
		{
			errMsg.assign("Error: opening log files");
			Server::getInstance()->logLockAccess();
			Server::getInstance()->logPreparePrintError();
			Server::getInstance()->logWriteln(errMsg.c_str());
			Server::getInstance()->logEndPrintError();
			Server::getInstance()->logUnlockAccess();
			
			delete vh;
			vh =0;
			continue;
		}

		if ( vh->initializeSSL() < 0 )
		{
			errMsg.assign("Error: initializing vhost");
			Server::getInstance()->logLockAccess();
			Server::getInstance()->logPreparePrintError();
			Server::getInstance()->logWriteln(errMsg.c_str());
			Server::getInstance()->logEndPrintError();
			Server::getInstance()->logUnlockAccess();
			
			delete vh;
			vh = 0;
			continue;
		}

		if(addVHost(vh))
		{
			errMsg.assign("Error: adding vhost");
			Server::getInstance()->logLockAccess();
			Server::getInstance()->logPreparePrintError();
			Server::getInstance()->logWriteln(errMsg.c_str());
			Server::getInstance()->logEndPrintError();
			Server::getInstance()->logUnlockAccess();
			
			delete vh;
			vh = 0;
			continue;
		}

  }
  parser.close();

	changeFilesOwner();

  return 0;
}

/*!
 *Save the virtual hosts to a XML configuration file.
 *\param filename The filename where write the XML file.
 */
int VhostManager::saveXMLConfigurationFile(const char *filename)
{
	File out;
	u_long nbw;

  mutex.lock();

	if(extSource)
	{
		int ret = extSource->save();
		mutex.unlock();
		return ret;
	}

	out.openFile(filename, File::MYSERVER_CREATE_ALWAYS | File::MYSERVER_OPEN_WRITE);
	out.writeToFile("<?xml version=\"1.0\"?>\r\n<VHOSTS>\r\n", 33, &nbw);

  try
  {
    list<Vhost*>::iterator i = hostList.begin();
    for( ; i != hostList.end() ; i++)
    {
      char port[6];
      list<Vhost::StringRegex*>::iterator il = (*i)->getIpList()->begin();
      list<Vhost::StringRegex*>::iterator hl = (*i)->getHostList()->begin();
      out.writeToFile("<VHOST>\r\n",9,&nbw);
      
      out.writeToFile("<NAME>",6,&nbw);
      out.writeToFile((*i)->getName(), strlen((*i)->getName()), &nbw);
      out.writeToFile("</NAME>\r\n",9,&nbw);
       
      for(; il != (*i)->getIpList()->end(); il++)
      {
        string *n = &((*il)->name);
        out.writeToFile("<IP>",4,&nbw);
        out.writeToFile(n->c_str(), n->length(), &nbw);
        out.writeToFile("</IP>\r\n",7,&nbw);
      }

      for(; hl != (*i)->getHostList()->end(); hl++)
      {
        string *n = &((*hl)->name);
        out.writeToFile("<HOST>",6,&nbw);
        out.writeToFile(n->c_str(), n->length(), &nbw);
        out.writeToFile("</HOST>\r\n",9,&nbw);
      }

      out.writeToFile("<PORT>",6,&nbw);
      sprintf(port,"%i", (*i)->getPort());
      out.writeToFile(port,(u_long)strlen(port),&nbw);
      out.writeToFile("</PORT>\r\n",9,&nbw);

      if((*i)->getVhostSSLContext()->getPrivateKeyFile().length())
      {
				string &pk = (*i)->getVhostSSLContext()->getPrivateKeyFile();
        out.writeToFile("<SSL_PRIVATEKEY>",16,&nbw);
        out.writeToFile(pk.c_str(), pk.length(),&nbw);
        out.writeToFile("</SSL_PRIVATEKEY>\r\n",19,&nbw);
      }
      
      if((*i)->getVhostSSLContext()->getCertificateFile().length())
      {
				string &certificate = (*i)->getVhostSSLContext()->getCertificateFile();
        out.writeToFile("<SSL_CERTIFICATE>", 17, &nbw);
        out.writeToFile(certificate.c_str(), (u_long)certificate.length(), 
												&nbw);
        out.writeToFile("</SSL_CERTIFICATE>\r\n", 20, &nbw);
      }

      if((*i)->getVhostSSLContext()->getPassword().length())
      {
				string& pw = (*i)->getVhostSSLContext()->getPassword();
        out.writeToFile("<SSL_PASSWORD>", 14, &nbw);
        out.writeToFile(pw.c_str(), pw.length(), &nbw);
        out.writeToFile("</SSL_PASSWORD>\r\n", 17, &nbw);
      }

      out.writeToFile("<PROTOCOL>", 10, &nbw);
      switch( (*i)->getProtocol())
      {
			  case PROTOCOL_HTTP:
          out.writeToFile("HTTP", 4, &nbw);
          break;
			  case PROTOCOL_HTTPS:
			  	out.writeToFile("HTTPS",5,&nbw);
			  	break;
			  case PROTOCOL_FTP:
			  	out.writeToFile("FTP",3,&nbw);
			  	break;
			  case PROTOCOL_CONTROL:
				  out.writeToFile("CONTROL", 7, &nbw);
				  break;
			  default:			
				  out.writeToFile((*i)->getProtocolName(), 
                          strlen((*i)->getProtocolName()), &nbw);
				  break;
      }
      out.writeToFile("</PROTOCOL>\r\n", 13, &nbw);
      
      out.writeToFile("<DOCROOT>", 9, &nbw);
      out.writeToFile((*i)->getDocumentRoot(), 
                      (u_long)strlen((*i)->getDocumentRoot()), &nbw);
      out.writeToFile("</DOCROOT>\r\n", 12, &nbw);
      
      out.writeToFile("<SYSFOLDER>", 11, &nbw);
      out.writeToFile((*i)->getSystemRoot(), 
                      (u_long)strlen((*i)->getSystemRoot()), &nbw);
      
      out.writeToFile("</SYSFOLDER>\r\n", 14, &nbw);
      
      out.writeToFile("<ACCESSLOG>", 13, &nbw);
      out.writeToFile((*i)->getAccessesLogFileName(),
                      (u_long)strlen((*i)->getAccessesLogFileName()), &nbw);
      out.writeToFile("</ACCESSLOG>\r\n", 16, &nbw);
      
      out.writeToFile("<WARNINGLOG>", 12, &nbw);
      out.writeToFile((*i)->getWarningsLogFileName(),
                      (u_long)strlen((*i)->getWarningsLogFileName()),&nbw);
      out.writeToFile("</WARNINGLOG>\r\n", 15, &nbw);
      
			{
				HashMap<string, string*>::Iterator it = (*i)->hashedData.begin();
				for(; it != (*i)->hashedData.end(); it++)
				{
					ostringstream outString;
					outString << "<" << it.getKey() << ">" << (*it)  << "</" 
										<< it.getKey() << ">" << endl;
					out.writeToFile(outString.str().c_str(),outString.str().size(),&nbw);
				}
				out.writeToFile("</VHOST>\r\n", 10, &nbw);
			}
		}
		out.writeToFile("</VHOSTS>\r\n", 11, &nbw);
		out.closeFile();
		mutex.unlock();
  }
  catch(...)
  {
    mutex.unlock();
  };

	return 0;
}


/*!
 *Get a virtual host by its position in the list.
 *Zero based list.
 *\param n The virtual host id.
 */
Vhost* VhostManager::getVHostByNumber(int n)
{
  Vhost* ret = 0;
  mutex.lock();
  try
  {
    list<Vhost*>::iterator i = hostList.begin();
    if(extSource)
    {
      ret=extSource->getVHostByNumber(n);
      mutex.unlock();
      return ret;
    }
    
    for( ; i != hostList.end(); i++)
    {
      if(!(n--))
      {
        ret = *i;
        ret->addRef();
        break;
      }
    }
    mutex.unlock();
    
    return ret;
  }
  catch(...)
  {
    mutex.unlock();
    return ret;
  };
}

/*!
 *Remove a virtual host by its position in the list
 *First position is zero.
 *\param n The virtual host identifier in the list.
 */
int VhostManager::removeVHost(int n)
{
  mutex.lock();
	try
  {
    list<Vhost*>::iterator i = hostList.begin();
    
    for( ;i != hostList.end(); i++)
    {
      if(!(n--))
      {
        delete *i;
        mutex.unlock();
        return 1;
      }
    }
    mutex.unlock();
    return 0;
  }
  catch(...)
  {
    mutex.unlock();
    return 0;   
  };
}

/*!
 *Set an external source for the virtual hosts.
 *\param nExtSource The new external source.
 */
void VhostManager::setExternalSource(VhostSource* nExtSource)
{
  mutex.lock();
  extSource = nExtSource;
  mutex.unlock();
}

/*!
 *Construct the object.
 */
VhostSource::VhostSource()
{

}

/*!
 *Destroy the object.
 */
VhostSource::~VhostSource()
{

}

/*!
 *Load the object.
 */
int VhostSource::load()
{
  return 0;
}

/*!
 *Save the object.
 */
int VhostSource::save()
{
  return 0;
}

/*!
 *Free the object.
 */
int VhostSource::free()
{
  return 0;
}

/*!
 *Add a virtual host to the source.
 */
int VhostSource::addVHost(Vhost*)
{
	return 0;
}

/*!
 *Get a virtual host.
 */
Vhost* VhostSource::getVHost(const char*, const char*, u_short)
{
  return 0;
}

/*!
 *Get a virtual host by its number.
 */
Vhost* VhostSource::getVHostByNumber(int n)
{
  return 0;
}

