/*
 * gen-apply.c - Generate the rules that are needed to use "__builtin_apply".
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

#include "jit-arch.h"
#include <jit/jit-defs.h>

#if defined(__APPLE__) && defined(__MACH__)
# define	JIT_MEMCPY	"_mem_copy"
#else
# define	JIT_MEMCPY	"mem_copy"
#endif

#include "jit-apply-func.h"
#include <stdio.h>
#include <config.h>
#if HAVE_STDLIB_H
	#include <stdlib.h>
#endif
#if HAVE_ALLOCA_H
	#include <alloca.h>
#endif

/*

This program tries to automatically discover the register assignment rules
that are used by gcc's "__builtin_apply" operator.  It is used to generate
the "jit-apply-rules.h" file.

*/

#if defined(__GNUC__)
	#define	PLATFORM_IS_GCC		1
#endif
#if defined(__i386) || defined(__i386__) || defined(_M_IX86)
	#define	PLATFORM_IS_X86		1
#if defined(__CYGWIN__) || defined(__CYGWIN32__) || \
	defined(_WIN32) || defined(WIN32)
	#define	PLATFORM_IS_WIN32	1
	#include <malloc.h>
	#ifndef alloca
		#define	alloca	_alloca
	#endif
#endif
#endif
#if defined(__APPLE__) && defined(__MACH__)
	#define	PLATFORM_IS_MACOSX	1
#endif
#if defined(__x86_64__) || defined(__x86_64)
	#define	PLATFORM_IS_X86_64	1
#endif
#if defined(__arm__) || defined(__arm)
	#define PLATFORM_IS_ARM		1
#endif


#if PLATFORM_IS_X86
/*
 * On x86 the extended precision numbers are 10 bytes long. However certain
 * ABIs define the long double size equal to 12 bytes. The extra 2 bytes are
 * for alignment purposes only and has no significance in computations.
 */
#define NFLOAT_SIGNIFICANT_BYTES (sizeof(jit_nfloat) != 12 ? sizeof(jit_nfloat) : 10)
#elif PLATFORM_IS_X86_64
/*
 * On x86_64 the extended precision numbers are 10 bytes long. However certain
 * ABIs define the long double size equal to 16 bytes. The extra 6 bytes are
 * for alignment purposes only and has no significance in computations.
 */
#define NFLOAT_SIGNIFICANT_BYTES (sizeof(jit_nfloat) != 16 ? sizeof(jit_nfloat) : 10)
#else
#define NFLOAT_SIGNIFICANT_BYTES sizeof(jit_nfloat)
#endif

#if defined(PLATFORM_IS_GCC) || defined(PLATFORM_IS_WIN32)

/*
 * Pick up pre-defined values on platforms where auto-detection doesn't work.
 */
#if defined(PLATFORM_IS_MACOSX)
	#include "gen-apply-macosx.h"
#endif

/*
 * Values that are detected.
 */
#ifndef JIT_APPLY_NUM_WORD_REGS
int num_word_regs = 0;
int num_float_regs = 0;
int num_double_regs = 0;
int num_nfloat_regs = 0;
int pass_stack_float_as_double = 0;
int pass_stack_float_as_nfloat = 0;
int pass_stack_double_as_nfloat = 0;
int pass_stack_nfloat_as_double = 0;
int pass_reg_float_as_double = 0;
int pass_reg_float_as_nfloat = 0;
int pass_reg_double_as_nfloat = 0;
int pass_reg_nfloat_as_double = 0;
int return_float_as_double = 0;
int return_float_as_nfloat = 0;
int return_double_as_nfloat = 0;
int return_nfloat_as_double = 0;
int floats_in_word_regs = 0;
int doubles_in_word_regs = 0;
int nfloats_in_word_regs = 0;
int return_floats_after = 0;
int return_doubles_after = 0;
int return_nfloats_after = 0;
int varargs_on_stack = 0;
int struct_return_special_reg = 0;
int struct_reg_overlaps_word_reg = 0;
int struct_return_in_reg[64];
int align_long_regs = 0;
int align_long_stack = 0;
int can_split_long = 0;
int x86_fastcall = 0;
int parent_frame_offset = 0;
int return_address_offset = 0;
int broken_frame_builtins = 0;
int max_struct_in_reg = 0;
int x86_pop_struct_return = 0;
int pad_float_regs = 0;
#else
int num_word_regs = JIT_APPLY_NUM_WORD_REGS;
int num_float_regs = JIT_APPLY_NUM_FLOAT_REGS;
int num_double_regs = JIT_APPLY_NUM_DOUBLE_REGS;
int num_nfloat_regs = JIT_APPLY_NUM_NFLOAT_REGS;
int pass_stack_float_as_double = JIT_APPLY_PASS_STACK_FLOAT_AS_DOUBLE;
int pass_stack_float_as_nfloat = JIT_APPLY_PASS_STACK_FLOAT_AS_NFLOAT;
int pass_stack_double_as_nfloat = JIT_APPLY_PASS_STACK_DOUBLE_AS_NFLOAT;
int pass_stack_nfloat_as_double = JIT_APPLY_PASS_STACK_NFLOAT_AS_DOUBLE;
int pass_reg_float_as_double = JIT_APPLY_PASS_REG_FLOAT_AS_DOUBLE;
int pass_reg_float_as_nfloat = JIT_APPLY_PASS_REG_FLOAT_AS_NFLOAT;
int pass_reg_double_as_nfloat = JIT_APPLY_PASS_REG_DOUBLE_AS_NFLOAT;
int pass_reg_nfloat_as_double = JIT_APPLY_PASS_REG_NFLOAT_AS_DOUBLE;
int return_float_as_double = JIT_APPLY_RETURN_FLOAT_AS_DOUBLE;
int return_float_as_nfloat = JIT_APPLY_RETURN_FLOAT_AS_NFLOAT;
int return_double_as_nfloat = JIT_APPLY_RETURN_DOUBLE_AS_NFLOAT;
int return_nfloat_as_double = JIT_APPLY_RETURN_NFLOAT_AS_DOUBLE;
int floats_in_word_regs = JIT_APPLY_FLOATS_IN_WORD_REGS;
int doubles_in_word_regs = JIT_APPLY_DOUBLES_IN_WORD_REGS;
int nfloats_in_word_regs = JIT_APPLY_NFLOATS_IN_WORD_REGS;
int return_floats_after = JIT_APPLY_RETURN_FLOATS_AFTER;
int return_doubles_after = JIT_APPLY_RETURN_DOUBLES_AFTER;
int return_nfloats_after = JIT_APPLY_RETURN_NFLOATS_AFTER;
int varargs_on_stack = JIT_APPLY_VARARGS_ON_STACK;
int struct_return_special_reg = JIT_APPLY_STRUCT_RETURN_SPECIAL_REG;
int struct_reg_overlaps_word_reg = JIT_APPLY_STRUCT_REG_OVERLAPS_WORD_REG;
int struct_return_in_reg[64] = JIT_APPLY_STRUCT_RETURN_IN_REG;
int align_long_regs = JIT_APPLY_ALIGN_LONG_REGS;
int align_long_stack = JIT_APPLY_ALIGN_LONG_STACK;
int can_split_long = JIT_APPLY_CAN_SPLIT_LONG;
int x86_fastcall = JIT_APPLY_X86_FASTCALL;
int parent_frame_offset = JIT_APPLY_PARENT_FRAME_OFFSET;
int return_address_offset = JIT_APPLY_RETURN_ADDRESS_OFFSET;
int broken_frame_builtins = JIT_APPLY_BROKEN_FRAME_BUILTINS;
int max_struct_in_reg = JIT_APPLY_MAX_STRUCT_IN_REG;
int x86_pop_struct_return = JIT_APPLY_X86_POP_STRUCT_RETURN;
int pad_float_regs = JIT_APPLY_PAD_FLOAT_REGS;
#endif
int max_apply_size = 0;

void *mem_copy(void *dest, const void *src, unsigned int len)
{
	unsigned char *d = (unsigned char *)dest;
	const unsigned char *s = (const unsigned char *)src;
	while(len > 0)
	{
		*d++ = *s++;
		--len;
	}
	return dest;
}
#define	jit_memcpy mem_copy

void mem_set(void *dest, int value, unsigned int len)
{
	unsigned char *d = (unsigned char *)dest;
	while(len > 0)
	{
		*d++ = (unsigned char)value;
		--len;
	}
}

int mem_cmp(const void *s1, const void *s2, unsigned int len)
{
	const unsigned char *str1 = (const unsigned char *)s1;
	const unsigned char *str2 = (const unsigned char *)s2;
	while(len > 0)
	{
		if(*str1 < *str2)
			return -1;
		else if(*str1 > *str2)
			return 1;
		++str1;
		++str2;
		--len;
	}
	return 0;
}

/*
 * Detect the number of word registers that are used in function calls.
 * We assume that the platform uses less than 32 registers in outgoing calls.
 */
void detect_word_regs(jit_nint arg1, jit_nint arg2, jit_nint arg3,
					  jit_nint arg4, jit_nint arg5, jit_nint arg6,
					  jit_nint arg7, jit_nint arg8, jit_nint arg9,
					  jit_nint arg10, jit_nint arg11, jit_nint arg12,
					  jit_nint arg13, jit_nint arg14, jit_nint arg15,
					  jit_nint arg16, jit_nint arg17, jit_nint arg18,
					  jit_nint arg19, jit_nint arg20, jit_nint arg21,
					  jit_nint arg22, jit_nint arg23, jit_nint arg24,
					  jit_nint arg25, jit_nint arg26, jit_nint arg27,
					  jit_nint arg28, jit_nint arg29, jit_nint arg30,
					  jit_nint arg31, jit_nint arg32)
{
	/* We fetch the number in the first stack argument, which will
	   correspond to the number of word registers that are present */
	jit_nint *args, *stack_args;
	jit_builtin_apply_args(jit_nint *, args);
	stack_args = (jit_nint *)(args[0]);
	num_word_regs = (int)(stack_args[0]);

	/* Detect the presence of a structure return register by checking
	   to see if "arg1" is in the second word position after "stack_args" */
	if(num_word_regs > 1 && args[2] == 0)
	{
		struct_return_special_reg = 1;
	}
}

/*
 * Detect the presence of a structure return register if there are 0 or 1
 * word registers as detected by "detect_word_regs".  The structure
 * below must be big enough to avoid being returned in a register.
 */
struct detect_struct_reg
{
	void *field1;
	void *field2;
	void *field3;
	void *field4;
	void *field5;
	void *field6;
	void *field7;
	void *field8;
};
struct detect_struct_reg detect_struct_buf;
struct detect_struct_reg detect_struct_return(jit_nint arg1, jit_nint arg2)
{
	struct detect_struct_reg ret;
	jit_nint *args, *stack_args;
	jit_builtin_apply_args(jit_nint *, args);
	stack_args = (jit_nint *)(args[0]);

	/* Set the return value */
	ret.field1 = 0;
	ret.field2 = 0;
	ret.field3 = 0;
	ret.field4 = 0;
	ret.field5 = 0;
	ret.field6 = 0;
	ret.field7 = 0;
	ret.field8 = 0;

	/* If the stack starts with something other than 1 or 2,
	   then the structure return pointer is passed on the stack */
	if(stack_args[0] != 1 && stack_args[0] != 2)
	{
		return ret;
	}

	/* Check the slots to determine where the pointer resides */
	if(num_word_regs == 0)
	{
		/* If there are no word registers and the stack top
		   does not look like a return pointer, then the
		   structure return must be in a special register
		   that is separate from the normal word regsiters */
		struct_return_special_reg = 1;
	}
	else
	{
		if(stack_args[0] == 2)
		{
			/* The first word argument is still in a register,
			   so there must be a special structure register.
			   If the first word argument was on the stack, then
			   the structure return is in an ordinary register */
			struct_return_special_reg = 1;
		}
	}

	/* Done */
	return ret;
}

/*
 * Determine if a special structure return register overlaps
 * with the first word register.
 */
struct detect_struct_reg detect_struct_overlap(jit_nint arg1, jit_nint arg2)
{
	struct detect_struct_reg ret;
	jit_nint *args;
	jit_builtin_apply_args(jit_nint *, args);

	/* Set the return value */
	ret.field1 = 0;
	ret.field2 = 0;
	ret.field3 = 0;
	ret.field4 = 0;
	ret.field5 = 0;
	ret.field6 = 0;
	ret.field7 = 0;
	ret.field8 = 0;

	/* Check for overlap */
	if(struct_return_special_reg && num_word_regs > 0)
	{
		if(args[1] == args[2])
		{
			struct_reg_overlaps_word_reg = 1;
		}
	}

	/* Done */
	return ret;
}

/*
 * Detect the number of floating-point registers.
 */
void detect_float_regs(float arg1, float arg2, float arg3,
					   float arg4, float arg5, float arg6,
					   float arg7, float arg8, float arg9,
					   float arg10, float arg11, float arg12,
					   float arg13, float arg14, float arg15,
					   float arg16, float arg17, float arg18,
					   float arg19, float arg20, float arg21,
					   float arg22, float arg23, float arg24,
					   float arg25, float arg26, float arg27,
					   float arg28, float arg29, float arg30,
					   float arg31, float arg32)
{
	jit_nint *args;
	float *stack_args;
	jit_builtin_apply_args(jit_nint *, args);
	stack_args = (float *)(args[0]);

	/* The first stack argument indicates the number of floating-point
	   registers.  At the moment we don't know if they overlap with
	   the word registers or not */
	num_float_regs = (int)(stack_args[0]);
}

