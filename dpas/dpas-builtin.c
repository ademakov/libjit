/*
 * dpas-builtin.c - Handling for Dynamic Pascal builtins.
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
#include <stdio.h>
#include <stdlib.h>

/*
 * Functions for writing values to stdout.
 */
static void dpas_write_ln(void)
{
	putc('\n', stdout);
}
static void dpas_write_int(jit_int value)
{
	printf("%ld", (long)value);
}
static void dpas_write_uint(jit_uint value)
{
	printf("%lu", (unsigned long)value);
}
static void dpas_write_long(jit_long value)
{
	jit_constant_t val;
	char *name;
	val.type = jit_type_long;
	val.un.long_value = value;
	name = dpas_constant_name(&val);
	fputs(name, stdout);
	jit_free(name);
}
static void dpas_write_ulong(jit_ulong value)
{
	jit_constant_t val;
	char *name;
	val.type = jit_type_ulong;
	val.un.ulong_value = value;
	name = dpas_constant_name(&val);
	fputs(name, stdout);
	jit_free(name);
}
static void dpas_write_nfloat(jit_nfloat value)
{
	printf("%g", (double)value);
}
static void dpas_write_string(char *value)
{
	if(value)
	{
		fputs(value, stdout);
	}
	else
	{
		fputs("(null)", stdout);
	}
}

/*
 * Call a native "write" function.
 */
static void call_write(jit_function_t func, const char *name,
					   void *native_func, jit_type_t arg_type,
					   jit_value_t value)
{
	jit_type_t signature;
	jit_value_t args[1];
	int num_args = 0;
	if(arg_type)
	{
		args[0] = jit_insn_convert(func, value, arg_type, 0);
		if(!(args[0]))
		{
			dpas_out_of_memory();
		}
		num_args = 1;
		signature = jit_type_create_signature
			(jit_abi_cdecl, jit_type_void, &arg_type, 1, 1);
	}
	else
	{
		num_args = 0;
		signature = jit_type_create_signature
			(jit_abi_cdecl, jit_type_void, 0, 0, 1);
	}
	if(!signature)
	{
		dpas_out_of_memory();
	}
	if(!jit_insn_call_native(func, name, native_func, signature,
							 args, num_args, JIT_CALL_NOTHROW))
	{
		dpas_out_of_memory();
	}
	jit_type_free(signature);
}

/*
 * Write values to "stdout".
 */
static dpas_semvalue dpas_write_inner
	(dpas_semvalue *args, int num_args, int newline)
{
	jit_function_t func = dpas_current_function();
	dpas_semvalue result;
	jit_type_t type;
	jit_type_t orig_type;
	const char *name;
	void *native_func;
	int index;
	for(index = 0; index < num_args; ++index)
	{
		result = dpas_lvalue_to_rvalue(args[index]);
		if(!dpas_sem_is_rvalue(result))
		{
			dpas_error("invalid value for parameter %d", index + 1);
		}
		else
		{
			orig_type = dpas_sem_get_type(result);
			type = jit_type_normalize(orig_type);
			if(jit_type_is_pointer(orig_type) &&
			   jit_type_get_ref(orig_type) == dpas_type_char)
			{
				type = jit_type_void_ptr;
				name = "dpas_write_string";
				native_func = (void *)dpas_write_string;
			}
			else if(type == jit_type_sbyte ||
			        type == jit_type_ubyte ||
			        type == jit_type_short ||
			        type == jit_type_ushort ||
			        type == jit_type_int)
			{
				type = jit_type_int;
				name = "dpas_write_int";
				native_func = (void *)dpas_write_int;
			}
			else if(type == jit_type_uint)
			{
				type = jit_type_uint;
				name = "dpas_write_uint";
				native_func = (void *)dpas_write_uint;
			}
			else if(type == jit_type_long)
			{
				type = jit_type_long;
				name = "dpas_write_long";
				native_func = (void *)dpas_write_long;
			}
			else if(type == jit_type_ulong)
			{
				type = jit_type_ulong;
				name = "dpas_write_ulong";
				native_func = (void *)dpas_write_ulong;
			}
			else if(type == jit_type_float32 ||
					type == jit_type_float64 ||
					type == jit_type_nfloat)
			{
				type = jit_type_nfloat;
				name = "dpas_write_nfloat";
				native_func = (void *)dpas_write_nfloat;
			}
			else
			{
				dpas_error("unprintable value for parameter %d", index + 1);
				continue;
			}
			call_write(func, name, native_func, type,
					   dpas_sem_get_value(result));
		}
	}
	if(newline)
	{
		call_write(func, "dpas_write_nl", (void *)dpas_write_ln, 0, 0);
	}
	dpas_sem_set_void(result);
	return result;
}
static dpas_semvalue dpas_write(dpas_semvalue *args, int num_args)
{
	return dpas_write_inner(args, num_args, 0);
}

