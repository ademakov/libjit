/*
 * jit-gen-alpha.h - Code generation macros for the alpha processor.
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

#ifndef	_JIT_GEN_ALPHA_H
#define	_JIT_GEN_ALPHA_H

#include "jit-rules.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  enumerate the 32 integer registers.
 *
 *  See JIT_REG_INFO in jit-rules-alpha.h for details about registers
 */
typedef enum {
	ALPHA_R0 = 0,	ALPHA_V0 = ALPHA_R0,	/* Function result */

	ALPHA_R1 = 1,	ALPHA_T0 = ALPHA_R1,	/* Temp registers */
	ALPHA_R2 = 2,	ALPHA_T1 = ALPHA_R2,
	ALPHA_R3 = 3,	ALPHA_T2 = ALPHA_R3,
	ALPHA_R4 = 4,	ALPHA_T3 = ALPHA_R4,
	ALPHA_R5 = 5,	ALPHA_T4 = ALPHA_R5,
	ALPHA_R6 = 6,	ALPHA_T5 = ALPHA_R6,
	ALPHA_R7 = 7,	ALPHA_T6 = ALPHA_R7,
	ALPHA_R8 = 8,	ALPHA_T7 = ALPHA_R8,

	ALPHA_R9 = 9,	ALPHA_S0 = ALPHA_R9,	/* Saved registers */
	ALPHA_R10 = 10,	ALPHA_S1 = ALPHA_R10,
	ALPHA_R11 = 11,	ALPHA_S2 = ALPHA_R11,
	ALPHA_R12 = 12,	ALPHA_S3 = ALPHA_R12,
	ALPHA_R13 = 13,	ALPHA_S4 = ALPHA_R13,
	ALPHA_R14 = 14,	ALPHA_S5 = ALPHA_R14,

	ALPHA_R15 = 15,	ALPHA_S6 = ALPHA_R15,   /* ALPHA_R15 can hold either a saved value */
			ALPHA_FP = ALPHA_R15,   /* or the frame pointer                    */

	ALPHA_R16 = 16, ALPHA_A0 = ALPHA_R16,	/* First 6 arguments */
	ALPHA_R17 = 17,	ALPHA_A1 = ALPHA_R17,
	ALPHA_R18 = 18,	ALPHA_A2 = ALPHA_R18,
	ALPHA_R19 = 19,	ALPHA_A3 = ALPHA_R19,
	ALPHA_R20 = 20,	ALPHA_A4 = ALPHA_R20,
	ALPHA_R21 = 21,	ALPHA_A5 = ALPHA_R21,

	ALPHA_R22 = 22,	ALPHA_T8  = ALPHA_R22,	/* More temp registers */
	ALPHA_R23 = 23,	ALPHA_T9  = ALPHA_R23,
	ALPHA_R24 = 24,	ALPHA_T10 = ALPHA_R24,
	ALPHA_R25 = 25,	ALPHA_T11 = ALPHA_R25,

	ALPHA_R26 = 26,	ALPHA_RA = ALPHA_R26,	/* Return address */

	ALPHA_R27 = 27,	ALPHA_T12 = ALPHA_R27,	/* ALPHA_R27 can hold either a temp value */
			ALPHA_PV  = ALPHA_R27,  /* or the procedure value                 */

	ALPHA_R28 = 28, ALPHA_AT = ALPHA_R28,	/* Reeserved for the assembler */

	ALPHA_R29 = 29,	ALPHA_GP = ALPHA_R29,	/* Global pointer */

	ALPHA_R30 = 30,	ALPHA_SP = ALPHA_R30,	/* Stack pointer */

	ALPHA_R31 = 31,	ALPHA_ZERO = ALPHA_R31	/* Contains the value 0 */
} ALPHA_REG;

/*
 *  enumerate the 32 floating-point registers.
 *
 *  See JIT_REG_INFO in jit-rules-alpha.h for details about registers
 */
typedef enum {
						/* Function result */
	ALPHA_F0 = 0,	ALPHA_FV0 = ALPHA_F0,	/* real part       */
	ALPHA_F1 = 1,	ALPHA_FV1 = ALPHA_F1,	/* complex part    */

	ALPHA_F2 = 2,	ALPHA_FS0 = ALPHA_F2,	/* Saved registers */
	ALPHA_F3 = 3,	ALPHA_FS1 = ALPHA_F3,
	ALPHA_F4 = 4,	ALPHA_FS2 = ALPHA_F4,
	ALPHA_F5 = 5,	ALPHA_FS3 = ALPHA_F5,
	ALPHA_F6 = 6,	ALPHA_FS4 = ALPHA_F6,
	ALPHA_F7 = 7,	ALPHA_FS5 = ALPHA_F7,
	ALPHA_F8 = 8,	ALPHA_FS6 = ALPHA_F8,
	ALPHA_F9 = 9,	ALPHA_FS7 = ALPHA_F9,

	ALPHA_F10 = 10,	ALPHA_FT0 = ALPHA_F10,	/* Temp registers */
	ALPHA_F11 = 11,	ALPHA_FT1 = ALPHA_F11,
	ALPHA_F12 = 12,	ALPHA_FT2 = ALPHA_F12,
	ALPHA_F13 = 13,	ALPHA_FT3 = ALPHA_F13,
	ALPHA_F14 = 14,	ALPHA_FT4 = ALPHA_F14,
	ALPHA_F15 = 15,	ALPHA_FT5 = ALPHA_F15,

	ALPHA_F16 = 16,	ALPHA_FA0 = ALPHA_F16,	/* First 6 arguments */
	ALPHA_F17 = 17,	ALPHA_FA1 = ALPHA_F17,
	ALPHA_F18 = 18,	ALPHA_FA2 = ALPHA_F18,
	ALPHA_F19 = 19,	ALPHA_FA3 = ALPHA_F19,
	ALPHA_F20 = 20,	ALPHA_FA4 = ALPHA_F20,
	ALPHA_F21 = 21,	ALPHA_FA5 = ALPHA_F21,

	ALPHA_F22 = 22,	ALPHA_FE0 = ALPHA_F22,	/* Temp registers for expression evaluation. */
	ALPHA_F23 = 23,	ALPHA_FE1 = ALPHA_F23,
	ALPHA_F24 = 24,	ALPHA_FE2 = ALPHA_F24,
	ALPHA_F25 = 25,	ALPHA_FE3 = ALPHA_F25,
	ALPHA_F26 = 26,	ALPHA_FE4 = ALPHA_F26,
	ALPHA_F27 = 27,	ALPHA_FE5 = ALPHA_F27,
	ALPHA_F28 = 28,	ALPHA_FE6 = ALPHA_F28,
	ALPHA_F29 = 29,	ALPHA_FE7 = ALPHA_F29,
	ALPHA_F30 = 30,	ALPHA_FE8 = ALPHA_F30,

	ALPHA_F31 = 31,	ALPHA_FZERO = ALPHA_F31	/*Contains the value 0.0 */
} ALPHA_FREG;

