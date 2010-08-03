%{
/*
 * gen-ops-parser.y - Bison grammar for the "gen-ops" program.
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

#include <config.h>
#include <ctype.h>
#include <stdio.h>
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#elif defined(HAVE_STRINGS_H)
# include <strings.h>
#endif

/*
 * Imports from the lexical analyser.
 */
extern int yylex(void);
extern void yyrestart(FILE *file);
#ifdef YYTEXT_POINTER
extern char *yytext;
#else
extern char yytext[];
#endif

/*
 * The task to execute
 */
#define TASK_GEN_NONE		0
#define TASK_GEN_HEADER		1
#define TASK_GEN_TABLE		2
 
/*
 * Value Flags
 */
#define VALUE_FLAG_EMPTY	0
#define VALUE_FLAG_INT		1
#define VALUE_FLAG_LONG		2
#define VALUE_FLAG_FLOAT32	3
#define VALUE_FLAG_FLOAT64	4
#define VALUE_FLAG_NFLOAT	5
#define VALUE_FLAG_ANY		6
#define VALUE_FLAG_PTR		7

/*
 * Opcode type flags
 */
#define OP_TYPE_BRANCH			0x0001
#define OP_TYPE_CALL			0x0002
#define OP_TYPE_CALL_EXTERNAL		0x0004
#define OP_TYPE_ADDRESS_OF_LABEL	0x0008
#define OP_TYPE_JUMP_TABLE		0x0010
#define OP_TYPE_REG			0x0020

/*
 * Operations
 */
#define	OP_NONE			0x00
#define	OP_ADD			0x01
#define	OP_SUB			0x02
#define	OP_MUL			0x03
#define	OP_DIV			0x04
#define	OP_REM			0x05
#define	OP_NEG			0x06
#define	OP_AND			0x07
#define	OP_OR			0x08
#define	OP_XOR			0x09
#define	OP_NOT			0x0A
#define	OP_EQ			0x0B
#define	OP_NE			0x0C
#define	OP_LT			0x0D
#define	OP_LE			0x0E
#define	OP_GT			0x0F
#define	OP_GE			0x10
#define	OP_SHL			0x11
#define	OP_SHR			0x12
#define	OP_SHR_UN		0x13
#define	OP_COPY			0x14
#define	OP_ADDRESS_OF		0x15

/*
 * Current file and line number.
 */
extern char *genops_filename;
extern long genops_linenum;

struct genops_opcode
{
	struct genops_opcode   *next;
	const char	       *name;
	int			type;
	int			oper;
	int			dest_flags;
	int			input1_flags;
	int			input2_flags;
	const char	       *expression;
};

/*
 * Opcode declaration list header
 */
struct genops_opcode_list
{
	const char *declaration;
	const char *define_start;
	const char *counter_prefix_expr;
	struct genops_opcode *first_opcode;
	struct genops_opcode *last_opcode;
};

/*
 * The task to execute.
 */
static int genops_task = TASK_GEN_NONE;

/*
 * The file to execute
 */
static FILE *genops_file = 0;
static int genops_file_needs_close = 0;

/*
 * Blocks coppied to the resulting file.
 */
static const char *start_code_block = 0;
static const char *start_header_block = 0;
static const char *end_code_block = 0;
static const char *end_header_block = 0;

/*
 * Current opcode declaration header
 */
static struct genops_opcode_list *opcode_header = 0;

/*
 * Report error message.
 */
static void
genops_error_message(char *filename, long linenum, char *msg)
{
	fprintf(stderr, "%s(%ld): %s\n", filename, linenum, msg);
}

/*
 * Report error messages from the parser.
 */
static void
yyerror(char *msg)
{
	genops_error_message(genops_filename, genops_linenum, msg);
}

/*
 * Create an opcode declaration header.
 */
static void
genops_create_opcode_header(const char *define_start, const char *declaration,
			    const char *counter_prefix_expr)
{
	struct genops_opcode_list *header;

	header = (struct genops_opcode_list *)malloc(sizeof(struct genops_opcode_list));
	if(!header)
	{
		exit(1);
	}
	if(counter_prefix_expr && (strlen(counter_prefix_expr) == 0))
	{
		counter_prefix_expr = 0;
	}
	header->declaration = declaration;
	header->define_start = define_start;
	header->counter_prefix_expr = counter_prefix_expr;
	header->first_opcode = 0;
	header->last_opcode = 0;
	opcode_header = header;
}

/*
 * Create an opcode entry
 */
static struct genops_opcode *
genops_add_opcode(const char *name, int type, int oper, int dest_flags, int input1_flags, int input2_flags, const char *expression)
{
	struct genops_opcode *opcode;

	opcode = (struct genops_opcode *) malloc(sizeof(struct genops_opcode));
	if(!opcode)
	{
		exit(1);
	}

	if(opcode_header->first_opcode == 0)
	{
		opcode_header->first_opcode = opcode;
	}
	if(opcode_header->last_opcode != 0)
	{
		opcode_header->last_opcode->next = opcode;
	}
	opcode_header->last_opcode = opcode;

	opcode->next = 0;
	opcode->name = name;
	opcode->type = type;
	opcode->oper = oper;
	opcode->dest_flags = dest_flags;
	opcode->input1_flags= input1_flags;
	opcode->input2_flags= input2_flags;
	opcode->expression = expression;

	return opcode;
}

static int
genops_output_flag(const char *flag, int needs_or)
{
	if(needs_or) printf(" | ");
	printf(flag);
	return 1;
}

static int
genops_output_prefix_flag(const char *prefix, const char *flag, int needs_or)
{
	if(needs_or) printf(" | ");
	printf("%s%s", prefix, flag);
	return 1;
}

static int
genops_output_type(int type, int needs_or)
{

	if(type & OP_TYPE_BRANCH)
	{
		needs_or = genops_output_flag("JIT_OPCODE_IS_BRANCH", needs_or);
	}
	if(type & OP_TYPE_CALL)
	{
		needs_or = genops_output_flag("JIT_OPCODE_IS_CALL", needs_or);
	}
	if(type & OP_TYPE_CALL_EXTERNAL)
	{
		needs_or = genops_output_flag("JIT_OPCODE_IS_CALL_EXTERNAL", needs_or);
	}
	if(type & OP_TYPE_ADDRESS_OF_LABEL)
	{
		needs_or = genops_output_flag("JIT_OPCODE_IS_ADDROF_LABEL", needs_or);
	}
	if(type & OP_TYPE_JUMP_TABLE)
	{
		needs_or = genops_output_flag("JIT_OPCODE_IS_JUMP_TABLE", needs_or);
	}
	if(type & OP_TYPE_REG)
	{
		needs_or = genops_output_flag("JIT_OPCODE_IS_REG", needs_or);
	}

	return needs_or;
}

static int
genops_output_expression(const char *expression, int needs_or)
{
	if(expression && (strlen(expression) > 0))
	{
		needs_or = genops_output_flag(expression, needs_or);
	}
	return needs_or;
}

static int
genops_output_oper(int oper, int needs_or)
{
	switch(oper)
	{
		case OP_ADD:
		{
			return genops_output_flag
				("JIT_OPCODE_OPER_ADD", needs_or);
		}
		break;

		case OP_SUB:
		{
			return genops_output_flag
				("JIT_OPCODE_OPER_SUB", needs_or);
		}
		break;

		case OP_MUL:
		{
			return genops_output_flag
				("JIT_OPCODE_OPER_MUL", needs_or);
		}
		break;

		case OP_DIV:
		{
			return genops_output_flag
				("JIT_OPCODE_OPER_DIV", needs_or);
		}
		break;

		case OP_REM:
		{
			return genops_output_flag
				("JIT_OPCODE_OPER_REM", needs_or);
		}
		break;

		case OP_NEG:
		{
			return genops_output_flag
				("JIT_OPCODE_OPER_NEG", needs_or);
		}
		break;

		case OP_AND:
		{
			return genops_output_flag
				("JIT_OPCODE_OPER_AND", needs_or);
		}
		break;

		case OP_OR:
		{
			return genops_output_flag
				("JIT_OPCODE_OPER_OR", needs_or);
		}
		break;

		case OP_XOR:
		{
			return genops_output_flag
				("JIT_OPCODE_OPER_XOR", needs_or);
		}
		break;

		case OP_NOT:
		{
			return genops_output_flag
				("JIT_OPCODE_OPER_NOT", needs_or);
		}
		break;

		case OP_EQ:
		{
			return genops_output_flag
				("JIT_OPCODE_OPER_EQ", needs_or);
		}
		break;

		case OP_NE:
		{
			return genops_output_flag
				("JIT_OPCODE_OPER_NE", needs_or);
		}
		break;

		case OP_LT:
		{
			return genops_output_flag
				("JIT_OPCODE_OPER_LT", needs_or);
		}
		break;

		case OP_LE:
		{
			return genops_output_flag
				("JIT_OPCODE_OPER_LE", needs_or);
		}
		break;

		case OP_GT:
		{
			return genops_output_flag
				("JIT_OPCODE_OPER_GT", needs_or);
		}
		break;

		case OP_GE:
		{
			return genops_output_flag
				("JIT_OPCODE_OPER_GE", needs_or);
		}
		break;

		case OP_SHL:
		{
			return genops_output_flag
				("JIT_OPCODE_OPER_SHL", needs_or);
		}
		break;

		case OP_SHR:
		{
			return genops_output_flag
				("JIT_OPCODE_OPER_SHR", needs_or);
		}
		break;

		case OP_SHR_UN:
		{
			return genops_output_flag
				("JIT_OPCODE_OPER_SHR_UN", needs_or);
		}
		break;

		case OP_COPY:
		{
			return genops_output_flag
				("JIT_OPCODE_OPER_COPY", needs_or);
		}
		break;

		case OP_ADDRESS_OF:
		{
			return genops_output_flag
				("JIT_OPCODE_OPER_ADDRESS_OF", needs_or);
		}
		break;
	}
	return needs_or;
}

static int
genops_output_value_flags(const char *prefix, int flags, int needs_or)
{
	switch(flags)
	{
		case VALUE_FLAG_EMPTY:
		{
			/* Nothing to do here */
		}
		break;

		case VALUE_FLAG_INT:
		{
			return genops_output_prefix_flag
				(prefix, "_INT", needs_or);
		}
		break;

		case VALUE_FLAG_LONG:
		{
			return genops_output_prefix_flag
				(prefix, "_LONG", needs_or);
		}
		break;

		case VALUE_FLAG_FLOAT32:
		{
			return genops_output_prefix_flag
				(prefix, "_FLOAT32", needs_or);
		}
		break;

		case VALUE_FLAG_FLOAT64:
		{
			return genops_output_prefix_flag
				(prefix, "_FLOAT64", needs_or);
		}
		break;

		case VALUE_FLAG_NFLOAT:
		{
			return genops_output_prefix_flag
				(prefix, "_NFLOAT", needs_or);
		}
		break;

		case VALUE_FLAG_ANY:
		{
			return genops_output_prefix_flag
				(prefix, "_ANY", needs_or);
		}
		break;

		case VALUE_FLAG_PTR:
		{
			return genops_output_prefix_flag
				(prefix, "_PTR", needs_or);
		}
		break;
	}
	return needs_or;
}

/*
 * Create an upper-case copy of a string.
 */
static char *
genops_string_upper(const char *string)
{
	if(string)
	{
		char *cp;
		char *new_string;

		new_string = strdup(string);
		for(cp = new_string; *cp; cp++)
		{
			*cp = toupper(*cp);
		}
		return new_string;
	}
	return 0;
}

%}

