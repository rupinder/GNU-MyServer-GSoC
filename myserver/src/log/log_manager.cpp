/*
  MyServer
  Copyright (C) 2006, 2008 Free Software Foundation, Inc.
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

#include <include/log/log_manager.h>

LogManager::LogManager (FiltersFactory* ff,
                        LoggingLevel level) : level (level)
{
  this->ff = ff;
  lsf = new LogStreamFactory ();
  mutex = new Mutex ();
  mutex->init ();
}

LogManager::~LogManager ()
{
  if (!empty ())
    clear ();
  delete mutex;
  delete lsf;
}

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
  mutex->unlock ();
  return !empty ();
}

int
LogManager::add (void* owner, string type, string location, 
                 list<string>& filters, u_long cycle)
{
  mutex->lock ();
  int success = 1;
  if (!contains (location))
    {
      LogStream* ls = lsf->create (ff, location, filters, cycle);
      if (ls)
        {
          success = (add (owner) || add (owner, type) ||
                     add (owner, type, location, ls));
        }
    }
  mutex->unlock ();
  return success;
}

int
LogManager::add (void* owner)
{
  int success = 1;
  if (owner)
    {
      if (!contains (owner))
        {
          map<string, map<string, LogStream*> > type;
          owners[owner] = type;
        }
      success = 0;
    }
  return success;
}

int
LogManager::add (void* owner, string type)
{
  int success = 1;
  if (type.size ())
    {
      if (!contains (owner, type))
        {
          map<string, LogStream*> target;
          owners[owner][type] = target;
        }
      success = 0;
    }
  return success;
}

int
LogManager::add (void* owner, string type, string location, LogStream* ls)
{
  logStreams[location] = ls;
  owners[owner][type][location] = ls;
  if (contains (location) && contains (owner, type, location))
    {
      return 0;
    }
  return 1;
}

/*
 * Remove all the logs owned by `owner' and `owner' as well.
 * After the call to this method, a call to contains (owner)
 * must return false.
 */
int 
LogManager::remove (void* owner)
{
  mutex->lock ();
  int success = 1;
  if (contains (owner))
    {
      map<string, map<string, LogStream*> > *m = &owners[owner];
      map<string, map<string, LogStream*> >::iterator it_1;
      for (it_1 = m->begin (); it_1 != m->end (); it_1++)
        {
          map<string, LogStream*> *t = &it_1->second;
          map<string, LogStream*>::iterator it_2;
          for (it_2 = t->begin (); it_2 != t->end (); it_2++)
            {
              delete it_2->second;
              logStreams.erase (it_2->first);
            }
          t->clear ();
        }
      m->clear ();
      owners.erase (owner);
      success = 0;
    }
  mutex->unlock ();
  return success;
}

int
LogManager::notify (void* owner, string type, string location, 
                    LogStreamEvent evt, void* message, void* reply)
{
  int success = 1;
  if (contains (owner, type, location))
    {
      success = owners[owner][type][location]->update (evt, message, reply);
    }
  return success;
}

int
LogManager::notify (void* owner, string type, LogStreamEvent evt, 
                    void* message, void* reply)
{
  int success = 1;
  if (contains (owner, type))
    {
      success = 0;
      map<string, LogStream*> m = owners[owner][type];
      map<string, LogStream*>::iterator it;
      for (it = m.begin (); it != m.end (); it++)
        {
          success |= notify (owner, type, it->first, evt, message, reply);
        }
    }
  return success;
}

int
LogManager::notify (void* owner, LogStreamEvent evt, void* message, 
                    void* reply)
{
  int success = 1;
  if (contains (owner))
    {
      success = 0;
      map<string, map<string, LogStream*> > m = owners[owner];
      map<string, map<string, LogStream*> >::iterator it;
      for (it = m.begin (); it != m.end (); it++)
        {
          success |= notify (owner, it->first, evt, message, reply);
        }
    }
  return success;
}

int
LogManager::chown (void* owner, int uid, int gid)
{
  int message[2];
  message[0] = uid;
  message[1] = gid;
  return notify (owner, MYSERVER_LOG_EVT_CHOWN, static_cast<void*>(message));
}

int
LogManager::chown (void* owner, string type, int uid, int gid)
{
  int message[2];
  message[0] = uid;
  message[1] = gid;
  return notify (owner, MYSERVER_LOG_EVT_CHOWN, static_cast<void*>(message));
}

int
LogManager::chown (void* owner, string type, string location, int uid, int gid)
{
  int message[2];
  message[0] = uid;
  message[1] = gid;
  return notify (owner, type, location, MYSERVER_LOG_EVT_CHOWN, static_cast<void*>(message));
}

