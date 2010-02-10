/*
  MyServer
  Copyright (C) 2006, 2008, 2009, 2010 Free Software Foundation, Inc.
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

#undef remove

#include <include/log/log_manager.h>
#include <include/server/server.h>

#include <cstdarg>

/*!
 * The constructor.
 *
 * \param ff The FiltersFactory object.
 * \param level The default level of logging.
 */
LogManager::LogManager (FiltersFactory* ff,LoggingLevel level) : level (level)
{
  this->ff = ff;
  lsf = new LogStreamFactory ();
  mutex = new Mutex ();
  mutex->init ();
  computeNewLine ();
  associateLoggingLevelsWithNames ();
}

/*!
 * The destructor. Deallocates all the LogStream objects.
 */
LogManager::~LogManager ()
{
  if (!empty ())
    clear ();
  delete mutex;
  delete lsf;
}

/*!
 * Precalculate the newline string for the host operating system.
 *
 * \return 0 on success, 1 on error.
 */
int
LogManager::computeNewLine ()
{
  ostringstream oss;
  oss << endl;
  if (newline.assign (oss.str ()).size ())
    {
      return 0;
    }
  return 1;
}

/*!
 * Empty the LogManager.
 *
 * \return 0 on success, 1 on error.
 */
int
LogManager::clear ()
{
  mutex->lock ();
  map<string, LogStream*>::iterator it;
  for (it = logStreams.begin (); it != logStreams.end (); it++)
    {
      delete it->second;
    }
  logStreams.clear ();
  owners.clear ();
  logStreamOwners.clear ();
  mutex->unlock ();
  return !empty ();
}

/*!
 * Add a new LogStream element to the LogManager. The same LogStream can be
 * shared between different owner objects. This means that you can pass
 * multiple times the same string as `location' parameter, as well as `owner'
 * and `type', but you can't pass more than one time the same
 * <owner, type, location> tuple.
 *
 * \param owner The object that will own the LogStream that is being added.
 * \param type The category which the LogStream that is being added belongs to.
 * \param location The location string for the new LogStream.
 * \param filters A list of strings, each representing a valid filter name.
 * \param cycle The cycle value for the LogStream.
 *
 * \return 0 on success, 1 on error.
 */
int
LogManager::add (const void *owner, string type, string location,
                 list<string>& filters, u_long cycle)
{
  mutex->lock ();
  int failure = 1;
  if (!contains (location))
    {
      LogStream* ls = lsf->create (ff, location, filters, cycle);
      if (ls)
        {
          failure = add (owner) || add (owner, type) ||
            add (owner, type, location, ls);
        }
      mutex->unlock ();
      return failure;
    }

  int oldSize;
  int newSize;
  ostringstream oss;

  oldSize = logStreamOwners[location].size ();
  if (!containsOwner (owner))
    {
      failure = add (owner) || add (owner, type) ||
        add (owner, type, location, logStreams[location]);
    }
  else if (!contains (owner, type))
    {
      failure = add (owner, type) ||
        add (owner, type, location, logStreams[location]);
    }
  newSize = logStreamOwners[location].size ();

  mutex->unlock ();

  if (!failure && newSize > oldSize && Server::getInstance ())
    return Server::getInstance ()->log (MYSERVER_LOG_MSG_WARNING,
                                     _("The %s log is shared among %i objects"),
                                        location.c_str (), newSize);
  return failure;
}

/*!
 * Helper method that inserts a new owner within the LogManager
 * data structures.
 *
 * \param owner The new owner.
 *
 * \return 0 on success, 1 on error.
 */
int
LogManager::add (const void *owner)
{
  int failure = 1;
  if (owner)
    {
      if (!containsOwner (owner))
        {
          map<string, map<string, LogStream*> > type;
          owners[owner] = type;
        }
      failure = 0;
    }
  return failure;
}

/*!
 * Helper method that associates a new log category to an object.
 *
 * \param owner The owner of the new log category.
 * \param type The name of the new log category.
 *
 * \return 0 on success, 1 on error.
 */
int
LogManager::add (const void *owner, string type)
{
  int failure = 1;
  if (type.size ())
    {
      if (!contains (owner, type))
        {
          map<string, LogStream*> target;
          owners[owner][type] = target;
        }
      failure = 0;
    }
  return failure;
}