/*
 * Define the structure of yylval.
 */
%union {
	int			value;
	char		       *name;
	struct
	{
		const char     *header;
		const char     *code;
	} blocks;
	struct
	{
		const char     *declaration;
		const char     *define_start;
		const char     *counter_prefix_expr;
	} opcode_list_header;
	struct
	{
		const char     *name;
		int		oper;
	} opcode_header;
	struct
	{
		int		dest_flags;
		int		input1_flags;
		int		input2_flags;
	} opcode_values;
	struct
	{
		int		type;
		int		dest_flags;
		int		input1_flags;
		int		input2_flags;
		const char     *expression;
	} opcode_properties;
	struct
	{
		const char     *name;
		int		type;
		int		oper;
		int		dest_flags;
		int		input1_flags;
		int		input2_flags;
		const char     *expression;
	} opcode;
}

/*
 * Primitive lexical tokens and keywords.
 */
%token IDENTIFIER		"an identifier"
%token CODE_BLOCK		"a block copied to the code"
%token HEADER_BLOCK		"a block copied to the header"
%token LITERAL			"literal string"
%token K_EMPTY			"empty"
%token K_ANY			"any"
%token K_INT			"int"
%token K_LONG			"long"
%token K_PTR			"ptr"
%token K_FLOAT32		"float32"
%token K_FLOAT64		"float64"
%token K_NFLOAT			"nfloat"
%token K_NEG			"neg"
%token K_EQ			"=="
%token K_NE			"!="
%token K_LT			"lt"
%token K_LE			"<="
%token K_GT			"gt"
%token K_GE			">="
%token K_SHL			"<<"
%token K_SHR			">>"
%token K_SHR_UN			"shr_un"
%token K_ADDRESS_OF		"address_of"
%token K_ADDRESS_OF_LABEL	"address_of_label"
%token K_BRANCH			"branch"
%token K_CALL			"call"
%token K_CALL_EXTERNAL		"call_external"
%token K_JUMP_TABLE		"jump_table"
%token K_OP_DEF			"op_def"
%token K_OP_TYPE		"op_type"
%token K_OP_VALUES		"op_values"
%token K_OPCODES		"opcodes"
%token K_REG			"reg"

