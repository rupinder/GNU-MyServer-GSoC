/*
MyServer
Copyright (C) 2007, 2008 Free Software Foundation, Inc.
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

#include <include/plugin/plugin_info.h>


using namespace std;

/*!
 *Construct a plugin info object.
 *\param name plugin name.
 *\param enabled is true if the plugin has to be enabled. 
 *\param global is true if the plugin's symbols have to be loaded globally.
 */
PluginInfo::PluginInfo(string& name, bool enabled, bool global)
{
  PluginInfo(name,enabled,global,0,0,0);
}

/*!
 *Construct a plugin info object.
 *\param name plugin name.
 *\param enabled is true if the plugin has to be enabled. 
 *\param global is true if the plugin's symbols have to be loaded globally.
 *\param version the version of the plugin, in the format: a.b.c.d where a = version >> 24, b = (version >> 16) & 255, c = (version >> 8) & 255, d = version & 255.
 *\param msMinVersion is the minimum MyServer version plugin is compatible. 
 *\param msMaxVersion is the maximum MyServer version plugin is compatible. 
 */
PluginInfo::PluginInfo(string& name, bool enabled, bool global, int version, int msMinVersion, int msMaxVersion)
{
  this->name = name;	
  this->enabled = enabled;
  this->global = global;
  this->version = version;
  this->msMinVersion = msMinVersion;
  this->msMaxVersion = msMaxVersion;
  this->plugin = NULL;
}


/*!
 *Destroy the object.
*/	
PluginInfo::~PluginInfo()
{
	detachPlugin();
}

/*!
 *Returns true if the plugin is enabled.
*/		
bool PluginInfo::isEnabled()
{
  return this->enabled;	
}

/*!
 *Returns true if the plugin is loaded globally.
*/
bool PluginInfo::isGlobal()
{
  return this->global;	
}
	
/*!
 * Adds a dependece to the plugin.
 *\param name plugin name
*/
void PluginInfo::addDependence(string name, int minVersion, int maxVersion)
{	
  dependences.put(name, new pair<int,int>(minVersion,maxVersion));
}
	
/*!
 * Returns the plugin version.
*/
int PluginInfo::getVersion()
{
  return this->version;
}

/*!
 * Sets the plugin version.
*/
void PluginInfo::setVersion(int v)
{
  this->version = v;
}

/*!
 * Returns the minimum MyServer version plugin is compatible.
*/
int PluginInfo::getMyServerMinVersion()
{
  return this->msMinVersion;
}
	

/*!
 * Returns the maximum MyServer version plugin is compatible.
*/
int PluginInfo::getMyServerMaxVersion()
{
  return this->msMaxVersion;
}


/*!
 * Sets the minimum MyServer version plugin is compatible.
*/
int PluginInfo::setMyServerMinVersion(int v)
{
  this->msMinVersion = v;
}
	
/*!
 * Sets the maximum MyServer version plugin is compatible.
*/
int PluginInfo::setMyServerMaxVersion(int v)
{
  this->msMaxVersion = v;
}

/*!
 * Returns the plugin name
*/
string PluginInfo::getName()
{
  return this->name;
}

/*!
 * Attaches the corrispondent Plugin object.
*/
void PluginInfo::attachPlugin(Plugin* plugin)
{
	this->plugin = plugin;
}


/*!
 * Returns the attached plugin object.
*/
Plugin* PluginInfo::getPlugin()
{
  return this->plugin;
}
	
/*!
 * Detaches the Plugin object.
*/
void PluginInfo::detachPlugin()
{
  if (this->plugin!=NULL)
  {
  	delete(this->plugin);
    this->plugin = NULL;
  }
}



Regex* PluginInfo::regex = new Regex("^[1-2]?[1-9]?[0-9](\\.[1-2]?[0-9]?[0-9](\\.[1-2]?[0-9]?[0-9](\\.[1-2]?[0-9]?[0-9])?)?)?$",REG_EXTENDED | REG_NOSUB);

/*!
 * Converts a string in the format "a.b.c.d" in an int in the format abcd where each number takes 8 bit.
*/
int PluginInfo::convertVersion(string* s)
{
	
  int ret = regex->exec(s->c_str(),0,NULL,0);

  if (ret!=0)
    return -1;	
  	
  string::size_type pos = s->find(".",0);
  if (pos == string::npos)
    return atoi(s->c_str()) << 24;
    
  int n1 = 0;
  int n2 = 0;
  int n3 = 0;
  int n4 = 0;
  
  string sa = s->substr(0,pos);
  n1 = atoi(sa.c_str());
  if (n1>255)
    return -1;

  string::size_type oldpos = pos;
  
  
  if (oldpos!=string::npos)
  {
  	pos = s->find(".",oldpos+1);
  	string sa = s->substr(oldpos+1,pos - oldpos);
    n2 = atoi(sa.c_str());
    if (n2>255)
      return -1;
  }
  
  
  oldpos = pos;
  if (oldpos!=string::npos)
  {
  	pos = s->find(".",oldpos+1);
  	string sa = s->substr(oldpos+1,pos - oldpos);
    n3 = atoi(sa.c_str());
    if (n3>255)
      return -1;
  }
  
  
  oldpos = pos;
  if (oldpos!=string::npos)
  {
  	pos = s->find(".",oldpos+1);
  	string sa = s->substr(oldpos+1,pos - oldpos);
    n4 = atoi(sa.c_str());
    if (n4>255)
      return -1;
  }

  return (n1<<24) + (n2<<16) + (n3<<8) + n4;
}