/*!
 * Helper method, that physically adds a LogStream to the
 * LogManager data structures.
 *
 * \param owner The owner object.
 * \param type The log category.
 * \param location The location string.
 * \param ls The LogStream object to add.
 *
 * \return 0 on success, 1 on error.
 */
int
LogManager::add (const void *owner, string type, string location, LogStream* ls)
{
  if (!contains (location))
    logStreams[location] = ls;

  if (!contains (owner, type, location))
    owners[owner][type][location] = ls;

  list<const void*>* l = &logStreamOwners[location];
  if (find (l->begin (), l->end (), owner) == l->end ())
    l->push_back (owner);

  if (contains (location) && contains (owner, type, location))
    return 0;

  return 1;
}

/*
 * Remove all the logs owned by `owner' and `owner' as well.
 * After the call to this method, a call to contains (owner)
 * must return false. If `owner' owns some LogStream shared with
 * another object, that LogStream will be left within the LogManager
 * until all its owners willn't be removed.
 *
 * \param owner The object whose LogStream objects will be removed.
 *
 * \return 0 on success, 1 on error.
 */
int
LogManager::remove (const void *owner)
{
  int failure = 1;


  mutex->lock ();

  if (!containsOwner (owner))
    {
      mutex->unlock ();
      return 1;
    }

  map<string, map<string, LogStream*> >* m = &owners[owner];
  map<string, map<string, LogStream*> >::iterator it_1;
  for (it_1 = m->begin (); it_1 != m->end (); it_1++)
    {
      map<string, LogStream*> *t = &it_1->second;
      map<string, LogStream*>::iterator it_2;
      for (it_2 = t->begin (); it_2 != t->end (); it_2++)
        {
          logStreamOwners[it_2->first].remove (owner);
          if (!logStreamOwners[it_2->first].size ())
            {
              delete it_2->second;
              logStreams.erase (it_2->first);
              logStreamOwners.erase (it_2->first);
            }
        }
      t->clear ();
    }
  m->clear ();
  owners.erase (owner);
  failure = 0;

  mutex->unlock ();
  return failure;
}

/*!
 * Deliver a message to a single LogStream object specified by the tuple
 * <owner, type, location>.
 *
 * \param owner The object which owns the target stream.
 * \param type The log category which the target stream belongs.
 * \param location The target stream.
 * \param evt The event of interest for the target stream. For the list
 * of all events that can be notified, check the `log_stream.h' header.
 * \param message The message to deliver.
 * \param reply The recipient reply.
 *
 * \return 0 on success, 1 on error.
 */
int
LogManager::notify (const void* owner, const string & type, const string &
                    location, LogStreamEvent evt, void* message, void* reply)
{
  int failure = 1;

  if (contains (owner, type, location))
    failure = owners[owner][type][location]->update (evt, message, reply);

  return failure;
}

/*!
 * Deliver a message to all the LogStream objects belonging to the `type'
 * log category of the `owner' object.
 *
 * \param owner The object which owns the target stream.
 * \param type The log category which the target stream belongs.
 * \param evt The event of interest for the target stream. For the list
 * of all events that can be notified, check the `log_stream.h' header.
 * \param message The message to deliver.
 * \param reply The recipient reply.
 *
 * \return 0 on success, 1 on error.
 */
int
LogManager::notify (const void* owner, const string & type, LogStreamEvent evt,
                    void* message, void* reply)
{
  int failure = 1;
  if (contains (owner, type))
    {
      failure = 0;
      map<string, LogStream*> m = owners[owner][type];
      map<string, LogStream*>::iterator it;
      for (it = m.begin (); it != m.end (); it++)
        failure |= notify (owner, type, it->first, evt, message, reply);
    }
  return failure;
}

/*!
 * Deliver a message to all the LogStream objects owned by `owner'.
 *
 * \param owner The object which owns the target stream.
 * \param evt The event of interest for the target stream. For the list
 * of all events that can be notified, check the `log_stream.h' header.
 * \param message The message to deliver.
 * \param reply The recipient reply.
 *
 * \return 0 on success, 1 on error.
 */
