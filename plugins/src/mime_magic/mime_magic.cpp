/*
MyServer
Copyright (C) 2007, 2009 The Free Software Foundation Inc.
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

#include <stdafx.h>
#include <string.h>
#include <include/server/server.h>
#include <include/base/multicast/multicast.h>
#include <include/protocol/http/http.h>
#include <include/plugin/plugin.h>
#include <include/conf/mime/mime_manager.h>
#include <include/base/hash_map/hash_map.h>
#include <magic.h>

#ifdef WIN32
#define EXPORTABLE(x) x _declspec(dllexport)
#else
#define EXPORTABLE(x) extern "C" x
#endif

class MagicHandler : public MimeManagerHandler
{
public:
  MagicHandler ()
  {

  }

  virtual u_long load (const char *resource)
  {
    cookie = magic_open (MAGIC_SYMLINK | MAGIC_MIME_TYPE);
    return magic_load (cookie, NULL);
  }

  virtual void close ()
  {
    HashMap<string, MimeRecord*>::Iterator it = records.begin ();
    for (; it != records.end (); it++)
      delete *it;

    magic_close (cookie);
  }

  virtual ~MagicHandler ()
  {
    close ();
  }

	virtual MimeRecord *getMIME (const char *file)
  {
    MimeRecord *rec = NULL;
    lock.lock ();
    try
      {
        /* FIXME: is this line reentrant and can be moved outside of the
         * critical section?  */
        const char *type = magic_file (cookie, file);

        /* Do not handle directories.  */
        if (type && strcmp (type, "application/x-directory"))
          {
            rec = records.get (type);
            if (!rec)
              {
                rec = new MimeRecord;
                rec->mimeType.assign (type);
                records.put (rec->mimeType, rec);
              }
          }
      }
    catch (...)
      {
        lock.unlock ();
        return NULL;
      }

    lock.unlock ();

    return rec;
  }

private:
  magic_t cookie;
  Mutex lock;
  HashMap<string, MimeRecord*> records;
};

static MagicHandler *handler;

EXPORTABLE(char*) name (char* name, u_long len)
{
	char* str = (char*) "mime_magic";
	if (name)
		strncpy (name, str, len);
	return str;
}

EXPORTABLE(int) load (void* server)
{
  handler = NULL;
	return 0;
}

static MimeManagerHandler *builder ()
{
  return new XmlMimeHandler ();
}

void XmlMimeHandler::registerBuilder (MimeManager& manager)
{
}

EXPORTABLE(int) postLoad (void* server)
{
  string name ("mime_magic");
	Server *serverInstance = (Server*)server;
  MimeManager *mimeManager = serverInstance->getMimeManager ();

  MagicHandler *handler = new MagicHandler;
  if (handler->load (NULL))
    {
      serverInstance->log (MYSERVER_LOG_MSG_ERROR,
                                  _("cannot load mime magic configuration"));
      return 1;
    }

  mimeManager->registerHandler (name, handler);
  mimeManager->registerBuilder (name, builder);
	return 0;
}

EXPORTABLE(int) unLoad()
{
  if (handler)
    delete handler;

	return 0;
}
