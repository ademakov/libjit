/*
 * dpas-builtin.c - Handling for Dynamic Pascal builtins.
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
 * Call a native builtin function.
 */
static jit_value_t call_builtin
	(jit_function_t func, const char *name, void *native_func,
	 jit_type_t arg1_type, jit_value_t value1,
	 jit_type_t arg2_type, jit_value_t value2,
	 jit_type_t return_type)
{
	jit_type_t signature;
	jit_type_t arg_types[2];
	jit_value_t args[2];
	int num_args = 0;
	if(arg1_type)
	{
		args[num_args] = jit_insn_convert(func, value1, arg1_type, 0);
		if(!(args[num_args]))
		{
			dpas_out_of_memory();
		}
		arg_types[num_args] = arg1_type;
		++num_args;
	}
	if(arg2_type)
	{
		args[num_args] = jit_insn_convert(func, value2, arg2_type, 0);
		if(!(args[num_args]))
		{
			dpas_out_of_memory();
		}
		arg_types[num_args] = arg2_type;
		++num_args;
	}
	signature = jit_type_create_signature
		(jit_abi_cdecl, return_type, arg_types, num_args, 1);
	if(!signature)
	{
		dpas_out_of_memory();
	}
	value1 = jit_insn_call_native(func, name, native_func, signature,
							      args, num_args, JIT_CALL_NOTHROW);
	if(!value1)
	{
		dpas_out_of_memory();
	}
	jit_type_free(signature);
	return value1;
}

/*
 * Call a native "write" function.
 */
