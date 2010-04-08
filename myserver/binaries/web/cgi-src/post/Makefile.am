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
# Makefile for post

AM_CXXFLAGS="-I$(top_srcdir)/lib"
lib_LTLIBRARIES = libpost.la
libpost_la_SOURCES = post.cpp
libpost_la_LDFLAGS = -avoid-version -module -shared
libpost_la_LIBADD = ../../../../src/libmyserver.la ../../../../lib/libgnu.la
SO_EXT=`$(LIBTOOL) --config | $(SED) -e"y/'/\"/" -e '/^shrext_cmds/ !d; s/^.*="\(.*\)"/\1/'`

all: all-am post.mscgi

post.mscgi: libpost.la
	cp .libs/libpost$(SO_EXT) ../../cgi-bin/post.mscgi

clean-local:
	test \! -e libpost.la || \
	$(LIBTOOL) $(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=uninstall libpost.la


.phony : post.mscgi
