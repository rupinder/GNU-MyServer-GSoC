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

AM_CXXFLAGS=-I"$(top_builddir)/lib" -I"$(top_srcdir)/lib" -I"$(top_srcdir)"
lib_LTLIBRARIES = libmath_sum.la
libmath_sum_la_SOURCES = math_sum.cpp
libmath_sum_la_LDFLAGS = -shared -export-dynamic -no-install -no-undefined -module \
										 -avoid-version -shrext .mscgi
libmath_sum_la_LIBADD = ../../../../src/libmyserver.la ../../../../lib/libgnu.la

all: all-am math_sum.mscgi

math_sum.mscgi: libmath_sum.la
	test \! -e .libs/libmath_sum.mscgi \
		|| cp .libs/libmath_sum.mscgi ../../cgi-bin/math_sum.mscgi

clean-generic:
	rm -f  ../../cgi-bin/math_sum.mscgi

.phony : math_sum.mscgi