/*
 * Number of registers that are used for parameters.
 *
 * 6 registers a0-a5 (i.e. $16-$21) to pass integer parameters
 * 6 registers fa0-fa5 (i.e. $f16-$f21) to pass floating-point parameters
 * Additional parameters are stored on the stack.
 */
#define ALPHA_NUM_PARAM_REGS      6

/*
 * each alpha machine code instruction is 32-bits long.
 */

typedef unsigned int * alpha_inst;

/* misc function prototypes */
void alpha_output_branch(jit_function_t, alpha_inst, int, jit_insn_t, int);
void jump_to_epilog(jit_gencode_t, alpha_inst, jit_block_t);

#define ALPHA_REG_MASK		0x1f
#define ALPHA_REGA_SHIFT	0x15
#define ALPHA_REGB_SHIFT	0x10
#define ALPHA_REGC_SHIFT	0x00

#define ALPHA_OP_MASK		0x3f
#define ALPHA_OP_SHIFT		0x1a

#define ALPHA_LIT_MASK		0x7f
#define ALPHA_LIT_SHIFT		0x0d

#define ALPHA_FUNC_MASK		0x7f
#define ALPHA_FP_FUNC_MASK	0x7ff
#define ALPHA_FUNC_SHIFT	0x5

#define ALPHA_FUNC_MEM_BRANCH_MASK	0x3
#define ALPHA_FUNC_MEM_BRANCH_SHIFT	0xe
#define ALPHA_HINT_MASK			0x3fff

#define ALPHA_OFFSET_MASK		0xffff
#define ALPHA_BRANCH_OFFSET_MASK	0x1fffff

/*
 * Define opcodes
 */

#define ALPHA_OP_LDA		0x08
#define ALPHA_OP_LDAH		0x09
#define ALPHA_OP_LDBU		0x0a
#define ALPHA_OP_LDQ_U		0x0b
#define ALPHA_OP_LDWU		0x0c
#define ALPHA_OP_STW		0x0d
#define ALPHA_OP_STB		0x0e
#define ALPHA_OP_STQ_U		0x0f
#define ALPHA_OP_ADDL		0x10
#define ALPHA_OP_S4ADDL		0x10
#define ALPHA_OP_SUBL		0x10
#define ALPHA_OP_S4SUBL		0x10
#define ALPHA_OP_CMPBGE		0x10
#define ALPHA_OP_S8ADDL		0x10
#define ALPHA_OP_S8SUBL		0x10
#define ALPHA_OP_CMPLT		0x10
#define ALPHA_OP_CMPULT		0x10
#define ALPHA_OP_ADDQ		0x10
#define ALPHA_OP_S4ADDQ		0x10
#define ALPHA_OP_SUBQ		0x10
#define ALPHA_OP_S4SUBQ		0x10
#define ALPHA_OP_CMPEQ		0x10
#define ALPHA_OP_S8ADDQ		0x10
#define ALPHA_OP_S8SUBQ		0x10
#define ALPHA_OP_CMPULE		0x10
#define ALPHA_OP_ADDLV		0x10
#define ALPHA_OP_SUBLV		0x10
#define ALPHA_OP_ADDQV		0x10
#define ALPHA_OP_SUBQV		0x10
#define ALPHA_OP_CMPLE		0x10
#define ALPHA_OP_AND		0x11
#define ALPHA_OP_BIC		0x11
#define ALPHA_OP_CMOVLBS	0x11
#define ALPHA_OP_CMOVLBC	0x11
#define ALPHA_OP_NOP		0x11
#define ALPHA_OP_CLR		0x11
#define ALPHA_OP_MOV		0x11
#define ALPHA_OP_OR		0x11
#define ALPHA_OP_BIS		0x11
#define ALPHA_OP_CMOVEQ		0x11
#define ALPHA_OP_CMOVNE		0x11
#define ALPHA_OP_NOT		0x11
#define ALPHA_OP_ORNOT		0x11
#define ALPHA_OP_XOR		0x11
#define ALPHA_OP_CMOVLT		0x11
#define ALPHA_OP_CMOVGE		0x11
#define ALPHA_OP_EQV		0x11
#define ALPHA_OP_XORNOT		0x11
#define ALPHA_OP_AMASK		0x11
#define ALPHA_OP_CMOVLE		0x11
#define ALPHA_OP_CMOVLE		0x11
#define ALPHA_OP_CMOVGT		0x11
#define ALPHA_OP_IMPLVER	0x11
#define ALPHA_OP_MSKBL		0x12
#define ALPHA_OP_EXTBL		0x12
#define ALPHA_OP_INSBL		0x12
#define ALPHA_OP_MSKWL		0x12
#define ALPHA_OP_EXTWL		0x12
#define ALPHA_OP_INSWL		0x12
#define ALPHA_OP_MSKLL		0x12
#define ALPHA_OP_EXTLL		0x12
#define ALPHA_OP_INSLL		0x12
#define ALPHA_OP_ZAP		0x12
#define ALPHA_OP_ZAPNOT		0x12
#define ALPHA_OP_MSKQL		0x12
#define ALPHA_OP_SRL		0x12
#define ALPHA_OP_EXTQA		0x12
#define ALPHA_OP_SLL		0x12
#define ALPHA_OP_INSQL		0x12
#define ALPHA_OP_SRA		0x12
#define ALPHA_OP_MSKWH		0x12
#define ALPHA_OP_INSWH		0x12
#define ALPHA_OP_EXTWH		0x12
#define ALPHA_OP_MSKLH		0x12
#define ALPHA_OP_INSLH		0x12
#define ALPHA_OP_EXTLH		0x12
#define ALPHA_OP_MSKQH		0x12
#define ALPHA_OP_INSQH		0x12
#define ALPHA_OP_EXTQH		0x12
#define ALPHA_OP_MULL		0x13
#define ALPHA_OP_MULQ		0x13
#define ALPHA_OP_UMULH		0x13
#define ALPHA_OP_MULLV		0x13
#define ALPHA_OP_MULLQV		0x13
#define ALPHA_OP_ITOFS		0x14
#define ALPHA_OP_ITOFF		0x14
#define ALPHA_OP_ITOFT		0x14
#define ALPHA_OP_ADDS		0x16
#define ALPHA_OP_SUBS		0x16
#define ALPHA_OP_ADDT		0x16
#define ALPHA_OP_SUBT		0x16
#define ALPHA_OP_MULT		0x16
#define ALPHA_OP_DIVT		0x16
#define ALPHA_OP_CPYS		0x17
#define ALPHA_OP_CPYSN		0x17
#define ALPHA_OP_CPYSE		0x17
#define ALPHA_OP_TRAPB		0x18
#define ALPHA_OP_JMP		0x1a
#define ALPHA_OP_JSR		0x1a
#define ALPHA_OP_RET		0x1a
#define ALPHA_OP_JSRCO		0x1a
#define ALPHA_OP_FTOIT		0x1c
#define ALPHA_OP_FTOIS		0x1c
#define ALPHA_OP_LDF		0x20
#define ALPHA_OP_LDG		0x21
#define ALPHA_OP_LDS		0x22
#define ALPHA_OP_LDT		0x23
#define ALPHA_OP_LDQF		0x23
#define ALPHA_OP_STF		0x24
#define ALPHA_OP_STG		0x25
#define ALPHA_OP_STS		0x26
#define ALPHA_OP_STT		0x27
#define ALPHA_OP_LDL		0x28
#define ALPHA_OP_LDQ		0x29
#define ALPHA_OP_LDL_L		0x2a
#define ALPHA_OP_LDQ_L		0x2b
#define ALPHA_OP_STL		0x2c
#define ALPHA_OP_STQ		0x2d
#define ALPHA_OP_STL_C		0x2e
#define ALPHA_OP_STQ_C		0x2f
#define ALPHA_OP_BR		0x30
#define ALPHA_OP_FBEQ		0x31
#define ALPHA_OP_FBLT		0x32
#define ALPHA_OP_FBLE		0x33
#define ALPHA_OP_BSR		0x34
#define ALPHA_OP_FBNE		0x35
#define ALPHA_OP_FBGE		0x36
#define ALPHA_OP_FBGT		0x37
#define ALPHA_OP_BLBC		0x38
#define ALPHA_OP_BEQ		0x39
#define ALPHA_OP_BLT		0x3a
#define ALPHA_OP_BLE		0x3b
#define ALPHA_OP_BLBS		0x3c
#define ALPHA_OP_BNE		0x3d
#define ALPHA_OP_BGE		0x3e
#define ALPHA_OP_BGT		0x3f

