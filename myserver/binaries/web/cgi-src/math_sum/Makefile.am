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
lib_LIBRARIES = libmath_sum.a
libmath_sum_a_SOURCES = math_sum.cpp

all : all-am math_sum.mscgi
	cp math_sum.mscgi ../../cgi-bin/


math_sum.mscgi : $(libmath_sum_a_OBJECTS) $(libmath_sum_a_DEPENDENCIES)
	-rm -f math_sum.mscgi
	$(CXX) $(libmath_sum_a_OBJECTS) ../../../../src/libmyserver.a  ../../../../lib/libgnu.a \
	 $(libmath_sum_a_LIBADD) $(PTHREAD_LIB) $(IDN_LIB) $(XNET_LIB) $(DL_LIB) $(ZLIB_LIB)\
	 $(XML_LIBS) $(INTLLIBS) $(LDFLAGS) -o math_sum.mscgi -shared $(CXXFLAGS) $(XML_CFLAGS)

install : math_sum.mscgi
	$(INSTALL) -D 'math_sum.mscgi' '$(DESTDIR)$(prefix)/var/web/cgi-bin/math_sum.mscgi'


clean-local:
	rm -f math_sum.mscgi
	rm -f *.o
