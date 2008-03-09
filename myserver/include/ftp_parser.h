/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     NUMBER_FTP_ARG = 258,
     STRING_FTP_ARG = 259,
     CHAR_FTP_ARG = 260,
     CRLF_FTP_SEP = 261,
     SPACE_FTP_SEP = 262,
     PUNCT_FTP_SEP = 263,
     USER_FTP_COMMAND = 264,
     PASS_FTP_COMMAND = 265,
     QUIT_FTP_COMMAND = 266,
     PORT_FTP_COMMAND = 267,
     PASV_FTP_COMMAND = 268,
     MODE_FTP_COMMAND = 269,
     TYPE_FTP_COMMAND = 270,
     STRU_FTP_COMMAND = 271,
     REST_FTP_COMMAND = 272,
     RETR_FTP_COMMAND = 273,
     LIST_FTP_COMMAND = 274,
     NLST_FTP_COMMAND = 275,
     ABOR_FTP_COMMAND = 276,
     CWD_FTP_COMMAND = 277,
     CDUP_FTP_COMMAND = 278,
     PWD_FTP_COMMAND = 279,
     HELP_FTP_COMMAND = 280,
     SYST_FTP_COMMAND = 281,
     STAT_FTP_COMMAND = 282,
     NOOP_FTP_COMMAND = 283
   };
#endif
/* Tokens.  */
#define NUMBER_FTP_ARG 258
#define STRING_FTP_ARG 259
#define CHAR_FTP_ARG 260
#define CRLF_FTP_SEP 261
#define SPACE_FTP_SEP 262
#define PUNCT_FTP_SEP 263
#define USER_FTP_COMMAND 264
#define PASS_FTP_COMMAND 265
#define QUIT_FTP_COMMAND 266
#define PORT_FTP_COMMAND 267
#define PASV_FTP_COMMAND 268
#define MODE_FTP_COMMAND 269
#define TYPE_FTP_COMMAND 270
#define STRU_FTP_COMMAND 271
#define REST_FTP_COMMAND 272
#define RETR_FTP_COMMAND 273
#define LIST_FTP_COMMAND 274
#define NLST_FTP_COMMAND 275
#define ABOR_FTP_COMMAND 276
#define CWD_FTP_COMMAND 277
#define CDUP_FTP_COMMAND 278
#define PWD_FTP_COMMAND 279
#define HELP_FTP_COMMAND 280
#define SYST_FTP_COMMAND 281
#define STAT_FTP_COMMAND 282
#define NOOP_FTP_COMMAND 283




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 15 "ftp_parser.ypp"
{
	int	m_nInt;
	char	m_nChar;
	char	*m_szStr;
	FtpHost m_host;
}
/* Line 1489 of yacc.c.  */
#line 112 "../include/ftp_parser.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


