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

int const FileStream::defaultFileMask = 
  File::MYSERVER_OPEN_APPEND | 
  File::MYSERVER_OPEN_ALWAYS |
  File::MYSERVER_OPEN_WRITE | 
  File::MYSERVER_OPEN_READ | 
  File::MYSERVER_NO_INHERIT;

FileStream::FileStream (FiltersFactory* ff, u_long cycle, Stream* out,
                        FiltersChain* fc) : 
  LogStream (ff, cycle, out, fc)
{
}

u_long
FileStream::streamSize ()
{
  return dynamic_cast<File*>(out)->getFileSize ();
}

int
FileStream::streamCycle ()
{
  char *buffer = 0;
  char *secondaryBuffer = 0;
  const u_long bufferSize = MYSERVER_KB (64);
  try
    {
      File newFile;
      File *currentFile = dynamic_cast<File*>(out);
      string filepath (currentFile->getFilename ());
      string newFileName (makeNewFileName (currentFile->getFilename ()));
      cycledStreams.push_back (newFileName);
      buffer = new char[bufferSize];
      if (buffer == 0)
        {
          return 1;
        }
      secondaryBuffer = new char[bufferSize];
      if (buffer == 0)
        {
          delete [] buffer;
          return 1;
        }
     
      if (newFile.openFile (newFileName,
                            File::MYSERVER_OPEN_WRITE |
                            File::MYSERVER_NO_INHERIT |
                            File::MYSERVER_CREATE_ALWAYS))
        {
          cerr << "could not open " << newFileName << endl;
          delete [] buffer;
          delete [] secondaryBuffer;
          return 1;
        }
      if (currentFile->seek (0))
        {
          delete [] buffer;
          delete [] secondaryBuffer;
          newFile.close ();
          return 1;
        }
      for (;;)
        {
          u_long nbr;
          u_long nbw;
          if(currentFile->read (buffer, bufferSize, &nbr))
            {
              delete [] buffer;
              delete [] secondaryBuffer;
              newFile.close ();
              return 1;
            }
          if (nbr == 0)
            break;
          if (newFile.writeToFile (buffer, nbr, &nbw))
            {
              delete [] buffer;
              delete [] secondaryBuffer;
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
          delete [] secondaryBuffer;
          return 1;
        }
      delete [] buffer;
      delete [] secondaryBuffer;
      return 0;
    }
  catch (...)
    {
      if (buffer)
        delete [] buffer;
      if (secondaryBuffer)
        delete [] secondaryBuffer;
      throw;
    };
}

string
FileStream::makeNewFileName (string oldFileName)
{
  string filedir;
  string filename;
  string ext;
  ostringstream newfilename;
  string time;

  FilesUtility::completePath (oldFileName);
  FilesUtility::splitPath (oldFileName, filedir, filename);
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
  newfilename << filedir << filename << "." << time <<
    (ext.size () ? "." : "") << ext;
  return newfilename.str ();
}

int
FileStream::chown (int uid, int gid)
{
  mutex->lock ();
  File* f = dynamic_cast<File*>(out);
  int success =  FilesUtility::chown (f->getFilename (), uid, gid);
  mutex->unlock ();
  return success;
}
