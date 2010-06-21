/*
  MyServer
  Copyright (C) 2007, 2008, 2009, 2010 Free Software Foundation, Inc.
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

#include "myserver.h"

#include <include/conf/vhost/xml_vhost_handler.h>
#include <include/conf/vhost/vhost.h>
#include <include/conf/mime/xml_mime_handler.h>
#include <include/server/server.h>
#include <include/base/file/files_utility.h>
#include <include/conf/mime/xml_mime_handler.h>

#include <include/conf/xml_conf.h>


static VhostManagerHandler *builder (ListenThreads *lt, LogManager *lm)
{
  return new XmlVhostHandler (lt, lm);
}

/*!
  Register the builder on the vhost manager.

  \param manager Where register the builder.
 */
void XmlVhostHandler::registerBuilder (VhostManager &manager)
{
  string xml ("xml");
  manager.registerBuilder (xml, builder);
}

/*!
  XmlVhostHandler add function.
  \param vh The virtual host to add.
 */
int XmlVhostHandler::addVHost (Vhost *vh)
{
  vector<Vhost*>::iterator it;

  /* Be sure there is a listening thread on the specified port.  */
  listenThreads->addListeningThread (vh->getPort ());

  it = hosts.begin ();

  try
    {
      if (!vh->getProtocolName ())
        {
          vh->setProtocolName ("http");
          Server::getInstance ()->log (MYSERVER_LOG_MSG_WARNING,
                 _("Protocol not defined for vhost: %s, using HTTP by default"),
                                             vh->getName ());
        }
      string protocol (vh->getProtocolName ());
      if (Server::getInstance ()->getProtocolsManager ()->getProtocol (protocol)
          == NULL)
        Server::getInstance ()->log (MYSERVER_LOG_MSG_ERROR,
                                _("The protocol \"%s\" is used but not loaded"),
                                    protocol.c_str ());

      hosts.push_back (vh);
      return 0;
    }
  catch (...)
    {
      return -1;
    };
}

/*!
  \see VhostManager#getVHost
 */
Vhost* XmlVhostHandler::getVHost (const char *host, const char *ip, u_short port)
{
  vector<Vhost*>::iterator it;
  /*
    Do a linear search here. We have to use the first full-matching
    virtual host.
  */
  for (it = hosts.begin (); it != hosts.end (); it++)
    {
      Vhost* vh = *it;

      /* Control if the host port is the correct one.  */
      if (vh->getPort () != port)
        continue;

      /* If ip is defined check that it is allowed to connect to the host.  */
      if (ip && !vh->isIPAllowed (ip))
        continue;

      /* If host is defined check if it is allowed to connect to the host.  */
      if (host && !vh->isHostAllowed (host))
        continue;

      /* Add a reference.  */
      vh->addRef ();
      return vh;
    }
  return 0;

  return 0;
}

/*!
  XmlVhostHandler costructor.
  \param lt A ListenThreads object to use to create new threads.
  \param lm The log manager to use.
 */
XmlVhostHandler::XmlVhostHandler (ListenThreads *lt, LogManager *lm)
{
  listenThreads = lt;
  logManager = lm;
}

/*!
  Clean the virtual hosts.
 */
void XmlVhostHandler::clean ()
{
  vector<Vhost*>::iterator it;

  it = hosts.begin ();
  try
    {
      for (;it != hosts.end (); it++)
        delete *it;

      hosts.clear ();
    }
  catch (...)
    {
      return;
    };
}

/*!
  vhostmanager destructor.
 */
XmlVhostHandler::~XmlVhostHandler ()
{
  clean ();
}

/*!
  Returns the entire virtual hosts list.
 */
vector<Vhost*>* XmlVhostHandler::getVHostList ()
{
  return &(this->hosts);
}

/*!
  Change the file owner for the log locations.
 */
void XmlVhostHandler::changeLocationsOwner ()
{
  if (Server::getInstance ()->getUid () ||
     Server::getInstance ()->getGid ())
    {
      string uid (Server::getInstance ()->getUid ());
      string gid (Server::getInstance ()->getGid ());

      /*
       *Change the log files owner if a different user or group
       *identifier is specified.
       */
      for (vector<Vhost*>::iterator it = hosts.begin (); it != hosts.end (); it++)
        {
          int err;
          Vhost* vh = *it;

          /* Chown the log files.  */
          err = logManager->chown (vh, "ACCESSLOG", uid, gid);
          if (err)
            Server::getInstance ()->log (MYSERVER_LOG_MSG_ERROR,
                    _("Error while changing accesses log locations owner"));

          err = logManager->chown (vh, "WARNINGLOG", uid, gid);
          if (err)
            Server::getInstance ()->log (MYSERVER_LOG_MSG_ERROR,
                    _("Error while changing log locations owner"));
        }
    }
}


/*!
  Returns the number of hosts in the list
 */
int XmlVhostHandler::getHostsNumber ()
{
  return hosts.size ();
}


/*!
  Load a log XML node.
 */
