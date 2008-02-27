%{
/*
 * gen-sel-parser.y - Bison grammar for the "gen-sel" program.
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

#include <config.h>
#include <stdio.h>
#ifdef HAVE_STRING_H
	#include <string.h>
#elif defined(HAVE_STRINGS_H)
	#include <strings.h>
#endif
#ifdef HAVE_STDLIB_H
	#include <stdlib.h>
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
 * Report error messages from the parser.
 */
static void yyerror(char *msg)
{
	puts(msg);
}

/*
 * Current file and line number.
 */
extern char *gensel_filename;
extern long gensel_linenum;

/*
 * Instruction type for the "inst" variable.
 */
static char *gensel_inst_type = "unsigned char *";
static int gensel_new_inst_type = 0;

/*
 * Amount of space to reserve for the primary instruction output.
 */
static int gensel_reserve_space = 32;
static int gensel_reserve_more_space = 128;

/*
 * First register in a stack arrangement.
 */
static int gensel_first_stack_reg = 8;	/* st0 under x86 */

/*
 * Option values.
 */
#define	GENSEL_OPT_SPILL_BEFORE			0x0001
#define	GENSEL_OPT_BINARY				0x0002
#define	GENSEL_OPT_UNARY				0x0004
#define	GENSEL_OPT_TERNARY				0x0008
#define	GENSEL_OPT_STACK				0x0010
#define	GENSEL_OPT_UNARY_BRANCH			0x0020
#define	GENSEL_OPT_BINARY_BRANCH		0x0040
#define	GENSEL_OPT_ONLY					0x0080
#define	GENSEL_OPT_MANUAL				0x0100
#define	GENSEL_OPT_UNARY_NOTE			0x0200
#define	GENSEL_OPT_BINARY_NOTE			0x0400
#define	GENSEL_OPT_MORE_SPACE			0x0800

/*
 * Pattern values.
 */
#define	GENSEL_PATT_END					0
#define	GENSEL_PATT_REG					1
#define	GENSEL_PATT_LREG				2
#define	GENSEL_PATT_FREG				3
#define	GENSEL_PATT_IMM					4
#define	GENSEL_PATT_IMMZERO				5
#define	GENSEL_PATT_IMMS8				6
#define	GENSEL_PATT_IMMU8				7
#define	GENSEL_PATT_IMMS16				8
#define	GENSEL_PATT_IMMU16				9
#define	GENSEL_PATT_LOCAL				10

/*
 * Information about clauses.
 */
typedef struct gensel_clause *gensel_clause_t;
struct gensel_clause
{
	int				pattern[8];
	char		   *filename;
	long			linenum;
	char		   *code;
	gensel_clause_t	next;
};

/*
 * Free a list of clauses.
 */
static void gensel_free_clauses(gensel_clause_t clauses)
{
	gensel_clause_t next;
	while(clauses != 0)
	{
		next = clauses->next;
		free(clauses->code);
		free(clauses);
		clauses = next;
	}
}

/*
 * Declare the register variables that are needed for a set of clauses.
 */
static void gensel_declare_regs(gensel_clause_t clauses, int options)
{
	int have_reg1 = 0;
	int have_reg2 = 0;
	int have_reg3 = 0;
	int have_imm = 0;
	int have_local = 0;
	int index;
	while(clauses != 0)
	{
		for(index = 0; clauses->pattern[index] != GENSEL_PATT_END; ++index)
		{
			switch(clauses->pattern[index])
			{
				case GENSEL_PATT_REG:
				case GENSEL_PATT_LREG:
				case GENSEL_PATT_FREG:
				{
					if(index == 0)
						have_reg1 = 1;
					else if(index == 1)
						have_reg2 = 1;
					else
						have_reg3 = 1;
				}
				break;

				case GENSEL_PATT_IMMZERO: break;

				case GENSEL_PATT_IMM:
				case GENSEL_PATT_IMMS8:
				case GENSEL_PATT_IMMU8:
				case GENSEL_PATT_IMMS16:
				case GENSEL_PATT_IMMU16:
				{
					have_imm = 1;
				}
				break;

				case GENSEL_PATT_LOCAL:
				{
					have_local = 1;
				}
				break;
			}
		}
		clauses = clauses->next;
	}
	if(have_reg1)
	{
		printf("\tint reg;\n");
	}
	if(have_reg2 && (options & GENSEL_OPT_STACK) == 0)
	{
		printf("\tint reg2;\n");
	}
	if(have_reg3 && (options & GENSEL_OPT_STACK) == 0)
	{
		printf("\tint reg3;\n");
	}
	if(have_imm)
	{
		printf("\tjit_nint imm_value;\n");
	}
	if(have_local)
	{
		printf("\tjit_nint local_offset;\n");
	}
}

/*
 * Output the code within a clause.
 */
static void gensel_output_clause_code(gensel_clause_t clause)
{
	char *code;
	int index;

	/* Output the line number information from the original file */
	printf("#line %ld \"%s\"\n", clause->linenum, clause->filename);

	/* Output the clause code */
	printf("\t\t");
	code = clause->code;
	while(*code != '\0')
	{
		if(*code == '$' && code[1] >= '1' && code[1] <= '9')
		{
			index = code[1] - '1';
			switch(clause->pattern[index])
			{
				case GENSEL_PATT_REG:
				case GENSEL_PATT_LREG:
				case GENSEL_PATT_FREG:
				{
					if(index == 0)
						printf("_jit_reg_info[reg].cpu_reg");
					else if(index == 1)
						printf("_jit_reg_info[reg2].cpu_reg");
					else
						printf("_jit_reg_info[reg3].cpu_reg");
				}
				break;

				case GENSEL_PATT_IMM:
				case GENSEL_PATT_IMMZERO:
				case GENSEL_PATT_IMMS8:
				case GENSEL_PATT_IMMU8:
				case GENSEL_PATT_IMMS16:
				case GENSEL_PATT_IMMU16:
				{
					printf("imm_value");
				}
				break;

				case GENSEL_PATT_LOCAL:
				{
					printf("local_offset");
				}
				break;
			}
			code += 2;
		}
		else if(*code == '%' && code[1] >= '1' && code[1] <= '9')
		{
			index = code[1] - '1';
			switch(clause->pattern[index])
			{
				case GENSEL_PATT_REG:
				case GENSEL_PATT_LREG:
				case GENSEL_PATT_FREG:
				{
					if(index == 0)
						printf("_jit_reg_info[_jit_reg_info[reg].other_reg].cpu_reg");
					else if(index == 1)
						printf("_jit_reg_info[_jit_reg_info[reg2].other_reg].cpu_reg");
					else
						printf("_jit_reg_info[_jit_reg_info[reg3].other_reg].cpu_reg");
				}
				break;
			}
			code += 2;
		}
		else if(*code == '\n')
		{
			putc(*code, stdout);
			putc('\t', stdout);
			++code;
		}
		else
		{
			putc(*code, stdout);
			++code;
		}
	}
	printf("\n");
}

/*
 * Output a single clause for a rule.
 */
static void gensel_output_clause(gensel_clause_t clause, int options)
{
	/* Cache the instruction pointer into "inst" */
	if(gensel_new_inst_type)
	{
		printf("\t\tjit_gen_load_inst_ptr(gen, inst);\n");
	}
	else
	{
		printf("\t\tinst = (%s)(gen->posn.ptr);\n", gensel_inst_type);
		printf("\t\tif(!jit_cache_check_for_n(&(gen->posn), %d))\n",
			   (((options & GENSEL_OPT_MORE_SPACE) == 0)
			   		? gensel_reserve_space : gensel_reserve_more_space));
		printf("\t\t{\n");
		printf("\t\t\tjit_cache_mark_full(&(gen->posn));\n");
		printf("\t\t\treturn;\n");
		printf("\t\t}\n");
	}

	/* Output the clause code */
	gensel_output_clause_code(clause);

	/* Copy "inst" back into the generation context */
	if(gensel_new_inst_type)
	{
		printf("\t\tjit_gen_save_inst_ptr(gen, inst);\n");
	}
	else
	{
		printf("\t\tgen->posn.ptr = (unsigned char *)inst;\n");
	}
}

/*
 * Output the clauses for a rule.
 */
static void gensel_output_clauses(gensel_clause_t clauses, int options)
{
	const char *arg1;
	const char *arg2;
	const char *arg3;
	const char *arg;
	const char *reg;
	const char *flag1;
	const char *flag2;
	const char *flag3;
	const char *flag;
	int destroy1;
	int destroy2;
	int destroy3;
	int destroy;
	gensel_clause_t clause;
	int first, index;
	int check_index;

	/* If the clause is manual, then output it as-is */
	if((options & GENSEL_OPT_MANUAL) != 0)
	{
		gensel_output_clause_code(clauses);
		return;
	}

	/* Determine the location of this instruction's arguments */
	if((options & GENSEL_OPT_TERNARY) != 0)
	{
		arg1 = "insn->dest";
		arg2 = "insn->value1";
		arg3 = "insn->value2";
		flag1 = "DEST";
		flag2 = "VALUE1";
		flag3 = "VALUE2";
		destroy1 = 0;
		destroy2 = 0;
		destroy3 = 0;
	}
	else
	{
		arg1 = "insn->value1";
		arg2 = "insn->value2";
		arg3 = "??";
		flag1 = "VALUE1";
		flag2 = "VALUE2";
		flag3 = "??";
		if((options & (GENSEL_OPT_BINARY_BRANCH |
					   GENSEL_OPT_UNARY_BRANCH |
					   GENSEL_OPT_BINARY_NOTE |
					   GENSEL_OPT_UNARY_NOTE)) != 0)
		{
			destroy1 = 0;
		}
		else
		{
			destroy1 = 1;
		}
		destroy2 = 0;
		destroy3 = 0;
	}

	/* If all of the clauses start with a register, then load the first
	   value into a register before we start checking cases */
	check_index = 0;
	if((options & (GENSEL_OPT_BINARY | GENSEL_OPT_UNARY |
				   GENSEL_OPT_BINARY_BRANCH | GENSEL_OPT_UNARY_BRANCH |
				   GENSEL_OPT_BINARY_NOTE | GENSEL_OPT_UNARY_NOTE)) != 0 &&
	   (options & GENSEL_OPT_STACK) == 0)
	{
		clause = clauses;
		while(clause != 0)
		{
			if(clause->pattern[0] != GENSEL_PATT_REG &&
			   clause->pattern[0] != GENSEL_PATT_LREG &&
			   clause->pattern[0] != GENSEL_PATT_FREG)
			{
				break;
			}
			clause = clause->next;
		}
		if(!clause)
		{
			printf("\treg = _jit_regs_load_value(gen, %s, %d, ",
				   arg1, destroy1);
			printf("(insn->flags & (JIT_INSN_%s_NEXT_USE | "
								   "JIT_INSN_%s_LIVE)));\n", flag1, flag1);
			check_index = 1;
		}
	}

	/* Output the clause checking and dispatching code */
	clause = clauses;
	first = 1;
	while(clause != 0)
	{
		if(clause->next)
		{
			if(first)
				printf("\tif(");
			else
				printf("\telse if(");
			for(index = check_index; clause->pattern[index]; ++index)
			{
				if(index == 0)
					arg = arg1;
				else if(index == 1)
					arg = arg2;
				else
					arg = arg3;
				switch(clause->pattern[index])
				{
					case GENSEL_PATT_REG:
					case GENSEL_PATT_LREG:
					case GENSEL_PATT_FREG:
					{
						printf("%s->in_register", arg);
					}
					break;

					case GENSEL_PATT_IMM:
					{
						printf("%s->is_constant", arg);
					}
					break;

					case GENSEL_PATT_IMMZERO:
					{
						printf("%s->is_nint_constant && ", arg);
						printf("%s->address == 0", arg);
					}
					break;

					case GENSEL_PATT_IMMS8:
					{
						printf("%s->is_nint_constant && ", arg);
						printf("%s->address >= -128 && ", arg);
						printf("%s->address <= 127", arg);
					}
					break;

					case GENSEL_PATT_IMMU8:
					{
						printf("%s->is_nint_constant && ", arg);
						printf("%s->address >= 0 && ", arg);
						printf("%s->address <= 255", arg);
					}
					break;

					case GENSEL_PATT_IMMS16:
					{
						printf("%s->is_nint_constant && ", arg);
						printf("%s->address >= -32768 && ", arg);
						printf("%s->address <= 32767", arg);
					}
					break;

					case GENSEL_PATT_IMMU16:
					{
						printf("%s->is_nint_constant && ", arg);
						printf("%s->address >= 0 && ", arg);
						printf("%s->address <= 65535", arg);
					}
					break;

					case GENSEL_PATT_LOCAL:
					{
						printf("%s->in_frame && !(%s->in_register)", arg, arg);
					}
					break;
				}
				if(clause->pattern[index + 1])
				{
					printf(" && ");
				}
			}
			printf(")\n\t{\n");
		}
		else if(first)
		{
			printf("\t{\n");
		}
		else
		{
			printf("\telse\n\t{\n");
		}
		if((options & GENSEL_OPT_STACK) == 0 || clause->next)
		{
			for(index = check_index; clause->pattern[index]; ++index)
			{
				if(index == 0)
				{
					arg = arg1;
					reg = "reg";
					flag = flag1;
					destroy = destroy1;
				}
				else if(index == 1)
				{
					arg = arg2;
					reg = "reg2";
					flag = flag2;
					destroy = destroy2;
				}
				else
				{
					arg = arg3;
					reg = "reg3";
					flag = flag3;
					destroy = destroy3;
				}
				switch(clause->pattern[index])
				{
					case GENSEL_PATT_REG:
					case GENSEL_PATT_LREG:
					case GENSEL_PATT_FREG:
					{
						printf("\t\t%s = _jit_regs_load_value(gen, %s, %d, ",
							   reg, arg, destroy);
						printf("(insn->flags & (JIT_INSN_%s_NEXT_USE | "
											   "JIT_INSN_%s_LIVE)));\n",
							   flag, flag);
					}
					break;
	
					case GENSEL_PATT_IMMZERO: break;

					case GENSEL_PATT_IMM:
					case GENSEL_PATT_IMMS8:
					case GENSEL_PATT_IMMU8:
					case GENSEL_PATT_IMMS16:
					case GENSEL_PATT_IMMU16:
					{
						printf("\t\timm_value = %s->address;\n", arg);
					}
					break;
	
					case GENSEL_PATT_LOCAL:
					{
						printf("\t\tlocal_offset = %s->frame_offset;\n", arg);
					}
					break;
				}
			}
		}
		else
		{
			if((options & GENSEL_OPT_ONLY) != 0)
			{
				printf("\t\tif(!_jit_regs_is_top(gen, insn->value1) ||\n");
				printf("\t\t   _jit_regs_num_used(gen, %d) != 1)\n",
					   gensel_first_stack_reg);
				printf("\t\t{\n");
				printf("\t\t\t_jit_regs_spill_all(gen);\n");
				printf("\t\t}\n");
			}
			if((options & GENSEL_OPT_TERNARY) != 0)
			{
				printf("\t\treg = _jit_regs_load_to_top_three\n");
				printf("\t\t\t(gen, insn->dest, insn->value1, insn->value2,\n");
				printf("\t\t\t\t(insn->flags & (JIT_INSN_DEST_NEXT_USE | "
											   "JIT_INSN_DEST_LIVE)),\n");
				printf("\t\t\t\t(insn->flags & (JIT_INSN_VALUE1_NEXT_USE | "
											   "JIT_INSN_VALUE1_LIVE)),\n");
				printf("\t\t\t\t(insn->flags & (JIT_INSN_VALUE2_NEXT_USE | "
											   "JIT_INSN_VALUE2_LIVE)), "
											   "%d);\n",
					   gensel_first_stack_reg);
			}
			else if((options & (GENSEL_OPT_BINARY |
								GENSEL_OPT_BINARY_BRANCH)) != 0)
			{
				printf("\t\treg = _jit_regs_load_to_top_two\n");
				printf("\t\t\t(gen, insn->value1, insn->value2,\n");
				printf("\t\t\t\t(insn->flags & (JIT_INSN_VALUE1_NEXT_USE | "
											   "JIT_INSN_VALUE1_LIVE)),\n");
				printf("\t\t\t\t(insn->flags & (JIT_INSN_VALUE2_NEXT_USE | "
											   "JIT_INSN_VALUE2_LIVE)), "
											   "%d);\n",
					   gensel_first_stack_reg);
			}
			else if((options & (GENSEL_OPT_UNARY |
								GENSEL_OPT_UNARY_BRANCH |
								GENSEL_OPT_UNARY_NOTE)) != 0)
			{
				printf("\t\treg = _jit_regs_load_to_top\n");
				printf("\t\t\t(gen, insn->value1,\n");
				printf("\t\t\t\t(insn->flags & (JIT_INSN_VALUE1_NEXT_USE | "
											   "JIT_INSN_VALUE1_LIVE)), "
											   "%d);\n",
					   gensel_first_stack_reg);
			}
		}
		if((options & (GENSEL_OPT_BINARY_BRANCH |
					   GENSEL_OPT_UNARY_BRANCH)) != 0)
		{
			/* Spill all other registers back to their original positions */
			printf("\t\t_jit_regs_spill_all(gen);\n");
		}
		gensel_output_clause(clause, options);
		printf("\t}\n");
		first = 0;
		clause = clause->next;
	}
}

/*
 * List of opcodes that are supported by the input rules.
 */
static char **supported = 0;
static char **supported_options = 0;
static int num_supported = 0;

/*
 * Add an opcode to the supported list.
 */
static void gensel_add_supported(char *name, char *option)
{
	supported = (char **)realloc
		(supported, (num_supported + 1) * sizeof(char *));
	if(!supported)
	{
		exit(1);
	}
	supported[num_supported] = name;
	supported_options = (char **)realloc
		(supported_options, (num_supported + 1) * sizeof(char *));
	if(!supported_options)
	{
		exit(1);
	}
	supported_options[num_supported++] = option;
}

/*
 * Output the list of supported opcodes.
 */
static void gensel_output_supported(void)
{
	int index;
	for(index = 0; index < num_supported; ++index)
	{
		if(supported_options[index])
		{
			if(supported_options[index][0] == '!')
			{
				printf("#ifndef %s\n", supported_options[index] + 1);
			}
			else
			{
				printf("#ifdef %s\n", supported_options[index]);
			}
			printf("case %s:\n", supported[index]);
			printf("#endif\n");
		}
		else
		{
			printf("case %s:\n", supported[index]);
		}
	}
	printf("\treturn 1;\n\n");
}

%}

