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

SUBDIRS = base conf connection connections_scheduler filter http_handler \
	log plugin protocol server GUI

bin_PROGRAMS = myserver
myserver_SOURCES = myserver.cpp

lib_LTLIBRARIES = libmyserver.la

libmyserver_la_SOURCES =
libmyserver_la_LIBADD =  \
    http_handler/proxy/proxy.lo  \
    http_handler/scgi/scgi.lo  \
    http_handler/http_dir/http_dir.lo  \
    http_handler/wincgi/wincgi.lo  \
    http_handler/isapi/isapi.lo  \
    http_handler/fastcgi/fastcgi.lo  \
    http_handler/mscgi/mscgi_manager.lo  \
    http_handler/mscgi/mscgi.lo  \
    http_handler/http_file/http_file.lo  \
    http_handler/cgi/cgi.lo  \
    log/log_manager.lo  \
    log/stream/log_stream.lo  \
    log/stream/file_stream.lo  \
    log/stream/console_stream.lo  \
    log/stream/log_stream_creator.lo  \
    log/stream/socket_stream_creator.lo  \
    log/stream/console_stream_creator.lo  \
    log/stream/socket_stream.lo  \
    log/stream/file_stream_creator.lo  \
    log/stream/log_stream_factory.lo  \
    connections_scheduler/connections_scheduler.lo  \
    connections_scheduler/listen_threads.lo  \
    server/clients_thread.lo  \
    server/server.lo  \
    base/multicast/multicast.lo  \
    base/home_dir/home_dir.lo  \
    base/ssl/ssl.lo  \
    base/slab/slab.lo  \
    base/unix_socket/unix_socket.lo  \
    base/mem_buff/mem_buff.lo  \
    base/exceptions/checked.lo  \
    base/exceptions/exceptions.lo  \
    base/crypt/sha1.lo  \
    base/crypt/crypt_algo_manager.lo  \
    base/crypt/crypt_algo.lo  \
    base/crypt/md5.lo  \
    base/pipe/pipe.lo  \
    base/string/stringutils.lo  \
    base/string/securestr.lo  \
    base/files_cache/membuf_file.lo  \
    base/files_cache/cached_file_buffer.lo  \
    base/files_cache/cached_file_factory.lo  \
    base/files_cache/cached_file.lo  \
    base/sync/mutex.lo  \
    base/sync/read_write_lock.lo  \
    base/sync/recursive_mutex.lo  \
    base/sync/event.lo  \
    base/sync/semaphore.lo  \
    base/dynamic_lib/dynamiclib.lo  \
    base/regex/myserver_regex.lo  \
    base/socket_pair/socket_pair.lo  \
    base/socket/socket.lo  \
    base/socket/ssl_socket.lo  \
    base/xml/xml_parser.lo  \
    base/read_directory/read_directory.lo  \
    base/file/file.lo  \
    base/file/files_utility.lo  \
    base/bitvec/bitvec.lo  \
    base/safetime/safetime.lo  \
    base/process/process.lo  \
    base/process/fork_server.lo  \
    base/process/process_server_manager.lo  \
    base/base64/mime_utils.lo  \
    base/thread/thread.lo  \
    base/utility.lo  \
    base/hash_map/hash_map.lo  \
    conf/main/xml_main_configuration.lo  \
    conf/main/main_configuration.lo  \
    conf/xml_conf.lo  \
    conf/nodetree.lo  \
    conf/mime/xml_mime_handler.lo  \
    conf/mime/mime_manager.lo  \
    conf/vhost/vhost.lo  \
    conf/vhost/xml_vhost_handler.lo  \
    conf/vhost/ip.lo  \
    conf/vhost/vhost_manager.lo  \
    conf/security/security_cache.lo  \
    conf/security/security_domain.lo  \
    conf/security/validator.lo  \
    conf/security/auth_method_factory.lo  \
    conf/security/security_manager.lo  \
    conf/security/xml_validator.lo  \
    conf/security/auth_method.lo  \
    conf/security/validator_factory.lo  \
    conf/security/auth_domain.lo  \
    conf/security/security_token.lo  \
    protocol/protocols_manager.lo  \
    protocol/control/control_header.lo  \
    protocol/control/control_protocol.lo  \
    protocol/protocol_buffer.lo  \
    protocol/gopher/gopher_content.lo  \
    protocol/gopher/engine.lo  \
    protocol/gopher/gopher.lo  \
    protocol/protocol.lo  \
    protocol/http/http_response.lo  \
    protocol/http/http_headers.lo  \
    protocol/http/dyn_http_command.lo  \
    protocol/http/http_request.lo  \
    protocol/http/http.lo  \
    protocol/http/http_req_security_domain.lo  \
    protocol/http/dyn_http_manager.lo  \
    protocol/http/dyn_http_manager_list.lo  \
    protocol/http/http_thread_context.lo  \
    protocol/http/dyn_http_command_manager.lo  \
    protocol/http/http_errors.lo  \
    protocol/http/env/env.lo  \
    protocol/http/http_header.lo  \
    protocol/http/http_data_handler.lo  \
    protocol/http/http_data_read.lo  \
    protocol/ftp/ftp_lexer.lo  \
    protocol/ftp/ftp.lo  \
    protocol/ftp/ftp_parser.lo  \
    protocol/https/https.lo  \
    protocol/url.lo  \
    connection/connection.lo  \
    plugin/plugin_info.lo  \
    plugin/plugins_manager.lo  \
    plugin/plugin.lo  \
    filter/stream.lo  \
    filter/console.lo  \
    filter/filter.lo  \
    filter/filters_factory.lo  \
    filter/filters_chain.lo  \
    filter/memory_stream.lo  \
    filter/gzip/gzip.lo  \
    filter/gzip/gzip_decompress.lo  \
		../lib/libgnu.la  \
		$(PTHREAD_LIB) $(IDN_LIB) $(XNET_LIB) $(DL_LIB) \
		$(ZLIB_LIB) $(XML_LIBS) \
		$(INTLLIBS) $(LDFLAGS)

myserver_CXXFLAGS = $(AM_CXXFLAGS) $(CXXFLAGS) -DHOST_STR=\"$(build)\" -DPREFIX=\"$(prefix)\"

myserver_LDFLAGS = -static -no-install
myserver_LDADD = libmyserver.la $(PTHREAD_LIB) $(IDN_LIB) \
	$(XNET_LIB) $(DL_LIB) $(ZLIB_LIB) \
	$(XML_LIBS) $(INTLLIBS) $(LDFLAGS)

all-local: libmyserver.la myserver${EXEEXT}
	$(LIBTOOL) --mode=install cp myserver$(EXEEXT) $(abs_top_builddir)/binaries/

clean-local:
	$(LIBTOOL) --mode=uninstall rm -f $(abs_top_builddir)/binaries/myserver$(EXEEXT)
