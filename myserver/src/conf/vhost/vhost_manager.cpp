/*
  MyServer
  Copyright (C) 2007, 2008, 2009 Free Software Foundation, Inc.
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
#include <include/conf/vhost/vhost_manager.h>
#include <include/conf/vhost/vhost.h>
#include <include/conf/mime/mime_manager.h>
#include <include/server/server.h>
#include <include/base/file/files_utility.h>

/*!
 *VhostManager add function.
 *\param vh The virtual host to add.
 */
int VhostManager::addVHost(Vhost* vh)
{
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
      if(!vh->getProtocolName())
        {
          string error;
          error.assign("Warning: protocol not defined for virtual host: " );
          error.append(vh->getName());
          error.append(", using HTTP by default");
          vh->setProtocolName("http");
          Server::getInstance()->logWriteln(error.c_str(), MYSERVER_LOG_MSG_ERROR);
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
 *\param lm The log manager to use.
 */
VhostManager::VhostManager(ListenThreads* lt, LogManager* lm)
{
  listenThreads = lt;
  hostList.clear();
  extSource = 0;
  mutex.init();
  logManager = lm;
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
 *Change the file owner for the log locations.
 */
void VhostManager::changeLocationsOwner ()
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
      for(list<Vhost*>::iterator it = hostList.begin (); it != hostList.end (); it++)
        {
          int err;
          Vhost* vh = *it;

          /* Chown the log files.  */
          err = logManager->chown (vh, "ACCESSLOG", uid, gid);
          if(err)
            {
              string str ("Error changing owner for accesses log locations");
              Server::getInstance()->logWriteln(str.c_str (), MYSERVER_LOG_MSG_ERROR);
            }

          err = logManager->chown (vh, "WARNINGLOG", uid, gid);
          if(err)
            {
              string str ("Error changing owner for errors log locations");
              Server::getInstance()->logWriteln(str.c_str (), MYSERVER_LOG_MSG_ERROR);
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
 *Load a log XML node.
 */
void
VhostManager::loadXMLlogData (string name, Vhost* vh, xmlNode* lcur)
{
  xmlAttr *attr;
  string opt;
  attr = lcur->properties;
  while (attr)
    {
      opt.append ((char*)attr->name);
      opt.append ("=");
      opt.append ((char*)attr->children->content);
      if (attr->next)
        {
          opt.append (",");
        }
      attr = attr->next;
    }
  string location;
  list<string> filters;
  u_long cycle;
  xmlNode* stream = lcur->children;
  for (; stream; stream = stream->next, location.assign (""), cycle = 0, filters.clear ())
    {
      if (stream->type == XML_ELEMENT_NODE &&
          !xmlStrcmp (stream->name, (xmlChar const*)"STREAM"))
        {
          xmlAttr* streamAttr = stream->properties;
          while (streamAttr)
            {
              if (!strcmp ((char*)streamAttr->name, "location"))
                {
                  location.assign ((char*)streamAttr->children->content);
                }
              else if (!strcmp ((char*)streamAttr->name, "cycle"))
                {
                  cycle = atoi ((char*)streamAttr->children->content);
                }
              streamAttr = streamAttr->next;
            }
          xmlNode* filterList = stream->children;
          for (; filterList; filterList = filterList->next)
            {
              if (filterList->type == XML_ELEMENT_NODE &&
                  !xmlStrcmp (filterList->name, (xmlChar const*)"FILTER"))
                {
                  if (filterList->children && filterList->children->content)
                    {
                      string filter ((char*)filterList->children->content);
                      filters.push_back (filter);
                    }
                }
            }
          int err = 1;
          string str ("VhostManager::loadXMLlogData : Unrecognized log type");

          if (!name.compare ("ACCESSLOG"))
            {
              err = vh->openAccessLog (location, filters, cycle);
              vh->setAccessLogOpt (opt.c_str ());
              str.assign ("Error opening accesses log location " + location);
            }
          else if (!name.compare ("WARNINGLOG"))
            {
              err = vh->openWarningLog (location, filters, cycle);
              vh->setWarningLogOpt (opt.c_str ());
              str.assign ("Error opening warnings log location " + location);
            }
          if (err)
            {
              Server::getInstance ()->logWriteln (str.c_str (), MYSERVER_LOG_MSG_ERROR);
            }
        }
    }
}

/*!
 *Load the virtual hosts from a XML configuration file
 *Returns non-null on errors.
 *\param filename The XML file to open.
 */
int VhostManager::loadXMLConfigurationFile(const char *filename)
{
  XmlParser parser;
  xmlDocPtr doc;
  xmlNodePtr node;
  string errMsg;
  if(parser.open(filename))
    {
      errMsg.assign("Error opening: ");
      errMsg.append(filename);
      Server::getInstance()->logWriteln(errMsg.c_str(), MYSERVER_LOG_MSG_ERROR);
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
      vh=new Vhost(logManager);
      if(vh == 0)
        {
          parser.close();
          clean();
          errMsg.assign("Error: allocating memory");
          Server::getInstance()->logWriteln(errMsg.c_str(), MYSERVER_LOG_MSG_ERROR);
          return -1;
        }
    
      SslContext* sslContext = vh->getVhostSSLContext();
    
      while(lcur)
        {
          if(!xmlStrcmp(lcur->name, (const xmlChar *)"HOST"))
            {
              int useRegex = 0;
              for (xmlAttr *attrs = lcur->properties; attrs; attrs = attrs->next)
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
                }

              vh->addHost((const char*)lcur->children->content, useRegex);
            }
          else if(!xmlStrcmp(lcur->name, (const xmlChar *)"NAME"))
            {
              vh->setName((char*)lcur->children->content);
            }
          else if(!xmlStrcmp(lcur->name, (const xmlChar *)"LOCATION"))
            {
              string loc (vh->getDocumentRoot());
              //loc.append ("/");
              for (xmlAttr *attrs = lcur->properties; attrs; attrs = attrs->next)
                {
                  if(!xmlStrcmp (attrs->name, (const xmlChar *)"path"))
                    loc.append ((const char*) attrs->children->content);
                }
              MimeRecord *rc = MimeManager::readRecord (lcur);
              vh->locationsMime.put (loc, rc);
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
          else if(!xmlStrcmp(lcur->name, (const xmlChar *)"CONNECTIONS_PRIORITY"))
            {
              vh->setDefaultPriority(atoi((const char*)lcur->children->content));

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
                  Server::getInstance()->logWriteln(errMsg.c_str(), MYSERVER_LOG_MSG_ERROR);
                }
              vh->setPort((u_short)val);
            }
          else if(!xmlStrcmp(lcur->name, (const xmlChar *)"PROTOCOL"))
            {
              char* lastChar = (char*)lcur->children->content;
              while(*lastChar != '\0')
                {
                  *lastChar = tolower (*lastChar);
                  lastChar++;
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
              loadXMLlogData ("ACCESSLOG", vh, lcur);
            }
          else if(!xmlStrcmp(lcur->name, (const xmlChar *)"WARNINGLOG"))
            {
              loadXMLlogData ("WARNINGLOG", vh, lcur);
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
        }//while(lcur)
      
      if (vh->openLogFiles ())
        {
          errMsg.assign ("Error: opening log files");
          Server::getInstance ()->logWriteln (errMsg.c_str (), MYSERVER_LOG_MSG_ERROR);
          delete vh;
          vh = 0;
          continue;
        }

      if ( vh->initializeSSL() < 0 )
        {
          errMsg.assign("Error: initializing vhost");
          Server::getInstance()->logWriteln(errMsg.c_str(), MYSERVER_LOG_MSG_ERROR);
          delete vh;
          vh = 0;
          continue;
        }

      if(addVHost(vh))
        {
          errMsg.assign("Error: adding vhost");
          Server::getInstance()->logWriteln(errMsg.c_str(), MYSERVER_LOG_MSG_ERROR);
          delete vh;
          vh = 0;
          continue;
        }
    }
  parser.close();

  changeLocationsOwner ();

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