void
XmlVhostHandler::loadXMLlogData (string name, Vhost* vh, xmlNode* lcur)
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
          !xmlStrcmp (stream->name, (xmlChar const*) "STREAM"))
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
                  !xmlStrcmp (filterList->name, (xmlChar const*) "FILTER"))
                {
                  if (filterList->children && filterList->children->content)
                    {
                      string filter ((char*)filterList->children->content);
                      filters.push_back (filter);
                    }
                }
            }
          int err = 1;
          string str ("XmlVhostHandler::loadXMLlogData : Unrecognized log type");

          if (!name.compare ("ACCESSLOG"))
            {
              err = vh->openAccessLog (location, filters, cycle);
              vh->setAccessLogOpt (opt.c_str ());
              if (err)
                Server::getInstance ()->log (MYSERVER_LOG_MSG_ERROR,
                                                   _("Error opening %s"), location.c_str ());
            }
          else if (!name.compare ("WARNINGLOG"))
            {
              err = vh->openWarningLog (location, filters, cycle);
              vh->setWarningLogOpt (opt.c_str ());
              if (err)
                Server::getInstance ()->log (MYSERVER_LOG_MSG_ERROR,
                                                   _("Error opening %s"), location.c_str ());
            }
          else
            Server::getInstance ()->log (MYSERVER_LOG_MSG_ERROR,
                                               _(" Unrecognized log type"));
        }
    }
}

/*!
  Load the virtual hosts from a XML configuration file
  Returns non-null on errors.
  \param filename The XML file to open.
 */
