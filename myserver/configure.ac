dnl GNU MySever configuration script
dnl
dnl GNU MyServer
dnl
dnl Copyright (C) 2002-2010 Free Software Foundation, Inc.
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 3 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.

AC_INIT([GNU MyServer],[m4_esyscmd(cat version)],[bug-myserver@gnu.org])
AC_PREREQ(2.65)
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([config.h])
CFLAGS="${CFLAGS=}"
CXXFLAGS="${CXXFLAGS=}"
LDFLAGS="${LDFLAGS=}"
AC_CANONICAL_HOST
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([gnu])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CONFIG_MACRO_DIR([m4])

AC_PROG_INSTALL
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL

gl_EARLY

AC_ENABLE_SHARED

AC_USE_SYSTEM_EXTENSIONS
AC_SEARCH_LIBS([strerror],[cposix])
AC_USE_SYSTEM_EXTENSIONS
AC_HEADER_STDC
AM_PROG_LEX
AC_PROG_YACC
AC_C_BIGENDIAN
LT_INIT

AC_DEFINE([WINVER], [0x0501], [WINVER])

AC_DEFINE([GNULIB_NAMESPACE], [gnulib], [GNULIB_NAMESPACE])

LT_INIT

AC_CANONICAL_HOST

PACKAGE_STRING="m4_esyscmd([cat version])"
MYSERVER_VERSION="${PACKAGE_STRING}"

AC_SUBST(MYSERVER_VERSION)

AC_DEFINE_UNQUOTED([MYSERVER_VERSION], ["${MYSERVER_VERSION}"],
				       [The software version])

AM_GNU_GETTEXT([external])

AC_CHECK_PROG(make_info, makeinfo, yes, no)
if test x$make_info = xno ; then
   AC_MSG_WARN([ Cannot find makeinfo: info file will not be created])
   MAKE_DOC=no
else
   MAKE_DOC=yes
fi

AC_ARG_ENABLE([ipv6], [--enable-ipv6 Build with IPv6 support],
		      enable_ipv6="yes", enable_ipv6="")

AC_ARG_ENABLE([tests], [--disable-tests Disable the tests suite],
                       enable_tests="$enableval", enable_tests="yes")

AC_ARG_ENABLE([gui], [--enable-gui Build and install GUI application],
		     enable_gui="yes", enable_gui="")


AC_CHECK_HEADER([argp.h], AC_DEFINE(ARGP, 1,
			  [Define if GNU argp tools are available]))
AC_CHECK_HEADER([error.h], AC_DEFINE(ERRORH, 1,
			   [Define if the error.h file is present]))
AC_CHECK_HEADER([grp.h], AC_DEFINE(GRP, 1, [Define if grp.h is available]))
AC_CHECK_HEADER([sys/sendfile.h], AC_DEFINE(SENDFILE, 1,
				  [Define if the sendfile syscall is present]))

gl_INIT

AC_CHECK_FUNCS(regcomp setgroups localtime_r posix_fallocate \
		           openat fstatat getpwnam ffsl readdir_r)

case "${host}" in
    *-*-mingw32*)
     LIBS="$LIBS -lwsock32 -lws2_32 -lgdi32 -lole32  -luuid -luserenv"
     MAKE_MSCGI=yes
     LTCFLAGS=""
     ;;
  *)
     dnl Looking for pthreads
        PTHREAD_LIB="" 
        AC_CHECK_LIB(pthread, pthread_create, PTHREAD_LIB="-lpthread",
               [AC_CHECK_LIB(pthreads, pthread_create, PTHREAD_LIB="-lpthreads",
                   [AC_CHECK_LIB(c_r, pthread_create, PTHREAD_LIB="-lc_r",
                       [AC_CHECK_FUNC(pthread_create)]
                    )]
                )]
        )
     if test x$PTHREAD_LIB = x; then
             AC_MSG_ERROR([
             *** This system appears to have no posix threads
             *** Threads are required for MyServer
             ])
    fi

    AC_SUBST(PTHREAD_LIB)

    dnl Looking for dl
    AC_SEARCH_LIBS([dlopen], [dl c], have_dl=yes)

    if test x$have_dl == xyes; then
        AC_DEFINE(HAVE_DL, 1, [LibDL support, define to 1 to enable])
        MAKE_MSCGI=yes
    else
         AC_MSG_WARN([
                *** dlLIB not found
                *** MSCGI support will not function
                    ])
         DL_LIB=""
   fi


  ;;