int
LogManager::log (void* owner, string message, bool appendNL,
                 LoggingLevel level)
{
  int success = 1;
  if (level >= this->level)
    {
      success = 0;
      if (appendNL)
        {
          ostringstream oss;
          oss << message << endl;
          message.assign (oss.str ());
        }
      if (level == MYSERVER_LOG_MSG_ERROR)
        {
          success = 
            notify (owner, MYSERVER_LOG_EVT_ENTER_ERROR_MODE) ||
            notify (owner, MYSERVER_LOG_EVT_LOG, static_cast<void*>(&message)) ||
            notify (owner, MYSERVER_LOG_EVT_EXIT_ERROR_MODE);
        }
      else
        {
          success = notify (owner, MYSERVER_LOG_EVT_LOG, static_cast<void*>(&message));
        }
    }
  return success;
}

int
LogManager::log (void* owner, string type, string message, bool appendNL,
                 LoggingLevel level)
{
  int success = 1;
  if (level >= this->level)
    {
      success = 0;
      if (appendNL)
        {
          ostringstream oss;
          oss << message << endl;
          message.assign (oss.str ());
        }
      if (level == MYSERVER_LOG_MSG_ERROR)
        {
          success = 
            notify (owner, type, MYSERVER_LOG_EVT_ENTER_ERROR_MODE) ||
            notify (owner, type, MYSERVER_LOG_EVT_LOG, static_cast<void*>(&message)) ||
            notify (owner, type, MYSERVER_LOG_EVT_EXIT_ERROR_MODE);
        }
      else
        {
          success = notify (owner, type, MYSERVER_LOG_EVT_LOG, static_cast<void*>(&message));
        }
    }
  return success;
}

int
LogManager::log (void* owner, string type, string location, string message, 
                 bool appendNL, LoggingLevel level)
{
  int success = 1;
  if (level >= this->level)
    {
      if (appendNL)
        {
          ostringstream oss;
          oss << message << endl;
          message.assign (oss.str ());
        }
      if (level == MYSERVER_LOG_MSG_ERROR)
        {
          success = 
            notify (owner, type, location, MYSERVER_LOG_EVT_ENTER_ERROR_MODE) ||
            notify (owner, type, location, MYSERVER_LOG_EVT_LOG, static_cast<void*>(&message)) ||
            notify (owner, type, location, MYSERVER_LOG_EVT_EXIT_ERROR_MODE);
        }
      else
        {
          success = notify (owner, type, location, MYSERVER_LOG_EVT_LOG, 
                            static_cast<void*>(&message));
        }
    }
  return success;
}

int
LogManager::close (void* owner)
{
  return notify (owner, MYSERVER_LOG_EVT_CLOSE);
}

int
LogManager::close (void* owner, string type)
{
  return notify (owner, type, MYSERVER_LOG_EVT_CLOSE);
}

int
LogManager::close (void* owner, string type, string location)
{
  return notify (owner, type, location, MYSERVER_LOG_EVT_CLOSE);
}

int
LogManager::setCycle (void* owner, u_long cycle)
{
  return notify (owner, MYSERVER_LOG_EVT_SET_CYCLE, static_cast<void*>(&cycle));
}

int
LogManager::setCycle (void* owner, string type, u_long cycle)
{
  return notify (owner, type, MYSERVER_LOG_EVT_SET_CYCLE, static_cast<void*>(&cycle));
}

int
LogManager::setCycle (void* owner, string type, string location, u_long cycle)
{
  return notify (owner, type, location, MYSERVER_LOG_EVT_SET_CYCLE, 
                 static_cast<void*>(&cycle));
}

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

int
LogManager::get (void* owner, list<string>* l)
{
  if (contains (owner))
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

int
LogManager::get (void* owner, string type, list<string>* l)
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

int
LogManager::get (void* owner, string type, string location, LogStream** ls)
{
  if (contains (owner, type, location))
    {
      *ls = owners[owner][type][location];
      return 0;
    }
  return 1;
}

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

LoggingLevel
LogManager::setLevel (LoggingLevel level)
{
  mutex->lock ();
  LoggingLevel oldLevel = level;
  this->level = level;
  mutex->unlock ();
  return oldLevel;
}

LoggingLevel
LogManager::getLevel ()
{
  return level;
}

void
LogManager::setFiltersFactory (FiltersFactory* ff)
{
  mutex->lock ();
  this->ff = ff;
  mutex->unlock ();
}

FiltersFactory* 
LogManager::getFiltersFactory ()
{
  return ff;
}

bool
LogManager::empty ()
{
  return logStreams.size () == 0 && owners.size () == 0;
}

bool
LogManager::contains (string location)
{
  return logStreams.count (location) > 0;
}

bool
LogManager::contains (void* owner)
{
  return owners.count (owner) > 0;
}

bool
LogManager::contains (void* owner, string type)
{
  return owners[owner].count (type) > 0;
}

bool
LogManager::contains (void* owner, string type, string location)
{
  return owners[owner][type].count (location) > 0;
}

int 
LogManager::count (void* owner)
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

int
LogManager::count (void* owner, string type)
{
  return owners[owner][type].size ();
}

int
LogManager::count (void* owner, string type, string location)
{
  return owners[owner][type].count (location);
}