/*
 * Define functions
 */

/* register operations -- use with ALPHA_OP_* == 0x10 */
#define ALPHA_FUNC_ADDL		0x00
#define ALPHA_FUNC_S4ADDL	0x02
#define ALPHA_FUNC_SUBL		0x09
#define ALPHA_FUNC_S4SUBL	0x0b
#define ALPHA_FUNC_CMPBGE	0x0f
#define ALPHA_FUNC_S8ADDL	0x12
#define ALPHA_FUNC_S8SUBL	0x1b
#define ALPHA_FUNC_CMPULT	0x1d
#define ALPHA_FUNC_ADDQ		0x20
#define ALPHA_FUNC_S4ADDQ	0x22
#define ALPHA_FUNC_SUBQ		0x29
#define ALPHA_FUNC_S4SUBQ	0x2b
#define ALPHA_FUNC_CMPEQ	0x2d
#define ALPHA_FUNC_S9ADDQ	0x32
#define ALPHA_FUNC_S9SUBQ	0x3b
#define ALPHA_FUNC_CMPULE	0x3d
#define ALPHA_FUNC_ADDLV	0x40
#define ALPHA_FUNC_SUBLV	0x49
#define ALPHA_FUNC_CMPLT	0x4d
#define ALPHA_FUNC_ADDQV	0x60
#define ALPHA_FUNC_SUBQV	0x69
#define ALPHA_FUNC_CMPLE	0x6d

/* bitwise operations -- use with ALPHA_OP_* == 0x11 */
#define ALPHA_FUNC_AND		0x00
#define ALPHA_FUNC_BIC		0x08
#define ALPHA_FUNC_CMOVLBS	0x14
#define ALPHA_FUNC_CMOVLBC	0x16
#define ALPHA_FUNC_NOOP		0x20
#define ALPHA_FUNC_CLR		0x20
#define ALPHA_FUNC_MOV		0x20
#define ALPHA_FUNC_OR		0x20
#define ALPHA_FUNC_CMOVEQ	0x24
#define ALPHA_FUNC_CMOVNE	0x2C
#define ALPHA_FUNC_NOT		0x28
#define ALPHA_FUNC_ORNOT	0x28
#define ALPHA_FUNC_XOR		0x40
#define ALPHA_FUNC_CMOVLT	0x44
#define ALPHA_FUNC_COMVGE	0x46
#define ALPHA_FUNC_EQV		0x48
#define ALPHA_FUNC_AMASK	0x61
#define ALPHA_FUNC_CMOVLE	0x64
#define ALPHA_FUNC_CMOVGT	0x66
#define ALPHA_FUNC_IMPLVER	0x6c
#define ALPHA_FUNC_CMOVGT	0x66

/* byte manipulation operations -- use with ALPHA_OP_* == 0x12 */
#define ALPHA_FUNC_MSKBL	0x02
#define ALPHA_FUNC_EXTBL	0x06
#define ALPHA_FUNC_INSBL	0x0b
#define ALPHA_FUNC_MSKWL	0x12
#define ALPHA_FUNC_EXTWL	0x16
#define ALPHA_FUNC_INSWL	0x1b
#define ALPHA_FUNC_MSKLL	0x22
#define ALPHA_FUNC_EXTLL	0x26
#define ALPHA_FUNC_INSLL	0x2b
#define ALPHA_FUNC_ZAP		0x30
#define ALPHA_FUNC_ZAPNOT	0x31
#define ALPHA_FUNC_MSKQL	0x32
#define ALPHA_FUNC_SRL		0x34
#define ALPHA_FUNC_EXTQL	0x36
#define ALPHA_FUNC_SLL		0x39
#define ALPHA_FUNC_INSQL	0x3b
#define ALPHA_FUNC_SRA		0x3c
#define ALPHA_FUNC_MSKWH	0x52
#define ALPHA_FUNC_INSWH	0x57
#define ALPHA_FUNC_EXTWH	0x5a
#define ALPHA_FUNC_MSKLH	0x62
#define ALPHA_FUNC_INSLH	0x67
#define ALPHA_FUNC_EXTLH	0x6a
#define ALPHA_FUNC_MSKQH	0x72
#define ALPHA_FUNC_INSQH	0x77
#define ALPHA_FUNC_EXTQH	0x7a