static void call_write
	(jit_function_t func, const char *name, void *native_func,
	 jit_type_t arg1_type, jit_value_t value1)
{
	call_builtin(func, name, native_func, arg1_type,
				 value1, 0, 0, jit_type_void);
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
 * Create a new object of a specific type and put its pointer into a variable.
 */
static dpas_semvalue dpas_new(dpas_semvalue *args, int num_args)
{
	dpas_semvalue result;
	jit_type_t type = dpas_sem_get_type(args[0]);
	jit_nuint size;
	jit_value_t value;
	if(dpas_sem_is_lvalue(args[0]) && jit_type_is_pointer(type))
	{
		size = jit_type_get_size(jit_type_get_ref(type));
		value = call_builtin
			(dpas_current_function(), "jit_calloc", (void *)jit_calloc,
			 jit_type_sys_uint,
			 jit_value_create_nint_constant
			 	(dpas_current_function(), jit_type_sys_uint, 1),
			 jit_type_sys_uint,
			 jit_value_create_nint_constant
			 	(dpas_current_function(), jit_type_sys_uint, (jit_nint)size),
			 jit_type_void_ptr);
		if(!value)
		{
			dpas_out_of_memory();
		}
		if(!jit_insn_store(dpas_current_function(),
						   dpas_sem_get_value(args[0]), value))
		{
			dpas_out_of_memory();
		}
	}
	else if(dpas_sem_is_lvalue_ea(args[0]) && jit_type_is_pointer(type))
	{
		size = jit_type_get_size(jit_type_get_ref(type));
		value = call_builtin
			(dpas_current_function(), "jit_calloc", (void *)jit_calloc,
			 jit_type_sys_uint,
			 jit_value_create_nint_constant
			 	(dpas_current_function(), jit_type_sys_uint, 1),
			 jit_type_sys_uint,
			 jit_value_create_nint_constant
			 	(dpas_current_function(), jit_type_sys_uint, (jit_nint)size),
			 jit_type_void_ptr);
		if(!value)
		{
			dpas_out_of_memory();
		}
		if(!jit_insn_store_relative(dpas_current_function(),
						            dpas_sem_get_value(args[0]), 0, value))
		{
			dpas_out_of_memory();
		}
	}
	else
	{
		if(!dpas_sem_is_error(args[0]))
		{
			dpas_error("invalid l-value used with `New'");
		}
	}
	dpas_sem_set_void(result);
	return result;
}

/*
 * Dispose of an object that is referenced by a pointer.
 */
static dpas_semvalue dpas_dispose(dpas_semvalue *args, int num_args)
{
	dpas_semvalue result;
	jit_type_t type = dpas_sem_get_type(args[0]);
	result = dpas_lvalue_to_rvalue(args[0]);
	if(dpas_sem_is_rvalue(result) && jit_type_is_pointer(type))
	{
		call_write(dpas_current_function(), "jit_free", (void *)jit_free,
				   jit_type_void_ptr, dpas_sem_get_value(result));
	}
	else if(!dpas_sem_is_error(result))
	{
		dpas_error("invalid argument used with `Dispose'");
	}
	dpas_sem_set_void(result);
	return result;
}

/*
 * Determine if two values/types have identical types.
 */
static dpas_semvalue dpas_same_type(dpas_semvalue *args, int num_args)
{
	dpas_semvalue result;
	if((!dpas_sem_is_rvalue(args[0]) && !dpas_sem_is_type(args[0])) ||
	   (!dpas_sem_is_rvalue(args[1]) && !dpas_sem_is_type(args[1])))
	{
		dpas_error("invalid operands to `SameType'");
		dpas_sem_set_error(result);
	}
	else if(dpas_type_identical(dpas_sem_get_type(args[0]),
								dpas_sem_get_type(args[1]), 0))
	{
		dpas_sem_set_rvalue
			(result, dpas_type_boolean,
			 jit_value_create_nint_constant
			 	(dpas_current_function(), dpas_type_boolean, 1));
	}
	else
	{
		dpas_sem_set_rvalue
			(result, dpas_type_boolean,
			 jit_value_create_nint_constant
			 	(dpas_current_function(), dpas_type_boolean, 0));
	}
	return result;
}

/*
 * Determine if two values/types have the same basic shape.
 */
static dpas_semvalue dpas_same_shape(dpas_semvalue *args, int num_args)
{
	dpas_semvalue result;
	if((!dpas_sem_is_rvalue(args[0]) && !dpas_sem_is_type(args[0])) ||
	   (!dpas_sem_is_rvalue(args[1]) && !dpas_sem_is_type(args[1])))
	{
		dpas_error("invalid operands to `SameShape'");
		dpas_sem_set_error(result);
	}
	else if(dpas_type_identical(dpas_sem_get_type(args[0]),
								dpas_sem_get_type(args[1]), 1))
	{
		dpas_sem_set_rvalue
			(result, dpas_type_boolean,
			 jit_value_create_nint_constant
			 	(dpas_current_function(), dpas_type_boolean, 1));
	}
	else
	{
		dpas_sem_set_rvalue
			(result, dpas_type_boolean,
			 jit_value_create_nint_constant
			 	(dpas_current_function(), dpas_type_boolean, 0));
	}
	return result;
}

/*
 * Mathematical functions.
 */
#define	dpas_math_unary(name,func)	\
static dpas_semvalue dpas_##name(dpas_semvalue *args, int num_args)	\
{ \
	dpas_semvalue result; \
	jit_value_t value; \
	if(!dpas_sem_is_rvalue(args[0])) \
	{ \
		dpas_error("invalid operand to unary `" #name "'"); \
		dpas_sem_set_error(result); \
	} \
	else \
	{ \
		value = func(dpas_current_function(), \
					 dpas_sem_get_value(dpas_lvalue_to_rvalue(args[0]))); \
		if(!value) \
		{ \
			dpas_out_of_memory(); \
		} \
		dpas_sem_set_rvalue(result, jit_value_get_type(value), value); \
	} \
	return result; \
}
#define	dpas_math_binary(name,func)	\
static dpas_semvalue dpas_##name(dpas_semvalue *args, int num_args)	\
{ \
	dpas_semvalue result; \
	jit_value_t value; \
	if(!dpas_sem_is_rvalue(args[0]) || !dpas_sem_is_rvalue(args[0])) \
	{ \
		dpas_error("invalid operands to binary `" #name "'"); \
		dpas_sem_set_error(result); \
	} \
	else \
	{ \
		value = func(dpas_current_function(), \
					 dpas_sem_get_value(dpas_lvalue_to_rvalue(args[0])), \
					 dpas_sem_get_value(dpas_lvalue_to_rvalue(args[1]))); \
		if(!value) \
		{ \
			dpas_out_of_memory(); \
		} \
		dpas_sem_set_rvalue(result, jit_value_get_type(value), value); \
	} \
	return result; \
}
#define	dpas_math_test(name,func)	\
static dpas_semvalue dpas_##name(dpas_semvalue *args, int num_args)	\
{ \
	dpas_semvalue result; \
	jit_value_t value; \
	if(!dpas_sem_is_rvalue(args[0])) \
	{ \
		dpas_error("invalid operand to unary `" #name "'"); \
		dpas_sem_set_error(result); \
	} \
	else \
	{ \
		value = func(dpas_current_function(), \
					 dpas_sem_get_value(dpas_lvalue_to_rvalue(args[0]))); \
		if(!value) \
		{ \
			dpas_out_of_memory(); \
		} \
		dpas_sem_set_rvalue(result, dpas_type_boolean, value); \
	} \
	return result; \
}
dpas_math_unary(acos, jit_insn_acos)
dpas_math_unary(asin, jit_insn_asin)
dpas_math_unary(atan, jit_insn_atan)
dpas_math_binary(atan2, jit_insn_atan2)
dpas_math_unary(ceil, jit_insn_ceil)
dpas_math_unary(cos, jit_insn_cos)
dpas_math_unary(cosh, jit_insn_cosh)
dpas_math_unary(exp, jit_insn_exp)
dpas_math_unary(floor, jit_insn_floor)
dpas_math_unary(log, jit_insn_log)
dpas_math_unary(log10, jit_insn_log10)
dpas_math_unary(rint, jit_insn_rint)
dpas_math_unary(round, jit_insn_round)
dpas_math_unary(sin, jit_insn_sin)
dpas_math_unary(sinh, jit_insn_sinh)
dpas_math_unary(sqrt, jit_insn_sqrt)
dpas_math_unary(tan, jit_insn_tan)
dpas_math_unary(tanh, jit_insn_tanh)
dpas_math_unary(trunc, jit_insn_trunc)
dpas_math_unary(abs, jit_insn_abs)
dpas_math_binary(min, jit_insn_min)
dpas_math_binary(max, jit_insn_max)
dpas_math_unary(sign, jit_insn_sign)
dpas_math_test(isnan, jit_insn_is_nan)
dpas_math_unary(isinf, jit_insn_is_inf)
dpas_math_test(finite, jit_insn_is_finite)

/*
 * Builtins that we currently recognize.
 */
#define	DPAS_BUILTIN_WRITE			1
#define	DPAS_BUILTIN_WRITELN		2
#define	DPAS_BUILTIN_FLUSH			3
#define	DPAS_BUILTIN_TERMINATE		4
#define	DPAS_BUILTIN_NEW			5
#define	DPAS_BUILTIN_DISPOSE		6
#define	DPAS_BUILTIN_SAMETYPE		7
#define	DPAS_BUILTIN_SAMESHAPE		8
#define	DPAS_BUILTIN_ACOS			9
#define	DPAS_BUILTIN_ASIN			10
#define	DPAS_BUILTIN_ATAN			11
#define	DPAS_BUILTIN_ATAN2			12
#define	DPAS_BUILTIN_CEIL			13
#define	DPAS_BUILTIN_COS			14
#define	DPAS_BUILTIN_COSH			15
#define	DPAS_BUILTIN_EXP			16
#define	DPAS_BUILTIN_FLOOR			17
#define	DPAS_BUILTIN_LOG			18
#define	DPAS_BUILTIN_LOG10			19
#define	DPAS_BUILTIN_RINT			20
#define	DPAS_BUILTIN_ROUND			21
#define	DPAS_BUILTIN_SIN			22
#define	DPAS_BUILTIN_SINH			23
#define	DPAS_BUILTIN_SQRT			24
#define	DPAS_BUILTIN_TAN			25
#define	DPAS_BUILTIN_TANH			26
#define	DPAS_BUILTIN_TRUNC			27
#define	DPAS_BUILTIN_ABS			28
#define	DPAS_BUILTIN_MIN			29
#define	DPAS_BUILTIN_MAX			30
#define	DPAS_BUILTIN_SIGN			31
#define	DPAS_BUILTIN_ISNAN			32
#define	DPAS_BUILTIN_ISINF			33
#define	DPAS_BUILTIN_FINITE			34

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
	{"Write",		DPAS_BUILTIN_WRITE,		dpas_write,		 -1},
	{"WriteLn",		DPAS_BUILTIN_WRITELN,	dpas_writeln,	 -1},
	{"Flush",		DPAS_BUILTIN_FLUSH,		dpas_flush,		  0},
	{"Terminate",	DPAS_BUILTIN_TERMINATE,	dpas_terminate,	  1},
	{"New",			DPAS_BUILTIN_NEW,		dpas_new,	 	  1},
	{"Dispose",		DPAS_BUILTIN_DISPOSE,	dpas_dispose, 	  1},
	{"SameType",	DPAS_BUILTIN_SAMETYPE,	dpas_same_type,   2},
	{"SameShape",	DPAS_BUILTIN_SAMESHAPE,	dpas_same_shape,  2},
	{"Acos",		DPAS_BUILTIN_ACOS,		dpas_acos,        1},
	{"Asin",		DPAS_BUILTIN_ASIN,		dpas_asin,        1},
	{"Atan",		DPAS_BUILTIN_ATAN,		dpas_atan,        1},
	{"Atan2",		DPAS_BUILTIN_ATAN2,		dpas_atan2,       2},
	{"Ceil",		DPAS_BUILTIN_CEIL,		dpas_ceil,        1},
	{"Cos",			DPAS_BUILTIN_COS,		dpas_cos,         1},
	{"Cosh",		DPAS_BUILTIN_COSH,		dpas_cosh,        1},
	{"Exp",			DPAS_BUILTIN_EXP,		dpas_exp,         1},
	{"Floor",		DPAS_BUILTIN_FLOOR,		dpas_floor,       1},
	{"Log",			DPAS_BUILTIN_LOG,		dpas_log,         1},
	{"Log10",		DPAS_BUILTIN_LOG10,		dpas_log10,       1},
	{"Rint",		DPAS_BUILTIN_RINT,		dpas_rint,        1},
	{"Round",		DPAS_BUILTIN_ROUND,		dpas_round,       1},
	{"Sin",			DPAS_BUILTIN_SIN,		dpas_sin,         1},
	{"Sinh",		DPAS_BUILTIN_SINH,		dpas_sinh,        1},
	{"Sqrt",		DPAS_BUILTIN_SQRT,		dpas_sqrt,        1},
	{"Tan",			DPAS_BUILTIN_TAN,		dpas_tan,         1},
	{"Tanh",		DPAS_BUILTIN_TANH,		dpas_tanh,        1},
	{"Trunc",		DPAS_BUILTIN_TRUNC,		dpas_trunc,       1},
	{"Abs",			DPAS_BUILTIN_ABS,		dpas_abs,         1},
	{"Min",			DPAS_BUILTIN_MIN,		dpas_min,         2},
	{"Max",			DPAS_BUILTIN_MAX,		dpas_max,         2},
	{"Sign",		DPAS_BUILTIN_SIGN,		dpas_sign,        1},
	{"IsNaN",		DPAS_BUILTIN_ISNAN,		dpas_isnan,       1},
	{"IsInf",		DPAS_BUILTIN_ISINF,		dpas_isinf,       1},
	{"Finite",		DPAS_BUILTIN_FINITE,	dpas_finite,      1},
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
