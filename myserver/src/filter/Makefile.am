AM_CXXFLAGS="-I$(top_srcdir)/lib"
lib_LIBRARIES = libfilter.a
libfilter_a_SOURCES = filter.cpp filters_chain.cpp filters_factory.cpp memory_stream.cpp stream.cpp \
			console.cpp
SUBDIRS = gzip
AM_CPPFLAGS = $(all_includes)

install:
# Do not do anything.
