dnl GNU MyServer
dnl
dnl Copyright (C) 2009 Free Software Foundation, Inc.
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
dnl
dnl AM_PYTHON_MODULE(MODULE_NAME, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl
AC_DEFUN([AM_PYTHON_MODULE],
[
  module_name=$1

  AC_MSG_CHECKING(for Python module: $module_name)
  no_module=""
  python_module=`/usr/bin/env python -c "import $module_name" &> /dev/null && echo "yes"`
  if test "$python_module" = "" ; then
    AC_MSG_RESULT(no)
    no_module=yes
  else
    AC_MSG_RESULT(yes)
  fi

  if test "x$no_module" = x ; then
    ifelse([$2], , :, [$2])
  else
    ifelse([$3], , :, [$3])
  fi
])
