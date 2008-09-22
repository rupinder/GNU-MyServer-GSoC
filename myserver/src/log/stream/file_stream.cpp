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

#include <include/log/stream/file_stream.h>

int const FileStream::defaultFileMask = File::MYSERVER_OPEN_APPEND | 
				       File::MYSERVER_OPEN_ALWAYS |
				       File::MYSERVER_OPEN_WRITE | 
				       File::MYSERVER_OPEN_READ | 
				       File::MYSERVER_NO_INHERIT;

FileStream::FileStream (FiltersFactory* filtersFactory,
			u_long cycleLog,
			Stream* outStream,
			FiltersChain* filtersChain) : 
  LogStream (filtersFactory, cycleLog, outStream, filtersChain)
{
}

u_long
FileStream::streamSize ()
{
  return dynamic_cast<File*>(outStream)->getFileSize ();
}

int
FileStream::streamCycle ()
{
  char *buffer = 0;
  char *buffer2 = 0;
  const u_long bufferSize = MYSERVER_KB (64);
  try
    {
      string filepath;
      string filedir;
      string filename;
      string ext;
      ostringstream newfilename;
      string time;

      filepath.assign (dynamic_cast<File*>(outStream)->getFilename ());
      FilesUtility::completePath (filepath);
      FilesUtility::splitPath (filepath, filedir, filename);
      
      FilesUtility::getFileExt (ext, filename);
      
      getRFC822LocalTime (time, 32);
      time = trim (time.substr (5, 32));
    
      for (int i = 0; i < static_cast<int>(time.length ()); i++)
	if ((time[i] == ' ') || (time[i] == ':'))
	  time[i]= '.';
      if (ext.size ())
	{
	  filename = (filename.substr (0, filename.find (string (".") + ext)));
	}
      newfilename << filedir << "/" << filename << "." << time <<
	(ext.size () ? "." : "") << ext;

      File newFile;
      File *currentFile = dynamic_cast<File*>(outStream);

      buffer = new char[bufferSize];
      if (buffer == 0)
	return 1;
      buffer2 = new char[bufferSize];
      if (buffer == 0)
	{
	  delete [] buffer;
	  return 1;
	}
     
      if (newFile.openFile (newfilename.str ().c_str (),
			    File::MYSERVER_OPEN_WRITE |
			    File::MYSERVER_NO_INHERIT |
			    File::MYSERVER_CREATE_ALWAYS))
	{
	  delete [] buffer;
	  delete [] buffer2;
	  return 1;
	}
      if (currentFile->setFilePointer (0))
	{
	  delete [] buffer;
	  delete [] buffer2;
	  newFile.close ();
	  return 1;
	}
      for (;;)
	{
	  u_long nbr;
	  u_long nbw;
	  if(currentFile->readFromFile (buffer, bufferSize, &nbr))
	    {
	      delete [] buffer;
	      delete [] buffer2;
	      newFile.close ();
	      return 1;
	    }
	  if (nbr == 0)
	    break;
	  if (newFile.writeToFile (buffer, nbr, &nbw))
	    {
	      delete [] buffer;
	      delete [] buffer2;
	      newFile.close ();
	      return 1;
	    }
	}
      newFile.close ();
      currentFile->close ();
      FilesUtility::deleteFile (filepath.c_str ());
      if (currentFile->openFile (filepath.c_str(), defaultFileMask))
	{
	  delete [] buffer;
	  delete [] buffer2;
	  return 1;
	}
      delete [] buffer;
      delete [] buffer2;
      return 0;
    }
  catch (...)
    {
      if (buffer)
	delete [] buffer;
      if (buffer2)
	delete [] buffer2;
      throw;
    };
}