/*
 * Define the structure of yylval.
 */
%union {
	char		   *name;
	struct
	{
		char	   *filename;
		long		linenum;
		char	   *block;
	}				code;
	int				options;
	struct
	{
		int			elem[8];
		int			size;

	}				pattern;
	struct
	{
		struct gensel_clause *head;
		struct gensel_clause *tail;

	}				clauses;
}

/*
 * Primitive lexical tokens and keywords.
 */
%token IDENTIFIER			"an identifier"
%token CODE_BLOCK			"a code block"
%token K_PTR				"`->'"
%token K_REG				"word register"
%token K_LREG				"long register"
%token K_FREG				"float register"
%token K_IMM				"immediate value"
%token K_IMMZERO			"immediate zero value"
%token K_IMMS8				"immediate signed 8-bit value"
%token K_IMMU8				"immediate unsigned 8-bit value"
%token K_IMMS16				"immediate signed 16-bit value"
%token K_IMMU16				"immediate unsigned 16-bit value"
%token K_LOCAL				"local variable"
%token K_SPILL_BEFORE		"`spill_before'"
%token K_BINARY				"`binary'"
%token K_UNARY				"`unary'"
%token K_UNARY_BRANCH		"`unary_branch'"
%token K_BINARY_BRANCH		"`binary_branch'"
%token K_UNARY_NOTE			"`unary_note'"
%token K_BINARY_NOTE		"`binary_note'"
%token K_TERNARY			"`ternary'"
%token K_STACK				"`stack'"
%token K_ONLY				"`only'"
%token K_MORE_SPACE			"`more_space'"
%token K_MANUAL				"`manual'"
%token K_INST_TYPE			"`%inst_type'"

