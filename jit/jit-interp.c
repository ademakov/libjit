/*
 * jit-interp.c - Fallback interpreter implementation.
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

/*

This file must be compiled with a C++ compiler, because it uses
C++ exceptions to manage JIT exception throws.  It is otherwise
straight vanilla ANSI C.

*/

#include "jit-interp.h"
#include "jit-rules.h"
#include "jit-memory.h"
#include <config.h>
#if HAVE_ALLOCA_H
	#include <alloca.h>
#endif
#ifdef JIT_WIN32_PLATFORM
	#include <malloc.h>
	#ifndef alloca
		#define	alloca	_alloca
	#endif
#endif
#include "jit-setjmp.h"

#if defined(JIT_BACKEND_INTERP)

/*
 * Macros that help with the definition of the interpreter switch loop.
 */
#define	VMSWITCH(pc)	\
		for(;;)	\
		{ \
			switch((int)*((jit_nint *)(pc)))
#define	VMSWITCHEND		\
		}
#define	VMCASE(opcode)	\
		case opcode
#define	VMBREAK			\
		break

/*
 * Modify the program counter and stack pointer.
 */
#define	VM_MODIFY_PC_AND_STACK(pcmod,stkmod)	\
		do { \
			pc += (jit_nint)(int)(pcmod); \
			stacktop += (jit_nint)(int)(stkmod); \
		} while (0)
#define	VM_MODIFY_STACK(stkmod)	\
		do { \
			stacktop += (jit_nint)(int)(stkmod); \
		} while (0)

/*
 * Fetch arguments of various types from the instruction stream.
 */
#define	VM_NINT_ARG		(((jit_nint *)(pc))[1])
#define	VM_NINT_ARG2	(((jit_nint *)(pc))[2])
#define	VM_NINT_ARG3	(((jit_nint *)(pc))[3])
#define	VM_BR_TARGET	(pc + VM_NINT_ARG)

/*
 * Fetch stack items from various positions.
 */
#define	VM_STK_INT0		(stacktop[0].int_value)
#define	VM_STK_INT1		(stacktop[1].int_value)
#define	VM_STK_INTP		(stacktop[-1].int_value)
#define	VM_STK_UINT0	(stacktop[0].uint_value)
#define	VM_STK_UINT1	(stacktop[1].uint_value)
#define	VM_STK_UINTP	(stacktop[-1].uint_value)
#define	VM_STK_LONG0	(stacktop[0].long_value)
#define	VM_STK_LONG1	(stacktop[1].long_value)
#define	VM_STK_LONGP	(stacktop[-1].long_value)
#define	VM_STK_ULONG0	(stacktop[0].ulong_value)
#define	VM_STK_ULONG1	(stacktop[1].ulong_value)
#define	VM_STK_ULONGP	(stacktop[-1].ulong_value)
#define	VM_STK_FLOAT320	(stacktop[0].float32_value)
#define	VM_STK_FLOAT321	(stacktop[1].float32_value)
#define	VM_STK_FLOAT32P	(stacktop[-1].float32_value)
#define	VM_STK_FLOAT640	(stacktop[0].float64_value)
#define	VM_STK_FLOAT641	(stacktop[1].float64_value)
#define	VM_STK_FLOAT64P	(stacktop[-1].float64_value)
#define	VM_STK_NFLOAT0	(stacktop[0].nfloat_value)
#define	VM_STK_NFLOAT1	(stacktop[1].nfloat_value)
#define	VM_STK_NFLOATP	(stacktop[-1].nfloat_value)
#define	VM_STK_PTR0		(stacktop[0].ptr_value)
#define	VM_STK_PTR1		(stacktop[1].ptr_value)
#define	VM_STK_PTR2		(stacktop[2].ptr_value)
#define	VM_STK_PTRP		(stacktop[-1].ptr_value)
#define	VM_STK_PTRP2	(stacktop[-2].ptr_value)
#ifdef JIT_NATIVE_INT32
#define	VM_STK_NINT0	VM_STK_INT0
#define	VM_STK_NINT1	VM_STK_INT1
#define	VM_STK_NUINT0	VM_STK_UINT0
#define	VM_STK_NUINT1	VM_STK_UINT1
#else
#define	VM_STK_NINT0	VM_STK_LONG0
#define	VM_STK_NINT1	VM_STK_LONG1
#define	VM_STK_NUINT0	VM_STK_ULONG0
#define	VM_STK_NUINT1	VM_STK_ULONG1
#endif

/*
 * Apply a relative adjustment to a pointer and cast to a specific type.
 */
#define	VM_REL(type,ptr)	\
			((type *)(((unsigned char *)(ptr)) + VM_NINT_ARG))

/*
 * Apply an array adjustment to a pointer.
 */
#define	VM_LOAD_ELEM(type)	\
			(*(((type *)VM_STK_PTR1) + VM_STK_NINT0))
#define	VM_STORE_ELEM(type,value)	\
			(*(((type *)VM_STK_PTR2) + VM_STK_NINT1) = (type)(value))

/*
 * Get the address of an argument or local variable at a particular offset.
 */
#define	VM_ARG(type)		\
			((type *)(((jit_item *)args) + VM_NINT_ARG))
#define	VM_LOC(type)		\
			((type *)(((jit_item *)frame) + VM_NINT_ARG))

/*
 * Handle the return value from a function that reports a builtin exception.
 */
#define	VM_BUILTIN(value)	\
			do { \
				builtin_exception = (value); \
				if(builtin_exception < JIT_RESULT_OK) \
				{ \
					goto handle_builtin; \
				} \
			} while (0)

/*
 * Call "jit_apply" from the interpreter, to invoke a native function.
 */
static void apply_from_interpreter
		(jit_type_t signature, void *func,
		 jit_item *args, unsigned int num_fixed_args,
		 void *return_area)
{
	unsigned int num_params;
	unsigned int param;
	jit_type_t type;
	void **apply_args;

	/* Allocate space for the apply arguments and initialize them */
	num_params = jit_type_num_params(signature);
	apply_args = (void **)alloca(sizeof(void *) * num_params);
	for(param = 0; param < num_params; ++param)
	{
		type = jit_type_normalize(jit_type_get_param(signature, param));
		switch(type->kind)
		{
			case JIT_TYPE_SBYTE:
			case JIT_TYPE_UBYTE:
			{
				apply_args[param] =
					((unsigned char *)args) + _jit_int_lowest_byte();
				++args;
			}
			break;

			case JIT_TYPE_SHORT:
			case JIT_TYPE_USHORT:
			{
				apply_args[param] =
					((unsigned char *)args) + _jit_int_lowest_short();
				++args;
			}
			break;

			case JIT_TYPE_INT:
			case JIT_TYPE_UINT:
			case JIT_TYPE_LONG:
			case JIT_TYPE_ULONG:
			case JIT_TYPE_FLOAT32:
			case JIT_TYPE_FLOAT64:
			case JIT_TYPE_NFLOAT:
			{
				apply_args[param] = args;
				++args;
			}
			break;

			case JIT_TYPE_STRUCT:
			case JIT_TYPE_UNION:
			{
				apply_args[param] = args;
				args += JIT_NUM_ITEMS_IN_STRUCT(jit_type_get_size(type));
			}
			break;

			default:
			{
				/* Shouldn't happen, but do something sane */
				apply_args[param] = args;
			}
			break;
		}
	}

	/* Apply the function */
	jit_apply(signature, func, apply_args, num_fixed_args, return_area);
}