int
LogManager::notify (const void* owner, LogStreamEvent evt, void* message,
                    void* reply)
{
  int failure = 1;
  if (containsOwner (owner))
    {
      failure = 0;
      map<string, map<string, LogStream*> > m = owners[owner];
      map<string, map<string, LogStream*> >::iterator it;
      for (it = m.begin (); it != m.end (); it++)
        failure |= notify (owner, it->first, evt, message, reply);
    }
  return failure;
}

/*!
 * Change the owner for all the LogStream objects owned by `owner'.
 *
 * \param owner The object which owns the target stream.
 * \param uid The new uid for the stream.
 * \param gid The new gid for the stream.
 *
 * \return 0 on success, 1 on error.
 */
int
LogManager::chown (const void *owner, string &uid, string &gid)
{
  string message[2];
  message[0] = uid;
  message[1] = gid;
  return notify (owner, MYSERVER_LOG_EVT_CHOWN, static_cast<void*>(message));
}

/*!
 * Change the owner for a group of LogStream objects specified by the tuple
 * <owner, type>.
 *
 * \param owner The object which owns the target stream.
 * \param type The category where the target stream can be found.
 * \param uid The new uid for the stream.
 * \param gid The new gid for the stream.
 *
 * \return 0 on success, 1 on error.
 */
int
LogManager::chown (const void *owner, string type, string &uid, string &gid)
{
  string message[2];
  message[0] = uid;
  message[1] = gid;
  return notify (owner, MYSERVER_LOG_EVT_CHOWN, static_cast<void*>(message));
}

/*!
 * Change the owner for a LogStream specified by the tuple <owner, type, location>.
 *
 * \param owner The object which owns the target stream.
 * \param type The category where the target stream can be found.
 * \param location The target stream.
 * \param uid The new uid for the stream.
 * \param gid The new gid for the stream.
 *
 * \return 0 on success, 1 on error.
 */
int
LogManager::chown (const void *owner, string type, string location, string &uid, string &gid)
{
  string message[2];
  message[0] = uid;
  message[1] = gid;
  return notify (owner, type, location, MYSERVER_LOG_EVT_CHOWN, static_cast<void*>(message));
}

/*!
 * Write a message on all the LogStream objects owned by the `owner' object.
 *
 * \param owner The object that owns the category.
 * \param message The message to write.
 * \param appendNL If set, tells the LogManager to append a newline sequence
 * to the message.
 * \param level If less than the LogManager's logging level, the message
 * will be discarded.
 *
 * \return 0 on success, 1 on error.
 */
int
LogManager::log (const void* owner, string & message, bool appendNL,
                 LoggingLevel level)
{
  int failure = 0;
  if (level < this->level)
    return 1;

  mutex->lock ();
  try
    {
      failure = notify (owner, MYSERVER_LOG_EVT_SET_MODE,
                        static_cast<void*>(&level))
        || notify (owner,MYSERVER_LOG_EVT_LOG, &message);
      if (appendNL)
        {
          LoggingLevel l = MYSERVER_LOG_MSG_PLAIN;
          failure |= (notify (owner, MYSERVER_LOG_EVT_SET_MODE,
                              (static_cast<void*>(&l)))
                      || notify (owner, MYSERVER_LOG_EVT_LOG,
                                 (static_cast<void*>(&newline))));
        }
    }
  catch (...)
    {
      mutex->unlock ();
    }
  mutex->unlock ();

  return failure;
}

/*!
 * Write a message on all the LogStream objects belonging to the `type'
 * category of the `owner' object.
 *
 * \param owner The object that owns the category.
 * \param type The log category where to write.
 * \param message The message to write.
 * \param appendNL If set, tells the LogManager to append a newline sequence
 * to the message.
 * \param level If less than the LogManager's logging level, the message
 * will be discarded.
 *
 * \return 0 on success, 1 on error.
 */
