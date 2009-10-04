/*
MyServer
Copyright (C) 2002-2008 Free Software Foundation, Inc.
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


#include <include/conf/security/validator.h>
#include <include/conf/security/validator_factory.h>
#include <include/conf/security/xml_validator.h>

#include <string>

using namespace std;

ValidatorFactory::ValidatorFactory ()
{
  XmlValidator* xmlValidator = new XmlValidator;
  string xml ("xml");

  validators.put (xml, xmlValidator);

}

ValidatorFactory::~ValidatorFactory ()
{
  validators.clear ();
}

/*!
 *Return a validator given its name.
 */
Validator* ValidatorFactory::getValidator (string &name)
{
  return validators.get (name);
}

/*!
 *Add a new validator to the factory.
 *\param name Validator name.
 *\param validator The validator to add.
 *\return The old validator registered with [name], in any.
 */
Validator* ValidatorFactory::addValidator (string &name, Validator* validator)
{
  return validators.put (name, validator);
}

/*!
 *Check if the specified validator is present in the factory.
 *\param name The validator name.
 *\return a bool value to indicate if it is present or not.
 */
bool ValidatorFactory::isValidatorPresent (string &name)
{
  return getValidator (name) != NULL;
}
