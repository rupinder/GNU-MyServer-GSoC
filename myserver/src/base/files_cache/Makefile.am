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
lib_LIBRARIES = libfiles_cache.a
libfiles_cache_a_SOURCES = cached_file_buffer.cpp cached_file.cpp \
			   cached_file_factory.cpp membuf_file.cpp
SUBDIRS =
AM_CPPFLAGS = $(all_includes)

