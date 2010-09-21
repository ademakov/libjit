%{
/*
 * gen-ops-scanner.l - Lex input file for the "gen-ops" scanner.
 *
 * Copyright (C) 2010  Southern Storm Software, Pty Ltd.
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

#include "gen-ops-parser.h"
#include <config.h>
#include <stdio.h>
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#elif defined(HAVE_STRINGS_H)
# include <strings.h>
#endif

#ifndef HAVE_UNISTD_H
# define YY_NO_UNISTD_H
#endif

extern YYSTYPE yylval;

/*
 * Current file and line number.
 */
char *genops_filename = "";
long genops_linenum = 1;

/*
 * Return a token code from the lexical analyser.
 */
#define	RETURNTOK(x)		return (x)

/*
 * Forward declarations.
 */
static void genops_skip_comment(void);
static char *genops_read_code_block(void);
static char *genops_read_header_block(void);
static char *genops_read_literal(void);

/*
 * Duplicate a string.
 */
static char *genops_strdup(const char *str)
{
	char *new_str;
	if(!str)
	{
		return 0;
	}
	new_str = (char *)malloc(strlen(str) + 1);
	if(!new_str)
	{
		return 0;
	}
	strcpy(new_str, str);
	return new_str;
}

%}

%option outfile="lex.yy.c"
%option noyywrap
%option nounput

DIGIT			[0-9]
IDALPHA			[a-zA-Z_]
WHITE			[ \t\v\r\f]

%%

"empty"			{ RETURNTOK(K_EMPTY); }
"int"			{ RETURNTOK(K_INT); }
"long"			{ RETURNTOK(K_LONG); }
"ptr"			{ RETURNTOK(K_PTR); }
"float32"		{ RETURNTOK(K_FLOAT32); }
"float64"		{ RETURNTOK(K_FLOAT64); }
"nfloat"		{ RETURNTOK(K_NFLOAT); }
"any"			{ RETURNTOK(K_ANY); }
"neg"			{ RETURNTOK(K_NEG); }
"<<"			{ RETURNTOK(K_SHL); }
">>"			{ RETURNTOK(K_SHR); }
"shr_un"		{ RETURNTOK(K_SHR_UN); }
"=="			{ RETURNTOK(K_EQ); }
"!="			{ RETURNTOK(K_NE); }
"<="			{ RETURNTOK(K_LE); }
">="			{ RETURNTOK(K_GE); }
"address_of"		{ RETURNTOK(K_ADDRESS_OF); }
"address_of_label"	{ RETURNTOK(K_ADDRESS_OF_LABEL); }
"branch"		{ RETURNTOK(K_BRANCH); }
"call"			{ RETURNTOK(K_CALL); }
"call_external"		{ RETURNTOK(K_CALL_EXTERNAL); }
"jump_table"		{ RETURNTOK(K_JUMP_TABLE); }
"op_def"		{ RETURNTOK(K_OP_DEF); }
"op_intrinsic"		{ RETURNTOK(K_OP_INTRINSIC); }
"op_type"		{ RETURNTOK(K_OP_TYPE); }
"op_values"		{ RETURNTOK(K_OP_VALUES); }
"opcodes"		{ RETURNTOK(K_OPCODES); }
"reg"			{ RETURNTOK(K_REG); }
"%option"		{ RETURNTOK(K_POPTION); }
"i_i"			{ RETURNTOK(K_i_i); }
"i_ii"			{ RETURNTOK(K_i_ii); }
"i_piii"		{ RETURNTOK(K_i_piii); }
"i_iI"			{ RETURNTOK(K_i_iI); }
"i_II"			{ RETURNTOK(K_i_II); }
"I_I"			{ RETURNTOK(K_I_I); }
"I_II"			{ RETURNTOK(K_I_II); }
"i_pIII"		{ RETURNTOK(K_i_pIII); }
"l_l"			{ RETURNTOK(K_l_l); }
"l_ll"			{ RETURNTOK(K_l_ll); }
"i_plll"		{ RETURNTOK(K_i_plll); }
"i_l"			{ RETURNTOK(K_i_l); }
"i_ll"			{ RETURNTOK(K_i_ll); }
"l_lI"			{ RETURNTOK(K_l_lI); }
"L_L"			{ RETURNTOK(K_L_L); }
"L_LL"			{ RETURNTOK(K_L_LL); }
"i_pLLL"		{ RETURNTOK(K_i_pLLL); }
"i_LL"			{ RETURNTOK(K_i_LL); }
"L_LI"			{ RETURNTOK(K_L_LI); }
"f_f"			{ RETURNTOK(K_f_f); }
"f_ff"			{ RETURNTOK(K_f_ff); }
"i_f"			{ RETURNTOK(K_i_f); }
"i_ff"			{ RETURNTOK(K_i_ff); }
"d_d"			{ RETURNTOK(K_d_d); }
"d_dd"			{ RETURNTOK(K_d_dd); }
"i_d"			{ RETURNTOK(K_i_d); }
"i_dd"			{ RETURNTOK(K_i_dd); }
"D_D"			{ RETURNTOK(K_D_D); }
"D_DD"			{ RETURNTOK(K_D_DD); }
"i_D"			{ RETURNTOK(K_i_D); }
"i_DD"			{ RETURNTOK(K_i_DD); }
"conv"			{ RETURNTOK(K_CONV); }
"conv_ovf"		{ RETURNTOK(K_CONV_OVF); }