/*
 * Define the yylval types of the various non-terminals.
 */
%type <name>				IDENTIFIER IfClause IdentifierList
%type <code>				CODE_BLOCK
%type <options>				Options OptionList Option PatternElement
%type <clauses>				Clauses Clause
%type <pattern>				Pattern Pattern2

%expect 0

%start Start
%%

Start
	: /* empty */
	| Rules
	;

Rules
	: Rule
	| Rules Rule
	;

Rule
	: IdentifierList IfClause ':' Options Clauses {
				if($2)
				{
					if(($2)[0] == '!')
					{
						printf("#ifndef %s\n\n", $2 + 1);
					}
					else
					{
						printf("#ifdef %s\n\n", $2);
					}
				}
				printf("case %s:\n{\n", $1);
				if(($4 & GENSEL_OPT_MANUAL) == 0)
				{
					printf("\t%s inst;\n", gensel_inst_type);
				}
				gensel_declare_regs($5.head, $4);
				if(($4 & GENSEL_OPT_SPILL_BEFORE) != 0)
				{
					printf("\t_jit_regs_spill_all(gen);\n");
				}
				gensel_output_clauses($5.head, $4);
				if(($4 & (GENSEL_OPT_BINARY | GENSEL_OPT_UNARY)) != 0)
				{
					printf("\tif((insn->flags & JIT_INSN_DEST_NEXT_USE) != 0)\n");
					printf("\t{\n");
					printf("\t\t_jit_regs_set_value(gen, reg, insn->dest, 0);\n");
					printf("\t}\n");
					printf("\telse\n");
					printf("\t{\n");
					printf("\t\tint other_reg;\n");
					printf("\t\tif(gen->contents[reg].is_long_start)\n");
					printf("\t\t{\n");
					printf("\t\t\tother_reg = _jit_reg_info[reg].other_reg;\n");
					printf("\t\t}\n");
					printf("\t\telse\n");
					printf("\t\t{\n");
					printf("\t\t\tother_reg = -1;\n");
					printf("\t\t}\n");
					printf("\t\t_jit_gen_spill_reg(gen, reg, other_reg, insn->dest);\n");
					printf("\t\tif(insn->dest->has_global_register)\n");
					printf("\t\t\tinsn->dest->in_global_register = 1;\n");
					printf("\t\telse\n");
					printf("\t\t\tinsn->dest->in_frame = 1;\n");
					printf("\t\t_jit_regs_free_reg(gen, reg, 1);\n");
					printf("\t}\n");
				}
				printf("}\nbreak;\n\n");
				if($2)
				{
					printf("#endif /* %s */\n\n", $2);
				}
				gensel_free_clauses($5.head);
				gensel_add_supported($1, $2);
			}
	| K_INST_TYPE IDENTIFIER	{
				gensel_inst_type = $2;
				gensel_new_inst_type = 1;
			}
	;