/* multiplication operations -- use with ALPHA_OP_* == 0x13 */
#define ALPHA_FUNC_MULL		0x00
#define ALPHA_FUNC_MULQ		0x20
#define ALPHA_FUNC_UMULH	0x30
#define ALPHA_FUNC_MULLV	0x40
#define ALPHA_FUNC_MULQV	0x60

/* integer to floating point operations -- use with ALPHA_OP_* == 0x14 */
#define ALPHA_FUNC_ITOFS	0x4
#define ALPHA_FUNC_ITOFF	0x14
#define ALPHA_FUNC_ITOFT	0x24

/* floating point arithmetic operations -- use with ALPHA_OP_* == 0x16 */
#define ALPHA_FUNC_ADDS		0x80
#define ALPHA_FUNC_SUBS		0x81
#define ALPHA_FUNC_ADDT		0xA0
#define ALPHA_FUNC_SUBT		0xA1
#define ALPHA_FUNC_MULT		0xA2
#define ALPHA_FUNC_DIVT		0xA3

/* floating point sign copy operations -- use with ALPHA_OP_* == 0x17 */
#define ALPHA_FUNC_CPYS		0x20
#define ALPHA_FUNC_CPYSN	0x21
#define ALPHA_FUNC_CPYSE	0x22

/* trap barrier -- use with ALPHA_OP_* == 0x18 */
#define ALPHA_FUNC_TRAPB	0x0

/* branching operations -- use with ALPHA_OP_* == 0x1a */
#define ALPHA_FUNC_JMP		0x0
#define ALPHA_FUNC_JSR		0x1
#define ALPHA_FUNC_RET		0x2
#define ALPHA_FUNC_JSRCO	0x3

/* floating point to integer operations -- use with ALPHA_OP_* == 0x1c */
#define ALPHA_FUNC_FTOIT	0x70
#define ALPHA_FUNC_FTOIS	0x78

/* encode registers */
#define alpha_encode_reg_a(reg) \
	((reg & ALPHA_REG_MASK) << ALPHA_REGA_SHIFT)

#define alpha_encode_reg_b(reg) \
	((reg & ALPHA_REG_MASK) << ALPHA_REGB_SHIFT)

#define alpha_encode_reg_c(reg) \
	((reg & ALPHA_REG_MASK) << ALPHA_REGC_SHIFT)

/* encode literals */
#define alpha_encode_lit(lit) \
	((lit & ALPHA_LIT_MASK) << ALPHA_LIT_SHIFT)

/* encode opcodes */
#define alpha_encode_op(op) \
	((op & ALPHA_OP_MASK) << ALPHA_OP_SHIFT)

/* encode function codes */
#define alpha_encode_func(func) \
	((func & ALPHA_FUNC_MASK) << ALPHA_FUNC_SHIFT)

#define alpha_encode_fp_func(func) \
	((func & ALPHA_FP_FUNC_MASK) << ALPHA_FUNC_SHIFT)

#define alpha_encode_func_mem_branch(func,hint) \
	(((func & ALPHA_FUNC_MEM_BRANCH_MASK) << ALPHA_FUNC_MEM_BRANCH_SHIFT) | \
		(hint & ALPHA_HINT_MASK))

/*
 * instruction encoding
 */

#define alpha_encode_mem(inst,op,dreg,sreg,offset) \
	*(inst)++ = (alpha_encode_op(op) | alpha_encode_reg_a(dreg)  | \
		alpha_encode_reg_b(sreg) | (offset & ALPHA_OFFSET_MASK))

#define alpha_encode_regops(inst,op,func,sreg0,sreg1,dreg) \
	*(inst)++ = (alpha_encode_op(op) | alpha_encode_reg_a(sreg0) | \
		alpha_encode_reg_b(sreg1)| alpha_encode_reg_c(dreg)  | \
		alpha_encode_func(func))

#define alpha_encode_fpops(inst,op,func,sreg0,sreg1,dreg) \
	*(inst)++ = (alpha_encode_op(op) | alpha_encode_reg_a(sreg0) | \
		alpha_encode_reg_b(sreg1)| alpha_encode_reg_c(dreg)  | \
		alpha_encode_fp_func(func))

#define alpha_encode_mem_branch(inst,op,func,dreg,sreg,hint) \
	*(inst)++ = (alpha_encode_op(op) | alpha_encode_reg_a(dreg) | \
		alpha_encode_reg_b(sreg) | \
		alpha_encode_func_mem_branch(func,hint))

#define alpha_encode_branch(inst,op,reg,offset) \
	*(inst)++ = (alpha_encode_op(op) | alpha_encode_reg_a(reg) | \
		(offset & ALPHA_BRANCH_OFFSET_MASK))

#define alpha_encode_regops_lit(inst,op,func,sreg,lit,dreg) \
	*(inst)++ = (alpha_encode_op(op) | alpha_encode_reg_a(sreg) | \
		alpha_encode_lit(lit)    | alpha_encode_reg_c(dreg)  | \
		alpha_encode_func(func)  | 0x1000)

/*
 * define pneumonics
 *
 * A note about the naming convention...
 *
 * Some pneumonics can be used to operate on literals (aka immediate 
 * values) and registers. For example, the operands for 'addl' can be
 * "sreg1, sreg2, dreg" or "sreg, lit, dreg". Since the opcodes are the 
 * same and the encoding of the literal is different than the encoding
 * of the register, we need more than one alpha_addl macro.
 *
 * The naming convention I picked is: 'alpha_addl' will operate on 
 * registers and 'alpha_addli' will operate on registers and a literal. 
 * The 'i' suffix stands for immediate. It is the convention used in 
 * MIPS assembly. So when you want to use one of these macros for an
 * immediate value, just use the pneumonic with the 'i' suffix.
 *
 * TODO: implement the missing macros!
 *
 * floating-point arithmetic, PAL (Privileged Architecture Library), and
 * some special instructions (like those used for prefetching data and halting the system)
 * have not been implmented yet.
 */

