# Makefile : Builds myserver, control, CGI-LIB, math_sum, counter, and post
#

if BUILD_CONTROL
CONTROL = control
endif

if BUILD_MSCGI
CGI_LIB = CGI-LIB
endif

am__aclocal_m4_deps = $(top_srcdir)/m4/fltk.m4 \
        $(top_srcdir)/m4/gnome-pthread-check.m4 \
        $(top_srcdir)/m4/libpng.m4 $(top_srcdir)/m4/libtool.m4 \
        $(top_srcdir)/m4/libxml.m4 $(top_srcdir)/configure.in


SUBDIRS = contrib source $(CONTROL) $(CGI_LIB) binaries Documentation 