int
LogManager::log (const void* owner, const string & type, string & message,
                 bool appendNL, LoggingLevel level)
{
  int failure = 0;
  if (level < this->level)
    return 1;

  mutex->lock ();
  try
    {
      failure = notify (owner, type, MYSERVER_LOG_EVT_SET_MODE,
                        static_cast<void*>(&level))
        || notify (owner, type, MYSERVER_LOG_EVT_LOG,
                   static_cast<void*>(&message));

      if (appendNL)
        {
          LoggingLevel l = MYSERVER_LOG_MSG_PLAIN;
          failure |= (notify (owner, MYSERVER_LOG_EVT_SET_MODE,
                              (static_cast<void*>(&l)))
                      || notify (owner, MYSERVER_LOG_EVT_LOG,
                                 (static_cast<void*>(&newline))));
        }
    }
  catch (...)
    {
      mutex->unlock ();
    }
  mutex->unlock ();

  return failure;
}

/*!
 * Write a message on a single LogStream, specified by the unique tuple
 * <owner, type, location>.
 *
 * \param owner The object that owns the LogStream.
 * \param type The log category where we want to write.
 * \param location The target LogStream.
 * \param message The message we want to write.
 * \param appendNL a flag that, if set, tells the LogManager to append
 * a new line sequence to the message, according to the host operating system
 * convention.
 * \param level The level of logging of this message. If it is less than
 * the LogManager's level of logging, the message will be discarded.
 *
 * \return 0 on success, 1 on error.
 */
int
LogManager::log (const void* owner, const string & type, const string & location,
                 string & message, bool appendNL, LoggingLevel level)
{
  int failure = 0;
  if (level < this->level)
    return 1;

  mutex->lock ();

  try
    {
      failure = notify (owner, type, MYSERVER_LOG_EVT_SET_MODE, &level)
        || notify (owner, type, MYSERVER_LOG_EVT_LOG, &message);

      if (appendNL)
        {
          LoggingLevel l = MYSERVER_LOG_MSG_PLAIN;
          failure |= notify (owner, MYSERVER_LOG_EVT_SET_MODE,
                             (static_cast<void*>(&l)))
            || notify (owner, MYSERVER_LOG_EVT_LOG,
                       static_cast<void*>(&newline));
        }
    }
  catch (...)
    {
      mutex->unlock ();
    }

  mutex->unlock ();
  return failure;
}


/*!
 * Write a message on a single LogStream specifying a formatting string.
 *
 * \see LogManager#log (void*, string, LoggingLevel, bool, bool, va_list)
 * \return 0 on success, 1 on error.
 */
int
LogManager::log (const void* owner, const string & type, LoggingLevel level,
                 bool ts, bool appendNL, const char *fmt, ...)
{
  int failure = 0;

  if (level < this->level)
    return 1;

  va_list argptr;

  va_start (argptr, fmt);

  failure = log (owner, type, level, ts, appendNL, fmt, argptr);

  va_end (argptr);

  return failure;
}

/*!
 * Write a message on a single LogStream specifying a formatting string.
 *
 * \param owner The object that owns the LogStream.
 * \param type The log category where we want to write.
 * \param message The message we want to write.
 * \param level The level of logging of this message. If it is less than
 * the LogManager's level of logging, the message will be discarded.
 * \param ts Specify if a timestamp should be added before the message.
 * \param appendNL a flag that, if set, tells the LogManager to append
 * a new line sequence to the message, according to the host operating system
 * convention.
 * \param fmt Specify the message to log, accepting an additional parameters
 * list, codes accepted are:
 * %s An const char* string.
 * %S A string*
 * %% A '%'.
 * %i An integer.
 *\param args Additional args used by fmt.
 *
 * \return 0 on success, 1 on error.
 */
