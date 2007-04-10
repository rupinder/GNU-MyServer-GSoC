# Makefile : Builds myserver, control, cgi-lib, math_sum, counter, and post
#

if BUILD_CONTROL
CONTROL = control
endif

if BUILD_MSCGI
CGI_LIB = cgi-lib
endif

am__aclocal_m4_deps = $(top_srcdir)/m4/gnome-pthread-check.m4 \
        $(top_srcdir)/m4/libpng.m4 $(top_srcdir)/m4/libtool.m4 \
        $(top_srcdir)/m4/libxml.m4 $(top_srcdir)/configure.in


SUBDIRS = source $(CONTROL) $(CGI_LIB) binaries documentation 
