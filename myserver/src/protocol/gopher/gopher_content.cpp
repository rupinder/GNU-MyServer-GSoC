/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010 Free
  Software Foundation, Inc.
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

#include <include/protocol/gopher/gopher_content.h>
#include <include/base/string/stringutils.h>


#include <iostream>
#include <sstream>
#include <string>


void GopherMenu::addIntro (string freeText, string host, string port)
{
  size_t size = freeText.size ();
  char myline[size];
  vector<GopherItem> tmp;
  istringstream str (freeText);
  while (!str.getline (myline, size).eof ())
    {
      tmp.push_back (GopherInfo (myline,
                                 "(None)",
                                 host.c_str (),
                                 port.c_str ()));
    }
  tmp.push_back (GopherInfo (myline,
                             "(None)",
                             host.c_str (),
                             port.c_str ()));
  Items.insert (Items.begin (),tmp.begin (), tmp.end ());
}

void GopherMenu::addItem (GopherItem i)
{
  Items.push_back (i);
}

void GopherMenu::toProtocol (Socket *s)
{
  for (u_long i = 0; i < Items.size (); i++)
    Items.at (i).toProtocol (s);

  s->send (".\n", 3, 0);
}

GopherItem::GopherItem (char t,
      const char d[],
      const char loc[],
      const char h[],
      const char p[])
{
  init (t, d, loc, h, p);
}

GopherItem::GopherItem ()
{
  init(0, "", "", "", "");
}

void GopherItem::init (char t,
           const char d[],
           const char loc[],
           const char h[],
           const char p[])
{
  Type (t);
  Desc (d);
  Rlocation (loc);
  Host (h);
  Port (p);
}

GopherItem::~GopherItem ()
{}

void GopherItem::toProtocol (Socket *s)
{
  string res;
  res = "-";
  res[0]=type;
  res.append (desc);
  res.append ("\t");
  res.append (rlocation);
  res.append ("\t");
  res.append (host);
  res.append ("\t");
  res.append (port);
  res.append ("\t\n");
  s->send (res.c_str (), res.length (), 0);
}

GopherInfo::GopherInfo ()
{
  init ('i', "", "", "", "");
}

GopherInfo::GopherInfo (const char d[],
      const char loc[],
      const char h[],
      const char p[])
{
  init ('i', d, loc, h, p);
}

GopherInfo::~GopherInfo (){}

GopherFile::GopherFile ()
{
  init ('0', "", "", "", "");
}

GopherFile::GopherFile (const char d[],
            const char loc[],
      const char h[],
      const char p[])
{
  init ('0', d, loc, h, p);
}

GopherFile::~GopherFile (){}



GopherDirectory::GopherDirectory ()
{
  init ('1', "", "", "", "");
}

GopherDirectory::GopherDirectory (const char d[],
          const char loc[],
          const char h[],
          const char p[])
{
  init ('1', d, loc, h, p);
}

GopherDirectory::~GopherDirectory ()
{}

GopherCSO::GopherCSO ()
{
  init ('2', "", "", "", "");
}

GopherCSO::~GopherCSO ()
{}

GopherCSO::GopherCSO (const char d[],
                      const char loc[],
                      const char h[],
                      const char p[])
{
  init ('2', d, loc, h, p);
}

GopherError::GopherError ()
{
  init ('3', "", "", "", "");
}

GopherError::~GopherError ()
{}

GopherError::GopherError (const char d[],
                          const char loc[],
                          const char h[],
                          const char p[])
{
  init ('3', d, loc, h, p);
}

GopherMacBinHex::GopherMacBinHex ()
{
  init ('4', "", "", "", "");
}

GopherMacBinHex::~GopherMacBinHex ()
{}

GopherMacBinHex::GopherMacBinHex (const char d[],
                                  const char loc[],
                                  const char h[],
                                  const char p[])
{
  init ('4', d, loc, h, p);
}

GopherDosBin::GopherDosBin ()
{
  init('5', "", "", "", "");
}

GopherDosBin::~GopherDosBin ()
{}

GopherDosBin::GopherDosBin (const char d[],
                            const char loc[],
                            const char h[],
                            const char p[])
{
  init ('5', d, loc, h, p);
}

GopherUnixUUencode::GopherUnixUUencode ()
{
  init ('6', "", "", "", "");
}

GopherUnixUUencode::~GopherUnixUUencode ()
{}

GopherUnixUUencode::GopherUnixUUencode (const char d[],
                                        const char loc[],
                                        const char h[],
                                        const char p[])
{
  init ('6', d, loc, h, p);
}

GopherSearchServer::GopherSearchServer ()
{
  init ('7', "", "", "", "");
}

GopherSearchServer::~GopherSearchServer ()
{}

GopherSearchServer::GopherSearchServer (const char d[],
                                        const char loc[],
                                        const char h[],
                                        const char p[])
{
  init ('7', d, loc, h, p);
}

GopherTelnet::GopherTelnet ()
{
  init ('8', "", "", "", "");
}

GopherTelnet::~GopherTelnet ()
{}

GopherTelnet::GopherTelnet (const char d[],
                            const char loc[],
                            const char h[],
                            const char p[])
{
  init ('8', d, loc, h, p);
}

GopherBin::GopherBin ()
{
  init ('9', "", "", "", "");
}

GopherBin::~GopherBin ()
{}

GopherBin::GopherBin (const char d[],
                      const char loc[],
                      const char h[],
                      const char p[])
{
  init ('9', d, loc, h, p);
}

GopherRendundantServer::GopherRendundantServer ()
{
  init (0, "", "", "", "");
}

GopherRendundantServer::~GopherRendundantServer ()
{}

GopherRendundantServer::GopherRendundantServer (const char d[],
                                                const char loc[],
                                                const char h[],
                                                const char p[])
{
  init (0, d, loc, h, p);
}

GopherGif::GopherGif ()
{
  init ('g', "", "", "", "");
}

GopherGif::~GopherGif ()
{}

GopherGif::GopherGif (const char d[],
                      const char loc[],
                      const char h[],
                      const char p[])
{
  init ('g', d, loc, h, p);
}

GopherImage::GopherImage ()
{
  init ('g', "", "", "", "");
}

GopherImage::~GopherImage ()
{}

GopherImage::GopherImage (const char d[],
                          const char loc[],
                          const char h[],
                          const char p[])
{
  init ('I', d, loc, h, p);
}


GopherData::GopherData (string data)
{
  buff = data;
}

void GopherData::toProtocol (Socket *s)
{
  string res = buff;
  res.append ("\r\n.\r\n");
  s->send (res.c_str (), res.length (), 0);
}


GopherFileContent::GopherFileContent (string fnome)
{
  sfile = new File ();
  sfile->openFile (fnome.c_str (), File::READ);
  buf.setLength (4096);
}

GopherFileContent::~GopherFileContent ()
{
  sfile->close ();
  delete sfile;
}

void GopherFileContent::toProtocol (Socket *s)
{
  u_long tmp;
  sfile->fastCopyToSocket (s, 0, &buf, &tmp);
}