/*
 * Define the yylval types of the various non-terminals.
 */
%type <name>			CODE_BLOCK HEADER_BLOCK
%type <name>			IDENTIFIER LITERAL Literal
%type <blocks>			Blocks
%type <opcode_list_header>	OpcodeListHeader
%type <value>			ValueFlag Op
%type <value>			OpcodeType OpcodeTypeFlag
%type <name>			OpcodeExpression
%type <opcode>			Opcode
%type <opcode_header>		OpcodeHeader
%type <opcode_properties>	OpcodeProperties
%type <opcode_values>		OpcodeValues

%expect 0

%error-verbose

%start Start
%%

Start
	: Blocks OpcodeDeclarations Blocks	{
			start_code_block = ($1).code;
			start_header_block = ($1).header;
			end_code_block = ($3).code;;
			end_header_block = ($3).header;
		}
	;

Blocks
	: /* empty */		{ ($$).header = 0; ($$).code = 0; }
	| CODE_BLOCK		{ ($$).header = 0; ($$).code = $1; }
	| HEADER_BLOCK		{ ($$).header = $1; ($$).code = 0; }
	| CODE_BLOCK HEADER_BLOCK	{
			 ($$).code = $1;
			 ($$).header = $2;
		}
	| HEADER_BLOCK CODE_BLOCK	{
			 ($$).code = $2;
			 ($$).header = $1;
		}
	;

