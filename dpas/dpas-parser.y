%{
/*
 * dpas-parser.y - Bison grammar for the Dynamic Pascal language.
 *
 * Copyright (C) 2004  Southern Storm Software, Pty Ltd.
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

#include "dpas-internal.h"
#include <jit/jit-dump.h>
#include <config.h>
#ifdef HAVE_STDLIB_H
	#include <stdlib.h>
#endif
#ifdef HAVE_STDARG_H
	#include <stdarg.h>
#elif HAVE_VARARGS_H
	#include <varargs.h>
#endif

/*
 * Imports from the lexical analyser.
 */
extern int yylex(void);
#ifdef YYTEXT_POINTER
extern char *yytext;
#else
extern char yytext[];
#endif

/*
 * Error reporting flag.
 */
int dpas_error_reported = 0;

/*
 * Report error messages from the parser.
 */
static void yyerror(char *msg)
{
	char *text = yytext;
	char *newmsg;
	int posn, outposn;
	dpas_error_reported = 1;
	if(!jit_strcmp(msg, "parse error") || !jit_strcmp(msg, "syntax error"))
	{
		/* This error is pretty much useless at telling the user
		   what is happening.  Try to print a better message
		   based on the current input token */
	simpleError:
		if(text && *text != '\0')
		{
			fprintf(stderr, "%s:%ld: parse error at or near `%s'\n",
					dpas_filename, dpas_linenum, text);
		}
		else
		{
			fprintf(stderr, "%s:%ld: parse error\n",
					dpas_filename, dpas_linenum);
		}
	}
	else if(!jit_strncmp(msg, "parse error, expecting `", 24))
	{
		/* We have to quote the token names in the "%token" declarations
		   within yacc grammars so that byacc doesn't mess up the output.
		   But the quoting causes Bison to output quote characters in
		   error messages which look awful.  This code attempts to fix
		   things up */
		newmsg = jit_strdup(msg);
	expectingError:
		if(newmsg)
		{
			posn = 0;
			outposn = 0;
			while(newmsg[posn] != '\0')
			{
				if(newmsg[posn] == '`')
				{
					if(newmsg[posn + 1] == '"' && newmsg[posn + 2] == '`')
					{
						/* Convert <`"`> into <`> */
						posn += 2;
						newmsg[outposn++] = '`';
					}
					else if(newmsg[posn + 1] == '"')
					{
						/* Convert <`"> into <> */
						++posn;
					}
					else if(newmsg[posn + 1] == '`' ||
					        newmsg[posn + 1] == '\'')
					{
						/* Convert <``> or <`'> into <`> */
						++posn;
						newmsg[outposn++] = '`';
					}
					else
					{
						/* Ordinary <`> on its own */
						newmsg[outposn++] = '`';
					}
				}
				else if(newmsg[posn] == '\\')
				{
					/* Ignore backslashes in the input */
				}
				else if(newmsg[posn] == '"' && newmsg[posn + 1] == '\'')
				{
					/* Convert <"'> into <> */
					++posn;
				}
				else if(newmsg[posn] == '\'' && newmsg[posn + 1] == '"' &&
				        newmsg[posn + 2] == '\'')
				{
					/* Convert <'"'> into <'> */
					posn += 2;
					newmsg[outposn++] = '\'';
				}
				else if(newmsg[posn] == '\'' && newmsg[posn + 1] == '\'')
				{
					/* Convert <''> into <'> */
					++posn;
					newmsg[outposn++] = '\'';
				}
				else if(newmsg[posn] == ' ' && newmsg[posn + 1] == '\'')
				{
					/*  bison 1.75 - <'> following a space becomes <`> */
					++posn;
					newmsg[outposn++] = ' ';
					newmsg[outposn++] = '`';
				}
				else if(newmsg[posn] == '"')
				{
					/* Ignore quotes - bison 1.75 */
				}
				else
				{
					/* Ordinary character */
					newmsg[outposn++] = newmsg[posn];
				}
				++posn;
			}
			newmsg[outposn] = '\0';
			if(text && *text != '\0')
			{
				fprintf(stderr, "%s:%ld: %s, at or near `%s'\n",
						dpas_filename, dpas_linenum, newmsg, text);
			}
			else
			{
				fprintf(stderr, "%s:%ld: %s\n",
						dpas_filename, dpas_linenum, newmsg);
			}
			jit_free(newmsg);
		}
		else
		{
			if(text && *text != '\0')
			{
				fprintf(stderr, "%s:%ld: %s at or near `%s'\n",
						dpas_filename, dpas_linenum, msg, text);
			}
			else
			{
				fprintf(stderr, "%s:%ld: %s\n",
						dpas_filename, dpas_linenum, msg);
			}
		}
	}
	else if(!jit_strncmp(msg, "parse error, unexpected ", 24))
	{
		/* The error probably has the form "parse error, unexpected ...,
		   expecting ..." - strip out the "unexpected" part */
		posn = 24;
		while(msg[posn] != '\0' &&
			  jit_strncmp(msg + posn, ", expecting ", 12) != 0)
		{
			++posn;
		}
		if(msg[posn] == '\0')
		{
			goto simpleError;
		}
		newmsg = (char *)jit_malloc(jit_strlen(msg) + 1);
		if(!newmsg)
		{
			goto defaultError;
		}
		jit_strcpy(newmsg, "parse error, expecting ");
		jit_strcat(newmsg, msg + posn + 12);
		goto expectingError;
	}
	else
	{
		/* The parser has probably included information as to what
		   is expected in this context, so report that */
	defaultError:
		if(text && *text != '\0')
		{
			fprintf(stderr, "%s:%ld: %s at or near `%s'\n",
					dpas_filename, dpas_linenum, msg, text);
		}
		else
		{
			fprintf(stderr, "%s:%ld: %s\n",
					dpas_filename, dpas_linenum, msg);
		}
	}
}

void dpas_error(const char *format, ...)
{
	va_list va;
#ifdef HAVE_STDARG_H
	va_start(va, format);
#else
	va_start(va);
#endif
	fprintf(stderr, "%s:%ld: ", dpas_filename, dpas_linenum);
	vfprintf(stderr, format, va);
	putc('\n', stderr);
	va_end(va);
	dpas_error_reported = 1;
}

void dpas_warning(const char *format, ...)
{
	va_list va;
#ifdef HAVE_STDARG_H
	va_start(va, format);
#else
	va_start(va);
#endif
	fprintf(stderr, "%s:%ld: warning: ", dpas_filename, dpas_linenum);
	vfprintf(stderr, format, va);
	putc('\n', stderr);
	va_end(va);
}

void dpas_error_on_line(const char *filename, long linenum,
                        const char *format, ...)
{
	va_list va;
#ifdef HAVE_STDARG_H
	va_start(va, format);
#else
	va_start(va);
#endif
	fprintf(stderr, "%s:%ld: ", filename, linenum);
	vfprintf(stderr, format, va);
	putc('\n', stderr);
	va_end(va);
	dpas_error_reported = 1;
}

static void dpas_undeclared(const char *name)
{
	dpas_error("`%s' is not declared in the current scope", name);
}

static void dpas_redeclared(const char *name, dpas_scope_item_t item)
{
	dpas_error("`%s' is already declared in the current scope", name);
	dpas_error_on_line(dpas_scope_item_filename(item),
					   dpas_scope_item_linenum(item),
					   "previous declaration of `%s' here", name);
}

/*
 * Add an item to a list of identifiers.
 */
static void identifier_list_add(char ***list, int *len, char *name)
{
	char **new_list = (char **)jit_realloc(*list, sizeof(char *) * (*len + 1));
	if(!new_list)
	{
		dpas_out_of_memory();
	}
	new_list[*len] = name;
	++(*len);
	*list = new_list;
}

/*
 * Free the contents of an identifier list.
 */
static void identifier_list_free(char **list, int len)
{
	int posn;
	for(posn = 0; posn < len; ++posn)
	{
		jit_free(list[posn]);
	}
	jit_free(list);
}

/*
 * Determine if an identifier list contains a specific item.
 */
static int identifier_list_contains(char **list, int len, const char *name)
{
	int posn;
	for(posn = 0; posn < len; ++posn)
	{
		if(list[posn] && !jit_stricmp(list[posn], name))
		{
			return 1;
		}
	}
	return 0;
}

/*
 * Add an item to a list of types.
 */
static void type_list_add(jit_type_t **list, int *len, jit_type_t type)
{
	jit_type_t *new_list = (jit_type_t *)
		jit_realloc(*list, sizeof(jit_type_t) * (*len + 1));
	if(!new_list)
	{
		dpas_out_of_memory();
	}
	new_list[*len] = type;
	++(*len);
	*list = new_list;
}

/*
 * Initialize a parameter list.
 */
static void parameter_list_init(dpas_params *list)
{
	list->names = 0;
	list->types = 0;
	list->len = 0;
	list->has_vararg = 0;
}

/*
 * Add an item to a list of parameters.
 */
static void parameter_list_add(dpas_params *list, char *name, jit_type_t type)
{
	char **new_names = (char **)
		jit_realloc(list->names, sizeof(char *) * (list->len + 1));
	jit_type_t *new_types = (jit_type_t *)
		jit_realloc(list->types, sizeof(jit_type_t) * (list->len + 1));
	if(!new_names || !new_types)
	{
		dpas_out_of_memory();
	}
	new_names[list->len] = name;
	new_types[list->len] = type;
	++(list->len);
	list->names = new_names;
	list->types = new_types;
}

/*
 * Free the contents of a parameter list.
 */
static void parameter_list_free(dpas_params *list)
{
	int posn;
	for(posn = 0; posn < list->len; ++posn)
	{
		jit_free(list->names[posn]);
		jit_type_free(list->types[posn]);
	}
	jit_free(list->names);
	jit_free(list->types);
}

/*
 * Determine if a parameter list contains a specific item.
 */
static int parameter_list_contains(dpas_params *list, const char *name)
{
	int posn;
	for(posn = 0; posn < list->len; ++posn)
	{
		if(list->names[posn] && !jit_stricmp(list->names[posn], name))
		{
			return 1;
		}
	}
	return 0;
}

/*
 * Create a new parameter list.
 */
static void parameter_list_create(dpas_params *list, char **names,
								  int num_names, jit_type_t type)
{
	char **temp = names;
	parameter_list_init(list);
	while(num_names > 0)
	{
		parameter_list_add(list, temp[0], jit_type_copy(type));
		--num_names;
		++temp;
	}
	if(names)
	{
		jit_free(names);
	}
}

/*
 * Merge two parameter lists into one.
 */
static void parameter_list_merge(dpas_params *list1, dpas_params *list2)
{
	int posn;
	char *name;
	for(posn = 0; posn < list2->len; ++posn)
	{
		name = list2->names[posn];
		if(name && parameter_list_contains(list1, name))
		{
			dpas_error("`%s' used twice in a parameter or field list", name);
			jit_free(name);
			name = 0;
		}
		parameter_list_add(list1, name, list2->types[posn]);
	}
	jit_free(list2->names);
	jit_free(list2->types);
}

/*
 * Handle a numeric binary operator.
 */
#define	handle_binary(name,func,arg1,arg2)	\
		do { \
			if(!dpas_sem_is_rvalue(arg1) || \
			   !dpas_type_is_numeric(dpas_sem_get_type(arg2)) || \
			   !dpas_sem_is_rvalue(arg1) || \
			   !dpas_type_is_numeric(dpas_sem_get_type(arg2))) \
			{ \
				if(!dpas_sem_is_error(arg1) && !dpas_sem_is_error(arg2)) \
				{ \
					dpas_error("invalid operands to binary `" name "'"); \
				} \
				dpas_sem_set_error(yyval.semvalue); \
			} \
			else \
			{ \
				jit_value_t value; \
				value = func \
					(dpas_current_function(), \
					 dpas_sem_get_value(arg1), \
					 dpas_sem_get_value(arg2)); \
				dpas_sem_set_rvalue \
					(yyval.semvalue, jit_value_get_type(value), value); \
			} \
		} while (0)

/*
 * Handle an integer binary operator.
 */
#define	handle_integer_binary(name,func,arg1,arg2)	\
		do { \
			if(!dpas_sem_is_rvalue(arg1) || \
			   !dpas_type_is_integer(dpas_sem_get_type(arg2)) || \
			   !dpas_sem_is_rvalue(arg1) || \
			   !dpas_type_is_integer(dpas_sem_get_type(arg2))) \
			{ \
				if(!dpas_sem_is_error(arg1) && !dpas_sem_is_error(arg2)) \
				{ \
					dpas_error("invalid operands to binary `" name "'"); \
				} \
				dpas_sem_set_error(yyval.semvalue); \
			} \
			else \
			{ \
				jit_value_t value; \
				value = func \
					(dpas_current_function(), \
					 dpas_sem_get_value(arg1), \
					 dpas_sem_get_value(arg2)); \
				dpas_sem_set_rvalue \
					(yyval.semvalue, jit_value_get_type(value), value); \
			} \
		} while (0)

/*
 * Handle a comparison binary operator.
 */
#define	handle_compare_binary(name,func,arg1,arg2)	\
		do { \
			if(dpas_sem_is_rvalue(arg1) && \
			   jit_type_is_pointer(dpas_sem_get_type(arg1)) && \
			   dpas_sem_is_rvalue(arg2) && \
			   jit_type_is_pointer(dpas_sem_get_type(arg2))) \
			{ \
				jit_value_t value; \
				value = func \
					(dpas_current_function(), \
					 dpas_sem_get_value(arg1), \
					 dpas_sem_get_value(arg2)); \
				dpas_sem_set_rvalue \
					(yyval.semvalue, jit_value_get_type(value), value); \
			} \
			else \
			{ \
				handle_binary(name, func, arg1, arg2); \
			} \
		} while (0)

%}

