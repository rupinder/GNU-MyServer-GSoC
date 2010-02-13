AM_CXXFLAGS="-I$(top_srcdir)/lib"
lib_LIBRARIES = libprotocol.a
libprotocol_a_SOURCES = protocol_buffer.cpp protocol.cpp protocols_manager.cpp \
			url.cpp
SUBDIRS = control ftp gopher http https
AM_CPPFLAGS = $(all_includes)

install:
# Do not do anything.