IdentifierList
	: IDENTIFIER				{ $$ = $1; }
	| IdentifierList ',' IDENTIFIER	{
				char *result = (char *)malloc(strlen($1) + strlen($3) + 16);
				if(!result)
				{
					exit(1);
				}
				strcpy(result, $1);
				strcat(result, ":\ncase ");
				strcat(result, $3);
				free($1);
				free($3);
				$$ = result;
			}
	;

IfClause
	: /* empty */				{ $$ = 0; }
	| '(' IDENTIFIER ')'		{ $$ = $2; }
	;

Options
	: /* empty */				{ $$ = 0; }
	| OptionList				{ $$ = $1; }
	;

OptionList
	: Option					{ $$ = $1; }
	| OptionList ',' Option		{ $$ = $1 | $3; }
	;

Option
	: K_SPILL_BEFORE			{ $$ = GENSEL_OPT_SPILL_BEFORE; }
	| K_BINARY					{ $$ = GENSEL_OPT_BINARY; }
	| K_UNARY					{ $$ = GENSEL_OPT_UNARY; }
	| K_UNARY_BRANCH			{ $$ = GENSEL_OPT_UNARY_BRANCH; }
	| K_BINARY_BRANCH			{ $$ = GENSEL_OPT_BINARY_BRANCH; }
	| K_UNARY_NOTE				{ $$ = GENSEL_OPT_UNARY_NOTE; }
	| K_BINARY_NOTE				{ $$ = GENSEL_OPT_BINARY_NOTE; }
	| K_TERNARY					{ $$ = GENSEL_OPT_TERNARY; }
	| K_STACK					{ $$ = GENSEL_OPT_STACK; }
	| K_ONLY					{ $$ = GENSEL_OPT_ONLY; }
	| K_MORE_SPACE				{ $$ = GENSEL_OPT_MORE_SPACE; }
	| K_MANUAL					{ $$ = GENSEL_OPT_MANUAL; }
	;

