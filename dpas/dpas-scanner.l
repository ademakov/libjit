%{
/*
 * dpas-scanner.l - Input file for lex for the Dynamic Pascal token syntax.
 *
 * Copyright (C) 2004  Southern Storm Software, Pty Ltd.
 *
 * This file is part of the libjit library.
 *
 * The libjit library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * The libjit library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the libjit library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "dpas-internal.h"
#include "dpas-parser.h"
#include <config.h>

#ifndef HAVE_UNISTD_H
# define YY_NO_UNISTD_H
#endif

extern YYSTYPE yylval;

/*
 * Current file and line number.
 */
char *dpas_filename = "";
long dpas_linenum = 1;

/*
 * Return a token code from the lexical analyser.
 */
#define	RETURNTOK(x)		return (x)

/*
 * Parse a string value.
 */
static char *dpas_parse_string(const char *text)
{
	int quote = *text++;
	char *str = (char *)jit_malloc(jit_strlen(text));
	char *temp;
	if(!str)
	{
		dpas_out_of_memory();
	}
	temp = str;
	while(*text != '\0')
	{
		if(text[0] == quote && text[1] == quote)
		{
			*temp++ = (char)quote;
			text += 2;
		}
		else if(text[0] == quote)
		{
			break;
		}
		else
		{
			*temp++ = *text++;
		}
	}
	*temp = '\0';
	return str;
}

/*
 * Parse a floating-point value.
 */
static jit_nfloat dpas_parse_float(const char *text)
{
	double value = 0.0;
	sscanf(text, "%lf", &value);
	return (jit_nfloat)value;
}

/*
 * Infer the best type for an integer constant.
 */
static void dpas_infer_type(YYSTYPE *lval)
{
	jit_ulong value = lval->int_const.value;
	if(value <= (jit_ulong)(jit_long)jit_max_int)
	{
		lval->int_const.type = jit_type_int;
	}
	else if(value <= (jit_ulong)jit_max_uint)
	{
		lval->int_const.type = jit_type_uint;
	}
	else if(value <= (jit_ulong)jit_max_long)
	{
		lval->int_const.type = jit_type_long;
	}
	else
	{
		lval->int_const.type = jit_type_ulong;
	}
}

/*
 * Parse a decimal integer value.
 */
static void dpas_parse_decimal(const char *text, YYSTYPE *lval)
{
	jit_ulong value = 0;
	while(*text != '\0')
	{
		value = value * 10 + (jit_ulong)(*text - '0');
		++text;
	}
	lval->int_const.value = value;
	dpas_infer_type(lval);
}

/*
 * Parse a hexadecimal integer value.
 */
static void dpas_parse_hex(const char *text, YYSTYPE *lval)
{
	jit_ulong value = 0;
	while(*text != '\0')
	{
		if(*text >= '0' && *text <= '9')
		{
			value = value * 16 + (jit_ulong)(*text - '0');
		}
		else if(*text >= 'A' && *text <= 'F')
		{
			value = value * 16 + (jit_ulong)(*text - 'A' + 10);
		}
		else if(*text >= 'a' && *text <= 'f')
		{
			value = value * 16 + (jit_ulong)(*text - 'a' + 10);
		}
		++text;
	}
	lval->int_const.value = value;
	dpas_infer_type(lval);
}

/*
 * Forward declaration.
 */
static void dpas_skip_comment(int star_style);

%}

%option outfile="lex.yy.c"
%option noyywrap
%option nounput
%option case-insensitive

DIGIT			[0-9]
HEX			[0-9A-Fa-f]
IDALPHA			[a-zA-Z_]
EXPONENT		[Ee][+-]?{DIGIT}+
WHITE			[ \t\v\r\f]

%%

"<>"			{ RETURNTOK(K_NE); }
"<="			{ RETURNTOK(K_LE); }
">="			{ RETURNTOK(K_GE); }
":="			{ RETURNTOK(K_ASSIGN); }
".."			{ RETURNTOK(K_DOT_DOT); }
"**"			{ RETURNTOK(K_POW); }