/* load and store operations */
#define alpha_lda(inst,dreg,sreg,offset)	alpha_encode_mem(inst,ALPHA_OP_LDA,dreg,sreg,offset)
#define alpha_ldah(inst,dreg,sreg,offset)	alpha_encode_mem(inst,ALPHA_OP_LDAH,dreg,sreg,offset)
#define alpha_ldbu(inst,dreg,sreg,offset)	alpha_encode_mem(inst,ALPHA_OP_LDBU,dreg,sreg,offset)
#define alpha_ldq_u(inst,dreg,sreg,offset)	alpha_encode_mem(inst,ALPHA_OP_LDQ_U,dreg,sreg,offset)
#define alpha_ldwu(inst,dreg,sreg,offset)	alpha_encode_mem(inst,ALPHA_OP_LDWU,dreg,sreg,offset)
#define alpha_stw(inst,dreg,sreg,offset)	alpha_encode_mem(inst,ALPHA_OP_STW,dreg,sreg,offset)
#define alpha_stb(inst,dreg,sreg,offset)	alpha_encode_mem(inst,ALPHA_OP_STB,dreg,sreg,offset)
#define alpha_stq_u(inst,dreg,sreg,offset)	alpha_encode_mem(inst,ALPHA_OP_STQ_U,dreg,sreg,offset)

/* arithmetic operations */
#define alpha_addl(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_ADDL,ALPHA_FUNC_ADDL,sreg0,sreg1,dreg)
#define alpha_s4addl(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_S4ADDL,ALPHA_FUNC_S4ADDL,sreg0,sreg1,dreg)
#define alpha_subl(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_SUBL,ALPHA_FUNC_SUBL,sreg0,sreg1,dreg)
#define alpha_s4subl(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_S4SUBL,ALPHA_FUNC_S4SUBL,sreg0,sreg1,dreg)
#define alpha_cmpbge(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_CMPBGE,ALPHA_FUNC_CMPBGE,sreg0,sreg1,dreg)
#define alpha_s8addl(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_S8ADDL,ALPHA_FUNC_S8ADDL,sreg0,sreg1,dreg)
#define alpha_s8subl(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_S8SUBL,ALPHA_FUNC_S8SUBL,sreg0,sreg1,dreg)
#define alpha_cmpult(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_CMPULT,ALPHA_FUNC_CMPULT,sreg0,sreg1,dreg)
#define alpha_addq(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_ADDQ,ALPHA_FUNC_ADDQ,sreg0,sreg1,dreg)
#define alpha_s4addq(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_S4ADDQ,ALPHA_FUNC_S4ADDQ,sreg0,sreg1,dreg)
#define alpha_subq(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_SUBQ,ALPHA_FUNC_SUBQ,sreg0,sreg1,dreg)
#define alpha_s4subq(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_S4SUBQ,ALPHA_FUNC_S4SUBQ,sreg0,sreg1,dreg)
#define alpha_cmpeq(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_CMPEQ,ALPHA_FUNC_CMPEQ,sreg0,sreg1,dreg)
#define alpha_s8addq(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_S8ADDQ,ALPHA_FUNC_S8ADDQ,sreg0,sreg1,dreg)
#define alpha_s8subq(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_S8SUBQ,ALPHA_FUNC_S8SUBQ,sreg0,sreg1,dreg)
#define alpha_cmpule(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_CMPULE,ALPHA_FUNC_CMPULE,sreg0,sreg1,dreg)
#define alpha_addlv(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_ADDLV,ALPHA_FUNC_ADDLV,sreg0,sreg1,dreg)
#define alpha_sublv(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_SUBLV,ALPHA_FUNC_SUBLV,sreg0,sreg1,dreg)
#define alpha_cmplt(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_CMPLT,ALPHA_FUNC_CMPLT,sreg0,sreg1,dreg)
#define alpha_addqv(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_ADDQV,ALPHA_FUNC_ADDQV,sreg0,sreg1,dreg)
#define alpha_subqv(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_SUBQV,ALPHA_FUNC_SUBQV,sreg0,sreg1,dreg)
#define alpha_cmple(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_CMPLE,ALPHA_FUNC_CMPLE,sreg0,sreg1,dreg)

/*
 * pseudo comparison operations
 *
 * Alpha doesn't have all possible comparison opcodes. For example, we
 * have cmple for A <= B, but no cmpge for A >= B. So we make cmpge by 
 * using cmple with the operands switched. Example, A >= B becomes B <= A
 */
#define alpha_cmpble(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_CMPBGE,ALPHA_FUNC_CMPBGE,sreg1,sreg0,dreg)
#define alpha_cmpugt(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_CMPULT,ALPHA_FUNC_CMPULT,sreg1,sreg0,dreg)
#define alpha_cmpuge(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_CMPULE,ALPHA_FUNC_CMPULE,sreg1,sreg0,dreg)
#define alpha_cmpgt(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_CMPLT,ALPHA_FUNC_CMPLT,sreg1,sreg0,dreg)
#define alpha_cmpge(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_CMPLE,ALPHA_FUNC_CMPLE,sreg1,sreg0,dreg)

/* bitwise / move operations */
#define alpha_and(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_AND,ALPHA_FUNC_AND,sreg0,sreg1,dreg)
#define alpha_bic(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_BIC,ALPHA_FUNC_BIC,sreg0,sreg1,dreg)
#define alpha_cmovlbs(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_CMOVLBS,ALPHA_FUNC_CMOVLBS,sreg0,sreg1,dreg)
#define alpha_cmovlbc(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_CMOVLBC,ALPHA_FUNC_CMOVLBC,sreg0,sreg1,dreg)
#define alpha_bis(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_BIS,ALPHA_FUNC_BIS,sreg0,sreg1,dreg)
#define alpha_cmoveq(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_CMOVEQ,ALPHA_FUNC_CMOVEQ,sreg0,sreg1,dreg)
#define alpha_cmovne(inst,sreg,dreg)		alpha_encode_regops(inst,ALPHA_OP_CMOVNE,ALPHA_FUNC_CMOVNE,ALPHA_ZERO,sreg,dreg)
#define alpha_ornot(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_ORNOT,ALPHA_FUNC_ORNOT,sreg0,sreg1,dreg)
#define alpha_xor(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_XOR,ALPHA_FUNC_XOR,sreg0,sreg1,dreg)
#define alpha_cmovlt(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_CMOVLT,ALPHA_FUNC_CMOVLT,sreg0,sreg1,dreg)
#define alpha_cmovge(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_CMOVGE,ALPHA_FUNC_CMOVGE,sreg0,sreg1,dreg)
#define alpha_eqv(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_EQV,ALPHA_FUNC_EQV,sreg0,sreg1,dreg)
#define alpha_amask(inst,sreg,dreg)		alpha_encode_regops(inst,ALPHA_OP_AMASK,ALPHA_FUNC_AMASK,ALPHA_ZERO,sreg,dreg)
#define alpha_cmovle(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_CMOVLE,ALPHA_FUNC_CMOVLE,sreg0,sreg1,dreg)
#define alpha_cmovgt(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_CMOVGT,ALPHA_FUNC_CMOVGT,sreg0,sreg1,dreg)
#define alpha_implver(inst,dreg)		alpha_encode_regops_lit(inst,ALPHA_OP_IMPLVER,ALPHA_FUNC_IMPLVER,ALPHA_ZERO,1,dreg)

