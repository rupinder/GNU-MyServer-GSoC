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
lib_LIBRARIES = libfilter.a
libfilter_a_SOURCES = filter.cpp filters_chain.cpp filters_factory.cpp memory_stream.cpp stream.cpp \
			console.cpp
SUBDIRS = gzip
AM_CPPFLAGS = $(all_includes)

install:
# Do not do anything.