/*
 * Write values to "stdout", followed by a newline.
 */
static dpas_semvalue dpas_writeln(dpas_semvalue *args, int num_args)
{
	return dpas_write_inner(args, num_args, 1);
}

/*
 * Flush stdout.
 */
static void dpas_flush_stdout(void)
{
	fflush(stdout);
}
static dpas_semvalue dpas_flush(dpas_semvalue *args, int num_args)
{
	dpas_semvalue result;
	call_write(dpas_current_function(), "dpas_flush_stdout",
			   (void *)dpas_flush_stdout, 0, 0);
	dpas_sem_set_void(result);
	return result;
}

/*
 * Terminate program execution with an exit status code.
 */
static void dpas_terminate_program(jit_int value)
{
	exit((int)value);
}
static dpas_semvalue dpas_terminate(dpas_semvalue *args, int num_args)
{
	dpas_semvalue result;
	result = dpas_lvalue_to_rvalue(args[0]);
	call_write(dpas_current_function(), "dpas_terminate_program",
			   (void *)dpas_terminate_program, jit_type_int,
			   dpas_sem_get_value(result));
	dpas_sem_set_void(result);
	return result;
}

/*
 * Builtins that we currently recognize.
 */
#define	DPAS_BUILTIN_WRITE			1
#define	DPAS_BUILTIN_WRITELN		2
#define	DPAS_BUILTIN_FLUSH			3
#define	DPAS_BUILTIN_TERMINATE		4

/*
 * Table that defines the builtins.
 */
typedef struct
{
	const char	   *name;
	int				identifier;
	dpas_semvalue	(*func)(dpas_semvalue *args, int num_args);
	int				num_args;	/* -1 for a vararg function */

} dpas_builtin;
static dpas_builtin const builtins[] = {
	{"Write",		DPAS_BUILTIN_WRITE,		dpas_write,		-1},
	{"WriteLn",		DPAS_BUILTIN_WRITELN,	dpas_writeln,	-1},
	{"Flush",		DPAS_BUILTIN_FLUSH,		dpas_flush,		 0},
	{"Terminate",	DPAS_BUILTIN_TERMINATE,	dpas_terminate,	 1},
};
#define	num_builtins	(sizeof(builtins) / sizeof(dpas_builtin))

int dpas_is_builtin(const char *name)
{
	int index;
	for(index = 0; index < num_builtins; ++index)
	{
		if(!jit_stricmp(name, builtins[index].name))
		{
			return builtins[index].identifier;
		}
	}
	return 0;
}

dpas_semvalue dpas_expand_builtin
	(int identifier, dpas_semvalue *args, int num_args)
{
	int index;
	dpas_semvalue value;
	for(index = 0; index < num_builtins; ++index)
	{
		if(builtins[index].identifier == identifier)
		{
			if(builtins[index].num_args != -1 &&
			   builtins[index].num_args != num_args)
			{
				dpas_error("incorrect number of arguments to `%s' builtin",
						   builtins[index].name);
				dpas_sem_set_error(value);
				return value;
			}
			return (*(builtins[index].func))(args, num_args);
		}
	}
	dpas_sem_set_error(value);
	return value;
}