/*
 * Detect the number of floating-point registers.
 */
void detect_double_regs(double arg1, double arg2, double arg3,
						double arg4, double arg5, double arg6,
						double arg7, double arg8, double arg9,
						double arg10, double arg11, double arg12,
						double arg13, double arg14, double arg15,
						double arg16, double arg17, double arg18,
						double arg19, double arg20, double arg21,
						double arg22, double arg23, double arg24,
						double arg25, double arg26, double arg27,
						double arg28, double arg29, double arg30,
						double arg31, double arg32)
{
	jit_nint *args;
	double *stack_args;
	jit_builtin_apply_args(jit_nint *, args);
	stack_args = (double *)(args[0]);

	/* The first stack argument indicates the number of floating-point
	   registers.  At the moment we don't know if they overlap with
	   the word registers or not */
	num_double_regs = (int)(stack_args[0]);
}

/*
 * Detect the number of native floating-point registers.
 */
void detect_nfloat_regs(jit_nfloat arg1, jit_nfloat arg2, jit_nfloat arg3,
						jit_nfloat arg4, jit_nfloat arg5, jit_nfloat arg6,
						jit_nfloat arg7, jit_nfloat arg8, jit_nfloat arg9,
						jit_nfloat arg10, jit_nfloat arg11, jit_nfloat arg12,
						jit_nfloat arg13, jit_nfloat arg14, jit_nfloat arg15,
						jit_nfloat arg16, jit_nfloat arg17, jit_nfloat arg18,
						jit_nfloat arg19, jit_nfloat arg20, jit_nfloat arg21,
						jit_nfloat arg22, jit_nfloat arg23, jit_nfloat arg24,
						jit_nfloat arg25, jit_nfloat arg26, jit_nfloat arg27,
						jit_nfloat arg28, jit_nfloat arg29, jit_nfloat arg30,
						jit_nfloat arg31, jit_nfloat arg32)
{
	jit_nint *args;
	jit_nfloat *stack_args;
	jit_builtin_apply_args(jit_nint *, args);
	stack_args = (jit_nfloat *)(args[0]);

	/* The first stack argument indicates the number of floating-point
	   registers.  At the moment we don't know if they overlap with
	   the word registers or not */
	num_nfloat_regs = (int)(stack_args[0]);
}

#ifdef JIT_NATIVE_INT32

/*
 * Detect if a "float" value will use a word register.
 */
void detect_float_overlap(float x, jit_nint y)
{
	/* We have an overlap if "y" is on the stack */
	jit_nint *args;
	jit_builtin_apply_args(jit_nint *, args);
	if(args[struct_return_special_reg + 1] != 1)
	{
		floats_in_word_regs = 1;
		num_float_regs = 0;
	}
}

#endif /* JIT_NATIVE_INT32 */

/*
 * Detect if a "double" value will use a word register.
 */
void detect_double_overlap(double x, jit_nint y, jit_nint z)
{
	/* We have an overlap if "x" is in the first word register slot */
	double temp;
	jit_nint *args;
	jit_builtin_apply_args(jit_nint *, args);
	mem_copy(&temp, args + struct_return_special_reg + 1, sizeof(temp));
	if(!mem_cmp(&temp, &x, sizeof(double)))
	{
		doubles_in_word_regs = 1;
		num_double_regs = 0;
	}
}

/*
 * Detect if a "nfloat" value will use a word register.
 */
void detect_nfloat_overlap(jit_nfloat x, jit_nint y, jit_nint z)
{
	/* We have an overlap if "x" is in the first word register slot */
	jit_nfloat temp;
	jit_nint *args;
	jit_builtin_apply_args(jit_nint *, args);
	mem_copy(&temp, args + struct_return_special_reg + 1, sizeof(temp));
	if(!mem_cmp(&temp, &x, sizeof(double)))
	{
		nfloats_in_word_regs = 1;
		num_nfloat_regs = 0;
	}
}

/*
 * Detect if floating-point registers are "double" or "long double" in size.
 */
void detect_float_reg_size_regs(double x, double y)
{
	double temp;
	jit_nint *args;
	jit_builtin_apply_args(jit_nint *, args);
	mem_copy(&temp, args + 1 + struct_return_special_reg + num_word_regs,
			 sizeof(temp));
	if((num_nfloat_regs > 0) && !mem_cmp(&temp, &x, sizeof(double)))
	{
		pass_reg_nfloat_as_double = 1;
	}
	else
	{
		mem_copy(&temp, args + 1 + struct_return_special_reg +
						num_word_regs + 1,
				 sizeof(temp));
		if(!mem_cmp(&temp, &x, sizeof(double)))
		{
			if(num_nfloat_regs > 0)
			{
				pass_reg_nfloat_as_double = 1;
			}
			pad_float_regs = 1;
		}
		else
		{
			mem_copy(&temp, args + 1 + struct_return_special_reg +
							num_word_regs + 2,
					 sizeof(temp));
			if(!mem_cmp(&temp, &x, sizeof(double)))
			{
				if(num_nfloat_regs > 0)
				{
					pass_reg_nfloat_as_double = 1;
				}
				pad_float_regs = 2;
			}
			else
			{
				pass_reg_double_as_nfloat = 1;
			}
		}
	}
}
void detect_float_reg_size_stack(jit_nfloat x, jit_nfloat y)
{
	double temp;
	double dx;
	jit_nint *args;
	jit_builtin_apply_args(jit_nint *, args);
	mem_copy(&temp, (void *)(args[0]), sizeof(temp));
	dx = (double)x;
	if(!mem_cmp(&temp, &dx, sizeof(double)))
	{
		pass_stack_nfloat_as_double = 1;
	}
}

/*
 * Detect the promotion rules for "float" values.
 */
void detect_float_promotion(float arg1, float arg2, float arg3,
					        float arg4, float arg5, float arg6,
					        float arg7, float arg8, float arg9,
					        float arg10, float arg11, float arg12,
					        float arg13, float arg14, float arg15,
					        float arg16, float arg17, float arg18,
					        float arg19, float arg20, float arg21,
					        float arg22, float arg23, float arg24,
					        float arg25, float arg26, float arg27,
					        float arg28, float arg29, float arg30,
					        float arg31, float arg32)
{
	jit_nint *args, *stack_args;
	float value, test;
	double dvalue;
	int reg_promote;
	int stack_promote;
	int index;

	/* Extract the arguments */
	jit_builtin_apply_args(jit_nint *, args);
	stack_args = (jit_nint *)(args[0]);
	reg_promote = 0;
	stack_promote = 0;

	/* Handle the easy promotion cases first */
	if(floats_in_word_regs)
	{
		/* The value will either be in 1 or 2 word registers */
		mem_copy(&value, args + 1 + struct_return_special_reg, sizeof(value));
		if(value != arg1)
		{
			reg_promote = 1;
		}
	}
	else if(num_float_regs > 0)
	{
		/* The value is in a float register, which is always promoted */
		reg_promote = 1;
	}

	/* Skip the arguments that will be stored in registers */
	index = 1;
	if(floats_in_word_regs)
	{
		if(reg_promote && sizeof(jit_nint) == sizeof(jit_int))
		{
			index += num_word_regs / 2;
		}
		else
		{
			index += num_word_regs;
		}
	}
	else if(num_float_regs > 0)
	{
		index += num_float_regs;
	}

	/* Get the value corresponding to argument "index" */
	switch(index)
	{
		case 1:			test = arg1; break;
		case 2:			test = arg2; break;
		case 3:			test = arg3; break;
		case 4:			test = arg4; break;
		case 5:			test = arg5; break;
		case 6:			test = arg6; break;
		case 7:			test = arg7; break;
		case 8:			test = arg8; break;
		case 9:			test = arg9; break;
		case 10:		test = arg10; break;
		case 11:		test = arg11; break;
		case 12:		test = arg12; break;
		case 13:		test = arg13; break;
		case 14:		test = arg14; break;
		case 15:		test = arg15; break;
		case 16:		test = arg16; break;
		case 17:		test = arg17; break;
		case 18:		test = arg18; break;
		case 19:		test = arg19; break;
		case 20:		test = arg20; break;
		case 30:		test = arg30; break;
		case 31:		test = arg31; break;
		case 32:		test = arg32; break;
		default:		test = (float)(-1.0); break;
	}

	/* Determine if stacked values are promoted */
	mem_copy(&value, stack_args, sizeof(value));
	if(value != test)
	{
		stack_promote = 1;
		mem_copy(&dvalue, stack_args, sizeof(dvalue));
		if(dvalue != (double)test)
		{
			stack_promote = 2;
		}
	}

	/* Set the appropriate promotion rules */
	if(reg_promote)
	{
		/* Promoting "float" to "nfloat" in registers */
		if(num_nfloat_regs > 0)
		{
			if(pass_reg_nfloat_as_double)
			{
				pass_reg_float_as_double = 1;
			}
			else
			{
				pass_reg_float_as_nfloat = 1;
			}
		}
	}
	if(stack_promote == 2)
	{
		/* Promoting "float" to "nfloat" on the stack */
		if(pass_stack_nfloat_as_double)
		{
			pass_stack_float_as_double = 1;
		}
		else
		{
			pass_stack_float_as_nfloat = 1;
		}
	}
	else if(stack_promote)
	{
		/* Promoting "float" to "double" on the stack */
		pass_stack_float_as_double = 1;
	}
}

/*
 * Detect the stack promotion rules for "double" values.
 */
void detect_double_promotion(double arg1, double arg2, double arg3,
					         double arg4, double arg5, double arg6,
					         double arg7, double arg8, double arg9,
					         double arg10, double arg11, double arg12,
					         double arg13, double arg14, double arg15,
					         double arg16, double arg17, double arg18,
					         double arg19, double arg20, double arg21,
					         double arg22, double arg23, double arg24,
					         double arg25, double arg26, double arg27,
					         double arg28, double arg29, double arg30,
					         double arg31, double arg32)
{
	jit_nint *args, *stack_args;
	double value, test;
	int stack_promote;
	int index;

	/* Extract the arguments */
	jit_builtin_apply_args(jit_nint *, args);
	stack_args = (jit_nint *)(args[0]);
	stack_promote = 0;

	/* Skip the arguments that will be stored in registers */
	index = 1;
	if(doubles_in_word_regs)
	{
		if(sizeof(jit_nint) == sizeof(jit_int))
		{
			index += num_word_regs / 2;
		}
		else
		{
			index += num_word_regs;
		}
	}
	else if(num_float_regs > 0)
	{
		index += num_float_regs;
	}

	/* Get the value corresponding to argument "index" */
	switch(index)
	{
		case 1:			test = arg1; break;
		case 2:			test = arg2; break;
		case 3:			test = arg3; break;
		case 4:			test = arg4; break;
		case 5:			test = arg5; break;
		case 6:			test = arg6; break;
		case 7:			test = arg7; break;
		case 8:			test = arg8; break;
		case 9:			test = arg9; break;
		case 10:		test = arg10; break;
		case 11:		test = arg11; break;
		case 12:		test = arg12; break;
		case 13:		test = arg13; break;
		case 14:		test = arg14; break;
		case 15:		test = arg15; break;
		case 16:		test = arg16; break;
		case 17:		test = arg17; break;
		case 18:		test = arg18; break;
		case 19:		test = arg19; break;
		case 20:		test = arg20; break;
		case 30:		test = arg30; break;
		case 31:		test = arg31; break;
		case 32:		test = arg32; break;
		default:		test = (double)(-1.0); break;
	}

	/* Determine if stacked values are promoted to "nfloat" */
	mem_copy(&value, stack_args, sizeof(value));
	if(value != test)
	{
		stack_promote = 1;
	}

	/* Set the appropriate promotion rules */
	if(stack_promote)
	{
		/* Promoting "double" to "nfloat" on the stack */
		pass_stack_double_as_nfloat = 1;
	}
}

/*
 * Determine if variable arguments are always passed on the stack,
 * even if the values would otherwise fit into a register.
 */
void detect_varargs_on_stack(jit_nint start, ...)
{
	jit_nint *args, *stack_args;
	jit_builtin_apply_args(jit_nint *, args);
	stack_args = (jit_nint *)(args[0]);
	if(num_word_regs == 0)
	{
		varargs_on_stack = 1;
	}
	else if(stack_args[0] == 1)
	{
		varargs_on_stack = 1;
	}
}

/*
 * Dummy functions for helping detect the size and position of "float",
 * "double", and "long double" return values.
 */
float return_float(void)
{
	return (float)123.0;
}
double return_double(void)
{
	return (double)456.7;
}
jit_nfloat return_nfloat(void)
{
	return (jit_nfloat)8901.2;
}

/*
 * Detect the behaviour of floating-point values in return blocks.
 */
