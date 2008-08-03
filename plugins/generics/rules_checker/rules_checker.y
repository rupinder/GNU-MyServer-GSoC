/*
MyServer
Copyright (C) 2007 The Free Software Foundation Inc.
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
%{
#include "heading.h"
#include <string.h>

	//#define YYPARSE_PARAM scanner

union YYSTYPE;
struct YYLTYPE;

int yylex(YYSTYPE*, YYLTYPE*, void*);
int yyerror(YYLTYPE*, void*, void*, int*, char *s);

%}

%union
{
  int int_val;
  string*	op_val;
}

%pure-parser
%locations

%lex-param{void *scanner}
%parse-param{void* context}
%parse-param{void* scanner}
%parse-param{int *ret}

%start	input 

%token	<op_val>	STRING
%token	<op_val>	ID
%token	<int_val>	exp_bin
%type	<int_val>	exp
%type	<op_val>	string_val
%type	<op_val>	function
%left EQ_DOUBLE
%left EQ_TRIPLE
%left	AND
%left	OR
%left NOT
%%

input: exp {*ret = $1;};

string_val: '\"' STRING '\"' {$$ = $2;} 
    | function {$$ = $1;}
    |  '\"' ID '\"' {$$ = $2;};

function: ID '(' ')' {$$ = call_function(context, $1);}
     | ID '(' string_val ')' {$$ = call_function(context, $1, $3);}


exp: string_val	{ $$ = ($1)->length(); }
    | string_val EQ_DOUBLE string_val {$$ = !($1)->compare(*$3) }
    | string_val EQ_TRIPLE string_val	{ $$ = !strcasecmp(($1)->c_str(), ($3)->c_str()); }
    | '(' exp ')' {$$ = $2;}
    | NOT exp	{ $$ = !$2; }
		| exp AND exp	{ $$ = $1 & $3; }
		| exp OR exp	{ $$ = $1 | $3; }
    | exp_bin {$$ = $1}
    | exp_bin EQ_DOUBLE exp_bin	{ $$ = ($1 == $3); }
		;

%%

int yyerror(YYLTYPE*, void*, void*, int*, char *s)
{
	return 0;
}