OpcodeDeclarations
	: OpcodeListHeader	{
			genops_create_opcode_header(($1).declaration,
						    ($1).define_start,
						    ($1).counter_prefix_expr);
		}
		'{' Opcodes '}'
	;

OpcodeListHeader
	: K_OPCODES '(' IDENTIFIER ',' Literal ')'	{
			($$).declaration = $3;
			($$).define_start = $5;
			($$).counter_prefix_expr = 0;
		}
	| K_OPCODES '(' IDENTIFIER ',' Literal ',' Literal ')'	{
			($$).declaration = $3;
			($$).define_start = $5;
			($$).counter_prefix_expr = $7;
		}
	;

Opcodes
	: Opcode		{ 
			genops_add_opcode(($1).name, ($1).type,
					  ($1).oper, ($1).dest_flags,
					  ($1).input1_flags,
					  ($1).input2_flags,
					  ($1).expression);
		}
	| Opcodes Opcode	{
			genops_add_opcode(($2).name, ($2).type,
					  ($2).oper, ($2).dest_flags,
					  ($2).input1_flags,
					  ($2).input2_flags,
					  ($2).expression);
		}
	;

Opcode
	: OpcodeHeader '{' '}'	{
			($$).name = ($1).name;
			($$).type = 0;
			($$).oper = ($1).oper;
			($$).dest_flags = VALUE_FLAG_EMPTY;
			($$).input1_flags = VALUE_FLAG_EMPTY;
			($$).input2_flags = VALUE_FLAG_EMPTY;
			($$).expression = 0;
		}
	| OpcodeHeader '{' OpcodeProperties '}'	{
			($$).name = ($1).name;
			($$).type = ($3).type;
			($$).oper = ($1).oper;
			($$).dest_flags = ($3).dest_flags;
			($$).input1_flags = ($3).input1_flags;
			($$).input2_flags = ($3).input2_flags;
			($$).expression = ($3).expression;
		}
	;