void detect_float_return(void)
{
	jit_nint *args;
	int offset;
	unsigned char *return_value;
	float float_value;
	double double_value;
	jit_nfloat nfloat_value;
	int float_size = 0;
	int double_size = 0;
	int nfloat_size = 0;
	float temp_float;
	double temp_double;
	jit_nfloat temp_nfloat;

	/* Allocate space for the outgoing arguments */
	jit_builtin_apply_args(jit_nint *, args);

	/* Call "return_float" and get its return structure */
	jit_builtin_apply(return_float, args, 0, 1, return_value);

	/* Find the location of the return value */
	/* and determine the size of the "float" return value */
	offset = 0;
	while(offset < 64)
	{
		mem_copy(&float_value, return_value + offset, sizeof(float));
		temp_float = (float)123.0;
		if(mem_cmp(&float_value, &temp_float, sizeof(float)))
		{
			mem_copy(&double_value, return_value + offset, sizeof(double));
			temp_double = (double)123.0;
			if(mem_cmp(&double_value, &temp_double, sizeof(double)))
			{
				mem_copy(&nfloat_value, return_value + offset,
						 sizeof(jit_nfloat));
				temp_nfloat = (jit_nfloat)123.0;
				if(!mem_cmp(&nfloat_value, &temp_nfloat, NFLOAT_SIGNIFICANT_BYTES))
				{
					float_size = 3;
					break;
				}
			}
			else
			{
				float_size = 2;
				break;
			}
		}
		else
		{
			float_size = 1;
			break;
		}
		offset += sizeof(void *);
	}

	/* Use the offset and size information to set the final parameters */
	return_floats_after = offset;
	if(float_size == 2)
	{
		return_float_as_double = 1;
	}
	else if(float_size == 3)
	{
		return_float_as_nfloat = 1;
	}

	/* Call "return_double" and get its return structure */
	jit_builtin_apply(return_double, args, 0, 1, return_value);

	/* Find the location of the return value */
	/* and determine the size of the "double" return value */
	offset = 0;
	while(offset < 64)
	{
		mem_copy(&double_value, return_value + offset, sizeof(double));
		temp_double = (double)456.7;
		if(mem_cmp(&double_value, &temp_double, sizeof(double)))
		{
			mem_copy(&nfloat_value, return_value + offset,
					 sizeof(jit_nfloat));
			temp_nfloat = (jit_nfloat)456.7;
			if(!mem_cmp(&nfloat_value, &temp_nfloat, NFLOAT_SIGNIFICANT_BYTES))
			{
				double_size = 3;
				break;
			}
		}
		else
		{
			double_size = 2;
			break;
		}
		offset += sizeof(void *);
	}

	/* Use the offset and size information to set the final parameters */
	return_doubles_after = offset;
	if(double_size == 3)
	{
		return_double_as_nfloat = 1;
	}

	/* Call "return_nfloat" and get its return structure */
	jit_builtin_apply(return_nfloat, args, 0, 1, return_value);

	/* Find the location of the return value */
	/* and determine the size of the "nfloat" return value */
	offset = 0;
	while(offset < 64)
	{
		mem_copy(&double_value, return_value + offset, sizeof(double));
		temp_double = (double)8901.2;
		if(mem_cmp(&double_value, &temp_double, sizeof(double)))
		{
			mem_copy(&nfloat_value, return_value + offset,
					 sizeof(jit_nfloat));
			temp_nfloat = (jit_nfloat)8901.2;
			if(!mem_cmp(&nfloat_value, &temp_nfloat, NFLOAT_SIGNIFICANT_BYTES))
			{
				nfloat_size = 3;
				break;
			}
		}
		else
		{
			nfloat_size = 2;
			break;
		}
		offset += sizeof(void *);
	}

	/* Use the offset and size information to set the final parameters */
	return_nfloats_after = offset;
	if(nfloat_size == 2)
	{
		return_nfloat_as_double = 1;
	}
}

/*
 * Detect whether small struct values are returned in registers.
 */
#define	declare_struct_test(n)	\
	struct detect_##n \
	{ \
		jit_sbyte value[(n)]; \
	}; \
	union detect_un_##n \
	{ \
		struct detect_##n d; \
		jit_nint value[64 / sizeof(jit_nint)]; \
	}; \
	struct detect_##n detect_struct_##n(void) \
	{ \
		struct detect_##n d; \
		mem_set(&d, 0xFF, sizeof(d)); \
		return d; \
	} \
	void run_detect_struct_##n(void) \
	{ \
		jit_nint *args; \
		volatile jit_nint stack[1]; \
		union detect_un_##n buffer; \
		void *apply_return; \
		jit_builtin_apply_args(jit_nint *, args); \
		args[0] = (jit_nint)stack; \
		stack[0] = (jit_nint)&buffer; \
		if(struct_return_special_reg || num_word_regs > 0) \
		{ \
			args[1] = (jit_nint)&buffer; \
			if(struct_reg_overlaps_word_reg) \
			{ \
				args[2] = (jit_nint)&buffer; \
			} \
		} \
		mem_set(&buffer, 0, sizeof(buffer)); \
		jit_builtin_apply(detect_struct_##n, args, \
						  sizeof(jit_nint), 0, apply_return); \
		if(buffer.d.value[0] == 0x00) \
		{ \
			struct_return_in_reg[(n) - 1] = 1; \
		} \
	}
#define	call_struct_test(n)	run_detect_struct_##n()
declare_struct_test(1);
declare_struct_test(2);
declare_struct_test(3);
declare_struct_test(4);
declare_struct_test(5);
declare_struct_test(6);
declare_struct_test(7);
declare_struct_test(8);
declare_struct_test(9);
declare_struct_test(10);
declare_struct_test(11);
declare_struct_test(12);
declare_struct_test(13);
declare_struct_test(14);
declare_struct_test(15);
declare_struct_test(16);
declare_struct_test(17);
declare_struct_test(18);
declare_struct_test(19);
declare_struct_test(20);
declare_struct_test(21);
declare_struct_test(22);
declare_struct_test(23);
declare_struct_test(24);
declare_struct_test(25);
declare_struct_test(26);
declare_struct_test(27);
declare_struct_test(28);
declare_struct_test(29);
declare_struct_test(30);
declare_struct_test(31);
declare_struct_test(32);
declare_struct_test(33);
declare_struct_test(34);
declare_struct_test(35);
declare_struct_test(36);
declare_struct_test(37);
declare_struct_test(38);
declare_struct_test(39);
declare_struct_test(40);
declare_struct_test(41);
declare_struct_test(42);
declare_struct_test(43);
declare_struct_test(44);
declare_struct_test(45);
declare_struct_test(46);
declare_struct_test(47);
declare_struct_test(48);
declare_struct_test(49);
declare_struct_test(50);
declare_struct_test(51);
declare_struct_test(52);
declare_struct_test(53);
declare_struct_test(54);
declare_struct_test(55);
declare_struct_test(56);
declare_struct_test(57);
declare_struct_test(58);
declare_struct_test(59);
declare_struct_test(60);
declare_struct_test(61);
declare_struct_test(62);
declare_struct_test(63);
declare_struct_test(64);
void detect_struct_conventions(void)
{
	/* Apply the structure return tests for all sizes from 1 to 64 */
	call_struct_test(1);
	call_struct_test(2);
	call_struct_test(3);
	call_struct_test(4);
	call_struct_test(5);
	call_struct_test(6);
	call_struct_test(7);
	call_struct_test(8);
	call_struct_test(9);
	call_struct_test(10);
	call_struct_test(11);
	call_struct_test(12);
	call_struct_test(13);
	call_struct_test(14);
	call_struct_test(15);
	call_struct_test(16);
#ifndef PLATFORM_IS_X86_64
	call_struct_test(17);
	call_struct_test(18);
	call_struct_test(19);
	call_struct_test(20);
	call_struct_test(21);
	call_struct_test(22);
	call_struct_test(23);
	call_struct_test(24);
	call_struct_test(25);
	call_struct_test(26);
	call_struct_test(27);
	call_struct_test(28);
	call_struct_test(29);
	call_struct_test(30);
	call_struct_test(31);
	call_struct_test(32);
	call_struct_test(33);
	call_struct_test(34);
	call_struct_test(35);
	call_struct_test(36);
	call_struct_test(37);
	call_struct_test(38);
	call_struct_test(39);
	call_struct_test(40);
	call_struct_test(41);
	call_struct_test(42);
	call_struct_test(43);
	call_struct_test(44);
	call_struct_test(45);
	call_struct_test(46);
	call_struct_test(47);
	call_struct_test(48);
	call_struct_test(49);
	call_struct_test(50);
	call_struct_test(51);
	call_struct_test(52);
	call_struct_test(53);
	call_struct_test(54);
	call_struct_test(55);
	call_struct_test(56);
	call_struct_test(57);
	call_struct_test(58);
	call_struct_test(59);
	call_struct_test(60);
	call_struct_test(61);
	call_struct_test(62);
	call_struct_test(63);
	call_struct_test(64);
#endif /* PLATFORM_IS_X86_64 */
}

/*
 * Detect alignment of "long" values in registers and on the stack.
 */