int
LogManager::log (const void* owner, const string & type, LoggingLevel level,
                 bool ts, bool appendNL, const char *fmt, va_list args)
{
  int failure = 0;

  if (level < this->level)
    return 1;

  mutex->lock ();

  ostringstream oss;

  try
    {
      if (ts)
        {
          char time[38];
          int len;
          time[0] = '[';
          getRFC822GMTTime (&time[1], 32);
          len = strlen (time);
          time[len + 0] = ']';
          time[len + 1] = ' ';
          time[len + 2] = '-';
          time[len + 3] = '-';
          time[len + 4] = ' ';
          time[len + 5] = '\0';
          string timestr (time);
          failure |= notify (owner, MYSERVER_LOG_EVT_LOG, &timestr);
        }

      while (*fmt)
        {
          if (*fmt != '%')
            oss << *fmt;
          else
            {
              fmt++;
              switch (*fmt)
                {
                case 's':
                  oss << static_cast<const char*> (va_arg (args, const char*));
                  break;

                case 'S':
                  oss << static_cast<string*> (va_arg (args, string*));
                  break;

                case '%':
                  oss << '%';
                  break;

                case 'i':
                  oss << static_cast<int> (va_arg (args, int));
                  break;

                default:
                  mutex->unlock ();
                  return 1;
                }
            }
          fmt++;
        }

      string message = oss.str ();

      failure = notify (owner, type, MYSERVER_LOG_EVT_SET_MODE,
                        static_cast<void*>(&level))
             || notify (owner, type, MYSERVER_LOG_EVT_LOG, &message);

      if (appendNL)
        failure |= notify (owner, type, MYSERVER_LOG_EVT_LOG, &newline);
    }
  catch (...)
    {
      mutex->unlock ();
    }

  mutex->unlock ();
  return failure;

}

/*!
 * Close all the LogStream objects owned by `owner'.
 *
 * \param owner The object about which we want to close all the LogStream
 * objects.
 *
 * \return 0 on success, 1 on error.
 */
int
LogManager::close (const void *owner)
{
  return notify (owner, MYSERVER_LOG_EVT_CLOSE);
}

/*!
 * Close all the LogStream objects belonging to the `type'
 * category of the `owner' object.
 *
 * \param owner The object that owns the `type' category.
 * \param type The log category to close.
 *
 * \return 0 on success, 1 on error.
 */
int
LogManager::close (const void *owner, string type)
{
  return notify (owner, type, MYSERVER_LOG_EVT_CLOSE);
}

/*!
 * Close the LogStream specified by the unique tuple <owner, type, location>.
 *
 * \param owner The object that owns the LogStream.
 * \param type The log category.
 * \param location The LogStream location.
 *
 * \return 0 on success, 1 on error.
 */
int
LogManager::close (const void *owner, string type, string location)
{
  return notify (owner, type, location, MYSERVER_LOG_EVT_CLOSE);
}

/*!
 * Set the cycle value for all the LogStream objects owned by `owner'.
 *
 * \param owner The object that wants to modify the cycle value for its
 * LogStream objects.
 * \param cycle The new cycle value.
 *
 * \return 0 on success, 1 on error.
 */
int
LogManager::setCycle (const void *owner, u_long cycle)
{
  return notify (owner, MYSERVER_LOG_EVT_SET_CYCLE, static_cast<void*>(&cycle));
}

/*!
 * Set the cycle value for all the LogStream objects belonging to the
 * log category `type' of the `owner' object.
 *
 * \param owner The object which owns the `type' category.
 * \param type The log category to modify.
 * \param cycle The new cycle value.
 *
 * \return 0 on success, 1 on error.
 */
int
LogManager::setCycle (const void *owner, string type, u_long cycle)
{
  return notify (owner, type, MYSERVER_LOG_EVT_SET_CYCLE, static_cast<void*>(&cycle));
}

/*!
 * Set the cycle value for the LogStream specified by the unique tuple
 * <owner, type, location>.
 *
 * \param owner The object that owns the LogStream.
 * \param type The log category which the LogStream belongs.
 * \param location The LogStream location.
 * \param cycle The new cycle value.
 *
 * \return 0 on success, 1 on error.
 */
int
LogManager::setCycle (const void *owner, string type, string location, u_long cycle)
{
  return notify (owner, type, location, MYSERVER_LOG_EVT_SET_CYCLE,
                 static_cast<void*>(&cycle));
}

/*!
 * Return the cycle value for the `location' LogStream.
 *
 * \param location The LogStream about which we want to retrieve the cycle
 * value.
 * \param cycle On a successful method execution, the cycle value will be
 * placed here.
 *
 * \return 0 on success, 1 on error.
 */
int
LogManager::getCycle (string location, u_long* cycle)
{
  if (contains (location))
    {
      *cycle = logStreams[location]->getCycle ();
      return 0;
    }
  return 1;
}