OpcodeHeader
	: K_OP_DEF '(' LITERAL ')'	{
			($$).name = $3;
			($$).oper = OP_NONE;
		}
	| K_OP_DEF '(' LITERAL ',' Op ')'	{
			($$).name = $3;
			($$).oper = $5;
		}
	;

OpcodeProperties
	: OpcodeType		{
			($$).type = $1;
			($$).dest_flags = VALUE_FLAG_EMPTY;
			($$).input1_flags = VALUE_FLAG_EMPTY;
			($$).input2_flags = VALUE_FLAG_EMPTY;
			($$).expression = 0;
		}
	| OpcodeValues			{
			($$).type = 0;
			($$).dest_flags = ($1).dest_flags;
			($$).input1_flags = ($1).input1_flags;
			($$).input2_flags = ($1).input2_flags;
			($$).expression = 0;
		}
	| OpcodeExpression		{
			($$).type = 0;
			($$).dest_flags = VALUE_FLAG_EMPTY;
			($$).input1_flags = VALUE_FLAG_EMPTY;
			($$).input2_flags = VALUE_FLAG_EMPTY;
			($$).expression = $1;
		}
	| OpcodeProperties ',' OpcodeType	{
			($$).type = $3;
			($$).dest_flags = ($1).dest_flags;
			($$).input1_flags = ($1).input1_flags;
			($$).input2_flags = ($1).input2_flags;
			($$).expression = ($1).expression;
		}
	| OpcodeProperties ',' OpcodeValues	{
			($$).type = ($1).type;
			($$).dest_flags = ($3).dest_flags;
			($$).input1_flags = ($3).input1_flags;
			($$).input2_flags = ($3).input2_flags;
			($$).expression = ($1).expression;
		}
	| OpcodeProperties ',' OpcodeExpression	{
			($$).type = ($1).type;
			($$).dest_flags = ($1).dest_flags;
			($$).input1_flags = ($1).input1_flags;
			($$).input2_flags = ($1).input2_flags;
			($$).expression = $3;
		}
	;

OpcodeType
	: K_OP_TYPE '(' OpcodeTypeFlag ')'	{ $$ = $3; }
	;

OpcodeTypeFlag
	: K_ADDRESS_OF_LABEL	{ $$ = OP_TYPE_ADDRESS_OF_LABEL; }
	| K_BRANCH		{ $$ = OP_TYPE_BRANCH; }
	| K_CALL		{ $$ = OP_TYPE_CALL; }
	| K_CALL_EXTERNAL	{ $$ = OP_TYPE_CALL_EXTERNAL; }
	| K_JUMP_TABLE		{ $$ = OP_TYPE_JUMP_TABLE; }
	| K_REG			{ $$ = OP_TYPE_REG; }
	;

OpcodeValues
	: K_OP_VALUES '(' ValueFlag ')'	{
			($$).dest_flags = $3;
			($$).input1_flags = VALUE_FLAG_EMPTY;
			($$).input2_flags = VALUE_FLAG_EMPTY;
		}
	|  K_OP_VALUES '(' ValueFlag ',' ValueFlag ')'	{
			($$).dest_flags = $3;
			($$).input1_flags = $5;
			($$).input2_flags = VALUE_FLAG_EMPTY;
		}
	|  K_OP_VALUES '(' ValueFlag ',' ValueFlag ',' ValueFlag ')'	{
			($$).dest_flags = $3;
			($$).input1_flags = $5;
			($$).input2_flags = $7;
		}
	;