#ifdef JIT_NATIVE_INT32
void detect_reg_alignment_one_word(jit_long y, jit_long z)
{
	jit_nint *args, *stack_args;
	jit_builtin_apply_args(jit_nint *, args);
	stack_args = (jit_nint *)(args[0]);
	if(!mem_cmp(stack_args, &y, sizeof(y)))
	{
		/* The value of y was pushed out onto the stack, so we
		   cannot split long values between registers and the stack */
		can_split_long = 0;
	}
}
void detect_reg_alignment_two_words(jit_int x, jit_long y, jit_long z)
{
	jit_nint *args, *stack_args;
	jit_builtin_apply_args(jit_nint *, args);
	stack_args = (jit_nint *)(args[0]);
	if(!mem_cmp(stack_args, &y, sizeof(y)))
	{
		/* The value of y was pushed out, so alignment has occurred */
		can_split_long = 0;
		align_long_regs = 1;
	}
	else if(!mem_cmp(stack_args, ((jit_nint *)(void *)&y) + 1,
			sizeof(jit_nint)))
	{
		/* The long value was split between registers and the stack */
		can_split_long = 1;
	}
}
void detect_reg_alignment_three_words(jit_int x, jit_long y, jit_long z)
{
	jit_nint *args, *stack_args;
	jit_builtin_apply_args(jit_nint *, args);
	stack_args = (jit_nint *)(args[0]);
	if(!mem_cmp(stack_args, &y, sizeof(y)))
	{
		/* The value of y was pushed out, so alignment has occurred */
		can_split_long = 0;
		align_long_regs = 1;
	}
	else if(!mem_cmp(stack_args, ((jit_nint *)(void *)&y) + 1,
			sizeof(jit_nint)))
	{
		/* The long value was split between registers and the stack,
		   so alignment has occurred, together with a split */
		can_split_long = 1;
		align_long_regs = 1;
	}
}
void detect_reg_alignment_more_words(jit_int x, jit_long y, jit_long z)
{
	jit_nint *args, *word_regs;
	jit_builtin_apply_args(jit_nint *, args);
	word_regs = args + struct_return_special_reg + 1;
	if(!mem_cmp(word_regs + 2, &y, sizeof(y)))
	{
		/* The value of "y" was aligned within the word registers */
		align_long_regs = 1;
	}
}
void detect_reg_split_even_words(jit_int x, jit_long y1, jit_long y2,
								 jit_long y3, jit_long y4, jit_long y5,
								 jit_long y6, jit_long y7, jit_long y8,
								 jit_long y9, jit_long y10, jit_long y11,
								 jit_long y12, jit_long y13, jit_long y14,
								 jit_long y15, jit_long y16, jit_long y17,
								 jit_long y18, jit_long y19, jit_long y20)
{
	jit_nint *args, *stack_args;
	int posn;
	jit_long value;
	jit_builtin_apply_args(jit_nint *, args);
	stack_args = (jit_nint *)(args[0]);
	for(posn = 1; posn <= 20; ++posn)
	{
		switch(posn)
		{
			case 1:		value = y1; break;
			case 2:		value = y2; break;
			case 3:		value = y3; break;
			case 4:		value = y4; break;
			case 5:		value = y5; break;
			case 6:		value = y6; break;
			case 7:		value = y7; break;
			case 8:		value = y8; break;
			case 9:		value = y9; break;
			case 10:	value = y10; break;
			case 11:	value = y11; break;
			case 12:	value = y12; break;
			case 13:	value = y13; break;
			case 14:	value = y14; break;
			case 15:	value = y15; break;
			case 16:	value = y16; break;
			case 17:	value = y17; break;
			case 18:	value = y18; break;
			case 19:	value = y19; break;
			default:	value = y20; break;
		}
		if(!mem_cmp(stack_args, ((jit_nint *)(void *)&value) + 1,
					sizeof(jit_nint)))
		{
			/* We've detected a register/stack split in this argument */
			can_split_long = 1;
			break;
		}
	}
}
void detect_reg_split_odd_words(jit_long y1, jit_long y2,
								jit_long y3, jit_long y4, jit_long y5,
								jit_long y6, jit_long y7, jit_long y8,
								jit_long y9, jit_long y10, jit_long y11,
								jit_long y12, jit_long y13, jit_long y14,
								jit_long y15, jit_long y16, jit_long y17,
								jit_long y18, jit_long y19, jit_long y20)
{
	jit_nint *args, *stack_args;
	int posn;
	jit_long value;
	jit_builtin_apply_args(jit_nint *, args);
	stack_args = (jit_nint *)(args[0]);
	for(posn = 1; posn <= 20; ++posn)
	{
		switch(posn)
		{
			case 1:		value = y1; break;
			case 2:		value = y2; break;
			case 3:		value = y3; break;
			case 4:		value = y4; break;
			case 5:		value = y5; break;
			case 6:		value = y6; break;
			case 7:		value = y7; break;
			case 8:		value = y8; break;
			case 9:		value = y9; break;
			case 10:	value = y10; break;
			case 11:	value = y11; break;
			case 12:	value = y12; break;
			case 13:	value = y13; break;
			case 14:	value = y14; break;
			case 15:	value = y15; break;
			case 16:	value = y16; break;
			case 17:	value = y17; break;
			case 18:	value = y18; break;
			case 19:	value = y19; break;
			default:	value = y20; break;
		}
		if(!mem_cmp(stack_args, ((jit_nint *)(void *)&value) + 1,
					sizeof(jit_nint)))
		{
			/* We've detected a register/stack split in this argument */
			can_split_long = 1;
			break;
		}
	}
}
void detect_stack_align_even_words
					(jit_nint arg1, jit_nint arg2, jit_nint arg3,
					 jit_nint arg4, jit_nint arg5, jit_nint arg6,
					 jit_nint arg7, jit_nint arg8, jit_nint arg9,
					 jit_nint arg10, jit_nint arg11, jit_nint arg12,
					 jit_nint arg13, jit_nint arg14, jit_nint arg15,
					 jit_nint arg16, jit_nint arg17, jit_nint arg18,
					 jit_nint arg19, jit_nint arg20, jit_nint arg21,
					 jit_nint arg22, jit_nint arg23, jit_nint arg24,
					 jit_nint arg25, jit_nint arg26, jit_nint arg27,
					 jit_nint arg28, jit_nint arg29, jit_nint arg30,
					 jit_nint arg31, jit_nint arg32, jit_nint arg33,
					 jit_long y, jit_long z)
{
	jit_nint *args, *stack_args;
	int index;
	jit_builtin_apply_args(jit_nint *, args);
	stack_args = (jit_nint *)(args[0]);
	index = 33 - num_word_regs;
	if(!mem_cmp(stack_args + index + 1, &y, sizeof(y)))
	{
		align_long_stack = 1;
	}
}
void detect_stack_align_odd_words
					(jit_nint arg1, jit_nint arg2, jit_nint arg3,
					 jit_nint arg4, jit_nint arg5, jit_nint arg6,
					 jit_nint arg7, jit_nint arg8, jit_nint arg9,
					 jit_nint arg10, jit_nint arg11, jit_nint arg12,
					 jit_nint arg13, jit_nint arg14, jit_nint arg15,
					 jit_nint arg16, jit_nint arg17, jit_nint arg18,
					 jit_nint arg19, jit_nint arg20, jit_nint arg21,
					 jit_nint arg22, jit_nint arg23, jit_nint arg24,
					 jit_nint arg25, jit_nint arg26, jit_nint arg27,
					 jit_nint arg28, jit_nint arg29, jit_nint arg30,
					 jit_nint arg31, jit_nint arg32,
					 jit_long y, jit_long z)
{
	jit_nint *args, *stack_args;
	int index;
	jit_builtin_apply_args(jit_nint *, args);
	stack_args = (jit_nint *)(args[0]);
	index = 32 - num_word_regs;
	if(!mem_cmp(stack_args + index + 1, &y, sizeof(y)))
	{
		align_long_stack = 1;
	}
}
void detect_long_alignment()
{
	jit_long value1, value2;
	jit_long align_values[20];
	int posn;
	value1 = (((jit_long)0x01020304) << 32) | ((jit_long)0x05060708);
	value2 = (((jit_long)0x090A0B0C) << 32) | ((jit_long)0x0D0E0F00);
	if(num_word_regs == 1)
	{
		detect_reg_alignment_one_word(value1, value2);
	}
	else if(num_word_regs == 2)
	{
		detect_reg_alignment_two_words((jit_int)(-1), value1, value2);
	}
	else if(num_word_regs == 3)
	{
		detect_reg_alignment_three_words((jit_int)(-1), value1, value2);
	}
	else if(num_word_regs > 0)
	{
		detect_reg_alignment_more_words((jit_int)(-1), value1, value2);
	}
	else if(x86_fastcall)
	{
		/* We can split long values using FASTCALL under Win32 */
		can_split_long = 1;
	}
	for(posn = 0; posn < 20; ++posn)
	{
		align_values[posn] = ((jit_long)(posn + 4567)) +
						     (((jit_long)((posn + 1) * 127)) << 32);
	}
	if((num_word_regs % 2) == 0)
	{
		detect_reg_split_even_words
			((jit_int)(-1), align_values[0], align_values[1],
			 align_values[2], align_values[3], align_values[4],
			 align_values[5], align_values[6], align_values[7],
			 align_values[8], align_values[9], align_values[10],
			 align_values[11], align_values[12], align_values[13],
			 align_values[14], align_values[15], align_values[16],
			 align_values[17], align_values[18], align_values[19]);
		detect_stack_align_even_words
			(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			 value1, value2);
	}
	else
	{
		detect_reg_split_odd_words
			(align_values[0], align_values[1],
			 align_values[2], align_values[3], align_values[4],
			 align_values[5], align_values[6], align_values[7],
			 align_values[8], align_values[9], align_values[10],
			 align_values[11], align_values[12], align_values[13],
			 align_values[14], align_values[15], align_values[16],
			 align_values[17], align_values[18], align_values[19]);
		detect_stack_align_odd_words
			(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			 value1, value2);
	}
}
#else	/* JIT_NATIVE_INT64 */
void detect_long_alignment()
{
	/* Long values are always aligned on 64-bit architectures */
}
#endif	/* JIT_NATIVE_INT64 */

/*
 * Determine the maximum size for the apply structure.
 */
void detect_max_sizes(void)
{
	max_apply_size = (struct_return_special_reg + num_word_regs + 1)
						* sizeof(jit_nint);
	if(pass_reg_nfloat_as_double)
	{
		max_apply_size += num_float_regs * sizeof(double);
	}
	else
	{
		max_apply_size += num_float_regs * sizeof(jit_nfloat);
	}
	if(pad_float_regs)
	{
		max_apply_size += pad_float_regs * sizeof(jit_nint);
	}
	if(x86_fastcall && max_apply_size < 12)
	{
		max_apply_size = 12;
	}
}

/*
 * Detect the offsets of parent frame and return address pointers
 * in the values returned by "__builtin_frame_address".  We have to
 * do this carefully, to deal with architectures that don't create
 * a real frame for leaf functions.
 */
#if defined(PLATFORM_IS_GCC)
void find_frame_offset_inner(void *looking_for, void **frame)
{
	int offset;
	if(looking_for == (void *)frame || !frame)
	{
		/* Can happen on Alpha platforms */
		broken_frame_builtins = 1;
		return;
	}
	for(offset = 0; offset >= -8; --offset)
	{
		if(frame[offset] == looking_for)
		{
			parent_frame_offset = offset * sizeof(void *);
			return;
		}
	}
	for(offset = 1; offset <= 8; ++offset)
	{
		if(frame[offset] == looking_for)
		{
			parent_frame_offset = offset * sizeof(void *);
			return;
		}
	}
}
void find_frame_offset_outer(void *looking_for)
{
	void *frame_address;
#if defined(_JIT_ARCH_GET_CURRENT_FRAME)
	_JIT_ARCH_GET_CURRENT_FRAME(frame_address);
#else
	frame_address = __builtin_frame_address(0);
#endif
	find_frame_offset_inner(looking_for, (void **)frame_address);
}
void find_return_offset(void *looking_for, void **frame)
{
	int offset;
	if(broken_frame_builtins)
	{
		return;
	}
	for(offset = 1; offset <= 8; ++offset)
	{
		if(frame[offset] == looking_for)
		{
			return_address_offset = offset * sizeof(void *);
			return;
		}
	}
	for(offset = 0; offset >= -8; --offset)
	{
		if(frame[offset] == looking_for)
		{
			return_address_offset = offset * sizeof(void *);
			return;
		}
	}
}
void detect_frame_offsets(void)
{
	void *frame_address, *return_address;
#if defined(_JIT_ARCH_GET_CURRENT_FRAME)
	_JIT_ARCH_GET_CURRENT_FRAME(frame_address);
#else
	frame_address = __builtin_frame_address(0);
#endif
	return_address = __builtin_return_address(0);
	find_frame_offset_outer(frame_address);
	find_return_offset(return_address, frame_address);
	if(parent_frame_offset == 0 && return_address_offset == 0)
	{
		/* Can happen on platforms like ia64 where there are so
		   many registers that the frame is almost never concrete */
		broken_frame_builtins = 1;
	}
}
#else
void detect_frame_offsets(void)
{
	/* We don't know how to detect the offsets, so assume some defaults */
	parent_frame_offset = 0;
	return_address_offset = sizeof(void *);
}
#endif

/*
 * Dump the definition of the "jit_apply_return" union, which defines
 * the layout of the return value from "__builtin_apply".
 */
void dump_return_union(void)
{
	const char *float_type;
	const char *double_type;
	const char *nfloat_type;

	/* Determine the float return types */
	if(return_float_as_nfloat)
	{
		float_type = "jit_nfloat";
	}
	else if(return_float_as_double)
	{
		float_type = "double";
	}
	else
	{
		float_type = "float";
	}
	if(return_double_as_nfloat)
	{
		double_type = "jit_nfloat";
	}
	else
	{
		double_type = "double";
	}
	if(return_nfloat_as_double)
	{
		nfloat_type = "double";
	}
	else
	{
		nfloat_type = "jit_nfloat";
	}

	/* Dump the definition of "jit_apply_return" */
	printf("typedef union\n{\n");
	printf("\tjit_nint int_value;\n");
	printf("\tjit_nuint uint_value;\n");
	printf("\tjit_long long_value;\n");
	printf("\tjit_ulong ulong_value;\n");
	if(return_floats_after)
	{
		printf("\tstruct { jit_ubyte pad[%d]; %s f_value; } float_value;\n",
			   return_floats_after, float_type);
	}
	else
	{
		printf("\tstruct { %s f_value; } float_value;\n", float_type);
	}
	if(return_doubles_after)
	{
		printf("\tstruct { jit_ubyte pad[%d]; %s f_value; } double_value;\n",
			   return_doubles_after, double_type);
	}
	else
	{
		printf("\tstruct { %s f_value; } double_value;\n", double_type);
	}
	if(return_nfloats_after)
	{
		printf("\tstruct { jit_ubyte pad[%d]; %s f_value; } nfloat_value;\n",
			   return_nfloats_after, nfloat_type);
	}
	else
	{
		printf("\tstruct { %s f_value; } nfloat_value;\n", nfloat_type);
	}
	if(max_struct_in_reg > 0)
	{
		printf("\tjit_ubyte small_struct_value[%d];\n", max_struct_in_reg);
	}
	printf("\n} jit_apply_return;\n\n");

	/* Output access macros for manipulating the contents */
	printf("#define jit_apply_return_get_sbyte(result)\t\\\n");
	printf("\t((jit_sbyte)((result)->int_value))\n");
	printf("#define jit_apply_return_get_ubyte(result)\t\\\n");
	printf("\t((jit_ubyte)((result)->int_value))\n");
	printf("#define jit_apply_return_get_short(result)\t\\\n");
	printf("\t((jit_short)((result)->int_value))\n");
	printf("#define jit_apply_return_get_ushort(result)\t\\\n");
	printf("\t((jit_ushort)((result)->int_value))\n");
	printf("#define jit_apply_return_get_int(result)\t\\\n");
	printf("\t((jit_int)((result)->int_value))\n");
	printf("#define jit_apply_return_get_uint(result)\t\\\n");
	printf("\t((jit_uint)((result)->uint_value))\n");
	printf("#define jit_apply_return_get_nint(result)\t\\\n");
	printf("\t((jit_nint)((result)->int_value))\n");
	printf("#define jit_apply_return_get_nuint(result)\t\\\n");
	printf("\t((jit_nuint)((result)->uint_value))\n");
	printf("#define jit_apply_return_get_long(result)\t\\\n");
	printf("\t((jit_long)((result)->long_value))\n");
	printf("#define jit_apply_return_get_ulong(result)\t\\\n");
	printf("\t((jit_ulong)((result)->ulong_value))\n");
	printf("#define jit_apply_return_get_float32(result)\t\\\n");
	printf("\t((jit_float32)((result)->float_value.f_value))\n");
	printf("#define jit_apply_return_get_float64(result)\t\\\n");
	printf("\t((jit_float64)((result)->double_value.f_value))\n");
	printf("#define jit_apply_return_get_nfloat(result)\t\\\n");
	printf("\t((jit_nfloat)((result)->nfloat_value.f_value))\n");
	printf("\n");
	printf("#define jit_apply_return_set_sbyte(result,value)\t\\\n");
	printf("\t(((result)->int_value) = ((jit_nint)(value)))\n");
	printf("#define jit_apply_return_set_ubyte(result,value)\t\\\n");
	printf("\t(((result)->int_value) = ((jit_nint)(value)))\n");
	printf("#define jit_apply_return_set_short(result,value)\t\\\n");
	printf("\t(((result)->int_value) = ((jit_nint)(value)))\n");
	printf("#define jit_apply_return_set_ushort(result,value)\t\\\n");
	printf("\t(((result)->int_value) = ((jit_nint)(value)))\n");
	printf("#define jit_apply_return_set_int(result,value)\t\\\n");
	printf("\t(((result)->int_value) = ((jit_nint)(value)))\n");
	printf("#define jit_apply_return_set_uint(result,value)\t\\\n");
	printf("\t(((result)->uint_value) = ((jit_nuint)(value)))\n");
	printf("#define jit_apply_return_set_nint(result,value)\t\\\n");
	printf("\t(((result)->int_value) = ((jit_nint)(value)))\n");
	printf("#define jit_apply_return_set_nuint(result,value)\t\\\n");
	printf("\t(((result)->uint_value) = ((jit_nuint)(value)))\n");
	printf("#define jit_apply_return_set_long(result,value)\t\\\n");
	printf("\t(((result)->long_value) = ((jit_long)(value)))\n");
	printf("#define jit_apply_return_set_ulong(result,value)\t\\\n");
	printf("\t(((result)->ulong_value) = ((jit_ulong)(value)))\n");
	printf("#define jit_apply_return_set_float32(result,value)\t\\\n");
	printf("\t(((result)->float_value.f_value) = ((%s)(value)))\n",
		   float_type);
	printf("#define jit_apply_return_set_float64(result,value)\t\\\n");
	printf("\t(((result)->double_value.f_value) = ((%s)(value)))\n",
		   double_type);
	printf("#define jit_apply_return_set_nfloat(result,value)\t\\\n");
	printf("\t(((result)->nfloat_value.f_value) = ((%s)(value)))\n",
		   nfloat_type);
	printf("\n");
}

