%{
/*
 * gen-rules-parser.y - Bison grammar for the "gen-rules" program.
 *
 * Copyright (C) 2004, 2006-2007  Southern Storm Software, Pty Ltd.
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
 * Current file and line number.
 */
extern char *gensel_filename;
extern long gensel_linenum;

/*
 * Report error message.
 */
static void
gensel_error_message(const char *filename, long linenum, const char *msg)
{
	fprintf(stderr, "%s(%ld): %s\n", filename, linenum, msg);
}

/*
 * Report error message and exit.
 */
static void
gensel_error(const char *filename, long linenum, const char *msg)
{
	gensel_error_message(filename, linenum, msg);
	exit(1);
}

/*
 * Report error messages from the parser.
 */
static void
yyerror(void *result, const char *msg)
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
 * Maximal number of input values in a pattern.
 */
#define MAX_INPUT				3

/*
 * Maximal number of scratch registers in a pattern.
 */
#define MAX_SCRATCH				6

/*
 * Maximal number of pattern elements.
 */
#define MAX_PATTERN				(MAX_INPUT + MAX_SCRATCH)

/*
 * Rule Options.
 */
#define	GENSEL_OPT_TERNARY			1
#define GENSEL_OPT_BRANCH			2
#define GENSEL_OPT_NOTE				3
#define	GENSEL_OPT_COPY				4
#define GENSEL_OPT_COMMUTATIVE			5
#define	GENSEL_OPT_STACK			6
#define GENSEL_OPT_X87_ARITH			7
#define GENSEL_OPT_X87_ARITH_REVERSIBLE		8

#define	GENSEL_OPT_MANUAL			9
#define	GENSEL_OPT_MORE_SPACE			10

/*
 * Pattern values.
 */
#define GENSEL_PATT_ANY				1
#define	GENSEL_PATT_REG				2
#define	GENSEL_PATT_LREG			3
#define	GENSEL_PATT_IMM				4
#define	GENSEL_PATT_IMMZERO			5
#define	GENSEL_PATT_IMMS8			6
#define	GENSEL_PATT_IMMU8			7
#define	GENSEL_PATT_IMMS16			8
#define	GENSEL_PATT_IMMU16			9
#define	GENSEL_PATT_IMMS32			10
#define	GENSEL_PATT_IMMU32			11
#define	GENSEL_PATT_LOCAL			12
#define	GENSEL_PATT_FRAME			13
#define	GENSEL_PATT_SCRATCH			14
#define	GENSEL_PATT_CLOBBER			15
#define	GENSEL_PATT_IF				16
#define	GENSEL_PATT_SPACE			17

/*
 * Value types.
 */
#define GENSEL_VALUE_STRING			1
#define GENSEL_VALUE_REGCLASS			2
#define	GENSEL_VALUE_ALL			3
#define GENSEL_VALUE_CLOBBER			4
#define GENSEL_VALUE_EARLY_CLOBBER		5

/*
 * Register class.
 */
typedef struct gensel_regclass *gensel_regclass_t;
struct gensel_regclass
{
	char			*id;
	char			*def;
	int			is_long;
	gensel_regclass_t	next;
};

/*
 * Option value.
 */
typedef struct gensel_value *gensel_value_t;
struct gensel_value
{
	int			type;
	void			*value;
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
	int			dest;
	gensel_option_t		pattern;
	char			*filename;
	long			linenum;
	char			*code;
	gensel_clause_t		next;
};

/*
 * Information about opcodes.
 */
typedef struct gensel_rule *gensel_rule_t;
struct gensel_rule
{
	char			*name;
	char			*condition;
	gensel_option_t		options;
	gensel_clause_t		clauses;
	gensel_rule_t		next;
};

static char *gensel_args[] = {
	"dest", "value1", "value2"
};
static char *gensel_imm_args[] = {
	"insn->dest->address", "insn->value1->address", "insn->value2->address"
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
static char *gensel_local_names[] = {
	"local_offset", "local_offset2", "local_offset3"
};

/*
 * Register classes.
 */
static int gensel_regclass_count = 0;
static gensel_regclass_t gensel_regclass_list;

/*
 * Create a register class.
 */
static void
gensel_create_regclass(char *id, char *def, int is_long)
{
	gensel_regclass_t rp;

	rp = (gensel_regclass_t) malloc(sizeof(struct gensel_regclass));
	if(!rp)
	{
		exit(1);
	}
	
	rp->id = id;
	rp->def = def;
	rp->is_long = is_long;
	rp->next = gensel_regclass_list;

	gensel_regclass_list = rp;
	++gensel_regclass_count;
}

gensel_regclass_t
gensel_lookup_regclass(char *id)
{
	gensel_regclass_t rp;

	rp = gensel_regclass_list;
	for(;;)
	{
		if(!rp)
		{
			gensel_error(
				gensel_filename, gensel_linenum,
				"invalid register class");
		}
		if(strcmp(id, rp->id) == 0)
		{
			return rp;
		}
		rp = rp->next;
	}
}

static int
gensel_regclass_get_index(gensel_regclass_t rp)
{
	int i;
	gensel_regclass_t curr;

	curr = gensel_regclass_list;
	for(i = 0; curr; curr = curr->next)
	{
		if(curr == rp)
		{
			return i;
		}

		i++;
	}

	return -1;
}

/*
 * Create a value.
 */
static gensel_value_t
gensel_create_value(int type)
{
	gensel_value_t vp;

	vp = (gensel_value_t) malloc(sizeof(struct gensel_value));
	if(!vp)
	{
		exit(1);
	}

	vp->type = type;
	vp->value = 0;
	vp->next = 0;

	return vp;
}

/*
 * Create literal string value.
 */
static gensel_value_t
gensel_create_string_value(char *value)
{
	gensel_value_t vp;

	vp = gensel_create_value(GENSEL_VALUE_STRING);
	vp->value = value;

	return vp;
}

/*
 * Create register class value.
 */
static gensel_value_t
gensel_create_regclass_value(char *value)
{
	gensel_value_t vp;

	vp = gensel_create_value(GENSEL_VALUE_REGCLASS);
	vp->value = gensel_lookup_regclass(value);

	return vp;
}

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
 * Create a register pattern element.
 */
static gensel_option_t
gensel_create_register(
	int flags,
	gensel_value_t value,
	gensel_value_t values)
{
	gensel_regclass_t regclass;

	if(flags)
	{
		value->next = gensel_create_value(flags);
		value->next->next = values;
	}
	else
	{
		value->next = values;
	}

	regclass = value->value;
	return gensel_create_option(
		regclass->is_long ? GENSEL_PATT_LREG : GENSEL_PATT_REG,
		value);
}

/*
 * Create a scratch register pattern element.
 */
static gensel_option_t
gensel_create_scratch(
	gensel_value_t regclass,
	gensel_value_t values)
{
	regclass->next = values;
	return gensel_create_option(GENSEL_PATT_SCRATCH, regclass);
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
		if(values->type == GENSEL_VALUE_STRING)
		{
			free(values->value);
		}
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
 * Free a list of rules.
 */
static void
gensel_free_rules(gensel_rule_t rules)
{
	gensel_rule_t next;
	while(rules != 0)
	{
		next = rules->next;
		gensel_free_options(rules->options);
		gensel_free_clauses(rules->clauses);
		free(rules->name);
		free(rules->condition);
		free(rules);
		rules = next;
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
static void
gensel_declare_regs(gensel_clause_t clauses, gensel_option_t options)
{
	gensel_option_t pattern;
	int regs, max_regs;
	int other_regs_mask;
	int imms, max_imms;
	int locals, max_locals;
	int scratch, others;

	max_regs = 0;
	other_regs_mask = 0;
	max_imms = 0;
	max_locals = 0;
	while(clauses != 0)
	{
		regs = 0;
		imms = 0;
		locals = 0;
		others = 0;
		scratch = 0;
		pattern = clauses->pattern;
		while(pattern)
		{
			switch(pattern->option)
			{
			case GENSEL_PATT_ANY:
				++others;
				break;

			case GENSEL_PATT_REG:
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
			case GENSEL_PATT_IMMS32:
			case GENSEL_PATT_IMMU32:
				++imms;
				break;

			case GENSEL_PATT_LOCAL:
			case GENSEL_PATT_FRAME:
				++locals;
				break;

			case GENSEL_PATT_SCRATCH:
				++scratch;
			}
			pattern = pattern->next;
		}
		if((regs + imms + locals + others) > MAX_INPUT)
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
		if(max_regs < (regs + scratch))
		{
			max_regs = regs + scratch;
		}
		if(max_imms < imms)
		{
			max_imms = imms;
		}
		if(max_locals < locals)
		{
			max_locals = locals;
		}
		clauses = clauses->next;
	}
	if(max_regs > 0)
	{
		printf("\tint reg");
		for(scratch = 1; scratch < max_regs; scratch++)
		{
			printf(", reg%d", scratch + 1);
		}
		printf(";\n");
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
			printf("\tint other_reg, other_reg3;\n");
			break;
		case 6:
			printf("\tint other_reg2, other_reg3;\n");
			break;
		case 7:
			printf("\tint other_reg, other_reg2, other_reg3;\n");
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
	switch(max_locals)
	{
	case 1:
		printf("\tjit_nint local_offset;\n");
		break;
	case 2:
		printf("\tjit_nint local_offset, local_offset2;\n");
		break;
	case 3:
		printf("\tjit_nint local_offset, local_offset2, local_offset3;\n");
		break;
	}
}

/*
 * Check if the pattern contains any registers.
 */
static int
gensel_contains_registers(gensel_option_t pattern)
{
	while(pattern)
	{
		switch(pattern->option)
		{
		case GENSEL_PATT_REG:
		case GENSEL_PATT_LREG:
		case GENSEL_PATT_SCRATCH:
		case GENSEL_PATT_CLOBBER:
			return 1;
		}
		pattern = pattern->next;
	}
	return 0;
}

/*
 * Returns first register in the pattern if any.
 */
static gensel_option_t
gensel_get_first_register(gensel_option_t pattern)
{
	while(pattern)
	{
		switch(pattern->option)
		{
		case GENSEL_PATT_REG:
		case GENSEL_PATT_LREG:
			return pattern;
		}
		pattern = pattern->next;
	}
	return 0;
}

static void
gensel_init_names(int count, char *names[], char *other_names[])
{
	int index;
	for(index = 0; index < count; index++)
	{
		if(names)
		{
			names[index] = "undefined";
		}
		if(other_names)
		{
			other_names[index] = "undefined";
		}
	}

}

static void
gensel_build_arg_index(
	gensel_option_t pattern,
	int count,
	char *names[],
	char *other_names[],
	int ternary,
	int free_dest)
{
	int index;

	gensel_init_names(count, names, other_names);

	index = 0;
	while(pattern)
	{
		switch(pattern->option)
		{
		case GENSEL_PATT_ANY:
			++index;
			break;

		case GENSEL_PATT_REG:
		case GENSEL_PATT_LREG:
		case GENSEL_PATT_LOCAL:
		case GENSEL_PATT_FRAME:
		case GENSEL_PATT_IMMZERO:
		case GENSEL_PATT_IMM:
		case GENSEL_PATT_IMMS8:
		case GENSEL_PATT_IMMU8:
		case GENSEL_PATT_IMMS16:
		case GENSEL_PATT_IMMU16:
		case GENSEL_PATT_IMMS32:
		case GENSEL_PATT_IMMU32:
			if(ternary || free_dest)
			{
				if(index < 3)
				{
					names[index] = gensel_args[index];
				}
			}
			else
			{
				if(index < 2)
				{
					names[index] = gensel_args[index + 1];
				}
			}
			++index;
			break;
		}
		pattern = pattern->next;
	}
}

static void
gensel_build_imm_arg_index(
	gensel_option_t pattern,
	int count,
	char *names[],
	char *other_names[],
	int ternary,
	int free_dest)
{
	int index;

	gensel_init_names(count, names, other_names);

	index = 0;
	while(pattern)
	{
		switch(pattern->option)
		{
		case GENSEL_PATT_ANY:
		case GENSEL_PATT_REG:
		case GENSEL_PATT_LREG:
		case GENSEL_PATT_LOCAL:
		case GENSEL_PATT_FRAME:
		case GENSEL_PATT_IMMZERO:
			++index;
			break;

		case GENSEL_PATT_IMM:
		case GENSEL_PATT_IMMS8:
		case GENSEL_PATT_IMMU8:
		case GENSEL_PATT_IMMS16:
		case GENSEL_PATT_IMMU16:
		case GENSEL_PATT_IMMS32:
		case GENSEL_PATT_IMMU32:
			if(ternary || free_dest)
			{
				if(index < 3)
				{
					names[index] = gensel_imm_args[index];
				}
			}
			else
			{
				if(index < 2)
				{
					names[index] = gensel_imm_args[index + 1];
				}
			}
			++index;
			break;
		}
		pattern = pattern->next;
	}
}

/*
 * Build index of input value names.
 */
static void
gensel_build_var_index(
	gensel_option_t pattern,
	char *names[MAX_PATTERN],
	char *other_names[MAX_PATTERN])
{
	int regs, imms, locals, index;

	gensel_init_names(MAX_PATTERN, names, other_names);

	regs = 0;
	imms = 0;
	locals = 0;
	index = 0;
	while(pattern)
	{
		switch(pattern->option)
		{
		case GENSEL_PATT_ANY:
			++index;
			break;

		case GENSEL_PATT_REG:
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
		case GENSEL_PATT_IMMS32:
		case GENSEL_PATT_IMMU32:
			names[index] = gensel_imm_names[imms];
			++imms;
			++index;
			break;

		case GENSEL_PATT_LOCAL:
		case GENSEL_PATT_FRAME:
			names[index] = gensel_local_names[locals];
			++locals;
			++index;
			break;

		case GENSEL_PATT_SCRATCH:
			names[index] = gensel_reg_names[regs];
			++regs;
			++index;
		}
		pattern = pattern->next;
	}
}

/*
 * Output the code.
 */
static void
gensel_output_code(
	gensel_option_t pattern,
	char *code,
	char *names[MAX_PATTERN],
	char *other_names[MAX_PATTERN],
	int free_dest,
	int in_line)
{
	char first;
	int index;
	
	/* Output the clause code */
	if(!in_line)
	{
		printf("\t\t");
	}
	while(*code != '\0')
	{
		first = '1';
		if(*code == '$' && code[1] >= first && code[1] < (first + MAX_PATTERN))
		{
			index = code[1] - first;
			printf("%s", names[index]);
			code += 2;
		}
		else if(*code == '%' && code[1] >= first && code[1] < (first + MAX_PATTERN))
		{
			index = code[1] - first;
			printf("%s", other_names[index]);
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
	if(!in_line)
	{
		printf("\n");
	}
}

/*
 * Output the code within a clause.
 */
static void
gensel_output_clause_code(
	gensel_clause_t clause,
	char *names[MAX_PATTERN],
	char *other_names[MAX_PATTERN],
	int free_dest)
{
	/* Output the line number information from the original file */
#if 0
	printf("#line %ld \"%s\"\n", clause->linenum, clause->filename);
#endif

	gensel_output_code(clause->pattern, clause->code, names, other_names, free_dest, 0);
}

static void
gensel_output_register(char *name, gensel_regclass_t regclass, gensel_value_t values)
{
	printf("\t\t_jit_regs_init_%s(&regs, insn, ", name);
	switch(values ? values->type : 0)
	{
	case GENSEL_VALUE_CLOBBER:
		printf("_JIT_REGS_CLOBBER");
		values = values->next;
		break;
	case GENSEL_VALUE_EARLY_CLOBBER:
		printf("_JIT_REGS_EARLY_CLOBBER");
		values = values->next;
		break;
	default:
		printf("0");
		break;
	}
	printf(", %s);\n", regclass->def);
	if(values && values->value)
	{
		char *reg;
		reg = values->value;
		if(values->next && values->next->value)
		{
			char *other_reg;
			other_reg = values->next->value;
			printf("\t\t_jit_regs_set_%s(gen, &regs, _jit_regs_lookup(\"%s\"), _jit_regs_lookup(\"%s\"));\n",
			       name, reg, other_reg);
		}
		else
		{
			printf("\t\t_jit_regs_set_%s(gen, &regs, _jit_regs_lookup(\"%s\"), -1);\n",
			       name, reg);
		}
	}
}

/*
 * Output value initialization code.
 */
static void
gensel_output_register_pattern(char *name, gensel_option_t pattern)
{
	gensel_output_register(name, pattern->values->value, pattern->values->next);
}

/*
 * Create an upper-case copy of a string.
 */
static char *
gensel_string_upper(char *string)
{
	char *cp;
	if(string)
	{
		string = strdup(string);
		for(cp = string; *cp; cp++)
		{
			*cp = toupper(*cp);
		}
	}
	return string;
}

/*
 * Output the clauses for a rule.
 */
static void gensel_output_clauses(gensel_clause_t clauses, gensel_option_t options)
{
	char *name;
	char *args[MAX_INPUT];
	char *names[MAX_PATTERN];
	char *other_names[MAX_PATTERN];
	gensel_clause_t clause;
	gensel_option_t pattern;
	gensel_option_t space, more_space;
	gensel_value_t values;
	int regs, imms, locals, scratch, index;
	int first, seen_option;
	int ternary, free_dest;
	int contains_registers;
	gensel_regclass_t regclass;
	char *uc_arg;

	/* If the clause is manual, then output it as-is */
	if(gensel_search_option(options, GENSEL_OPT_MANUAL))
	{
		gensel_init_names(MAX_PATTERN, names, other_names);
		gensel_output_clause_code(clauses, names, other_names, 0);
		return;
	}

	clause = clauses;
	contains_registers = 0;
	while(clause)
	{
		contains_registers = gensel_contains_registers(clause->pattern);
		if(contains_registers)
		{
			break;
		}
		clause = clause->next;
	}

	printf("\t%s inst;\n", gensel_inst_type);
	if(contains_registers)
	{
		printf("\t_jit_regs_t regs;\n");
	}
	gensel_declare_regs(clauses, options);

	ternary = (0 != gensel_search_option(options, GENSEL_OPT_TERNARY));

	/* Output the clause checking and dispatching code */
	clause = clauses;
	first = 1;
	while(clause)
	{
		contains_registers = gensel_contains_registers(clause->pattern);
		free_dest = clause->dest;

		gensel_build_arg_index(clause->pattern, 3, args, 0, ternary, free_dest);

		if(clause->next)
		{
			if(first)
				printf("\tif(");
			else
				printf("\telse if(");

			index = 0;
			seen_option = 0;
			pattern = clause->pattern;
			while(pattern)
			{
				switch(pattern->option)
				{
				case GENSEL_PATT_ANY:
					++index;
					break;

				case GENSEL_PATT_REG:
				case GENSEL_PATT_LREG:
					/* Do not check if the value is in
					   a register as the allocator will
					   load them anyway as long as other
					   conditions are met. */
#if 0
					if(seen_option)
					{
						printf(" && ");
					}
					printf("insn->%s->in_register", args[index]);
#endif
					++index;
					break;

				case GENSEL_PATT_IMM:
					if(seen_option)
					{
						printf(" && ");
					}
					printf("insn->%s->is_constant", args[index]);
					seen_option = 1;
					++index;
					break;

				case GENSEL_PATT_IMMZERO:
					if(seen_option)
					{
						printf(" && ");
					}
					printf("insn->%s->is_nint_constant && ", args[index]);
					printf("insn->%s->address == 0", args[index]);
					seen_option = 1;
					++index;
					break;

				case GENSEL_PATT_IMMS8:
					if(seen_option)
					{
						printf(" && ");
					}
					printf("insn->%s->is_nint_constant && ", args[index]);
					printf("insn->%s->address >= -128 && ", args[index]);
					printf("insn->%s->address <= 127", args[index]);
					seen_option = 1;
					++index;
					break;

				case GENSEL_PATT_IMMU8:
					if(seen_option)
					{
						printf(" && ");
					}
					printf("insn->%s->is_nint_constant && ", args[index]);
					printf("insn->%s->address >= 0 && ", args[index]);
					printf("insn->%s->address <= 255", args[index]);
					seen_option = 1;
					++index;
					break;

				case GENSEL_PATT_IMMS16:
					if(seen_option)
					{
						printf(" && ");
					}
					printf("insn->%s->is_nint_constant && ", args[index]);
					printf("insn->%s->address >= -32768 && ", args[index]);
					printf("insn->%s->address <= 32767", args[index]);
					seen_option = 1;
					++index;
					break;

				case GENSEL_PATT_IMMU16:
					if(seen_option)
					{
						printf(" && ");
					}
					printf("insn->%s->is_nint_constant && ", args[index]);
					printf("insn->%s->address >= 0 && ", args[index]);
					printf("insn->%s->address <= 65535", args[index]);
					seen_option = 1;
					++index;
					break;

				case GENSEL_PATT_IMMS32:
					if(seen_option)
					{
						printf(" && ");
					}
					printf("insn->%s->is_nint_constant && ", args[index]);
					printf("insn->%s->address >= -2147483648 && ", args[index]);
					printf("insn->%s->address <= 2147483647", args[index]);
					seen_option = 1;
					++index;
					break;

				case GENSEL_PATT_IMMU32:
					if(seen_option)
					{
						printf(" && ");
					}
					printf("insn->%s->is_nint_constant && ", args[index]);
					printf("insn->%s->address >= 0 && ", args[index]);
					printf("insn->%s->address <= 4294967295", args[index]);
					seen_option = 1;
					++index;
					break;

				case GENSEL_PATT_LOCAL:
					if(seen_option)
					{
						printf(" && ");
					}
					printf("!insn->%s->is_constant && ", args[index]);
					printf("!insn->%s->in_register && ", args[index]);
					printf("!insn->%s->has_global_register && ", args[index]);
					/* If the value is used again in the same basic block
					   it is highly likely that using a register instead
					   of the stack will be a win. Assume that if the
					   "local" pattern is not the last one then it must
					   be followed by a "reg" pattern. */
					uc_arg = gensel_string_upper(args[index]);
					printf("(insn->flags & JIT_INSN_%s_NEXT_USE) == 0", uc_arg);
					free(uc_arg);
					seen_option = 1;
					++index;
					break;

				case GENSEL_PATT_FRAME:
					if(seen_option)
					{
						printf(" && ");
					}
					printf("!insn->%s->is_constant && ", args[index]);
					printf("!insn->%s->has_global_register", args[index]);
					seen_option = 1;
					++index;
					break;

				case GENSEL_PATT_IF:
					if(seen_option)
					{
						printf(" && ");
					}
					printf("(");
					gensel_build_imm_arg_index(
						clause->pattern, MAX_PATTERN,
						names, other_names, ternary, free_dest);
					gensel_output_code(
						clause->pattern,
						pattern->values->value,
						names, other_names, free_dest, 1);
					printf(")");
					seen_option = 1;
					break;
				}
				pattern = pattern->next;
			}
			if(!seen_option)
			{
				printf("1");
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

		if(contains_registers)
		{
			seen_option = 0;
			printf("\t\t_jit_regs_init(gen, &regs, ");
			if(ternary)
			{
				seen_option = 1;
				printf("_JIT_REGS_TERNARY");
			}
			else if(free_dest)
			{
				seen_option = 1;
				printf("_JIT_REGS_FREE_DEST");
			}
			if(gensel_search_option(options, GENSEL_OPT_BRANCH))
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
			/* x87 options */
			if(gensel_search_option(options, GENSEL_OPT_X87_ARITH))
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
			else if(gensel_search_option(options, GENSEL_OPT_X87_ARITH_REVERSIBLE))
			{
				if(seen_option)
				{
					printf(" | ");
				}
				else
				{
					seen_option = 1;
				}
				printf("_JIT_REGS_X87_ARITH | _JIT_REGS_REVERSIBLE");
			}
			if(!seen_option)
			{
				printf("0");
			}
			printf(");\n");

			if(!(ternary || free_dest
			     || gensel_search_option(options, GENSEL_OPT_NOTE)
			     || gensel_search_option(options, GENSEL_OPT_BRANCH)))
			{
				pattern = gensel_get_first_register(clause->pattern);
				gensel_output_register("dest", pattern ? pattern->values->value : 0, 0);
			}
		}

		regs = 0;
		index = 0;
		scratch = 0;
		pattern = clause->pattern;
		while(pattern)
		{
			switch(pattern->option)
			{
			case GENSEL_PATT_ANY:
				++index;
				break;

			case GENSEL_PATT_REG:
			case GENSEL_PATT_LREG:
				gensel_output_register_pattern(args[index], pattern);
				++regs;
				++index;
				break;

			case GENSEL_PATT_IMMZERO:
			case GENSEL_PATT_IMM:
			case GENSEL_PATT_IMMS8:
			case GENSEL_PATT_IMMU8:
			case GENSEL_PATT_IMMS16:
			case GENSEL_PATT_IMMU16:
			case GENSEL_PATT_IMMS32:
			case GENSEL_PATT_IMMU32:
				++index;
				break;

			case GENSEL_PATT_LOCAL:
				printf("\t\t_jit_gen_fix_value(insn->%s);\n", args[index]);
				++index;
				break;

			case GENSEL_PATT_FRAME:
				printf("\t\t_jit_regs_force_out(gen, insn->%s, %d);\n",
				       args[index], (free_dest && index == 0));
				printf("\t\t_jit_gen_fix_value(insn->%s);\n", args[index]);
				++index;
				break;

			case GENSEL_PATT_SCRATCH:
				regclass = pattern->values->value;
				printf("\t\t_jit_regs_add_scratch(&regs, %s);\n",
				       regclass->def);
				if(pattern->values->next && pattern->values->next->value)
				{
					name = pattern->values->next->value;
					printf("\t\t_jit_regs_set_scratch(gen, &regs, %d, _jit_regs_lookup(\"%s\"));\n",
					       scratch, name);
				}
				++regs;
				++scratch;
				++index;
				break;

			case GENSEL_PATT_CLOBBER:
				values = pattern->values;
				while(values)
				{
					if(!values->value)
					{
						continue;
					}
					switch(values->type)
					{
					case GENSEL_VALUE_STRING:
						name = values->value;
						printf("\t\t_jit_regs_clobber(&regs, _jit_regs_lookup(\"%s\"));\n",
						       name);
						break;

					case GENSEL_VALUE_REGCLASS:
						regclass = values->value;
						printf("\t\t_jit_regs_clobber_class(gen, &regs, %s);\n",
							regclass->def);
						break;

					case GENSEL_VALUE_ALL:
						printf("\t\t_jit_regs_clobber_all(gen, &regs);\n");
						break;
					}
					values = values->next;
				}
				break;
			}
			pattern = pattern->next;
		}

		if(gensel_search_option(options, GENSEL_OPT_BRANCH))
		{
			/* Spill all other registers back to their original positions */
			if(contains_registers)
			{
				printf("\t\t_jit_regs_clobber_all(gen, &regs);\n");
			}
			else
			{
				printf("\t\t_jit_regs_spill_all(gen);\n");
			}
		}

		if(gensel_new_inst_type)
		{
			if(contains_registers)
			{
				printf("\t\t_jit_regs_assign(gen, &regs);\n");
				printf("\t\t_jit_regs_gen(gen, &regs);\n");
			}
			printf("\t\tjit_gen_load_inst_ptr(gen, inst);\n");
		}
		else
		{
			space = gensel_search_option(clause->pattern, GENSEL_PATT_SPACE);
			more_space = gensel_search_option(options, GENSEL_OPT_MORE_SPACE);

			if(contains_registers)
			{
				printf("\t\t_jit_regs_begin(gen, &regs, insn, ");
			}
			else
			{
				printf("\t\t_jit_gen_check_space(gen, ");
			}
			if(space && space->values && space->values->value)
			{
				printf("(");
				gensel_build_imm_arg_index(
					clause->pattern, MAX_PATTERN,
					names, other_names, ternary, free_dest);
				gensel_output_code(
					clause->pattern,
					space->values->value,
					names, other_names, free_dest, 1);
				printf(")");
			}
			else
			{
				printf("%d", ((more_space == 0)
					      ? gensel_reserve_space
					      : gensel_reserve_more_space));
			}
			printf(");\n");
		}

		printf("\t\tinst = (%s)(gen->ptr);\n", gensel_inst_type);

		regs = 0;
		imms = 0;
		locals = 0;
		index = 0;
		scratch = 0;
		pattern = clause->pattern;
		while(pattern)
		{
			switch(pattern->option)
			{
			case GENSEL_PATT_ANY:
				++index;
				break;

			case GENSEL_PATT_REG:
				printf("\t\t%s = _jit_reg_info[_jit_regs_get_%s(&regs)].cpu_reg;\n",
				       gensel_reg_names[regs], args[index]);
				++regs;
				++index;
				break;

			case GENSEL_PATT_LREG:
				printf("\t\t%s = _jit_reg_info[_jit_regs_get_%s(&regs)].cpu_reg;\n",
				       gensel_reg_names[regs], args[index]);
				printf("\t\t%s = _jit_reg_info[_jit_regs_get_%s_other(&regs)].cpu_reg;\n",
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
			case GENSEL_PATT_IMMS32:
			case GENSEL_PATT_IMMU32:
				printf("\t\t%s = insn->%s->address;\n",
				       gensel_imm_names[imms], args[index]);
				++imms;
				++index;
				break;

			case GENSEL_PATT_LOCAL:
			case GENSEL_PATT_FRAME:
				printf("\t\t%s = insn->%s->frame_offset;\n",
				       gensel_local_names[locals], args[index]);
				++locals;
				++index;
				break;

			case GENSEL_PATT_SCRATCH:
				printf("\t\t%s = _jit_reg_info[_jit_regs_get_scratch(&regs, %d)].cpu_reg;\n",
				       gensel_reg_names[regs], scratch);
				++regs;
				++scratch;
				++index;
				break;

			}
			pattern = pattern->next;
		}

		gensel_build_var_index(clause->pattern, names, other_names);
		gensel_output_clause_code(clause, names, other_names, free_dest);

		/* Copy "inst" back into the generation context */
		if(gensel_new_inst_type)
		{
			printf("\t\tjit_gen_save_inst_ptr(gen, inst);\n");
		}
		else
		{
			printf("\t\tgen->ptr = (unsigned char *)inst;\n");
		}
		if(contains_registers)
		{
			printf("\t\t_jit_regs_commit(gen, &regs, insn);\n");
		}

		printf("\t}\n");

		first = 0;
		clause = clause->next;
	}
}

/*
 * Generate output for each rule
 */
static void gensel_output_rules(gensel_rule_t rule)
{
	while(rule)
	{
		if(rule->condition)
		{
			if(rule->condition[0] == '!')
			{
				printf("#ifndef %s\n\n", rule->condition + 1);
			}
			else
			{
				printf("#ifdef %s\n\n", rule->condition);
			}
		}

		printf("case %s:\n{\n", rule->name);
		gensel_output_clauses(rule->clauses, rule->options);
		printf("}\nbreak;\n\n");

		if(rule->condition)
		{
			printf("#endif /* %s */\n\n", rule->condition);
		}

		rule = rule->next;
	}
}

/*
 * Output register usage information for one rule.
 */
static void
gensel_output_register_usage_for_rule(gensel_clause_t clause, gensel_option_t options)
{
	char *args[MAX_INPUT];
	char *names[MAX_PATTERN];
	char *other_names[MAX_PATTERN];
	char *arg;
	gensel_option_t pattern;
	gensel_value_t values;
	gensel_regclass_t regclass;
	int is_first;
	int seen_option;
	int index;
	int ternary;
	int free_dest;
	int regclass_index;
	int clobbered_classes;
	int index_start;
	int previous_had_local_pattern;
	int pattern_types[3];
	int can_be_in_mem[3];
	unsigned *unnamed_counts;
	unsigned has_local;

	is_first = 1;
	ternary = (0 != gensel_search_option(options, GENSEL_OPT_TERNARY));
	unnamed_counts = calloc(gensel_regclass_count, sizeof(unsigned));

	previous_had_local_pattern = 0;
	can_be_in_mem[0] = 0;
	can_be_in_mem[1] = 0;
	can_be_in_mem[2] = 0;
	pattern_types[0] = -1;
	pattern_types[1] = -1;
	pattern_types[2] = -1;

	for(; clause; clause = clause->next)
	{
		free_dest = clause->dest;
		if(ternary || free_dest)
		{
			index_start = 0;
		}
		else
		{
			index_start = 1;
		}

		index = 0;
		has_local = 0;
		for(pattern = clause->pattern; pattern; pattern = pattern->next)
		{
			if(!previous_had_local_pattern && index_start + index < 3)
			{
				pattern_types[index_start + index] = pattern->option;
			}

			switch(pattern->option)
			{
			case GENSEL_PATT_LOCAL:
			case GENSEL_PATT_FRAME:
				if(index_start + index < 3)
				{
					can_be_in_mem[index_start + index] = 1;
				}
				has_local = 1;
				++index;
				break;

			case GENSEL_PATT_ANY:
			case GENSEL_PATT_REG:
			case GENSEL_PATT_LREG:
			case GENSEL_PATT_IMM:
			case GENSEL_PATT_IMMS8:
			case GENSEL_PATT_IMMU8:
			case GENSEL_PATT_IMMS16:
			case GENSEL_PATT_IMMU16:
			case GENSEL_PATT_IMMS32:
			case GENSEL_PATT_IMMU32:
				++index;
				break;
			}
		}

		if(has_local && clause->next)
		{
			previous_had_local_pattern = 1;
		}
		else
		{
			if(is_first && !clause->next)
			{
				/* We only have one clause, no need for checks */
				printf("\t/*");
			}
			else if(is_first)
			{
				printf("\tif(");
			}
			else if(clause->next)
			{
				printf("\telse if(");
			}
			else
			{
				printf("\telse /*");
			}

			gensel_build_arg_index(clause->pattern, 3, args, 0,
				ternary, free_dest);

			index = 0;
			seen_option = 0;
			for(pattern = clause->pattern; pattern; pattern = pattern->next)
			{
				if(previous_had_local_pattern && index_start + index < 3
					&& pattern_types[index_start + index] != pattern->option
					&& pattern_types[index_start + index] != GENSEL_PATT_LOCAL)
				{
					previous_had_local_pattern = 0;
					can_be_in_mem[0] = 0;
					can_be_in_mem[1] = 0;
					can_be_in_mem[2] = 0;
				}

				switch(pattern->option)
				{
				case GENSEL_PATT_ANY:
				case GENSEL_PATT_REG:
				case GENSEL_PATT_LREG:
					++index;
					break;

				case GENSEL_PATT_IMM:
					if(seen_option)
					{
						printf(" && ");
					}
					printf("insn->%s->is_constant", args[index]);
					seen_option = 1;
					++index;
					break;

				case GENSEL_PATT_IMMZERO:
					if(seen_option)
					{
						printf(" && ");
					}
					printf("insn->%s->is_nint_constant && ", args[index]);
					printf("insn->%s->address == 0", args[index]);
					seen_option = 1;
					++index;
					break;

				case GENSEL_PATT_IMMS8:
					if(seen_option)
					{
						printf(" && ");
					}
					printf("insn->%s->is_nint_constant && ", args[index]);
					printf("insn->%s->address >= -128 && ", args[index]);
					printf("insn->%s->address <= 127", args[index]);
					seen_option = 1;
					++index;
					break;

				case GENSEL_PATT_IMMU8:
					if(seen_option)
					{
						printf(" && ");
					}
					printf("insn->%s->is_nint_constant && ", args[index]);
					printf("insn->%s->address >= 0 && ", args[index]);
					printf("insn->%s->address <= 255", args[index]);
					seen_option = 1;
					++index;
					break;

				case GENSEL_PATT_IMMS16:
					if(seen_option)
					{
						printf(" && ");
					}
					printf("insn->%s->is_nint_constant && ", args[index]);
					printf("insn->%s->address >= -32768 && ", args[index]);
					printf("insn->%s->address <= 32767", args[index]);
					seen_option = 1;
					++index;
					break;

				case GENSEL_PATT_IMMU16:
					if(seen_option)
					{
						printf(" && ");
					}
					printf("insn->%s->is_nint_constant && ", args[index]);
					printf("insn->%s->address >= 0 && ", args[index]);
					printf("insn->%s->address <= 65535", args[index]);
					seen_option = 1;
					++index;
					break;

				case GENSEL_PATT_IMMS32:
					if(seen_option)
					{
						printf(" && ");
					}
					printf("insn->%s->is_nint_constant && ", args[index]);
					printf("insn->%s->address >= -2147483648 && ", args[index]);
					printf("insn->%s->address <= 2147483647", args[index]);
					seen_option = 1;
					++index;
					break;

				case GENSEL_PATT_IMMU32:
					if(seen_option)
					{
						printf(" && ");
					}
					printf("insn->%s->is_nint_constant && ", args[index]);
					printf("insn->%s->address >= 0 && ", args[index]);
					printf("insn->%s->address <= 4294967295", args[index]);
					seen_option = 1;
					++index;
					break;

				case GENSEL_PATT_IF:
					if(seen_option)
					{
						printf(" && ");
					}
					printf("(");
					gensel_build_imm_arg_index(
						clause->pattern, MAX_PATTERN,
						names, other_names, ternary, free_dest);
					gensel_output_code(
						clause->pattern, pattern->values->value,
						names, other_names, free_dest, 1);
					printf(")");
					seen_option = 1;
					break;
				}
			}

			if(clause->next)
			{
				printf(")\n\t{\n");
			}
			else
			{
				printf("*/\n\t{\n");
			}

			index = 0;
			clobbered_classes = 0;
			for(pattern = clause->pattern; pattern; pattern = pattern->next)
			{
				switch(pattern->option)
				{
				case GENSEL_PATT_IMM:
				case GENSEL_PATT_IMMS8:
				case GENSEL_PATT_IMMU8:
				case GENSEL_PATT_IMMS16:
				case GENSEL_PATT_IMMU16:
				case GENSEL_PATT_IMMS32:
				case GENSEL_PATT_IMMU32:
					++index;
					break;

				case GENSEL_PATT_REG:
				case GENSEL_PATT_LREG:
					regclass = pattern->values->value;
					values = pattern->values->next;
					if(values && values->value)
					{
						if(values->type == GENSEL_VALUE_EARLY_CLOBBER)
						{
							printf("\t\tregmap->early_clobber |= (1 << _jit_regs_lookup(\"%s\"))",
								(char *)values->value);

							if(values->next && values->next->value)
							{
								printf("| (1 << _jit_regs_lookup(\"%s\"));\n",
									(char *)values->next->value);
							}
							else
							{
								printf(";\n");
							}
						}
						else if(values->type == GENSEL_VALUE_CLOBBER)
						{
							printf("\t\tregmap->clobber |= (1 << _jit_regs_lookup(\"%s\"))",
								(char *)values->value);

							if(values->next && values->next->value)
							{
								printf("| (1 << _jit_regs_lookup(\"%s\"));\n",
									(char *)values->next->value);
							}
							else
							{
								printf(";\n");
							}
						}

						printf("\t\tregmap->%s = _jit_regs_lookup(\"%s\");\n",
							args[index], (char *)values->value);
					}
					else
					{
						regclass_index = gensel_regclass_get_index(regclass);

						printf("\t\tregmap->%s = _JIT_REG_USAGE_UNNAMED;\n",
							args[index]);

						if(regclass->is_long)
						{
							printf("\t\tregmap->%s_other = _JIT_REG_USAGE_UNNAMED;\n",
								args[index]);
						}
					}

					++index;
					break;

				case GENSEL_PATT_SCRATCH:
					regclass = pattern->values->value;
					values = pattern->values->next;
					if(values && values->value)
					{
						if(values->type == GENSEL_VALUE_EARLY_CLOBBER)
						{
							printf("\t\tregmap->early_clobber |= (1 << _jit_regs_lookup(\"%s\"))",
								(char *)values->value);
						}
						else
						{
							printf("\t\tregmap->clobber |= (1 << _jit_regs_lookup(\"%s\"))",
								(char *)values->value);
						}

						if(values->next && values->next->value)
						{
							printf("| (1 << _jit_regs_lookup(\"%s\"))",
								(char *)values->next->value);
						}

						printf(";\n");
					}
					else
					{
						regclass_index = gensel_regclass_get_index(regclass);
						++unnamed_counts[regclass_index];

						if(regclass->is_long)
						{
							++unnamed_counts[regclass_index];
						}
					}
					break;

				case GENSEL_PATT_CLOBBER:
					values = pattern->values;
					while(values)
					{
						if(!values->value)
						{
							continue;
						}
						switch(values->type)
						{
						case GENSEL_VALUE_STRING:
							printf("\t\tregmap->clobber |= 1 << _jit_regs_lookup(\"%s\");\n",
								(char *)values->value);
							break;

						case GENSEL_VALUE_REGCLASS:
							clobbered_classes |= 1 << gensel_regclass_get_index(values->value);
							break;

						case GENSEL_VALUE_ALL:
							clobbered_classes = ~0;
							break;
						}
						values = values->next;
					}
					break;
				}
			}

			for(index = 0; index < gensel_regclass_count; index++)
			{
				if(unnamed_counts[index] != 0)
				{
					printf("\t\tregmap->unnamed[%d] = %d;\n",
						index, unnamed_counts[index]);

					unnamed_counts[index] = 0;
				}
			}

			if(clobbered_classes != 0)
			{
				printf("\t\tregmap->clobbered_classes = 0x%x;\n", clobbered_classes);
			}

			printf("\t\tregmap->flags = ");
			is_first = 1;

			for(index = 0; index < 3; index++)
			{
				if(can_be_in_mem[index])
				{
					if(is_first)
					{
						is_first = 0;
					}
					else
					{
						printf(" | ");
					}

					arg = gensel_string_upper(gensel_args[index]);
					printf("JIT_INSN_%s_CAN_BE_MEM", arg);
					free(arg);

					can_be_in_mem[index] = 0;
				}
			}
			previous_had_local_pattern = 0;

			if(gensel_search_option(options, GENSEL_OPT_COMMUTATIVE))
			{
				if(is_first)
				{
					is_first = 0;
				}
				else
				{
					printf(" | ");
				}
				printf("JIT_INSN_COMMUTATIVE");
			}
			else if(!ternary && !free_dest)
			{
				if(is_first)
				{
					is_first = 0;
				}
				else
				{
					printf(" | ");
				}
				printf("JIT_INSN_DEST_INTERFERES_VALUE2");
			}


			if(is_first)
			{
				printf("0");
			}
			printf(";\n");

			is_first = 0;

			printf("\t}\n");
		}
	}

	free(unnamed_counts);
}

/*
 * Output register usage information for all rules.
 */
static void
gensel_output_register_usage(gensel_rule_t rule)
{
	while(rule)
	{
		if(rule->condition)
		{
			if(rule->condition[0] == '!')
			{
				printf("#ifndef %s\n\n", rule->condition + 1);
			}
			else
			{
				printf("#ifdef %s\n\n", rule->condition);
			}
		}

		printf("case %s:\n{\n", rule->name);
		gensel_output_register_usage_for_rule(rule->clauses, rule->options);
		printf("}\nbreak;\n\n");

		if(rule->condition)
		{
			printf("#endif /* %s */\n\n", rule->condition);
		}

		rule = rule->next;
	}
}

/*
 * Output the list of supported opcodes.
 */
static void gensel_output_supported(gensel_rule_t rule)
{
	while(rule)
	{
		if(rule->condition)
		{
			if(rule->condition[0] == '!')
			{
				printf("#ifndef %s\n\n", rule->condition + 1);
			}
			else
			{
				printf("#ifdef %s\n\n", rule->condition);
			}
		}

		printf("case %s:\n", rule->name);

		if(rule->condition)
		{
			printf("#endif /* %s */\n\n", rule->condition);
		}

		rule = rule->next;
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
	struct gensel_rule	*rule;
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
	struct
	{
		struct gensel_rule	*head;
		struct gensel_rule	*tail;
	}	rules;
}

/*
 * Primitive lexical tokens and keywords.
 */
%token IDENTIFIER		"an identifier"
%token CODE_BLOCK		"a code block"
%token LITERAL			"literal string"
%token K_PTR			"`->'"
%token K_ANY			"any value"
%token K_ALL			"all registers"
%token K_IMM			"immediate value"
%token K_IMMZERO		"immediate zero value"
%token K_IMMS8			"immediate signed 8-bit value"
%token K_IMMU8			"immediate unsigned 8-bit value"
%token K_IMMS16			"immediate signed 16-bit value"
%token K_IMMU16			"immediate unsigned 16-bit value"
%token K_IMMS32			"immediate signed 32-bit value"
%token K_IMMU32			"immediate unsigned 32-bit value"
%token K_LOCAL			"local variable"
%token K_FRAME			"local variable forced out into the stack frame"
%token K_NOTE			"`note'"
%token K_TERNARY		"`ternary'"
%token K_BRANCH			"`branch'"
%token K_COPY			"`copy'"
%token K_COMMUTATIVE		"`commutative'"
%token K_IF			"`if'"
%token K_CLOBBER		"`clobber'"
%token K_SCRATCH		"`scratch'"
%token K_SPACE			"`space'"
%token K_STACK			"`stack'"
%token K_X87_ARITH		"`x87_arith'"
%token K_X87_ARITH_REVERSIBLE	"`x87_arith_reversible'"
%token K_INST_TYPE		"`%inst_type'"
%token K_REG_CLASS		"`%reg_class'"
%token K_LREG_CLASS		"`%lreg_class'"

/* deperecated keywords */

%token K_MANUAL			"`manual'"
%token K_MORE_SPACE		"`more_space'"

/*
 * Define the yylval types of the various non-terminals.
 */
%type <code>			CODE_BLOCK
%type <name>			IDENTIFIER LITERAL
%type <name>			IfClause IdentifierList Literal
%type <tag>			OptionTag InputTag DestFlag RegFlag
%type <clauses>			Clauses Clause
%type <options>			Options Pattern
%type <option>			Option PatternElement
%type <values>			ValuePair ClobberList ClobberSpec
%type <value>			Value ClobberEntry RegClass
%type <rules>			Rules
%type <rule>			Rule

%parse-param {struct gensel_rule **result_out}

%expect 0

%error-verbose

%start Start
%%

Start
	: /* empty */		{ *result_out = 0; }
	| Rules			{ *result_out = $1.head; }
	;

Rules
	: Rule			{
			$$.head = $1;
			$$.tail = $1;
		}
	| Rules Rule		{
			if($1.head == 0)
			{
				$$.head = $2;
				$$.tail = $2;
			}
			else
			{
				$1.tail->next = $2;
				$$.head = $1.head;
				$$.tail = $2;
			}
		}
	;

Rule
	: IdentifierList IfClause ':' Options Clauses {
			gensel_rule_t rule;
			rule = malloc(sizeof(*rule));
			if(!rule)
			{
				exit(1);
			}

			rule->name = $1;
			rule->condition = $2;
			rule->options = $4.head;
			rule->clauses = $5.head;

			$$ = rule;
		}
	| K_INST_TYPE IDENTIFIER	{
			gensel_inst_type = $2;
			gensel_new_inst_type = 1;

			$$ = 0;
		}
	| K_REG_CLASS IDENTIFIER IDENTIFIER {
			gensel_create_regclass($2, $3, 0);
			$$ = 0;
		}
	| K_LREG_CLASS IDENTIFIER IDENTIFIER {
			gensel_create_regclass($2, $3, 1);
			$$ = 0;
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
	: /* empty */			{
			$$.head = 0;
			$$.tail = 0;
		}
	| Option			{
			$$.head = $1;
			$$.tail = $1;
		}
	| Options ',' Option		{
			$1.tail->next = $3;
			$$.head = $1.head;
			$$.tail = $3;
		}
	;

Option
	: OptionTag			{
			$$ = gensel_create_option($1, 0);
		}
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
	: '[' DestFlag Pattern ']' K_PTR CODE_BLOCK {
			gensel_clause_t clause;
			clause = (gensel_clause_t)malloc(sizeof(struct gensel_clause));
			if(!clause)
			{
				exit(1);
			}
			clause->dest = $2;
			clause->pattern = $3.head;
			clause->filename = $6.filename;
			clause->linenum = $6.linenum;
			clause->code = $6.block;
			clause->next = 0;
			$$.head = clause;
			$$.tail = clause;
		}
	;

Pattern
	: /* empty */			{
			$$.head = 0;
			$$.tail = 0;
		}
	| PatternElement		{
			$$.head = $1;
			$$.tail = $1;
		}
	| Pattern ',' PatternElement	{
			$1.tail->next = $3;
			$$.head = $1.head;
			$$.tail = $3;
		}
	;
	
PatternElement
	: InputTag			{
			$$ = gensel_create_option($1, 0);
		}
	| RegFlag RegClass		{
			$$ = gensel_create_register($1, $2, 0);
		}
	| RegFlag RegClass '(' Value ')'	{
			$$ = gensel_create_register($1, $2, $4);
		}
	| RegFlag RegClass '(' ValuePair ')'	{
			gensel_regclass_t regclass;
			regclass = $2->value;
			if(!regclass->is_long)
			{
				gensel_error(
					gensel_filename, gensel_linenum,
					"not a long pair register");
			}
			$$ = gensel_create_register($1, $2, $4.head);
		}
	| K_SCRATCH RegClass {
			gensel_regclass_t regclass;
			regclass = $2->value;
			if(regclass->is_long)
			{
				gensel_error(
					gensel_filename, gensel_linenum,
					"scratch register cannot be a long pair");
			}
			$$ = gensel_create_scratch($2, 0);
		}
	| K_SCRATCH RegClass '(' Value ')' {
			gensel_regclass_t regclass;
			regclass = $2->value;
			if(regclass->is_long)
			{
				gensel_error(
					gensel_filename, gensel_linenum,
					"scratch register cannot be a long pair");
			}
			$$ = gensel_create_scratch($2, $4);
		}
	| K_CLOBBER '(' ClobberSpec ')'	{
			$$ = gensel_create_option(GENSEL_PATT_CLOBBER, $3.head);
		}
	| K_IF '(' Value ')'		{
			$$ = gensel_create_option(GENSEL_PATT_IF, $3);
		}
	| K_SPACE '(' Value ')'		{
			$$ = gensel_create_option(GENSEL_PATT_SPACE, $3);
		}
	;

ClobberSpec
	: K_ALL				{
			$$.head = $$.tail = gensel_create_value(GENSEL_VALUE_ALL);
		}
	| ClobberList			{
			$$ = $1;
		}
	;

ClobberList
	: ClobberEntry			{
			$$.head = $1;
			$$.tail = $1;
		}
	| ClobberList ',' ClobberEntry	{
			$1.tail->next = $3;
			$$.head = $1.head;
			$$.tail = $3;
		}
	;

ClobberEntry
	: RegClass			{ $$ = $1; }
	| Value				{ $$ = $1; }
	;

RegClass
	: IDENTIFIER			{
			$$ = gensel_create_regclass_value($1);
		}
	;

Value
	: Literal			{
			$$ = gensel_create_string_value($1);
		}
	;

ValuePair
	: Value	':' Value		{
			$1->next = $3;
			$$.head = $1;
			$$.tail = $3;
		}
	;

OptionTag
	: K_TERNARY			{ $$ = GENSEL_OPT_TERNARY; }
	| K_BRANCH			{ $$ = GENSEL_OPT_BRANCH; }
	| K_NOTE			{ $$ = GENSEL_OPT_NOTE; }
	| K_COPY			{ $$ = GENSEL_OPT_COPY; }
	| K_COMMUTATIVE			{ $$ = GENSEL_OPT_COMMUTATIVE; }
	| K_STACK			{ $$ = GENSEL_OPT_STACK; }
	| K_X87_ARITH			{ $$ = GENSEL_OPT_X87_ARITH; }
	| K_X87_ARITH_REVERSIBLE	{ $$ = GENSEL_OPT_X87_ARITH_REVERSIBLE; }

	/* deprecated: */
	| K_MANUAL			{ $$ = GENSEL_OPT_MANUAL; }
	| K_MORE_SPACE			{ $$ = GENSEL_OPT_MORE_SPACE; }
	;

InputTag
	: K_IMM				{ $$ = GENSEL_PATT_IMM; }
	| K_IMMZERO			{ $$ = GENSEL_PATT_IMMZERO; }
	| K_IMMS8			{ $$ = GENSEL_PATT_IMMS8; }
	| K_IMMU8			{ $$ = GENSEL_PATT_IMMU8; }
	| K_IMMS16			{ $$ = GENSEL_PATT_IMMS16; }
	| K_IMMU16			{ $$ = GENSEL_PATT_IMMU16; }
	| K_IMMS32			{ $$ = GENSEL_PATT_IMMS32; }
	| K_IMMU32			{ $$ = GENSEL_PATT_IMMU32; }
	| K_LOCAL			{ $$ = GENSEL_PATT_LOCAL; }
	| K_FRAME			{ $$ = GENSEL_PATT_FRAME; }
	| K_ANY				{ $$ = GENSEL_PATT_ANY; }
	;

DestFlag
	: /* empty */			{ $$ = 0; }
	| '='				{ $$ = 1; }
	;

RegFlag
	: /* empty */			{ $$ = 0; }
	| '*'				{ $$ = GENSEL_VALUE_CLOBBER; }
	| '+'				{ $$ = GENSEL_VALUE_EARLY_CLOBBER; }
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
" * Copyright (C) 2004, 2006-2007  Southern Storm Software, Pty Ltd.\n" \
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
	gensel_rule_t result;
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
	gensel_filename = argv[1];
	gensel_linenum = 1;
	yyrestart(file);
	if(yyparse(&result))
	{
		fclose(file);
		return 1;
	}
	fclose(file);

	printf("/%c Automatically generated from %s - DO NOT EDIT %c/\n",
		   '*', argv[1], '*');
	printf("/%c\n%s%c/\n\n", '*', COPYRIGHT_MSG, '*');

	printf("#if defined(JIT_INCLUDE_RULES)\n\n");
	gensel_output_rules(result);

	printf("#elif defined(JIT_INCLUDE_REGISTER_USAGE)\n\n");
	gensel_output_register_usage(result);

	printf("#elif defined(JIT_INCLUDE_SUPPORTED)\n\n");
	gensel_output_supported(result);

	printf("#endif\n");

	gensel_free_rules(result);
	return 0;
}
