/*
 * jit-rules-alpha.h - Rules that define the characteristics of the alpha.
 *
 * Copyright (C) 2006  Southern Storm Software, Pty Ltd.
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

#ifndef	_JIT_RULES_ALPHA_H
#define	_JIT_RULES_ALPHA_H

#ifdef	__cplusplus
extern	"C" {
#endif

/*
 * alpha has 32 64-bit floating-point registers. Each can hold either
 * 1 32-bit float or 1 64-bit double.
 */
#define JIT_REG_ALPHA_FLOAT	\
	(JIT_REG_FLOAT32 | JIT_REG_FLOAT64 | JIT_REG_NFLOAT)

/*
 * alpha has 32 64-bit integer registers that can hold WORD and LONG values
 */
#define JIT_REG_ALPHA_INT	\
	(JIT_REG_WORD | JIT_REG_LONG)

/*
 * Integer registers
 *
 * $0 - function results.
 * $1..$8 - Temp registers. 
 * $9..$14 - Saved registers.
 * $15/$fp - Frame pointer or saved register.
 * $16-$21 - First 6 arguments.
 * $22-$25 - Temp registers.
 * $26 - Return address.
 * $27 - Procedure value or temp register.
 * $28/$at - Reserved for the assembler.
 * $29/$gp - Global pointer.
 * $30/$sp - Stack pointer.
 * $31 - Contains the value 0.
 *
 * Floating-point registers
 *
 * $f0,$f1 - function results. $f0 = real component $f1 = imaginary component
 * $f2..$f9 - Saved registers.
 * $f10..$f15 - Temp registers.
 * $f16..$f21 - First 6 arguments.
 * $f22..$f30 - Temp registers for expression evaluation.
 * $f31 - Contains the value 0.0.
 */
#define JIT_REG_INFO	\
	{   "v0",  0, -1, JIT_REG_FIXED}, \
	{   "t0",  1, -1, JIT_REG_ALPHA_INT | JIT_REG_CALL_USED}, \
	{   "t1",  2, -1, JIT_REG_ALPHA_INT | JIT_REG_CALL_USED}, \
	{   "t2",  3, -1, JIT_REG_ALPHA_INT | JIT_REG_CALL_USED}, \
	{   "t3",  4, -1, JIT_REG_ALPHA_INT | JIT_REG_CALL_USED}, \
	{   "t4",  5, -1, JIT_REG_ALPHA_INT | JIT_REG_CALL_USED}, \
	{   "t5",  6, -1, JIT_REG_ALPHA_INT | JIT_REG_CALL_USED}, \
	{   "t6",  7, -1, JIT_REG_ALPHA_INT | JIT_REG_CALL_USED}, \
	{   "t7",  8, -1, JIT_REG_ALPHA_INT | JIT_REG_CALL_USED}, \
	{   "s0",  9, -1, JIT_REG_ALPHA_INT | JIT_REG_GLOBAL}, \
 	{   "s1", 10, -1, JIT_REG_ALPHA_INT | JIT_REG_GLOBAL}, \
	{   "s2", 11, -1, JIT_REG_ALPHA_INT | JIT_REG_GLOBAL}, \
	{   "s3", 12, -1, JIT_REG_ALPHA_INT | JIT_REG_GLOBAL}, \
	{   "s4", 13, -1, JIT_REG_ALPHA_INT | JIT_REG_GLOBAL}, \
	{   "s5", 14, -1, JIT_REG_ALPHA_INT | JIT_REG_GLOBAL}, \
	{   "fp", 15, -1, JIT_REG_FIXED | JIT_REG_FRAME}, \
	{   "a0", 16, -1, JIT_REG_FIXED}, \
	{   "a1", 17, -1, JIT_REG_FIXED}, \
	{   "a2", 18, -1, JIT_REG_FIXED}, \
	{   "a3", 19, -1, JIT_REG_FIXED}, \
	{   "a4", 20, -1, JIT_REG_FIXED}, \
	{   "a5", 21, -1, JIT_REG_FIXED}, \
	{   "t8", 22, -1, JIT_REG_ALPHA_INT | JIT_REG_CALL_USED}, \
	{   "t9", 23, -1, JIT_REG_ALPHA_INT | JIT_REG_CALL_USED}, \
	{  "t10", 24, -1, JIT_REG_ALPHA_INT | JIT_REG_CALL_USED}, \
	{  "t11", 25, -1, JIT_REG_ALPHA_INT | JIT_REG_CALL_USED}, \
	{   "ra", 26, -1, JIT_REG_FIXED}, \
	{   "pv", 27, -1, JIT_REG_FIXED}, \
	{   "at", 28, -1, JIT_REG_FIXED}, \
	{   "gp", 29, -1, JIT_REG_FIXED}, \
	{   "sp", 30, -1, JIT_REG_FIXED | JIT_REG_STACK_PTR}, \
	{ "zero", 31, -1, JIT_REG_FIXED}, \
	{  "fv0", 0, -1, JIT_REG_FIXED}, \
	{  "fv1", 1, -1, JIT_REG_FIXED}, \
	{  "fs0", 2, -1, JIT_REG_ALPHA_FLOAT | JIT_REG_GLOBAL}, \
	{  "fs1", 3, -1, JIT_REG_ALPHA_FLOAT | JIT_REG_GLOBAL}, \
	{  "fs2", 4, -1, JIT_REG_ALPHA_FLOAT | JIT_REG_GLOBAL}, \
	{  "fs3", 5, -1, JIT_REG_ALPHA_FLOAT | JIT_REG_GLOBAL}, \
	{  "fs4", 6, -1, JIT_REG_ALPHA_FLOAT | JIT_REG_GLOBAL}, \
	{  "fs5", 7, -1, JIT_REG_ALPHA_FLOAT | JIT_REG_GLOBAL}, \
	{  "fs6", 8, -1, JIT_REG_ALPHA_FLOAT | JIT_REG_GLOBAL}, \
	{  "fs7", 9, -1, JIT_REG_ALPHA_FLOAT | JIT_REG_GLOBAL}, \
	{  "ft0", 10, -1, JIT_REG_ALPHA_FLOAT | JIT_REG_CALL_USED}, \
	{  "ft1", 11, -1, JIT_REG_ALPHA_FLOAT | JIT_REG_CALL_USED}, \
	{  "ft2", 12, -1, JIT_REG_ALPHA_FLOAT | JIT_REG_CALL_USED}, \
	{  "ft3", 13, -1, JIT_REG_ALPHA_FLOAT | JIT_REG_CALL_USED}, \
	{  "ft4", 14, -1, JIT_REG_ALPHA_FLOAT | JIT_REG_CALL_USED}, \
	{  "ft5", 15, -1, JIT_REG_ALPHA_FLOAT | JIT_REG_CALL_USED}, \
	{  "fa0", 16, -1, JIT_REG_FIXED}, \
	{  "fa1", 17, -1, JIT_REG_FIXED}, \
	{  "fa2", 18, -1, JIT_REG_FIXED}, \
	{  "fa3", 19, -1, JIT_REG_FIXED}, \
	{  "fa4", 20, -1, JIT_REG_FIXED}, \
	{  "fa5", 21, -1, JIT_REG_FIXED}, \
	{  "fe0", 22, -1, JIT_REG_FIXED}, \
	{  "fe1", 23, -1, JIT_REG_FIXED}, \
	{  "fe2", 24, -1, JIT_REG_FIXED}, \
	{  "fe3", 25, -1, JIT_REG_FIXED}, \
	{  "fe4", 26, -1, JIT_REG_FIXED}, \
	{  "fe5", 27, -1, JIT_REG_FIXED}, \
	{  "fe6", 28, -1, JIT_REG_FIXED}, \
	{  "fe7", 29, -1, JIT_REG_FIXED}, \
	{  "fe8", 30, -1, JIT_REG_FIXED}, \
	{"fzero", 31, -1, JIT_REG_FIXED},

