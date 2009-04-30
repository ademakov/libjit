%{
/*
 * gen-rules-scanner.l - Lex input file for the "gen-rules" scanner.
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

#include "gen-rules-parser.h"
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
char *gensel_filename = "";
long gensel_linenum = 1;

/*
 * Return a token code from the lexical analyser.
 */
#define	RETURNTOK(x)		return (x)

/*
 * Forward declarations.
 */
static void gensel_skip_comment(void);
static char *gensel_read_block(void);
static char *gensel_read_literal(void);

/*
 * Duplicate a string.
 */
static char *gensel_strdup(const char *str)
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

"->"			{ RETURNTOK(K_PTR); }
"any"			{ RETURNTOK(K_ANY); }
"all"			{ RETURNTOK(K_ALL); }
"imm"			{ RETURNTOK(K_IMM); }
"immzero"		{ RETURNTOK(K_IMMZERO); }
"imms8"			{ RETURNTOK(K_IMMS8); }
"immu8"			{ RETURNTOK(K_IMMU8); }
"imms16"		{ RETURNTOK(K_IMMS16); }
"immu16"		{ RETURNTOK(K_IMMU16); }
"imms32"		{ RETURNTOK(K_IMMS32); }
"immu32"		{ RETURNTOK(K_IMMU32); }
"local"			{ RETURNTOK(K_LOCAL); }
"frame"			{ RETURNTOK(K_FRAME); }
"ternary"		{ RETURNTOK(K_TERNARY); }
"branch"		{ RETURNTOK(K_BRANCH); }
"note"			{ RETURNTOK(K_NOTE); }
"copy"			{ RETURNTOK(K_COPY); }
"commutative"		{ RETURNTOK(K_COMMUTATIVE); }
"if"			{ RETURNTOK(K_IF); }
"clobber"		{ RETURNTOK(K_CLOBBER); }
"scratch"		{ RETURNTOK(K_SCRATCH); }
"space"			{ RETURNTOK(K_SPACE); }
"stack"			{ RETURNTOK(K_STACK); }
"x87_arith"		{ RETURNTOK(K_X87_ARITH); }
"x87_arith_reversible"	{ RETURNTOK(K_X87_ARITH_REVERSIBLE); }
"%inst_type"		{ RETURNTOK(K_INST_TYPE); }
"%regclass"		{ RETURNTOK(K_REG_CLASS); }
"%lregclass"		{ RETURNTOK(K_LREG_CLASS); }

"manual"		{ RETURNTOK(K_MANUAL); }
"more_space"		{ RETURNTOK(K_MORE_SPACE); }

"!"?{IDALPHA}({DIGIT}|{IDALPHA})*	{
			yylval.name = gensel_strdup(yytext);
			if(!(yylval.name))
			{
				exit(1);
			}
			RETURNTOK(IDENTIFIER);
		}

{WHITE}+		;

\n			{ ++gensel_linenum; }

\"			{ yylval.name = gensel_read_literal();
			  RETURNTOK(LITERAL); }

"{"			{ yylval.code.filename = gensel_filename;
			  yylval.code.linenum = gensel_linenum;
			  yylval.code.block = gensel_read_block();
			  RETURNTOK(CODE_BLOCK); }

"/*"			{ gensel_skip_comment(); }

.			{ RETURNTOK(((int)(yytext[0])) & 0xFF); }

%%

/*
 * Skip a comment in the input stream.
 */
static void gensel_skip_comment(void)
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
				++gensel_linenum;
			}
		}
		else if(ch == '\n')
		{
			++gensel_linenum;
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
static char *gensel_read_block(void)
{
	char *buf = 0;
	int buflen = 0;
	int bufmax = 0;
	int ch;
	int level = 1;
	ADD_CH('{');
	for(;;)
	{
		ch = input();
		if(ch == EOF)
		{
			fprintf(stderr, "Unexpected EOF in code block\n");
			exit(1);
		}
		ADD_CH(ch);
		if(ch == '{')
		{
			++level;
		}
		else if(ch == '\n')
		{
			++gensel_linenum;
		}
		else if(ch == '}')
		{
			--level;
			if(level == 0)
			{
				break;
			}
		}
	}
	return buf;
}

/*
 * Read a literal string from the input stream.
 */
static char *gensel_read_literal()
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
