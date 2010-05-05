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
AM_CXXFLAGS=-I"$(top_builddir)/lib" -I"$(top_srcdir)/lib" -I"$(top_srcdir)"
lib_LTLIBRARIES = libftp.la
libftp_la_SOURCES = ftp.cpp ftp_lexer.cpp ftp_parser.cpp
EXTRA_DIST = ftp_lexer.out.cpp  ftp_parser.ypp  ftp_lexer.lpp
SUBDIRS =

build_parser: ftp_lexer.lpp ftp_parser.ypp
	$(YACC) $(YFLAGS) --defines=../../../include/protocol/ftp/ftp_parser.h \
	--output=ftp_parser.cpp ftp_parser.ypp
	$(LEX) --header-file=../../../include/protocol/ftp/ftp_lexer.h \
	--outfile=ftp_lexer.out.cpp ftp_lexer.lpp
