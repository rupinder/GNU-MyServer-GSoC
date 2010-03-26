# GNU MyServer
#
# Copyright (C) 2002-2010 Free Software Foundation, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# Makefile for math_sum

AM_CXXFLAGS="-I$(top_srcdir)/lib"
lib_LTLIBRARIES = libmath_sum.la
libmath_sum_la_SOURCES = math_sum.cpp
libmath_sum_la_LDFLAGS = -avoid-version -module -shared
libmath_sum_la_LIBADD = ../../../../src/libmyserver.a ../../../../lib/libgnu.a
SO_EXT=`$(LIBTOOL) --config | $(SED) -e"y/'/\"/" -e '/^shrext_cmds/ !d; s/^.*="\(.*\)"/\1/'`

all : all-am math_sum.mscgi
	cp libmath_sum$(SO_EXT) ../../cgi-bin/math_sum.mscgi

math_sum.mscgi:
	$(LIBTOOL) $(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=install $(INSTALL) \
		$(INSTALL_STRIP_FLAG) libmath_sum.la $(abs_builddir)

clean-local:
	test \! -e libmath_sum.la || \
	$(LIBTOOL) $(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=uninstall libmath_sum.la