Clauses
	: Clause					{ $$ = $1; }
	| Clauses Clause			{
				$1.tail->next = $2.head;
				$$.head = $1.head;
				$$.tail = $2.tail;
			}
	;

Clause
	: '[' Pattern ']' K_PTR CODE_BLOCK	{
				gensel_clause_t clause;
				int index;
				clause = (gensel_clause_t)malloc(sizeof(struct gensel_clause));
				if(!clause)
				{
					exit(1);
				}
				for(index = 0; index < 8; ++index)
				{
					clause->pattern[index] = $2.elem[index];
				}
				clause->filename = $5.filename;
				clause->linenum = $5.linenum;
				clause->code = $5.block;
				clause->next = 0;
				$$.head = clause;
				$$.tail = clause;
			}
	;

Pattern
	: /* empty */		{
				$$.elem[0] = GENSEL_PATT_END;
				$$.size = 0;
			}
	| Pattern2			{ $$ = $1; }
	;

Pattern2
	: PatternElement				{
				$$.elem[0] = $1;
				$$.elem[1] = GENSEL_PATT_END;
				$$.size = 1;
			}
	| Pattern2 ',' PatternElement	{
				$$.elem[$1.size] = $3;
				$$.elem[$1.size + 1] = GENSEL_PATT_END;
				$$.size = $1.size + 1;
			}
	;