/*
 * Define the structure of yylval.
 */
%union {
	char		   *name;
	struct
	{
		jit_ulong	value;
		jit_type_t	type;
	}				int_const;
	jit_nfloat		real_const;
	jit_constant_t	const_value;
	jit_type_t		type;
	struct
	{
		char	  **list;
		int			len;
	}				id_list;
	struct
	{
		jit_type_t *list;
		int			len;
	}				type_list;
	dpas_params		parameters;
	struct
	{
		char	   *name;
		jit_type_t	type;
	}				procedure;
	struct
	{
		jit_type_t	type;
		dpas_params	bounds;
	}				param_type;
	dpas_semvalue	semvalue;
}

/*
 * Primitive lexical tokens.
 */
%token IDENTIFIER			"an identifier"
%token INTEGER_CONSTANT		"an integer value"
%token STRING_CONSTANT		"a string literal"
%token REAL_CONSTANT		"a floating point value"

/*
 * Keywords.
 */
%token K_AND				"`and'"
%token K_ARRAY				"`array'"
%token K_BEGIN				"`begin'"
%token K_CASE				"`case'"
%token K_CATCH				"`catch'"
%token K_CONST				"`const'"
%token K_DIV				"`div'"
%token K_DO					"`do'"
%token K_DOWNTO				"`downto'"
%token K_ELSE				"`else'"
%token K_END				"`end'"
%token K_EXIT				"`exit'"
%token K_FINALLY			"`finally'"
%token K_FOR				"`for'"
%token K_FORWARD			"`forward'"
%token K_FUNCTION			"`function'"
%token K_GOTO				"`goto'"
%token K_IF					"`if'"
%token K_IN					"`in'"
%token K_LABEL				"`label'"
%token K_IMPORT				"`import'"
%token K_MOD				"`mod'"
%token K_MODULE				"`module'"
%token K_NIL				"`nil'"
%token K_NOT				"`not'"
%token K_OF					"`of'"
%token K_OR					"`or'"
%token K_PACKED				"`packed'"
%token K_POW				"`pow'"
%token K_PROCEDURE			"`procedure'"
%token K_PROGRAM			"`program'"
%token K_RECORD				"`record'"
%token K_REPEAT				"`repeat'"
%token K_SET				"`set'"
%token K_SHL				"`shl'"
%token K_SHR				"`shr'"
%token K_SIZEOF				"`sizeof'"
%token K_THEN				"`then'"
%token K_THROW				"`throw'"
%token K_TO					"`to'"
%token K_TRY				"`try'"
%token K_TYPE				"`type'"
%token K_UNTIL				"`until'"
%token K_VAR				"`var'"
%token K_VA_ARG				"`va_arg'"
%token K_WITH				"`with'"
%token K_WHILE				"`while'"
%token K_XOR				"`xor'"