/*
 * Dump the details of the "apply" structure.
 */
void dump_apply_structure(void)
{
	const char *name;
	if(num_float_regs > 0)
	{
		if(pass_reg_float_as_double)
		{
			name = "jit_float64";
		}
		else if(pass_reg_float_as_nfloat)
		{
			name = "jit_nfloat";
		}	
		else
		{
			name = "jit_float32";
		}
		printf("typedef %s jit_reg_float;\n\n", name);
	}
	if(num_double_regs > 0)
	{
		if(pass_reg_double_as_nfloat)
		{
			name = "jit_nfloat";
		}
		else
		{
			name = "jit_float64";
		}
		printf("typedef %s jit_reg_double;\n\n", name);
	}
	if(num_nfloat_regs > 0)
	{
		if(pass_reg_nfloat_as_double)
		{
			name = "jit_float64";
		}
		else
		{
			name = "jit_nfloat";
		}
		printf("typedef %s jit_reg_nfloat;\n\n", name);
	}
	if((num_float_regs > 0) ||
	   (num_double_regs > 0) ||
	   (num_nfloat_regs > 0))
	{
		printf("typedef union\n{\n");
		if(num_float_regs > 0)
		{
			printf("\tjit_reg_float float_value;\n");
		}
		if(num_double_regs > 0)
		{
			printf("\tjit_reg_double double_value;\n");
		}
		if(num_nfloat_regs > 0)
		{
			printf("\tjit_reg_nfloat nfloat_value;\n");
		}
		if(pad_float_regs > 0)
		{
			printf("\tchar __pad[%i];\n",
				   (int)(sizeof(double) + pad_float_regs * sizeof(jit_nint)));
		}
		printf("} jit_reg_float_struct;\n\n");
	}
	printf("typedef struct\n{\n");
	printf("\tunsigned char *stack_args;\n");
	if(struct_return_special_reg)
	{
		printf("\tvoid *struct_ptr;\n");
	}
	if(num_word_regs > 0)
	{
		printf("\tjit_nint word_regs[%d];\n", num_word_regs);
	}
	else if(x86_fastcall)
	{
		printf("\tjit_nint word_regs[2];\n");
	}
	if(pad_float_regs)
	{
		printf("\tjit_nint pad[%d];\n", pad_float_regs);
	}
	if((num_float_regs > 0) ||
	   (num_double_regs > 0) ||
	   (num_nfloat_regs > 0))
	{
		printf("\tjit_reg_float_struct float_regs[%d];\n", num_float_regs);
	}
	printf("\n} jit_apply_struct;\n\n");
}

/*
 * Dump macro definitions that are used to build the apply parameter block.
 */