/* pseudo bitwise / move instructions */
#define alpha_mov(inst,sreg,dreg)		alpha_encode_regops(inst,ALPHA_OP_MOV,ALPHA_FUNC_MOV,ALPHA_ZERO,sreg,dreg)
#define alpha_nop(inst)				alpha_mov(inst,ALPHA_ZERO,ALPHA_ZERO)
#define alpha_unop(inst)			alpha_nop(inst)
#define alpha_clr(inst,dreg)			alpha_mov(inst,ALPHA_ZERO,dreg)
#define alpha_or(inst,sreg0,sreg1,dreg)		alpha_encode_regops(inst,ALPHA_OP_OR,ALPHA_FUNC_OR,sreg0,sreg1,dreg)
#define alpha_ori(inst,sreg,lit,dreg)		alpha_encode_regops_lit(inst,ALPHA_OP_OR,ALPHA_FUNC_OR,sreg,lit,dreg)

/* byte manipulation instructions */
#define alpha_mskbl(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_MSKBL,ALPHA_FUNC_MSKBL,sreg0,sreg1,dreg)
#define alpha_extbl(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_EXTBL,ALPHA_FUNC_EXTBL,sreg0,sreg1,dreg)
#define alpha_insbl(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_INSBL,ALPHA_FUNC_INSBL,sreg0,sreg1,dreg)
#define alpha_mskwl(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_MSKWL,ALPHA_FUNC_MSKWL,sreg0,sreg1,dreg)
#define alpha_extwl(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_EXTWL,ALPHA_FUNC_EXTWL,sreg0,sreg1,dreg)
#define alpha_inswl(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_INSWL,ALPHA_FUNC_INSWL,sreg0,sreg1,dreg)
#define alpha_mskll(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_MSKLL,ALPHA_FUNC_MSKLL,sreg0,sreg1,dreg)
#define alpha_extll(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_EXTLL,ALPHA_FUNC_EXTLL,sreg0,sreg1,dreg)
#define alpha_insll(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_INSLL,ALPHA_FUNC_INSLL,sreg0,sreg1,dreg)
#define alpha_zap(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_ZAP,ALPHA_FUNC_ZAP,sreg0,sreg1,dreg)
#define alpha_zapnot(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_ZAPNOT,ALPHA_FUNC_ZAPNOT,sreg0,sreg1,dreg)
#define alpha_mskql(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_MSKQL,ALPHA_FUNC_MSKQL,sreg0,sreg1,dreg)
#define alpha_srl(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_SRL,ALPHA_FUNC_SRL,sreg0,sreg1,dreg)
#define alpha_srli(inst,sreg,lit,dreg)		alpha_encode_regops_lit(inst,ALPHA_OP_SRL,ALPHA_FUNC_SRL,sreg,lit,dreg)
#define alpha_extql(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_EXTQL,ALPHA_FUNC_EXTQL,sreg0,sreg1,dreg)
#define alpha_sll(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_SLL,ALPHA_FUNC_SLL,sreg0,sreg1,dreg)
#define alpha_slli(inst,sreg,lit,dreg)		alpha_encode_regops_lit(inst,ALPHA_OP_SLL,ALPHA_FUNC_SLL,sreg,lit,dreg)
#define alpha_insql(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_INSQL,ALPHA_FUNC_INSQL,sreg0,sreg1,dreg)
#define alpha_sra(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_SRA,ALPHA_FUNC_SRA,sreg0,sreg1,dreg)
#define alpha_srai(inst,sreg,lit,dreg)		alpha_encode_regops_lit(inst,ALPHA_OP_SRA,ALPHA_FUNC_SRA,sreg,lit,dreg)
#define alpha_mskwh(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_MSKWH,ALPHA_FUNC_MSKWH,sreg0,sreg1,dreg)
#define alpha_inswh(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_INSWH,ALPHA_FUNC_INSWH,sreg0,sreg1,dreg)
#define alpha_extwh(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_EXTWH,ALPHA_FUNC_EXTWH,sreg0,sreg1,dreg)
#define alpha_msklh(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_MSKLH,ALPHA_FUNC_MSKLH,sreg0,sreg1,dreg)
#define alpha_inslh(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_INSLH,ALPHA_FUNC_INSLH,sreg0,sreg1,dreg)
#define alpha_extlh(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_EXTLH,ALPHA_FUNC_EXTLH,sreg0,sreg1,dreg)
#define alpha_mskqh(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_MSKQH,ALPHA_FUNC_MSKQH,sreg0,sreg1,dreg)
#define alpha_insqh(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_INSQH,ALPHA_FUNC_INSQH,sreg0,sreg1,dreg)
#define alpha_extqh(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_EXTQH,ALPHA_FUNC_EXTQH,sreg0,sreg1,dreg)

/* multiplication operations */
#define alpha_mull(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_MULL,ALPHA_FUNC_MULL,sreg0,sreg1,dreg)
#define alpha_mulq(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_MULQ,ALPHA_FUNC_MULQ,sreg0,sreg1,dreg)
#define alpha_umulh(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_UMULH,ALPHA_FUNC_UMULH,sreg0,sreg1,dreg)
#define alpha_mullv(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_MULLV,ALPHA_FUNC_MULLV,sreg0,sreg1,dreg)
#define alpha_mulqv(inst,sreg0,sreg1,dreg)	alpha_encode_regops(inst,ALPHA_OP_MULQV,ALPHA_FUNC_MULQV,sreg0,sreg1,dreg)

