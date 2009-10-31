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
dnl AM_PYTHON(MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl
AC_DEFUN([AM_PYTHON],
[
  python_version_min=$1

  AC_MSG_CHECKING(for Python - version >= $python_version_min)
  no_python=""
  python_version=`/usr/bin/env python --version 2>&1`
  if test "$python_version" = "" ; then
    AC_MSG_RESULT(no)
    no_python=yes
  else
    python_major_version=$(echo $python_version | \
        sed 's/Python \([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/')
    python_minor_version=$(echo $python_version | \
        sed 's/Python \([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/')
    python_micro_version=$(echo $python_version | \
        sed 's/Python \([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/')

    python_major_min=$(echo $python_version_min | \
        sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/')
    if test "x${python_major_min}" = "x" ; then
      python_major_min=0
    fi
  
    python_minor_min=$(echo $python_version_min | \
        sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/')
    if test "x${python_minor_min}" = "x" ; then
      python_minor_min=0
    fi
  
    python_micro_min=$(echo $python_version_min | \
        sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/')
    if test "x${python_micro_min}" = "x" ; then
      python_micro_min=0
    fi

    python_version_proper=$(expr \
        $python_major_version \> $python_major_min \| \
        $python_major_version \= $python_major_min \& \
        $python_minor_version \> $python_minor_min \| \
        $python_major_version \= $python_major_min \& \
        $python_minor_version \= $python_minor_min \& \
        $python_micro_version \>= $python_micro_min)

    if test "$python_version_proper" = "1" ; then
      AC_MSG_RESULT([$python_major_version.$python_minor_version.$python_micro_version])
    else
      AC_MSG_RESULT(too old)
      no_python=yes
    fi
  fi

  if test "x$no_python" = x ; then
    ifelse([$2], , :, [$2])
  else
    ifelse([$3], , :, [$3])
  fi
])
