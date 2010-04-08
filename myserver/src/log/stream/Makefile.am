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
lib_LTLIBRARIES = libstream.la
libstream_la_SOURCES = console_stream.cpp file_stream.cpp \
		log_stream.cpp log_stream_factory.cpp \
		log_stream_creator.cpp file_stream_creator.cpp \
		console_stream_creator.cpp socket_stream.cpp \
		socket_stream_creator.cpp
SUBDIRS = 
INCLUDE = $(all_includes)
