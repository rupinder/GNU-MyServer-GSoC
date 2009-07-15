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

#include <include/base/ssl/ssl.h>
#include <include/base/file/files_utility.h>

#include <string.h>

extern "C"
{
#if GCRY_CONTROL
#include <errno.h>
#include <gcrypt.h>
GCRY_THREAD_OPTION_PTHREAD_IMPL;
#endif

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
}

SslContext::SslContext ()
{
  context = 0;
  method = 0;

  certificateFile.assign ("");
  privateKeyFile.assign ("");
}

/*!
 * Initialize SSL on the virtual host.
 */
int SslContext::initialize ()
{
  context = 0;
  method = 0;
  method = SSLv23_server_method ();
  context = SSL_CTX_new (method);

  if (!context)
    return -1;
  /*
   * The specified file doesn't exist.
   */
  if (FilesUtility::fileExists (certificateFile.c_str ()) == 0)
    return -1;

  if (SSL_CTX_use_certificate_file (context, certificateFile.c_str (),
                                    SSL_FILETYPE_PEM) != 1)
    return -1;

  /*
   * The specified file doesn't exist.
   */
  if (FilesUtility::fileExists(privateKeyFile) == 0)
    return -1;

  if (SSL_CTX_use_PrivateKey_file(context, privateKeyFile.c_str(),
                                  SSL_FILETYPE_PEM) != 1)
    return -1;

  return 1;
}  

int SslContext::free ()
{
  int ret = 0;
  if (context)
    {
      SSL_CTX_free (context);
      ret = 1;
      context = 0;
    }
  else 
    ret = 0;
  certificateFile.assign ("");
  privateKeyFile.assign ("");
  return ret;
}

void initializeSSL ()
{
  static bool initialized = false;

  if (!initialized)
  {
#if GCRY_CONTROL
    gcry_control (GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);
#endif
    gnutls_global_init ();

    initialized = true;
  }
}

void cleanupSSL ()
{

}