/* memory branch operations */
#define alpha_jmp(inst,dreg,sreg,hint)		alpha_encode_mem_branch(inst,ALPHA_OP_JMP,ALPHA_FUNC_JMP,dreg,sreg,hint)
#define alpha_jsr(inst,dreg,sreg,hint)		alpha_encode_mem_branch(inst,ALPHA_OP_JSR,ALPHA_FUNC_JSR,dreg,sreg,hint)
#define alpha_ret(inst,sreg,hint)		alpha_encode_mem_branch(inst,ALPHA_OP_RET,ALPHA_FUNC_RET,ALPHA_ZERO,sreg,hint)
#define alpha_jsrco(inst,dreg,sreg,hint)	alpha_encode_mem_branch(inst,ALPHA_OP_JSRCO,ALPHA_FUNC_JSRCO,dreg,sreg,hint)

/* trap barrier */
#define alpha_trapb(inst)			alpha_encode_mem_branch(inst,ALPHA_OP_TRAPB,ALPHA_FUNC_TRAPB,0,0,0)

/* memory operations */
#define alpha_ldf(inst,dreg,sreg,offset)	alpha_encode_mem(inst,ALPHA_OP_LDF,dreg,sreg,offset)
#define alpha_ldg(inst,dreg,sreg,offset)	alpha_encode_mem(inst,ALPHA_OP_LDG,dreg,sreg,offset)
#define alpha_lds(inst,dreg,sreg,offset)	alpha_encode_mem(inst,ALPHA_OP_LDS,dreg,sreg,offset)
#define alpha_ldt(inst,dreg,sreg,offset)	alpha_encode_mem(inst,ALPHA_OP_LDT,dreg,sreg,offset)
#define alpha_stf(inst,dreg,sreg,offset)	alpha_encode_mem(inst,ALPHA_OP_STF,dreg,sreg,offset)
#define alpha_stg(inst,dreg,sreg,offset)	alpha_encode_mem(inst,ALPHA_OP_STG,dreg,sreg,offset)
#define alpha_sts(inst,dreg,sreg,offset)	alpha_encode_mem(inst,ALPHA_OP_STS,dreg,sreg,offset)
#define alpha_stt(inst,dreg,sreg,offset)	alpha_encode_mem(inst,ALPHA_OP_STT,dreg,sreg,offset)
#define alpha_ldl(inst,dreg,sreg,offset)	alpha_encode_mem(inst,ALPHA_OP_LDL,dreg,sreg,offset)
#define alpha_ldq(inst,dreg,sreg,offset)	alpha_encode_mem(inst,ALPHA_OP_LDQ,dreg,sreg,offset)
#define alpha_ldl_l(inst,dreg,sreg,offset)	alpha_encode_mem(inst,ALPHA_OP_LDL_L,dreg,sreg,offset)
#define alpha_ldq_l(inst,dreg,sreg,offset)	alpha_encode_mem(inst,ALPHA_OP_LDQ_L,dreg,sreg,offset)
#define alpha_ldqf(inst,dreg,sreg,offset)	alpha_encode_mem(inst,ALPHA_OP_LDQF,dreg,sreg,offset)
#define alpha_stl(inst,dreg,sreg,offset)	alpha_encode_mem(inst,ALPHA_OP_STL,dreg,sreg,offset)
#define alpha_stq(inst,dreg,sreg,offset)	alpha_encode_mem(inst,ALPHA_OP_STQ,dreg,sreg,offset)
#define alpha_stl_c(inst,dreg,sreg,offset)	alpha_encode_mem(inst,ALPHA_OP_STL_C,dreg,sreg,offset)
#define alpha_stq_c(inst,dreg,sreg,offset)	alpha_encode_mem(inst,ALPHA_OP_STQ_C,dreg,sreg,offset)
#define alpha_br(inst,reg,offset)		alpha_encode_branch(inst,ALPHA_OP_BR,reg,offset)
#define alpha_fbeq(inst,reg,offset)		alpha_encode_branch(inst,ALPHA_OP_FBEQ,reg,offset)
#define alpha_fblt(inst,reg,offset)		alpha_encode_branch(inst,ALPHA_OP_FBLT,reg,offset)
#define alpha_fble(inst,reg,offset)		alpha_encode_branch(inst,ALPHA_OP_FBLE,reg,offset)
#define alpha_bsr(inst,reg,offset)		alpha_encode_branch(inst,ALPHA_OP_BSR,reg,offset)
#define alpha_fbne(inst,reg,offset)		alpha_encode_branch(inst,ALPHA_OP_FBNE,reg,offset)
#define alpha_fbge(inst,reg,offset)		alpha_encode_branch(inst,ALPHA_OP_FBGE,reg,offset)
#define alpha_fbgt(inst,reg,offset)		alpha_encode_branch(inst,ALPHA_OP_FBGT,reg,offset)
#define alpha_blbc(inst,reg,offset)		alpha_encode_branch(inst,ALPHA_OP_BLBC,reg,offset)
#define alpha_beq(inst,reg,offset)		alpha_encode_branch(inst,ALPHA_OP_BEQ,reg,offset)
#define alpha_blt(inst,reg,offset)		alpha_encode_branch(inst,ALPHA_OP_BLT,reg,offset)
#define alpha_ble(inst,reg,offset) 		alpha_encode_branch(inst,ALPHA_OP_BLE,reg,offset)
#define alpha_blbs(inst,reg,offset)		alpha_encode_branch(inst,ALPHA_OP_BLBS,reg,offset)
#define alpha_bne(inst,reg,offset)		alpha_encode_branch(inst,ALPHA_OP_BNE,reg,offset)
#define alpha_bge(inst,reg,offset)		alpha_encode_branch(inst,ALPHA_OP_BGE,reg,offset)
#define alpha_bgt(inst,reg,offset)		alpha_encode_branch(inst,ALPHA_OP_BGT,reg,offset)

/* Floating point conversion instructions */
#define alpha_ftoit(inst,fsreg,dreg)		alpha_encode_fpop(inst,ALPHA_OP_FTOIT,ALPHA_FUNC_FTOIT,fsreg,ALPHA_ZERO,dreg)
#define alpha_ftois(inst,fsreg,dreg)		alpha_encode_fpop(inst,ALPHA_OP_FTOIS,ALPHA_FUNC_FTOIS,fsreg,ALPHA_ZERO,dreg)

