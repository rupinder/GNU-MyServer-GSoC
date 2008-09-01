# Makefile for MyServer (./src)
SUBDIRS = base conf connection connections_scheduler env files_cache \
					filter find_data http_handler log plugin protocol server


# STATIC_AR = find_data/libfind_data.a \
# 								 		connection/libconnection.a env/libenv.a\
# 								 		log/liblog.a conf/mime/libmime.a\
# 										conf/security/libsecurity.a conf/vhost/libvhost.a\
# 										conf/libconf.a\
# 										connections_scheduler/libconnections_scheduler.a\
# 										http_handler/wincgi/libwincgi.a\
# 										http_handler/cgi/libcgi.a\
# 										http_handler/mscgi/libmscgi.a\
# 										http_handler/fastcgi/libfastcgi.a\
# 										http_handler/scgi/libscgi.a\
# 										http_handler/isapi/libisapi.a\
# 										http_handler/libhttp_handler.a base/file/libfile.a\
# 										base/pipe/libpipe.a base/base64/libbase64.a\
# 										base/mem_buff/libmem_buff.a base/libbase.a\
# 										base/home_dir/libhome_dir.a\
# 										base/multicast/libmulticast.a base/sync/libsync.a\
# 										base/safetime/libsafetime.a\
# 										base/socket/libsocket.a\
# 										base/hash_map/libhash_map.a\
# 										base/dynamic_lib/libdynamic_lib.a\
# 										base/process/libprocess.a base/md5/libmd5.a\
# 										base/ssl/libssl.a base/xml/libxml.a\
# 										base/string/libstring.a base/regex/libregex.a\
# 										base/thread/libthread.a protocol/ftp/libftp.a\
# 										protocol/control/libcontrol.a\
# 										protocol/libprotocol.a protocol/https/libhttps.a\
# 										protocol/http/libhttp.a\
# 										files_cache/libfiles_cache.a filter/gzip/libgzip.a\
# 										filter/libfilter.a plugin/executor/libexecutor.a\
# 										plugin/http_manager/libhttp_manager.a\
# 										plugin/libplugin.a plugin/protocol/libprotocol.a\
# 										plugin/filter/libfilter.a\
# 										plugin/http_command/libhttp_command.a\
# 										server/libserver.a

build_subdirs:
	list='$(SUBDIRS)'; for subdir in $$list; do \
  (cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) all) \
	done;

libmyserver.a: build_subdirs
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

all : myserver
	cp myserver ../binaries
