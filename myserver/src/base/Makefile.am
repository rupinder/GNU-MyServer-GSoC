lib_LIBRARIES = libbase.a
libbase_a_SOURCES = utility.cpp
SUBDIRS = base64 bitvec dynamic_lib file files_cache read_directory hash_map \
	  home_dir md5 mem_buff multicast pipe process regex safetime slab \
	  socket socket_pair ssl string sync thread xml unix_socket
AM_CPPFLAGS = $(all_includes) -I$(top_builddir)/lib -I$(top_srcdir)/lib
install:
# Do not do anything.