"!"?{IDALPHA}({DIGIT}|{IDALPHA})*	{
			yylval.name = genops_strdup(yytext);
			if(!(yylval.name))
			{
				exit(1);
			}
			RETURNTOK(IDENTIFIER);
		}

{WHITE}+		;

\n			{ ++genops_linenum; }

\"			{ yylval.name = genops_read_literal();
			  RETURNTOK(LITERAL); }

"%{"			{ yylval.name = genops_read_code_block();
			  RETURNTOK(CODE_BLOCK); }

"%["			{ yylval.name = genops_read_header_block();
			  RETURNTOK(HEADER_BLOCK); }

"/*"			{ genops_skip_comment(); }

.			{ RETURNTOK(((int)(yytext[0])) & 0xFF); }

%%

/*
 * Skip a comment in the input stream.
 */
static void genops_skip_comment(void)
{
	int ch;
	for(;;)
	{
		ch = input();
		if(ch == EOF)
		{
			break;
		}
		else if(ch == '*')
		{
			ch = input();
			while(ch == '*')
			{
				ch = input();
			}
			if(ch == EOF || ch == '/')
			{
				break;
			}
			else if(ch == '\n')
			{
				++genops_linenum;
			}
		}
		else if(ch == '\n')
		{
			++genops_linenum;
		}
	}
}

/*
 * Add a character to a reallocatable buffer.
 */
#define	ADD_CH(c)	\
		do { \
			if((buflen + 1) >= bufmax) \
			{ \
				buf = (char *)realloc(buf, bufmax + 64); \
				if(!buf) \
				{ \
					exit(1); \
				} \
				bufmax += 64; \
			} \
			buf[buflen++] = (char)c; \
			buf[buflen] = (char)'\0'; \
		} while (0)

/*
 * Read a literal code block from the input stream.
 */
static char *genops_read_code_block(void)
{
	char *buf = 0;
	int buflen = 0;
	int bufmax = 0;
	int ch;

	ch = input();
	for(;;)
	{
		if(ch == EOF)
		{
			fprintf(stderr, "Unexpected EOF in code block\n");
			exit(1);
		}
		if(ch == '\n')
		{
			ADD_CH(ch);
			++genops_linenum;
		}
		else if(ch == '%')
		{
			ch = input();
			if(ch == '}')
			{
				break;
			}
			ADD_CH('%');
			continue;
		}
		else
		{
			ADD_CH(ch);
		}
		ch = input();
	}
	return buf;
}

/*
 * Read a literal header block from the input stream.
 */
static char *genops_read_header_block(void)
{
	char *buf = 0;
	int buflen = 0;
	int bufmax = 0;
	int ch;

	ch = input();
	for(;;)
	{
		if(ch == EOF)
		{
			fprintf(stderr, "Unexpected EOF in code block\n");
			exit(1);
		}
		if(ch == '\n')
		{
			ADD_CH(ch);
			++genops_linenum;
		}
		else if(ch == '%')
		{
			ch = input();
			if(ch == ']')
			{
				break;
			}
			ADD_CH('%');
			continue;
		}
		else
		{
			ADD_CH(ch);
		}
		ch = input();
	}
	return buf;
}

/*
 * Read a literal string from the input stream.
 */
static char *genops_read_literal()
{
	char *buf = 0;
	int buflen = 0;
	int bufmax = 0;
	int escape = 0;
	int ch;
	for(;;)
	{
		ch = input();
		if(ch == EOF)
		{
			fprintf(stderr, "Unexpected EOF in string literal\n");
			exit(1);
		}
		if(ch == '\n')
		{
			fprintf(stderr, "Unexpected newline in string literal\n");
			exit(1);
		}
		if(escape)
		{
			escape = 0;
			if(ch == 'n')
			{
				ch = '\n';
			}
			else if(ch == 't')
			{
				ch = '\t';
			}
		}
		else
		{
			if(ch == '\\')
			{
				escape = 1;
				continue;
			}
			if(ch == '"')
			{
				break;
			}
		}
		ADD_CH(ch);
	}
	return buf;
}