OpcodeExpression
	: Literal			{ $$ = $1; }
	;

Op
	: /* empty */			{ $$ = OP_NONE; }
	| '+'				{ $$ = OP_ADD; }
	| '-'				{ $$ = OP_SUB; }
	| '*'				{ $$ = OP_MUL; }
	| '/'				{ $$ = OP_DIV; }
	| '%'				{ $$ = OP_REM; }
	| K_NEG				{ $$ = OP_NEG; }
	| '&'				{ $$ = OP_AND; }
	| '|'				{ $$ = OP_OR; }
	| '^'				{ $$ = OP_XOR; }
	| '~'				{ $$ = OP_NOT; }
	| K_SHL				{ $$ = OP_SHL; }
	| K_SHR				{ $$ = OP_SHR; }
	| K_SHR_UN			{ $$ = OP_SHR_UN; }
	| K_EQ				{ $$ = OP_EQ; }
	| K_NE				{ $$ = OP_NE; }
	| K_LE				{ $$ = OP_LE; }
	| '<'				{ $$ = OP_LT; }
	| K_GE				{ $$ = OP_GE; }
	| '>'				{ $$ = OP_GT; }
	| '='				{ $$ = OP_COPY; }
	| K_ADDRESS_OF			{ $$ = OP_ADDRESS_OF; }
	;

ValueFlag
	: K_EMPTY			{ $$ = VALUE_FLAG_EMPTY; }
	| K_INT				{ $$ = VALUE_FLAG_INT; }
	| K_LONG			{ $$ = VALUE_FLAG_LONG; }
	| K_PTR				{ $$ = VALUE_FLAG_PTR; }
	| K_FLOAT32			{ $$ = VALUE_FLAG_FLOAT32; }
	| K_FLOAT64			{ $$ = VALUE_FLAG_FLOAT64; }
	| K_NFLOAT			{ $$ = VALUE_FLAG_NFLOAT; }
	| K_ANY				{ $$ = VALUE_FLAG_ANY; }
	;

Literal
	: LITERAL			{ $$ = $1; }
	| Literal LITERAL		{
			char *cp = malloc(strlen($1) + strlen($2) + 1);
			if(!cp)
			{
				exit(1);
			}
			strcpy(cp, $1);
			strcat(cp, $2);
			free($1);
			free($2);
			$$ = cp;
		}
	;

%%

/*
 * Parse the commandline arguments
 */
static int
genops_parseargs(int argc, char *argv[])
{
	int current;

	current = 1;
	while(current < argc - 1)
	{
		if(!strcmp(argv[current], "-H") ||
		   !strcmp(argv[current], "--header"))
		{
			genops_task = TASK_GEN_HEADER;
		}
		else if(!strcmp(argv[current], "-T") ||
			!strcmp(argv[current], "--table"))
		{
			genops_task = TASK_GEN_TABLE;
		}
		else if(!strcmp(argv[current], "--help"))
		{
			return 1;
		}
		else
		{
			fprintf(stderr, "Invalid commandline option %s\n",
				argv[current]);
			return 1;
		}
		++current;
	}
	if(genops_task == TASK_GEN_NONE)
	{
		return 1;
	}
	if(argc > 1)
	{
		if(strcmp(argv[argc - 1], "-"))
		{
			genops_file = fopen(argv[argc - 1], "r");
			if(!genops_file)
			{
				perror(argv[argc - 1]);
				return 1;
			}
			genops_file_needs_close = 1;
		}
		else
		{
			genops_file = stdin;
		}
	}
	else
	{
		return 1;
	}
	return 0;
}

/*
 * Output the opcode declaration header
 */
