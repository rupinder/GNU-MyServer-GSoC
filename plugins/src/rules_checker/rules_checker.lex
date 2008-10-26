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
#include "tok.h"
%}

%option never-interactive
%option nomain
%option reentrant
%option nounput
%option yylineno
%option bison-locations
%option bison-bridge
%option batch

character [a-zA-Z0-9\.:\\\/@$£?%?אטלעש]
str  {character}+
id   [a-zA-Z]{character}*


%%
"true" { yylval->int_val = 1; return exp_bin; }
"false" { yylval->int_val = 0; return exp_bin; }
"and"		{ return AND; }
"or"		{ return OR; }
"not"		{ return NOT; }
"("		{ return '('; }
")"		{ return ')'; }
"==="		{ yylval->op_val = allocate_new_str(yytext); return EQ_TRIPLE; }
"=="		{ yylval->op_val = allocate_new_str(yytext); return EQ_DOUBLE; }
"\""  {return '\"';}
{id} 	{ yylval->op_val = allocate_new_str(yytext); return ID; }
{str}	{ yylval->op_val = allocate_new_str(yytext); return STRING; }

[ \t]*		{}
[\n]*		{}

.		{/*TODO: ERROR*/}


%%

int yyparse(void*, void*, int*);


int scan_string(void* context, const char* str, int* val)
{
	int ret;
	YY_BUFFER_STATE state;
	yyscan_t  scanner = NULL;
	yylex_init(&scanner);
	
	state = yy_scan_string (str, scanner);

	ret = yyparse(context, scanner, val);

	yy_delete_buffer(state, scanner);
	yylex_destroy(scanner);

	return ret;
}