/*
 * Operators.
 */
%token K_NE					"`<>'"
%token K_LE					"`<='"
%token K_GE					"`>='"
%token K_ASSIGN				"`:='"
%token K_DOT_DOT			"`..'"

/*
 * Define the yylval types of the various non-terminals.
 */
%type <name>				IDENTIFIER STRING_CONSTANT Identifier Directive
%type <int_const>			INTEGER_CONSTANT
%type <real_const>			REAL_CONSTANT
%type <const_value>			Constant ConstantValue BasicConstant
%type <id_list>				IdentifierList
%type <type_list>			ArrayBoundsList VariantCaseList

%type <type>				Type TypeIdentifier SimpleType StructuredType
%type <type>				BoundType Variant VariantList

%type <param_type>			ParameterType ConformantArray

%type <parameters>			FormalParameterList FormalParameterSection
%type <parameters>			FormalParameterSections FieldList FixedPart
%type <parameters>			RecordSection VariantPart BoundSpecificationList
%type <parameters>			BoundSpecification

%type <procedure>			ProcedureHeading FunctionHeading
%type <procedure>			ProcedureOrFunctionHeading

%type <semvalue>			Variable Expression SimpleExpression
%type <semvalue>			AdditionExpression Term Factor Power

%expect 3

%start Program
%%

/*
 * Programs.
 */

Program
	: ProgramHeading ImportDeclarationPart ProgramBlock '.'
	;

ProgramHeading
	: K_PROGRAM Identifier '(' IdentifierList ')' ';'	{
				jit_free($2);
				identifier_list_free($4.list, $4.len);
			}
	| K_PROGRAM Identifier ';'	{
				jit_free($2);
			}
	| K_MODULE Identifier '(' IdentifierList ')' ';'	{
				/* The "module" keyword is provided as an alternative
				   to "program" because it looks odd to put "program"
				   on code included via an "import" clause */
				jit_free($2);
				identifier_list_free($4.list, $4.len);
			}
	| K_MODULE Identifier ';'	{
				jit_free($2);
			}
	;

ImportDeclarationPart
	: /* empty */
	| K_IMPORT ImportDeclarations ';'
	;

ImportDeclarations
	: Identifier		{
				dpas_import($1);
				jit_free($1);
			}
	| ImportDeclarations ',' Identifier	{
				dpas_import($3);
				jit_free($3);
			}
	;

ProgramBlock
	: ProgramBlock2		{
				/* Get the "main" function for this module */
				jit_function_t func = dpas_current_function();

				/* Make sure that the function is properly terminated */
				if(!jit_insn_default_return(func))
				{
					dpas_out_of_memory();
				}

				/* Compile the function */
				/* TODO */
				jit_dump_function(stdout, func, "main");

				/* Pop the "main" function */
				dpas_pop_function();
			}
	;

ProgramBlock2
	: Block
	| K_END
	;

/*
 * Identifiers and lists of identifiers.
 */

Identifier
	: IDENTIFIER			{ $$ = $1; }
	;

IdentifierList
	: Identifier						{
				$$.list = 0;
				$$.len = 0;
				identifier_list_add(&($$.list), &($$.len), $1);
			}
	| IdentifierList ',' Identifier		{
				$$ = $1;
				if(identifier_list_contains($$.list, $$.len, $3))
				{
					dpas_error("`%s' declared twice in an identifier list", $3);
					identifier_list_add(&($$.list), &($$.len), 0);
					jit_free($3);
				}
				else
				{
					identifier_list_add(&($$.list), &($$.len), $3);
				}
			}
	;

/*
 * Blocks.
 */

Block
	: DeclarationPart StatementPart
	;

DeclarationPart
	: LabelDeclarationPart ConstantDefinitionPart TypeDefinitionPart
	  VariableDeclarationPart ProcedureAndFunctionDeclarationPart
	;

LabelDeclarationPart
	: /* empty */
	| K_LABEL LabelList ';'
	;

LabelList
	: Label
	| LabelList ',' Label
	;

Label
	: INTEGER_CONSTANT			{ /* label declarations are ignored */ }
	;

ConstantDefinitionPart
	: /* empty */
	| K_CONST ConstantDefinitionList
	;

ConstantDefinitionList
	: ConstantDefinition
	| ConstantDefinitionList ConstantDefinition
	;

ConstantDefinition
	: Identifier '=' Constant ';'		{
				dpas_scope_item_t item;
				item = dpas_scope_lookup(dpas_scope_current(), $1, 0);
				if(item)
				{
					dpas_redeclared($1, item);
				}
				else
				{
					dpas_scope_add_const(dpas_scope_current(), $1, &($3),
										 dpas_filename, dpas_linenum);
				}
				jit_free($1);
				jit_type_free($3.type);
			}
	;

TypeDefinitionPart
	: /* empty */
	| K_TYPE TypeDefinitionList		{
				/* Check for undefined record types that were referred
				   to using a pointer reference of the form "^name" */
				dpas_scope_check_undefined(dpas_scope_current());
			}
	;

TypeDefinitionList
	: TypeDefinition
	| TypeDefinitionList TypeDefinition
	;

TypeDefinition
	: Identifier '=' Type ';'			{
				dpas_scope_item_t item;
				jit_type_t type;
				item = dpas_scope_lookup(dpas_scope_current(), $1, 0);
				type = (item ? dpas_scope_item_type(item) : 0);
				if(!item)
				{
					dpas_type_set_name($3, $1);
					dpas_scope_add(dpas_scope_current(), $1, $3,
								   DPAS_ITEM_TYPE, 0, 0,
								   dpas_filename, dpas_linenum);
				}
				else if(dpas_scope_item_kind(item) == DPAS_ITEM_TYPE &&
					    jit_type_get_tagged_kind(type) == DPAS_TAG_NAME &&
					    jit_type_get_tagged_type(type) == 0 &&
						jit_type_get_tagged_kind($3) == DPAS_TAG_NAME)
				{
					/* This is a defintion of a record type that was
					   previously encountered in a forward pointer
					   reference of the form "^name".  We need to
					   back-patch the previous type with the type info */
					jit_type_set_tagged_type
						(type, jit_type_get_tagged_type($3), 1);
				}
				else
				{
					dpas_redeclared($1, item);
				}
				jit_free($1);
				jit_type_free($3);
			}
	;

VariableDeclarationPart
	: /* empty */
	| K_VAR VariableDeclarationList
	;

VariableDeclarationList
	: VariableDeclaration
	| VariableDeclarationList VariableDeclaration
	;