/* 32 floating-point registers + 32 integer registers */
#define JIT_NUM_REGS			64

/*
 * The number of registers that are used for global register 
 * allocation. Set to zero if global register allocation should not be 
 * used.
 */
#define JIT_NUM_GLOBAL_REGS		14

/*
 * Define to 1 if we should always load values into registers
 * before operating on them.  i.e. the CPU does not have reg-mem
 * and mem-reg addressing modes.
 *
 * The maximum number of operands for an alpha instruction is 3,
 * all of which must be registers.
 */
#define JIT_ALWAYS_REG_REG		1

/*
 * The maximum number of bytes to allocate for the prolog.
 * This may be shortened once we know the true prolog size.
 */
#define JIT_PROLOG_SIZE			(7 /* instructions */ * 4 /* bytes per instruction */)

/*
 * Preferred alignment for the start of functions.
 *
 * Use the alignment that gcc uses. See gcc/config/alpha/alpha.h
 */
#define JIT_FUNCTION_ALIGNMENT		32

/*
 * Define this to 1 if the platform allows reads and writes on
 * any byte boundary.  Define to 0 if only properly-aligned
 * memory accesses are allowed.
 *
 * All memory access on alpha must be naturally aligned. There are 
 * unaligned load and store instructions to operate on arbitrary byte 
 * boundaries. However sometimes compilers don't always spot where 
 * to use them due to programming tricks with pointers. The kernel will 
 * do the fetch transparently if the access is unaligned and not done 
 * with the proper instructions. Kernel assisted unaligned accesses 
 * don't change the behavior of the program. 
 *
 * TODO: benchmark this to determine what is more costly... setting
 * up everything to be aligned or dealing with the unaligned accesses.
 */
#define JIT_ALIGN_OVERRIDES		1

/*
 * The jit_extra_gen_state macro can be supplied to add extra fields to 
 * the struct jit_gencode type in jit-rules.h, for extra CPU-specific 
 * code generation state information.
 */
#define jit_extra_gen_state		/* empty */;

/*
 * The jit_extra_gen_init macro initializes this extra information, and 
 * the jit_extra_gen_cleanup macro cleans it up when code generation is 
 * complete.
 */
#define jit_extra_gen_init(gen)		do { ; } while (0)
#define jit_extra_gen_cleanup(gen)	do { ; } while (0)

/*
 * Parameter passing rules.
 */
#define JIT_CDECL_WORD_REG_PARAMS	{16,17,18,19,20,21,-1}
#define JIT_MAX_WORD_REG_PARAMS		6
#define JIT_INITIAL_STACK_OFFSET	(14*8)
#define JIT_INITIAL_FRAME_SIZE 		(sizeof(void*))

#ifdef	__cplusplus
};
#endif

#endif	/* _JIT_RULES_ALPHA_H */
