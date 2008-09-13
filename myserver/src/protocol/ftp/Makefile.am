lib_LIBRARIES = libftp.a
libftp_a_SOURCES = ftp.cpp ftp_lexer.cpp ftp_parser.cpp
SUBDIRS =
INCLUDES = $(all_includes)

build_parser: ftp_lexer.lpp ftp_parser.ypp
	bison --defines=../../../include/protocol/ftp/ftp_parser.h \
	--output=ftp_parser.cpp ftp_parser.ypp
	flex --header-file=../../../include/protocol/ftp/ftp_lexer.h \
	--outfile=ftp_lexer.cpp ftp_lexer.lpp
