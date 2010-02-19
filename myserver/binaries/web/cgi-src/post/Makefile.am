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
lib_LIBRARIES = libpost.a
libpost_a_SOURCES = post.cpp

all : all-am post.mscgi
	cp post.mscgi ../../cgi-bin/


post.mscgi : $(libpost_a_OBJECTS) $(libpost_a_DEPENDENCIES)
	-rm -f post.mscgi
	$(CXX) $(libpost_a_OBJECTS) ../../../../src/libmyserver.a  ../../../../lib/libgnu.a \
	 $(libpost_a_LIBADD) $(PTHREAD_LIB) $(IDN_LIB) $(XNET_LIB) $(DL_LIB) $(ZLIB_LIB)\
	 $(XML_LIBS) $(INTLLIBS) $(LDFLAGS) -o post.mscgi -shared $(CXXFLAGS) $(XML_CFLAGS)

install : post.mscgi
	$(INSTALL) -D 'post.mscgi' '$(DESTDIR)$(prefix)/var/web/cgi-bin/post.mscgi'


clean-local:
	rm -f post.mscgi
	rm -f *.o