int XmlVhostHandler::load (const char *filename)
{
  XmlParser parser;
  xmlDocPtr doc;
  xmlNodePtr node;
  if (parser.open (filename))
    {
      Server::getInstance ()->log (MYSERVER_LOG_MSG_ERROR,
                                         _("Error opening %s"), filename);
      return -1;
    }
  doc = parser.getDoc ();
  node = doc->children->children;

  for (;node;node = node->next )
    {
      xmlNodePtr lcur;
      Vhost *vh;
      if (xmlStrcmp (node->name, (const xmlChar *) "VHOST"))
        continue;
      lcur = node->children;
      vh = new Vhost (logManager);
      if (vh == 0)
        {
          parser.close ();
          clean ();
          Server::getInstance ()->log (MYSERVER_LOG_MSG_ERROR,
                                             _("internal error"), filename);
          return -1;
        }

      SslContext* sslContext = vh->getVhostSSLContext ();

      while (lcur)
        {
          XmlConf::build (lcur, vh->getHashedDataTrees (),
                          vh->getHashedData ());

          if (!xmlStrcmp (lcur->name, (const xmlChar *) "HOST"))
            {
              int useRegex = 0;
              for (xmlAttr *attrs = lcur->properties; attrs; attrs = attrs->next)
                {
                  if (!xmlStrcmp (attrs->name, (const xmlChar *) "isRegex")
                      && attrs->children && attrs->children->content
                      && (!xmlStrcmp (attrs->children->content,
                                     (const xmlChar *) "YES")))
                        useRegex = 1;
                }

              vh->addHost ((const char*)lcur->children->content, useRegex);
            }
          else if (!xmlStrcmp (lcur->name, (const xmlChar *) "NAME"))
            {
              vh->setName ((char*)lcur->children->content);
            }
          else if (!xmlStrcmp (lcur->name, (const xmlChar *) "LOCATION"))
            {
              string loc (vh->getDocumentRoot ());
              loc.append ("/");

              for (xmlAttr *attrs = lcur->properties; attrs; attrs = attrs->next)
                if (!xmlStrcmp (attrs->name, (const xmlChar *) "path"))
                  loc.append ((const char*) attrs->children->content);

              MimeRecord *record = XmlMimeHandler::readRecord (lcur);
              MimeRecord *prev = vh->addLocationMime (loc, record);
              if (prev)
                {
                  Server::getInstance ()->log (MYSERVER_LOG_MSG_ERROR,
                     _("The location `%s' is registered multiple times"),
                                               loc.c_str ());

                  delete prev;
                }

              vh->getLocationsMime ()->put (loc, record);
            }
          else if (!xmlStrcmp (lcur->name, (const xmlChar *) "SSL_PRIVATEKEY"))
            {
              string pk ((char*)lcur->children->content);
              sslContext->setPrivateKeyFile (pk);
            }
          else if (!xmlStrcmp (lcur->name, (const xmlChar *) "SSL_CERTIFICATE"))
            {
              string certificate ((char*)lcur->children->content);
              sslContext->setCertificateFile (certificate);
            }
          else if (!xmlStrcmp (lcur->name, (const xmlChar *) "CONNECTIONS_PRIORITY"))
            {
              vh->setDefaultPriority (atoi ((const char*)lcur->children->content));
            }
          else if (!xmlStrcmp (lcur->name, (const xmlChar *) "SSL_PASSWORD"))
            {
              string pw ((char*)lcur->children->content);
              sslContext->setPassword (pw);
            }
          else if (!xmlStrcmp (lcur->name, (const xmlChar *) "IP"))
            {
              int useRegex = 0;
              xmlAttr *attrs = lcur->properties;

              while (attrs)
                {
                  if (!xmlStrcmp (attrs->name, (const xmlChar *) "isRegex")
                      && !xmlStrcmp (attrs->children->content,
                                     (const xmlChar *) "YES"))
                    useRegex = 1;

                  attrs = attrs->next;
                }

              vh->addIP ((char*)lcur->children->content, useRegex);
            }
          else if (!xmlStrcmp (lcur->name, (const xmlChar *) "PORT"))
            {
              int val = atoi ((char*)lcur->children->content);
              if (val > (1 << 16) || val <= 0)
                Server::getInstance ()->log (MYSERVER_LOG_MSG_ERROR,
                      _("Specified invalid port %s"), lcur->children->content);
              vh->setPort ((u_short)val);
            }
          else if (!xmlStrcmp (lcur->name, (const xmlChar *) "PROTOCOL"))
            {
              char* lastChar = (char*)lcur->children->content;
              while (*lastChar != '\0')
                {
                  *lastChar = tolower (*lastChar);
                  lastChar++;
                }
              vh->setProtocolName ((char*)lcur->children->content);
            }
          else if (!xmlStrcmp (lcur->name, (const xmlChar *) "DOCROOT"))
            {
              char* lastChar = (char*)lcur->children->content;
              while (*(lastChar+1) != '\0')
                lastChar++;

              if (*lastChar == '\\' || *lastChar == '/')
                *lastChar = '\0';

              vh->setDocumentRoot ((const char*)lcur->children->content);
            }
          else if (!xmlStrcmp (lcur->name, (const xmlChar *) "SYSROOT"))
            {
              char* lastChar = (char*)lcur->children->content;

              while (*(lastChar+1) != '\0')
                lastChar++;

              if (*lastChar == '\\' || *lastChar == '/')
                *lastChar = '\0';

              vh->setSystemRoot ((const char*)lcur->children->content);
            }
          else if (!xmlStrcmp (lcur->name, (const xmlChar *) "ACCESSLOG"))
            {
              loadXMLlogData ("ACCESSLOG", vh, lcur);
            }
          else if (!xmlStrcmp (lcur->name, (const xmlChar *) "WARNINGLOG"))
            {
              loadXMLlogData ("WARNINGLOG", vh, lcur);
            }
          else if (!xmlStrcmp (lcur->name, (const xmlChar *) "MIME_FILE"))
            {
              string hnd ("xml");
              for (xmlAttr *attrs = lcur->properties; attrs; attrs = attrs->next)
                {
                  if (!xmlStrcmp (attrs->name, (const xmlChar *) "name")
                      && attrs->children && attrs->children->content)
                    hnd.assign((const char*) attrs->children->content);
                }

              const char *filename = (const char*) lcur->children->content;
              MimeManagerHandler *handler =
                Server::getInstance ()->getMimeManager ()->buildHandler (hnd);

              try
                {
                  handler->load (filename);
                }
              catch (...)
                {
                  delete handler;
                  handler = NULL;
                  Server::getInstance ()->log (MYSERVER_LOG_MSG_ERROR,
                                         _("Error loading mime types file: %s"),
                                               filename);

                }
              vh->setMimeHandler (handler);
            }
          else if (!xmlStrcmp (lcur->name, (const xmlChar *) "THROTTLING_RATE"))
            {
              u_long rate = (u_long)atoi ((char*)lcur->children->content);
              vh->setThrottlingRate (rate);
            }

          lcur = lcur->next;
        }/* while (lcur)  */

      if (vh->openLogFiles ())
        {
          Server::getInstance ()->log (MYSERVER_LOG_MSG_ERROR,
                                             _("Error opening log files"));
          delete vh;
          vh = 0;
          continue;
        }

      if (vh->initializeSSL () < 0)
        {
          Server::getInstance ()->log (MYSERVER_LOG_MSG_ERROR,
                                             _("Error initializing SSL for %s"),
                                             vh->getName ());
          delete vh;
          vh = 0;
          continue;
        }

      if (addVHost (vh))
        {
          Server::getInstance ()->log (MYSERVER_LOG_MSG_ERROR,
                                             _("Internal error"));
          delete vh;
          vh = 0;
          continue;
        }
    }
  parser.close ();

  changeLocationsOwner ();

  return 0;
}

/*!
  \see VhostManager#getVHost
 */
Vhost* XmlVhostHandler::getVHost (u_long n)
{
  if (n > hosts.size ())
    return NULL;

  return hosts.at (n - 1);
}

/*!
  Remove a virtual host by its position in the list
  First position is zero.
  \param n The virtual host identifier in the list.
 */
int XmlVhostHandler::removeVHost (u_long n)
{
  vector<Vhost*>::iterator it = hosts.erase (hosts.begin () + n - 1);
  delete *it;
  return 0;
}
