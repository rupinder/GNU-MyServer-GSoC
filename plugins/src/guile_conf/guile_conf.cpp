/*
  MyServer
  Copyright (C) 2009, 2010 The Free Software Foundation Inc.
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
#include <myserver.h>

#include <libguile.h>
#include <guile/gh.h>
#include <include/plugin/plugin.h>
#include <include/conf/main/main_configuration.h>
#include <include/server/server.h>

#include <string.h>

#define CONF_FILE_NAME "myserver.sch"

PLUGIN_NAME ("guile_conf");

/*! Define the interface to read from the main configuration file.  */
class GuileConfiguration : public MainConfiguration
{
public:
  virtual const char *getValue (const char* field);
  virtual void readData (list<NodeTree<string>*> *hashedDataTrees,
                         HashMap<string, NodeTree<string>*> *hashedData);
};


static MainConfiguration *genGuileMainConf (Server *server, const char *arg)
{
  return new GuileConfiguration ();
}


extern MainConfiguration* (*genMainConf) (Server *server, const char *arg);

const char *
GuileConfiguration::getValue (const char* field)
{
  /* TODO.  */
  return NULL;
}


static NodeTree<string>*
traverse (SCM node, HashMap<string, NodeTree<string>*> *hashedData)
{
  NodeTree<string> *newNode = new NodeTree<string> ();
  if (gh_list_p (node))
    {
      while (gh_pair_p (gh_car (node)) && gh_symbol_p (gh_car (gh_car (node))))
        {
          size_t len;
          char* attr = gh_symbol2newstr (gh_car (gh_car (node)), &len);
          char* value = gh_scm2newstr (gh_cdr (gh_car (node)), &len);

          string attrStr (attr);
          string valueStr (value);

          if (! strcmp (attr, "name"))
            hashedData->put (valueStr, newNode);

          newNode->addAttr (attrStr, valueStr);
          node = gh_cdr (node);
          free (attr);
          free (value);
        }

      if (gh_string_p (gh_car (node)))
        {
          size_t len;
          char *str = gh_scm2newstr (gh_car (node), &len);
          newNode->setValue (new string (str));
          free (str);
        }
      else
        {
          while (! gh_null_p (node))
            {
              NodeTree<string>* child = traverse (gh_car (node), hashedData);
              newNode->addChild (child);
              node = gh_cdr (node);
            }
        }
    }

  return newNode;
}

static Server *serverInstance;

void
GuileConfiguration::readData (list<NodeTree<string>*> *hashedDataTrees,
                              HashMap<string, NodeTree<string>*> *hashedData)
{
  gh_eval_file (CONF_FILE_NAME);
  SCM list = gh_lookup ("configuration");

  if (gh_null_p (list))
    {
      serverInstance->log (MYSERVER_LOG_MSG_ERROR,
                   _("GuileConf: cannot find symbol: %s"), "`configuration'");
      return;
    }

  while (! gh_null_p (list))
    {
      hashedDataTrees->push_back (traverse (gh_car (list), hashedData));
      list = gh_cdr (list);
    }
}

EXPORTABLE(int) load (void* server)
{
  /* TODO: This plugin can be loaded only with --plugins.  Fail otherwise.  */
  scm_init_guile ();
  serverInstance = static_cast<Server *> (server);

  if (! FilesUtility::nodeExists (CONF_FILE_NAME))
    {
      serverInstance->log (MYSERVER_LOG_MSG_ERROR,
                           _("GuileConf: cannot find file %s"), CONF_FILE_NAME);
      return 1;
    }

  genMainConf = &genGuileMainConf;
  return 0;
}

EXPORTABLE(int) postLoad (void* server)
{
  return 0;
}

EXPORTABLE(int) unLoad ()
{
  return 0;
}