void _jit_run_function(jit_function_interp_t func, jit_item *args,
					   jit_item *return_area)
{
	jit_item *frame;
	jit_item *stacktop;
	void **pc;
	jit_int builtin_exception;
	jit_nint temparg;
	void *tempptr;
	void *tempptr2;
	jit_function_t call_func;
	struct jit_backtrace call_trace;
	void *entry;
	void *exception_object = 0;
	void *handler;
	jit_jmp_buf *jbuf;

	/* Set up the stack frame for this function */
	frame = (jit_item *)alloca(func->frame_size);
	stacktop = frame + func->working_area;
	frame = stacktop;

	/* Get the initial program counter */
	pc = jit_function_interp_entry_pc(func);

	/* Create a "setjmp" point if this function has a "try" block.
	   This is used to catch exceptions on their way up the stack */
	if(func->func->has_try)
	{
		jbuf = (jit_jmp_buf *)alloca(sizeof(jit_jmp_buf));
		_jit_unwind_push_setjmp(jbuf);
		if(setjmp(jbuf->buf))
		{
			/* An exception has been thrown by lower-level code */
			exception_object = jit_exception_get_last_and_clear();
			goto handle_exception;
		}
	}
	else
	{
		jbuf = 0;
	}

	/* Enter the instruction dispatch loop */
	VMSWITCH(pc)
	{
		/******************************************************************
		 * Simple opcodes.
		 ******************************************************************/

		VMCASE(JIT_OP_NOP):
		{
			/* Nothing to do except move on to the next instruction */
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		/******************************************************************
		 * Conversion opcodes.
		 ******************************************************************/

		VMCASE(JIT_OP_TRUNC_SBYTE):
		{
			/* Truncate an integer to a signed 8-bit value */
			VM_STK_INT0 = (jit_int)(jit_sbyte)VM_STK_INT0;
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_TRUNC_UBYTE):
		{
			/* Truncate an integer to an unsigned 8-bit value */
			VM_STK_INT0 = (jit_int)(jit_ubyte)VM_STK_INT0;
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_TRUNC_SHORT):
		{
			/* Truncate an integer to a signed 16-bit value */
			VM_STK_INT0 = (jit_int)(jit_short)VM_STK_INT0;
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_TRUNC_USHORT):
		{
			/* Truncate an integer to an unsigned 16-bit value */
			VM_STK_INT0 = (jit_int)(jit_ushort)VM_STK_INT0;
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_TRUNC_INT):
		{
			/* Truncate an integer to a signed 32-bit value */
			/* In the interpreter, this is a NOP */
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_TRUNC_UINT):
		{
			/* Truncate an integer to an unsigned 32-bit value */
			/* In the interpreter, this is a NOP */
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_CHECK_SBYTE):
		{
			/* Truncate an integer to a signed 8-bit value, and check */
			VM_BUILTIN(jit_int_to_sbyte_ovf(&VM_STK_INT0, VM_STK_INT0));
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_CHECK_UBYTE):
		{
			/* Truncate an integer to an unsigned 8-bit value, and check */
			VM_BUILTIN(jit_int_to_ubyte_ovf(&VM_STK_INT0, VM_STK_INT0));
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_CHECK_SHORT):
		{
			/* Truncate an integer to a signed 16-bit value, and check */
			VM_BUILTIN(jit_int_to_short_ovf(&VM_STK_INT0, VM_STK_INT0));
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_CHECK_USHORT):
		{
			/* Truncate an integer to an unsigned 16-bit value, and check */
			VM_BUILTIN(jit_int_to_ushort_ovf(&VM_STK_INT0, VM_STK_INT0));
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_CHECK_INT):
		{
			/* Truncate an integer to a signed 32-bit value, and check */
			VM_BUILTIN(jit_uint_to_int_ovf(&VM_STK_INT0, VM_STK_UINT0));
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_CHECK_UINT):
		{
			/* Truncate an integer to an unsigned 32-bit value, and check */
			VM_BUILTIN(jit_int_to_uint_ovf(&VM_STK_UINT0, VM_STK_INT0));
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_LOW_WORD):
		{
			/* Fetch the low word of a 64-bit value */
			VM_STK_UINT0 = (jit_uint)VM_STK_LONG0;
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_EXPAND_INT):
		{
			/* Expand a signed 32-bit value to a 64-bit value */
			VM_STK_LONG0 = (jit_long)VM_STK_INT0;
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_EXPAND_UINT):
		{
			/* Expand an unsigned 32-bit value to a 64-bit value */
			VM_STK_ULONG0 = (jit_ulong)VM_STK_UINT0;
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_CHECK_LOW_WORD):
		{
			/* Fetch the low word of a 64-bit value, and check */
			VM_BUILTIN(jit_long_to_uint_ovf(&VM_STK_UINT0, VM_STK_LONG0));
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_CHECK_SIGNED_LOW_WORD):
		{
			/* Fetch the signed low word of a 64-bit value, and check */
			VM_BUILTIN(jit_long_to_int_ovf(&VM_STK_INT0, VM_STK_LONG0));
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_CHECK_LONG):
		{
			/* Convert unsigned 64-bit into signed, and check */
			VM_BUILTIN(jit_ulong_to_long_ovf(&VM_STK_LONG0, VM_STK_ULONG0));
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_CHECK_ULONG):
		{
			/* Convert signed 64-bit into unsigned, and check */
			VM_BUILTIN(jit_long_to_ulong_ovf(&VM_STK_ULONG0, VM_STK_LONG0));
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFLOAT_TO_INT):
		{
			/* Convert native float into 32-bit signed integer */
			VM_STK_INT0 = jit_nfloat_to_int(VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFLOAT_TO_UINT):
		{
			/* Convert native float into 32-bit unsigned integer */
			VM_STK_UINT0 = jit_nfloat_to_uint(VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFLOAT_TO_LONG):
		{
			/* Convert native float into 64-bit signed integer */
			VM_STK_LONG0 = jit_nfloat_to_long(VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFLOAT_TO_ULONG):
		{
			/* Convert native float into 64-bit unsigned integer */
			VM_STK_ULONG0 = jit_nfloat_to_ulong(VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_CHECK_NFLOAT_TO_INT):
		{
			/* Convert native float into 32-bit signed integer, and check */
			VM_BUILTIN(jit_nfloat_to_int_ovf(&VM_STK_INT0, VM_STK_NFLOAT0));
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_CHECK_NFLOAT_TO_UINT):
		{
			/* Convert native float into 32-bit unsigned integer, and check */
			VM_BUILTIN(jit_nfloat_to_uint_ovf(&VM_STK_UINT0, VM_STK_NFLOAT0));
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_CHECK_NFLOAT_TO_LONG):
		{
			/* Convert native float into 64-bit signed integer, and check */
			VM_BUILTIN(jit_nfloat_to_long_ovf(&VM_STK_LONG0, VM_STK_NFLOAT0));
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_CHECK_NFLOAT_TO_ULONG):
		{
			/* Convert native float into 64-bit unsigned integer, and check */
			VM_BUILTIN(jit_nfloat_to_ulong_ovf(&VM_STK_ULONG0, VM_STK_NFLOAT0));
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_INT_TO_NFLOAT):
		{
			/* Convert 32-bit signed integer into native float */
			VM_STK_NFLOAT0 = jit_int_to_nfloat(VM_STK_INT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_UINT_TO_NFLOAT):
		{
			/* Convert 32-bit unsigned integer into native float */
			VM_STK_NFLOAT0 = jit_uint_to_nfloat(VM_STK_UINT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_LONG_TO_NFLOAT):
		{
			/* Convert 64-bit signed integer into native float */
			VM_STK_NFLOAT0 = jit_long_to_nfloat(VM_STK_LONG0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_ULONG_TO_NFLOAT):
		{
			/* Convert 64-bit unsigned integer into native float */
			VM_STK_NFLOAT0 = jit_ulong_to_nfloat(VM_STK_ULONG0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFLOAT_TO_FLOAT32):
		{
			/* Convert native float into 32-bit float */
			VM_STK_FLOAT320 = jit_nfloat_to_float32(VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFLOAT_TO_FLOAT64):
		{
			/* Convert native float into 64-bit float */
			VM_STK_FLOAT640 = jit_nfloat_to_float64(VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_FLOAT32_TO_NFLOAT):
		{
			/* Convert 32-bit float into native float */
			VM_STK_NFLOAT0 = jit_float32_to_nfloat(VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_FLOAT64_TO_NFLOAT):
		{
			/* Convert 64-bit float into native float */
			VM_STK_NFLOAT0 = jit_float64_to_nfloat(VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		/******************************************************************
		 * Arithmetic opcodes.
		 ******************************************************************/

		VMCASE(JIT_OP_IADD):
		{
			/* Add signed 32-bit integers */
			VM_STK_INT1 = VM_STK_INT1 + VM_STK_INT0;
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_IADD_OVF):
		{
			/* Add signed 32-bit integers, and check */
			VM_BUILTIN(jit_int_add_ovf
				(&VM_STK_INT1, VM_STK_INT1, VM_STK_INT0));
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_IADD_OVF_UN):
		{
			/* Add unsigned 32-bit integers, and check */
			VM_BUILTIN(jit_uint_add_ovf
				(&VM_STK_UINT1, VM_STK_UINT1, VM_STK_UINT0));
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_ISUB):
		{
			/* Subtract signed 32-bit integers */
			VM_STK_INT1 = VM_STK_INT1 - VM_STK_INT0;
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_ISUB_OVF):
		{
			/* Subtract signed 32-bit integers, and check */
			VM_BUILTIN(jit_int_sub_ovf
				(&VM_STK_INT1, VM_STK_INT1, VM_STK_INT0));
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_ISUB_OVF_UN):
		{
			/* Subtract unsigned 32-bit integers, and check */
			VM_BUILTIN(jit_uint_sub_ovf
				(&VM_STK_UINT1, VM_STK_UINT1, VM_STK_UINT0));
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_IMUL):
		{
			/* Multiply signed 32-bit integers */
			VM_STK_INT1 = VM_STK_INT1 * VM_STK_INT0;
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_IMUL_OVF):
		{
			/* Multiply signed 32-bit integers, and check */
			VM_BUILTIN(jit_int_mul_ovf
				(&VM_STK_INT1, VM_STK_INT1, VM_STK_INT0));
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_IMUL_OVF_UN):
		{
			/* Multiply unsigned 32-bit integers, and check */
			VM_BUILTIN(jit_uint_mul_ovf
				(&VM_STK_UINT1, VM_STK_UINT1, VM_STK_UINT0));
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_IDIV):
		{
			/* Divide signed 32-bit integers */
			VM_BUILTIN(jit_int_div
				(&VM_STK_INT1, VM_STK_INT1, VM_STK_INT0));
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_IDIV_UN):
		{
			/* Divide unsigned 32-bit integers */
			VM_BUILTIN(jit_uint_div
				(&VM_STK_UINT1, VM_STK_UINT1, VM_STK_UINT0));
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_IREM):
		{
			/* Remainder signed 32-bit integers */
			VM_BUILTIN(jit_int_rem
				(&VM_STK_INT1, VM_STK_INT1, VM_STK_INT0));
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_IREM_UN):
		{
			/* Remainder unsigned 32-bit integers */
			VM_BUILTIN(jit_uint_rem
				(&VM_STK_UINT1, VM_STK_UINT1, VM_STK_UINT0));
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_INEG):
		{
			/* Negate signed 32-bit integer */
			VM_STK_INT0 = -VM_STK_INT0;
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_LADD):
		{
			/* Add signed 64-bit integers */
			VM_STK_LONG1 = VM_STK_LONG1 + VM_STK_LONG0;
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LADD_OVF):
		{
			/* Add signed 64-bit integers, and check */
			VM_BUILTIN(jit_long_add_ovf
				(&VM_STK_LONG1, VM_STK_LONG1, VM_STK_LONG0));
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LADD_OVF_UN):
		{
			/* Add unsigned 64-bit integers, and check */
			VM_BUILTIN(jit_ulong_add_ovf
				(&VM_STK_ULONG1, VM_STK_ULONG1, VM_STK_ULONG0));
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LSUB):
		{
			/* Subtract signed 64-bit integers */
			VM_STK_LONG1 = VM_STK_LONG1 - VM_STK_LONG0;
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LSUB_OVF):
		{
			/* Subtract signed 64-bit integers, and check */
			VM_BUILTIN(jit_long_sub_ovf
				(&VM_STK_LONG1, VM_STK_LONG1, VM_STK_LONG0));
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LSUB_OVF_UN):
		{
			/* Subtract unsigned 64-bit integers, and check */
			VM_BUILTIN(jit_ulong_sub_ovf
				(&VM_STK_ULONG1, VM_STK_ULONG1, VM_STK_ULONG0));
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LMUL):
		{
			/* Multiply signed 64-bit integers */
			VM_STK_LONG1 = VM_STK_LONG1 * VM_STK_LONG0;
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LMUL_OVF):
		{
			/* Multiply signed 64-bit integers, and check */
			VM_BUILTIN(jit_long_mul_ovf
				(&VM_STK_LONG1, VM_STK_LONG1, VM_STK_LONG0));
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LMUL_OVF_UN):
		{
			/* Multiply unsigned 64-bit integers, and check */
			VM_BUILTIN(jit_ulong_mul_ovf
				(&VM_STK_ULONG1, VM_STK_ULONG1, VM_STK_ULONG0));
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LDIV):
		{
			/* Divide signed 64-bit integers */
			VM_BUILTIN(jit_long_div
				(&VM_STK_LONG1, VM_STK_LONG1, VM_STK_LONG0));
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LDIV_UN):
		{
			/* Divide unsigned 64-bit integers */
			VM_BUILTIN(jit_ulong_div
				(&VM_STK_ULONG1, VM_STK_ULONG1, VM_STK_ULONG0));
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LREM):
		{
			/* Remainder signed 64-bit integers */
			VM_BUILTIN(jit_long_rem
				(&VM_STK_LONG1, VM_STK_LONG1, VM_STK_LONG0));
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LREM_UN):
		{
			/* Remainder unsigned 64-bit integers */
			VM_BUILTIN(jit_ulong_rem
				(&VM_STK_ULONG1, VM_STK_ULONG1, VM_STK_ULONG0));
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LNEG):
		{
			/* Negate signed 64-bit integer */
			VM_STK_LONG0 = -VM_STK_LONG0;
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_FADD):
		{
			/* Add 32-bit floats */
			VM_STK_FLOAT321 = VM_STK_FLOAT321 + VM_STK_FLOAT320;
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_FSUB):
		{
			/* Subtract 32-bit floats */
			VM_STK_FLOAT321 = VM_STK_FLOAT321 - VM_STK_FLOAT320;
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_FMUL):
		{
			/* Multiply 32-bit floats */
			VM_STK_FLOAT321 = VM_STK_FLOAT321 * VM_STK_FLOAT320;
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_FDIV):
		{
			/* Divide 32-bit floats */
			VM_STK_FLOAT321 = VM_STK_FLOAT321 / VM_STK_FLOAT320;
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_FREM):
		{
			/* Remainder 32-bit floats */
			VM_STK_FLOAT321 = jit_float32_rem
				(VM_STK_FLOAT321, VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_FREM_IEEE):
		{
			/* Remainder 32-bit floats, with IEEE rules */
			VM_STK_FLOAT321 = jit_float32_ieee_rem
				(VM_STK_FLOAT321, VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_FNEG):
		{
			/* Negate 32-bit float */
			VM_STK_FLOAT320 = -VM_STK_FLOAT320;
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_DADD):
		{
			/* Add 64-bit floats */
			VM_STK_FLOAT641 = VM_STK_FLOAT641 + VM_STK_FLOAT640;
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_DSUB):
		{
			/* Subtract 64-bit floats */
			VM_STK_FLOAT641 = VM_STK_FLOAT641 - VM_STK_FLOAT640;
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_DMUL):
		{
			/* Multiply 64-bit floats */
			VM_STK_FLOAT641 = VM_STK_FLOAT641 * VM_STK_FLOAT640;
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_DDIV):
		{
			/* Divide 64-bit floats */
			VM_STK_FLOAT641 = VM_STK_FLOAT641 / VM_STK_FLOAT640;
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_DREM):
		{
			/* Remainder 64-bit floats */
			VM_STK_FLOAT641 = jit_float64_rem
				(VM_STK_FLOAT641, VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_DREM_IEEE):
		{
			/* Remainder 64-bit floats, with IEEE rules */
			VM_STK_FLOAT641 = jit_float64_ieee_rem
				(VM_STK_FLOAT641, VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_DNEG):
		{
			/* Negate 64-bit float */
			VM_STK_FLOAT640 = -VM_STK_FLOAT640;
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFADD):
		{
			/* Add native floats */
			VM_STK_NFLOAT1 = VM_STK_NFLOAT1 + VM_STK_NFLOAT0;
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFSUB):
		{
			/* Subtract native floats */
			VM_STK_NFLOAT1 = VM_STK_NFLOAT1 - VM_STK_NFLOAT0;
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFMUL):
		{
			/* Multiply native floats */
			VM_STK_NFLOAT1 = VM_STK_NFLOAT1 * VM_STK_NFLOAT0;
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFDIV):
		{
			/* Divide native floats */
			VM_STK_NFLOAT1 = VM_STK_NFLOAT1 / VM_STK_NFLOAT0;
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFREM):
		{
			/* Remainder native floats */
			VM_STK_NFLOAT1 = jit_nfloat_rem
				(VM_STK_NFLOAT1, VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFREM_IEEE):
		{
			/* Remainder native floats, with IEEE rules */
			VM_STK_NFLOAT1 = jit_nfloat_ieee_rem
				(VM_STK_NFLOAT1, VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFNEG):
		{
			/* Negate native float */
			VM_STK_NFLOAT0 = -VM_STK_NFLOAT0;
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		/******************************************************************
		 * Bitwise opcodes.
		 ******************************************************************/

		VMCASE(JIT_OP_IAND):
		{
			/* Bitwise and signed 32-bit integers */
			VM_STK_INT1 = VM_STK_INT1 & VM_STK_INT0;
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_IOR):
		{
			/* Bitwise or signed 32-bit integers */
			VM_STK_INT1 = VM_STK_INT1 | VM_STK_INT0;
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_IXOR):
		{
			/* Bitwise xor signed 32-bit integers */
			VM_STK_INT1 = VM_STK_INT1 ^ VM_STK_INT0;
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_INOT):
		{
			/* Bitwise not signed 32-bit integers */
			VM_STK_INT0 = ~VM_STK_INT0;
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_ISHL):
		{
			/* Shift left signed 32-bit integers */
			VM_STK_INT1 = VM_STK_INT1 << (VM_STK_UINT0 & 0x1F);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_ISHR):
		{
			/* Shift right signed 32-bit integers */
			VM_STK_INT1 = VM_STK_INT1 >> (VM_STK_UINT0 & 0x1F);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_ISHR_UN):
		{
			/* Shift right unsigned 32-bit integers */
			VM_STK_UINT1 = VM_STK_UINT1 >> (VM_STK_UINT0 & 0x1F);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LAND):
		{
			/* Bitwise and signed 64-bit integers */
			VM_STK_LONG1 = VM_STK_LONG1 & VM_STK_LONG0;
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LOR):
		{
			/* Bitwise or signed 64-bit integers */
			VM_STK_LONG1 = VM_STK_LONG1 | VM_STK_LONG0;
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LXOR):
		{
			/* Bitwise xor signed 64-bit integers */
			VM_STK_LONG1 = VM_STK_LONG1 ^ VM_STK_LONG0;
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LNOT):
		{
			/* Bitwise not signed 64-bit integers */
			VM_STK_LONG0 = ~VM_STK_LONG0;
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_LSHL):
		{
			/* Shift left signed 64-bit integers */
			VM_STK_LONG1 = (VM_STK_LONG1 << (VM_STK_UINT0 & 0x3F));
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LSHR):
		{
			/* Shift right signed 64-bit integers */
			VM_STK_LONG1 = (VM_STK_LONG1 >> (VM_STK_UINT0 & 0x3F));
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LSHR_UN):
		{
			/* Shift right signed 64-bit integers */
			VM_STK_ULONG1 = (VM_STK_ULONG1 >> (VM_STK_UINT0 & 0x3F));
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		/******************************************************************
		 * Branch opcodes.
		 ******************************************************************/

		VMCASE(JIT_OP_BR):
		{
			/* Unconditional branch */
			pc = VM_BR_TARGET;
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_IFALSE):
		{
			/* Branch if signed 32-bit integer is false */
			if(VM_STK_INT0 == 0)
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(1);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 1);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_ITRUE):
		{
			/* Branch if signed 32-bit integer is true */
			if(VM_STK_INT0 != 0)
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(1);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 1);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_IEQ):
		{
			/* Branch if signed 32-bit integers are equal */
			if(VM_STK_INT1 == VM_STK_INT0)
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_INE):
		{
			/* Branch if signed 32-bit integers are not equal */
			if(VM_STK_INT1 != VM_STK_INT0)
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_ILT):
		{
			/* Branch if signed 32-bit integers are less than */
			if(VM_STK_INT1 < VM_STK_INT0)
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_ILT_UN):
		{
			/* Branch if unsigned 32-bit integers are less than */
			if(VM_STK_UINT1 < VM_STK_UINT0)
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_ILE):
		{
			/* Branch if signed 32-bit integers are less than or equal */
			if(VM_STK_INT1 <= VM_STK_INT0)
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_ILE_UN):
		{
			/* Branch if unsigned 32-bit integers are less than or equal */
			if(VM_STK_UINT1 <= VM_STK_UINT0)
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_IGT):
		{
			/* Branch if signed 32-bit integers are greater than */
			if(VM_STK_INT1 > VM_STK_INT0)
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_IGT_UN):
		{
			/* Branch if unsigned 32-bit integers are greater than */
			if(VM_STK_UINT1 > VM_STK_UINT0)
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_IGE):
		{
			/* Branch if signed 32-bit integers are greater than or equal */
			if(VM_STK_INT1 >= VM_STK_INT0)
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_IGE_UN):
		{
			/* Branch if unsigned 32-bit integers are greater than or equal */
			if(VM_STK_UINT1 >= VM_STK_UINT0)
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_LFALSE):
		{
			/* Branch if signed 64-bit integer is false */
			if(VM_STK_LONG0 == 0)
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(1);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 1);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_LTRUE):
		{
			/* Branch if signed 64-bit integer is true */
			if(VM_STK_LONG0 != 0)
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(1);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 1);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_LEQ):
		{
			/* Branch if signed 64-bit integers are equal */
			if(VM_STK_LONG1 == VM_STK_LONG0)
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_LNE):
		{
			/* Branch if signed 64-bit integers are not equal */
			if(VM_STK_LONG1 != VM_STK_LONG0)
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_LLT):
		{
			/* Branch if signed 64-bit integers are less than */
			if(VM_STK_LONG1 < VM_STK_LONG0)
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_LLT_UN):
		{
			/* Branch if unsigned 64-bit integers are less than */
			if(VM_STK_ULONG1 < VM_STK_ULONG0)
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_LLE):
		{
			/* Branch if signed 64-bit integers are less than or equal */
			if(VM_STK_LONG1 <= VM_STK_LONG0)
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_LLE_UN):
		{
			/* Branch if unsigned 64-bit integers are less than or equal */
			if(VM_STK_ULONG1 <= VM_STK_ULONG0)
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_LGT):
		{
			/* Branch if signed 64-bit integers are greater than */
			if(VM_STK_LONG1 > VM_STK_LONG0)
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_LGT_UN):
		{
			/* Branch if unsigned 64-bit integers are greater than */
			if(VM_STK_ULONG1 > VM_STK_ULONG0)
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_LGE):
		{
			/* Branch if signed 64-bit integers are greater than or equal */
			if(VM_STK_LONG1 >= VM_STK_LONG0)
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_LGE_UN):
		{
			/* Branch if unsigned 64-bit integers are greater than or equal */
			if(VM_STK_ULONG1 >= VM_STK_ULONG0)
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_FEQ):
		{
			/* Branch if 32-bit floats are equal */
			if(jit_float32_eq(VM_STK_FLOAT321, VM_STK_FLOAT320))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_FNE):
		{
			/* Branch if 32-bit floats are not equal */
			if(jit_float32_ne(VM_STK_FLOAT321, VM_STK_FLOAT320))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_FLT):
		{
			/* Branch if 32-bit floats are less than */
			if(jit_float32_lt(VM_STK_FLOAT321, VM_STK_FLOAT320))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_FLE):
		{
			/* Branch if 32-bit floats are less than or equal */
			if(jit_float32_le(VM_STK_FLOAT321, VM_STK_FLOAT320))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_FGT):
		{
			/* Branch if 32-bit floats are greater than */
			if(jit_float32_gt(VM_STK_FLOAT321, VM_STK_FLOAT320))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_FGE):
		{
			/* Branch if 32-bit floats are greater than or equal */
			if(jit_float32_ge(VM_STK_FLOAT321, VM_STK_FLOAT320))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_FEQ_INV):
		{
			/* Branch if 32-bit floats are equal; invert nan test */
			if(!jit_float32_ne(VM_STK_FLOAT321, VM_STK_FLOAT320))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_FNE_INV):
		{
			/* Branch if 32-bit floats are not equal; invert nan test */
			if(!jit_float32_eq(VM_STK_FLOAT321, VM_STK_FLOAT320))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_FLT_INV):
		{
			/* Branch if 32-bit floats are less than; invert nan test */
			if(!jit_float32_ge(VM_STK_FLOAT321, VM_STK_FLOAT320))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_FLE_INV):
		{
			/* Branch if 32-bit floats are less or equal; invert nan test */
			if(!jit_float32_gt(VM_STK_FLOAT321, VM_STK_FLOAT320))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_FGT_INV):
		{
			/* Branch if 32-bit floats are greater than; invert nan test */
			if(!jit_float32_le(VM_STK_FLOAT321, VM_STK_FLOAT320))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_FGE_INV):
		{
			/* Branch if 32-bit floats are greater or equal; invert nan test */
			if(!jit_float32_lt(VM_STK_FLOAT321, VM_STK_FLOAT320))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_DEQ):
		{
			/* Branch if 64-bit floats are equal */
			if(jit_float64_eq(VM_STK_FLOAT641, VM_STK_FLOAT640))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_DNE):
		{
			/* Branch if 64-bit floats are not equal */
			if(jit_float64_ne(VM_STK_FLOAT641, VM_STK_FLOAT640))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_DLT):
		{
			/* Branch if 64-bit floats are less than */
			if(jit_float64_lt(VM_STK_FLOAT641, VM_STK_FLOAT640))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_DLE):
		{
			/* Branch if 64-bit floats are less than or equal */
			if(jit_float64_le(VM_STK_FLOAT641, VM_STK_FLOAT640))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_DGT):
		{
			/* Branch if 64-bit floats are greater than */
			if(jit_float64_gt(VM_STK_FLOAT641, VM_STK_FLOAT640))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_DGE):
		{
			/* Branch if 64-bit floats are greater than or equal */
			if(jit_float64_ge(VM_STK_FLOAT641, VM_STK_FLOAT640))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_DEQ_INV):
		{
			/* Branch if 64-bit floats are equal; invert nan test */
			if(!jit_float64_ne(VM_STK_FLOAT641, VM_STK_FLOAT640))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_DNE_INV):
		{
			/* Branch if 64-bit floats are not equal; invert nan test */
			if(!jit_float64_eq(VM_STK_FLOAT641, VM_STK_FLOAT640))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_DLT_INV):
		{
			/* Branch if 64-bit floats are less than; invert nan test */
			if(!jit_float64_ge(VM_STK_FLOAT641, VM_STK_FLOAT640))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_DLE_INV):
		{
			/* Branch if 64-bit floats are less or equal; invert nan test */
			if(!jit_float64_gt(VM_STK_FLOAT641, VM_STK_FLOAT640))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_DGT_INV):
		{
			/* Branch if 64-bit floats are greater than; invert nan test */
			if(!jit_float64_le(VM_STK_FLOAT641, VM_STK_FLOAT640))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_DGE_INV):
		{
			/* Branch if 64-bit floats are greater or equal; invert nan test */
			if(!jit_float64_lt(VM_STK_FLOAT641, VM_STK_FLOAT640))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_NFEQ):
		{
			/* Branch if native floats are equal */
			if(jit_nfloat_eq(VM_STK_NFLOAT1, VM_STK_NFLOAT0))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_NFNE):
		{
			/* Branch if native floats are not equal */
			if(jit_nfloat_ne(VM_STK_NFLOAT1, VM_STK_NFLOAT0))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_NFLT):
		{
			/* Branch if native floats are less than */
			if(jit_nfloat_lt(VM_STK_NFLOAT1, VM_STK_NFLOAT0))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_NFLE):
		{
			/* Branch if native floats are less than or equal */
			if(jit_nfloat_le(VM_STK_NFLOAT1, VM_STK_NFLOAT0))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_NFGT):
		{
			/* Branch if native floats are greater than */
			if(jit_nfloat_gt(VM_STK_NFLOAT1, VM_STK_NFLOAT0))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_NFGE):
		{
			/* Branch if native floats are greater than or equal */
			if(jit_nfloat_ge(VM_STK_NFLOAT1, VM_STK_NFLOAT0))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_NFEQ_INV):
		{
			/* Branch if native floats are equal; invert nan test */
			if(!jit_nfloat_ne(VM_STK_NFLOAT1, VM_STK_NFLOAT0))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_NFNE_INV):
		{
			/* Branch if native floats are not equal; invert nan test */
			if(!jit_nfloat_eq(VM_STK_NFLOAT1, VM_STK_NFLOAT0))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_NFLT_INV):
		{
			/* Branch if native floats are less than; invert nan test */
			if(!jit_nfloat_ge(VM_STK_NFLOAT1, VM_STK_NFLOAT0))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_NFLE_INV):
		{
			/* Branch if native floats are less or equal; invert nan test */
			if(!jit_nfloat_gt(VM_STK_NFLOAT1, VM_STK_NFLOAT0))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_NFGT_INV):
		{
			/* Branch if native floats are greater than; invert nan test */
			if(!jit_nfloat_le(VM_STK_NFLOAT1, VM_STK_NFLOAT0))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_BR_NFGE_INV):
		{
			/* Branch if native floats are greater or equal; invert nan test */
			if(!jit_nfloat_lt(VM_STK_NFLOAT1, VM_STK_NFLOAT0))
			{
				pc = VM_BR_TARGET;
				VM_MODIFY_STACK(2);
			}
			else
			{
				VM_MODIFY_PC_AND_STACK(2, 2);
			}
		}
		VMBREAK;

		/******************************************************************
		 * Comparison opcodes.
		 ******************************************************************/

		VMCASE(JIT_OP_ICMP):
		{
			/* Compare signed 32-bit integers */
			VM_STK_INT1 = jit_int_cmp(VM_STK_INT1, VM_STK_INT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_ICMP_UN):
		{
			/* Compare unsigned 32-bit integers */
			VM_STK_UINT1 = jit_uint_cmp(VM_STK_UINT1, VM_STK_UINT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LCMP):
		{
			/* Compare signed 64-bit integers */
			VM_STK_INT1 = jit_long_cmp(VM_STK_LONG1, VM_STK_LONG0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LCMP_UN):
		{
			/* Compare unsigned 64-bit integers */
			VM_STK_INT1 = jit_long_cmp(VM_STK_ULONG1, VM_STK_ULONG0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_FCMPL):
		{
			/* Compare 32-bit floats, with less nan */
			VM_STK_INT1 = jit_float32_cmpl(VM_STK_FLOAT321, VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_FCMPG):
		{
			/* Compare 32-bit floats, with greater nan */
			VM_STK_INT1 = jit_float32_cmpg(VM_STK_FLOAT321, VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_DCMPL):
		{
			/* Compare 64-bit floats, with less nan */
			VM_STK_INT1 = jit_float64_cmpl(VM_STK_FLOAT641, VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_DCMPG):
		{
			/* Compare 64-bit floats, with greater nan */
			VM_STK_INT1 = jit_float64_cmpg(VM_STK_FLOAT641, VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFCMPL):
		{
			/* Compare native floats, with less nan */
			VM_STK_INT1 = jit_float64_cmpl(VM_STK_NFLOAT1, VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFCMPG):
		{
			/* Compare native floats, with greater nan */
			VM_STK_INT1 = jit_float64_cmpg(VM_STK_NFLOAT1, VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_IEQ):
		{
			/* Compare signed 32-bit integers for equal */
			VM_STK_INT1 = (jit_int)(VM_STK_INT1 == VM_STK_INT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_INE):
		{
			/* Compare signed 32-bit integers for not equal */
			VM_STK_INT1 = (jit_int)(VM_STK_INT1 != VM_STK_INT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_ILT):
		{
			/* Compare signed 32-bit integers for less than */
			VM_STK_INT1 = (jit_int)(VM_STK_INT1 < VM_STK_INT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_ILT_UN):
		{
			/* Compare unsigned 32-bit integers for less than */
			VM_STK_INT1 = (jit_int)(VM_STK_UINT1 < VM_STK_UINT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_ILE):
		{
			/* Compare signed 32-bit integers for less than or equal */
			VM_STK_INT1 = (jit_int)(VM_STK_INT1 <= VM_STK_INT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_ILE_UN):
		{
			/* Compare unsigned 32-bit integers for less than or equal */
			VM_STK_INT1 = (jit_int)(VM_STK_UINT1 <= VM_STK_UINT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_IGT):
		{
			/* Compare signed 32-bit integers for greater than */
			VM_STK_INT1 = (jit_int)(VM_STK_INT1 > VM_STK_INT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_IGT_UN):
		{
			/* Compare unsigned 32-bit integers for greater than */
			VM_STK_INT1 = (jit_int)(VM_STK_UINT1 > VM_STK_UINT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_IGE):
		{
			/* Compare signed 32-bit integers for greater than or equal */
			VM_STK_INT1 = (jit_int)(VM_STK_INT1 >= VM_STK_INT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_IGE_UN):
		{
			/* Compare unsigned 32-bit integers for greater than or equal */
			VM_STK_INT1 = (jit_int)(VM_STK_UINT1 >= VM_STK_UINT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LEQ):
		{
			/* Compare signed 64-bit integers for equal */
			VM_STK_INT1 = (jit_int)(VM_STK_LONG1 == VM_STK_LONG0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LNE):
		{
			/* Compare signed 64-bit integers for not equal */
			VM_STK_INT1 = (jit_int)(VM_STK_LONG1 != VM_STK_LONG0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LLT):
		{
			/* Compare signed 64-bit integers for less than */
			VM_STK_INT1 = (jit_int)(VM_STK_LONG1 < VM_STK_LONG0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LLT_UN):
		{
			/* Compare unsigned 64-bit integers for less than */
			VM_STK_INT1 = (jit_int)(VM_STK_ULONG1 < VM_STK_ULONG0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LLE):
		{
			/* Compare signed 64-bit integers for less than or equal */
			VM_STK_INT1 = (jit_int)(VM_STK_LONG1 <= VM_STK_LONG0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LLE_UN):
		{
			/* Compare unsigned 64-bit integers for less than or equal */
			VM_STK_INT1 = (jit_int)(VM_STK_ULONG1 <= VM_STK_ULONG0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LGT):
		{
			/* Compare signed 64-bit integers for greater than */
			VM_STK_INT1 = (jit_int)(VM_STK_LONG1 > VM_STK_LONG0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LGT_UN):
		{
			/* Compare unsigned 64-bit integers for greater than */
			VM_STK_INT1 = (jit_int)(VM_STK_ULONG1 > VM_STK_ULONG0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LGE):
		{
			/* Compare signed 64-bit integers for greater than or equal */
			VM_STK_INT1 = (jit_int)(VM_STK_LONG1 >= VM_STK_LONG0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LGE_UN):
		{
			/* Compare unsigned 64-bit integers for greater than or equal */
			VM_STK_INT1 = (jit_int)(VM_STK_ULONG1 >= VM_STK_ULONG0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_FEQ):
		{
			/* Compare 32-bit floats for equal */
			VM_STK_INT1 = jit_float32_eq(VM_STK_FLOAT321, VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_FNE):
		{
			/* Compare 32-bit floats for not equal */
			VM_STK_INT1 = jit_float32_ne(VM_STK_FLOAT321, VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_FLT):
		{
			/* Compare 32-bit floats for less than */
			VM_STK_INT1 = jit_float32_lt(VM_STK_FLOAT321, VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_FLE):
		{
			/* Compare 32-bit floats for less than or equal */
			VM_STK_INT1 = jit_float32_le(VM_STK_FLOAT321, VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_FGT):
		{
			/* Compare 32-bit floats for greater than */
			VM_STK_INT1 = jit_float32_gt(VM_STK_FLOAT321, VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_FGE):
		{
			/* Compare 32-bit floats for greater than or equal */
			VM_STK_INT1 = jit_float32_ge(VM_STK_FLOAT321, VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_FEQ_INV):
		{
			/* Compare 32-bit floats for equal; invert nan test */
			VM_STK_INT1 = !jit_float32_ne(VM_STK_FLOAT321, VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_FNE_INV):
		{
			/* Compare 32-bit floats for not equal; invert nan test */
			VM_STK_INT1 = !jit_float32_eq(VM_STK_FLOAT321, VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_FLT_INV):
		{
			/* Compare 32-bit floats for less than; invert nan test */
			VM_STK_INT1 = !jit_float32_ge(VM_STK_FLOAT321, VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_FLE_INV):
		{
			/* Compare 32-bit floats for less than or equal; invert nan test */
			VM_STK_INT1 = !jit_float32_gt(VM_STK_FLOAT321, VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_FGT_INV):
		{
			/* Compare 32-bit floats for greater than; invert nan test */
			VM_STK_INT1 = !jit_float32_le(VM_STK_FLOAT321, VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_FGE_INV):
		{
			/* Compare 32-bit floats for greater or equal; invert nan test */
			VM_STK_INT1 = !jit_float32_lt(VM_STK_FLOAT321, VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_DEQ):
		{
			/* Compare 64-bit floats for equal */
			VM_STK_INT1 = jit_float64_eq(VM_STK_FLOAT641, VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_DNE):
		{
			/* Compare 64-bit floats for not equal */
			VM_STK_INT1 = jit_float64_ne(VM_STK_FLOAT641, VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_DLT):
		{
			/* Compare 64-bit floats for less than */
			VM_STK_INT1 = jit_float64_lt(VM_STK_FLOAT641, VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_DLE):
		{
			/* Compare 64-bit floats for less than or equal */
			VM_STK_INT1 = jit_float64_le(VM_STK_FLOAT641, VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_DGT):
		{
			/* Compare 64-bit floats for greater than */
			VM_STK_INT1 = jit_float64_gt(VM_STK_FLOAT641, VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_DGE):
		{
			/* Compare 64-bit floats for greater than or equal */
			VM_STK_INT1 = jit_float64_ge(VM_STK_FLOAT641, VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_DEQ_INV):
		{
			/* Compare 64-bit floats for equal; invert nan test */
			VM_STK_INT1 = !jit_float64_ne(VM_STK_FLOAT641, VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_DNE_INV):
		{
			/* Compare 64-bit floats for equal; invert nan test */
			VM_STK_INT1 = !jit_float64_eq(VM_STK_FLOAT641, VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_DLT_INV):
		{
			/* Compare 64-bit floats for equal; invert nan test */
			VM_STK_INT1 = !jit_float64_ge(VM_STK_FLOAT641, VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_DLE_INV):
		{
			/* Compare 64-bit floats for equal; invert nan test */
			VM_STK_INT1 = !jit_float64_gt(VM_STK_FLOAT641, VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_DGT_INV):
		{
			/* Compare 64-bit floats for equal; invert nan test */
			VM_STK_INT1 = !jit_float64_le(VM_STK_FLOAT641, VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_DGE_INV):
		{
			/* Compare 64-bit floats for equal; invert nan test */
			VM_STK_INT1 = !jit_float64_lt(VM_STK_FLOAT641, VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFEQ):
		{
			/* Compare native floats for equal */
			VM_STK_INT1 = jit_nfloat_eq(VM_STK_NFLOAT1, VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFNE):
		{
			/* Compare native floats for not equal */
			VM_STK_INT1 = jit_nfloat_ne(VM_STK_NFLOAT1, VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFLT):
		{
			/* Compare native floats for less than */
			VM_STK_INT1 = jit_nfloat_lt(VM_STK_NFLOAT1, VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFLE):
		{
			/* Compare native floats for less than or equal */
			VM_STK_INT1 = jit_nfloat_le(VM_STK_NFLOAT1, VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFGT):
		{
			/* Compare native floats for greater than */
			VM_STK_INT1 = jit_nfloat_gt(VM_STK_NFLOAT1, VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFGE):
		{
			/* Compare native floats for greater than or equal */
			VM_STK_INT1 = jit_nfloat_ge(VM_STK_NFLOAT1, VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFEQ_INV):
		{
			/* Compare native floats for equal; invert nan test */
			VM_STK_INT1 = !jit_nfloat_ne(VM_STK_NFLOAT1, VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFNE_INV):
		{
			/* Compare native floats for not equal; invert nan test */
			VM_STK_INT1 = !jit_nfloat_eq(VM_STK_NFLOAT1, VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFLT_INV):
		{
			/* Compare native floats for less than; invert nan test */
			VM_STK_INT1 = !jit_nfloat_ge(VM_STK_NFLOAT1, VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFLE_INV):
		{
			/* Compare native floats for less than or equal; invert nan test */
			VM_STK_INT1 = !jit_nfloat_gt(VM_STK_NFLOAT1, VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFGT_INV):
		{
			/* Compare native floats for greater than; invert nan test */
			VM_STK_INT1 = !jit_nfloat_le(VM_STK_NFLOAT1, VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFGE_INV):
		{
			/* Compare native floats for greater or equal; invert nan test */
			VM_STK_INT1 = !jit_nfloat_lt(VM_STK_NFLOAT1, VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_IS_FNAN):
		{
			/* Check a 32-bit float for "not a number" */
			VM_STK_INT0 = jit_float32_is_nan(VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_IS_FINF):
		{
			/* Check a 32-bit float for "infinity" */
			VM_STK_INT0 = jit_float32_is_inf(VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_IS_FFINITE):
		{
			/* Check a 32-bit float for "finite" */
			VM_STK_INT0 = jit_float32_is_finite(VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_IS_DNAN):
		{
			/* Check a 64-bit float for "not a number" */
			VM_STK_INT0 = jit_float64_is_nan(VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_IS_DINF):
		{
			/* Check a 64-bit float for "infinity" */
			VM_STK_INT0 = jit_float64_is_inf(VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_IS_DFINITE):
		{
			/* Check a 64-bit float for "finite" */
			VM_STK_INT0 = jit_float64_is_finite(VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_IS_NFNAN):
		{
			/* Check a native float for "not a number" */
			VM_STK_INT0 = jit_nfloat_is_nan(VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_IS_NFINF):
		{
			/* Check a native float for "infinity" */
			VM_STK_INT0 = jit_nfloat_is_inf(VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_IS_NFFINITE):
		{
			/* Check a native float for "finite" */
			VM_STK_INT0 = jit_nfloat_is_finite(VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		/******************************************************************
		 * Mathematical functions.
		 ******************************************************************/

		VMCASE(JIT_OP_FACOS):
		{
			/* Compute 32-bit float "acos" */
			VM_STK_FLOAT320 = jit_float32_acos(VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_FASIN):
		{
			/* Compute 32-bit float "asin" */
			VM_STK_FLOAT320 = jit_float32_asin(VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_FATAN):
		{
			/* Compute 32-bit float "atan" */
			VM_STK_FLOAT320 = jit_float32_atan(VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_FATAN2):
		{
			/* Compute 32-bit float "atan2" */
			VM_STK_FLOAT321 = jit_float32_atan2
				(VM_STK_FLOAT321, VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_FCEIL):
		{
			/* Compute 32-bit float "ceil" */
			VM_STK_FLOAT320 = jit_float32_ceil(VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_FCOS):
		{
			/* Compute 32-bit float "cos" */
			VM_STK_FLOAT320 = jit_float32_cos(VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_FCOSH):
		{
			/* Compute 32-bit float "cosh" */
			VM_STK_FLOAT320 = jit_float32_cosh(VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_FEXP):
		{
			/* Compute 32-bit float "exp" */
			VM_STK_FLOAT320 = jit_float32_exp(VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_FFLOOR):
		{
			/* Compute 32-bit float "floor" */
			VM_STK_FLOAT320 = jit_float32_floor(VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_FLOG):
		{
			/* Compute 32-bit float "log" */
			VM_STK_FLOAT320 = jit_float32_log(VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_FLOG10):
		{
			/* Compute 32-bit float "log10" */
			VM_STK_FLOAT320 = jit_float32_log10(VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_FPOW):
		{
			/* Compute 32-bit float "pow" */
			VM_STK_FLOAT321 = jit_float32_pow
				(VM_STK_FLOAT321, VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_FRINT):
		{
			/* Compute 32-bit float "rint" */
			VM_STK_FLOAT320 = jit_float32_rint(VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_FROUND):
		{
			/* Compute 32-bit float "round" */
			VM_STK_FLOAT320 = jit_float32_round(VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_FSIN):
		{
			/* Compute 32-bit float "sin" */
			VM_STK_FLOAT320 = jit_float32_sin(VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_FSINH):
		{
			/* Compute 32-bit float "sinh" */
			VM_STK_FLOAT320 = jit_float32_sinh(VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_FSQRT):
		{
			/* Compute 32-bit float "sqrt" */
			VM_STK_FLOAT320 = jit_float32_sqrt(VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_FTAN):
		{
			/* Compute 32-bit float "tan" */
			VM_STK_FLOAT320 = jit_float32_tan(VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_FTANH):
		{
			/* Compute 32-bit float "tanh" */
			VM_STK_FLOAT320 = jit_float32_tanh(VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_DACOS):
		{
			/* Compute 64-bit float "acos" */
			VM_STK_FLOAT640 = jit_float64_acos(VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_DASIN):
		{
			/* Compute 64-bit float "asin" */
			VM_STK_FLOAT640 = jit_float64_asin(VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_DATAN):
		{
			/* Compute 64-bit float "atan" */
			VM_STK_FLOAT640 = jit_float64_atan(VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_DATAN2):
		{
			/* Compute 64-bit float "atan2" */
			VM_STK_FLOAT641 = jit_float64_atan2
				(VM_STK_FLOAT641, VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_DCEIL):
		{
			/* Compute 64-bit float "ceil" */
			VM_STK_FLOAT640 = jit_float64_ceil(VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_DCOS):
		{
			/* Compute 64-bit float "cos" */
			VM_STK_FLOAT640 = jit_float64_cos(VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_DCOSH):
		{
			/* Compute 64-bit float "cosh" */
			VM_STK_FLOAT640 = jit_float64_cosh(VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_DEXP):
		{
			/* Compute 64-bit float "exp" */
			VM_STK_FLOAT640 = jit_float64_exp(VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_DFLOOR):
		{
			/* Compute 64-bit float "floor" */
			VM_STK_FLOAT640 = jit_float64_floor(VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_DLOG):
		{
			/* Compute 64-bit float "log" */
			VM_STK_FLOAT640 = jit_float64_log(VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_DLOG10):
		{
			/* Compute 64-bit float "log10" */
			VM_STK_FLOAT640 = jit_float64_log10(VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_DPOW):
		{
			/* Compute 64-bit float "pow" */
			VM_STK_FLOAT641 = jit_float64_pow
				(VM_STK_FLOAT641, VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_DRINT):
		{
			/* Compute 64-bit float "rint" */
			VM_STK_FLOAT640 = jit_float64_rint(VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_DROUND):
		{
			/* Compute 64-bit float "round" */
			VM_STK_FLOAT640 = jit_float64_round(VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_DSIN):
		{
			/* Compute 64-bit float "sin" */
			VM_STK_FLOAT640 = jit_float64_sin(VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_DSINH):
		{
			/* Compute 64-bit float "sinh" */
			VM_STK_FLOAT640 = jit_float64_sinh(VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_DSQRT):
		{
			/* Compute 64-bit float "sqrt" */
			VM_STK_FLOAT640 = jit_float64_sqrt(VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_DTAN):
		{
			/* Compute 64-bit float "tan" */
			VM_STK_FLOAT640 = jit_float64_tan(VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_DTANH):
		{
			/* Compute 64-bit float "tanh" */
			VM_STK_FLOAT640 = jit_float64_tanh(VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFACOS):
		{
			/* Compute native float "acos" */
			VM_STK_NFLOAT0 = jit_nfloat_acos(VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFASIN):
		{
			/* Compute native float "asin" */
			VM_STK_NFLOAT0 = jit_nfloat_asin(VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFATAN):
		{
			/* Compute native float "atan" */
			VM_STK_NFLOAT0 = jit_nfloat_atan(VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFATAN2):
		{
			/* Compute native float "atan2" */
			VM_STK_NFLOAT1 = jit_nfloat_atan2
				(VM_STK_NFLOAT1, VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFCEIL):
		{
			/* Compute native float "ceil" */
			VM_STK_NFLOAT0 = jit_nfloat_ceil(VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFCOS):
		{
			/* Compute native float "cos" */
			VM_STK_NFLOAT0 = jit_nfloat_cos(VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFCOSH):
		{
			/* Compute native float "cosh" */
			VM_STK_NFLOAT0 = jit_nfloat_cosh(VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFEXP):
		{
			/* Compute native float "exp" */
			VM_STK_NFLOAT0 = jit_nfloat_exp(VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFFLOOR):
		{
			/* Compute native float "floor" */
			VM_STK_NFLOAT0 = jit_nfloat_floor(VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFLOG):
		{
			/* Compute native float "log" */
			VM_STK_NFLOAT0 = jit_nfloat_log(VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFLOG10):
		{
			/* Compute native float "log10" */
			VM_STK_NFLOAT0 = jit_nfloat_log10(VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFPOW):
		{
			/* Compute native float "pow" */
			VM_STK_NFLOAT1 = jit_nfloat_pow
				(VM_STK_NFLOAT1, VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFRINT):
		{
			/* Compute native float "rint" */
			VM_STK_NFLOAT0 = jit_nfloat_rint(VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFROUND):
		{
			/* Compute native float "round" */
			VM_STK_NFLOAT0 = jit_nfloat_round(VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFSIN):
		{
			/* Compute native float "sin" */
			VM_STK_NFLOAT0 = jit_nfloat_sin(VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFSINH):
		{
			/* Compute native float "sinh" */
			VM_STK_NFLOAT0 = jit_nfloat_sinh(VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFSQRT):
		{
			/* Compute native float "sqrt" */
			VM_STK_NFLOAT0 = jit_nfloat_sqrt(VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFTAN):
		{
			/* Compute native float "tan" */
			VM_STK_NFLOAT0 = jit_nfloat_tan(VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFTANH):
		{
			/* Compute native float "tanh" */
			VM_STK_NFLOAT0 = jit_nfloat_tanh(VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		/******************************************************************
		 * Absolute, minimum, maximum, and sign.
		 ******************************************************************/

		VMCASE(JIT_OP_IABS):
		{
			/* Compute the absolute value of a signed 32-bit integer value */
			VM_STK_INT0 = jit_int_abs(VM_STK_INT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_LABS):
		{
			/* Compute the absolute value of a signed 64-bit integer value */
			VM_STK_LONG0 = jit_long_abs(VM_STK_LONG0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_FABS):
		{
			/* Compute the absolute value of a 32-bit float value */
			VM_STK_FLOAT320 = jit_float32_abs(VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_DABS):
		{
			/* Compute the absolute value of a 64-bit float value */
			VM_STK_FLOAT640 = jit_float64_abs(VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFABS):
		{
			/* Compute the absolute value of a native float value */
			VM_STK_NFLOAT0 = jit_nfloat_abs(VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_IMIN):
		{
			/* Compute the minimum of two signed 32-bit integer values */
			VM_STK_INT1 = jit_int_min(VM_STK_INT1, VM_STK_INT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_IMIN_UN):
		{
			/* Compute the minimum of two unsigned 32-bit integer values */
			VM_STK_UINT1 = jit_uint_min(VM_STK_UINT1, VM_STK_UINT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LMIN):
		{
			/* Compute the minimum of two signed 64-bit integer values */
			VM_STK_LONG1 = jit_long_min(VM_STK_LONG1, VM_STK_LONG0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LMIN_UN):
		{
			/* Compute the minimum of two unsigned 64-bit integer values */
			VM_STK_ULONG1 = jit_ulong_min(VM_STK_ULONG1, VM_STK_ULONG0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_FMIN):
		{
			/* Compute the minimum of two 32-bit float values */
			VM_STK_FLOAT321 = jit_float32_min(VM_STK_FLOAT321, VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_DMIN):
		{
			/* Compute the minimum of two 64-bit float values */
			VM_STK_FLOAT641 = jit_float64_min(VM_STK_FLOAT641, VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFMIN):
		{
			/* Compute the minimum of two native float values */
			VM_STK_NFLOAT1 = jit_nfloat_min(VM_STK_NFLOAT1, VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_IMAX):
		{
			/* Compute the maximum of two signed 32-bit integer values */
			VM_STK_INT1 = jit_int_max(VM_STK_INT1, VM_STK_INT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_IMAX_UN):
		{
			/* Compute the maximum of two unsigned 32-bit integer values */
			VM_STK_UINT1 = jit_uint_max(VM_STK_UINT1, VM_STK_UINT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LMAX):
		{
			/* Compute the maximum of two signed 64-bit integer values */
			VM_STK_LONG1 = jit_long_max(VM_STK_LONG1, VM_STK_LONG0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LMAX_UN):
		{
			/* Compute the maximum of two unsigned 64-bit integer values */
			VM_STK_ULONG1 = jit_ulong_max(VM_STK_ULONG1, VM_STK_ULONG0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_FMAX):
		{
			/* Compute the maximum of two 32-bit float values */
			VM_STK_FLOAT321 = jit_float32_max(VM_STK_FLOAT321, VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_DMAX):
		{
			/* Compute the maximum of two 64-bit float values */
			VM_STK_FLOAT641 = jit_float64_max(VM_STK_FLOAT641, VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFMAX):
		{
			/* Compute the maximum of two native float values */
			VM_STK_NFLOAT1 = jit_nfloat_max(VM_STK_NFLOAT1, VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_ISIGN):
		{
			/* Compute the sign of a signed 32-bit integer value */
			VM_STK_INT0 = jit_int_sign(VM_STK_INT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_LSIGN):
		{
			/* Compute the sign of a signed 64-bit integer value */
			VM_STK_INT0 = jit_long_sign(VM_STK_LONG0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_FSIGN):
		{
			/* Compute the sign of a 32-bit float value */
			VM_STK_INT0 = jit_float32_sign(VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_DSIGN):
		{
			/* Compute the sign of a 64-bit float value */
			VM_STK_INT0 = jit_float64_sign(VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_NFSIGN):
		{
			/* Compute the sign of a native float value */
			VM_STK_INT0 = jit_nfloat_sign(VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		/******************************************************************
		 * Pointer check opcodes.
		 ******************************************************************/

		VMCASE(JIT_OP_CHECK_NULL):
		{
			/* Check the top of stack to see if it is null */
			if(!VM_STK_PTR0)
			{
				VM_BUILTIN(JIT_RESULT_NULL_REFERENCE);
			}
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_CHECK_NULL_N):
		{
			/* Check a pointer for null "n" items down the stack.
			   This opcode is specific to the interpreter */
			if(!(stacktop[VM_NINT_ARG].ptr_value))
			{
				VM_BUILTIN(JIT_RESULT_NULL_REFERENCE);
			}
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

		/******************************************************************
		 * Function calls.
		 ******************************************************************/

		VMCASE(JIT_OP_CALL):
		VMCASE(JIT_OP_CALL_TAIL):
		{
			/* Call a function that is under the control of the JIT */
			call_func = (jit_function_t)VM_NINT_ARG;
			VM_MODIFY_PC_AND_STACK(2, 0);
			entry = call_func->entry_point;
			_jit_backtrace_push(&call_trace, pc, 0, 0);
			if(!entry)
			{
				entry = _jit_function_compile_on_demand(call_func);
			}
			_jit_run_function((jit_function_interp_t)entry, stacktop,
							  return_area);
			_jit_backtrace_pop();
		}
		VMBREAK;

		VMCASE(JIT_OP_CALL_INDIRECT):
		{
			/* Call a native function via an indirect pointer */
			tempptr = (void *)VM_NINT_ARG;
			temparg = VM_NINT_ARG2;
			VM_MODIFY_PC_AND_STACK(3, 2);
			_jit_backtrace_push(&call_trace, pc, 0, 0);
			apply_from_interpreter((jit_type_t)tempptr,
								   (void *)VM_STK_PTRP2,
								   stacktop,
								   (unsigned int)temparg,
								   VM_STK_PTRP);
			_jit_backtrace_pop();
		}
		VMBREAK;

		VMCASE(JIT_OP_CALL_VTABLE_PTR):
		{
			/* Call a JIT-managed function via an indirect vtable pointer */
			call_func = (jit_function_t)(VM_STK_PTR0);
			if(!call_func)
			{
				VM_BUILTIN(JIT_RESULT_NULL_FUNCTION);
			}
			VM_MODIFY_PC_AND_STACK(1, 1);
			entry = call_func->entry_point;
			_jit_backtrace_push(&call_trace, pc, 0, 0);
			if(!entry)
			{
				entry = _jit_function_compile_on_demand(call_func);
			}
			_jit_run_function((jit_function_interp_t)entry, stacktop,
							  return_area);
			_jit_backtrace_pop();
		}
		VMBREAK;

		VMCASE(JIT_OP_CALL_EXTERNAL):
		{
			/* Call an external native function */
			tempptr = (void *)VM_NINT_ARG;
			tempptr2 = (void *)VM_NINT_ARG2;
			temparg = VM_NINT_ARG3;
			VM_MODIFY_PC_AND_STACK(4, 1);
			_jit_backtrace_push(&call_trace, pc, 0, 0);
			apply_from_interpreter((jit_type_t)tempptr,
								   (void *)tempptr2,
								   stacktop,
								   (unsigned int)temparg,
								   VM_STK_PTRP);
			_jit_backtrace_pop();
		}
		VMBREAK;

		VMCASE(JIT_OP_RETURN):
		{
			/* Return from the current function, with no result */
			if(jbuf)
			{
				_jit_unwind_pop_setjmp();
			}
			return;
		}
		/* Not reached */

		VMCASE(JIT_OP_RETURN_INT):
		{
			/* Return from the current function, with an integer result */
			return_area->int_value = VM_STK_INT0;
			if(jbuf)
			{
				_jit_unwind_pop_setjmp();
			}
			return;
		}
		/* Not reached */

		VMCASE(JIT_OP_RETURN_LONG):
		{
			/* Return from the current function, with a long result */
			return_area->long_value = VM_STK_LONG0;
			if(jbuf)
			{
				_jit_unwind_pop_setjmp();
			}
			return;
		}
		/* Not reached */

		VMCASE(JIT_OP_RETURN_FLOAT32):
		{
			/* Return from the current function, with a 32-bit float result */
			return_area->float32_value = VM_STK_FLOAT320;
			if(jbuf)
			{
				_jit_unwind_pop_setjmp();
			}
			return;
		}
		/* Not reached */

		VMCASE(JIT_OP_RETURN_FLOAT64):
		{
			/* Return from the current function, with a 64-bit float result */
			return_area->float64_value = VM_STK_FLOAT640;
			if(jbuf)
			{
				_jit_unwind_pop_setjmp();
			}
			return;
		}
		/* Not reached */

		VMCASE(JIT_OP_RETURN_NFLOAT):
		{
			/* Return from the current function, with a native float result */
			return_area->nfloat_value = VM_STK_NFLOAT0;
			if(jbuf)
			{
				_jit_unwind_pop_setjmp();
			}
			return;
		}
		/* Not reached */

		VMCASE(JIT_OP_RETURN_SMALL_STRUCT):
		{
			/* Return from the current function, with a small structure */
		#if JIT_APPLY_MAX_STRUCT_IN_REG != 0
			jit_memcpy(return_area->struct_value, VM_STK_PTR0,
					   (unsigned int)VM_NINT_ARG);
		#endif
			if(jbuf)
			{
				_jit_unwind_pop_setjmp();
			}
			return;
		}
		/* Not reached */

		VMCASE(JIT_OP_SETUP_FOR_NESTED):
		{
			/* Set up to call a nested function who is our child */
			stacktop[-1].ptr_value = args;
			stacktop[-2].ptr_value = frame;
			VM_MODIFY_PC_AND_STACK(1, -2);
		}
		VMBREAK;

		VMCASE(JIT_OP_SETUP_FOR_SIBLING):
		{
			/* Set up to call a nested function who is our sibling, a sibling
			   of one of our ancestors, or one of our ancestors directly */
			temparg = VM_NINT_ARG;
			tempptr = &(args[0]);
			while(temparg > 0)
			{
				tempptr = (((jit_item *)tempptr)[1]).ptr_value;
				--temparg;
			}
			stacktop[-1].ptr_value = (((jit_item *)tempptr)[1]).ptr_value;
			stacktop[-2].ptr_value = (((jit_item *)tempptr)[0]).ptr_value;
			VM_MODIFY_PC_AND_STACK(1, -2);
		}
		VMBREAK;

		VMCASE(JIT_OP_IMPORT_LOCAL):
		{
			/* Import the address of a local variable from an outer scope */
			temparg = VM_NINT_ARG2;
			tempptr = args[0].ptr_value;
			tempptr2 = args[1].ptr_value;
			while(temparg > 1)
			{
				tempptr = ((jit_item *)tempptr2)[0].ptr_value;
				tempptr2 = ((jit_item *)tempptr2)[1].ptr_value;
				--temparg;
			}
			VM_STK_PTRP = ((jit_item *)tempptr) + VM_NINT_ARG;
			VM_MODIFY_PC_AND_STACK(3, -1);
		}
		VMBREAK;

		VMCASE(JIT_OP_IMPORT_ARG):
		{
			/* Import the address of an argument from an outer scope */
			temparg = VM_NINT_ARG2;
			tempptr = args[1].ptr_value;
			while(temparg > 1)
			{
				tempptr = ((jit_item *)tempptr)[1].ptr_value;
				--temparg;
			}
			VM_STK_PTRP = ((jit_item *)tempptr) + VM_NINT_ARG;
			VM_MODIFY_PC_AND_STACK(3, -1);
		}
		VMBREAK;

		VMCASE(JIT_OP_PUSH_STRUCT):
		{
			/* Push a structure value onto the stack, given a pointer to it */
			tempptr = VM_STK_PTR0;
			temparg = VM_NINT_ARG;
			stacktop -= (JIT_NUM_ITEMS_IN_STRUCT(temparg) - 1);
			jit_memcpy(stacktop, tempptr, (unsigned int)temparg);
			VM_MODIFY_PC_AND_STACK(2, 0);
		}
		VMBREAK;

		/******************************************************************
		 * Exception handling.
		 ******************************************************************/

		VMCASE(JIT_OP_THROW):
		{
			/* Throw an exception, which may be handled in this function */
			exception_object = VM_STK_PTR0;
		handle_exception:
			if(jit_function_from_pc(func->func->context, pc, &handler)
					== func->func && handler != 0)
			{
				/* We have an appropriate "catch" handler in this function */
				pc = (void **)handler;
				stacktop = frame - 1;
				stacktop->ptr_value = exception_object;
			}
			else
			{
				/* Throw the exception up to the next level */
				_jit_unwind_pop_setjmp();
				jit_exception_throw(exception_object);
			}
		}
		VMBREAK;

		VMCASE(JIT_OP_LOAD_PC):
		{
			/* Load the current program counter onto the stack */
			VM_STK_PTRP = (void *)pc;
			VM_MODIFY_PC_AND_STACK(1, -1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LEAVE_FINALLY):
		{
			/* Return from a "finally" handler */
			pc = (void **)VM_STK_PTR0;
			VM_MODIFY_STACK(1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LEAVE_FILTER):
		{
			/* Return from a "filter" handler: pc and value on stack */
			pc = (void **)(stacktop[1].ptr_value);
			stacktop[1] = stacktop[0];
			VM_MODIFY_STACK(1);
		}
		VMBREAK;

		VMCASE(JIT_OP_CALL_FILTER):
		{
			/* Call a "filter" handler with pc and value on stack */
			stacktop[-1] = stacktop[0];
			stacktop[0].ptr_value = (void *)(pc + 2);
			VM_MODIFY_STACK(-1);
			pc = VM_BR_TARGET;
		}
		VMBREAK;

		VMCASE(JIT_OP_CALL_FINALLY):
		{
			/* Call a "finally" handler */
			VM_STK_PTRP = (void *)(pc + 2);
			VM_MODIFY_STACK(-1);
			pc = VM_BR_TARGET;
		}
		VMBREAK;

		/******************************************************************
		 * Pointer-relative loads and stores.
		 ******************************************************************/

		VMCASE(JIT_OP_LOAD_RELATIVE_SBYTE):
		{
			/* Load a signed 8-bit integer from a relative pointer */
			VM_STK_INT0 = *VM_REL(jit_sbyte, VM_STK_PTR0);
			VM_MODIFY_PC_AND_STACK(2, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_LOAD_RELATIVE_UBYTE):
		{
			/* Load an unsigned 8-bit integer from a relative pointer */
			VM_STK_INT0 = *VM_REL(jit_ubyte, VM_STK_PTR0);
			VM_MODIFY_PC_AND_STACK(2, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_LOAD_RELATIVE_SHORT):
		{
			/* Load a signed 16-bit integer from a relative pointer */
			VM_STK_INT0 = *VM_REL(jit_short, VM_STK_PTR0);
			VM_MODIFY_PC_AND_STACK(2, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_LOAD_RELATIVE_USHORT):
		{
			/* Load an unsigned 16-bit integer from a relative pointer */
			VM_STK_INT0 = *VM_REL(jit_ushort, VM_STK_PTR0);
			VM_MODIFY_PC_AND_STACK(2, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_LOAD_RELATIVE_INT):
		{
			/* Load a 32-bit integer from a relative pointer */
			VM_STK_INT0 = *VM_REL(jit_int, VM_STK_PTR0);
			VM_MODIFY_PC_AND_STACK(2, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_LOAD_RELATIVE_LONG):
		{
			/* Load a 64-bit integer from a relative pointer */
			VM_STK_LONG0 = *VM_REL(jit_long, VM_STK_PTR0);
			VM_MODIFY_PC_AND_STACK(2, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_LOAD_RELATIVE_FLOAT32):
		{
			/* Load a 32-bit float from a relative pointer */
			VM_STK_FLOAT320 = *VM_REL(jit_float32, VM_STK_PTR0);
			VM_MODIFY_PC_AND_STACK(2, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_LOAD_RELATIVE_FLOAT64):
		{
			/* Load a 64-bit float from a relative pointer */
			VM_STK_FLOAT640 = *VM_REL(jit_float64, VM_STK_PTR0);
			VM_MODIFY_PC_AND_STACK(2, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_LOAD_RELATIVE_NFLOAT):
		{
			/* Load a native float from a relative pointer */
			VM_STK_NFLOAT0 = *VM_REL(jit_nfloat, VM_STK_PTR0);
			VM_MODIFY_PC_AND_STACK(2, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_LOAD_RELATIVE_STRUCT):
		{
			/* Load a structure from a relative pointer */
			tempptr = VM_REL(void, VM_STK_PTR0);
			temparg = VM_NINT_ARG2;
			stacktop -= (JIT_NUM_ITEMS_IN_STRUCT(temparg) - 1);
			jit_memcpy(stacktop, tempptr, temparg);
			VM_MODIFY_PC_AND_STACK(3, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_STORE_RELATIVE_BYTE):
		{
			/* Store an 8-bit integer value to a relative pointer */
			*VM_REL(jit_sbyte, VM_STK_PTR1) = (jit_sbyte)VM_STK_INT0;
			VM_MODIFY_PC_AND_STACK(2, 2);
		}
		VMBREAK;

		VMCASE(JIT_OP_STORE_RELATIVE_SHORT):
		{
			/* Store a 16-bit integer value to a relative pointer */
			*VM_REL(jit_short, VM_STK_PTR1) = (jit_short)VM_STK_INT0;
			VM_MODIFY_PC_AND_STACK(2, 2);
		}
		VMBREAK;

		VMCASE(JIT_OP_STORE_RELATIVE_INT):
		{
			/* Store a 32-bit integer value to a relative pointer */
			*VM_REL(jit_int, VM_STK_PTR1) = VM_STK_INT0;
			VM_MODIFY_PC_AND_STACK(2, 2);
		}
		VMBREAK;

		VMCASE(JIT_OP_STORE_RELATIVE_LONG):
		{
			/* Store a 64-bit integer value to a relative pointer */
			*VM_REL(jit_long, VM_STK_PTR1) = VM_STK_LONG0;
			VM_MODIFY_PC_AND_STACK(2, 2);
		}
		VMBREAK;

		VMCASE(JIT_OP_STORE_RELATIVE_FLOAT32):
		{
			/* Store a 32-bit float value to a relative pointer */
			*VM_REL(jit_float32, VM_STK_PTR1) = VM_STK_FLOAT320;
			VM_MODIFY_PC_AND_STACK(2, 2);
		}
		VMBREAK;

		VMCASE(JIT_OP_STORE_RELATIVE_FLOAT64):
		{
			/* Store a 64-bit float value to a relative pointer */
			*VM_REL(jit_float64, VM_STK_PTR1) = VM_STK_FLOAT640;
			VM_MODIFY_PC_AND_STACK(2, 2);
		}
		VMBREAK;

		VMCASE(JIT_OP_STORE_RELATIVE_NFLOAT):
		{
			/* Store a native float value to a relative pointer */
			*VM_REL(jit_nfloat, VM_STK_PTR1) = VM_STK_NFLOAT0;
			VM_MODIFY_PC_AND_STACK(2, 2);
		}
		VMBREAK;

		VMCASE(JIT_OP_STORE_RELATIVE_STRUCT):
		{
			/* Store a structure value to a relative pointer */
			temparg = VM_NINT_ARG2;
			tempptr = stacktop;
			stacktop += JIT_NUM_ITEMS_IN_STRUCT(temparg);
			jit_memcpy(VM_REL(void, VM_STK_PTR0), tempptr, temparg);
			VM_MODIFY_PC_AND_STACK(3, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_ADD_RELATIVE):
		{
			/* Add a relative offset to a pointer */
			VM_STK_PTR0 = VM_REL(void, VM_STK_PTR0);
			VM_MODIFY_PC_AND_STACK(2, 0);
		}
		VMBREAK;

		/******************************************************************
		 * Array element loads and stores.
		 ******************************************************************/

		VMCASE(JIT_OP_LOAD_ELEMENT_SBYTE):
		{
			/* Load a signed 8-bit integer value from an array */
			VM_STK_INT1 = VM_LOAD_ELEM(jit_sbyte);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LOAD_ELEMENT_UBYTE):
		{
			/* Load an unsigned 8-bit integer value from an array */
			VM_STK_INT1 = VM_LOAD_ELEM(jit_ubyte);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LOAD_ELEMENT_SHORT):
		{
			/* Load a signed 16-bit integer value from an array */
			VM_STK_INT1 = VM_LOAD_ELEM(jit_short);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LOAD_ELEMENT_USHORT):
		{
			/* Load an unsigned 16-bit integer value from an array */
			VM_STK_INT1 = VM_LOAD_ELEM(jit_ushort);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LOAD_ELEMENT_INT):
		{
			/* Load a signed 32-bit integer value from an array */
			VM_STK_INT1 = VM_LOAD_ELEM(jit_int);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LOAD_ELEMENT_UINT):
		{
			/* Load an unsigned 32-bit integer value from an array */
			VM_STK_UINT1 = VM_LOAD_ELEM(jit_uint);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LOAD_ELEMENT_LONG):
		{
			/* Load a signed 64-bit integer value from an array */
			VM_STK_LONG1 = VM_LOAD_ELEM(jit_long);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LOAD_ELEMENT_ULONG):
		{
			/* Load an unsigned 64-bit integer value from an array */
			VM_STK_ULONG1 = VM_LOAD_ELEM(jit_ulong);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LOAD_ELEMENT_FLOAT32):
		{
			/* Load a 32-bit float value from an array */
			VM_STK_FLOAT321 = VM_LOAD_ELEM(jit_float32);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LOAD_ELEMENT_FLOAT64):
		{
			/* Load a 64-bit float value from an array */
			VM_STK_FLOAT641 = VM_LOAD_ELEM(jit_float64);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LOAD_ELEMENT_NFLOAT):
		{
			/* Load a native float value from an array */
			VM_STK_NFLOAT1 = VM_LOAD_ELEM(jit_nfloat);
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_STORE_ELEMENT_BYTE):
		{
			/* Store a 8-bit integer value to an array */
			VM_STORE_ELEM(jit_sbyte, VM_STK_INT0);
			VM_MODIFY_PC_AND_STACK(1, 3);
		}
		VMBREAK;

		VMCASE(JIT_OP_STORE_ELEMENT_SHORT):
		{
			/* Store a 16-bit integer value to an array */
			VM_STORE_ELEM(jit_short, VM_STK_INT0);
			VM_MODIFY_PC_AND_STACK(1, 3);
		}
		VMBREAK;

		VMCASE(JIT_OP_STORE_ELEMENT_INT):
		{
			/* Store a 32-bit integer value to an array */
			VM_STORE_ELEM(jit_int, VM_STK_INT0);
			VM_MODIFY_PC_AND_STACK(1, 3);
		}
		VMBREAK;

		VMCASE(JIT_OP_STORE_ELEMENT_LONG):
		{
			/* Store a 64-bit integer value to an array */
			VM_STORE_ELEM(jit_long, VM_STK_LONG0);
			VM_MODIFY_PC_AND_STACK(1, 3);
		}
		VMBREAK;

		VMCASE(JIT_OP_STORE_ELEMENT_FLOAT32):
		{
			/* Store a 32-bit float value to an array */
			VM_STORE_ELEM(jit_float32, VM_STK_FLOAT320);
			VM_MODIFY_PC_AND_STACK(1, 3);
		}
		VMBREAK;

		VMCASE(JIT_OP_STORE_ELEMENT_FLOAT64):
		{
			/* Store a 64-bit float value to an array */
			VM_STORE_ELEM(jit_float64, VM_STK_FLOAT640);
			VM_MODIFY_PC_AND_STACK(1, 3);
		}
		VMBREAK;

		VMCASE(JIT_OP_STORE_ELEMENT_NFLOAT):
		{
			/* Store a native float value to an array */
			VM_STORE_ELEM(jit_nfloat, VM_STK_NFLOAT0);
			VM_MODIFY_PC_AND_STACK(1, 3);
		}
		VMBREAK;

		/******************************************************************
		 * Block operations.
		 ******************************************************************/

		VMCASE(JIT_OP_MEMCPY):
		{
			/* Copy a block of memory */
			jit_memcpy(VM_STK_PTR2, VM_STK_PTR1, VM_STK_NUINT0);
			VM_MODIFY_PC_AND_STACK(1, 3);
		}
		VMBREAK;

		VMCASE(JIT_OP_MEMMOVE):
		{
			/* Move a block of memory */
			jit_memmove(VM_STK_PTR2, VM_STK_PTR1, VM_STK_NUINT0);
			VM_MODIFY_PC_AND_STACK(1, 3);
		}
		VMBREAK;

		VMCASE(JIT_OP_MEMSET):
		{
			/* Set a block of memory to a value */
			jit_memset(VM_STK_PTR2, (int)VM_STK_INT1, VM_STK_NUINT0);
			VM_MODIFY_PC_AND_STACK(1, 3);
		}
		VMBREAK;

		/******************************************************************
		 * Argument variable access opcodes.
		 ******************************************************************/

		VMCASE(JIT_OP_LDARG_SBYTE):
		{
			/* Load a signed 8-bit integer argument onto the stack */
			VM_STK_INTP = *VM_ARG(jit_sbyte);
			VM_MODIFY_PC_AND_STACK(2, -1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LDARG_UBYTE):
		{
			/* Load an unsigned 8-bit integer argument onto the stack */
			VM_STK_INTP = *VM_ARG(jit_ubyte);
			VM_MODIFY_PC_AND_STACK(2, -1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LDARG_SHORT):
		{
			/* Load a signed 16-bit integer argument onto the stack */
			VM_STK_INTP = *VM_ARG(jit_short);
			VM_MODIFY_PC_AND_STACK(2, -1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LDARG_USHORT):
		{
			/* Load an unsigned 16-bit integer argument onto the stack */
			VM_STK_INTP = *VM_ARG(jit_ushort);
			VM_MODIFY_PC_AND_STACK(2, -1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LDARG_INT):
		{
			/* Load a 32-bit integer argument onto the stack */
			VM_STK_INTP = *VM_ARG(jit_int);
			VM_MODIFY_PC_AND_STACK(2, -1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LDARG_LONG):
		{
			/* Load a 64-bit integer argument onto the stack */
			VM_STK_LONGP = *VM_ARG(jit_long);
			VM_MODIFY_PC_AND_STACK(2, -1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LDARG_FLOAT32):
		{
			/* Load a 32-bit float argument onto the stack */
			VM_STK_FLOAT32P = *VM_ARG(jit_float32);
			VM_MODIFY_PC_AND_STACK(2, -1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LDARG_FLOAT64):
		{
			/* Load a 64-bit float argument onto the stack */
			VM_STK_FLOAT64P = *VM_ARG(jit_float64);
			VM_MODIFY_PC_AND_STACK(2, -1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LDARG_NFLOAT):
		{
			/* Load a native float argument onto the stack */
			VM_STK_NFLOATP = *VM_ARG(jit_nfloat);
			VM_MODIFY_PC_AND_STACK(2, -1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LDARG_STRUCT):
		{
			/* Load a structure argument onto the stack */
			temparg = VM_NINT_ARG2;
			stacktop -= JIT_NUM_ITEMS_IN_STRUCT(temparg);
			jit_memcpy(stacktop, VM_ARG(void), temparg);
			VM_MODIFY_PC_AND_STACK(3, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_LDARGA):
		{
			/* Load the address of an argument onto the stack */
			VM_STK_PTRP = VM_ARG(void);
			VM_MODIFY_PC_AND_STACK(2, -1);
		}
		VMBREAK;

		VMCASE(JIT_OP_STARG_BYTE):
		{
			/* Store an 8-bit integer into a stack argument */
			*VM_ARG(jit_sbyte) = (jit_sbyte)VM_STK_INT0;
			VM_MODIFY_PC_AND_STACK(2, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_STARG_SHORT):
		{
			/* Store a 16-bit integer into a stack argument */
			*VM_ARG(jit_short) = (jit_short)VM_STK_INT0;
			VM_MODIFY_PC_AND_STACK(2, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_STARG_INT):
		{
			/* Store a 32-bit integer into a stack argument */
			*VM_ARG(jit_int) = VM_STK_INT0;
			VM_MODIFY_PC_AND_STACK(2, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_STARG_LONG):
		{
			/* Store a 64-bit integer into a stack argument */
			*VM_ARG(jit_long) = VM_STK_LONG0;
			VM_MODIFY_PC_AND_STACK(2, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_STARG_FLOAT32):
		{
			/* Store a 32-bit float into a stack argument */
			*VM_ARG(jit_float32) = VM_STK_FLOAT320;
			VM_MODIFY_PC_AND_STACK(2, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_STARG_FLOAT64):
		{
			/* Store a 64-bit float into a stack argument */
			*VM_ARG(jit_float64) = VM_STK_FLOAT640;
			VM_MODIFY_PC_AND_STACK(2, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_STARG_NFLOAT):
		{
			/* Store a native float into a stack argument */
			*VM_ARG(jit_nfloat) = VM_STK_NFLOAT0;
			VM_MODIFY_PC_AND_STACK(2, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_STARG_STRUCT):
		{
			/* Store a structure value into a stack argument */
			temparg = VM_NINT_ARG2;
			jit_memcpy(VM_ARG(void), stacktop, temparg);
			stacktop += JIT_NUM_ITEMS_IN_STRUCT(temparg);
			VM_MODIFY_PC_AND_STACK(3, 0);
		}
		VMBREAK;

		/******************************************************************
		 * Local variable frame access opcodes.
		 ******************************************************************/

		VMCASE(JIT_OP_LDLOC_SBYTE):
		{
			/* Load a signed 8-bit integer local onto the stack */
			VM_STK_INTP = *VM_LOC(jit_sbyte);
			VM_MODIFY_PC_AND_STACK(2, -1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LDLOC_UBYTE):
		{
			/* Load an unsigned 8-bit integer local onto the stack */
			VM_STK_INTP = *VM_LOC(jit_ubyte);
			VM_MODIFY_PC_AND_STACK(2, -1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LDLOC_SHORT):
		{
			/* Load a signed 16-bit integer local onto the stack */
			VM_STK_INTP = *VM_LOC(jit_short);
			VM_MODIFY_PC_AND_STACK(2, -1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LDLOC_USHORT):
		{
			/* Load an unsigned 16-bit integer local onto the stack */
			VM_STK_INTP = *VM_LOC(jit_ushort);
			VM_MODIFY_PC_AND_STACK(2, -1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LDLOC_INT):
		{
			/* Load a 32-bit integer local onto the stack */
			VM_STK_INTP = *VM_LOC(jit_int);
			VM_MODIFY_PC_AND_STACK(2, -1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LDLOC_LONG):
		{
			/* Load a 64-bit integer local onto the stack */
			VM_STK_LONGP = *VM_LOC(jit_long);
			VM_MODIFY_PC_AND_STACK(2, -1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LDLOC_FLOAT32):
		{
			/* Load a 32-bit float local onto the stack */
			VM_STK_FLOAT32P = *VM_LOC(jit_float32);
			VM_MODIFY_PC_AND_STACK(2, -1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LDLOC_FLOAT64):
		{
			/* Load a 64-bit float local onto the stack */
			VM_STK_FLOAT64P = *VM_LOC(jit_float64);
			VM_MODIFY_PC_AND_STACK(2, -1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LDLOC_NFLOAT):
		{
			/* Load a native float local onto the stack */
			VM_STK_NFLOATP = *VM_LOC(jit_nfloat);
			VM_MODIFY_PC_AND_STACK(2, -1);
		}
		VMBREAK;

		VMCASE(JIT_OP_LDLOC_STRUCT):
		{
			/* Load a structure local onto the stack */
			temparg = VM_NINT_ARG2;
			stacktop -= JIT_NUM_ITEMS_IN_STRUCT(temparg);
			jit_memcpy(stacktop, VM_LOC(void), temparg);
			VM_MODIFY_PC_AND_STACK(3, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_LDLOCA):
		{
			/* Load the address of an local onto the stack */
			VM_STK_PTRP = VM_LOC(void);
			VM_MODIFY_PC_AND_STACK(2, -1);
		}
		VMBREAK;

		VMCASE(JIT_OP_STLOC_BYTE):
		{
			/* Store an 8-bit integer into a stack local */
			*VM_LOC(jit_sbyte) = (jit_sbyte)VM_STK_INT0;
			VM_MODIFY_PC_AND_STACK(2, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_STLOC_SHORT):
		{
			/* Store a 16-bit integer into a stack local */
			*VM_LOC(jit_short) = (jit_short)VM_STK_INT0;
			VM_MODIFY_PC_AND_STACK(2, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_STLOC_INT):
		{
			/* Store a 32-bit integer into a stack local */
			*VM_LOC(jit_int) = VM_STK_INT0;
			VM_MODIFY_PC_AND_STACK(2, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_STLOC_LONG):
		{
			/* Store a 64-bit integer into a stack local */
			*VM_LOC(jit_long) = VM_STK_LONG0;
			VM_MODIFY_PC_AND_STACK(2, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_STLOC_FLOAT32):
		{
			/* Store a 32-bit float into a stack local */
			*VM_LOC(jit_float32) = VM_STK_FLOAT320;
			VM_MODIFY_PC_AND_STACK(2, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_STLOC_FLOAT64):
		{
			/* Store a 64-bit float into a stack local */
			*VM_LOC(jit_float64) = VM_STK_FLOAT640;
			VM_MODIFY_PC_AND_STACK(2, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_STLOC_NFLOAT):
		{
			/* Store a native float into a stack local */
			*VM_LOC(jit_nfloat) = VM_STK_NFLOAT0;
			VM_MODIFY_PC_AND_STACK(2, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_STLOC_STRUCT):
		{
			/* Store a structure value into a stack local */
			temparg = VM_NINT_ARG2;
			jit_memcpy(VM_LOC(void), stacktop, temparg);
			stacktop += JIT_NUM_ITEMS_IN_STRUCT(temparg);
			VM_MODIFY_PC_AND_STACK(3, 0);
		}
		VMBREAK;

		/******************************************************************
		 * Stack management
		 ******************************************************************/

		VMCASE(JIT_OP_POP_STACK):
		{
			/* Pop a specific number of items from the stack */
			temparg = VM_NINT_ARG;
			VM_MODIFY_PC_AND_STACK(2, temparg);
		}
		VMBREAK;

		VMCASE(JIT_OP_POP):
		{
			/* Pop a single item from the stack */
			VM_MODIFY_PC_AND_STACK(1, 1);
		}
		VMBREAK;

		VMCASE(JIT_OP_POP_2):
		{
			/* Pop two items from the stack */
			VM_MODIFY_PC_AND_STACK(1, 2);
		}
		VMBREAK;

		VMCASE(JIT_OP_POP_3):
		{
			/* Pop three items from the stack */
			VM_MODIFY_PC_AND_STACK(1, 3);
		}
		VMBREAK;

		VMCASE(JIT_OP_PUSH_RETURN_INT):
		{
			/* Push an integer return value back onto the stack */
			VM_STK_INTP = return_area->int_value;
			VM_MODIFY_PC_AND_STACK(1, -1);
		}
		VMBREAK;

		VMCASE(JIT_OP_PUSH_RETURN_LONG):
		{
			/* Push a long integer return value back onto the stack */
			VM_STK_LONGP = return_area->long_value;
			VM_MODIFY_PC_AND_STACK(1, -1);
		}
		VMBREAK;

		VMCASE(JIT_OP_PUSH_RETURN_FLOAT32):
		{
			/* Push a 32-bit float return value back onto the stack */
			VM_STK_FLOAT32P = return_area->float32_value;
			VM_MODIFY_PC_AND_STACK(1, -1);
		}
		VMBREAK;

		VMCASE(JIT_OP_PUSH_RETURN_FLOAT64):
		{
			/* Push a 64-bit float return value back onto the stack */
			VM_STK_FLOAT64P = return_area->float64_value;
			VM_MODIFY_PC_AND_STACK(1, -1);
		}
		VMBREAK;

		VMCASE(JIT_OP_PUSH_RETURN_NFLOAT):
		{
			/* Push a native float return value back onto the stack */
			VM_STK_NFLOATP = return_area->nfloat_value;
			VM_MODIFY_PC_AND_STACK(1, -1);
		}
		VMBREAK;

		VMCASE(JIT_OP_PUSH_RETURN_SMALL_STRUCT):
		{
			/* Push a small structure return value back onto the stack */
			temparg = VM_NINT_ARG;
			stacktop -= JIT_NUM_ITEMS_IN_STRUCT(temparg);
		#if JIT_APPLY_MAX_STRUCT_IN_REG != 0
			jit_memcpy(stacktop, return_area->struct_value, temparg);
		#endif
			VM_MODIFY_PC_AND_STACK(2, 0);
		}
		VMBREAK;

		VMCASE(JIT_OP_PUSH_RETURN_AREA_PTR):
		{
			/* Push the address of "return_area" for an external call */
			VM_STK_PTRP = return_area;
			VM_MODIFY_PC_AND_STACK(1, -1);
		}
		VMBREAK;

		/******************************************************************
		 * Stack management
		 ******************************************************************/

		VMCASE(JIT_OP_PUSH_CONST_INT):
		{
			/* Push an integer constant onto the stack */
			VM_STK_INTP = (jit_int)VM_NINT_ARG;
			VM_MODIFY_PC_AND_STACK(2, -1);
		}
		VMBREAK;

		VMCASE(JIT_OP_PUSH_CONST_LONG):
		{
			/* Push a long constant onto the stack */
		#ifdef JIT_NATIVE_INT64
			VM_STK_LONGP = (jit_long)VM_NINT_ARG;
			VM_MODIFY_PC_AND_STACK(2, -1);
		#else
			jit_memcpy(stacktop - 1, pc + 1, sizeof(jit_long));
			VM_MODIFY_PC_AND_STACK
				(1 + (sizeof(jit_long) / sizeof(void *)), -1);
		#endif
		}
		VMBREAK;

		VMCASE(JIT_OP_PUSH_CONST_FLOAT32):
		{
			/* Push a 32-bit float constant onto the stack */
			jit_memcpy(stacktop - 1, pc + 1, sizeof(jit_float32));
			VM_MODIFY_PC_AND_STACK
				(1 + (sizeof(jit_float32) / sizeof(void *)), -1);
		}
		VMBREAK;

		VMCASE(JIT_OP_PUSH_CONST_FLOAT64):
		{
			/* Push a 64-bit float constant onto the stack */
			jit_memcpy(stacktop - 1, pc + 1, sizeof(jit_float64));
			VM_MODIFY_PC_AND_STACK
				(1 + (sizeof(jit_float64) / sizeof(void *)), -1);
		}
		VMBREAK;

		VMCASE(JIT_OP_PUSH_CONST_NFLOAT):
		{
			/* Push a native float constant onto the stack */
			jit_memcpy(stacktop - 1, pc + 1, sizeof(jit_nfloat));
			VM_MODIFY_PC_AND_STACK
				(1 + (sizeof(jit_nfloat) / sizeof(void *)), -1);
		}
		VMBREAK;

		/******************************************************************
		 * Opcodes that aren't used by the interpreter.  These are replaced
		 * by more specific instructions during function compilation.
		 ******************************************************************/

		VMCASE(JIT_OP_IMPORT):
		VMCASE(JIT_OP_COPY_LOAD_SBYTE):
		VMCASE(JIT_OP_COPY_LOAD_UBYTE):
		VMCASE(JIT_OP_COPY_LOAD_SHORT):
		VMCASE(JIT_OP_COPY_LOAD_USHORT):
		VMCASE(JIT_OP_COPY_INT):
		VMCASE(JIT_OP_COPY_LONG):
		VMCASE(JIT_OP_COPY_FLOAT32):
		VMCASE(JIT_OP_COPY_FLOAT64):
		VMCASE(JIT_OP_COPY_NFLOAT):
		VMCASE(JIT_OP_COPY_STRUCT):
		VMCASE(JIT_OP_COPY_STORE_BYTE):
		VMCASE(JIT_OP_COPY_STORE_SHORT):
		VMCASE(JIT_OP_ADDRESS_OF):
		VMCASE(JIT_OP_INCOMING_REG):
		VMCASE(JIT_OP_INCOMING_FRAME_POSN):
		VMCASE(JIT_OP_OUTGOING_REG):
		VMCASE(JIT_OP_RETURN_REG):
		VMCASE(JIT_OP_PUSH_INT):
		VMCASE(JIT_OP_PUSH_LONG):
		VMCASE(JIT_OP_PUSH_FLOAT32):
		VMCASE(JIT_OP_PUSH_FLOAT64):
		VMCASE(JIT_OP_PUSH_NFLOAT):
		VMCASE(JIT_OP_FLUSH_SMALL_STRUCT):
		VMCASE(JIT_OP_END_MARKER):
		VMCASE(JIT_OP_ENTER_CATCH):
		VMCASE(JIT_OP_ENTER_FINALLY):
		VMCASE(JIT_OP_ENTER_FILTER):
		VMCASE(JIT_OP_CALL_FILTER_RETURN):
		VMCASE(JIT_OP_PREPARE_FOR_LEAVE):
		VMCASE(JIT_OP_PREPARE_FOR_RETURN):
		{
			/* Shouldn't happen, but skip the instruction anyway */
			VM_MODIFY_PC_AND_STACK(1, 0);
		}
		VMBREAK;

	}
	VMSWITCHEND

handle_builtin: ;
	jit_exception_builtin(builtin_exception);
}

int jit_function_apply
	(jit_function_t func, void **args, void *return_area)
{
	if(func)
	{
		return jit_function_apply_vararg
			(func, func->signature, args, return_area);
	}
	else
	{
		return jit_function_apply_vararg(func, 0, args, return_area);
	}
}

/* Imported from "jit-rules-interp.c" */
unsigned int _jit_interp_calculate_arg_size
		(jit_function_t func, jit_type_t signature);

int jit_function_apply_vararg
	(jit_function_t func, jit_type_t signature, void **args, void *return_area)
{
	struct jit_backtrace call_trace;
	jit_function_interp_t entry;
	jit_item interp_return_area;
	jit_item *arg_buffer;
	jit_item *temp_arg;
	jit_type_t type;
	unsigned int num_params;
	unsigned int param;
	jit_jmp_buf jbuf;

	/* Push a "setjmp" context onto the stack, so that we can catch
	   any exceptions that are thrown up to this level and prevent
	   them from propagating further */
	_jit_unwind_push_setjmp(&jbuf);
	if(setjmp(jbuf.buf))
	{
		_jit_unwind_pop_setjmp();
		return 1;
	}

	/* Initialize the backtrace information */
	_jit_backtrace_push(&call_trace, 0, 0, 0);

	/* Clear the exception context */
	jit_exception_clear_last();

	/* Bail out if the function is NULL */
	if(!func)
	{
		jit_exception_builtin(JIT_RESULT_NULL_FUNCTION);
	}

	/* Make sure that the function is compiled */
	if(func->is_compiled)
	{
		entry = (jit_function_interp_t)(func->entry_point);
	}
	else
	{
		entry = (jit_function_interp_t)
			_jit_function_compile_on_demand(func);
	}

	/* Populate the low-level argument buffer */
	if(!signature)
	{
		signature = func->signature;
		arg_buffer = (jit_item *)alloca(entry->args_size);
	}
	else if(signature == func->signature)
	{
		arg_buffer = (jit_item *)alloca(entry->args_size);
	}
	else
	{
		arg_buffer = (jit_item *)alloca
			(_jit_interp_calculate_arg_size(func, signature));
	}
	temp_arg = arg_buffer;
	if(func->nested_parent)
	{
		jit_exception_builtin(JIT_RESULT_CALLED_NESTED);
	}
	type = jit_type_get_return(signature);
	if(jit_type_return_via_pointer(type))
	{
		if(!return_area)
		{
			return_area = alloca(jit_type_get_size(type));
		}
		temp_arg->ptr_value = return_area;
		++temp_arg;
	}
	num_params = jit_type_num_params(signature);
	for(param = 0; param < num_params; ++param)
	{
		type = jit_type_normalize
			(jit_type_get_param(signature, param));
		if(!(args[param]))
		{
			jit_exception_builtin(JIT_RESULT_NULL_REFERENCE);
		}
		switch(type->kind)
		{
			case JIT_TYPE_SBYTE:
			{
				temp_arg->int_value = *((jit_sbyte *)(args[param]));
				++temp_arg;
			}
			break;

			case JIT_TYPE_UBYTE:
			{
				temp_arg->int_value = *((jit_ubyte *)(args[param]));
				++temp_arg;
			}
			break;

			case JIT_TYPE_SHORT:
			{
				temp_arg->int_value = *((jit_short *)(args[param]));
				++temp_arg;
			}
			break;

			case JIT_TYPE_USHORT:
			{
				temp_arg->int_value = *((jit_ushort *)(args[param]));
				++temp_arg;
			}
			break;

			case JIT_TYPE_INT:
			case JIT_TYPE_UINT:
			{
				temp_arg->int_value = *((jit_int *)(args[param]));
				++temp_arg;
			}
			break;

			case JIT_TYPE_LONG:
			case JIT_TYPE_ULONG:
			{
				temp_arg->long_value = *((jit_long *)(args[param]));
				++temp_arg;
			}
			break;

			case JIT_TYPE_FLOAT32:
			{
				temp_arg->float32_value =
					*((jit_float32 *)(args[param]));
				++temp_arg;
			}
			break;

			case JIT_TYPE_FLOAT64:
			{
				temp_arg->float64_value =
					*((jit_float64 *)(args[param]));
				++temp_arg;
			}
			break;

			case JIT_TYPE_NFLOAT:
			{
				temp_arg->nfloat_value =
					*((jit_nfloat *)(args[param]));
				++temp_arg;
			}
			break;

			case JIT_TYPE_STRUCT:
			case JIT_TYPE_UNION:
			{
				jit_memcpy(temp_arg, args[param],
						   jit_type_get_size(type));
				temp_arg += JIT_NUM_ITEMS_IN_STRUCT
					(jit_type_get_size(type));
			}
			break;
		}
	}

	/* Run the function */
	_jit_run_function(entry, arg_buffer, &interp_return_area);

	/* Copy the return value into place, if it isn't already there */
	if(return_area)
	{
		type = jit_type_normalize(jit_type_get_return(signature));
		if(type && type != jit_type_void)
		{
			switch(type->kind)
			{
				case JIT_TYPE_SBYTE:
				case JIT_TYPE_UBYTE:
				{
					*((jit_sbyte *)return_area) =
						(jit_sbyte)(interp_return_area.int_value);
				}
				break;

				case JIT_TYPE_SHORT:
				case JIT_TYPE_USHORT:
				{
					*((jit_short *)return_area) =
						(jit_short)(interp_return_area.int_value);
				}
				break;

				case JIT_TYPE_INT:
				case JIT_TYPE_UINT:
				{
					*((jit_int *)return_area) =
						interp_return_area.int_value;
				}
				break;

				case JIT_TYPE_LONG:
				case JIT_TYPE_ULONG:
				{
					*((jit_long *)return_area) =
						interp_return_area.long_value;
				}
				break;

				case JIT_TYPE_FLOAT32:
				{
					*((jit_float32 *)return_area) =
						interp_return_area.float32_value;
				}
				break;

				case JIT_TYPE_FLOAT64:
				{
					*((jit_float64 *)return_area) =
						interp_return_area.float64_value;
				}
				break;

				case JIT_TYPE_NFLOAT:
				{
					*((jit_nfloat *)return_area) =
						interp_return_area.nfloat_value;
				}
				break;

				case JIT_TYPE_STRUCT:
				case JIT_TYPE_UNION:
				{
					if(!jit_type_return_via_pointer(type))
					{
						jit_memcpy(return_area, &interp_return_area,
								   jit_type_get_size(type));
					}
				}
				break;
			}
		}
	}

	/* Pop the "setjmp" context and exit */
	_jit_unwind_pop_setjmp();
	return 1;
}

#endif /* JIT_BACKEND_INTERP */