VariableDeclaration
	: IdentifierList ':' Type ';'		{
				/* Add each of the variables to the current scope */
				int posn;
				dpas_scope_item_t item;
				for(posn = 0; posn < $1.len; ++posn)
				{
					item = dpas_scope_lookup(dpas_scope_current(),
										     $1.list[posn], 0);
					if(item)
					{
						dpas_redeclared($1.list[posn], item);
					}
					else
					{
						if(dpas_current_function())
						{
							jit_value_t value;
							value = jit_value_create
								(dpas_current_function(), $3);
							if(!value)
							{
								dpas_out_of_memory();
							}
							dpas_scope_add(dpas_scope_current(),
										   $1.list[posn], $3,
										   DPAS_ITEM_VARIABLE, value, 0,
										   dpas_filename, dpas_linenum);
						}
						else
						{
							/* Allocate some memory to hold the global data */
							void *space = jit_calloc(1, jit_type_get_size($3));
							if(!space)
							{
								dpas_out_of_memory();
							}
							dpas_scope_add(dpas_scope_current(),
										   $1.list[posn], $3,
										   DPAS_ITEM_GLOBAL_VARIABLE, space, 0,
										   dpas_filename, dpas_linenum);
						}
					}
				}

				/* Free the id list and type, which we don't need any more */
				identifier_list_free($1.list, $1.len);
				jit_type_free($3);
			}
	;

ProcedureAndFunctionDeclarationPart
	: /* empty */
	| ProcedureOrFunctionList
	;

ProcedureOrFunctionList
	: ProcedureOrFunctionDeclaration ';'
	| ProcedureOrFunctionList ProcedureOrFunctionDeclaration ';'
	;

StatementPart
	: K_BEGIN StatementSequence OptSemi K_END
	| K_BEGIN OptSemi K_END
	| K_BEGIN error K_END
	;

OptSemi
	: /* empty */
	| ';'
	;

/*
 * Procedure and function declarations.
 */

ProcedureOrFunctionDeclaration
	: ProcedureOrFunctionHeading ';' 	{
				unsigned int num_params;
				unsigned int param;
				jit_type_t type;
				const char *name;
				dpas_scope_item_t item;
				jit_function_t func;

				/* Declare the procedure/function into the current scope */
				item = dpas_scope_lookup(dpas_scope_current(), $1.name, 0);
				if(item)
				{
					dpas_redeclared($1.name, item);
				}
				else
				{
					dpas_scope_add(dpas_scope_current(), $1.name, $1.type,
								   DPAS_ITEM_PROCEDURE, 0, 0,
								   dpas_filename, dpas_linenum);
				}

				/* Push into a new scope for the procedure/function body */
				dpas_scope_push();
				func = dpas_new_function($1.type);

				/* Declare the parameters into the scope.  If a name
				   is NULL, then it indicates that a duplicate parameter
				   name was detected, and replaced with no name */
				num_params = jit_type_num_params($1.type);
				for(param = 0; param < num_params; ++param)
				{
					name = jit_type_get_name($1.type, param);
					if(name)
					{
						jit_value_t value;
						value = jit_value_get_param(func, param);
						if(!value)
						{
							dpas_out_of_memory();
						}
						dpas_scope_add(dpas_scope_current(), name,
									   jit_type_get_param($1.type, param),
									   DPAS_ITEM_VARIABLE, value, 0,
									   dpas_filename, dpas_linenum);
					}
				}

				/* Declare the function return variable into the scope */
				type = jit_type_get_return($1.type);
				if(type != jit_type_void)
				{
					if(dpas_scope_lookup(dpas_scope_current(), $1.name, 0))
					{
						dpas_error("`%s' is declared as both a parameter "
								   "and a function name", $1.name);
					}
					else
					{
						dpas_scope_add(dpas_scope_current(), $1.name, type,
									   DPAS_ITEM_FUNC_RETURN, 0, 0,
									   dpas_filename, dpas_linenum);
					}
				}
			} Body {
				jit_function_t func = dpas_current_function();
				int result;

				/* Make sure that the function is properly terminated */
				result = jit_insn_default_return(func);
				if(!result)
				{
					dpas_out_of_memory();
				}
				else if(result == 1 &&
				        jit_type_get_return($1.type) != jit_type_void)
				{
					dpas_error("control reached the end of a function");
				}

				/* Pop from the procedure/function scope */
				dpas_pop_function();
				dpas_scope_pop();

				/* TODO: compile the procedure/function */

				jit_dump_function(stdout, func, $1.name);

				/* Free values that we no longer require */
				jit_free($1.name);
				jit_type_free($1.type);
			}
	;

ProcedureOrFunctionHeading
	: ProcedureHeading			{ $$ = $1; }
	| FunctionHeading			{ $$ = $1; }
	;

Body
	: Block			{}
	| Directive		{}
	;

Directive
	: K_FORWARD								{ $$ = 0; }
	| K_IMPORT '(' STRING_CONSTANT ')'		{ $$ = $3; }
	;

ProcedureHeading
	: K_PROCEDURE Identifier FormalParameterList	{
				$$.name = $2;
				$$.type = jit_type_create_signature
					(($3.has_vararg ? jit_abi_vararg : jit_abi_cdecl),
					 jit_type_void, $3.types, (unsigned int)($3.len), 1);
				if(!($$.type))
				{
					dpas_out_of_memory();
				}
				if(!jit_type_set_names($$.type, $3.names,
									   (unsigned int)($3.len)))
				{
					dpas_out_of_memory();
				}
				parameter_list_free(&($3));
			}
	;

FunctionHeading
	: K_FUNCTION Identifier FormalParameterList ':' TypeIdentifier	{
				$$.name = $2;
				$$.type = jit_type_create_signature
					(($3.has_vararg ? jit_abi_vararg : jit_abi_cdecl),
					 $5, $3.types, (unsigned int)($3.len), 1);
				if(!($$.type))
				{
					dpas_out_of_memory();
				}
				if(!jit_type_set_names($$.type, $3.names,
									   (unsigned int)($3.len)))
				{
					dpas_out_of_memory();
				}
				parameter_list_free(&($3));
				jit_type_free($5);
			}
	;

FormalParameterList
	: /* empty */		{ parameter_list_init(&($$)); }
	| '(' FormalParameterSections ')'					{ $$ = $2; }
	| '(' FormalParameterSections ';' K_DOT_DOT ')'		{
				$$ = $2;
				$$.has_vararg = 1;
			}
	;

FormalParameterSections
	: FormalParameterSection		{ $$ = $1; }
	| FormalParameterSections ';' FormalParameterSection	{
				$$ = $1;
				parameter_list_merge(&($$), &($3));
			}
	;

FormalParameterSection
	: IdentifierList ':' ParameterType			{
				parameter_list_create(&($$), $1.list, $1.len, $3.type);
				if($3.bounds.len > 0)
				{
					/* We should be using "var" with conformant array types */
					dpas_warning("`%s' should be declared as `var'",
								 $1.list[0]);
					if($1.len > 1)
					{
						dpas_error("too many parameter names for "
								   "conformant array specification");
					}
					parameter_list_merge(&($$), &($3.bounds));
				}
				jit_type_free($3.type);
			}
	| K_VAR IdentifierList ':' ParameterType	{
				jit_type_t type = jit_type_create_pointer($4.type, 0);
				if(!type)
				{
					dpas_out_of_memory();
				}
				if($4.bounds.len == 0)
				{
					type = jit_type_create_tagged(type, DPAS_TAG_VAR, 0, 0, 0);
					if(!type)
					{
						dpas_out_of_memory();
					}
				}
				parameter_list_create(&($$), $2.list, $2.len, type);
				if($4.bounds.len > 0)
				{
					if($2.len > 1)
					{
						dpas_error("too many parameter names for "
								   "conformant array specification");
					}
					parameter_list_merge(&($$), &($4.bounds));
				}
				jit_type_free(type);
			}
	| ProcedureHeading		{
				parameter_list_init(&($$));
				parameter_list_add(&($$), $1.name, $1.type);
			}
	| FunctionHeading		{
				parameter_list_init(&($$));
				parameter_list_add(&($$), $1.name, $1.type);
			}
	;