void dump_apply_macros(void)
{
	int apply_size;
	const char *name;
	const char *word_reg_limit;
	char buf[32];

	/* Declare the "jit_apply_builder" structure */
	printf("typedef struct\n{\n");
	printf("\tjit_apply_struct *apply_args;\n");
	printf("\tunsigned int stack_used;\n");
	if(num_word_regs > 0 || x86_fastcall)
	{
		printf("\tunsigned int word_used;\n");
		if(x86_fastcall)
		{
			printf("\tunsigned int word_max;\n");
		}
	}
	if(num_float_regs > 0)
	{
		printf("\tunsigned int float_used;\n");
	}
	printf("\tvoid *struct_return;\n");
	printf("\n} jit_apply_builder;\n\n");

	/* Include jit-apply-func.h here to allow the backend to add it's definitions */
	printf("#include \"jit-apply-func.h\"\n\n");

	printf("void\n_jit_builtin_apply_add_struct(jit_apply_builder *builder, void *value, jit_type_t struct_type);\n\n");

	printf("void\n_jit_builtin_apply_get_struct(jit_apply_builder *builder, void *value, jit_type_t struct_type);\n\n");

	printf("void\n_jit_builtin_apply_get_struct_return(jit_apply_builder *builder, void *return_value, jit_apply_return *apply_return, jit_type_t struct_type);\n\n");

	/* Macro to initialize the apply builder */
	printf("#define jit_apply_builder_init(builder,type)\t\\\n");
	printf("\tdo { \\\n");
	apply_size = max_apply_size;
	printf("\t\t(builder)->apply_args = (jit_apply_struct *)alloca(sizeof(jit_apply_struct)); \\\n");
	if(apply_size > sizeof(void *))
	{
		printf("\t\tjit_memset((builder)->apply_args, 0, %d); \\\n", apply_size);
	}
	printf("\t\t(builder)->apply_args->stack_args = (unsigned char *)alloca(jit_type_get_max_arg_size((type))); \\\n");
	printf("\t\t(builder)->stack_used = 0; \\\n");
	if(x86_fastcall)
	{
		printf("\t\t(builder)->word_used = 0; \\\n");
		printf("\t\tif(jit_type_get_abi((type)) == jit_abi_fastcall) \\\n");
		printf("\t\t\t(builder)->word_max = 2; \\\n");
		printf("\t\telse; \\\n");
		printf("\t\t\t(builder)->word_max = 0; \\\n");
		word_reg_limit = "(builder)->word_max";
	}
	else if(num_word_regs > 0)
	{
		printf("\t\t(builder)->word_used = 0; \\\n");
		sprintf(buf, "%d", num_word_regs);
		word_reg_limit = buf;
	}
	else
	{
		word_reg_limit = "???";
	}
	if(num_float_regs > 0)
	{
		printf("\t\t(builder)->float_used = 0; \\\n");
	}
	printf("\t\t(builder)->struct_return = 0; \\\n");
	printf("\t} while (0)\n\n");

	/* Macro to initialize the apply builder in closure parse mode.
	   The "args" parameter is the result of calling "__builtin_apply_args" */
	printf("#define jit_apply_parser_init(builder,type,args)\t\\\n");
	printf("\tdo { \\\n");
	printf("\t\t(builder)->apply_args = (jit_apply_struct *)(args); \\\n");
	printf("\t\t(builder)->stack_used = 0; \\\n");
	if(x86_fastcall)
	{
		printf("\t\t(builder)->word_used = 0; \\\n");
		printf("\t\tif(jit_type_get_abi((type)) == jit_abi_fastcall) \\\n");
		printf("\t\t\t(builder)->word_max = 2; \\\n");
		printf("\t\telse; \\\n");
		printf("\t\t\t(builder)->word_max = 0; \\\n");
	}
	else if(num_word_regs > 0)
	{
		printf("\t\t(builder)->word_used = 0; \\\n");
	}
	if(num_float_regs > 0)
	{
		printf("\t\t(builder)->float_used = 0; \\\n");
	}
	printf("\t\t(builder)->struct_return = 0; \\\n");
	printf("\t} while (0)\n\n");

	/* Macro to add a word argument to the apply parameters */
	printf("#define jit_apply_builder_add_word(builder,value) \\\n");
	printf("\tdo { \\\n");
	if(num_word_regs > 0 || x86_fastcall)
	{
		printf("\t\tif((builder)->word_used < %s) \\\n", word_reg_limit);
		printf("\t\t{ \\\n");
		printf("\t\t\t(builder)->apply_args->word_regs[(builder)->word_used] = (jit_nint)(value); \\\n");
		printf("\t\t\t++((builder)->word_used); \\\n");
		if(struct_reg_overlaps_word_reg)
		{
			/* We need to set the struct register slot as well */
			printf("\t\t\tif((builder)->word_used == 1) \\\n");
			printf("\t\t\t{ \\\n");
			printf("\t\t\t\t(builder)->apply_args->struct_ptr = (void *)(jit_nint)(value); \\\n");
			printf("\t\t\t} \\\n");
		}
		printf("\t\t} \\\n");
		printf("\t\telse \\\n");
		printf("\t\t{ \\\n");
		printf("\t\t\t*((jit_nint*)((builder)->apply_args->stack_args + (builder)->stack_used)) = (jit_nint)(value); \\\n");
		printf("\t\t\t(builder)->stack_used += sizeof(jit_nint); \\\n");
		printf("\t\t} \\\n");
	}
	else
	{
		printf("\t\t*((jit_nint*)((builder)->apply_args->stack_args + (builder)->stack_used)) = (jit_nint)(value); \\\n");
		printf("\t\t(builder)->stack_used += sizeof(jit_nint); \\\n");
	}
	printf("\t} while (0)\n\n");

	/* Macro to get a word argument from the apply parameters */
	printf("#define jit_apply_parser_get_word(builder,type,value) \\\n");
	printf("\tdo { \\\n");
	if(num_word_regs > 0 || x86_fastcall)
	{
		printf("\t\tif((builder)->word_used < %s) \\\n", word_reg_limit);
		printf("\t\t{ \\\n");
		printf("\t\t\t(value) = (type)((builder)->apply_args->word_regs[(builder)->word_used]); \\\n");
		printf("\t\t\t++((builder)->word_used); \\\n");
		printf("\t\t} \\\n");
		printf("\t\telse \\\n");
		printf("\t\t{ \\\n");
		printf("\t\t\t(value) = (type)(*((jit_nint*)((builder)->apply_args->stack_args + (builder)->stack_used))); \\\n");
		printf("\t\t\t(builder)->stack_used += sizeof(jit_nint); \\\n");
		printf("\t\t} \\\n");
	}
	else
	{
		printf("\t\t(value) = (type)(*((jit_nint*)((builder)->apply_args->stack_args + (builder)->stack_used))); \\\n");
		printf("\t\t(builder)->stack_used += sizeof(jit_nint); \\\n");
	}
	printf("\t} while (0)\n\n");

	/* Macro to align the word registers for a "long" value */
	printf("#define jit_apply_builder_align_regs(builder,num_words,align) \\\n");
	if((align_long_regs || !can_split_long) &&
	   (num_word_regs > 0 || x86_fastcall))
	{
		printf("\tdo { \\\n");
		printf("\t\tif((align) > sizeof(jit_nint) && (num_words) > 1) \\\n");
		printf("\t\t{ \\\n");
		if(align_long_regs)
		{
			printf("\t\t\tif(((builder)->word_used %% 2) == 1) \\\n");
			printf("\t\t\t{ \\\n");
			printf("\t\t\t\t++((builder)->word_used); \\\n");
			printf("\t\t\t} \\\n");
		}
		if(!can_split_long)
		{
			printf("\t\t\tif((%s - (builder)->word_used) < (num_words)) \\\n", word_reg_limit);
			printf("\t\t\t{ \\\n");
			printf("\t\t\t\t(builder)->word_used = %s; \\\n", word_reg_limit);
			printf("\t\t\t} \\\n");
		}
		printf("\t\t} \\\n");
		printf("\t} while (0)\n\n");
	}
	else
	{
		printf("\tdo { ; } while (0)\n\n");
	}

	/* Macro to align the stack for a "long" value */
	printf("#define jit_apply_builder_align_stack(builder,num_words,align) \\\n");
	if(align_long_stack)
	{
		printf("\tdo { \\\n");
		printf("\t\tif((align) > sizeof(jit_nint) && (num_words) > 1) \\\n");
		printf("\t\t{ \\\n");
		printf("\t\t\tif(((builder)->stack_used %% 2) == 1) \\\n");
		printf("\t\t\t{ \\\n");
		printf("\t\t\t\t++((builder)->stack_used); \\\n");
		printf("\t\t\t} \\\n");
		printf("\t\t} \\\n");
		printf("\t} while (0)\n\n");
	}
	else
	{
		printf("\tdo { ; } while (0)\n\n");
	}

	/* Macro to add a large (e.g. dword) argument to the apply parameters */
	printf("#define jit_apply_builder_add_large_inner(builder,ptr,size,align) \\\n");
	printf("\tdo { \\\n");
	printf("\t\tunsigned int __num_words = ((size) + sizeof(jit_nint) - 1) / sizeof(jit_nint); \\\n");
	if(num_word_regs > 0 || x86_fastcall)
	{
		printf("\t\tjit_apply_builder_align_regs((builder), __num_words, (align)); \\\n");
		printf("\t\tif((%s - (builder)->word_used) >= __num_words) \\\n", word_reg_limit);
		printf("\t\t{ \\\n");
		printf("\t\t\tjit_memcpy((builder)->apply_args->word_regs + (builder)->word_used, (ptr), (size)); \\\n");
		printf("\t\t\t(builder)->word_used += __num_words; \\\n");
		printf("\t\t} \\\n");
		printf("\t\telse if((builder)->word_used < %s) \\\n", word_reg_limit);
		printf("\t\t{ \\\n");
		printf("\t\t\tunsigned int __split = (%s - (builder)->word_used); \\\n", word_reg_limit);
		printf("\t\t\tjit_memcpy((builder)->apply_args->word_regs + (builder)->word_used, (ptr), __split * sizeof(jit_nint)); \\\n");
		printf("\t\t\tjit_memcpy((builder)->apply_args->stack_args, ((jit_nint *)(ptr)) + __split, (size) - __split * sizeof(jit_nint)); \\\n");
		printf("\t\t\t(builder)->word_used = %s; \\\n", word_reg_limit);
		printf("\t\t\t(builder)->stack_used = __num_words - __split; \\\n");
		printf("\t\t} \\\n");
		printf("\t\telse \\\n");
		printf("\t\t{ \\\n");
		printf("\t\t\tjit_apply_builder_align_stack((builder), __num_words, (align)); \\\n");
		printf("\t\t\tjit_memcpy((builder)->apply_args->stack_args + (builder)->stack_used, (ptr), (size)); \\\n");
		printf("\t\t\t(builder)->stack_used += __num_words * sizeof(jit_nint); \\\n");
		printf("\t\t\t(builder)->word_used = %s; \\\n", word_reg_limit);
		printf("\t\t} \\\n");
	}
	else
	{
		printf("\t\tjit_apply_builder_align_stack((builder), __num_words, (align)); \\\n");
		printf("\t\tjit_memcpy((builder)->apply_args->stack_args + (builder)->stack_used, (ptr), (size)); \\\n");
		printf("\t\t(builder)->stack_used += __num_words * sizeof(jit_nint); \\\n");
	}
	printf("\t} while (0)\n\n");
	printf("#define jit_apply_builder_add_large(builder,type,value) \\\n");
	printf("\tdo { \\\n");
	printf("\t\ttype __temp = (type)(value); \\\n");
	printf("\t\tjit_apply_builder_add_large_inner((builder), &__temp, sizeof(__temp), sizeof(jit_nint)); \\\n");
	printf("\t} while (0)\n\n");

	/* Macro to get a large (e.g. dword) argument from the apply parameters */
	printf("#define jit_apply_parser_get_large(builder,type,finaltype,value) \\\n");
	printf("\tdo { \\\n");
	printf("\t\ttype __temp; \\\n");
	printf("\t\tunsigned int __num_words = (sizeof(__temp) + sizeof(jit_nint) - 1) / sizeof(jit_nint); \\\n");
	if(num_word_regs > 0 || x86_fastcall)
	{
		printf("\t\tjit_apply_builder_align_regs((builder), __num_words, sizeof(type)); \\\n");
		printf("\t\tif((%s - (builder)->word_used) >= __num_words) \\\n", word_reg_limit);
		printf("\t\t{ \\\n");
		printf("\t\t\tjit_memcpy(&__temp, (builder)->apply_args->word_regs + (builder)->word_used, sizeof(__temp)); \\\n");
		printf("\t\t\t(builder)->word_used += __num_words; \\\n");
		printf("\t\t} \\\n");
		printf("\t\telse if((builder)->word_used < %s) \\\n", word_reg_limit);
		printf("\t\t{ \\\n");
		printf("\t\t\tunsigned int __split = (%s - (builder)->word_used); \\\n", word_reg_limit);
		printf("\t\t\tjit_memcpy(&__temp, (builder)->apply_args->word_regs + (builder)->word_used, __split * sizeof(jit_nint)); \\\n");
		printf("\t\t\tjit_memcpy(((jit_nint *)&__temp) + __split, (builder)->apply_args->stack_args, (__num_words - __split) * sizeof(jit_nint)); \\\n");
		printf("\t\t\t(builder)->word_used = %s; \\\n", word_reg_limit);
		printf("\t\t\t(builder)->stack_used = __num_words - __split; \\\n");
		printf("\t\t} \\\n");
		printf("\t\telse \\\n");
		printf("\t\t{ \\\n");
		printf("\t\t\tjit_apply_builder_align_stack((builder), __num_words, sizeof(type)); \\\n");
		printf("\t\t\tjit_memcpy(&__temp, (builder)->apply_args->stack_args + (builder)->stack_used, sizeof(__temp)); \\\n");
		printf("\t\t\t(builder)->stack_used += __num_words * sizeof(jit_nint); \\\n");
		printf("\t\t\t(builder)->word_used = %s; \\\n", word_reg_limit);
		printf("\t\t} \\\n");
	}
	else
	{
		printf("\t\tjit_apply_builder_align_stack((builder), __num_words, sizeof(type)); \\\n");
		printf("\t\tjit_memcpy(&__temp, (builder)->apply_args->stack_args + (builder)->stack_used, sizeof(__temp)); \\\n");
		printf("\t\t(builder)->stack_used += __num_words * sizeof(jit_nint); \\\n");
	}
	printf("\t\t(value) = (finaltype)(__temp); \\\n");
	printf("\t} while (0)\n\n");

	/* Macro to add a large (e.g. dword) argument to the apply parameters
	   on the stack, ignoring word registers */
	printf("#define jit_apply_builder_add_large_stack(builder,type,value) \\\n");
	printf("\tdo { \\\n");
	printf("\t\ttype __temp = (type)(value); \\\n");
	printf("\t\tunsigned int __num_words = (sizeof(__temp) + sizeof(jit_nint) - 1) / sizeof(jit_nint); \\\n");
	printf("\t\tjit_apply_builder_align_stack((builder), __num_words, sizeof(type)); \\\n");
	printf("\t\tjit_memcpy((builder)->apply_args->stack_args + (builder)->stack_used, &__temp, sizeof(__temp)); \\\n");
	printf("\t\t(builder)->stack_used += __num_words * sizeof(jit_nint); \\\n");
	printf("\t} while (0)\n\n");

	/* Macro to get a large (e.g. dword) argument from the apply parameters
	   on the stack, ignoring word registers */
	printf("#define jit_apply_parser_get_large_stack(builder,type,finaltype,value) \\\n");
	printf("\tdo { \\\n");
	printf("\t\ttype __temp; \\\n");
	printf("\t\tunsigned int __num_words = (sizeof(__temp) + sizeof(jit_nint) - 1) / sizeof(jit_nint); \\\n");
	printf("\t\tjit_apply_builder_align_stack((builder), __num_words, sizeof(type)); \\\n");
	printf("\t\tjit_memcpy(&__temp, (builder)->apply_args->stack_args + (builder)->stack_used, sizeof(__temp)); \\\n");
	printf("\t\t(builder)->stack_used += __num_words * sizeof(jit_nint); \\\n");
	printf("\t\t(value) = (finaltype)(__temp); \\\n");
	printf("\t} while (0)\n\n");

	/* Macro to set the structure return area */
	printf("#define jit_apply_builder_add_struct_return(builder,size,return_buf) \\\n");
	printf("\tdo { \\\n");
	printf("\t\tunsigned int __struct_size = (unsigned int)(size); \\\n");
	printf("\t\tif(__struct_size >= 1 && __struct_size <= 64 && \\\n");
	printf("\t\t   (_jit_apply_return_in_reg[(__struct_size - 1) / 8] \\\n");
	printf("\t\t       & (1 << ((__struct_size - 1) %% 8))) != 0) \\\n");
	printf("\t\t{ \\\n");
	printf("\t\t\t(builder)->struct_return = 0; \\\n");
	printf("\t\t} \\\n");
	printf("\t\telse \\\n");
	printf("\t\t{ \\\n");
	printf("\t\t\tif((return_buf) != 0) \\\n");
	printf("\t\t\t\t(builder)->struct_return = (void *)(return_buf); \\\n");
	printf("\t\t\telse \\\n");
	printf("\t\t\t\t(builder)->struct_return = alloca(__struct_size); \\\n");
	if(struct_return_special_reg && !struct_reg_overlaps_word_reg)
	{
		printf("\t\t\t(builder)->apply_args->struct_ptr = (builder)->struct_return; \\\n");
	}
	else
	{
		printf("\t\t\tjit_apply_builder_add_word((builder), (builder)->struct_return); \\\n");
	}
	printf("\t\t} \\\n");
	printf("\t} while (0)\n\n");

	/* Macro to extract the structure return value, if it is in registers */
	printf("#define jit_apply_builder_get_struct_return(builder,size,return_buf,apply_return) \\\n");
	printf("\tdo { \\\n");
	printf("\t\tif(!((builder)->struct_return)) \\\n");
	printf("\t\t{ \\\n");
	printf("\t\t\tjit_memcpy((return_buf), (apply_return), (size)); \\\n");
	printf("\t\t} \\\n");
	printf("\t\telse if((builder)->struct_return != (void *)(return_buf)) \\\n");
	printf("\t\t{ \\\n");
	printf("\t\t\tjit_memcpy((return_buf), (builder)->struct_return, (size)); \\\n");
	printf("\t\t} \\\n");
	printf("\t} while (0)\n\n");

	/* Macro to start the vararg area */
	printf("#define jit_apply_builder_start_varargs(builder) \\\n");
	printf("\tdo { \\\n");
	if(varargs_on_stack)
	{
		if(num_word_regs > 0 || x86_fastcall)
		{
			printf("\t\t(builder)->word_used = %s; \\\n", word_reg_limit);
		}
		if(num_float_regs > 0)
		{
			printf("\t\t(builder)->float_used = %d; \\\n", num_float_regs);
		}
	}
	printf("\t} while (0)\n\n");

	/* Macro to start the vararg area when parsing a closure */
	printf("#define jit_apply_parser_start_varargs(builder) \\\n");
	printf("\tdo { \\\n");
	if(varargs_on_stack)
	{
		if(num_word_regs > 0 || x86_fastcall)
		{
			printf("\t\t(builder)->word_used = %s; \\\n", word_reg_limit);
		}
		if(num_float_regs > 0)
		{
			printf("\t\t(builder)->float_used = %d; \\\n", num_float_regs);
		}
	}
	printf("\t} while (0)\n\n");

	/* Add parameter values of various types */
	printf("#define jit_apply_builder_add_sbyte(builder,value) \\\n");
	printf("\tjit_apply_builder_add_word((builder), (value));\n");
	printf("#define jit_apply_builder_add_ubyte(builder,value) \\\n");
	printf("\tjit_apply_builder_add_word((builder), (value));\n");
	printf("#define jit_apply_builder_add_short(builder,value) \\\n");
	printf("\tjit_apply_builder_add_word((builder), (value));\n");
	printf("#define jit_apply_builder_add_ushort(builder,value) \\\n");
	printf("\tjit_apply_builder_add_word((builder), (value));\n");
	printf("#define jit_apply_builder_add_int(builder,value) \\\n");
	printf("\tjit_apply_builder_add_word((builder), (value));\n");
	printf("#define jit_apply_builder_add_uint(builder,value) \\\n");
	printf("\tjit_apply_builder_add_word((builder), (jit_nuint)(value));\n");
	printf("#define jit_apply_builder_add_nint(builder,value) \\\n");
	printf("\tjit_apply_builder_add_word((builder), (value));\n");
	printf("#define jit_apply_builder_add_nuint(builder,value) \\\n");
	printf("\tjit_apply_builder_add_word((builder), (jit_nuint)(value));\n");
#ifdef JIT_NATIVE_INT32
	printf("#define jit_apply_builder_add_long(builder,value) \\\n");
	printf("\tjit_apply_builder_add_large((builder), jit_long, (value));\n");
	printf("#define jit_apply_builder_add_ulong(builder,value) \\\n");
	printf("\tjit_apply_builder_add_large((builder), jit_ulong, (value));\n");
#else
	printf("#define jit_apply_builder_add_long(builder,value) \\\n");
	printf("\tjit_apply_builder_add_word((builder), (value));\n");
	printf("#define jit_apply_builder_add_ulong(builder,value) \\\n");
	printf("\tjit_apply_builder_add_word((builder), (jit_nuint)(value));\n");
#endif
	if(num_float_regs > 0)
	{
		/* Pass floating point values in registers, if possible */
		printf("#define jit_apply_builder_add_float32(builder,value) \\\n");
		printf("\tdo { \\\n");
		printf("\t\tif((builder)->float_used < %d) \\\n", num_float_regs);
		printf("\t\t{ \\\n");
		printf("\t\t\t(builder)->apply_args->float_regs[(builder)->float_used].float_value = (jit_reg_float)(value); \\\n");
		printf("\t\t\t++((builder)->float_used); \\\n");
		printf("\t\t} \\\n");
		printf("\t\telse \\\n");
		printf("\t\t{ \\\n");
		if(pass_stack_float_as_double)
			name = "jit_float64";
		else if(pass_stack_float_as_nfloat)
			name = "jit_nfloat";
		else
			name = "jit_float32";
		printf("\t\t\t%s __temp = (%s)(value); \\\n", name, name);
		printf("\t\t\tjit_memcpy((builder)->apply_args->stack_args + (builder)->stack_used, &__temp, sizeof(__temp)); \\\n");
		printf("\t\t\t(builder)->stack_used += (sizeof(%s) + sizeof(jit_nint) - 1) & ~(sizeof(jit_nint) - 1); \\\n", name);
		printf("\t\t} \\\n");
		printf("\t} while (0)\n");
	}
	else if(floats_in_word_regs)
	{
		/* Pass floating point values in word registers */
		if(pass_reg_float_as_double)
			name = "jit_float64";
		else if(pass_reg_float_as_nfloat)
			name = "jit_nfloat";
		else
			name = "jit_float32";
		printf("#define jit_apply_builder_add_float32(builder,value) \\\n");
		printf("\tjit_apply_builder_add_large((builder), %s, (value));\n", name);
	}
	else
	{
		/* Pass floating point values on the stack */
		if(pass_stack_float_as_double)
			name = "jit_float64";
		else if(pass_stack_float_as_nfloat)
			name = "jit_nfloat";
		else
			name = "jit_float32";
		printf("#define jit_apply_builder_add_float32(builder,value) \\\n");
		printf("\tjit_apply_builder_add_large_stack((builder), %s, (value));\n", name);
	}
	if(num_double_regs > 0)
	{
		printf("#define jit_apply_builder_add_float64(builder,value) \\\n");
		printf("\tdo { \\\n");
		printf("\t\tif((builder)->float_used < %d) \\\n", num_double_regs);
		printf("\t\t{ \\\n");
		printf("\t\t\t(builder)->apply_args->float_regs[(builder)->float_used].double_value = (jit_reg_double)(value); \\\n");
		printf("\t\t\t++((builder)->float_used); \\\n");
		printf("\t\t} \\\n");
		printf("\t\telse \\\n");
		if(pass_stack_double_as_nfloat)
			name = "jit_nfloat";
		else
			name = "jit_float64";
		printf("\t\t{ \\\n");
		printf("\t\t\t%s __temp = (%s)(value); \\\n", name, name);
		printf("\t\t\tjit_memcpy((builder)->apply_args->stack_args + (builder)->stack_used, &__temp, sizeof(__temp)); \\\n");
		printf("\t\t\t(builder)->stack_used += (sizeof(%s) + sizeof(jit_nint) - 1) & ~(sizeof(jit_nint) - 1); \\\n", name);
		printf("\t\t} \\\n");
		printf("\t} while (0)\n");
	}
	else if(doubles_in_word_regs)
	{
		/* Pass double values in word registers */
		if(pass_reg_double_as_nfloat)
			name = "jit_nfloat";
		else
			name = "jit_float64";
		printf("#define jit_apply_builder_add_float64(builder,value) \\\n");
		printf("\tjit_apply_builder_add_large((builder), %s, (value));\n", name);
	}
	else
	{
		/* Pass double values on the stack */
		if(pass_stack_double_as_nfloat)
			name = "jit_nfloat";
		else
			name = "jit_float64";
		printf("#define jit_apply_builder_add_float64(builder,value) \\\n");
		printf("\tjit_apply_builder_add_large_stack((builder), %s, (value));\n", name);
	}
	if(num_nfloat_regs > 0)
	{
		printf("#define jit_apply_builder_add_nfloat(builder,value) \\\n");
		printf("\tdo { \\\n");
		printf("\t\tif((builder)->float_used < %d) \\\n", num_nfloat_regs);
		printf("\t\t{ \\\n");
		printf("\t\t\t(builder)->apply_args->float_regs[(builder)->float_used].nfloat_value = (jit_reg_nfloat)(value); \\\n");
		printf("\t\t\t++((builder)->float_used); \\\n");
		printf("\t\t} \\\n");
		printf("\t\telse \\\n");
		if(pass_stack_nfloat_as_double)
			name = "jit_float64";
		else
			name = "jit_nfloat";
		printf("\t\t{ \\\n");
		printf("\t\t\t%s __temp = (%s)(value); \\\n", name, name);
		printf("\t\t\tjit_memcpy((builder)->apply_args->stack_args + (builder)->stack_used, &__temp, sizeof(__temp)); \\\n");
		printf("\t\t\t(builder)->stack_used += (sizeof(%s) + sizeof(jit_nint) - 1) & ~(sizeof(jit_nint) - 1); \\\n", name);
		printf("\t\t} \\\n");
		printf("\t} while (0)\n");
	}
	else if(nfloats_in_word_regs)
	{
		/* Pass nfloat values in word registers */
		if(pass_reg_nfloat_as_double)
			name = "jit_float64";
		else
			name = "jit_nfloat";
		printf("#define jit_apply_builder_add_nfloat(builder,value) \\\n");
		printf("\tjit_apply_builder_add_large((builder), %s, (value));\n", name);
	}
	else
	{
		/* Pass nfloat values on the stack */
		if(pass_stack_nfloat_as_double)
			name = "jit_float64";
		else
			name = "jit_nfloat";
		printf("#define jit_apply_builder_add_nfloat(builder,value) \\\n");
		printf("\tjit_apply_builder_add_large_stack((builder), %s, (value));\n", name);
	}
	printf("#define jit_apply_builder_add_struct(builder,value,size,align) \\\n");
	printf("\tdo { \\\n");
	printf("\t\tunsigned int __size = (size); \\\n");
	printf("\t\tunsigned int __align; __align = (align); \\\n");
	printf("\t\tjit_apply_builder_add_large_inner((builder), (value), __size, __align); \\\n");
	printf("\t} while (0)\n\n");

	printf("\n");

	/* Get parameter values of various types from a closure's arguments */
	printf("#define jit_apply_parser_get_sbyte(builder,value) \\\n");
	printf("\tjit_apply_parser_get_word((builder), jit_sbyte, (value));\n");
	printf("#define jit_apply_parser_get_ubyte(builder,value) \\\n");
	printf("\tjit_apply_parser_get_word((builder), jit_ubyte, (value));\n");
	printf("#define jit_apply_parser_get_short(builder,value) \\\n");
	printf("\tjit_apply_parser_get_word((builder), jit_short, (value));\n");
	printf("#define jit_apply_parser_get_ushort(builder,value) \\\n");
	printf("\tjit_apply_parser_get_word((builder), jit_ushort, (value));\n");
	printf("#define jit_apply_parser_get_int(builder,value) \\\n");
	printf("\tjit_apply_parser_get_word((builder), jit_int, (value));\n");
	printf("#define jit_apply_parser_get_uint(builder,value) \\\n");
	printf("\tjit_apply_parser_get_word((builder), jit_uint, (value));\n");
	printf("#define jit_apply_parser_get_nint(builder,value) \\\n");
	printf("\tjit_apply_parser_get_word((builder), jit_nint, (value));\n");
	printf("#define jit_apply_parser_get_nuint(builder,value) \\\n");
	printf("\tjit_apply_parser_get_word((builder), jit_nuint, (value));\n");
#ifdef JIT_NATIVE_INT32
	printf("#define jit_apply_parser_get_long(builder,value) \\\n");
	printf("\tjit_apply_parser_get_large((builder), jit_long, jit_long, (value));\n");
	printf("#define jit_apply_parser_get_ulong(builder,value) \\\n");
	printf("\tjit_apply_parser_get_large((builder), jit_ulong, jit_ulong, (value));\n");
#else
	printf("#define jit_apply_parser_get_long(builder,value) \\\n");
	printf("\tjit_apply_parser_get_word((builder), jit_long, (value));\n");
	printf("#define jit_apply_parser_get_ulong(builder,value) \\\n");
	printf("\tjit_apply_parser_get_word((builder), jit_ulong, (value));\n");
#endif
	if(num_float_regs > 0)
	{
		/* Pass float values in registers, if possible */
		printf("#define jit_apply_parser_get_float32(builder,value) \\\n");
		printf("\tdo { \\\n");
		printf("\t\tif((builder)->float_used < %d) \\\n", num_float_regs);
		printf("\t\t{ \\\n");
		printf("\t\t\t(value) = (jit_float32)((builder)->apply_args->float_regs[(builder)->float_used].float_value); \\\n");
		printf("\t\t\t++((builder)->float_used); \\\n");
		printf("\t\t} \\\n");
		printf("\t\telse \\\n");
		printf("\t\t{ \\\n");
		if(pass_stack_float_as_double)
			name = "jit_float64";
		else if(pass_stack_float_as_nfloat)
			name = "jit_nfloat";
		else
			name = "jit_float32";
		printf("\t\t\t%s __temp; \\\n", name);
		printf("\t\t\tjit_memcpy(&__temp, (builder)->apply_args->stack_args + (builder)->stack_used, sizeof(__temp)); \\\n");
		printf("\t\t\t(builder)->stack_used += (sizeof(%s) + sizeof(jit_nint) - 1) & ~(sizeof(jit_nint) - 1); \\\n", name);
		printf("\t\t\t(value) = (jit_float32)__temp; \\\n");
		printf("\t\t} \\\n");
		printf("\t} while (0)\n");
	}
	else if(floats_in_word_regs)
	{
		/* Pass float values in word registers */
		if(pass_reg_float_as_double)
			name = "jit_float64";
		else if(pass_reg_float_as_nfloat)
			name = "jit_nfloat";
		else
			name = "jit_float32";
		printf("#define jit_apply_parser_get_float32(builder,value) \\\n");
		printf("\tjit_apply_parser_get_large((builder), %s, jit_float32, (value));\n", name);
	}
	else
	{
		/* Pass float values on the stack */
		if(pass_stack_float_as_double)
			name = "jit_float64";
		else if(pass_stack_float_as_nfloat)
			name = "jit_nfloat";
		else
			name = "jit_float32";
		printf("#define jit_apply_parser_get_float32(builder,value) \\\n");
		printf("\tjit_apply_parser_get_large_stack((builder), %s, jit_float32, (value));\n", name);
	}
	if(num_double_regs > 0)
	{
		/* Pass double values in registers, if possible */
		printf("#define jit_apply_parser_get_float64(builder,value) \\\n");
		printf("\tdo { \\\n");
		printf("\t\tif((builder)->float_used < %d) \\\n", num_double_regs);
		printf("\t\t{ \\\n");
		printf("\t\t\t(value) = (jit_float64)((builder)->apply_args->float_regs[(builder)->float_used].double_value); \\\n");
		printf("\t\t\t++((builder)->float_used); \\\n");
		printf("\t\t} \\\n");
		printf("\t\telse \\\n");
		if(pass_stack_double_as_nfloat)
			name = "jit_nfloat";
		else
			name = "jit_float64";
		printf("\t\t{ \\\n");
		printf("\t\t\t%s __temp; \\\n", name);
		printf("\t\t\tjit_memcpy(&__temp, (builder)->apply_args->stack_args + (builder)->stack_used, sizeof(__temp)); \\\n");
		printf("\t\t\t(builder)->stack_used += (sizeof(%s) + sizeof(jit_nint) - 1) & ~(sizeof(jit_nint) - 1); \\\n", name);
		printf("\t\t\t(value) = (jit_float64)__temp; \\\n");
		printf("\t\t} \\\n");
		printf("\t} while (0)\n");
	}
	else if(doubles_in_word_regs)
	{
		/* Pass double values in word registers */
		if(pass_reg_double_as_nfloat)
			name = "jit_nfloat";
		else
			name = "jit_float64";
		printf("#define jit_apply_parser_get_float64(builder,value) \\\n");
		printf("\tjit_apply_parser_get_large((builder), %s, jit_float64, (value));\n", name);
	}
	else
	{
		/* Pass double values on the stack */
		/* Pass nfloat values in registers, if possible */
		if(pass_stack_double_as_nfloat)
			name = "jit_nfloat";
		else
			name = "jit_float64";
		printf("#define jit_apply_parser_get_float64(builder,value) \\\n");
		printf("\tjit_apply_parser_get_large_stack((builder), %s, jit_float64, (value));\n", name);
	}
	if(num_nfloat_regs > 0)
	{
		printf("#define jit_apply_parser_get_nfloat(builder,value) \\\n");
		printf("\tdo { \\\n");
		printf("\t\tif((builder)->float_used < %d) \\\n", num_nfloat_regs);
		printf("\t\t{ \\\n");
		printf("\t\t\t(value) = (jit_nfloat)((builder)->apply_args->float_regs[(builder)->float_used].nfloat_value); \\\n");
		printf("\t\t\t++((builder)->float_used); \\\n");
		printf("\t\t} \\\n");
		printf("\t\telse \\\n");
		if(pass_stack_nfloat_as_double)
			name = "jit_float64";
		else
			name = "jit_nfloat";
		printf("\t\t{ \\\n");
		printf("\t\t\t%s __temp; \\\n", name);
		printf("\t\t\tjit_memcpy(&__temp, (builder)->apply_args->stack_args + (builder)->stack_used, sizeof(__temp)); \\\n");
		printf("\t\t\t(builder)->stack_used += (sizeof(%s) + sizeof(jit_nint) - 1) & ~(sizeof(jit_nint) - 1); \\\n", name);
		printf("\t\t\t(value) = (jit_nfloat)__temp; \\\n");
		printf("\t\t} \\\n");
		printf("\t} while (0)\n");
	}
	else if(nfloats_in_word_regs)
	{
		/* Pass nfloat values in word registers */
		if(pass_reg_nfloat_as_double)
			name = "jit_float64";
		else
			name = "jit_nfloat";
		printf("#define jit_apply_parser_get_nfloat(builder,value) \\\n");
		printf("\tjit_apply_parser_get_large((builder), %s, jit_nfloat, (value));\n", name);
	}
	else
	{
		/* Pass nfloat values on the stack */
		if(pass_stack_nfloat_as_double)
			name = "jit_float64";
		else
			name = "jit_nfloat";
		printf("#define jit_apply_parser_get_nfloat(builder,value) \\\n");
		printf("\tjit_apply_parser_get_large_stack((builder), %s, jit_nfloat, (value));\n", name);
	}
	printf("#define jit_apply_parser_get_struct_return(builder,value) \\\n");
	if(struct_return_special_reg && !struct_reg_overlaps_word_reg)
	{
		printf("\tdo { \\\n");
		printf("\t\t(value) = (builder)->apply_args->struct_ptr; \\\n");
		printf("\t} while (0)\n");
	}
	else
	{
		printf("\tjit_apply_parser_get_word((builder), void *, (value));\n");
	}
	printf("#define jit_apply_parser_get_struct(builder,size,align,value) \\\n");
	printf("\tdo { \\\n");
	printf("\t\tunsigned int __size = (size); \\\n");
	printf("\t\tunsigned int __num_words = (__size + sizeof(jit_nint) - 1) / sizeof(jit_nint); \\\n");
	if(num_word_regs > 0 || x86_fastcall)
	{
		printf("\t\tif((%s - (builder)->word_used) >= __num_words) \\\n", word_reg_limit);
		printf("\t\t{ \\\n");
		printf("\t\t\tjit_memcpy((value), (builder)->apply_args->word_regs + (builder)->word_used, __size); \\\n");
		printf("\t\t\t(builder)->word_used += __num_words; \\\n");
		printf("\t\t} \\\n");
		printf("\t\telse \\\n");
		printf("\t\t{ \\\n");
		printf("\t\t\tjit_memcpy((value), (builder)->apply_args->stack_args + (builder)->stack_used, __size); \\\n");
		printf("\t\t\t(builder)->stack_used += __num_words * sizeof(jit_nint); \\\n");
		printf("\t\t\t(builder)->word_used = %s; \\\n", word_reg_limit);
		printf("\t\t} \\\n");
	}
	else
	{
		printf("\t\tjit_memcpy((value), (builder)->apply_args->stack_args + (builder)->stack_used, __size); \\\n");
		printf("\t\t(builder)->stack_used += __num_words * sizeof(jit_nint); \\\n");
	}
	printf("\t} while (0)\n");
	printf("\n");
}

