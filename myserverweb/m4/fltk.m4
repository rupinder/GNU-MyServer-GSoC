# Config paths for fltk
# Writen for MyServer
# based on glib.m4

dnl AM_PATH_FLTK([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])

AC_DEFUN(AM_PATH_FLTK,
[
AC_ARG_WITH(fltk-prefix,[  --with-fltk-prefix=PFX   Prefix where FLTK is installed (optional)],
            fltk_config_prefix="$withval", fltk_config_prefix="")
AC_ARG_WITH(fltk-exec-prefix,[  --with-fltk-exec-prefix=PFX Exec prefix where FLTK is installed (optional)], fltk_config_exec_prefix="$withval", fltk_config_exec_prefix="")

  if test x$fltk_config_exec_prefix != x ; then
     fltk_config_args="$fltk_config_args"
     if test x${FLTK_CONFIG+set} != xset ; then
        FLTK_CONFIG=$fltk_config_exec_prefix/bin/fltk-config
     fi
  fi
  if test x$fltk_config_prefix != x ; then
     fltk_config_args="$fltk_config_args"
     if test x${FLTK_CONFIG+set} != xset ; then
        FLTK_CONFIG=$fltk_config_prefix/bin/fltk-config
     fi
  fi

  AC_PATH_PROG(FLTK_CONFIG, fltk-config, no)
  min_fltk_version=ifelse([$1], ,1.1.0,$1)
  AC_MSG_CHECKING(for FLTK - version >= $min_fltk_version)
  no_fltk=""
  if test "$FLTK_CONFIG" = "no" ; then
    no_fltk=yes
    FLTK_VERSION=""
  else
    FLTK_CFLAGS=`$FLTK_CONFIG $fltk_config_args --cflags`
    FLTK_LIBS=`$FLTK_CONFIG $fltk_config_args --ldflags`
    FLTK_VERSION=`$FLTK_CONFIG $fltk_config_args --version`
  fi
  if test "x$no_fltk" = x ; then
     AC_MSG_RESULT(yes (version $FLTK_VERSION))
     ifelse([$2], , :, [$2])
  else
     if test "x$FLTK_VERSION" = x; then
        AC_MSG_RESULT(no)
     else
        AC_MSG_RESULT(no (version $FLTK_VERSION))
     fi	
     FLTK_CFLAGS=""
     FLTK_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(FLTK_CFLAGS)
  AC_SUBST(FLTK_LIBS)
])  