"and"			{ RETURNTOK(K_AND); }
"array"			{ RETURNTOK(K_ARRAY); }
"begin"			{ RETURNTOK(K_BEGIN); }
"case"			{ RETURNTOK(K_CASE); }
"catch"			{ RETURNTOK(K_CATCH); }
"const"			{ RETURNTOK(K_CONST); }
"div"			{ RETURNTOK(K_DIV); }
"do"			{ RETURNTOK(K_DO); }
"downto"		{ RETURNTOK(K_DOWNTO); }
"else"			{ RETURNTOK(K_ELSE); }
"end"			{ RETURNTOK(K_END); }
"exit"			{ RETURNTOK(K_EXIT); }
"fastcall"		{ RETURNTOK(K_FASTCALL); }
"finally"		{ RETURNTOK(K_FINALLY); }
"for"			{ RETURNTOK(K_FOR); }
"forward"		{ RETURNTOK(K_FORWARD); }
"function"		{ RETURNTOK(K_FUNCTION); }
"goto"			{ RETURNTOK(K_GOTO); }
"if"			{ RETURNTOK(K_IF); }
"in"			{ RETURNTOK(K_IN); }
"label"			{ RETURNTOK(K_LABEL); }
"import"		{ RETURNTOK(K_IMPORT); }
"mod"			{ RETURNTOK(K_MOD); }
"module"		{ RETURNTOK(K_MODULE); }
"nil"			{ RETURNTOK(K_NIL); }
"not"			{ RETURNTOK(K_NOT); }
"of"			{ RETURNTOK(K_OF); }
"or"			{ RETURNTOK(K_OR); }
"packed"		{ RETURNTOK(K_PACKED); }
"pow"			{ RETURNTOK(K_POW); }
"procedure"		{ RETURNTOK(K_PROCEDURE); }
"program"		{ RETURNTOK(K_PROGRAM); }
"record"		{ RETURNTOK(K_RECORD); }
"repeat"		{ RETURNTOK(K_REPEAT); }
"set"			{ RETURNTOK(K_SET); }
"shl"			{ RETURNTOK(K_SHL); }
"shr"			{ RETURNTOK(K_SHR); }
"sizeof"		{ RETURNTOK(K_SIZEOF); }
"stdcall"		{ RETURNTOK(K_STDCALL); }
"then"			{ RETURNTOK(K_THEN); }
"throw"			{ RETURNTOK(K_THROW); }
"to"			{ RETURNTOK(K_TO); }
"try"			{ RETURNTOK(K_TRY); }
"type"			{ RETURNTOK(K_TYPE); }
"until"			{ RETURNTOK(K_UNTIL); }
"var"			{ RETURNTOK(K_VAR); }
"va_arg"		{ RETURNTOK(K_VA_ARG); }
"with"			{ RETURNTOK(K_WITH); }
"while"			{ RETURNTOK(K_WHILE); }
"xor"			{ RETURNTOK(K_XOR); }

'(''|[^'])*'		{ yylval.name = dpas_parse_string(yytext);
			  RETURNTOK(STRING_CONSTANT); }

\"(\"\"|[^"])*\"	{ yylval.name = dpas_parse_string(yytext);
			  RETURNTOK(STRING_CONSTANT); }

{IDALPHA}({DIGIT}|{IDALPHA})*	{
			yylval.name = jit_strdup(yytext);
			if(!(yylval.name))
			{
				dpas_out_of_memory();
			}
			RETURNTOK(IDENTIFIER);
		}

{DIGIT}+{EXPONENT}	{ yylval.real_const = dpas_parse_float(yytext);
		  	  RETURNTOK(REAL_CONSTANT); }
{DIGIT}+"."{DIGIT}*{EXPONENT}	{ yylval.real_const = dpas_parse_float(yytext);
		  		  RETURNTOK(REAL_CONSTANT); }
{DIGIT}+"."{DIGIT}+	{ yylval.real_const = dpas_parse_float(yytext);
		  	  RETURNTOK(REAL_CONSTANT); }
{DIGIT}+"."[^.]		{ yylval.real_const = dpas_parse_float(yytext);
			  RETURNTOK(REAL_CONSTANT); }

{DIGIT}{HEX}*[hH]	{ dpas_parse_hex(yytext, &yylval);
			  RETURNTOK(INTEGER_CONSTANT); }

{DIGIT}+		{ dpas_parse_decimal(yytext, &yylval);
			  RETURNTOK(INTEGER_CONSTANT); }

{WHITE}+		;

\n			{ ++dpas_linenum; }

"{"			{ dpas_skip_comment(0); }
"(*"			{ dpas_skip_comment(1); }

.			{ RETURNTOK(((int)(yytext[0])) & 0xFF); }

%%

void dpas_load_file(char *filename, FILE *file)
{
	char *saved_filename;
	long saved_linenum;
	YY_BUFFER_STATE saved_buffer;
	YY_BUFFER_STATE new_buffer;
	extern int yyparse(void);

	/* Save the current state */
	saved_filename = dpas_filename;
	saved_linenum = dpas_linenum;
	saved_buffer = YY_CURRENT_BUFFER;

	/* Create a buffer for the new file */
	new_buffer = yy_create_buffer(file, BUFSIZ);
	if(!new_buffer)
	{
		dpas_out_of_memory();
	}

	/* Switch to the new state */
	dpas_filename = filename;
	dpas_linenum = 1;
	yy_switch_to_buffer(new_buffer);

	/* Call the parser */
	if(yyparse())
	{
		dpas_error_reported = 1;
	}

	/* Bail out if this was the top-most file in the parse process,
	   because flex cannot switch to a NULL buffer */
	if(!saved_buffer)
	{
		return;
	}

	/* Switch back to the original file */
	dpas_filename = saved_filename;
	dpas_linenum = saved_linenum;
	yy_switch_to_buffer(saved_buffer);

	/* Delete the buffer that we used on the file we just parsed */
	yy_delete_buffer(new_buffer);
}

/*
 * Skip a comment in the input stream.
 */
static void dpas_skip_comment(int star_style)
{
	int ch;
	for(;;)
	{
		ch = input();
		if(ch == EOF)
		{
			break;
		}
		else if(ch == '}' && !star_style)
		{
			break;
		}
		else if(ch == '*' && star_style)
		{
			ch = input();
			while(ch == '*')
			{
				ch = input();
			}
			if(ch == EOF || ch == ')')
			{
				break;
			}
			else if(ch == '\n')
			{
				++dpas_linenum;
			}
		}
		else if(ch == '\n')
		{
			++dpas_linenum;
		}
	}
}
