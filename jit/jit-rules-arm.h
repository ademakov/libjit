/*
 * jit-rules-arm.h - Rules that define the characteristics of the ARM.
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

#ifndef	_JIT_RULES_ARM_H
#define	_JIT_RULES_ARM_H

#ifdef	__cplusplus
extern	"C" {
#endif

/*
 * Information about all of the registers, in allocation order.
 * We use r0-r5 for general-purpose values and r6-r8 for globals.
 *
 * Of the floating-point registers, we only use f0-f3 at present,
 * so that we don't have to worry about saving the values of f4-f7.
 * The floating-point registers are only present on some ARM cores.
 * "_jit_init_backend" will disable the FP registers if they don't exist.
 */
#define	JIT_REG_INFO	\
	{"r0",   0,  1, JIT_REG_WORD | JIT_REG_CALL_USED}, \
	{"r1",   1, -1, JIT_REG_WORD | JIT_REG_CALL_USED}, \
	{"r2",   2,  3, JIT_REG_WORD | JIT_REG_CALL_USED}, \
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
	{"f0",   0, -1, JIT_REG_FLOAT | JIT_REG_CALL_USED}, \
	{"f1",   1, -1, JIT_REG_FLOAT | JIT_REG_CALL_USED}, \
	{"f2",   2, -1, JIT_REG_FLOAT | JIT_REG_CALL_USED}, \
	{"f3",   3, -1, JIT_REG_FLOAT | JIT_REG_CALL_USED},
#define	JIT_NUM_REGS	20

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

#ifdef	__cplusplus
};
#endif

#endif	/* _JIT_RULES_ARM_H */
