
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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
     ALLO_FTP_COMMAND = 280,
     STOR_FTP_COMMAND = 281,
     STOU_FTP_COMMAND = 282,
     DELE_FTP_COMMAND = 283,
     APPE_FTP_COMMAND = 284,
     MKD_FTP_COMMAND = 285,
     RMD_FTP_COMMAND = 286,
     RNFR_FTP_COMMAND = 287,
     RNTO_FTP_COMMAND = 288,
     HELP_FTP_COMMAND = 289,
     SYST_FTP_COMMAND = 290,
     STAT_FTP_COMMAND = 291,
     NOOP_FTP_COMMAND = 292,
     SITE_FTP_COMMAND = 293,
     CHMOD_FTP_COMMAND = 294,
     UTIME_FTP_COMMAND = 295,
     SIZE_FTP_COMMAND = 296
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
#define ALLO_FTP_COMMAND 280
#define STOR_FTP_COMMAND 281
#define STOU_FTP_COMMAND 282
#define DELE_FTP_COMMAND 283
#define APPE_FTP_COMMAND 284
#define MKD_FTP_COMMAND 285
#define RMD_FTP_COMMAND 286
#define RNFR_FTP_COMMAND 287
#define RNTO_FTP_COMMAND 288
#define HELP_FTP_COMMAND 289
#define SYST_FTP_COMMAND 290
#define STAT_FTP_COMMAND 291
#define NOOP_FTP_COMMAND 292
#define SITE_FTP_COMMAND 293
#define CHMOD_FTP_COMMAND 294
#define UTIME_FTP_COMMAND 295
#define SIZE_FTP_COMMAND 296




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 16 "ftp_parser.ypp"

  int		m_nInt;
  char		m_nChar;
  char		m_szStr[PARSER_STR_LEN];
  FtpHost 	m_host;



/* Line 1676 of yacc.c  */
#line 143 "../../../include/protocol/ftp/ftp_parser.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
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



