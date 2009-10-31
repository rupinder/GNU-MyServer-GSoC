/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2007, 2008, 2009 Free Software Foundation, Inc.
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

#ifndef SSL_H
# define SSL_H

# include "stdafx.h"

# include <gnutls/openssl.h>
# include <string>

using namespace std;

class SslContext
{
public:
	SslContext ();

	int initialize ();
	int free ();

	SSL_CTX* getContext (){return context;}
	SSL_METHOD* getMethod (){return method;}

	string& getCertificateFile (){return certificateFile;}
	string& getPrivateKeyFile (){return privateKeyFile;}
	string& getPassword (){return password;}

	void setCertificateFile (string& c){certificateFile.assign (c);}
	void setPrivateKeyFile (string& pk){privateKeyFile.assign (pk);}
	void setPassword (string& p){password.assign (p);}

private:
	SSL_CTX* context;
	SSL_METHOD* method;

	string certificateFile;
	string privateKeyFile;
	string password;
};

void initializeSSL ();
void cleanupSSL ();

#endif