/*!
 * Retrieve a list of strings each representing a LogStream location
 * of all the LogStreams owned by the owner object.
 *
 * \param owner The object about which we want to get all LogStream
 * object.
 * \param l The list that will be filled on successful method execution.
 *
 * \return 0 on success, 1 on error.
 */
int
LogManager::get (const void *owner, list<string>* l)
{
  if (containsOwner (owner))
    {
      l->clear ();
      map<string, map<string, LogStream*> > m = owners[owner];
      map<string, map<string, LogStream*> >::iterator it;
      for (it = m.begin (); it != m.end (); it++)
        {
          map<string, LogStream*> k = it->second;
          map<string, LogStream*>::iterator it_1;
          for (it_1 = k.begin (); it_1 != k.end (); it_1++)
            {
              l->push_back (it_1->first);
            }
        }
      return 0;
    }
  return 1;
}

/*!
 * Retrieve a list of strings each representing the location of
 * a LogStream belonging to the `type' category of the `owner' object.
 *
 * \param owner The object that should own the wanted information.
 * \param type The log category where the wanted information can be found.
 * \param l The list that will be filled on a successful method execution.
 *
 * \return 0 on success, 1 on error.
 */
int
LogManager::get (const void *owner, string type, list<string>* l)
{
  if (contains (owner, type))
    {
      l->clear ();
      map<string, LogStream*> m = owners[owner][type];
      map<string, LogStream*>::iterator it;
      for (it = m.begin (); it != m.end (); it++)
        {
          l->push_back (it->first);
        }
      return 0;
    }
  return 1;
}

/*!
 * Get a LogStream object by its location, the log category which it belongs
 * and its owner.
 *
 * \param owner The object that should own the LogStream.
 * \param type The log category which the LogStream that will be retrieved should
 * belong.
 * \param location The LogStream location.
 * \param ls On a successful method execution, the LogStream object will be
 * placed here.
 *
 * \return 0 on success, 1 on error.
 */
int
LogManager::get (const void *owner, string type, string location, LogStream** ls)
{
  if (contains (owner, type, location))
    {
      *ls = owners[owner][type][location];
      return 0;
    }
  return 1;
}

/*!
 * Retrieve the filters list used by the `location' LogStream.
 *
 * \param location The LogStream about which we want to retrieve the
 * filters list.
 * \param l A list that will be filled by a successful method execution.
 *
 * \return 0 on success, 1 on error.
 */
int
LogManager::getFilters (string location, list<string>* l)
{
  if (contains (location))
    {
      *l = logStreams[location]->getFiltersChain ()->getFilters ();
      return 0;
    }
  return 1;
}

/*!
 * Set the logging level for the LogManager. Check
 * the `log_stream.h' header file for more information about
 * the LoggingLevel enumeration.
 *
 * \param level the new level of logging.
 *
 * \return the old level of logging.
 */
LoggingLevel
LogManager::setLevel (LoggingLevel level)
{
  mutex->lock ();
  LoggingLevel oldLevel = level;
  this->level = level;
  mutex->unlock ();
  return oldLevel;
}

/*!
 * Return the level of logging used by the LogManager. Check
 * the `log_stream.h' header file for more information about
 * the LoggingLevel enumeration.
 *
 * \return the level of logging used by the LogManager
 */
LoggingLevel
LogManager::getLevel ()
{
  return level;
}

/*!
 * Set the FiltersFactory used by the LogManager.
 *
 * \param ff The FiltersFactory object that will be used by the LogManager.
 */
void
LogManager::setFiltersFactory (FiltersFactory* ff)
{
  mutex->lock ();
  this->ff = ff;
  mutex->unlock ();
}

/*!
 * Get the FiltersFactory used by the LogManager.
 *
 * \return the FiltersFactory object used by the LogManager.
 */
FiltersFactory*
LogManager::getFiltersFactory ()
{
  return ff;
}

/*!
 * Check if the LogManager is empty.
 *
 * \return true if the LogManager is empty.
 */
bool
LogManager::empty ()
{
  return
    logStreams.size () == 0 &&
    owners.size () == 0 &&
    logStreamOwners.size () == 0;
}

