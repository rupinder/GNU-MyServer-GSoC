/*
  MyServer
  Copyright (C) 2006, 2008, 2009 Free Software Foundation, Inc.
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
  File::APPEND |
  File::FILE_OPEN_ALWAYS |
  File::WRITE |
  File::READ |
  File::NO_INHERIT;

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
  File *currentFile = dynamic_cast<File*>(out);
  string filepath (currentFile->getFilename ());
  string newFileName (makeNewFileName (currentFile->getFilename ()));

  if (FilesUtility::copyFile (currentFile->getFilename (), newFileName.c_str (), 1))
    return 1;

  cycledStreams.push_back (newFileName);

  currentFile->truncate ();

  return 0;
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
FileStream::chown (string &uid, string &gid)
{
  mutex->lock ();
  File* f = dynamic_cast<File*>(out);
  int success =  FilesUtility::chown (f->getFilename (), uid, gid);
  mutex->unlock ();
  return success;
}
