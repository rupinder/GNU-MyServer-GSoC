/*
*MyServer
*Copyright (C) 2005 The MyServer Team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "../stdafx.h"
#include "../include/stream.h"
#include "../include/filter.h"
#include "../include/dyn_filter.h"
#include "../include/lfind.h"
#include <string>
#include <sstream>

using namespace std;

typedef int (*getHeaderPROC)(u_long, void*, char*, u_long, u_long*); 
typedef int (*getFooterPROC)(u_long, void*, char*, u_long, u_long*);
typedef int (*readPROC)(u_long, void*, char*, u_long, u_long*);
typedef int (*writePROC)(u_long, void*, const char*, u_long, u_long*);
typedef int (*flushPROC)(u_long, void*, u_long*);
typedef int (*modifyDataPROC)(u_long, void*);
typedef int (*loadPROC)();
typedef int (*unloadPROC)();
typedef const char* (*getNamePROC)(u_long, void*, char*, u_long);

/*! Single instance of the class. */
DynamicFiltersManager * DynamicFiltersManager::dynamicfiltersmanager = 0;

/*! Mutex for the filters counter. */
Mutex DynamicFiltersManager::counterMutex;

/*! Count the used filters. */
u_long DynamicFiltersManager::counter=0;

/*!
 *Construct the object.
 */
DynamicFilterFile::DynamicFilterFile()
{

}

/*!
 *Destroy the object.
 */
DynamicFilterFile::~DynamicFilterFile()
{

}

/*!
 *Load a filter from a file. Returns zero on success.
 */
int DynamicFilterFile::loadFromFile(const char* name)
{
  int ret = file.loadLibrary(name);
  if(!ret)
  {
    loadPROC proc=(loadPROC)file.getProc("load"); 
    if(proc)
      return proc();
  }
  return ret;
}

/*!
 *Close the filter.
 */
int DynamicFilterFile::close()
{
  if(file.validHandle())
  {
    unloadPROC proc=(unloadPROC)file.getProc("unload"); 
    if(proc)
      proc();
  }
  return file.close();
}

/*!
 *Get the filter header.
 */
int DynamicFilterFile::getHeader(u_long id, Stream* s, char* buffer, 
                                 u_long len, u_long* nbw)
{
  getHeaderPROC proc;
  if(!file.validHandle())
    return -1;
  proc = (getHeaderPROC) file.getProc("getHeader");
  if(proc)
    return proc(id, s, buffer, len, nbw);

  return -1;
}

/*!
 *Get the filter footer.
 */
int DynamicFilterFile::getFooter(u_long id, Stream* s, char* buffer, 
                                 u_long len, u_long* nbw)
{
  getFooterPROC proc;
  if(!file.validHandle())
    return -1;
  proc = (getFooterPROC) file.getProc("getFooter");
  if(proc)
    return proc(id, s, buffer, len, nbw);

  return -1;
}

/*!
 *Read using the filter from the specified stream.
 */
int DynamicFilterFile::read(u_long id, Stream* s, char* buffer, 
                            u_long len, u_long* nbr)
{
  readPROC proc;
  if(!file.validHandle())
    return -1;
  proc = (readPROC) file.getProc("read");
  if(proc)
    return proc(id, s, buffer, len, nbr);

  return -1;
}

/*!
 *Write to the stream using the filter.
 */
int DynamicFilterFile::write(u_long id, Stream* s, const char* buffer, 
                             u_long len, u_long* nbw)
{
  writePROC proc;
  if(!file.validHandle())
    return -1;
  proc = (writePROC) file.getProc("write");
  if(proc)
    return proc(id, s, buffer, len, nbw);

  return -1;
}

/*!
 *Flush remaining data to the stream.
 */
int DynamicFilterFile::flush(u_long id, Stream* s, u_long* nbw)
{
  flushPROC proc;
  if(!file.validHandle())
    return -1;
  proc = (flushPROC) file.getProc("flush");
  if(proc)
    return proc(id, s, nbw);

  return -1;
}

/*!
 *Check if the filter modifies the data.
 */
int DynamicFilterFile::modifyData(u_long id, Stream* s )
{
  modifyDataPROC proc;
  if(!file.validHandle())
    return -1;
  proc = (modifyDataPROC) file.getProc("modifyData");
  if(proc)
    return proc(id, s);

  return -1;
}

/*!
 *Get the name for the filter.
 */
const char* DynamicFilterFile::getName(u_long id, Stream* s, 
                                       char* buffer, u_long len)
{
  getNamePROC proc;
  if(!file.validHandle())
    return 0;
  proc = (getNamePROC) file.getProc("getName");
  if(proc)
    return proc(id, s, buffer, len);

  return 0;
}

/*!
 *Read [len] characters using the filter. Returns -1 on errors.
 */
int DynamicFilter::read(char* buffer, u_long len, u_long *nbr)
{
  *nbr=0; 
  if(!file)
    return 0;

  return file->read(id, stream, buffer, len, nbr);
}

/*!
 *Write [len] characters to the stream. Returns -1 on errors.
 */
int DynamicFilter::write(const char* buffer, 
                         u_long len, u_long* nbw)
{
  *nbw=0; 
  if(!file)
    return 0;
  
  return file->write(id, stream, buffer, len, nbw);
}

/*!
 *Get an header for the filter. Returns -1 on errors.
 */
