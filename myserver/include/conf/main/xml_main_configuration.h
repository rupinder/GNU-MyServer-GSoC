/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2009 Free Software Foundation, Inc.
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

#ifndef XML_MAIN_CONFIGURATION_H
# define XML_MAIN_CONFIGURATION_H

# include "stdafx.h"
# include <include/conf/main/main_configuration.h>
# include <include/base/xml/xml_parser.h>


class XmlMainConfiguration : public MainConfiguration
{
public:
  XmlMainConfiguration ();
  virtual ~XmlMainConfiguration ();

  virtual int open (const char* filename)
  {
    return xmlParser.open (filename);
  }

  virtual int open (string const &filename)
  {
    return open (filename.c_str());
  };

  virtual char *getValue (const char* field)
  {
    return xmlParser.getValue (field);
  }

  virtual char *getValue (string const &field)
  {
    return getValue (field.c_str());
  };

  virtual int close ()
  {
    return xmlParser.close ();
  }

  virtual xmlDocPtr getDoc ()
  {
    return xmlParser.getDoc ();
  }

private:
  XmlParser xmlParser;
};

#endif
