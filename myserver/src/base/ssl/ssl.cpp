/*
  MyServer
  Copyright (C) 2007, 2008, 2009, 2010 Free Software Foundation, Inc.
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

#include <include/base/ssl/ssl.h>
#include <include/base/file/files_utility.h>
#include <include/base/sync/mutex.h>

#include <string.h>

#include <include/base/exceptions/checked.h>

#if HAVE_PTHREAD
# include <pthread.h>
#endif

#if HAVE_LIBGCRYPT
# include <errno.h>
# ifdef WIN32
#  undef socklen_t
# endif
# include <gcrypt.h>
# if HAVE_PTHREAD
  /* Hack.  */
# undef malloc
# define malloc gnulib::malloc
GCRY_THREAD_OPTION_PTHREAD_IMPL;
# undef malloc
# endif
#endif


SslContext::SslContext ()
{
  context = 0;
  method = 0;

  certificateFile.assign ("");
  privateKeyFile.assign ("");
}

/*!
  Initialize SSL on the virtual host.
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
    The specified file doesn't exist.
   */
  if (FilesUtility::nodeExists (certificateFile.c_str ()) == 0)
    return -1;

  if (SSL_CTX_use_certificate_file (context, certificateFile.c_str (),
                                    SSL_FILETYPE_PEM) != 1)
    return -1;

  /*
    The specified file doesn't exist.
   */
  if (FilesUtility::nodeExists (privateKeyFile) == 0)
    return -1;

  if (SSL_CTX_use_PrivateKey_file (context, privateKeyFile.c_str (),
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

#if !HAVE_LIBGCRYPT || !HAVE_PTHREAD

static int gcry_lock (void **mutex)
{
  return ((Mutex *) *mutex)->lock ();
}

static int gcry_unlock (void **mutex)
{
  return ((Mutex *) *mutex)->unlock ();
}

static int gcry_init (void **mutex)
{
  *mutex = new Mutex ();
  return 0;
}

static int gcry_destroy (void **mutex)
{
  delete (Mutex *) *mutex;
  return 0;
}

static struct gcry_thread_cbs myserver_gcry_cbs =
  {
    GCRY_THREAD_OPTION_USER,
    NULL,
    gcry_init,
    gcry_destroy,
    gcry_lock,
    gcry_unlock
  };

#endif

void initializeSSL ()
{
  static bool initialized = false;

  if (!initialized)
    {
#if HAVE_LIBGCRYPT && HAVE_PTHREAD
      gcry_control (GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);
      gcry_control (GCRYCTL_INITIALIZATION_FINISHED);
#else
      gcry_control (GCRYCTL_SET_THREAD_CBS, &myserver_gcry_cbs);
      gcry_control (GCRYCTL_INITIALIZATION_FINISHED);
#endif
      gnutls_global_init ();
      initialized = true;
    }
}

void cleanupSSL ()
{

}
