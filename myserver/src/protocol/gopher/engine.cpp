/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2002-2009 Free Software Foundation, Inc.
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

#include "stdafx.h"

#include <include/protocol/gopher/gopher_content.h>
#include <include/protocol/gopher/engine.h>
#include <include/conf/vhost/vhost.h>
#include <include/server/server.h>
#include <include/base/read_directory/read_directory.h>
#include <include/base/file/files_utility.h>
#include <include/base/file/file.h>
#include <include/base/mem_buff/mem_buff.h>


#include <iostream>
#include <fstream>
#include <sstream>

GopherEngine::GopherEngine()
{
  string info = "info";
  string cso = "cso";
  string binhex = "binhex";
  string dosbin = "dosbin";
  string UUencode = "UUencode";
  string telnet = "telnet";
  string bin = "bin";
  string image = "image";
  string gif = "gif";
  string text = "text";
  handlers.put (info, &GopherEngine::infoFile);
  handlers.put (text, &GopherEngine::textFile);
  handlers.put (cso, &GopherEngine::csoFile);
  handlers.put (binhex, &GopherEngine::binhexFile);
  handlers.put (dosbin, &GopherEngine::dosbinFile);
  handlers.put (UUencode, &GopherEngine::UUencodeFile);
  handlers.put (telnet, &GopherEngine::telnetFile);
  handlers.put (bin, &GopherEngine::binFile);
  handlers.put (image, &GopherEngine::imageFile);
  handlers.put (gif, &GopherEngine::gifFile);
}

GopherContent &GopherEngine::incoming(GopherRequest req, Vhost *host)
{
  ReadDirectory fd;
  GopherContent *Gud;
  GopherMenu *Gu;
  stringstream Port;
  Port << host->getPort ();
  port = Port.str ();
  hostname = req.getConnection ()->getLocalIpAddr ();
  abs_path = host->getDocumentRoot ();
  Host = host;

  string path = abs_path;
  path.append ("/");
  path.append (req.getRequest ());
  if(! fd.findfirst (path.c_str ()))
    {
      Gu = new GopherMenu;
      if(FilesUtility::getPathRecursionLevel (path)>0)
        {
          do {
            if(fd.name[0]!='.')
              {
                if (fd.attrib & FILE_ATTRIBUTE_DIRECTORY)
                  dirManagement (fd.name, req.getRequest (), *Gu);
                else
                  fileManagement (fd.name, req.getRequest (),  *Gu);
              }
          }while(!fd.findnext ());
        }
      else
        Gu->addIntro(string(_("Invalid requested Path\n")), hostname, port);
      fd.findclose ();
    }
  else
    {
      Gud = (GopherContent*)new GopherFileContent (path);
      return *Gud;
    }
  return *Gu;
}

void GopherEngine::dirManagement (const string &fname, const string &path, GopherMenu &tmp)
{
  string file_path = path;
  file_path.append ("/");
  file_path.append (fname);
  tmp.addItem (GopherDirectory (fname.c_str (),
        file_path.c_str (),
        hostname.c_str (),
        port.c_str ()));
}

void GopherEngine::fileManagement (const string &fname, const string &path,GopherMenu &tmp)
{
  string ext;
  string f_path = path;
  if (f_path != "")
    f_path.append ("/");
  f_path.append (fname);
  string abs_name = abs_path;
  if (abs_name != "")
    abs_name.append ("/");
  abs_name.append (f_path);

  FilesUtility::getFileExt (ext,fname);
  if (handlers.containsKey (ext.c_str ()))
    (this->*handlers.get(ext.c_str ())) (fname, path, tmp);
  else
    {
      MimeRecord *mime = getMIME (abs_name);
      if (mime!=NULL)
        {
          string mymime = mime->mimeType;
          mymime = mymime.substr (0,mymime.find ('/',0));
          if (handlers.containsKey (mymime.c_str ()))
            (this->*handlers.get (mymime.c_str ()))(fname, path, tmp);
        }
      else
        tmp.addItem (GopherFile (fname.c_str (),
                                 f_path.c_str (),
                                 hostname.c_str () ,
                                 port.c_str ()));
    }
}

/*!
 * Returns the MIME type passing its extension.
 * Returns zero if the file is registered.
 */
