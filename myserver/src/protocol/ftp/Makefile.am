lib_LIBRARIES = libftp.a
libftp_a_SOURCES = ftp.cpp ftp_lexer.cpp ftp_parser.cpp
SUBDIRS =
AM_CPPFLAGS = $(all_includes)

build_parser: ftp_lexer.lpp ftp_parser.ypp
	$(YACC) $(YFLAGS) --defines=../../../include/protocol/ftp/ftp_parser.h \
	--output=ftp_parser.cpp ftp_parser.ypp
	$(LEX) --header-file=../../../include/protocol/ftp/ftp_lexer.h \
	--outfile=ftp_lexer.cpp ftp_lexer.lpp