/*!
 * Check if a LogStream which points to `location' is already
 * present in the LogManager.
 *
 * \param location The `location' about which we want to know the
 * existence.
 *
 * \return true if `location' is already present within the LogManager.
 */
bool
LogManager::contains (const string & location)
{
  return logStreams.count (location) > 0
    && logStreamOwners[location].size ();
}

/*!
 * Query the LogManager to ask it if the `owner' object is actually
 * present within it.
 *
 * \param owner The object about which we want to know the existence within
 * the LogManager.
 *
 * \return true if `owner' is actually present within the LogManager.
 */
bool
LogManager::containsOwner (const void* owner)
{
  return owners.count (owner) > 0;
}

/*!
 * Query the LogManager to ask if the category `type' belongs to
 * the `owner' object.
 *
 * \param owner An object that may own some log category.
 * \param type The category we are interested in.
 *
 * \return true if the tupe <owner, type> exists.
 */
bool
LogManager::contains (const void* owner, const string & type)
{
  return owners[owner].count (type) > 0;
}

/*!
 * Query the LogManager to ask it if `location' belongs to the `type'
 * log category of the `owner' object.
 *
 * \param owner An object that may own some LogStream object.
 * \param type The log category that we are interested to query.
 * \param location The target of the query.
 *
 * \return true if the unique <owner, type, location> tuple exists.
 */
bool
LogManager::contains (const void* owner, const string & type, const string &
                      location)
{
  return owners[owner][type].count (location) > 0 &&
    (find (logStreamOwners[location].begin (), logStreamOwners[location].end (), owner) !=
     logStreamOwners[location].end ());
}

/*!
 * Given an owner object, get the number of LogStream objects that
 * belong to all its log categories.
 *
 * \param owner A pointer to an object that may own some LogStream.
 *
 * \return The total number of LogStream objects belonging to all
 * its log categories.
 */
int
LogManager::count (const void* owner)
{
  int size = 0;
  map<string, map<string, LogStream*> > m = owners[owner];
  map<string, map<string, LogStream*> >::iterator it;
  for (it = m.begin (); it != m.end (); it++)
    {
      size += it->second.size ();
    }
  return size;
}

/*!
 * Given an owner and one of its log categories, retrieve the
 * number of LogStream objects which belongs to that category.
 *
 * \param owner A pointer to an object which may own some LogStream.
 * \param type A log category owned by `owner'.
 *
 * \return The number of LogStream objects that belong to `type'.
 */
int
LogManager::count (const void* owner, const string & type)
{
  return owners[owner][type].size ();
}

/*!
 * Given an owner object, one of its log categories and
 * a location, returns the number of occurrences of `location'.
 *
 * \param owner A pointer to an object that may own `location'.
 * \param type The category of logs which `location' should belong.
 * \param location The location string.
 *
 * \return The number of occurrences of `location'. This number should never
 * be greater than 1, since the tuple <owner, type, location> is a key.
 */
int
LogManager::count (const void* owner, const string & type, const string &
                   location)
{
  return owners[owner][type].count (location);
}

/*!
 * Retrieve a list of objects that are currently using `location'.
 *
 * \param location We want to know which objects are using it.
 * \param l The location owners will be inserted here.
 *
 * \return 0 if location is owned at least by an object, 1 else.
 */
int
LogManager::getOwnersList (string location, list<const void*>* l)
{
  if (contains (location))
    {
      *l = logStreamOwners[location];
      return 0;
    }
  return 1;
}

/*!
 * \return a list of strings each representing one of logging levels that
 * MyServer understands.
 */
list<string>
LogManager::getLoggingLevelsByNames ()
{
  list<string> l;
  for (map<LoggingLevel, string>::iterator it = loggingLevels.begin ();
       it != loggingLevels.end (); it++)
      l.push_back (it->second);

  return l;
}

void
LogManager::associateLoggingLevelsWithNames ()
{
  loggingLevels[MYSERVER_LOG_MSG_INFO] = "info";
  loggingLevels[MYSERVER_LOG_MSG_WARNING] = "warning";
  loggingLevels[MYSERVER_LOG_MSG_ERROR] = "error";
}
