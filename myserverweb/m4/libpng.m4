# Config paths for libpng
# Writen for MyServer
# based on glib.m4

dnl AM_PATH_LIBPNG([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
AC_DEFUN([AM_PATH_LIBPNG],
[
AC_ARG_WITH(libpng-prefix,[  --with-libpng-prefix=PFX   Prefix where libPNG is installed (optional)],
            libpng_config_prefix="$withval", glib_config_prefix="")
AC_ARG_WITH(libpng-exec-prefix,[  --with-libpng-exec-prefix=PFX Exec prefix where libpng is installed (optional)], libpng_config_exec_prefix="$withval", libpng_config_exec_prefix="")

  if test x$libpng_config_exec_prefix != x ; then
     libpng_config_args="$libpng_config_args --exec-prefix=$libpng_config_exec_prefix"
     if test x${LIBPNG_CONFIG+set} != xset ; then
        LIBPNG_CONFIG=$libpng_config_exec_prefix/bin/libpng-config
     fi
  fi
  if test x$libpng_config_prefix != x ; then
     libpng_config_args="$libpng_config_args --prefix=$libpng_config_prefix"
     if test x${LIBPNG_CONFIG+set} != xset ; then
        LIBPNG_CONFIG=$libpng_config_prefix/bin/libpng-config
     fi
  fi

  AC_PATH_PROG(LIBPNG_CONFIG, libpng-config, no)
  min_libpng_version=ifelse([$1], ,1.2.5,$1)
  AC_MSG_CHECKING(for libPNG - version >= $min_libpng_version)
  no_libpng=""
  if test "$LIBPNG_CONFIG" = "no" ; then
    no_libpng=yes
    LIBPNG_VERSION=""
  else
    LIBPNG_CFLAGS=`$LIBPNG_CONFIG $libpng_config_args --cflags`
    LIBPNG_LIBS=`$LIBPNG_CONFIG $libpng_config_args --libs`
    LIBPNG_VERSION=`$LIBPNG_CONFIG $libpng_config_args --version`
  fi
  if test "x$no_libpng" = x ; then
     AC_MSG_RESULT(yes (version $LIBPNG_VERSION))
     ifelse([$2], , :, [$2])
  else
     if test "x$LIBPNG_VERSION" = x; then
        AC_MSG_RESULT(no)
     else
        AC_MSG_RESULT(no (version $LIBPNG_VERSION))
     fi	
     LIBPNG_CFLAGS=""
     LIBPNG_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(LIBPNG_CFLAGS)
  AC_SUBST(LIBPNG_LIBS)
])  