PatternElement
	: K_REG						{ $$ = GENSEL_PATT_REG; }
	| K_LREG					{ $$ = GENSEL_PATT_LREG; }
	| K_FREG					{ $$ = GENSEL_PATT_FREG; }
	| K_IMM						{ $$ = GENSEL_PATT_IMM; }
	| K_IMMZERO					{ $$ = GENSEL_PATT_IMMZERO; }
	| K_IMMS8					{ $$ = GENSEL_PATT_IMMS8; }
	| K_IMMU8					{ $$ = GENSEL_PATT_IMMU8; }
	| K_IMMS16					{ $$ = GENSEL_PATT_IMMS16; }
	| K_IMMU16					{ $$ = GENSEL_PATT_IMMU16; }
	| K_LOCAL					{ $$ = GENSEL_PATT_LOCAL; }
	;

%%

#define	COPYRIGHT_MSG	\
" * Copyright (C) 2004  Southern Storm Software, Pty Ltd.\n" \
" *\n" \
" * This file is part of the libjit library.\n" \
" *\n" \
" * The libjit library is free software: you can redistribute it and/or\n" \
" * modify it under the terms of the GNU Lesser General Public License\n" \
" * as published by the Free Software Foundation, either version 2.1 of\n" \
" * the License, or (at your option) any later version.\n" \
" *\n" \
" * The libjit library is distributed in the hope that it will be useful,\n" \
" * but WITHOUT ANY WARRANTY; without even the implied warranty of\n" \
" * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n" \
" * Lesser General Public License for more details.\n" \
" *\n" \
" * You should have received a copy of the GNU Lesser General Public\n" \
" * License along with the libjit library.  If not, see\n" \
" * <http://www.gnu.org/licenses/>.\n"
 
int main(int argc, char *argv[])
{
	FILE *file;
	if(argc != 2)
	{
		fprintf(stderr, "Usage: %s input.sel >output.slc\n", argv[0]);
		return 1;
	}
	file = fopen(argv[1], "r");
	if(!file)
	{
		perror(argv[1]);
		return 1;
	}
	printf("/%c Automatically generated from %s - DO NOT EDIT %c/\n",
		   '*', argv[1], '*');
	printf("/%c\n%s%c/\n\n", '*', COPYRIGHT_MSG, '*');
	printf("#if defined(JIT_INCLUDE_RULES)\n\n");
	gensel_filename = argv[1];
	gensel_linenum = 1;
	yyrestart(file);
	if(yyparse())
	{
		fclose(file);
		return 1;
	}
	fclose(file);
	printf("#elif defined(JIT_INCLUDE_SUPPORTED)\n\n");
	gensel_output_supported();
	printf("#endif\n");
	return 0;
}