ParameterType
	: TypeIdentifier			{
				$$.type = $1;
				parameter_list_init(&($$.bounds));
			}
	| ConformantArray			{ $$ = $1; }
	;

ConformantArray
	: K_PACKED K_ARRAY '[' BoundSpecification ']' K_OF TypeIdentifier	{
				$$.type = dpas_create_conformant_array($7, $4.len, 1);
				$$.bounds = $4;
			}
	| K_ARRAY '[' BoundSpecificationList ']' K_OF ParameterType			{
				$$.type = dpas_create_conformant_array($6.type, $3.len, 0);
				$$.bounds = $3;
				parameter_list_merge(&($$.bounds), &($6.bounds));
			}
	;

BoundSpecificationList
	: BoundSpecification		{ $$ = $1; }
	| BoundSpecificationList ';' BoundSpecification	{
				$$ = $1;
				parameter_list_merge(&($$), &($3));
			}
	;

BoundSpecification
	: Identifier K_DOT_DOT Identifier ':' TypeIdentifier	{
				if($5 != jit_type_int)
				{
					char *name = dpas_type_name($5);
					dpas_error("`%s' cannot be used for array bounds; "
							   "must be `Integer'", name);
					jit_free(name);
				}
				jit_type_free($5);
				parameter_list_init(&($$));
				parameter_list_add(&($$), $1, jit_type_int);
				if(jit_strcmp($1, $3) != 0)
				{
					parameter_list_add(&($$), $3, jit_type_int);
				}
				else
				{
					dpas_error("`%s' used twice in a parameter or "
							   "field list", $1);
					parameter_list_add(&($$), 0, jit_type_int);
					jit_free($3);
				}
			}
	;

/*
 * Statements.
 */

StatementSequence
	: Statement
	| StatementSequence ';' Statement
	;

Statement
	: Label ':' InnerStatement
	| InnerStatement
	;

InnerStatement
	: Variable K_ASSIGN Expression			{
				/* TODO: type checking and non-local variables */
				if(dpas_sem_is_lvalue($1) && dpas_sem_is_rvalue($3))
				{
					if(!jit_insn_store(dpas_current_function(),
									   dpas_sem_get_value($1),
									   dpas_sem_get_value($3)))
					{
						dpas_out_of_memory();
					}
				}
				else
				{
					dpas_error("invalid assignment");
				}
			}
	| Identifier							{}
	| Identifier '(' ExpressionList ')'		{}
	| K_GOTO Label
	| CompoundStatement
	| K_WHILE Expression K_DO Statement
	| K_REPEAT StatementSequence OptSemi K_UNTIL Expression
	| K_FOR Identifier K_ASSIGN Expression Direction Expression K_DO Statement
	| K_IF Expression K_THEN Statement
	| K_IF Expression K_THEN Statement K_ELSE Statement
	| CaseStatement
	| K_WITH VariableList K_DO Statement
	| K_THROW Expression
	| K_THROW
	| TryStatement
	| K_EXIT
	;

CompoundStatement
	: K_BEGIN StatementSequence OptSemi K_END
	| K_BEGIN error K_END
	;

Direction
	: K_TO
	| K_DOWNTO
	;

CaseStatement
	: K_CASE Expression K_OF CaseLimbList
	;

CaseLimbList
	: CaseLimb
	| CaseLimbList ';' CaseLimb
	;

CaseLimb
	: CaseLabelList ':' Statement
	;

CaseLabelList
	: Constant						{}
	| CaseLabelList ',' Constant	{}
	;

VariableList
	: Variable						{}
	| VariableList ',' Variable		{}
	;

TryStatement
	: K_TRY StatementSequence OptSemi CatchClause FinallyClause K_END
	;

CatchClause
	: /* empty */
	| K_CATCH Identifier ':' Type StatementSequence OptSemi
	;

FinallyClause
	: /* empty */
	| K_FINALLY StatementSequence OptSemi
	;

/*
 * Expressions.
 */

Expression
	: SimpleExpression							{ $$ = $1; }
	| SimpleExpression '=' SimpleExpression		{
				handle_compare_binary("=", jit_insn_eq, $1, $3);
			}
	| SimpleExpression K_NE SimpleExpression	{
				handle_compare_binary("<>", jit_insn_ne, $1, $3);
			}
	| SimpleExpression '<' SimpleExpression		{
				handle_compare_binary("<", jit_insn_lt, $1, $3);
			}
	| SimpleExpression '>' SimpleExpression		{
				handle_compare_binary(">", jit_insn_gt, $1, $3);
			}
	| SimpleExpression K_LE SimpleExpression	{
				handle_compare_binary("<=", jit_insn_le, $1, $3);
			}
	| SimpleExpression K_GE SimpleExpression	{
				handle_compare_binary(">=", jit_insn_ge, $1, $3);
			}
	| SimpleExpression K_IN SimpleExpression	{
				/* TODO */
			}
	;

ExpressionList
	: Expression						{ /* TODO */ }
	| ExpressionList ',' Expression		{ /* TODO */ }
	;

SimpleExpression
	: AdditionExpression			{ $$ = $1; }
	| '+' AdditionExpression		{
				if(!dpas_sem_is_rvalue($2) ||
				   !dpas_type_is_numeric(dpas_sem_get_type($2)))
				{
					if(!dpas_sem_is_error($2))
					{
						dpas_error("invalid operand to unary `+'");
					}
					dpas_sem_set_error($$);
				}
				else
				{
					$$ = $2;
				}
			}
	| '-' AdditionExpression		{
				if(!dpas_sem_is_rvalue($2) ||
				   !dpas_type_is_numeric(dpas_sem_get_type($2)))
				{
					if(!dpas_sem_is_error($2))
					{
						dpas_error("invalid operand to unary `-'");
					}
					dpas_sem_set_error($$);
				}
				else
				{
					jit_value_t value;
					value = jit_insn_neg
						(dpas_current_function(), dpas_sem_get_value($2));
					dpas_sem_set_rvalue($$, jit_value_get_type(value), value);
				}
			}
	;

AdditionExpression
	: Term							{ $$ = $1; }
	| AdditionExpression '+' Term	{
				handle_binary("+", jit_insn_add, $1, $3);
			}
	| AdditionExpression '-' Term	{
				handle_binary("-", jit_insn_sub, $1, $3);
			}
	| AdditionExpression K_OR Term	{
				if(dpas_sem_is_rvalue($1) &&
				   dpas_type_is_boolean(dpas_sem_get_type($1)) &&
				   dpas_sem_is_rvalue($3) &&
				   dpas_type_is_boolean(dpas_sem_get_type($3)))
				{
					/* Output code to compute a short-circuited "or" */
					jit_label_t label1 = jit_label_undefined;
					jit_label_t label2 = jit_label_undefined;
					jit_value_t value, const_value;
					if(!jit_insn_branch_if
							(dpas_current_function(),
							 dpas_sem_get_value($1), &label1))
					{
						dpas_out_of_memory();
					}
					if(!jit_insn_branch_if
							(dpas_current_function(),
							 dpas_sem_get_value($3), &label1))
					{
						dpas_out_of_memory();
					}
					value = jit_value_create
						(dpas_current_function(), jit_type_int);
					const_value = jit_value_create_nint_constant
						(dpas_current_function(), jit_type_int, 0);
					if(!value || !const_value)
					{
						dpas_out_of_memory();
					}
					if(!jit_insn_store(dpas_current_function(),
									   value, const_value))
					{
						dpas_out_of_memory();
					}
					if(!jit_insn_branch(dpas_current_function(), &label2))
					{
						dpas_out_of_memory();
					}
					if(!jit_insn_label(dpas_current_function(), &label1))
					{
						dpas_out_of_memory();
					}
					const_value = jit_value_create_nint_constant
						(dpas_current_function(), jit_type_int, 1);
					if(!const_value)
					{
						dpas_out_of_memory();
					}
					if(!jit_insn_store(dpas_current_function(),
									   value, const_value))
					{
						dpas_out_of_memory();
					}
					if(!jit_insn_label(dpas_current_function(), &label2))
					{
						dpas_out_of_memory();
					}
					dpas_sem_set_rvalue($$, dpas_type_boolean, value);
				}
				else
				{
					handle_integer_binary("or", jit_insn_or, $1, $3);
				}
			}
	;

