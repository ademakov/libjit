%{
/*
 * gen-rules-parser.y - Bison grammar for the "gen-rules" program.
 *
 * Copyright (C) 2006  Southern Storm Software, Pty Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
 * Current file and line number.
 */
extern char *gensel_filename;
extern long gensel_linenum;

/*
 * Report error message.
 */
static void
gensel_error_message(char *filename, long linenum, char *msg)
{
	fprintf(stderr, "%s(%ld): %s\n", filename, linenum, msg);
}

/*
 * Report error message and exit.
 */
static void
gensel_error(char *filename, long linenum, char *msg)
{
	gensel_error_message(filename, linenum, msg);
	exit(1);
}

/*
 * Report error messages from the parser.
 */
static void
yyerror(char *msg)
{
	gensel_error_message(gensel_filename, gensel_linenum, msg);
}

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
 * Maximal number of input values in a pattern.
 */
#define MAX_INPUT				3

/*
 * Maximal number of scratch registers in a pattern.
 */
#define MAX_SCRATCH				6

/*
 * Options.
 */
#define	GENSEL_OPT_BINARY			1
#define	GENSEL_OPT_UNARY			2
#define	GENSEL_OPT_UNARY_BRANCH			3
#define	GENSEL_OPT_BINARY_BRANCH		4
#define	GENSEL_OPT_UNARY_NOTE			5
#define	GENSEL_OPT_BINARY_NOTE			6
#define	GENSEL_OPT_TERNARY			7
#define	GENSEL_OPT_STACK			8
#define	GENSEL_OPT_ONLY				9
#define	GENSEL_OPT_COPY				10
#define GENSEL_OPT_X87ARITH			11
#define GENSEL_OPT_COMMUTATIVE			12
#define GENSEL_OPT_REVERSIBLE			13

#define	GENSEL_OPT_MANUAL			14
#define	GENSEL_OPT_MORE_SPACE			15
#define	GENSEL_OPT_SPILL_BEFORE			16

/*
 * Pattern values.
 */
#define	GENSEL_PATT_REG				1
#define	GENSEL_PATT_LREG			2
#define	GENSEL_PATT_FREG			3
#define	GENSEL_PATT_IMM				4
#define	GENSEL_PATT_IMMZERO			5
#define	GENSEL_PATT_IMMS8			6
#define	GENSEL_PATT_IMMU8			7
#define	GENSEL_PATT_IMMS16			8
#define	GENSEL_PATT_IMMU16			9
#define	GENSEL_PATT_LOCAL			10
#define	GENSEL_PATT_SCRATCH			11
#define	GENSEL_PATT_CLOBBER			12
#define	GENSEL_PATT_IF				13
#define	GENSEL_PATT_SPACE			14

/*
 * Option value.
 */
typedef struct gensel_value *gensel_value_t;
struct gensel_value
{
	char			*value;
	gensel_value_t		next;
};

/*
 * Option information.
 */
typedef struct gensel_option *gensel_option_t;
struct gensel_option
{
	int			option;
	gensel_value_t		values;
	gensel_option_t		next;
};

/*
 * Information about clauses.
 */
typedef struct gensel_clause *gensel_clause_t;
struct gensel_clause
{
	gensel_option_t		pattern;
	char			*filename;
	long			linenum;
	char			*code;
	gensel_clause_t		next;
};

static char *gensel_reg_names[] = {
	"reg", "reg2", "reg3", "reg4", "reg5", "reg6", "reg7", "reg8", "reg9"
};
static char *gensel_other_reg_names[] = {
	"other_reg", "other_reg2", "other_reg3"
};
static char *gensel_imm_names[] = {
	"imm_value", "imm_value2", "imm_value3"
};

/*
 * Create an option.
 */
static gensel_option_t
gensel_create_option(int option, gensel_value_t values)
{
	gensel_option_t op;

	op = (gensel_option_t) malloc(sizeof(struct gensel_option));
	if(!op)
	{
		exit(1);
	}

	op->option = option;
	op->values = values;
	op->next = 0;
	return op;
}

/*
 * Free a list of values.
 */
static void
gensel_free_values(gensel_value_t values)
{
	gensel_value_t next;
	while(values)
	{
		next = values->next;
		free(values->value);
		free(values);
		values = next;
	}
}

/*
 * Free a list of options.
 */
static void
gensel_free_options(gensel_option_t options)
{
	gensel_option_t next;
	while(options)
	{
		next = options->next;
		gensel_free_values(options->values);
		free(options);
		options = next;
	}
}

/*
 * Free a list of clauses.
 */
static void
gensel_free_clauses(gensel_clause_t clauses)
{
	gensel_clause_t next;
	while(clauses != 0)
	{
		next = clauses->next;
		gensel_free_options(clauses->pattern);
		free(clauses->code);
		free(clauses);
		clauses = next;
	}
}

/*
 * Look for the option.
 */
static gensel_option_t
gensel_search_option(gensel_option_t options, int tag)
{
	while(options && options->option != tag)
	{
		options = options->next;
	}
	return options;
}

/*
 * Declare the register variables that are needed for a set of clauses.
 */
static void gensel_declare_regs(gensel_clause_t clauses, gensel_option_t options)
{
	gensel_option_t pattern;
	gensel_value_t values;
	int regs, max_regs;
	int other_regs_mask;
	int imms, max_imms;
	int have_local;
	int scratch, max_scratch;
	int have_clobber;
	int others;

	max_regs = 0;
	other_regs_mask = 0;
	max_imms = 0;
	have_local = 0;
	max_scratch = 0;
	have_clobber = 0;
	while(clauses != 0)
	{
		regs = 0;
		imms = 0;
		others = 0;
		scratch = 0;
		pattern = clauses->pattern;
		while(pattern)
		{
			switch(pattern->option)
			{
			case GENSEL_PATT_REG:
			case GENSEL_PATT_FREG:
				++regs;
				break;

			case GENSEL_PATT_LREG:
				other_regs_mask |= (1 << regs);
				++regs;
				break;

			case GENSEL_PATT_IMMZERO:
				++others;
				break;

			case GENSEL_PATT_IMM:
			case GENSEL_PATT_IMMS8:
			case GENSEL_PATT_IMMU8:
			case GENSEL_PATT_IMMS16:
			case GENSEL_PATT_IMMU16:
				++imms;
				break;

			case GENSEL_PATT_LOCAL:
				have_local = 1;
				++others;
				break;

			case GENSEL_PATT_SCRATCH:
				values = pattern->values;
				while(values)
				{
					++scratch;
					values = values->next;
				}

			case GENSEL_PATT_CLOBBER:
				values = pattern->values;
				while(values)
				{
					if(values->value && strcmp(values->value, "*") != 0)
					{
						have_clobber = 1;
						break;
					}
					values = values->next;
				}
			}
			pattern = pattern->next;
		}
		if((regs + imms + others) > MAX_INPUT)
		{
			gensel_error(
				clauses->filename,
				clauses->linenum,
				"too many input args in the pattern");
		}
		if(scratch > MAX_SCRATCH)
		{
			gensel_error(
				clauses->filename,
				clauses->linenum,
				"too many scratch args in the pattern");
		}
		if(max_regs < regs)
		{
			max_regs = regs;
		}
		if(max_imms < imms)
		{
			max_imms = imms;
		}
		if(max_scratch < scratch)
		{
			max_scratch = scratch;
		}
		clauses = clauses->next;
	}
	switch(max_regs)
	{
	case 1:
		printf("\tint reg;\n");
		break;
	case 2:
		printf("\tint reg, reg2;\n");
		break;
	case 3:
		printf("\tint reg, reg2, reg3;\n");
		break;
	}
	if(other_regs_mask)
	{
		switch(other_regs_mask)
		{
		case 1:
			printf("\tint other_reg;\n");
			break;
		case 2:
			printf("\tint other_reg2;\n");
			break;
		case 3:
			printf("\tint other_reg, other_reg2;\n");
			break;
		case 4:
			printf("\tint other_reg3;\n");
			break;
		case 5:
			printf("\tint other_reg, othre_reg3;\n");
			break;
		case 6:
			printf("\tint other_reg2, othre_reg3;\n");
			break;
		case 7:
			printf("\tint other_reg, other_reg2, othre_reg3;\n");
			break;
		}
	}
	switch(max_imms)
	{
	case 1:
		printf("\tjit_nint imm_value;\n");
		break;
	case 2:
		printf("\tjit_nint imm_value, imm_value2;\n");
		break;
	case 3:
		printf("\tjit_nint imm_value, imm_value2, imm_value3;\n");
		break;
	}
	for(scratch = 0; scratch < max_scratch; scratch++)
	{
		if((scratch + max_regs) == 0)
		{
			printf("\tint reg;\n");
		}
		else
		{
			printf("\tint reg%d;\n", scratch + max_regs + 1);
		}
	}
	if(have_local)
	{
		printf("\tjit_nint local_offset;\n");
	}
	if(have_clobber)
	{
		printf("\tint clobber;\n");
	}
}

/*
 * Build index of input value names.
 */
static void
gensel_build_index(gensel_option_t pattern, char *names[9], char *other_names[9])
{
	gensel_value_t values;
	int regs, imms, index;

	for(index = 0; index < 9; index++)
	{
		names[index] = "undefined";
		other_names[index] = "undefined";
	}

	regs = 0;
	imms = 0;
	index = 0;
	while(pattern)
	{
		switch(pattern->option)
		{
		case GENSEL_PATT_REG:
		case GENSEL_PATT_FREG:
			names[index] = gensel_reg_names[regs];
			++regs;
			++index;
			break;

		case GENSEL_PATT_LREG:
			names[index] = gensel_reg_names[regs];
			other_names[index] = gensel_other_reg_names[regs];
			++regs;
			++index;
			break;

		case GENSEL_PATT_IMMZERO:
			++index;
			break;

		case GENSEL_PATT_IMM:
		case GENSEL_PATT_IMMS8:
		case GENSEL_PATT_IMMU8:
		case GENSEL_PATT_IMMS16:
		case GENSEL_PATT_IMMU16:
			names[index] = gensel_imm_names[imms];
			++imms;
			++index;
			break;

		case GENSEL_PATT_LOCAL:
			names[index] = "local_offset";
			++index;
			break;

		case GENSEL_PATT_SCRATCH:
			values = pattern->values;
			while(values)
			{
				names[index] = gensel_reg_names[regs];
				++regs;
				++index;
				values = values->next;
			}
		}
		pattern = pattern->next;
	}
}

/*
 * Output the code.
 */
static void
gensel_output_code(gensel_option_t pattern, char *code)
{
	char *names[9];
	char *other_names[9];
	int index;
	
	gensel_build_index(pattern, names, other_names);

	/* Output the clause code */
	printf("\t\t");
	while(*code != '\0')
	{
		if(*code == '$' && code[1] >= '1' && code[1] <= '9')
		{
			index = code[1] - '1';
			printf(names[index]);
			code += 2;
		}
		else if(*code == '%' && code[1] >= '1' && code[1] <= '9')
		{
			index = code[1] - '1';
			printf(other_names[index]);
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
 * Output the code within a clause.
 */
static void
gensel_output_clause_code(gensel_clause_t clause)
{
	/* Output the line number information from the original file */
	printf("#line %ld \"%s\"\n", clause->linenum, clause->filename);

	gensel_output_code(clause->pattern, clause->code);
}

/*
 * Output a single clause for a rule.
 */
static void gensel_output_clause(gensel_clause_t clause, gensel_option_t options)
{
	gensel_option_t space, more_space;

	/* Cache the instruction pointer into "inst" */
	if(gensel_new_inst_type)
	{
		printf("\t\tjit_gen_load_inst_ptr(gen, inst);\n");
	}
	else
	{
		space = gensel_search_option(clause->pattern, GENSEL_PATT_SPACE);
		more_space = gensel_search_option(options, GENSEL_OPT_MORE_SPACE);

		printf("\t\tinst = (%s)(gen->posn.ptr);\n", gensel_inst_type);
		printf("\t\tif(!jit_cache_check_for_n(&(gen->posn), ");
		if(space && space->values && space->values->value)
		{
			printf("(");
			gensel_output_code(clause->pattern, space->values->value);
			printf(")");
		}
		else
		{
			printf("%d", ((more_space == 0)
				      ? gensel_reserve_space
				      : gensel_reserve_more_space));
		}
		printf("))\n");
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
static void gensel_output_clauses(gensel_clause_t clauses, gensel_option_t options)
{
	char *args[MAX_INPUT];
	gensel_clause_t clause;
	gensel_option_t pattern;
	gensel_value_t values;
	int first, seen_option;
	int regs, imms, index;
	int scratch, clobber_all;

	/* If the clause is manual, then output it as-is */
	if(gensel_search_option(options, GENSEL_OPT_MANUAL))
	{
		if(gensel_search_option(options, GENSEL_OPT_SPILL_BEFORE))
		{
			printf("\t_jit_regs_spill_all(gen);\n");
		}
		gensel_output_clause_code(clauses);
		return;
	}

	printf("\t%s inst;\n", gensel_inst_type);
	printf("\t_jit_regs_t regs;\n");
	gensel_declare_regs(clauses, options);
	if(gensel_search_option(options, GENSEL_OPT_SPILL_BEFORE))
	{
		printf("\t_jit_regs_spill_all(gen);\n");
	}

	/* Determine the location of this instruction's arguments */
	if(gensel_search_option(options, GENSEL_OPT_TERNARY))
	{
		args[0] = "dest";
		args[1] = "value1";
		args[2] = "value2";
	}
	else
	{
		args[0] = "value1";
		args[1] = "value2";
		args[2] = "??";
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

			index = 0;
			seen_option = 0;
			pattern = clause->pattern;
			while(pattern && index < MAX_INPUT)
			{
				switch(pattern->option)
				{
				case GENSEL_PATT_REG:
				case GENSEL_PATT_LREG:
				case GENSEL_PATT_FREG:
					if(index > 0 || seen_option)
					{
						printf(" && ");
					}
					printf("insn->%s->in_register", args[index]);
					++index;
					break;

				case GENSEL_PATT_IMM:
					if(index > 0 || seen_option)
					{
						printf(" && ");
					}
					printf("insn->%s->is_constant", args[index]);
					++index;
					break;

				case GENSEL_PATT_IMMZERO:
					if(index > 0 || seen_option)
					{
						printf(" && ");
					}
					printf("insn->%s->is_nint_constant && ", args[index]);
					printf("insn->%s->address == 0", args[index]);
					++index;
					break;

				case GENSEL_PATT_IMMS8:
					if(index > 0 || seen_option)
					{
						printf(" && ");
					}
					printf("insn->%s->is_nint_constant && ", args[index]);
					printf("insn->%s->address >= -128 && ", args[index]);
					printf("insn->%s->address <= 127", args[index]);
					++index;
					break;

				case GENSEL_PATT_IMMU8:
					if(index > 0 || seen_option)
					{
						printf(" && ");
					}
					printf("insn->%s->is_nint_constant && ", args[index]);
					printf("insn->%s->address >= 0 && ", args[index]);
					printf("insn->%s->address <= 255", args[index]);
					++index;
					break;

				case GENSEL_PATT_IMMS16:
					if(index > 0 || seen_option)
					{
						printf(" && ");
					}
					printf("insn->%s->is_nint_constant && ", args[index]);
					printf("insn->%s->address >= -32768 && ", args[index]);
					printf("insn->%s->address <= 32767", args[index]);
					++index;
					break;

				case GENSEL_PATT_IMMU16:
					if(index > 0 || seen_option)
					{
						printf(" && ");
					}
					printf("insn->%s->is_nint_constant && ", args[index]);
					printf("insn->%s->address >= 0 && ", args[index]);
					printf("insn->%s->address <= 65535", args[index]);
					++index;
					break;

				case GENSEL_PATT_LOCAL:
					if(index > 0 || seen_option)
					{
						printf(" && ");
					}
					printf("insn->%s->in_frame && !(insn->%s->in_register)",
					       args[index], args[index]);
					++index;
					break;

				case GENSEL_PATT_IF:
					if(index > 0 || seen_option)
					{
						printf(" && ");
					}
					printf("(");
					gensel_output_code(clause->pattern, pattern->values->value);
					printf(")");
					seen_option = 1;
					break;
				}
				pattern = pattern->next;
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

		if(gensel_search_option(options, GENSEL_OPT_STACK)
		   && gensel_search_option(options, GENSEL_OPT_ONLY))
		{
			printf("\t\tif(!_jit_regs_is_top(gen, insn->value1) ||\n");
			printf("\t\t   _jit_regs_num_used(gen, %d) != 1)\n",
			       gensel_first_stack_reg);
			printf("\t\t{\n");
			printf("\t\t\t_jit_regs_spill_all(gen);\n");
			printf("\t\t}\n");
		}

		clobber_all = 0;
		pattern = gensel_search_option(clause->pattern, GENSEL_PATT_CLOBBER);
		if(pattern)
		{
			values = pattern->values;
			while(values)
			{
				if(values->value && strcmp(values->value, "*") == 0)
				{
					clobber_all = 1;
					break;
				}
				values = values->next;
			}
		}

		seen_option = 0;
		printf("\t\t_jit_regs_init(&regs, ");
		if(clobber_all)
		{
			seen_option = 1;
			printf("_JIT_REGS_CLOBBER_ALL");
		}
		if(gensel_search_option(options, GENSEL_OPT_TERNARY))
		{
			if(seen_option)
			{
				printf(" | ");
			}
			else
			{
				seen_option = 1;
			}
			printf("_JIT_REGS_TERNARY");
		}
		if(gensel_search_option(options, GENSEL_OPT_BINARY_BRANCH)
		   || gensel_search_option(options, GENSEL_OPT_UNARY_BRANCH))
		{
			if(seen_option)
			{
				printf(" | ");
			}
			else
			{
				seen_option = 1;
			}
			printf("_JIT_REGS_BRANCH");
		}
		if(gensel_search_option(options, GENSEL_OPT_COPY))
		{
			if(seen_option)
			{
				printf(" | ");
			}
			else
			{
				seen_option = 1;
			}
			printf("_JIT_REGS_COPY");
		}
		if(gensel_search_option(options, GENSEL_OPT_STACK))
		{
			if(seen_option)
			{
				printf(" | ");
			}
			else
			{
				seen_option = 1;
			}
			printf("_JIT_REGS_STACK");
		}
		if(gensel_search_option(options, GENSEL_OPT_X87ARITH))
		{
			if(seen_option)
			{
				printf(" | ");
			}
			else
			{
				seen_option = 1;
			}
			printf("_JIT_REGS_X87_ARITH");
		}
		if(gensel_search_option(options, GENSEL_OPT_COMMUTATIVE))
		{
			if(seen_option)
			{
				printf(" | ");
			}
			else
			{
				seen_option = 1;
			}
			printf("_JIT_REGS_COMMUTATIVE");
		}
		if(gensel_search_option(options, GENSEL_OPT_REVERSIBLE))
		{
			if(seen_option)
			{
				printf(" | ");
			}
			else
			{
				seen_option = 1;
			}
			printf("_JIT_REGS_REVERSIBLE");
		}
		if(!seen_option)
		{
			printf("0");
		}
		printf(");\n");

		if(!(gensel_search_option(options, GENSEL_OPT_TERNARY)
		     || gensel_search_option(options, GENSEL_OPT_BINARY_NOTE)
		     || gensel_search_option(options, GENSEL_OPT_BINARY_BRANCH)
		     || gensel_search_option(options, GENSEL_OPT_UNARY_NOTE)
		     || gensel_search_option(options, GENSEL_OPT_UNARY_BRANCH)))
		{
			printf("\t\t_jit_regs_set_dest(&regs, insn, 0, -1, -1);\n");
		}

		regs = 0;
		index = 0;
		pattern = clause->pattern;
		while(pattern && index < MAX_INPUT)
		{
			switch(pattern->option)
			{
			case GENSEL_PATT_REG:
			case GENSEL_PATT_FREG:
				if(pattern->values && pattern->values->value)
				{
					printf("\t\t%s = _jit_regs_lookup(\"%s\")];\n",
					       gensel_reg_names[regs],
					       pattern->values->value);
					printf("\t\t_jit_regs_set_%s(&regs, insn, 0, %s, -1);\n",
					       args[index], gensel_reg_names[regs]);
				}
				else
				{
					printf("\t\t_jit_regs_set_%s(&regs, insn, 0, -1, -1);\n",
					       args[index]);
				}
				++regs;
				++index;
				break;

			case GENSEL_PATT_LREG:
				if(pattern->values && pattern->values->value)
				{
					if(pattern->values->next
					   && pattern->values->next->value)
					{
						printf("\t\t%s = _jit_regs_lookup(\"%s\")];\n",
						       gensel_reg_names[regs],
						       pattern->values->value);
						printf("\t\t%s = _jit_regs_lookup(\"%s\")];\n",
						       gensel_other_reg_names[regs],
						       pattern->values->next->value);
						printf("\t\t_jit_regs_set_%s(&regs, insn, 0, %s, %s);\n",
						       args[index],
						       gensel_reg_names[regs],
						       gensel_other_reg_names[regs]);
					}
					else
					{
						printf("\t\t%s = _jit_regs_lookup(\"%s\")];\n",
						       gensel_reg_names[regs],
						       pattern->values->value);
						printf("\t\t_jit_regs_set_%s(&regs, insn, 0, %s, -1);\n",
						       args[index], gensel_reg_names[regs]);
					}
				}
				else
				{
					printf("\t\t_jit_regs_set_%s(&regs, insn, 0, -1, -1);\n",
					       args[index]);
				}
				++regs;
				++index;
				break;

			case GENSEL_PATT_IMMZERO:
			case GENSEL_PATT_IMM:
			case GENSEL_PATT_IMMS8:
			case GENSEL_PATT_IMMU8:
			case GENSEL_PATT_IMMS16:
			case GENSEL_PATT_IMMU16:
			case GENSEL_PATT_LOCAL:
				++index;
				break;

			case GENSEL_PATT_SCRATCH:
				values = pattern->values;
				while(values)
				{
					if(values->value && strcmp(values->value, "?") != 0)
					{
						printf("\t\t%s = _jit_regs_lookup(\"%s\")];\n",
						       gensel_reg_names[regs],
						       pattern->values->value);
						printf("\t\t_jit_regs_set_scratch(&regs, clobber);\n");
					}
					else
					{
						printf("\t\t_jit_regs_set_scratch(&regs, -1);\n");
					}
					++regs;
					++index;
					values = values->next;
				}
				break;

			case GENSEL_PATT_CLOBBER:
				values = pattern->values;
				while(values)
				{
					if(values->value && strcmp(values->value, "*") != 0)
					{
						printf("\t\tclobber = _jit_regs_lookup(\"%s\")];\n",
						       pattern->values->value);
						printf("\t\t_jit_regs_set_clobber(&regs, clobber);\n");
					}
					values = values->next;
				}
				break;
			}
			pattern = pattern->next;
		}

		printf("\t\tif(!_jit_regs_assign(gen, &regs))\n");
		printf("\t\t{\n");
		printf("\t\t\treturn;\n");
		printf("\t\t}\n");
		printf("\t\tif(!_jit_regs_gen(gen, &regs))\n");
		printf("\t\t{\n");
		printf("\t\t\treturn;\n");
		printf("\t\t}\n");

		regs = 0;
		imms = 0;
		index = 0;
		scratch = 0;
		pattern = clause->pattern;
		while(pattern && index < MAX_INPUT)
		{
			switch(pattern->option)
			{
			case GENSEL_PATT_REG:
			case GENSEL_PATT_FREG:
				printf("\t\t%s = _jit_reg_info[_jit_regs_%s(&regs)].cpu_reg;\n",
				       gensel_reg_names[regs], args[index]);
				++regs;
				++index;
				break;

			case GENSEL_PATT_LREG:
				printf("\t\t%s = _jit_reg_info[_jit_regs_%s(&regs)].cpu_reg;\n",
				       gensel_reg_names[regs], args[index]);
				printf("\t\t%s = _jit_reg_info[_jit_regs_%s_other(&regs)].cpu_reg;\n",
				       gensel_other_reg_names[regs], args[index]);
				++regs;
				++index;
				break;

			case GENSEL_PATT_IMMZERO:
				++index;
				break;

			case GENSEL_PATT_IMM:
			case GENSEL_PATT_IMMS8:
			case GENSEL_PATT_IMMU8:
			case GENSEL_PATT_IMMS16:
			case GENSEL_PATT_IMMU16:
				printf("\t\t%s = insn->%s->address;\n",
				       gensel_imm_names[imms], args[index]);
				++imms;
				++index;
				break;

			case GENSEL_PATT_LOCAL:
				printf("\t\tlocal_offset = insn->%s->frame_offset;\n",
				       args[index]);
				index++;
				break;

			case GENSEL_PATT_SCRATCH:
				values = pattern->values;
				while(values)
				{
					printf("\t\t%s = _jit_regs_scratch(&regs, %d);\n",
					       gensel_reg_names[regs], scratch);
					++regs;
					++scratch;
					values = values->next;
				}
				break;

			}
			pattern = pattern->next;
		}

		if(gensel_search_option(options, GENSEL_OPT_BINARY_BRANCH)
		   || gensel_search_option(options, GENSEL_OPT_UNARY_BRANCH))
		{
			/* Spill all other registers back to their original positions */
			printf("\t\t_jit_regs_spill_all(gen);\n");
		}

		gensel_output_clause(clause, options);

		printf("\t\t_jit_regs_commit(gen, &regs);\n");

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
	int			tag;
	char			*name;
	struct gensel_value	*value;
	struct gensel_option	*option;
	struct
	{
		char	*filename;
		long	linenum;
		char	*block;

	}	code;
	struct
	{
		struct gensel_value	*head;
		struct gensel_value	*tail;

	}	values;
	struct
	{
		struct gensel_option	*head;
		struct gensel_option	*tail;

	}	options;
	struct
	{
		struct gensel_clause	*head;
		struct gensel_clause	*tail;

	}	clauses;
}

/*
 * Primitive lexical tokens and keywords.
 */
%token IDENTIFIER		"an identifier"
%token CODE_BLOCK		"a code block"
%token LITERAL			"literal string"
%token K_PTR			"`->'"
%token K_REG			"word register"
%token K_LREG			"long register"
%token K_FREG			"float register"
%token K_IMM			"immediate value"
%token K_IMMZERO		"immediate zero value"
%token K_IMMS8			"immediate signed 8-bit value"
%token K_IMMU8			"immediate unsigned 8-bit value"
%token K_IMMS16			"immediate signed 16-bit value"
%token K_IMMU16			"immediate unsigned 16-bit value"
%token K_LOCAL			"local variable"
%token K_BINARY			"`binary'"
%token K_UNARY			"`unary'"
%token K_UNARY_BRANCH		"`unary_branch'"
%token K_BINARY_BRANCH		"`binary_branch'"
%token K_UNARY_NOTE		"`unary_note'"
%token K_BINARY_NOTE		"`binary_note'"
%token K_TERNARY		"`ternary'"
%token K_STACK			"`stack'"
%token K_ONLY			"`only'"
%token K_COPY			"`copy'"
%token K_X87ARITH		"`x87arith'"
%token K_COMMUTATIVE		"`commutative'"
%token K_REVERSIBLE		"`reversible'"
%token K_IF			"`if'"
%token K_CLOBBER		"`clobber'"
%token K_SCRATCH		"`scratch'"
%token K_SPACE			"`space'"
%token K_INST_TYPE		"`%inst_type'"

/* deperecated keywords */

%token K_MANUAL			"`manual'"
%token K_MORE_SPACE		"`more_space'"
%token K_SPILL_BEFORE		"`spill_before'"

/*
 * Define the yylval types of the various non-terminals.
 */
%type <code>			CODE_BLOCK
%type <name>			IDENTIFIER LITERAL
%type <name>			IfClause IdentifierList Literal
%type <tag>			OptionTag InputTag RegTag LRegTag
%type <clauses>			Clauses Clause
%type <options>			Options OptionList Pattern Pattern2
%type <option>			Option PatternElement Scratch Clobber If Space
%type <values>			ValuePair ValueList
%type <value>			Value

%expect 0

%error-verbose

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
			gensel_output_clauses($5.head, $4.head);
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
	: IDENTIFIER			{ $$ = $1; }
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
	: /* empty */			{ $$ = 0; }
	| '(' IDENTIFIER ')'		{ $$ = $2; }
	;

Options
	: /* empty */			{ $$.head = $$.tail = 0; }
	|  OptionList			{ $$ = $1; }
	;

OptionList
	: Option			{
			$$.head = $1;
			$$.tail = $1;
		}
	| OptionList ',' Option		{
			$1.tail->next = $3;
			$$.head = $1.head;
			$$.tail = $3;
		}
	;

Option
	: OptionTag			{ $$ = gensel_create_option($1, 0); }
	;

Clauses
	: Clause			{ $$ = $1; }
	| Clauses Clause		{
			$1.tail->next = $2.head;
			$$.head = $1.head;
			$$.tail = $2.tail;
		}
	;

Clause
	: '[' Pattern ']' K_PTR CODE_BLOCK	{
			gensel_clause_t clause;
			clause = (gensel_clause_t)malloc(sizeof(struct gensel_clause));
			if(!clause)
			{
				exit(1);
			}
			clause->pattern = $2.head;
			clause->filename = $5.filename;
			clause->linenum = $5.linenum;
			clause->code = $5.block;
			clause->next = 0;
			$$.head = clause;
			$$.tail = clause;
		}
	;

Pattern
	: /* empty */			{ $$.head = $$.tail = 0; }
	| Pattern2			{
			$$.head = $1.head;
			$$.tail = $1.tail;
		}
	;

Pattern2
	: PatternElement		{
			$$.head = $1;
			$$.tail = $1;
		}
	| Pattern2 ',' PatternElement	{
			$1.tail->next = $3;
			$$.head = $1.head;
			$$.tail = $3;
		}
	;

PatternElement
	: InputTag			{ $$ = gensel_create_option($1, 0); }
	| RegTag '(' Value ')'		{ $$ = gensel_create_option($1, $3); }
	| LRegTag '(' Value ')'		{ $$ = gensel_create_option($1, $3); }
	| LRegTag '(' ValuePair ')'	{ $$ = gensel_create_option($1, $3.head); }
	| Scratch
	| Clobber
	| If
	| Space
	;

Scratch
	: K_SCRATCH '(' ValueList ')'	{
			$$ = gensel_create_option(GENSEL_PATT_SCRATCH, $3.head);
		}
	;

Clobber
	: K_CLOBBER '(' ValueList ')'	{
			$$ = gensel_create_option(GENSEL_PATT_CLOBBER, $3.head);
		}
	;
	
If
	: K_IF '(' Value ')'		{
			$$ = gensel_create_option(GENSEL_PATT_IF, $3);
		}
	;

Space
	: K_SPACE '(' Value ')'		{
			$$ = gensel_create_option(GENSEL_PATT_SPACE, $3);
		}
	;

ValuePair
	: Value	':' Value		{
			$1->next = $3;
			$$.head = $1;
			$$.tail = $3;
		}
	;

ValueList
	: Value				{
			$$.head = $1;
			$$.tail = $1;
		}
	| ValueList ',' Value		{
			$1.tail->next = $3;
			$$.head = $1.head;
			$$.tail = $3;
		}
	;

Value
	: Literal			{
			gensel_value_t vp;
			vp = (gensel_value_t) malloc(sizeof(struct gensel_value));
			if(!vp)
			{
				exit(1);
			}
			vp->value = $1;
			vp->next = 0;
			$$ = vp;
		}
	;

OptionTag
	: K_BINARY			{ $$ = GENSEL_OPT_BINARY; }
	| K_UNARY			{ $$ = GENSEL_OPT_UNARY; }
	| K_UNARY_BRANCH		{ $$ = GENSEL_OPT_UNARY_BRANCH; }
	| K_BINARY_BRANCH		{ $$ = GENSEL_OPT_BINARY_BRANCH; }
	| K_UNARY_NOTE			{ $$ = GENSEL_OPT_UNARY_NOTE; }
	| K_BINARY_NOTE			{ $$ = GENSEL_OPT_BINARY_NOTE; }
	| K_TERNARY			{ $$ = GENSEL_OPT_TERNARY; }
	| K_STACK			{ $$ = GENSEL_OPT_STACK; }
	| K_ONLY			{ $$ = GENSEL_OPT_ONLY; }
	| K_COPY			{ $$ = GENSEL_OPT_COPY; }
	| K_X87ARITH			{ $$ = GENSEL_OPT_X87ARITH; }
	| K_COMMUTATIVE			{ $$ = GENSEL_OPT_COMMUTATIVE; }
	| K_REVERSIBLE			{ $$ = GENSEL_OPT_REVERSIBLE; }

	/* deprecated: */
	| K_MANUAL			{ $$ = GENSEL_OPT_MANUAL; }
	| K_MORE_SPACE			{ $$ = GENSEL_OPT_MORE_SPACE; }
	| K_SPILL_BEFORE		{ $$ = GENSEL_OPT_SPILL_BEFORE; }
	;
	
InputTag
	: K_REG				{ $$ = GENSEL_PATT_REG; }
	| K_LREG			{ $$ = GENSEL_PATT_LREG; }
	| K_FREG			{ $$ = GENSEL_PATT_FREG; }
	| K_IMM				{ $$ = GENSEL_PATT_IMM; }
	| K_IMMZERO			{ $$ = GENSEL_PATT_IMMZERO; }
	| K_IMMS8			{ $$ = GENSEL_PATT_IMMS8; }
	| K_IMMU8			{ $$ = GENSEL_PATT_IMMU8; }
	| K_IMMS16			{ $$ = GENSEL_PATT_IMMS16; }
	| K_IMMU16			{ $$ = GENSEL_PATT_IMMU16; }
	| K_LOCAL			{ $$ = GENSEL_PATT_LOCAL; }
	;

RegTag
	: K_REG				{ $$ = GENSEL_PATT_REG; }
	| K_FREG			{ $$ = GENSEL_PATT_FREG; }
	;

LRegTag
	: K_LREG			{ $$ = GENSEL_PATT_LREG; }
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

#define	COPYRIGHT_MSG	\
" * Copyright (C) 2004  Southern Storm Software, Pty Ltd.\n" \
" *\n" \
" * This program is free software; you can redistribute it and/or modify\n" \
" * it under the terms of the GNU General Public License as published by\n" \
" * the Free Software Foundation; either version 2 of the License, or\n" \
" * (at your option) any later version.\n" \
" *\n" \
" * This program is distributed in the hope that it will be useful,\n" \
" * but WITHOUT ANY WARRANTY; without even the implied warranty of\n" \
" * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n" \
" * GNU General Public License for more details.\n" \
" *\n" \
" * You should have received a copy of the GNU General Public License\n" \
" * along with this program; if not, write to the Free Software\n" \
" * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n"
 
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
