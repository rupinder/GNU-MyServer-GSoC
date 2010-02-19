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
AM_CXXFLAGS="-I$(top_srcdir)/lib"
SUBDIRS = base conf connection connections_scheduler filter http_handler \
	log plugin protocol server GUI
AM_CPPFLAGS = $(all_includes)
bin_PROGRAMS = myserver
myserver_SOURCES = myserver.cpp
myserver_CXXFLAGS = $(CXXFLAGS) -DHOST_STR=\"$(build)\" -DPREFIX=\"$(prefix)\"
myserver_LDFLAGS =  -rdynamic
myserver_LDADD = libmyserver.a $(PTHREAD_LIB) $(IDN_LIB) \
	$(XNET_LIB) $(DL_LIB) $(ZLIB_LIB) \
	$(XML_LIBS)  ../lib/libgnu.a $(INTLLIBS) $(LDFLAGS)

clean-libmyserver.a:
	rm -f libmyserver.a

libmyserver.a: clean-libmyserver.a
	find . | grep .*\\.$(OBJEXT)$$ | xargs $(AR) cru libmyserver.a
	$(RANLIB) libmyserver.a

all-local: libmyserver.a myserver${EXEEXT}
	cp myserver${EXEEXT} ../binaries

clean-local:
	rm -f libmyserver.a
	find . | grep .*\\.$(OBJEXT)$$ | xargs rm -f