Term
	: Power							{ $$ = $1; }
	| Term '*' Power				{
				handle_binary("*", jit_insn_div, $1, $3);
			}
	| Term '/' Power				{
				/* Standard Pascal always returns a floating-point
				   result for '/', but we don't do that here yet */
				handle_binary("/", jit_insn_div, $1, $3);
			}
	| Term K_DIV Power				{
				handle_binary("div", jit_insn_div, $1, $3);
			}
	| Term K_MOD Power				{
				handle_binary("mod", jit_insn_rem, $1, $3);
			}
	| Term K_AND Power				{
				if(dpas_sem_is_rvalue($1) &&
				   dpas_type_is_boolean(dpas_sem_get_type($1)) &&
				   dpas_sem_is_rvalue($3) &&
				   dpas_type_is_boolean(dpas_sem_get_type($3)))
				{
					/* Output code to compute a short-circuited "and" */
					jit_label_t label1 = jit_label_undefined;
					jit_label_t label2 = jit_label_undefined;
					jit_value_t value, const_value;
					if(!jit_insn_branch_if_not
							(dpas_current_function(),
							 dpas_sem_get_value($1), &label1))
					{
						dpas_out_of_memory();
					}
					if(!jit_insn_branch_if_not
							(dpas_current_function(),
							 dpas_sem_get_value($3), &label1))
					{
						dpas_out_of_memory();
					}
					value = jit_value_create
						(dpas_current_function(), jit_type_int);
					const_value = jit_value_create_nint_constant
						(dpas_current_function(), jit_type_int, 1);
					if(!value || !const_value)
					{
						dpas_out_of_memory();
					}
					if(!jit_insn_store(dpas_current_function(),
									   value, const_value))
					{
						dpas_out_of_memory();
					}
					if(!jit_insn_branch(dpas_current_function(), &label2))
					{
						dpas_out_of_memory();
					}
					if(!jit_insn_label(dpas_current_function(), &label1))
					{
						dpas_out_of_memory();
					}
					const_value = jit_value_create_nint_constant
						(dpas_current_function(), jit_type_int, 0);
					if(!const_value)
					{
						dpas_out_of_memory();
					}
					if(!jit_insn_store(dpas_current_function(),
									   value, const_value))
					{
						dpas_out_of_memory();
					}
					if(!jit_insn_label(dpas_current_function(), &label2))
					{
						dpas_out_of_memory();
					}
					dpas_sem_set_rvalue($$, dpas_type_boolean, value);
				}
				else
				{
					handle_integer_binary("and", jit_insn_and, $1, $3);
				}
			}
	| Term K_XOR Power				{
				handle_integer_binary("xor", jit_insn_xor, $1, $3);
			}
	| Term K_SHL Power				{
				handle_integer_binary("shl", jit_insn_shl, $1, $3);
			}
	| Term K_SHR Power				{
				handle_integer_binary("shr", jit_insn_shr, $1, $3);
			}
	;

Power
	: Factor						{ $$ = $1; }
	| Power K_POW Factor			{
				handle_binary("pow", jit_insn_pow, $1, $3);
			}
	;

Factor
	: Variable						{ $$ = $1; }
	| BasicConstant					{
				jit_value_t value = jit_value_create_constant
					(dpas_current_function(), &($1));
				dpas_sem_set_rvalue($$, $1.type, value);
			}
	| '[' ExpressionList ']'		{ /* TODO */ }
	| '[' ']'						{ /* TODO */ }
	| K_NOT Factor					{
				jit_value_t value;
				if(dpas_sem_is_rvalue($2) &&
				   dpas_type_is_boolean(dpas_sem_get_type($2)))
				{
					value = jit_insn_to_not_bool
						(dpas_current_function(), dpas_sem_get_value($2));
					dpas_sem_set_rvalue($$, dpas_type_boolean, value);
				}
				else if(dpas_sem_is_rvalue($2) &&
						dpas_type_is_integer(dpas_sem_get_type($2)))
				{
					value = jit_insn_not
						(dpas_current_function(), dpas_sem_get_value($2));
					dpas_sem_set_rvalue($$, jit_value_get_type(value), value);
				}
				else
				{
					if(!dpas_sem_is_error($2))
					{
						dpas_error("invalid operand to unary `not'");
					}
					dpas_sem_set_error($$);
				}
			}
	| '&' Factor					{
				jit_type_t type;
				if(dpas_sem_is_lvalue($2))
				{
					jit_value_t value;
					value = jit_insn_address_of
						(dpas_current_function(), dpas_sem_get_value($2));
					type = jit_type_create_pointer(dpas_sem_get_type($2), 1);
					if(!type)
					{
						dpas_out_of_memory();
					}
					dpas_sem_set_rvalue($$, type, value);
				}
				else if(dpas_sem_is_lvalue_ea($2))
				{
					/* Turn the effective address into an r-value */
					type = jit_type_create_pointer(dpas_sem_get_type($2), 1);
					if(!type)
					{
						dpas_out_of_memory();
					}
					dpas_sem_set_rvalue($$, type, dpas_sem_get_value($2));
				}
				else
				{
					if(!dpas_sem_is_error($2))
					{
						dpas_error("l-value required for address-of operator");
					}
					dpas_sem_set_error($$);
				}
			}
	| '@' Factor					{
				jit_type_t type;
				if(dpas_sem_is_lvalue($2))
				{
					jit_value_t value;
					value = jit_insn_address_of
						(dpas_current_function(), dpas_sem_get_value($2));
					type = jit_type_create_pointer(dpas_sem_get_type($2), 1);
					if(!type)
					{
						dpas_out_of_memory();
					}
					dpas_sem_set_rvalue($$, jit_value_get_type(value), value);
				}
				else if(dpas_sem_is_lvalue_ea($2))
				{
					/* Turn the effective address into an r-value */
					type = jit_type_create_pointer(dpas_sem_get_type($2), 1);
					if(!type)
					{
						dpas_out_of_memory();
					}
					dpas_sem_set_rvalue($$, type, dpas_sem_get_value($2));
				}
				else
				{
					if(!dpas_sem_is_error($2))
					{
						dpas_error("l-value required for address-of operator");
					}
					dpas_sem_set_error($$);
				}
			}
	| '(' Expression ')'			{ $$ = $2; }
	| Variable '(' ExpressionList ')'		{ /* TODO */ }
	| K_VA_ARG '(' TypeIdentifier ')'		{ /* TODO */ }
	| K_SIZEOF '(' Variable ')'				{ /* TODO */ }
	| '(' K_IF Expression K_THEN Expression K_ELSE Expression ')'	{
				/* TODO */
			}
	;

