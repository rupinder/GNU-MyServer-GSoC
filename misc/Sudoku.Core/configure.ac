#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(Sudoku, 0.0.1, alexandru.iancu at gmail.com)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_SRCDIR([src/Program.cs])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

######################
# Checks for programs.

dnl pkg-config
#AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
#if test "x$PKG_CONFIG" = "xno"; then
#	AC_MSG_ERROR([You need to install pkg-config])
#fi

dnl C# compiler
# Microsoft's C# compiler
#AC_PATH_PROG(CSC, csc, no)
# Mono's C# compiler for 1.0 and 2.0(without generics)
AC_PATH_PROG(MCS, mcs, no)
# Mono's C# compiler for 2.0 including generics
AC_PATH_PROG(GCS, gmcs, no)
# Mono's C# compiler for 3.0 including linq
#AC_PATH_PROG(SCS, smcs, no)

AC_PATH_PROG(MONO, mono, no)

CS="C#"
if test "x$CSC" = "xno" -a "x$MCS" = "xno" -a "x$GCS" = "no" -a "x$SCS" = "no" ; then
	dnl AC_MSG_ERROR([You need to install a C# compiler])
	AC_MSG_ERROR([No $CS compiler found])
fi

# try first scs
#if test "x$MCS" = "xno" ; then
#	MCS=$SCS
#fi
#else gcs
if test "x$MCS" = "xno" ; then
	MCS=$GCS
fi
#else MS's csc
if test "x$MCS" = "xno" ; then
	MCS=$CSC
fi

if test "x$MONO" = "xno"; then
	AC_MSG_ERROR([No mono runtime found])
fi

######################
# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_SUBST(MCS)
AC_SUBST(MCSFLAGS)

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