#define alpha_itofs(inst,sreg,fdreg)		alpha_encode_fpop(inst,ALPHA_OP_ITOFS,ALPHA_FUNC_ITOFS,sreg,ALPHA_ZERO,fdreg)
#define alpha_itoff(inst,sreg,fdreg)		alpha_encode_fpop(inst,ALPHA_OP_ITOFF,ALPHA_FUNC_ITOFF,sreg,ALPHA_ZERO,fdreg)
#define alpha_itoft(inst,sreg,fdreg)		alpha_encode_fpop(inst,ALPHA_OP_ITOFT,ALPHA_FUNC_ITOFT,sreg,ALPHA_ZERO,fdreg)

/* Floating point arithmetic instructions */
#define alpha_adds(inst,fsreg0,fsreg1,fdreg)	 alpha_encode_fpop(inst,ALPHA_OP_ADDS,ALPHA_FUNC_ADDS,fsreg0,fsreg1,fdreg)
#define alpha_subs(inst,fsreg0,fsreg1,fdreg)	 alpha_encode_fpop(inst,ALPHA_OP_SUBS,ALPHA_FUNC_SUBS,fsreg0,fsreg1,fdreg)
#define alpha_addt(inst,fsreg0,fsreg1,fdreg)	 alpha_encode_fpop(inst,ALPHA_OP_ADDT,ALPHA_FUNC_ADDT,fsreg0,fsreg1,fdreg)
#define alpha_subt(inst,fsreg0,fsreg1,fdreg)	 alpha_encode_fpop(inst,ALPHA_OP_SUBT,ALPHA_FUNC_SUBT,fsreg0,fsreg1,fdreg)
#define alpha_mult(inst,fsreg0,fsreg1,fdreg)	 alpha_encode_fpop(inst,ALPHA_OP_MULT,ALPHA_FUNC_MULT,fsreg0,fsreg1,fdreg)
#define alpha_divt(inst,fsreg0,fsreg1,fdreg)	 alpha_encode_fpop(inst,ALPHA_OP_DIVT,ALPHA_FUNC_DIVT,fsreg0,fsreg1,fdreg)

/* Floating point sign copy instructions */
#define alpha_cpys(inst,fsreg0,fsreg1,fdreg)	alpha_encode_fpop(inst,ALPHA_OP_CPYS,ALPHA_FUNC_CPYS,fsreg0,fsreg1,fdreg)
#define alpha_cpysn(inst,fsreg0,fsreg1,fdreg)	alpha_encode_fpop(inst,ALPHA_OP_CPYSN,ALPHA_FUNC_CPYSN,fsreg0,fsreg1,fdreg)
#define alpha_cpyse(inst,fsreg0,fsreg1,fdreg)	alpha_encode_fpop(inst,ALPHA_OP_CPYSE,ALPHA_FUNC_CPYSE,fsreg0,fsreg1,fdreg)

/* load immediate pseudo instruction. */
#define _alpha_li64(code,dreg,val)				\
do {								\
	unsigned long c1 = val;					\
	unsigned int d1,d2,d3,d4;				\
								\
	/* This little bit of code deals with the sign bit   */	\
	/* It can be found in alpha_emit_set_long_const(...) */	\
	/* from gcc-3.4.6/gcc/config/alpha/alpha.c           */ \
								\
	d1  = ((c1 & 0xffff) ^ 0x8000) - 0x8000;		\
	c1 -= d1;						\
	d2  = ((c1 & 0xffffffff) ^ 0x80000000) - 0x80000000;	\
	c1  = (c1 - d2) >> 32;					\
	d3  = ((c1 & 0xffff) ^ 0x8000) - 0x8000;		\
	c1 -= d3;						\
	d4  = ((c1 & 0xffffffff) ^ 0x80000000) - 0x80000000;	\
								\
	alpha_ldah(code,dreg,ALPHA_ZERO,d4>>16);		\
	alpha_lda(code,dreg,dreg,d3);				\
	alpha_slli(code,dreg,32,dreg);				\
	alpha_ldah(code,dreg,dreg,d2>>16);			\
	alpha_lda(code,dreg,dreg,d1);				\
								\
} while (0)

#define _alpha_li32(code,dreg,val)				\
do {								\
	unsigned int c1 = val;					\
	unsigned int d1,d2;					\
								\
	d1  = ((c1 & 0xffff) ^ 0x8000) - 0x8000;		\
	c1 -= d1;						\
	d2  = ((c1 & 0xffffffff) ^ 0x80000000) - 0x80000000;	\
								\
	alpha_ldah(code,dreg,ALPHA_ZERO,d2>>16);		\
	alpha_lda(code,dreg,dreg,d1);				\
	alpha_slli(code,dreg,32,dreg);				\
	alpha_srli(code,dreg,32,dreg);				\
								\
} while (0)

#define _alpha_li16(code,dreg,val)						\
do {										\
	alpha_lda(code,dreg,ALPHA_ZERO,(((val & 0xffff) ^ 0x8000) - 0x8000));	\
	alpha_slli(code,dreg,48,dreg);						\
	alpha_srli(code,dreg,48,dreg);						\
										\
} while (0)

#define _alpha_li8(code,dreg,val)					\
do {									\
	alpha_lda(code,dreg,ALPHA_ZERO,(((val & 0xff) ^ 0x80) - 0x80));	\
	alpha_slli(code,dreg,56,dreg);					\
	alpha_srli(code,dreg,56,dreg);					\
									\
} while (0)

#define alpha_li(code,dreg,val)					\
do {								\
	switch (sizeof(val)) {					\
		case 1:						\
			_alpha_li8(code,dreg,val);		\
			break;					\
		case 2:						\
			_alpha_li16(code,dreg,val);		\
			break;					\
		case 4:						\
			_alpha_li32(code,dreg,val);		\
			break;					\
		case 8:						\
			_alpha_li64(code,dreg,val);		\
			break;					\
		default:					\
			_alpha_li64(code,dreg,val);		\
			break;					\
	}							\
} while (0)

/*
 * Call a subroutine at a specific target location.
 */

#define alpha_call(inst,target)					\
do {								\
	alpha_li(inst,ALPHA_AT,(unsigned long)target);		\
	alpha_jsr(inst,ALPHA_RA,ALPHA_AT,1);			\
} while (0)

#ifdef __cplusplus
};
#endif

#endif /* _JIT_GEN_ALPHA_H */