Variable
	: Identifier						{
				dpas_scope_item_t item;
				item = dpas_scope_lookup(dpas_scope_current(), $1, 1);
				if(!item)
				{
					dpas_error("`%s' is not declared in the current scope", $1);
					dpas_sem_set_error($$);
				}
				else
				{
					switch(dpas_scope_item_kind(item))
					{
						case DPAS_ITEM_TYPE:
						{
							dpas_sem_set_type($$, dpas_scope_item_type(item));
						}
						break;

						case DPAS_ITEM_VARIABLE:
						{
							dpas_sem_set_lvalue
								($$, dpas_scope_item_type(item),
								 (jit_value_t)dpas_scope_item_info(item));
						}
						break;

						case DPAS_ITEM_GLOBAL_VARIABLE:
						{
							jit_value_t value;
							void *address = dpas_scope_item_info(item);
							value = jit_value_create_nint_constant
								(dpas_current_function(),
								 jit_type_void_ptr, (jit_nint)address);
							if(!value)
							{
								dpas_out_of_memory();
							}
							dpas_sem_set_lvalue_ea
								($$, dpas_scope_item_type(item), value);
						}
						break;

						case DPAS_ITEM_CONSTANT:
						{
							jit_value_t const_value;
							const_value = jit_value_create_constant
								(dpas_current_function(),
								 (jit_constant_t *)dpas_scope_item_info(item));
							if(!const_value)
							{
								dpas_out_of_memory();
							}
							dpas_sem_set_rvalue
								($$, dpas_scope_item_type(item), const_value);
						}
						break;

						case DPAS_ITEM_PROCEDURE:
						{
							dpas_sem_set_procedure
								($$, dpas_scope_item_type(item), item);
						}
						break;

						case DPAS_ITEM_WITH:
						{
							/* TODO */
							dpas_sem_set_error($$);
						}
						break;

						case DPAS_ITEM_FUNC_RETURN:
						{
							dpas_sem_set_return
								($$, dpas_scope_item_type(item));
						}
						break;

						default:
						{
							dpas_sem_set_error($$);
						}
						break;
					}
				}
				jit_free($1);
			}
	| Variable '[' ExpressionList ']'	{
				/* TODO */
			}
	| Variable '.' Identifier			{
				/* Fetch the effective address of a structure field */
				/* TODO */
			}
	| Variable '^'						{
				/* Dereference a pointer value */
				jit_value_t value;
				if(!jit_type_is_pointer(dpas_sem_get_type($1)))
				{
					if(!dpas_sem_is_error($1))
					{
						dpas_error("invalid operand to unary `^'");
					}
					dpas_sem_set_error($$);
				}
				else if(dpas_sem_is_lvalue($1) || dpas_sem_is_rvalue($1))
				{
					/* Turn the pointer value into an effective address */
					value = jit_insn_add_relative
						(dpas_current_function(), dpas_sem_get_value($1), 0);
					if(!value)
					{
						dpas_out_of_memory();
					}
					dpas_sem_set_lvalue_ea
						($$, jit_type_get_ref(dpas_sem_get_type($1)), value);
				}
				else if(dpas_sem_is_lvalue_ea($1))
				{
					/* Fetch the pointer value and construct a new adddress */
					value = jit_insn_load_relative
						(dpas_current_function(), dpas_sem_get_value($1),
						 0, jit_type_void_ptr);
					if(!value)
					{
						dpas_out_of_memory();
					}
					dpas_sem_set_lvalue_ea
						($$, jit_type_get_ref(dpas_sem_get_type($1)), value);
				}
				else
				{
					if(!dpas_sem_is_error($1))
					{
						dpas_error("invalid operand to unary `^'");
					}
					dpas_sem_set_error($$);
				}
			}
	;

/*
 * Types.
 */

TypeIdentifier
	: Identifier			{
				dpas_scope_item_t item;
				item = dpas_scope_lookup(dpas_scope_current(), $1, 1);
				if(!item)
				{
					dpas_undeclared($1);
					$$ = jit_type_void;
				}
				else if(dpas_scope_item_kind(item) != DPAS_ITEM_TYPE)
				{
					dpas_error("`%s' does not refer to a type in the "
							   "current scope", $1);
					$$ = jit_type_void;
				}
				else
				{
					$$ = jit_type_copy(dpas_scope_item_type(item));
				}
				jit_free($1);
			}
	;

Type
	: SimpleType				{ $$ = $1; }
	| StructuredType			{ $$ = $1; }
	| K_PACKED StructuredType	{ $$ = $2; }
	;

SimpleType
	: TypeIdentifier				{ $$ = $1; }
	| '(' IdentifierList ')'		{
				jit_type_t type;
				int posn;
				dpas_scope_item_t item;
				jit_constant_t value;

				/* Create the enumerated type */
				type = dpas_create_enum(jit_type_int, $2.len);

				/* Declare all of the identifiers into the current scope
				   as constants whose values correspond to the positions */
				for(posn = 0; posn < $2.len; ++posn)
				{
					if(!($2.list[posn]))
					{
						/* Placeholder for a duplicate identifier.  The error
						   was already reported inside "IdentifierList" */
						continue;
					}
					item = dpas_scope_lookup(dpas_scope_current(),
											 $2.list[posn], 0);
					if(item)
					{
						dpas_redeclared($2.list[posn], item);
						continue;
					}
					value.type = type;
					value.un.int_value = (jit_int)posn;
					dpas_scope_add_const(dpas_scope_current(),
										$2.list[posn], &value,
										dpas_filename, dpas_linenum);
				}

				/* Free the identifier list, which we no longer need */
				identifier_list_free($2.list, $2.len);

				/* Return the type as our semantic value */
				$$ = type;
			}
	| Constant K_DOT_DOT Constant	{
				/* Infer a common type for the subrange */
				jit_type_t type = dpas_common_type($1.type, $3.type, 1);
				if(type)
				{
					/* TODO: check that value1 <= value2 */
					dpas_subrange range;
					dpas_convert_constant(&(range.first), &($1), type);
					dpas_convert_constant(&(range.last), &($3), type);
					$$ = dpas_create_subrange(type, &range);
					jit_type_free($1.type);
					jit_type_free($3.type);
				}
				else
				{
					char *name1 = dpas_type_name($1.type);
					char *name2 = dpas_type_name($3.type);
					if(!jit_strcmp(name1, name2))
					{
						dpas_error("cannot declare a subrange within `%s'",
								   name1);
					}
					else
					{
						dpas_error("cannot declare a subrange within "
								   "`%s' or `%s'", name1, name2);
					}
					jit_free(name1);
					jit_free(name2);
					$$ = $1.type;
					jit_type_free($3.type);
				}
			}
	;

StructuredType
	: K_ARRAY '[' ArrayBoundsList ']' K_OF Type	{
				$$ = dpas_create_array($3.list, $3.len, $6);
			}
	| K_RECORD FieldList K_END		{
				jit_type_t type;
				type = jit_type_create_struct
					($2.types, (unsigned int)($2.len), 1);
				if(!type)
				{
					dpas_out_of_memory();
				}
				if(!jit_type_set_names
						(type, $2.names, (unsigned int)($2.len)))
				{
					dpas_out_of_memory();
				}
				type = jit_type_create_tagged(type, DPAS_TAG_NAME, 0, 0, 0);
				parameter_list_free(&($2));
				$$ = type;
			}
	| K_SET K_OF Type				{
				if(!dpas_is_set_compatible($3))
				{
					char *name = dpas_type_name($3);
					dpas_error("`%s' cannot be used as the member type "
							   "for a set", name);
					jit_free(name);
				}
				$$ = jit_type_create_tagged
						(jit_type_uint, DPAS_TAG_SET,
						 $3, (jit_meta_free_func)jit_type_free, 0);
			}
	| '^' Identifier			{
				dpas_scope_item_t item;
				item = dpas_scope_lookup(dpas_scope_current(), $2, 1);
				if(!item)
				{
					/* The name is not declared yet, so it is probably a
					   forward reference to some later record type.  We need
					   to add a placeholder for the later definition */
					char *name;
					name = jit_strdup($2);
					if(!name)
					{
						dpas_out_of_memory();
					}
					$$ = jit_type_create_tagged(0, DPAS_TAG_NAME,
												name, jit_free, 0);
					if(!($$))
					{
						dpas_out_of_memory();
					}
					dpas_scope_add(dpas_scope_current(), $2, $$,
								   DPAS_ITEM_TYPE, 0, 0,
								   dpas_filename, dpas_linenum);
				}
				else if(dpas_scope_item_kind(item) != DPAS_ITEM_TYPE)
				{
					dpas_error("`%s' does not refer to a type in the "
							   "current scope", $2);
					$$ = jit_type_void;
				}
				else
				{
					$$ = jit_type_copy(dpas_scope_item_type(item));
				}
				$$ = jit_type_create_pointer($$, 0);
				if(!($$))
				{
					dpas_out_of_memory();
				}
				jit_free($2);
			}
	;

