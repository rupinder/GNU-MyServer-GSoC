/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2008, 2009, 2010 Free Software Foundation, Inc.
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

#ifndef VALIDATOR_FACTORY_H
# define VALIDATOR_FACTORY_H

# include "myserver.h"
# include <include/base/hash_map/hash_map.h>
# include <string>

using namespace std;

class Validator;

class ValidatorFactory
{
public:
  ValidatorFactory ();
  virtual ~ValidatorFactory ();
  Validator* getValidator (string &name);
  Validator* addValidator (string &name, Validator* validator);
  bool isValidatorPresent (string &name);
private:
  HashMap<string, Validator*> validators;
};
#endif