int main(int argc, char *argv[])
{
	int size;
	int flags;

#ifndef JIT_APPLY_NUM_WORD_REGS
	/* Detect the number of word registers */
	detect_word_regs(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
	                 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
					 27, 28, 29, 30, 31);

	/* Detect the presence of a structure return register if
	   "detect_word_regs" was unable to do so */
	if(num_word_regs <= 1)
	{
		detect_struct_buf = detect_struct_return(1, 2);
	}

	/* Determine if the special structure register overlaps a word register */
	detect_struct_buf = detect_struct_overlap(1, 2);

	/* Detect the number of float registers */
	detect_float_regs(0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0,
					  9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0,
					  17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0,
					  25.0, 26.0, 27.0, 28.0, 29.0, 30.0, 31.0);

	/* Detect the number of double registers */
	detect_double_regs(0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0,
					   9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0,
					   17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0,
					   25.0, 26.0, 27.0, 28.0, 29.0, 30.0, 31.0);

	/* Detect the number of native floating-point registers */
	detect_nfloat_regs(0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0,
					  9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0,
					  17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0,
					  25.0, 26.0, 27.0, 28.0, 29.0, 30.0, 31.0);

	/* Determine if floating-point values are passed in word registers */
	if(num_float_regs > 0 && num_word_regs > 0)
	{
	#ifdef JIT_NATIVE_INT32
		if(num_word_regs == 1)
		{
			detect_float_overlap((float)(123.78), 1);
		}
		else
	#endif
		{
			detect_double_overlap(123.78, 1, 2);
		}
	}

	if(num_nfloat_regs > 0 && num_word_regs > 0)
	{
		detect_nfloat_overlap(123.78, 1, 2);
	}

	/* Determine if "long double" values should be demoted to "double" */
	if(floats_in_word_regs)
	{
		pass_reg_nfloat_as_double = 1;
	}
	else if(num_float_regs > 0)
	{
		detect_float_reg_size_regs(48.67, 182.36);
	}
	else
	{
		detect_float_reg_size_stack(48.67, 182.36);
	}
	if(sizeof(jit_float64) == sizeof(jit_nfloat))
	{
		pass_stack_nfloat_as_double = 1;
		pass_reg_nfloat_as_double = 1;
	}

	/* Determine if "float" should be promoted to "double" */
	detect_float_promotion(0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0,
					       9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0,
					       17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0,
					       25.0, 26.0, 27.0, 28.0, 29.0, 30.0, 31.0);

	/* Determine if "double" should be promoted to "nfloat" */
	detect_double_promotion(0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0,
					        9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0,
					        17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0,
					        25.0, 26.0, 27.0, 28.0, 29.0, 30.0, 31.0);

	/* Determine if variable arguments are always passed on the stack */
	detect_varargs_on_stack(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
	                        14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
					        27, 28, 29, 30, 31);

	/* Detect the size and positioning of floating-point return values */
	detect_float_return();

	/* Detect the calling conventions for structures */
	detect_struct_conventions();

	/* Detect support for x86 FASTCALL handling code */
#if defined(PLATFORM_IS_X86)
	x86_fastcall = 1;
	floats_in_word_regs = 1;
#endif

	/* Detect whether x86 platforms pop the structure return pointer */
#if defined(PLATFORM_IS_X86)
	x86_pop_struct_return = 1;
	/* TODO */
#endif

	/* Detect the alignment of "long" values */
	detect_long_alignment();

	/* Detect the location of parent frames and return addresses
	   in the value returned by "__builtin_frame_address" */
	detect_frame_offsets();
#endif

	/* Determine the maximum sizes */
	detect_max_sizes();

	/* Print the results */
	printf("/%c This file was auto-generated by \"gen-apply\" - DO NOT EDIT %c/\n\n", '*', '*');
	printf("#ifndef _JIT_APPLY_RULES_H\n");
	printf("#define _JIT_APPLY_RULES_H\n\n");
	printf("#define JIT_APPLY_NUM_WORD_REGS %d\n", num_word_regs);
	printf("#define JIT_APPLY_NUM_FLOAT_REGS %d\n", num_float_regs);
	printf("#define JIT_APPLY_NUM_DOUBLE_REGS %d\n", num_double_regs);
	printf("#define JIT_APPLY_NUM_NFLOAT_REGS %d\n", num_nfloat_regs);
	printf("#define JIT_APPLY_PASS_STACK_FLOAT_AS_DOUBLE %d\n",
		   pass_stack_float_as_double);
	printf("#define JIT_APPLY_PASS_STACK_FLOAT_AS_NFLOAT %d\n",
		   pass_stack_float_as_nfloat);
	printf("#define JIT_APPLY_PASS_STACK_DOUBLE_AS_NFLOAT %d\n",
		   pass_stack_double_as_nfloat);
	printf("#define JIT_APPLY_PASS_STACK_NFLOAT_AS_DOUBLE %d\n",
		   pass_stack_nfloat_as_double);
	printf("#define JIT_APPLY_PASS_REG_FLOAT_AS_DOUBLE %d\n",
		   pass_reg_float_as_double);
	printf("#define JIT_APPLY_PASS_REG_FLOAT_AS_NFLOAT %d\n",
		   pass_reg_float_as_nfloat);
	printf("#define JIT_APPLY_PASS_REG_DOUBLE_AS_NFLOAT %d\n",
		   pass_reg_double_as_nfloat);
	printf("#define JIT_APPLY_PASS_REG_NFLOAT_AS_DOUBLE %d\n",
		   pass_reg_nfloat_as_double);
	printf("#define JIT_APPLY_RETURN_FLOAT_AS_DOUBLE %d\n", return_float_as_double);
	printf("#define JIT_APPLY_RETURN_FLOAT_AS_NFLOAT %d\n", return_float_as_nfloat);
	printf("#define JIT_APPLY_RETURN_DOUBLE_AS_NFLOAT %d\n", return_double_as_nfloat);
	printf("#define JIT_APPLY_RETURN_NFLOAT_AS_DOUBLE %d\n", return_nfloat_as_double);
	printf("#define JIT_APPLY_FLOATS_IN_WORD_REGS %d\n", floats_in_word_regs);
	printf("#define JIT_APPLY_DOUBLES_IN_WORD_REGS %d\n", doubles_in_word_regs);
	printf("#define JIT_APPLY_NFLOATS_IN_WORD_REGS %d\n", nfloats_in_word_regs);
	printf("#define JIT_APPLY_RETURN_FLOATS_AFTER %d\n", return_floats_after);
	printf("#define JIT_APPLY_RETURN_DOUBLES_AFTER %d\n", return_doubles_after);
	printf("#define JIT_APPLY_RETURN_NFLOATS_AFTER %d\n", return_nfloats_after);
	printf("#define JIT_APPLY_VARARGS_ON_STACK %d\n", varargs_on_stack);
	printf("#define JIT_APPLY_STRUCT_RETURN_SPECIAL_REG %d\n", struct_return_special_reg);
	printf("#define JIT_APPLY_STRUCT_REG_OVERLAPS_WORD_REG %d\n",
		   struct_reg_overlaps_word_reg);
	printf("#define JIT_APPLY_ALIGN_LONG_REGS %d\n", align_long_regs);
	printf("#define JIT_APPLY_ALIGN_LONG_STACK %d\n", align_long_stack);
	printf("#define JIT_APPLY_CAN_SPLIT_LONG %d\n", can_split_long);
	printf("#define JIT_APPLY_STRUCT_RETURN_IN_REG_INIT \\\n\t{");
	max_struct_in_reg = 0;
	for(size = 0; size < 64; size += 8)
	{
		flags = 0;
		if(struct_return_in_reg[size])
		{
			flags |= 0x01;
			max_struct_in_reg = size + 1;
		}
		if(struct_return_in_reg[size + 1])
		{
			flags |= 0x02;
			max_struct_in_reg = size + 2;
		}
		if(struct_return_in_reg[size + 2])
		{
			flags |= 0x04;
			max_struct_in_reg = size + 3;
		}
		if(struct_return_in_reg[size + 3])
		{
			flags |= 0x08;
			max_struct_in_reg = size + 4;
		}
		if(struct_return_in_reg[size + 4])
		{
			flags |= 0x10;
			max_struct_in_reg = size + 5;
		}
		if(struct_return_in_reg[size + 5])
		{
			flags |= 0x20;
			max_struct_in_reg = size + 6;
		}
		if(struct_return_in_reg[size + 6])
		{
			flags |= 0x40;
			max_struct_in_reg = size + 7;
		}
		if(struct_return_in_reg[size + 7])
		{
			flags |= 0x80;
			max_struct_in_reg = size + 8;
		}
		if(size != 0)
		{
			printf(", 0x%02X", flags);
		}
		else
		{
			printf("0x%02X", flags);
		}
	}
	printf("}\n");
	printf("#define JIT_APPLY_MAX_STRUCT_IN_REG %d\n", max_struct_in_reg);
	printf("#define JIT_APPLY_MAX_APPLY_SIZE %d\n", max_apply_size);
	printf("#define JIT_APPLY_X86_FASTCALL %d\n", x86_fastcall);
	printf("#define JIT_APPLY_PARENT_FRAME_OFFSET %d\n", parent_frame_offset);
	printf("#define JIT_APPLY_RETURN_ADDRESS_OFFSET %d\n",
		   return_address_offset);
	printf("#define JIT_APPLY_BROKEN_FRAME_BUILTINS %d\n",
		   broken_frame_builtins);
	printf("#define JIT_APPLY_X86_POP_STRUCT_RETURN %d\n",
		   x86_pop_struct_return);
	printf("#define JIT_APPLY_PAD_FLOAT_REGS %d\n", pad_float_regs);
	printf("\n");

	/* Dump the definition of the "jit_apply_return" union */
	dump_return_union();

	/* Dump the definition of the apply structure */
	dump_apply_structure();

	/* Dump the macros that are used to perform function application */
	dump_apply_macros();

	/* Print the footer on the output */
	printf("#endif /%c _JIT_APPLY_RULES_H %c/\n", '*', '*');

	/* Done */
	return 0;
}

#else /* !(PLATFORM_IS_GCC || PLATFORM_IS_WIN32) */

int main(int argc, char *argv[])
{
	printf("#error \"gcc is required to detect the apply rules\"\n");
	return 0;
}

#endif /* !(PLATFORM_IS_GCC || PLATFORM_IS_WIN32) */