ArrayBoundsList
	: BoundType						{
				$$.list = 0;
				$$.len = 0;
				type_list_add(&($$.list), &($$.len), $1);
			}
	| ArrayBoundsList ',' BoundType	{
				$$ = $1;
				type_list_add(&($$.list), &($$.len), $3);
			}
	;

BoundType
	: SimpleType		{
				/* The type must be an enumeration or integer subrange */
				if(jit_type_get_tagged_kind($1) == DPAS_TAG_ENUM)
				{
					$$ = $1;
				}
				else if(jit_type_get_tagged_kind($1) == DPAS_TAG_SUBRANGE &&
						jit_type_get_tagged_type($1) == jit_type_int)
				{
					$$ = $1;
				}
				else
				{
					char *name = dpas_type_name($1);
					dpas_error("`%s' cannot be used as an array bound", name);
					jit_free(name);
					$$ = 0;
				}
			}
	;

FieldList
	: /* empty */					{ parameter_list_init(&($$)); }
	| FixedPart						{ $$ = $1; }
	| FixedPart ';'					{ $$ = $1; }
	| FixedPart ';' VariantPart		{
				$$ = $1;
				parameter_list_merge(&($$), &($3));
			}
	| FixedPart ';' VariantPart ';'	{
				$$ = $1;
				parameter_list_merge(&($$), &($3));
			}
	| VariantPart					{ $$ = $1; }
	| VariantPart ';'				{ $$ = $1; }
	;

FixedPart
	: RecordSection					{ $$ = $1; }
	| FixedPart ';' RecordSection	{
				$$ = $1;
				parameter_list_merge(&($$), &($3));
			}
	;

RecordSection
	: IdentifierList ':' Type		{
				parameter_list_create(&($$), $1.list, $1.len, $3);
				jit_type_free($3);
			}
	| ProcedureOrFunctionHeading	{
				parameter_list_init(&($$));
				parameter_list_add(&($$), $1.name, $1.type);
			}
	;

VariantPart
	: K_CASE Identifier ':' TypeIdentifier K_OF VariantList	{
				parameter_list_init(&($$));
				parameter_list_add(&($$), $2, $4);
				parameter_list_add(&($$), 0, $6);
			}
	| K_CASE ':' TypeIdentifier K_OF VariantList	{
				parameter_list_init(&($$));
				parameter_list_add(&($$), 0, $5);
				jit_type_free($3);
			}
	;

VariantList
	: VariantCaseList	{
				/* Create a union with all of the case limbs */
				$$ = jit_type_create_union
					($1.list, (unsigned int)($1.len), 0);
				jit_free($1.list);
			}
	;

VariantCaseList
	: Variant			{
				$$.list = 0;
				$$.len = 0;
				type_list_add(&($$.list), &($$.len), $1);
			}
	| VariantCaseList ';' Variant	{
				$$ = $1;
				type_list_add(&($$.list), &($$.len), $3);
			}
	;

Variant
	: VariantCaseLabelList ':' '(' FieldList ')'	{
				jit_type_t type;
				type = jit_type_create_struct
					($4.types, (unsigned int)($4.len), 1);
				if(!type)
				{
					dpas_out_of_memory();
				}
				if(!jit_type_set_names
						(type, $4.names, (unsigned int)($4.len)))
				{
					dpas_out_of_memory();
				}
				parameter_list_free(&($4));
				$$ = type;
			}
	;

/* We ignore the variant case labels, as we don't perform any
   checks on the variant structure - we just use it as a union */
VariantCaseLabelList
	: Constant							{}
	| VariantCaseLabelList ',' Constant	{}
	;

/*
 * Constants.
 */

Constant
	: ConstantValue				{ $$ = $1; }
	| '+' ConstantValue			{
				if($2.type != jit_type_int &&
				   $2.type != jit_type_uint &&
				   $2.type != jit_type_long &&
				   $2.type != jit_type_ulong &&
				   $2.type != jit_type_nfloat)
				{
					char *name = dpas_type_name($2.type);
					dpas_error("unary `+' cannot be applied to a constant "
							   "of type `%s'", name);
					jit_free(name);
				}
				$$ = $2;
			}
	| '-' ConstantValue			{
				if($2.type == jit_type_int)
				{
					$$.type = $2.type;
					$$.un.int_value = -($2.un.int_value);
				}
				else if($2.type == jit_type_uint)
				{
					$$.type = jit_type_long;
					$$.un.long_value = -((jit_long)($2.un.uint_value));
				}
				else if($2.type == jit_type_long)
				{
					$$.type = jit_type_long;
					$$.un.long_value = -($2.un.long_value);
				}
				else if($2.type == jit_type_ulong)
				{
					$$.type = jit_type_long;
					$$.un.long_value = -((jit_long)($2.un.ulong_value));
				}
				else if($2.type == jit_type_nfloat)
				{
					$$.type = jit_type_nfloat;
					$$.un.nfloat_value = -($2.un.nfloat_value);
				}
				else
				{
					char *name = dpas_type_name($2.type);
					dpas_error("unary `-' cannot be applied to a constant "
							   "of type `%s'", name);
					jit_free(name);
					$$ = $2;
				}
			}
	;

ConstantValue
	: BasicConstant				{ $$ = $1; }
	| Identifier				{
				dpas_scope_item_t item;
				item = dpas_scope_lookup(dpas_scope_current(), $1, 1);
				if(!item)
				{
					dpas_error("`%s' is not declared in the current scope", $1);
					$$.type = jit_type_int;
					$$.un.int_value = 0;
				}
				else if(dpas_scope_item_kind(item) != DPAS_ITEM_CONSTANT)
				{
					dpas_error("`%s' is not declared as a constant "
							   "in the current scope", $1);
					$$.type = jit_type_int;
					$$.un.int_value = 0;
				}
				else
				{
					$$ = *((jit_constant_t *)(dpas_scope_item_info(item)));
				}
				jit_free($1);
			}
	;

BasicConstant
	: INTEGER_CONSTANT			{
				$$.type = $1.type;
				if($1.type == jit_type_int)
				{
					$$.un.int_value = (jit_int)($1.value);
				}
				else if($1.type == jit_type_uint)
				{
					$$.un.uint_value = (jit_uint)($1.value);
				}
				else if($1.type == jit_type_long)
				{
					$$.un.long_value = (jit_long)($1.value);
				}
				else
				{
					$$.un.ulong_value = $1.value;
				}
			}
	| REAL_CONSTANT				{
				$$.type = jit_type_nfloat;
				$$.un.nfloat_value = $1;
			}
	| STRING_CONSTANT			{
				/* Note: the string pointer leaks from the parser,
				   but since we probably need it to hang around for
				   runtime, this shouldn't be serious */
				$$.type = dpas_type_string;
				$$.un.ptr_value = $1;
			}
	| K_NIL						{
				$$.type = dpas_type_nil;
				$$.un.ptr_value = 0;
			}
	;
