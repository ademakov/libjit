/*
 * jit-rules-arm.h - Rules that define the characteristics of the ARM.
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

#ifndef	_JIT_RULES_ARM_H
#define	_JIT_RULES_ARM_H

#include "jit-apply-rules.h"

#ifdef	__cplusplus
extern	"C" {
#endif

/*
 * Determine if this ARM core has floating point registers available.
 */
#if JIT_APPLY_NUM_FLOAT_REGS != 0 || JIT_APPLY_RETURN_FLOATS_AFTER != 0
	#define	JIT_ARM_HAS_FLOAT_REGS	1
#endif

/*
 * Information about all of the registers, in allocation order.
 * We use r0-r5 for general-purpose values and r6-r8 for globals.
 *
 * Of the floating-point registers, we only use f0-f3 at present,
 * so that we don't have to worry about saving the values of f4-f7.
 * The floating-point registers are only present on some ARM cores.
 */
#ifdef JIT_ARM_HAS_FLOAT_REGS
	#define	JIT_REG_ARM_FLOAT	\
	  (JIT_REG_FLOAT32 | JIT_REG_FLOAT64 | JIT_REG_NFLOAT | JIT_REG_CALL_USED)
#else
	#define	JIT_REG_ARM_FLOAT	JIT_REG_FIXED
#endif
#define	JIT_REG_INFO	\
	{"r0",   0,  1, JIT_REG_WORD | JIT_REG_LONG | JIT_REG_CALL_USED}, \
	{"r1",   1, -1, JIT_REG_WORD | JIT_REG_CALL_USED}, \
	{"r2",   2,  3, JIT_REG_WORD | JIT_REG_LONG | JIT_REG_CALL_USED}, \
	{"r3",   3, -1, JIT_REG_WORD | JIT_REG_CALL_USED}, \
	{"r4",   4, -1, JIT_REG_WORD}, \
	{"r5",   5, -1, JIT_REG_WORD}, \
	{"r6",   6, -1, JIT_REG_WORD | JIT_REG_GLOBAL},	\
	{"r7",   7, -1, JIT_REG_WORD | JIT_REG_GLOBAL}, \
	{"r8",   8, -1, JIT_REG_WORD | JIT_REG_GLOBAL}, \
	{"r9",   9, -1, JIT_REG_FIXED},                 	/* pic reg */ \
	{"r10", 10, -1, JIT_REG_FIXED},						/* stack limit */ \
	{"fp",  11, -1, JIT_REG_FIXED | JIT_REG_FRAME},	\
	{"r12", 12, -1, JIT_REG_FIXED | JIT_REG_CALL_USED}, /* work reg */ \
	{"sp",  13, -1, JIT_REG_FIXED | JIT_REG_STACK_PTR}, \
	{"lr",  14, -1, JIT_REG_FIXED}, \
	{"pc",  15, -1, JIT_REG_FIXED}, \
	{"f0",   0, -1, JIT_REG_ARM_FLOAT}, \
	{"f1",   1, -1, JIT_REG_ARM_FLOAT}, \
	{"f2",   2, -1, JIT_REG_ARM_FLOAT}, \
	{"f3",   3, -1, JIT_REG_ARM_FLOAT},
#define	JIT_NUM_REGS		20
#define	JIT_NUM_GLOBAL_REGS	3

/*
 * Define to 1 if we should always load values into registers
 * before operating on them.  i.e. the CPU does not have reg-mem
 * and mem-reg addressing modes.
 */
#define	JIT_ALWAYS_REG_REG		1

/*
 * The maximum number of bytes to allocate for the prolog.
 * This may be shortened once we know the true prolog size.
 */
#define	JIT_PROLOG_SIZE			48

/*
 * Preferred alignment for the start of functions.
 */
#define	JIT_FUNCTION_ALIGNMENT	4

/*
 * Define this to 1 if the platform allows reads and writes on
 * any byte boundary.  Define to 0 if only properly-aligned
 * memory accesses are allowed.
 */
#define	JIT_ALIGN_OVERRIDES		0

/*
 * Extra state information that is added to the "jit_gencode" structure.
 */
#define	JIT_ARM_MAX_CONSTANTS		32
#define	jit_extra_gen_state	\
			int constants[JIT_ARM_MAX_CONSTANTS]; \
			int *fixup_constants[JIT_ARM_MAX_CONSTANTS]; \
			int num_constants; \
			int align_constants; \
			unsigned int *first_constant_use
#define	jit_extra_gen_init(gen)	\
			do { \
				(gen)->num_constants = 0; \
				(gen)->align_constants = 0; \
				(gen)->first_constant_use = 0; \
			} while (0)
#define	jit_extra_gen_cleanup(gen)	do { ; } while (0)

/*
 * Parameter passing rules.  We start by assuming that lr, sp, fp,
   r8, r7, r6, r5, and r4 need to be saved in the local frame.
 */
#define	JIT_CDECL_WORD_REG_PARAMS		{0, 1, 2, 3, -1}
#define	JIT_MAX_WORD_REG_PARAMS			4
#define	JIT_INITIAL_STACK_OFFSET		(sizeof(void *))
#define	JIT_INITIAL_FRAME_SIZE			(8 * sizeof(void *))
#define	JIT_USE_PARAM_AREA				1

#ifdef	__cplusplus
};
#endif

#endif	/* _JIT_RULES_ARM_H */
