/*
 * gen-apply-helper.c - Helper routines to generate apply rules.
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

#include "gen-apply-helper.h"

/*
 * Detect the number of word registers that are used in function calls.
 * We assume that the platform uses less than 32 registers in outgoing calls.
 */
void
detect_word_regs(jit_nint arg1, jit_nint arg2, jit_nint arg3,
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
 * Detect the number of floating-point registers.
 */
void
detect_float_regs(float arg1, float arg2, float arg3,
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
void
detect_double_regs(double arg1, double arg2, double arg3,
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
void
detect_nfloat_regs(jit_nfloat arg1, jit_nfloat arg2, jit_nfloat arg3,
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

/*
 * Dummy functions for helping detect the size and position of "float",
 * "double", and "long double" return values.
 */
float
return_float(void)
{
	return (float)123.0;
}
double
return_double(void)
{
	return (double)456.7;
}
jit_nfloat
return_nfloat(void)
{
	return (jit_nfloat)8901.2;
}

