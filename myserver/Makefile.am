# Makefile : Builds myserver, control, math_sum, and post
#

if BUILD_CONTROL
CONTROL = control
endif

if BUILD_TESTS
TESTS = tests
endif

am__aclocal_m4_deps = $(top_srcdir)/m4/gnome-pthread-check.m4 \
        $(top_srcdir)/m4/fltk.m4 $(top_srcdir)/m4/libtool.m4 \
        $(top_srcdir)/m4/libxml.m4 $(top_srcdir)/m4/cppunit.m4 \
        $(top_srcdir)/configure.in


SUBDIRS = src $(CONTROL) $(CGI_LIB) binaries documentation $(TESTS)
