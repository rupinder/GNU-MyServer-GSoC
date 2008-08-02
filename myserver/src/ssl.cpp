/*
MyServer
Copyright (C) 2007, 2008 Free Software Foundation, Inc.
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

#include "../include/ssl.h"
#include "../include/files_utility.h"

/*!
 *SSL password callback function.
 */
static int passwordCb(char *buf,int num,int /*!rwflag*/,void *userdata)
{
  if((size_t)num < strlen((char*)userdata) + 1)
    return 0;

  ((string*)userdata)->assign(buf);

  return ((string*)userdata)->length();
}

SslContext::SslContext()
{
  context = 0;
  method = 0;

  certificateFile.assign("");
  privateKeyFile.assign("");
  password.assign("");
}

/*!
 *Initialize SSL on the virtual host.
 */
int SslContext::initialize()
{
  context = 0;
  method = 0;
#ifndef DO_NOT_USE_SSL
  method = SSLv23_server_method();
  context = SSL_CTX_new(method);

  if(!context)
    return -1;
  /*
   *The specified file doesn't exist.
   */
  if(FilesUtility::fileExists(certificateFile.c_str()) == 0)
  {
    return -1;
  }
  
  if(!(SSL_CTX_use_certificate_chain_file(context, certificateFile.c_str())))
    return -1;

  SSL_CTX_set_default_passwd_cb_userdata(context, &password);

  SSL_CTX_set_default_passwd_cb(context, passwordCb);

  /*
   *The specified file doesn't exist.
   */
  if(FilesUtility::fileExists(privateKeyFile) == 0)
    return -1;

  if(!(SSL_CTX_use_PrivateKey_file(context, privateKeyFile.c_str(), 
                                   SSL_FILETYPE_PEM)))
    return -1;


#if (OPENSSL_VERSION_NUMBER < 0x0090600fL)
  SSL_CTX_set_verify_depth(context, 1);
#endif
  return 1;
#else
  return 1;
#endif
}  

/*!
 *Generate a RSA key and pass it to the SSL context.
 */
void SslContext::generateRsaKey()
{
#ifndef DO_NOT_USE_SSL
  RSA *rsa;

  rsa = RSA_generate_key(2048, RSA_F4, NULL, NULL);

  if (!SSL_CTX_set_tmp_rsa(context, rsa))
    return;

  RSA_free(rsa);
#endif
}


int SslContext::free()
{
#ifndef DO_NOT_USE_SSL
  int ret = 0;
  if(context)
  {
    SSL_CTX_free(context);
    ret = 1;
    context = 0;
  }
  else 
    ret = 0;
  certificateFile.assign("");
  privateKeyFile.assign("");
  return ret;
#else
  return 1;
#endif
}

void initializeSSL()
{
#ifndef DO_NOT_USE_SSL
  static bool initialized = false;
  if(!initialized)
  {
    SSL_load_error_strings();
    SSL_library_init();
    initialized = true;
  }
#endif
}

void cleanupSSL()
{

}
