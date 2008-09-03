# Makefile for MyServer (./src)
SUBDIRS = base conf connection connections_scheduler filter\
					http_handler log plugin protocol server .


#build_subdirs:
#	list='$(SUBDIRS)'; for subdir in $$list; do \
#  (cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) all) \
#	done;

libmyserver.a:
	rm -f libmyserver.a
	find -iregex .*\\.o | xargs ar cru libmyserver.a
	ranlib libmyserver.a

bin_PROGRAMS = myserver
myserver_SOURCES = myserver.cpp

INCLUDES = $(all_includes)

myserver_LDADD =  	$(all_libraries) $(PTHREAD_LIB) $(IDN_LIB) \
										$(XNET_LIB) $(EVENT_LIB) $(DL_LIB) $(OPENSSL_LIB) $(ZLIB_LIB) \
										$(XML_LIBS)

myserver_DEPENDENCIES = libmyserver.a

myserver_CXXFLAGS = $(CXXFLAGS) $(XML_CFLAGS) -DHOST_STR=\"$(build)\" -DPREFIX=\"$(prefix)\"
myserver_LDFLAGS =  -rdynamic libmyserver.a

build_parser: protocol/ftp/ftp_lexer.lpp protocol/ftp/ftp_parser.ypp
	bison --defines=../include/protocol/ftp/ftp_parser.h protocol/ftp/ftp_parser.ypp
	cp protocol/ftp/ftp_parser.tab.cpp protocol/ftp/ftp_parser.cpp
	flex --header-file=../include/protocol/ftp/ftp_lexer.h protocol/ftp/ftp_lexer.lpp
	cp ./protocol/ftp/lex.yy.c ./protocol/ftp/ftp_lexer.cpp

all : myserver${EXEEXT}
	cp myserver${EXEEXT} ../binaries