MimeRecord* GopherEngine::getMIME (string &filename)
{
  const char *handler = NULL;

  if (Host->isMIME ())
    return Host->getMIME ()->getMIME (filename);

  return Server::getInstance ()->getMimeManager ()->getMIME (filename, handler);
}

void GopherEngine::textFile(const string &fname, const string &path,GopherMenu &tmp)
{
  string f_path = path;
  if (f_path != "")
    f_path.append ("/");
  f_path.append (fname);
  tmp.addItem (GopherFile (fname.c_str (),
                           f_path.c_str (),
                           hostname.c_str () ,
                           port.c_str ()));
}
void GopherEngine::csoFile(const string &fname, const string &path,GopherMenu &tmp)
{
  string f_path = path;
  if (f_path != "")
    f_path.append ("/");
  f_path.append (fname);
  tmp.addItem (GopherCSO (fname.c_str (),
                          f_path.c_str (),
                          hostname.c_str () ,
                          port.c_str ()));
}

void GopherEngine::binhexFile (const string &fname, const string &path, GopherMenu &tmp)
{
  string f_path = path;
  if (f_path != "")
    f_path.append ("/");
  f_path.append (fname);
  tmp.addItem (GopherMacBinHex (fname.c_str (),
                                f_path.c_str (),
                                hostname.c_str () ,
                                port.c_str ()));
}

void GopherEngine::dosbinFile (const string &fname, const string &path, GopherMenu &tmp)
{
  string f_path = path;
  if (f_path != "")
    f_path.append ("/");
  f_path.append (fname);
  tmp.addItem (GopherDosBin (fname.c_str (),
                             f_path.c_str (),
                             hostname.c_str () ,
                             port.c_str ()));
}

void GopherEngine::UUencodeFile (const string &fname, const string &path, GopherMenu &tmp)
{
  string f_path = path;
  if (f_path != "")
    f_path.append ("/");
  f_path.append (fname);
  tmp.addItem (GopherUnixUUencode (fname.c_str (),
                                   f_path.c_str (),
                                   hostname.c_str () ,
                                   port.c_str ()));
}

void GopherEngine::telnetFile (const string &fname, const string &path, GopherMenu &tmp)
{
  string f_path = path;
  if (f_path != "")
    f_path.append ("/");
  f_path.append (fname);
  tmp.addItem (GopherTelnet (fname.c_str (),
                             f_path.c_str (),
                             hostname.c_str () ,
                             port.c_str ()));
}
void GopherEngine::binFile (const string &fname, const string &path, GopherMenu &tmp)
{
  string f_path = path;
  if (f_path != "")
    f_path.append ("/");
  f_path.append (fname);
  tmp.addItem (GopherBin (fname.c_str (),
                          f_path.c_str (),
                          hostname.c_str () ,
                          port.c_str ()));
}

void GopherEngine::imageFile (const string &fname, const string &path, GopherMenu &tmp)
{
  string f_path = path;
  if (f_path != "")
    f_path.append ("/");
  f_path.append (fname);
  tmp.addItem (GopherImage (fname.c_str (),
                            f_path.c_str (),
                            hostname.c_str () ,
                            port.c_str ()));
}

void GopherEngine::gifFile (const string &fname, const string &path, GopherMenu &tmp)
{
  string f_path = path;
  if (f_path != "")
    f_path.append ("/");
  f_path.append (fname);
  tmp.addItem (GopherGif (fname.c_str (),
                          f_path.c_str (),
                          hostname.c_str () ,
                          port.c_str ()));
}

void GopherEngine::infoFile (const string &fname, const string &path,GopherMenu &tmp)
{
  string file_path;
  file_path.append (abs_path);
  file_path.append ("/");
  file_path.append (path);
  file_path.append ("/");
  file_path.append (fname);

  File f;
  int state;
  u_long fsize;
  state = f.openFile (file_path.c_str (), File::READ);
  if(!state)
    {
      MemBuf buff;
      u_long real;
      int sts;
      fsize = f.getFileSize ();
      buff.setLength (fsize);
      f.read (buff.getBuffer (), fsize, &real);
      if(real == fsize)
        tmp.addIntro (buff.getBuffer (), hostname, port);
      f.close ();
    }
}