int DynamicFilter::getHeader(char* buffer, 
                             u_long len, u_long* nbw)
{
  *nbw=0; 
  if(!file)
    return 0;
  
  return file->getHeader(id, stream, buffer, len, nbw);
}

/*!
 *Get a footer for the filter. Returns -1 on errors.
 */
int DynamicFilter::getFooter(char* buffer, u_long len, u_long* nbw)
{
  *nbw=0; 
  if(!file)
    return 0;
  
  return file->getFooter(id, stream, buffer, len, nbw);
}

/*!
 *Construct the DynamicFilter object.
 */
DynamicFilter::DynamicFilter()
{
  parent = 0;
}

/*!
 *Destroy the DynamicFilter object.
 */
DynamicFilter::~DynamicFilter()
{

}

/*!
 *Set a numeric ID for the filter object.
 */
void DynamicFilter::setId(u_long i)
{
  id = i;
}

/*!
 *Get the numeric ID for the filter.
 */
u_long DynamicFilter::getId()
{
  return id;
}

/*!
 *Set the stream where apply the filter.
 */
void DynamicFilter::setParent(Stream* p)
{
  parent = p;
}


/*!
 *Construct the object passing a stream and a numeric ID.
 */
DynamicFilter::DynamicFilter(Stream* s, u_long i)
{
  stream = s;
  id=i;

}

/*!
 *Flush everything to the stream. Returns -1 on errors.
 */
int DynamicFilter::flush(u_long *nbw)
{
  *nbw = 0;
  return 0;
}

/*!
 *Get the stream used by the filter.
 */
Stream* DynamicFilter::getParent()
{
  return parent;
}

/*!
 *Returns a nonzero value if the filter modify the input/output data.
 */
int DynamicFilter::modifyData()
{
  if(!file)
    return 0;
  return file->modifyData(id, stream);
}

/*!
 *Return a string with the filter name. 
 *If an external buffer is provided write the name there too.
 */
const char* DynamicFilter::getName(char* name, u_long len)
{
  if(!file)
    return 0;
  return file->getName(id, stream, name, len);
}



/*!
 *Construct the object.
 */
DynamicFiltersManager::DynamicFiltersManager()
{
  if(dynamicfiltersmanager)
    dynamicfiltersmanager->clear();
  dynamicfiltersmanager = this;
}

/*!
 *Destroy the object.
 */
DynamicFiltersManager::~DynamicFiltersManager()
{
  clear();
}

/*!
 *Clear everything.
 */
void DynamicFiltersManager::clear()
{
  int i;
  for(i=0; i< filters.size(); i++)
  {
    DynamicFilterFile *dff = filters.getData(i);
    dff->close();
  }
  filters.clear();
}

/*!
 *Load all the modules in the directory.
 */
int DynamicFiltersManager::loadFilters(const char* dir, XmlParser* parser)
{
	FindData fd;
	int ret;	
  string filename;
  string completeFileName;	


#ifdef WIN32
  filename.assign(dir);
  filename.append("/*.*");
#endif	

#ifdef NOT_WIN
	filename.assign(dir);
#endif	

  ret = fd.findfirst(dir);

  if(ret==-1)
		return 0;

	do
	{	
		if(fd.name[0]=='.')
			continue;
		/*!
     *Do not consider file other than dynamic libraries.
     */
#ifdef WIN32
		if(!strstr(fd.name,".dll"))
#endif
#ifdef NOT_WIN
		if(!strstr(fd.name,".so"))
#endif		
			continue;
    completeFileName.assign(dir);
    completeFileName.append("/");
    completeFileName.append(fd.name);
		if(add(completeFileName.c_str(), parser))
    {
      clear();
      fd.findclose();
      return -1;
    }
	}while(!fd.findnext());
	fd.findclose();
  return 0;
}

/*!
 *Add a filter to the collection.
 */
int DynamicFiltersManager::add(const char* file, XmlParser* /*parser*/)
{
  DynamicFilterFile* f;
  const char* name;

  if(!dynamicfiltersmanager)
    return 0;

  f = new DynamicFilterFile();
 
  if(f->loadFromFile(file))
  {
    delete f;
    return -1;
  }

  name = f->getName(0, 0, 0, 0);

  if(!name)
  {
    f->close();
    delete f;
    return -1;
  }

  dynamicfiltersmanager->filters.insert(name, f);
  return 0;
}

/*!
 *Factory method to create a dynamic filter. Return the new Filter on success.
 */
Filter* DynamicFiltersManager::createFilter(const char* name)
{
  DynamicFilterFile* file;
  DynamicFilter* filter;

  if(!dynamicfiltersmanager)
    return 0;

  file = dynamicfiltersmanager->filters.getData(name);

  if(!file)
    return 0;

  filter = new DynamicFilter();

  if(!file)
    return 0;

  counterMutex.lock();
  filter->setId(counter++);
  counterMutex.unlock();
  
  return filter; 
}

/*!
 *Register the loaded filters on the FiltersFactory object.
 */
int DynamicFiltersManager::registerFilters(FiltersFactory* ff)
{
  int i=0;

  if(!dynamicfiltersmanager)
    return -1;
  for(i=0; i< filters.size(); i++)
  {
    DynamicFilterFile* f = dynamicfiltersmanager->filters.getData(i);
    if(f)
    {
      if(ff->insert(f->getName(0, 0, 0, 0), DynamicFiltersManager::createFilter))
        return -1;
    }
  }
  return 0;
}