esac

AC_SUBST(DL_LIB)

if test x$PTHREAD_LIB != x; then
   AC_DEFINE(HAVE_PTHREAD, 1, [POSIX threads])
fi

dnl See if we need -lxnet for arpa
AC_CHECK_LIB(xnet, accept, have_xnet=yes)
if test x$have_xnet = xyes; then
   XNET_LIB="-lxnet"
else
   XNET_LIB=""
fi
AC_SUBST(XNET_LIB)


AC_SEARCH_LIBS([event_add], [event event_core event_extra],,
                            [AC_MSG_ERROR([*** Cannot find libevent
                            *** libevent is required for MyServer])])
AC_CHECK_HEADER([event.h],,[AC_MSG_ERROR([*** Cannnot find libevent header
                            *** libevent is required for MyServer])])

AC_CHECK_FUNCS(event_loopbreak,
        AC_DEFINE(EVENT_LOOPBREAK, 1,
        [Define if the event_loopbreak function is present]))

# Libidn checks
# -------------
AC_CHECK_HEADER(idna.h,, 
[
   AC_MSG_WARN([idna.h not found which is part of GNU libidn.])
])

AC_ARG_WITH(idn-lib,
  [[  --with-idn-lib=FILE     Use the given path to the idn library.]],
  [
	if test "$withval" != "yes" -a "$withval" != ""; then
		 IDN_LIB=$withval
		 fi
  ])

if test "x$IDN_LIB" = "x"; then
    AC_CHECK_LIB(idn, stringprep_check_version, 	
		have_idn="yes")

		if test "x$have_idn" = "xyes"; then
				IDN_LIB="-lidn"
				AC_DEFINE(HAVE_IDN, 1,
				[Define if the IDN library is present])
		fi
  fi

AC_SUBST(IDN_LIB)

dnl Looking for LibXL2
AM_PATH_XML2(2.6.0, have_xml2=yes)
if test x$have_xml2 != xyes; then
    AC_MSG_ERROR([
*** Cannot find libxml2
*** libxml2 is required
])

else
	CPPFLAGS="$CPPFLAGS $XML_CPPFLAGS"
fi

AC_CHECK_LIB(gpg-error, gpg_err_init)
AC_CHECK_LIB(gcrypt, gcry_control)
AC_CHECK_LIB(gnutls, main)
AC_CHECK_LIB(gnutls-openssl, main)

if test x$ac_cv_lib_gnutls_openssl_main != xyes; then
   AC_MSG_ERROR([Cannot find ssl libraries, gnutls is required])
fi

