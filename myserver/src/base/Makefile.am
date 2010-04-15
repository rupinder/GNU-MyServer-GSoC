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
lib_LTLIBRARIES = libbase.la
libbase_la_SOURCES = utility.cpp
SUBDIRS = base64 bitvec crypt dynamic_lib file exceptions files_cache \
	  read_directory hash_map home_dir mem_buff multicast pipe process \
          regex safetime slab socket socket_pair ssl string sync thread xml \
	  unix_socket
AM_CPPFLAGS = $(all_includes) -I$(top_builddir)/lib -I$(top_srcdir)/lib

install:
# Do not do anything.
