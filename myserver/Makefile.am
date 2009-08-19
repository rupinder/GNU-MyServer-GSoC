if BUILD_TESTS
TESTS_DIR = tests
TESTS = tests/tests_suite$(EXEEXT)
endif

if BUILD_DOC
DOCS = documentation
endif

am__aclocal_m4_deps = $(top_srcdir)/m4/gnome-pthread-check.m4 \
        $(top_srcdir)/m4/libtool.m4 \
        $(top_srcdir)/m4/libxml.m4 $(top_srcdir)/m4/cppunit.m4 \
        $(top_srcdir)/configure.in

SUBDIRS = src include $(CONTROL) $(CGI_LIB) binaries $(DOCS) $(TESTS_DIR)

EXTRA_DIST = stdafx.h stdafx.cpp doxygen SConstruct