dnl Looking for zlib
AC_CHECK_LIB(z, uncompress, have_libz=yes)
if test x$have_libz = xyes; then
   AC_DEFINE(HAVE_ZLIB, 1, [Define if the ZLIB library is present])
    ZLIB_LIB="-lz"

    dnl Be sure that it is a good vr
    AC_CHECK_LIB(z, compressBound, have_libz=yes)
    if test x$have_libz != xyes; then
        AC_MSG_WARN([
*** Zlib is too old (http://www.gzip.org/zlib/)
*** compressBound checking will not be enabled
])
    else
        AC_DEFINE(GZIP_COMPRESS_BOUNDS, 1,
	[Memory checking for Zlib, not defined in older vrs])
    fi
else
    AC_MSG_WARN([
*** Cannot find Zlib (http://www.gzip.org/zlib/)
*** gzip file transfer will not be enabled
])
    ZLIB_LIB=""
fi

AC_SUBST(ZLIB_LIB)
AC_SUBST(SSL_LIB)

if test x$enable_ipv6 = xyes; then
    dnl Check for IPv6
    AC_MSG_CHECKING([for IPv6])
    AC_RUN_IFELSE([AC_LANG_SOURCE([[ /* AF_INET6 available check */
    #include <sys/types.h>
    #include <sys/socket.h>
    main()
    {
     if (socket(AF_INET6, SOCK_STREAM, 0) < 0)
       exit(1);
     else
       exit(0);
    }
    ]])],[ AC_MSG_RESULT(yes)
      AC_DEFINE(HAVE_IPV6, 1, [Enable IPv6 support])
    ],[ AC_MSG_RESULT(no)
      AC_DEFINE(HAVE_IPV6, 0, [Disable IPv6 support])
      AC_MSG_ERROR([
*** Cannot enable IPv6 support
])
    ],[ AC_MSG_RESULT(no)
      AC_DEFINE(HAVE_IPV6, 0, [Disable IPv6 support])
      AC_MSG_ERROR([
*** Cannot enable IPv6 support
])
    
    ])
else
    AC_DEFINE(HAVE_IPV6, 0, [Disable IPv6 support])
fi

if test x$enable_tests == xyes; then
  dnl Looking for cppunit
  AM_PATH_CPPUNIT(1.12.0, have_cppunit=yes)
  if test x$have_cppunit != xyes; then
    AC_MSG_WARN([
      *** Cannot find CppUnit (http://cppunit.sourceforge.net/.)
      *** tests driver program will not be built
    ])
    MAKE_TESTS=no
    CPPUNIT_LDFLAGS=""
  else
    MAKE_TESTS=yes
    CPPUNIT_LDFLAGS="-lcppunit"
  fi
else
  MAKE_TESTS=no
fi

if test x$enable_gui == xyes; then
  dnl Checking Python version
  AM_PYTHON(2.6.0, have_python=yes)
  if test x$have_python != xyes; then
    AC_MSG_WARN([
      *** Cannot find matching Python (http://python.org) version
      *** GUI will not be built
    ])
    MAKE_GUI=no
  else
    MAKE_GUI=yes
    AM_PYTHON_MODULE(lxml, have_lxml=yes)
    if test x$have_lxml != xyes; then
      AC_MSG_WARN([
        *** Cannon find lxml Python module (http://codespeak.net/lxml/)
        *** GUI will not be built
      ])
      MAKE_GUI=no
    fi
    AM_PYTHON_MODULE(gtk, have_gtk=yes)
    if test x$have_gtk != xyes; then
      AC_MSG_WARN([
        *** Cannon find PyGTK Python module (http://www.pygtk.org/)
        *** GUI will not be built
      ])
      MAKE_GUI=no
    fi
    AM_PYTHON_MODULE(gobject, have_gobject=yes)
    if test x$have_gobject != xyes; then
      AC_MSG_WARN([
        *** Cannon find PyGObject Python module (http://www.pygtk.org/)
        *** GUI will not be built
      ])
      MAKE_GUI=no
    fi
  fi
else
  MAKE_GUI=no
fi

LDFLAGS="$LDFLAGS $LIBS $pic_flag"
CFLAGS="$pic_flag $CFLAGS"
CXXFLAGS="$pic_flag $CXXFLAGS"

AC_SUBST(CFLAGS)
AC_SUBST(CXXFLAGS)
AC_SUBST(LDFLAGS)
AC_SUBST(CPPUNIT_LDFLAGS)

AC_TYPE_UID_T
AC_TYPE_GETGROUPS

AM_CONDITIONAL(BUILD_TESTS, test "$MAKE_TESTS" = "yes")
AM_CONDITIONAL(BUILD_MSCGI, test "$MAKE_MSCGI" = "yes")
AM_CONDITIONAL(BUILD_DOC, test "$MAKE_DOC" = "yes")
AM_CONDITIONAL(BUILD_GUI, test "$MAKE_GUI" = "yes")

AC_SUBST(BUILD_TESTS)
AC_SUBST(BUILD_DOC)
AC_SUBST(BUILD_MSCGI)

dnl AC_CONFIG_SUBDIRS()
AC_CONFIG_FILES([ po/Makefile.in
    Makefile
    binaries/Makefile
    binaries/certificates/Makefile
    binaries/plugins/Makefile
    binaries/web/Makefile
    binaries/web/cgi-bin/Makefile
    binaries/web/cgi-src/Makefile
    binaries/web/cgi-src/post/Makefile
    binaries/web/cgi-src/math_sum/Makefile
    binaries/logs/Makefile
    binaries/system/Makefile
    binaries/system/css/Makefile
    binaries/system/errors/Makefile
    binaries/system/icons/Makefile
    binaries/system/icons/codes/Makefile
    documentation/Makefile
    include/Makefile
    include/connection/Makefile
    include/log/Makefile
    include/log/stream/Makefile
    include/conf/Makefile
    include/conf/main/Makefile
    include/conf/mime/Makefile
    include/conf/security/Makefile
    include/conf/vhost/Makefile
    include/connections_scheduler/Makefile
    include/http_handler/Makefile
    include/http_handler/wincgi/Makefile
    include/http_handler/cgi/Makefile
    include/http_handler/mscgi/Makefile
    include/http_handler/fastcgi/Makefile
    include/http_handler/http_file/Makefile
    include/http_handler/proxy/Makefile
    include/http_handler/scgi/Makefile
    include/http_handler/isapi/Makefile
    include/http_handler/http_dir/Makefile
    include/base/Makefile
    include/base/bitvec/Makefile
    include/base/read_directory/Makefile
    include/base/file/Makefile
    include/base/pipe/Makefile
    include/base/base64/Makefile
    include/base/mem_buff/Makefile
    include/base/home_dir/Makefile
    include/base/multicast/Makefile
    include/base/sync/Makefile
    include/base/safetime/Makefile
    include/base/slab/Makefile
    include/base/socket/Makefile
    include/base/socket_pair/Makefile
    include/base/hash_map/Makefile
    include/base/dynamic_lib/Makefile
    include/base/process/Makefile
    include/base/crypt/Makefile
    include/base/ssl/Makefile
    include/base/xml/Makefile
    include/base/files_cache/Makefile
    include/base/string/Makefile
    include/base/regex/Makefile
    include/base/thread/Makefile
    include/base/unix_socket/Makefile
    include/protocol/Makefile
    include/protocol/ftp/Makefile
    include/protocol/control/Makefile
    include/protocol/https/Makefile
    include/protocol/http/Makefile
    include/protocol/http/env/Makefile
    include/protocol/gopher/Makefile
    include/filter/Makefile
    include/filter/gzip/Makefile
    include/plugin/Makefile
    include/server/Makefile
    lib/Makefile
    src/Makefile
    src/connection/Makefile
    src/log/Makefile
    src/log/stream/Makefile
    src/conf/Makefile
    src/conf/main/Makefile
    src/conf/mime/Makefile
    src/conf/security/Makefile
    src/conf/vhost/Makefile
    src/connections_scheduler/Makefile
    src/http_handler/Makefile
    src/http_handler/wincgi/Makefile
    src/http_handler/cgi/Makefile
    src/http_handler/mscgi/Makefile
    src/http_handler/fastcgi/Makefile
    src/http_handler/http_file/Makefile
    src/http_handler/scgi/Makefile
    src/http_handler/proxy/Makefile
    src/http_handler/isapi/Makefile
    src/http_handler/http_dir/Makefile
    src/base/Makefile
    src/base/bitvec/Makefile
    src/base/read_directory/Makefile
    src/base/file/Makefile
    src/base/pipe/Makefile
    src/base/base64/Makefile
    src/base/mem_buff/Makefile
    src/base/home_dir/Makefile
    src/base/multicast/Makefile
    src/base/sync/Makefile
    src/base/safetime/Makefile
    src/base/socket/Makefile
    src/base/socket_pair/Makefile
    src/base/hash_map/Makefile
    src/base/dynamic_lib/Makefile
    src/base/process/Makefile
    src/base/crypt/Makefile
    src/base/slab/Makefile
    src/base/ssl/Makefile
    src/base/xml/Makefile
    src/base/files_cache/Makefile
    src/base/string/Makefile
    src/base/regex/Makefile
    src/base/thread/Makefile
    src/base/unix_socket/Makefile
    src/protocol/Makefile
    src/protocol/ftp/Makefile
    src/protocol/control/Makefile
    src/protocol/https/Makefile
    src/protocol/http/Makefile
    src/protocol/http/env/Makefile
    src/protocol/gopher/Makefile
    src/filter/Makefile
    src/filter/gzip/Makefile
    src/plugin/Makefile
    src/server/Makefile
    src/GUI/Makefile
    tests/Makefile
    ])
AC_OUTPUT