static void
genops_output_defines(const char *define_start, const char *counter_prefix_expr)
{
	struct genops_opcode *current;
	int start_len;
	int opcode_no;
	int name_len;
	int num_tabs;

	start_len = strlen(define_start);
	opcode_no = 0;
	current = opcode_header->first_opcode;
	while(current)
	{
		char *upper_name;

		upper_name = genops_string_upper(current->name);
		if(upper_name == 0)
		{
			/* Out of memory */
			perror(genops_filename);
			exit(1);
		}
		printf("#define\t%s%s", define_start, upper_name);
		name_len = strlen(upper_name) + 8 + start_len;
		num_tabs = 8 - name_len / 8;
		while(num_tabs > 0)
		{
			printf("\t");
			--num_tabs;
		}
		if(counter_prefix_expr)
		{
			printf("(%s + 0x%04X)\n", counter_prefix_expr, opcode_no);
		}
		else
		{
			printf("0x%04X\n", opcode_no);
		}
		free(upper_name);
		current = current->next;
		++opcode_no;
	}
	/*
	 * Print the definition for the number of opcodes
	 */
	printf("#define\t%s%s", define_start, "NUM_OPCODES");
	name_len = 11 + 8 + start_len;
	num_tabs = 8 - name_len / 8;
	while(num_tabs > 0)
	{
		printf("\t");
		--num_tabs;
	}
	printf("0x%04X\n", opcode_no);
}

static void
genops_output_defs(const char *filename)
{
	printf("/%c Automatically generated from %s - DO NOT EDIT %c/\n",
		   '*', filename, '*');
	if(start_header_block)
	{
		printf("%s", start_header_block);
	}
	genops_output_defines(opcode_header->define_start,
			      opcode_header->counter_prefix_expr);
	if(end_header_block)
	{
		printf("%s", end_header_block);
	}
}

/*
 * Output the opcode description table.
 */
static void
genops_output_ops(void)
{
	struct genops_opcode *current;

	printf("%s = {\n", opcode_header->declaration);
	current = opcode_header->first_opcode;
	while(current)
	{
		int needs_or = 0;

		printf("\t{\"%s\"", current->name);
		if(current->type || current->oper || current->dest_flags ||
		   current->input1_flags || current->input2_flags||
		   current->expression)
			printf(", ");
		needs_or = genops_output_type(current->type, 0);
		needs_or = genops_output_oper(current->oper, needs_or);
		needs_or = genops_output_value_flags("JIT_OPCODE_DEST", current->dest_flags, needs_or);
		needs_or = genops_output_value_flags("JIT_OPCODE_SRC1", current->input1_flags, needs_or);
		needs_or = genops_output_value_flags("JIT_OPCODE_SRC2", current->input2_flags, needs_or);
		needs_or = genops_output_expression(current->expression, needs_or);
		if(!needs_or)
		{
			/*
			 * None of the above flags needed to be printed so
			 * we have to add the 0 value here.
			 */
			printf(", 0");
		}
		printf("}");
		current = current->next;
		if(current)
		{
			printf(",\n");
		}
		else
		{
			printf("\n");
		}
	}
	printf("};\n");
}

static void
genops_output_opcode_table(const char *filename)
{
	printf("/%c Automatically generated from %s - DO NOT EDIT %c/\n",
		   '*', filename, '*');
	if(start_code_block)
	{
		printf("%s", start_code_block);
	}
	genops_output_ops();
	if(end_code_block)
	{
		printf("%s", end_code_block);
	}
}

#define USAGE \
"Usage: %s option file\n" \
"Generates an opcode header or table from opcode definitions\n" \
"and writes the result to standard output\n\n" \
"Options:\n" \
"  -H, --header\tgenerate a header file\n" \
"  -T, --table\tgenerate a file with the opcode table\n" \
"  --help\tdisplay this information\n" \
"\nExactly one of header or table option must be supplied.\n" \
"If file is - standard input is used.\n"

int main(int argc, char *argv[])
{
	if(genops_parseargs(argc, argv))
	{
		fprintf(stderr, USAGE, argv[0]);
		return 1;
	}
	genops_filename = argv[argc - 1];
	genops_linenum = 1;
	yyrestart(genops_file);
	if(yyparse())
	{
		if(genops_file_needs_close)
		{
			fclose(genops_file);
		}
		return 1;
	}
	if(genops_file_needs_close)
	{
		fclose(genops_file);
	}
	switch(genops_task)
	{
		case TASK_GEN_HEADER:
		{
			genops_output_defs(genops_filename);
		}
		break;

		case TASK_GEN_TABLE:
		{
			genops_output_opcode_table(genops_filename);
		}
		break;
	}
	return 0;
}
