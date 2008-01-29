/*
 * jit-gen-x86-64.h - Macros for generating x86_64 code.
 *
 * Copyright (C) 2008  Southern Storm Software, Pty Ltd.
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

#ifndef	_JIT_GEN_X86_64_H
#define	_JIT_GEN_X86_64_H

#include <jit/jit-defs.h>
#include "jit-gen-x86.h"

#ifdef	__cplusplus
extern	"C" {
#endif

/*
 * X86_64 64 bit general purpose integer registers.
 */
typedef enum
{
	X86_64_RAX = 0,
	X86_64_RCX = 1,
	X86_64_RDX = 2,
	X86_64_RBX = 3,
	X86_64_RSP = 4,
	X86_64_RBP = 5,
	X86_64_RSI = 6,
	X86_64_RDI = 7,
	X86_64_R8  = 8,
	X86_64_R9  = 9,
	X86_64_R10 = 10,
	X86_64_R11 = 11,
	X86_64_R12 = 12,
	X86_64_R13 = 13,
	X86_64_R14 = 14,
	X86_64_R15 = 15,
	X86_64_RIP = 16		/* This register encoding doesn't exist in the */
						/* instructions. It's used for RIP relative encoding. */
} X86_64_Reg_No;

/*
 * X86-64 xmm registers.
 */
typedef enum
{
	X86_64_XMM0 = 0,
	X86_64_XMM1 = 1,
	X86_64_XMM2 = 2,
	X86_64_XMM3 = 3,
	X86_64_XMM4 = 4,
	X86_64_XMM5 = 5,
	X86_64_XMM6 = 6,
	X86_64_XMM7 = 7,
	X86_64_XMM8 = 8,
	X86_64_XMM9 = 9,
	X86_64_XMM10 = 10,
	X86_64_XMM11 = 11,
	X86_64_XMM12 = 12,
	X86_64_XMM13 = 13,
	X86_64_XMM14 = 14,
	X86_64_XMM15 = 15
} X86_64_XMM_Reg_No;

/*
 * Bits in the REX prefix byte.
 */
typedef enum
{
	X86_64_REX_B = 1,	/* 1-bit (high) extension of the ModRM r/m field */
						/* SIB base field, or opcode reg field, thus */
						/* permitting access to 16 registers. */
	X86_64_REX_X = 2,	/* 1-bit (high) extension of the SIB index field */
						/* thus permitting access to 16 registers. */
	X86_64_REX_R = 4,	/* 1-bit (high) extension of the ModRM reg field, */
						/* thus permitting access to 16 registers. */
	X86_64_REX_W = 8	/* 0 = Default operand size */
						/* 1 = 64 bit operand size */
} X86_64_REX_Bits;

/*
 * Helper union for emmitting 64 bit immediate values.
 */
typedef union
{
	jit_long val;
	unsigned char b[8];
} x86_64_imm_buf;

#define x86_64_imm_emit64(inst, imm) \
	do { \
		x86_64_imm_buf imb; \
		imb.val = (jit_long)(imm); \
		*(inst)++ = imb.b[0]; \
		*(inst)++ = imb.b[1]; \
		*(inst)++ = imb.b[2]; \
		*(inst)++ = imb.b[3]; \
		*(inst)++ = imb.b[4]; \
		*(inst)++ = imb.b[5]; \
		*(inst)++ = imb.b[6]; \
		*(inst)++ = imb.b[7]; \
	} while (0)

#define x86_64_imm_emit_max32(inst, imm, size) \
	do { \
		switch((size)) \
		{ \
			case 1: \
			{ \
				x86_imm_emit8(inst, (imm)); \
			} \
			break; \
			case 2: \
			{ \
				x86_imm_emit16(inst, (imm)); \
			} \
			break; \
			case 4: \
			case 8: \
			{ \
				x86_imm_emit32((inst), (imm)); \
			} \
			break; \
			default: \
			{ \
				jit_assert(0); \
			} \
		} \
	} while(0)

#define x86_64_imm_emit_max64(inst, imm, size) \
	do { \
		switch((size)) \
		{ \
			case 1: \
			{ \
				x86_imm_emit8(inst, (imm)); \
			} \
			break; \
			case 2: \
			{ \
				x86_imm_emit16(inst, (imm)); \
			} \
			break; \
			case 4: \
			{ \
				x86_imm_emit32((inst), (imm)); \
			} \
			break; \
			case 8: \
			{ \
				x86_64_imm_emit64(inst, (imm)); \
			} \
			break; \
			default: \
			{ \
				jit_assert(0); \
			} \
		} \
	} while(0)

#define x86_64_rex(rex_bits)	(0x40 | (rex_bits))
#define x86_64_rex_emit(inst, width, modrm_reg, index_reg, rm_base_opcode_reg) \
	do { \
		unsigned char __rex_bits = \
			(((width) > 4) ? X86_64_REX_W : 0) | \
			(((modrm_reg) > 7) ? X86_64_REX_R : 0) | \
			(((index_reg) > 7) ? X86_64_REX_X : 0) | \
			(((rm_base_opcode_reg) > 7) ? X86_64_REX_B : 0); \
		if((__rex_bits != 0)) \
		{ \
			 *(inst)++ = x86_64_rex(__rex_bits); \
		} \
	} while(0)

/*
 * Helper for emitting the rex prefix for opcodes with 64bit default size.
 */
#define x86_64_rex_emit64(inst, width, modrm_reg, index_reg, rm_base_opcode_reg) \
	do { \
		x86_64_rex_emit((inst), 0, (modrm_reg), (index_reg), (rm_base_opcode_reg)); \
	} while(0)

/* In 64 bit mode, all registers have a low byte subregister */
#undef X86_IS_BYTE_REG
#define X86_IS_BYTE_REG(reg) 1

#define x86_64_reg_emit(inst, r, regno) \
	do { \
		x86_reg_emit((inst), ((r) & 0x7), ((regno) & 0x7)); \
	} while(0)

#define x86_64_mem_emit(inst, r, disp) \
	do { \
		x86_address_byte ((inst), 0, ((r) & 0x7), 4); \
		x86_address_byte ((inst), 0, 4, 5); \
		x86_imm_emit32((inst), (disp)); \
	} while(0)

#define x86_64_mem64_emit(inst, r, disp) \
	do { \
		x86_address_byte ((inst), 0, ((r) & 0x7), 4); \
		x86_address_byte ((inst), 0, 4, 5); \
		x86_64_imm_emit64((inst), (disp)); \
	} while(0)

#define x86_64_membase_emit(inst, reg, basereg, disp) \
	do { \
		if((basereg) == X86_64_RIP) \
		{ \
			x86_address_byte((inst), 0, ((reg) & 0x7), 5); \
			x86_imm_emit32((inst), (disp)); \
		} \
		else \
		{ \
			x86_membase_emit((inst), ((reg) & 0x7), ((basereg) & 0x7), (disp)); \
		} \
	} while(0)

#define x86_64_memindex_emit(inst, r, basereg, disp, indexreg, shift) \
	do { \
		x86_memindex_emit((inst), ((r) & 0x7), ((basereg) & 0x7), (disp), ((indexreg) & 0x7), (shift)); \
	} while(0)

/*
 * RSP, RBP and the corresponding upper registers (R12 and R13) can't be used
 * for relative addressing without displacement because their codes are used
 * for encoding addressing modes with diplacement.
 * So we do a membase addressing in this case with a zero offset.
 */
#define x86_64_regp_emit(inst, r, regno) \
	do { \
		switch(regno) \
		{ \
			case X86_64_RSP: \
			case X86_64_RBP: \
			case X86_64_R12: \
			case X86_64_R13: \
			{ \
				x86_64_membase_emit((inst), (r), (regno), 0); \
			} \
			break; \
			default: \
			{ \
				x86_address_byte((inst), 0, ((r) & 0x7), ((regno) & 0x7)); \
			} \
			break; \
		} \
	} while(0)

/*
 * Helper to encode an opcode where the encoding is different between
 * 8bit and 16 ... 64 bit width in the following way:
 * 8 bit == opcode given
 * 16 ... 64 bit = opcode given | 0x1
 */
#define x86_64_opcode1_emit(inst, opc, size) \
	do { \
		switch ((size)) \
		{ \
			case 1: \
			{ \
				*(inst)++ = (unsigned char)(opc); \
			} \
			break;	\
			case 2: \
			case 4: \
			case 8: \
			{ \
				*(inst)++ = ((unsigned char)(opc) | 0x1); \
			} \
			break;\
			default: \
			{ \
				jit_assert(0); \
			} \
		} \
	} while(0)

/*
 * Macros to implement the simple opcodes.
 */
#define x86_64_alu_reg_reg_size(inst, opc, dreg, sreg, size) \
	do { \
		switch(size) \
		{ \
			case 1: \
			{ \
				x86_64_rex_emit(inst, size, (dreg), 0, (sreg)); \
				*(inst)++ = (((unsigned char)(opc)) << 3) + 2; \
				x86_64_reg_emit((inst), (dreg), (sreg)); \
			} \
			break; \
			case 2: \
			{ \
				*(inst)++ = (unsigned char)0x66; \
			} \
			case 4: \
			case 8: \
			{ \
				x86_64_rex_emit(inst, size, (dreg), 0, (sreg)); \
				*(inst)++ = (((unsigned char)(opc)) << 3) + 3; \
				x86_64_reg_emit((inst), (dreg), (sreg)); \
			} \
		} \
	} while(0)

#define x86_64_alu_regp_reg_size(inst, opc, dregp, sreg, size) \
	do { \
		switch(size) \
		{ \
			case 1: \
			{ \
				x86_64_rex_emit(inst, size, (sreg), 0, (dregp)); \
				*(inst)++ = (((unsigned char)(opc)) << 3); \
				x86_64_regp_emit((inst), (sreg), (dregp));	\
			} \
			break; \
			case 2: \
			{ \
				*(inst)++ = (unsigned char)0x66; \
			} \
			case 4: \
			case 8: \
			{ \
				x86_64_rex_emit(inst, size, (sreg), 0, (dregp)); \
				*(inst)++ = (((unsigned char)(opc)) << 3) + 1; \
				x86_64_regp_emit((inst), (sreg), (dregp));	\
			} \
		} \
	} while(0)

#define x86_64_alu_mem_reg_size(inst, opc, mem, sreg, size) \
	do { \
		switch(size) \
		{ \
			case 1: \
			{ \
				x86_64_rex_emit(inst, size, (sreg), 0, 0); \
				*(inst)++ = (((unsigned char)(opc)) << 3); \
				x86_64_mem_emit((inst), (sreg), (mem));	\
			} \
			break; \
			case 2: \
			{ \
				*(inst)++ = (unsigned char)0x66; \
			} \
			case 4: \
			case 8: \
			{ \
				x86_64_rex_emit(inst, size, (sreg), 0, 0); \
				*(inst)++ = (((unsigned char)(opc)) << 3) + 1; \
				x86_64_mem_emit((inst), (sreg), (mem));	\
			} \
		} \
	} while(0)

#define x86_64_alu_membase_reg_size(inst, opc, basereg, disp, sreg, size) \
	do { \
		switch(size) \
		{ \
			case 1: \
			{ \
				x86_64_rex_emit(inst, size, (sreg), 0, (basereg)); \
				*(inst)++ = (((unsigned char)(opc)) << 3); \
				x86_64_membase_emit((inst), (sreg), (basereg), (disp)); \
			} \
			break; \
			case 2: \
			{ \
				*(inst)++ = (unsigned char)0x66; \
			} \
			case 4: \
			case 8: \
			{ \
				x86_64_rex_emit(inst, size, (sreg), 0, (basereg)); \
				*(inst)++ = (((unsigned char)(opc)) << 3) + 1; \
				x86_64_membase_emit((inst), (sreg), (basereg), (disp)); \
			} \
		} \
	} while(0)

#define x86_64_alu_memindex_reg_size(inst, opc, basereg, disp, indexreg, shift, sreg, size) \
	do { \
		switch(size) \
		{ \
			case 1: \
			{ \
				x86_64_rex_emit(inst, size, (sreg), (indexreg), (basereg)); \
				*(inst)++ = (((unsigned char)(opc)) << 3); \
				x86_64_memindex_emit((inst), (sreg), (basereg), (disp), (indexreg), (shift)); \
			} \
			break; \
			case 2: \
			{ \
				*(inst)++ = (unsigned char)0x66; \
			} \
			case 4: \
			case 8: \
			{ \
				x86_64_rex_emit(inst, size, (sreg), (indexreg), (basereg)); \
				*(inst)++ = (((unsigned char)(opc)) << 3) + 1; \
				x86_64_memindex_emit((inst), (sreg), (basereg), (disp), (indexreg), (shift)); \
			} \
		} \
	} while(0)

#define x86_64_alu_reg_regp_size(inst, opc, dreg, sregp, size) \
	do { \
		switch(size) \
		{ \
			case 1: \
			{ \
				x86_64_rex_emit(inst, size, (dreg), 0, (sregp)); \
				*(inst)++ = (((unsigned char)(opc)) << 3) + 2; \
				x86_64_regp_emit((inst), (dreg), (sregp));	\
			} \
			break; \
			case 2: \
			{ \
				*(inst)++ = (unsigned char)0x66; \
			} \
			case 4: \
			case 8: \
			{ \
				x86_64_rex_emit(inst, size, (dreg), 0, (sregp)); \
				*(inst)++ = (((unsigned char)(opc)) << 3) + 3; \
				x86_64_regp_emit((inst), (dreg), (sregp));	\
			} \
		} \
	} while(0)

#define x86_64_alu_reg_mem_size(inst, opc, dreg, mem, size) \
	do { \
		switch(size) \
		{ \
			case 1: \
			{ \
				x86_64_rex_emit(inst, size, (dreg), 0, 0); \
				*(inst)++ = (((unsigned char)(opc)) << 3) + 2; \
				x86_64_mem_emit((inst), (dreg), (mem));	\
			} \
			break; \
			case 2: \
			{ \
				*(inst)++ = (unsigned char)0x66; \
			} \
			case 4: \
			case 8: \
			{ \
				x86_64_rex_emit(inst, size, (dreg), 0, 0); \
				*(inst)++ = (((unsigned char)(opc)) << 3) + 3; \
				x86_64_mem_emit((inst), (dreg), (mem));	\
			} \
		} \
	} while(0)

#define x86_64_alu_reg_membase_size(inst, opc, dreg, basereg, disp, size) \
	do { \
		switch(size) \
		{ \
			case 1: \
			{ \
				x86_64_rex_emit(inst, size, (dreg), 0, (basereg)); \
				*(inst)++ = (((unsigned char)(opc)) << 3) + 2; \
				x86_64_membase_emit((inst), (dreg), (basereg), (disp)); \
			} \
			break; \
			case 2: \
			{ \
				*(inst)++ = (unsigned char)0x66; \
			} \
			case 4: \
			case 8: \
			{ \
				x86_64_rex_emit(inst, size, (dreg), 0, (basereg)); \
				*(inst)++ = (((unsigned char)(opc)) << 3) + 3; \
				x86_64_membase_emit((inst), (dreg), (basereg), (disp)); \
			} \
		} \
	} while(0)

#define x86_64_alu_reg_memindex_size(inst, opc, dreg, basereg, disp, indexreg, shift, size) \
	do { \
		switch(size) \
		{ \
			case 1: \
			{ \
				x86_64_rex_emit(inst, size, (dreg), (indexreg), (basereg)); \
				*(inst)++ = (((unsigned char)(opc)) << 3) + 2; \
				x86_64_memindex_emit((inst), (dreg), (basereg), (disp), (indexreg), (shift)); \
			} \
			break; \
			case 2: \
			{ \
				*(inst)++ = (unsigned char)0x66; \
			} \
			case 4: \
			case 8: \
			{ \
				x86_64_rex_emit(inst, size, (dreg), (indexreg), (basereg)); \
				*(inst)++ = (((unsigned char)(opc)) << 3) + 3; \
				x86_64_memindex_emit((inst), (dreg), (basereg), (disp), (indexreg), (shift)); \
			} \
		} \
	} while(0)

/*
 * The immediate value has to be at most 32 bit wide.
 */
#define x86_64_alu_reg_imm_size(inst, opc, dreg, imm, size) \
	do { \
		if((dreg) == X86_64_RAX) \
		{ \
			switch(size) \
			{ \
				case 1: \
				{ \
					*(inst)++ = (((unsigned char)(opc)) << 3) + 4; \
					x86_imm_emit8((inst), (imm)); \
				} \
				break; \
				case 2: \
				{ \
					*(inst)++ = (unsigned char)0x66; \
					*(inst)++ = (((unsigned char)(opc)) << 3) + 5; \
					x86_imm_emit16((inst), (imm)); \
				} \
				break; \
				case 4: \
				case 8: \
				{ \
					x86_64_rex_emit((inst), (size), 0, 0, 0); \
					*(inst)++ = (((unsigned char)(opc)) << 3) + 5; \
					x86_imm_emit32((inst), (imm)); \
				} \
			} \
		} \
		else if(x86_is_imm8((imm))) \
		{ \
			switch(size) \
			{ \
				case 1: \
				{ \
					x86_64_rex_emit(inst, size, 0, 0, (dreg)); \
					*(inst)++ = (unsigned char)0x80; \
				} \
				break; \
				case 2: \
				{ \
					*(inst)++ = (unsigned char)0x66; \
				} \
				case 4: \
				case 8: \
				{ \
					x86_64_rex_emit(inst, size, 0, 0, (dreg)); \
					*(inst)++ = (unsigned char)0x83; \
				} \
			} \
			x86_64_reg_emit((inst), (opc), (dreg)); \
			x86_imm_emit8((inst), (imm)); \
		} \
		else \
		{ \
			switch(size) \
			{ \
				case 1: \
				{ \
					x86_64_rex_emit(inst, size, 0, 0, (dreg)); \
					*(inst)++ = (unsigned char)0x80; \
					x86_64_reg_emit((inst), (opc), (dreg)); \
					x86_imm_emit8((inst), (imm)); \
					jit_assert(1); \
				} \
				break; \
				case 2: \
				{ \
					*(inst)++ = (unsigned char)0x66; \
					x86_64_rex_emit(inst, size, 0, 0, (dreg)); \
					*(inst)++ = (unsigned char)0x81; \
					x86_64_reg_emit((inst), (opc), (dreg)); \
					x86_imm_emit16((inst), (imm)); \
				} \
				break; \
				case 4: \
				case 8: \
				{ \
					x86_64_rex_emit(inst, size, 0, 0, (dreg)); \
					*(inst)++ = (unsigned char)0x81; \
					x86_64_reg_emit((inst), (opc), (dreg)); \
					x86_imm_emit32((inst), (imm)); \
				} \
			} \
		} \
	} while(0)

#define x86_64_alu_regp_imm_size(inst, opc, reg, imm, size) \
	do { \
		if(x86_is_imm8((imm))) \
		{ \
			switch(size) \
			{ \
				case 1: \
				{ \
					x86_64_rex_emit(inst, size, 0, 0, (reg)); \
					*(inst)++ = (unsigned char)0x80; \
				} \
				break; \
				case 2: \
				{ \
					*(inst)++ = (unsigned char)0x66; \
				} \
				case 4: \
				case 8: \
				{ \
					x86_64_rex_emit(inst, size, 0, 0, (reg)); \
					*(inst)++ = (unsigned char)0x83; \
				} \
			} \
			x86_64_regp_emit((inst), (opc), (reg)); \
			x86_imm_emit8((inst), (imm)); \
		} \
		else \
		{ \
			switch(size) \
			{ \
				case 1: \
				{ \
					x86_64_rex_emit(inst, size, 0, 0, (reg)); \
					*(inst)++ = (unsigned char)0x80; \
					x86_64_regp_emit((inst), (opc), (reg)); \
					x86_imm_emit8((inst), (imm)); \
					jit_assert(1); \
				} \
				break; \
				case 2: \
				{ \
					*(inst)++ = (unsigned char)0x66; \
					x86_64_rex_emit(inst, size, 0, 0, (reg)); \
					*(inst)++ = (unsigned char)0x81; \
					x86_64_regp_emit((inst), (opc), (reg)); \
					x86_imm_emit16((inst), (imm)); \
				} \
				break; \
				case 4: \
				case 8: \
				{ \
					x86_64_rex_emit(inst, size, 0, 0, (reg)); \
					*(inst)++ = (unsigned char)0x81; \
					x86_64_regp_emit((inst), (opc), (reg)); \
					x86_imm_emit32((inst), (imm)); \
				} \
			} \
		} \
	} while(0)

#define x86_64_alu_mem_imm_size(inst, opc, mem, imm, size) \
	do { \
		if(x86_is_imm8((imm))) \
		{ \
			switch(size) \
			{ \
				case 1: \
				{ \
					x86_64_rex_emit((inst), (size), 0, 0, 0); \
					*(inst)++ = (unsigned char)0x80; \
				} \
				break; \
				case 2: \
				{ \
					*(inst)++ = (unsigned char)0x66; \
				} \
				case 4: \
				case 8: \
				{ \
					x86_64_rex_emit((inst), (size), 0, 0, 0); \
					*(inst)++ = (unsigned char)0x83; \
				} \
			} \
			x86_64_mem_emit((inst), (opc), (mem));	\
			x86_imm_emit8((inst), (imm));	\
		} \
		else \
		{ \
			switch(size) \
			{ \
				case 1: \
				{ \
					x86_64_rex_emit((inst), (size), 0, 0, 0); \
					*(inst)++ = (unsigned char)0x80; \
					x86_64_mem_emit((inst), (opc), (mem));	\
					x86_imm_emit8((inst), (imm)); \
					jit_assert(1); \
				} \
				break; \
				case 2: \
				{ \
					*(inst)++ = (unsigned char)0x66; \
					x86_64_rex_emit((inst), (size), 0, 0, 0); \
					*(inst)++ = (unsigned char)0x81; \
					x86_64_mem_emit((inst), (opc), (mem));	\
					x86_imm_emit16((inst), (imm)); \
				} \
				break; \
				case 4: \
				case 8: \
				{ \
					x86_64_rex_emit((inst), (size), 0, 0, 0); \
					*(inst)++ = (unsigned char)0x81; \
					x86_64_mem_emit((inst), (opc), (mem));	\
					x86_imm_emit32((inst), (imm)); \
				} \
			} \
		} \
	} while(0)

#define x86_64_alu_membase_imm_size(inst, opc, basereg, disp, imm, size) \
	do { \
		if(x86_is_imm8((imm))) \
		{ \
			switch(size) \
			{ \
				case 1: \
				{ \
					x86_64_rex_emit((inst), (size), 0, 0, (basereg)); \
					*(inst)++ = (unsigned char)0x80; \
				} \
				break; \
				case 2: \
				{ \
					*(inst)++ = (unsigned char)0x66; \
				} \
				case 4: \
				case 8: \
				{ \
					x86_64_rex_emit((inst), (size), 0, 0, (basereg)); \
					*(inst)++ = (unsigned char)0x83; \
				} \
			} \
			x86_64_membase_emit((inst), (opc), (basereg), (disp));	\
			x86_imm_emit8((inst), (imm));	\
		} \
		else \
		{ \
			switch(size) \
			{ \
				case 1: \
				{ \
					x86_64_rex_emit((inst), (size), 0, 0, (basereg)); \
					*(inst)++ = (unsigned char)0x80; \
					x86_64_membase_emit((inst), (opc), (basereg), (disp));	\
					x86_imm_emit8((inst), (imm)); \
					jit_assert(1); \
				} \
				break; \
				case 2: \
				{ \
					*(inst)++ = (unsigned char)0x66; \
					x86_64_rex_emit((inst), (size), 0, 0, (basereg)); \
					*(inst)++ = (unsigned char)0x81; \
					x86_64_membase_emit((inst), (opc), (basereg), (disp));	\
					x86_imm_emit16((inst), (imm)); \
				} \
				break; \
				case 4: \
				case 8: \
				{ \
					x86_64_rex_emit((inst), (size), 0, 0, (basereg)); \
					*(inst)++ = (unsigned char)0x81; \
					x86_64_membase_emit((inst), (opc), (basereg), (disp));	\
					x86_imm_emit32((inst), (imm)); \
				} \
			} \
		} \
	} while(0)

#define x86_64_alu_memindex_imm_size(inst, opc, basereg, disp, indexreg, shift, imm, size) \
	do { \
		if(x86_is_imm8((imm))) \
		{ \
			switch(size) \
			{ \
				case 1: \
				{ \
					x86_64_rex_emit((inst), (size), 0, (indexreg), (basereg)); \
					*(inst)++ = (unsigned char)0x80; \
				} \
				break; \
				case 2: \
				{ \
					*(inst)++ = (unsigned char)0x66; \
				} \
				case 4: \
				case 8: \
				{ \
					x86_64_rex_emit((inst), (size), 0, (indexreg), (basereg)); \
					*(inst)++ = (unsigned char)0x83; \
				} \
			} \
			x86_64_memindex_emit((inst), (opc), (basereg), (disp), (indexreg), (shift)); \
			x86_imm_emit8((inst), (imm)); \
		} \
		else \
		{ \
			switch(size) \
			{ \
				case 1: \
				{ \
					x86_64_rex_emit((inst), (size), 0, (indexreg), (basereg)); \
					*(inst)++ = (unsigned char)0x80; \
					x86_64_memindex_emit((inst), (opc), (basereg), (disp), (indexreg), (shift)); \
					x86_imm_emit8((inst), (imm)); \
					jit_assert(1); \
				} \
				break; \
				case 2: \
				{ \
					*(inst)++ = (unsigned char)0x66; \
					x86_64_rex_emit((inst), (size), 0, (indexreg), (basereg)); \
					*(inst)++ = (unsigned char)0x81; \
					x86_64_memindex_emit((inst), (opc), (basereg), (disp), (indexreg), (shift)); \
					x86_imm_emit16((inst), (imm)); \
				} \
				break; \
				case 4: \
				case 8: \
				{ \
					x86_64_rex_emit((inst), (size), 0, (indexreg), (basereg)); \
					*(inst)++ = (unsigned char)0x81; \
					x86_64_memindex_emit((inst), (opc), (basereg), (disp), (indexreg), (shift)); \
					x86_imm_emit32((inst), (imm)); \
				} \
			} \
		} \
	} while(0)

/*
 * Instructions with one opcode (plus optional r/m)
 */

#define x86_64_alu1_reg(inst, opc1, r, reg) \
	do { \
		x86_64_rex_emit((inst), 0, 0, 0, (reg)); \
		*(inst)++ = (unsigned char)(opc1); \
		x86_64_reg_emit((inst), (r), (reg));	\
	} while(0)

#define x86_64_alu1_regp(inst, opc1, r, regp) \
	do { \
		x86_64_rex_emit((inst), 0, 0, 0, (regp)); \
		*(inst)++ = (unsigned char)(opc1); \
		x86_64_regp_emit((inst), (r), (regp));	\
	} while(0)

#define x86_64_alu1_mem(inst, opc1, r, mem) \
	do { \
		*(inst)++ = (unsigned char)(opc1); \
		x86_64_mem_emit((inst), (r), (mem)); \
	} while(0)

#define x86_64_alu1_membase(inst, opc1, r, basereg, disp) \
	do { \
		x86_64_rex_emit((inst), 0, 0, 0, (basereg)); \
		*(inst)++ = (unsigned char)(opc1); \
		x86_64_membase_emit((inst), (r), (basereg), (disp)); \
	} while(0)
	
#define x86_64_alu1_memindex(inst, opc1, r, basereg, disp, indexreg, shift) \
	do { \
		x86_64_rex_emit((inst), 0, 0, (indexreg), (basereg)); \
		*(inst)++ = (unsigned char)(opc1); \
		x86_64_memindex_emit((inst), (r), (basereg), (disp), (indexreg), (shift)); \
	} while(0)

#define x86_64_alu1_reg_size(inst, opc1, r, reg, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit((inst), (size), 0, 0, (reg)); \
		x86_64_opcode1_emit((inst), (opc1), (size)); \
		x86_64_reg_emit((inst), (r), (reg));	\
	} while(0)

#define x86_64_alu1_regp_size(inst, opc1, r, regp, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit((inst), (size), 0, 0, (regp)); \
		x86_64_opcode1_emit((inst), (opc1), (size)); \
		x86_64_regp_emit((inst), (r), (regp));	\
	} while(0)

#define x86_64_alu1_mem_size(inst, opc1, r, mem, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit((inst), (size), 0, 0, 0); \
		x86_64_opcode1_emit((inst), (opc1), (size)); \
		x86_64_mem_emit((inst), (r), (mem)); \
	} while(0)

#define x86_64_alu1_membase_size(inst, opc1, r, basereg, disp, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit((inst), (size), 0, 0, (basereg)); \
		x86_64_opcode1_emit((inst), (opc1), (size)); \
		x86_64_membase_emit((inst), (r), (basereg), (disp)); \
	} while(0)
	
#define x86_64_alu1_memindex_size(inst, opc1, r, basereg, disp, indexreg, shift, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit((inst), (size), 0, (indexreg), (basereg)); \
		x86_64_opcode1_emit((inst), (opc1), (size)); \
		x86_64_memindex_emit((inst), (r), (basereg), (disp), (indexreg), (shift)); \
	} while(0)

#define x86_64_alu2_reg_reg_size(inst, opc1, opc2, dreg, sreg, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit((inst), (size), (dreg), 0, (sreg)); \
		*(inst)++ = (unsigned char)(opc1); \
		*(inst)++ = (unsigned char)(opc2); \
		x86_64_reg_emit((inst), (dreg), (sreg));	\
	} while(0)

#define x86_64_alu2_reg_regp_size(inst, opc1, opc2, dreg, sregp, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit((inst), (size), (dreg), 0, (sregp)); \
		*(inst)++ = (unsigned char)(opc1); \
		*(inst)++ = (unsigned char)(opc2); \
		x86_64_regp_emit((inst), (dreg), (sregp));	\
	} while(0)

#define x86_64_alu2_reg_mem_size(inst, opc1, opc2, dreg, mem, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit((inst), (size), (dreg), 0, 0); \
		*(inst)++ = (unsigned char)(opc1); \
		*(inst)++ = (unsigned char)(opc2); \
		x86_64_mem_emit((inst), (dreg), (mem)); \
	} while(0)

#define x86_64_alu2_reg_membase_size(inst, opc1, opc2, dreg, basereg, disp, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit((inst), (size), (dreg), 0, (basereg)); \
		*(inst)++ = (unsigned char)(opc1); \
		*(inst)++ = (unsigned char)(opc2); \
		x86_64_membase_emit((inst), (dreg), (basereg), (disp)); \
	} while(0)
	
#define x86_64_alu2_reg_memindex_size(inst, opc1, opc2, dreg, basereg, disp, indexreg, shift, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit((inst), (size), (dreg), (indexreg), (basereg)); \
		*(inst)++ = (unsigned char)(opc1); \
		*(inst)++ = (unsigned char)(opc2); \
		x86_64_memindex_emit((inst), (dreg), (basereg), (disp), (indexreg), (shift)); \
	} while(0)

/*
 * xmm instructions with two opcodes
 */
#define x86_64_xmm2_reg_reg(inst, opc1, opc2, r, reg) \
	do { \
		x86_64_rex_emit(inst, 0, (r), 0, (reg)); \
		*(inst)++ = (unsigned char)(opc1); \
		*(inst)++ = (unsigned char)(opc2); \
		x86_64_reg_emit(inst, (r), (reg)); \
	} while(0)

#define x86_64_xmm2_reg_regp(inst, opc1, opc2, r, regp) \
	do { \
		x86_64_rex_emit(inst, 0, (r), 0, (regp)); \
		*(inst)++ = (unsigned char)(opc1); \
		*(inst)++ = (unsigned char)(opc2); \
		x86_64_regp_emit(inst, (r), (regp)); \
	} while(0)

#define x86_64_xmm2_reg_membase(inst, opc1, opc2, r, basereg, disp) \
	do { \
		x86_64_rex_emit(inst, 0, (r), 0, (basereg)); \
		*(inst)++ = (unsigned char)(opc1); \
		*(inst)++ = (unsigned char)(opc2); \
		x86_64_membase_emit(inst, (r), (basereg), (disp)); \
	} while(0)

#define x86_64_xmm2_reg_memindex(inst, opc1, opc2, r, basereg, disp, indexreg, shift) \
	do { \
		x86_64_rex_emit(inst, 0, (r), (indexreg), (basereg)); \
		*(inst)++ = (unsigned char)(opc1); \
		*(inst)++ = (unsigned char)(opc2); \
		x86_64_memindex_emit((inst), (r), (basereg), (disp), (indexreg), (shift)); \
	} while(0)

/*
 * xmm instructions with a prefix and two opcodes
 */
#define x86_64_p1_xmm2_reg_reg(inst, p1, opc1, opc2, r, reg) \
	do { \
		*(inst)++ = (unsigned char)(p1); \
		x86_64_rex_emit(inst, 0, (r), 0, (reg)); \
		*(inst)++ = (unsigned char)(opc1); \
		*(inst)++ = (unsigned char)(opc2); \
		x86_64_reg_emit(inst, (r), (reg)); \
	} while(0)

#define x86_64_p1_xmm2_reg_regp(inst, p1, opc1, opc2, r, regp) \
	do { \
		*(inst)++ = (unsigned char)(p1); \
		x86_64_rex_emit(inst, 0, (r), 0, (regp)); \
		*(inst)++ = (unsigned char)(opc1); \
		*(inst)++ = (unsigned char)(opc2); \
		x86_64_regp_emit(inst, (r), (regp)); \
	} while(0)

#define x86_64_p1_xmm2_reg_membase(inst, p1, opc1, opc2, r, basereg, disp) \
	do { \
		*(inst)++ = (unsigned char)(p1); \
		x86_64_rex_emit(inst, 0, (r), 0, (basereg)); \
		*(inst)++ = (unsigned char)(opc1); \
		*(inst)++ = (unsigned char)(opc2); \
		x86_64_membase_emit(inst, (r), (basereg), (disp)); \
	} while(0)

#define x86_64_p1_xmm2_reg_memindex(inst, p1, opc1, opc2, r, basereg, disp, indexreg, shift) \
	do { \
		*(inst)++ = (unsigned char)(p1); \
		x86_64_rex_emit(inst, 0, (r), (indexreg), (basereg)); \
		*(inst)++ = (unsigned char)(opc1); \
		*(inst)++ = (unsigned char)(opc2); \
		x86_64_memindex_emit((inst), (r), (basereg), (disp), (indexreg), (shift)); \
	} while(0)

/*
 * Group1 general instructions
 */
#define x86_64_alu_reg_reg(inst, opc, dreg, sreg) \
	do { \
		x86_64_alu_reg_reg_size((inst), (opc), (dreg), (sreg), 8); \
	} while(0)

#define x86_64_alu_reg_imm(inst, opc, dreg, imm) \
	do { \
		x86_64_alu_reg_imm_size((inst), (opc), (dreg), (imm), 8); \
	} while(0)

/*
 * ADC: Add with carry
 */
#define x86_64_adc_reg_reg_size(inst, dreg, sreg, size) \
	do { \
		x86_64_alu_reg_reg_size((inst), 2, (dreg), (sreg), (size)); \
	} while(0)

#define x86_64_adc_regp_reg_size(inst, dregp, sreg, size) \
	do { \
		x86_64_alu_regp_reg_size((inst), 2, (dregp), (sreg), (size)); \
	} while(0)

#define x86_64_adc_mem_reg_size(inst, mem, sreg, size) \
	do { \
		x86_64_alu_mem_reg_size((inst), 2, (mem), (sreg), (size)); \
	} while(0)

#define x86_64_adc_membase_reg_size(inst, basereg, disp, sreg, size) \
	do { \
		x86_64_alu_membase_reg_size((inst), 2, (basereg), (disp), (sreg), (size)); \
	} while(0)

#define x86_64_adc_memindex_reg_size(inst, basereg, disp, indexreg, shift, sreg, size) \
	do { \
		x86_64_alu_memindex_reg_size((inst), 2, (basereg), (disp), (indexreg), (shift), (sreg), (size)); \
	} while(0)

#define x86_64_adc_reg_regp_size(inst, dreg, sregp, size) \
	do { \
		x86_64_alu_reg_regp_size((inst), 2, (dreg), (sregp), (size)); \
	} while(0)

#define x86_64_adc_reg_mem_size(inst, dreg, mem, size) \
	do { \
		x86_64_alu_reg_mem_size((inst), 2, (dreg), (mem), (size)); \
	} while(0)

#define x86_64_adc_reg_membase_size(inst, dreg, basereg, disp, size) \
	do { \
		x86_64_alu_reg_membase_size((inst), 2, (dreg), (basereg), (disp), (size)); \
	} while(0)

#define x86_64_adc_reg_memindex_size(inst, dreg, basereg, disp, indexreg, shift, size) \
	do { \
		x86_64_alu_reg_memindex_size((inst), 2, (dreg), (basereg), (disp), (indexreg), (shift), (size)); \
	} while(0)

#define x86_64_adc_reg_imm_size(inst, dreg, imm, size) \
	do { \
		x86_64_alu_reg_imm_size((inst), 2, (dreg), (imm), (size)); \
	} while(0)

#define x86_64_adc_regp_imm_size(inst, reg, imm, size) \
	do { \
		x86_64_alu_regp_imm_size((inst), 2, (reg), (imm), (size)); \
	} while(0)

#define x86_64_adc_mem_imm_size(inst, mem, imm, size) \
	do { \
		x86_64_alu_mem_imm_size(inst, 2, mem, imm, size); \
	} while(0)

#define x86_64_adc_membase_imm_size(inst, basereg, disp, imm, size) \
	do { \
		x86_64_alu_membase_imm_size((inst), 2, (basereg), (disp), (imm), (size)); \
	} while(0)

#define x86_64_adc_memindex_imm_size(inst, basereg, disp, indexreg, shift, imm, size) \
	do { \
		x86_64_alu_memindex_imm_size((inst), 2, (basereg), (disp), (indexreg), (shift), (imm), (size)); \
	} while(0)

/*
 * ADD
 */
#define x86_64_add_reg_reg_size(inst, dreg, sreg, size) \
	do { \
		x86_64_alu_reg_reg_size((inst), 0, (dreg), (sreg), (size)); \
	} while(0)

#define x86_64_add_regp_reg_size(inst, dregp, sreg, size) \
	do { \
		x86_64_alu_regp_reg_size((inst), 0, (dregp), (sreg), (size)); \
	} while(0)

#define x86_64_add_mem_reg_size(inst, mem, sreg, size) \
	do { \
		x86_64_alu_mem_reg_size((inst), 0, (mem), (sreg), (size)); \
	} while(0)

#define x86_64_add_membase_reg_size(inst, basereg, disp, sreg, size) \
	do { \
		x86_64_alu_membase_reg_size((inst), 0, (basereg), (disp), (sreg), (size)); \
	} while(0)

#define x86_64_add_memindex_reg_size(inst, basereg, disp, indexreg, shift, sreg, size) \
	do { \
		x86_64_alu_memindex_reg_size((inst), 0, (basereg), (disp), (indexreg), (shift), (sreg), (size)); \
	} while(0)

#define x86_64_add_reg_regp_size(inst, dreg, sregp, size) \
	do { \
		x86_64_alu_reg_regp_size((inst), 0, (dreg), (sregp), (size)); \
	} while(0)

#define x86_64_add_reg_mem_size(inst, dreg, mem, size) \
	do { \
		x86_64_alu_reg_mem_size((inst), 0, (dreg), (mem), (size)); \
	} while(0)

#define x86_64_add_reg_membase_size(inst, dreg, basereg, disp, size) \
	do { \
		x86_64_alu_reg_membase_size((inst), 0, (dreg), (basereg), (disp), (size)); \
	} while(0)

#define x86_64_add_reg_memindex_size(inst, dreg, basereg, disp, indexreg, shift, size) \
	do { \
		x86_64_alu_reg_memindex_size((inst), 0, (dreg), (basereg), (disp), (indexreg), (shift), (size)); \
	} while(0)

#define x86_64_add_reg_imm_size(inst, dreg, imm, size) \
	do { \
		x86_64_alu_reg_imm_size((inst), 0, (dreg), (imm), (size)); \
	} while(0)

#define x86_64_add_regp_imm_size(inst, reg, imm, size) \
	do { \
		x86_64_alu_regp_imm_size((inst), 0, (reg), (imm), (size)); \
	} while(0)

#define x86_64_add_mem_imm_size(inst, mem, imm, size) \
	do { \
		x86_64_alu_mem_imm_size(inst, 0, mem, imm, size); \
	} while(0)

#define x86_64_add_membase_imm_size(inst, basereg, disp, imm, size) \
	do { \
		x86_64_alu_membase_imm_size((inst), 0, (basereg), (disp), (imm), (size)); \
	} while(0)

#define x86_64_add_memindex_imm_size(inst, basereg, disp, indexreg, shift, imm, size) \
	do { \
		x86_64_alu_memindex_imm_size((inst), 0, (basereg), (disp), (indexreg), (shift), (imm), (size)); \
	} while(0)

/*
 * AND
 */
#define x86_64_and_reg_reg_size(inst, dreg, sreg, size) \
	do { \
		x86_64_alu_reg_reg_size((inst), 4, (dreg), (sreg), (size)); \
	} while(0)

#define x86_64_and_regp_reg_size(inst, dregp, sreg, size) \
	do { \
		x86_64_alu_regp_reg_size((inst), 4, (dregp), (sreg), (size)); \
	} while(0)

#define x86_64_and_mem_reg_size(inst, mem, sreg, size) \
	do { \
		x86_64_alu_mem_reg_size((inst), 4, (mem), (sreg), (size)); \
	} while(0)

#define x86_64_and_membase_reg_size(inst, basereg, disp, sreg, size) \
	do { \
		x86_64_alu_membase_reg_size((inst), 4, (basereg), (disp), (sreg), (size)); \
	} while(0)

#define x86_64_and_memindex_reg_size(inst, basereg, disp, indexreg, shift, sreg, size) \
	do { \
		x86_64_alu_memindex_reg_size((inst), 4, (basereg), (disp), (indexreg), (shift), (sreg), (size)); \
	} while(0)

#define x86_64_and_reg_regp_size(inst, dreg, sregp, size) \
	do { \
		x86_64_alu_reg_regp_size((inst), 4, (dreg), (sregp), (size)); \
	} while(0)

#define x86_64_and_reg_mem_size(inst, dreg, mem, size) \
	do { \
		x86_64_alu_reg_mem_size((inst), 4, (dreg), (mem), (size)); \
	} while(0)

#define x86_64_and_reg_membase_size(inst, dreg, basereg, disp, size) \
	do { \
		x86_64_alu_reg_membase_size((inst), 4, (dreg), (basereg), (disp), (size)); \
	} while(0)

#define x86_64_and_reg_memindex_size(inst, dreg, basereg, disp, indexreg, shift, size) \
	do { \
		x86_64_alu_reg_memindex_size((inst), 4, (dreg), (basereg), (disp), (indexreg), (shift), (size)); \
	} while(0)

#define x86_64_and_reg_imm_size(inst, dreg, imm, size) \
	do { \
		x86_64_alu_reg_imm_size((inst), 4, (dreg), (imm), (size)); \
	} while(0)

#define x86_64_and_regp_imm_size(inst, reg, imm, size) \
	do { \
		x86_64_alu_regp_imm_size((inst), 4, (reg), (imm), (size)); \
	} while(0)

#define x86_64_and_mem_imm_size(inst, mem, imm, size) \
	do { \
		x86_64_alu_mem_imm_size(inst, 4, mem, imm, size); \
	} while(0)

#define x86_64_and_membase_imm_size(inst, basereg, disp, imm, size) \
	do { \
		x86_64_alu_membase_imm_size((inst), 4, (basereg), (disp), (imm), (size)); \
	} while(0)

#define x86_64_and_memindex_imm_size(inst, basereg, disp, indexreg, shift, imm, size) \
	do { \
		x86_64_alu_memindex_imm_size((inst), 4, (basereg), (disp), (indexreg), (shift), (imm), (size)); \
	} while(0)

/*
 * CMP: compare
 */
#define x86_64_cmp_reg_reg_size(inst, dreg, sreg, size) \
	do { \
		x86_64_alu_reg_reg_size((inst), 7, (dreg), (sreg), (size)); \
	} while(0)

#define x86_64_cmp_regp_reg_size(inst, dregp, sreg, size) \
	do { \
		x86_64_alu_regp_reg_size((inst), 7, (dregp), (sreg), (size)); \
	} while(0)

#define x86_64_cmp_mem_reg_size(inst, mem, sreg, size) \
	do { \
		x86_64_alu_mem_reg_size((inst), 7, (mem), (sreg), (size)); \
	} while(0)

#define x86_64_cmp_membase_reg_size(inst, basereg, disp, sreg, size) \
	do { \
		x86_64_alu_membase_reg_size((inst), 7, (basereg), (disp), (sreg), (size)); \
	} while(0)

#define x86_64_cmp_memindex_reg_size(inst, basereg, disp, indexreg, shift, sreg, size) \
	do { \
		x86_64_alu_memindex_reg_size((inst), 7, (basereg), (disp), (indexreg), (shift), (sreg), (size)); \
	} while(0)

#define x86_64_cmp_reg_regp_size(inst, dreg, sregp, size) \
	do { \
		x86_64_alu_reg_regp_size((inst), 7, (dreg), (sregp), (size)); \
	} while(0)

#define x86_64_cmp_reg_mem_size(inst, dreg, mem, size) \
	do { \
		x86_64_alu_reg_mem_size((inst), 7, (dreg), (mem), (size)); \
	} while(0)

#define x86_64_cmp_reg_membase_size(inst, dreg, basereg, disp, size) \
	do { \
		x86_64_alu_reg_membase_size((inst), 7, (dreg), (basereg), (disp), (size)); \
	} while(0)

#define x86_64_cmp_reg_memindex_size(inst, dreg, basereg, disp, indexreg, shift, size) \
	do { \
		x86_64_alu_reg_memindex_size((inst), 7, (dreg), (basereg), (disp), (indexreg), (shift), (size)); \
	} while(0)

#define x86_64_cmp_reg_imm_size(inst, dreg, imm, size) \
	do { \
		x86_64_alu_reg_imm_size((inst), 7, (dreg), (imm), (size)); \
	} while(0)

#define x86_64_cmp_regp_imm_size(inst, reg, imm, size) \
	do { \
		x86_64_alu_regp_imm_size((inst), 7, (reg), (imm), (size)); \
	} while(0)

#define x86_64_cmp_mem_imm_size(inst, mem, imm, size) \
	do { \
		x86_64_alu_mem_imm_size(inst, 7, mem, imm, size); \
	} while(0)

#define x86_64_cmp_membase_imm_size(inst, basereg, disp, imm, size) \
	do { \
		x86_64_alu_membase_imm_size((inst), 7, (basereg), (disp), (imm), (size)); \
	} while(0)

#define x86_64_cmp_memindex_imm_size(inst, basereg, disp, indexreg, shift, imm, size) \
	do { \
		x86_64_alu_memindex_imm_size((inst), 7, (basereg), (disp), (indexreg), (shift), (imm), (size)); \
	} while(0)

/*
 * OR
 */
#define x86_64_or_reg_reg_size(inst, dreg, sreg, size) \
	do { \
		x86_64_alu_reg_reg_size((inst), 1, (dreg), (sreg), (size)); \
	} while(0)

#define x86_64_or_regp_reg_size(inst, dregp, sreg, size) \
	do { \
		x86_64_alu_regp_reg_size((inst), 1, (dregp), (sreg), (size)); \
	} while(0)

#define x86_64_or_mem_reg_size(inst, mem, sreg, size) \
	do { \
		x86_64_alu_mem_reg_size((inst), 1, (mem), (sreg), (size)); \
	} while(0)

#define x86_64_or_membase_reg_size(inst, basereg, disp, sreg, size) \
	do { \
		x86_64_alu_membase_reg_size((inst), 1, (basereg), (disp), (sreg), (size)); \
	} while(0)

#define x86_64_or_memindex_reg_size(inst, basereg, disp, indexreg, shift, sreg, size) \
	do { \
		x86_64_alu_memindex_reg_size((inst), 1, (basereg), (disp), (indexreg), (shift), (sreg), (size)); \
	} while(0)

#define x86_64_or_reg_regp_size(inst, dreg, sregp, size) \
	do { \
		x86_64_alu_reg_regp_size((inst), 1, (dreg), (sregp), (size)); \
	} while(0)

#define x86_64_or_reg_mem_size(inst, dreg, mem, size) \
	do { \
		x86_64_alu_reg_mem_size((inst), 1, (dreg), (mem), (size)); \
	} while(0)

#define x86_64_or_reg_membase_size(inst, dreg, basereg, disp, size) \
	do { \
		x86_64_alu_reg_membase_size((inst), 1, (dreg), (basereg), (disp), (size)); \
	} while(0)

#define x86_64_or_reg_memindex_size(inst, dreg, basereg, disp, indexreg, shift, size) \
	do { \
		x86_64_alu_reg_memindex_size((inst), 1, (dreg), (basereg), (disp), (indexreg), (shift), (size)); \
	} while(0)

#define x86_64_or_reg_imm_size(inst, dreg, imm, size) \
	do { \
		x86_64_alu_reg_imm_size((inst), 1, (dreg), (imm), (size)); \
	} while(0)

#define x86_64_or_regp_imm_size(inst, reg, imm, size) \
	do { \
		x86_64_alu_regp_imm_size((inst), 1, (reg), (imm), (size)); \
	} while(0)

#define x86_64_or_mem_imm_size(inst, mem, imm, size) \
	do { \
		x86_64_alu_mem_imm_size(inst, 1, mem, imm, size); \
	} while(0)

#define x86_64_or_membase_imm_size(inst, basereg, disp, imm, size) \
	do { \
		x86_64_alu_membase_imm_size((inst), 1, (basereg), (disp), (imm), (size)); \
	} while(0)

#define x86_64_or_memindex_imm_size(inst, basereg, disp, indexreg, shift, imm, size) \
	do { \
		x86_64_alu_memindex_imm_size((inst), 1, (basereg), (disp), (indexreg), (shift), (imm), (size)); \
	} while(0)

/*
 * SBB: Subtract with borrow from al
 */
#define x86_64_sbb_reg_reg_size(inst, dreg, sreg, size) \
	do { \
		x86_64_alu_reg_reg_size((inst), 3, (dreg), (sreg), (size)); \
	} while(0)

#define x86_64_sbb_regp_reg_size(inst, dregp, sreg, size) \
	do { \
		x86_64_alu_regp_reg_size((inst), 3, (dregp), (sreg), (size)); \
	} while(0)

#define x86_64_sbb_mem_reg_size(inst, mem, sreg, size) \
	do { \
		x86_64_alu_mem_reg_size((inst), 3, (mem), (sreg), (size)); \
	} while(0)

#define x86_64_sbb_membase_reg_size(inst, basereg, disp, sreg, size) \
	do { \
		x86_64_alu_membase_reg_size((inst), 3, (basereg), (disp), (sreg), (size)); \
	} while(0)

#define x86_64_sbb_memindex_reg_size(inst, basereg, disp, indexreg, shift, sreg, size) \
	do { \
		x86_64_alu_memindex_reg_size((inst), 3, (basereg), (disp), (indexreg), (shift), (sreg), (size)); \
	} while(0)

#define x86_64_sbb_reg_regp_size(inst, dreg, sregp, size) \
	do { \
		x86_64_alu_reg_regp_size((inst), 3, (dreg), (sregp), (size)); \
	} while(0)

#define x86_64_sbb_reg_mem_size(inst, dreg, mem, size) \
	do { \
		x86_64_alu_reg_mem_size((inst), 3, (dreg), (mem), (size)); \
	} while(0)

#define x86_64_sbb_reg_membase_size(inst, dreg, basereg, disp, size) \
	do { \
		x86_64_alu_reg_membase_size((inst), 3, (dreg), (basereg), (disp), (size)); \
	} while(0)

#define x86_64_sbb_reg_memindex_size(inst, dreg, basereg, disp, indexreg, shift, size) \
	do { \
		x86_64_alu_reg_memindex_size((inst), 3, (dreg), (basereg), (disp), (indexreg), (shift), (size)); \
	} while(0)

#define x86_64_sbb_reg_imm_size(inst, dreg, imm, size) \
	do { \
		x86_64_alu_reg_imm_size((inst), 3, (dreg), (imm), (size)); \
	} while(0)

#define x86_64_sbb_regp_imm_size(inst, reg, imm, size) \
	do { \
		x86_64_alu_regp_imm_size((inst), 3, (reg), (imm), (size)); \
	} while(0)

#define x86_64_sbb_mem_imm_size(inst, mem, imm, size) \
	do { \
		x86_64_alu_mem_imm_size(inst, 3, mem, imm, size); \
	} while(0)

#define x86_64_sbb_membase_imm_size(inst, basereg, disp, imm, size) \
	do { \
		x86_64_alu_membase_imm_size((inst), 3, (basereg), (disp), (imm), (size)); \
	} while(0)

#define x86_64_sbb_memindex_imm_size(inst, basereg, disp, indexreg, shift, imm, size) \
	do { \
		x86_64_alu_memindex_imm_size((inst), 3, (basereg), (disp), (indexreg), (shift), (imm), (size)); \
	} while(0)

/*
 * SUB: Subtract
 */
#define x86_64_sub_reg_reg_size(inst, dreg, sreg, size) \
	do { \
		x86_64_alu_reg_reg_size((inst), 5, (dreg), (sreg), (size)); \
	} while(0)

#define x86_64_sub_regp_reg_size(inst, dregp, sreg, size) \
	do { \
		x86_64_alu_regp_reg_size((inst), 5, (dregp), (sreg), (size)); \
	} while(0)

#define x86_64_sub_mem_reg_size(inst, mem, sreg, size) \
	do { \
		x86_64_alu_mem_reg_size((inst), 5, (mem), (sreg), (size)); \
	} while(0)

#define x86_64_sub_membase_reg_size(inst, basereg, disp, sreg, size) \
	do { \
		x86_64_alu_membase_reg_size((inst), 5, (basereg), (disp), (sreg), (size)); \
	} while(0)

#define x86_64_sub_memindex_reg_size(inst, basereg, disp, indexreg, shift, sreg, size) \
	do { \
		x86_64_alu_memindex_reg_size((inst), 5, (basereg), (disp), (indexreg), (shift), (sreg), (size)); \
	} while(0)

#define x86_64_sub_reg_regp_size(inst, dreg, sregp, size) \
	do { \
		x86_64_alu_reg_regp_size((inst), 5, (dreg), (sregp), (size)); \
	} while(0)

#define x86_64_sub_reg_mem_size(inst, dreg, mem, size) \
	do { \
		x86_64_alu_reg_mem_size((inst), 5, (dreg), (mem), (size)); \
	} while(0)

#define x86_64_sub_reg_membase_size(inst, dreg, basereg, disp, size) \
	do { \
		x86_64_alu_reg_membase_size((inst), 5, (dreg), (basereg), (disp), (size)); \
	} while(0)

#define x86_64_sub_reg_memindex_size(inst, dreg, basereg, disp, indexreg, shift, size) \
	do { \
		x86_64_alu_reg_memindex_size((inst), 5, (dreg), (basereg), (disp), (indexreg), (shift), (size)); \
	} while(0)

#define x86_64_sub_reg_imm_size(inst, dreg, imm, size) \
	do { \
		x86_64_alu_reg_imm_size((inst), 5, (dreg), (imm), (size)); \
	} while(0)

#define x86_64_sub_regp_imm_size(inst, reg, imm, size) \
	do { \
		x86_64_alu_regp_imm_size((inst), 5, (reg), (imm), (size)); \
	} while(0)

#define x86_64_sub_mem_imm_size(inst, mem, imm, size) \
	do { \
		x86_64_alu_mem_imm_size(inst, 5, mem, imm, size); \
	} while(0)

#define x86_64_sub_membase_imm_size(inst, basereg, disp, imm, size) \
	do { \
		x86_64_alu_membase_imm_size((inst), 5, (basereg), (disp), (imm), (size)); \
	} while(0)

#define x86_64_sub_memindex_imm_size(inst, basereg, disp, indexreg, shift, imm, size) \
	do { \
		x86_64_alu_memindex_imm_size((inst), 5, (basereg), (disp), (indexreg), (shift), (imm), (size)); \
	} while(0)

/*
 * XOR
 */
#define x86_64_xor_reg_reg_size(inst, dreg, sreg, size) \
	do { \
		x86_64_alu_reg_reg_size((inst), 6, (dreg), (sreg), (size)); \
	} while(0)

#define x86_64_xor_regp_reg_size(inst, dregp, sreg, size) \
	do { \
		x86_64_alu_regp_reg_size((inst), 6, (dregp), (sreg), (size)); \
	} while(0)

#define x86_64_xor_mem_reg_size(inst, mem, sreg, size) \
	do { \
		x86_64_alu_mem_reg_size((inst), 6, (mem), (sreg), (size)); \
	} while(0)

#define x86_64_xor_membase_reg_size(inst, basereg, disp, sreg, size) \
	do { \
		x86_64_alu_membase_reg_size((inst), 6, (basereg), (disp), (sreg), (size)); \
	} while(0)

#define x86_64_xor_memindex_reg_size(inst, basereg, disp, indexreg, shift, sreg, size) \
	do { \
		x86_64_alu_memindex_reg_size((inst), 6, (basereg), (disp), (indexreg), (shift), (sreg), (size)); \
	} while(0)

#define x86_64_xor_reg_regp_size(inst, dreg, sregp, size) \
	do { \
		x86_64_alu_reg_regp_size((inst), 6, (dreg), (sregp), (size)); \
	} while(0)

#define x86_64_xor_reg_mem_size(inst, dreg, mem, size) \
	do { \
		x86_64_alu_reg_mem_size((inst), 6, (dreg), (mem), (size)); \
	} while(0)

#define x86_64_xor_reg_membase_size(inst, dreg, basereg, disp, size) \
	do { \
		x86_64_alu_reg_membase_size((inst), 6, (dreg), (basereg), (disp), (size)); \
	} while(0)

#define x86_64_xor_reg_memindex_size(inst, dreg, basereg, disp, indexreg, shift, size) \
	do { \
		x86_64_alu_reg_memindex_size((inst), 6, (dreg), (basereg), (disp), (indexreg), (shift), (size)); \
	} while(0)

#define x86_64_xor_reg_imm_size(inst, dreg, imm, size) \
	do { \
		x86_64_alu_reg_imm_size((inst), 6, (dreg), (imm), (size)); \
	} while(0)

#define x86_64_xor_regp_imm_size(inst, reg, imm, size) \
	do { \
		x86_64_alu_regp_imm_size((inst), 6, (reg), (imm), (size)); \
	} while(0)

#define x86_64_xor_mem_imm_size(inst, mem, imm, size) \
	do { \
		x86_64_alu_mem_imm_size(inst, 6, mem, imm, size); \
	} while(0)

#define x86_64_xor_membase_imm_size(inst, basereg, disp, imm, size) \
	do { \
		x86_64_alu_membase_imm_size((inst), 6, (basereg), (disp), (imm), (size)); \
	} while(0)

#define x86_64_xor_memindex_imm_size(inst, basereg, disp, indexreg, shift, imm, size) \
	do { \
		x86_64_alu_memindex_imm_size((inst), 6, (basereg), (disp), (indexreg), (shift), (imm), (size)); \
	} while(0)

/*
 * dec
 */
#define x86_64_dec_reg_size(inst, reg, size) \
	do { \
		x86_64_alu1_reg_size((inst), 0xfe, 1, (reg), (size)); \
	} while(0)

#define x86_64_dec_regp_size(inst, regp, size) \
	do { \
		x86_64_alu1_regp_size((inst), 0xfe, 1, (regp), (size)); \
	} while(0)

#define x86_64_dec_mem_size(inst, mem, size) \
	do { \
		x86_64_alu1_mem_size((inst), 0xfe, 1, (mem), (size)); \
	} while(0)

#define x86_64_dec_membase_size(inst, basereg, disp, size) \
	do { \
		x86_64_alu1_membase_size((inst), 0xfe, 1, (basereg), (disp), (size)); \
	} while(0)

#define x86_64_dec_memindex_size(inst, basereg, disp, indexreg, shift, size) \
	do { \
		x86_64_alu1_memindex_size((inst), 0xfe, 1, (basereg), (disp), (indexreg), (shift), (size)); \
	} while(0)

/*
 * div: unsigned division RDX:RAX / operand
 */
#define x86_64_div_reg_size(inst, reg, size) \
	do { \
		x86_64_alu1_reg_size((inst), 0xf6, 6, (reg), (size)); \
	} while(0)

#define x86_64_div_regp_size(inst, regp, size) \
	do { \
		x86_64_alu1_regp_size((inst), 0xf6, 6, (regp), (size)); \
	} while(0)

#define x86_64_div_mem_size(inst, mem, size) \
	do { \
		x86_64_alu1_mem_size((inst), 0xf6, 6, (mem), (size)); \
	} while(0)

#define x86_64_div_membase_size(inst, basereg, disp, size) \
	do { \
		x86_64_alu1_membase_size((inst), 0xf6, 6, (basereg), (disp), (size)); \
	} while(0)

#define x86_64_div_memindex_size(inst, basereg, disp, indexreg, shift, size) \
	do { \
		x86_64_alu1_memindex_size((inst), 0xf6, 6, (basereg), (disp), (indexreg), (shift), (size)); \
	} while(0)

/*
 * idiv: signed division RDX:RAX / operand
 */
#define x86_64_idiv_reg_size(inst, reg, size) \
	do { \
		x86_64_alu1_reg_size((inst), 0xf6, 7, (reg), (size)); \
	} while(0)

#define x86_64_idiv_regp_size(inst, regp, size) \
	do { \
		x86_64_alu1_regp_size((inst), 0xf6, 7, (regp), (size)); \
	} while(0)

#define x86_64_idiv_mem_size(inst, mem, size) \
	do { \
		x86_64_alu1_mem_size((inst), 0xf6, 7, (mem), (size)); \
	} while(0)

#define x86_64_idiv_membase_size(inst, basereg, disp, size) \
	do { \
		x86_64_alu1_membase_size((inst), 0xf6, 7, (basereg), (disp), (size)); \
	} while(0)

#define x86_64_idiv_memindex_size(inst, basereg, disp, indexreg, shift, size) \
	do { \
		x86_64_alu1_memindex_size((inst), 0xf6, 7, (basereg), (disp), (indexreg), (shift), (size)); \
	} while(0)

/*
 * inc
 */
#define x86_64_inc_reg_size(inst, reg, size) \
	do { \
		x86_64_alu1_reg_size((inst), 0xfe, 0, (reg), (size)); \
	} while(0)

#define x86_64_inc_regp_size(inst, regp, size) \
	do { \
		x86_64_alu1_regp_size((inst), 0xfe, 0, (regp), (size)); \
	} while(0)

#define x86_64_inc_mem_size(inst, mem, size) \
	do { \
		x86_64_alu1_mem_size((inst), 0xfe, 0, (mem), (size)); \
	} while(0)

#define x86_64_inc_membase_size(inst, basereg, disp, size) \
	do { \
		x86_64_alu1_membase_size((inst), 0xfe, 0, (basereg), (disp), (size)); \
	} while(0)

#define x86_64_inc_memindex_size(inst, basereg, disp, indexreg, shift, size) \
	do { \
		x86_64_alu1_memindex_size((inst), 0xfe, 0, (basereg), (disp), (indexreg), (shift), (size)); \
	} while(0)

/*
 * mul: multiply RDX:RAX = RAX * operand
 * is_signed == 0 -> unsigned multiplication
 * signed multiplication otherwise.
 */
#define x86_64_mul_reg_issigned_size(inst, reg, is_signed, size) \
	do { \
		x86_64_alu1_reg_size((inst), 0xf6, ((is_signed) ? 5 : 4), (reg), (size)); \
	} while(0)

#define x86_64_mul_regp_issigned_size(inst, regp, is_signed, size) \
	do { \
		x86_64_alu1_regp_size((inst), 0xf6, ((is_signed) ? 5 : 4), (regp), (size)); \
	} while(0)

#define x86_64_mul_mem_issigned_size(inst, mem, is_signed, size) \
	do { \
		x86_64_alu1_mem_size((inst), 0xf6, ((is_signed) ? 5 : 4), (mem), (size)); \
	} while(0)

#define x86_64_mul_membase_issigned_size(inst, basereg, disp, is_signed, size) \
	do { \
		x86_64_alu1_membase_size((inst), 0xf6, ((is_signed) ? 5 : 4), (basereg), (disp), (size)); \
	} while(0)

#define x86_64_mul_memindex_issigned_size(inst, basereg, disp, indexreg, shift, is_signed, size) \
	do { \
		x86_64_alu1_memindex_size((inst), 0xf6, ((is_signed) ? 5 : 4), (basereg), (disp), (indexreg), (shift), (size)); \
	} while(0)

/*
 * neg 
 */
#define x86_64_neg_reg_size(inst, reg, size) \
	do { \
		x86_64_alu1_reg_size((inst), 0xf6, 3, (reg), (size)); \
	} while(0)

#define x86_64_neg_regp_size(inst, regp, size) \
	do { \
		x86_64_alu1_regp_size((inst), 0xf6, 3, (regp), (size)); \
	} while(0)

#define x86_64_neg_mem_size(inst, mem, size) \
	do { \
		x86_64_alu1_mem_size((inst), 0xf6, 3, (mem), (size)); \
	} while(0)

#define x86_64_neg_membase_size(inst, basereg, disp, size) \
	do { \
		x86_64_alu1_membase_size((inst), 0xf6, 3, (basereg), (disp), (size)); \
	} while(0)

#define x86_64_neg_memindex_size(inst, basereg, disp, indexreg, shift, size) \
	do { \
		x86_64_alu1_memindex_size((inst), 0xf6, 3, (basereg), (disp), (indexreg), (shift), (size)); \
	} while(0)

/*
 * not
 */
#define x86_64_not_reg_size(inst, reg, size) \
	do { \
		x86_64_alu1_reg_size((inst), 0xf6, 2, (reg), (size)); \
	} while(0)

#define x86_64_not_regp_size(inst, regp, size) \
	do { \
		x86_64_alu1_regp_size((inst), 0xf6, 2, (regp), (size)); \
	} while(0)

#define x86_64_not_mem_size(inst, mem, size) \
	do { \
		x86_64_alu1_mem_size((inst), 0xf6, 2, (mem), (size)); \
	} while(0)

#define x86_64_not_membase_size(inst, basereg, disp, size) \
	do { \
		x86_64_alu1_membase_size((inst), 0xf6, 2, (basereg), (disp), (size)); \
	} while(0)

#define x86_64_not_memindex_size(inst, basereg, disp, indexreg, shift, size) \
	do { \
		x86_64_alu1_memindex_size((inst), 0xf6, 2, (basereg), (disp), (indexreg), (shift), (size)); \
	} while(0)

/*
 * Lea instructions
 */
#define x86_64_lea_mem_size(inst, dreg, mem, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit((inst), (size), 0, 0, (dreg)); \
		x86_lea_mem((inst), ((dreg) & 0x7), (mem)); \
	} while(0)

#define x86_64_lea_membase_size(inst, dreg, basereg, disp, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit(inst, (size), (dreg), 0, (basereg)); \
		*(inst)++ = (unsigned char)0x8d;	\
		x86_64_membase_emit((inst), (dreg), (basereg), (disp));	\
	} while (0)

#define x86_64_lea_memindex_size(inst, dreg, basereg, disp, indexreg, shift, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit((inst), (size), (dreg), (indexreg), (basereg)); \
		*(inst)++ = (unsigned char)0x8d; \
		x86_64_memindex_emit ((inst), (dreg), (basereg), (disp), (indexreg), (shift)); \
	} while(0)

/*
 * Move instructions.
 */
#define x86_64_mov_reg_reg_size(inst, dreg, sreg, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit(inst, (size), (dreg), 0, (sreg)); \
		x86_64_opcode1_emit(inst, 0x8a, (size)); \
		x86_64_reg_emit((inst), ((dreg) & 0x7), ((sreg) & 0x7)); \
	} while(0)

#define x86_64_mov_regp_reg_size(inst, regp, sreg, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit(inst, (size), (sreg), 0, (regp)); \
		x86_64_opcode1_emit(inst, 0x88, (size)); \
		x86_64_regp_emit((inst), (sreg), (regp)); \
	} while (0)

#define x86_64_mov_membase_reg_size(inst, basereg, disp, sreg, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit(inst, (size), (sreg), 0, (basereg)); \
		x86_64_opcode1_emit(inst, 0x88, (size)); \
		x86_64_membase_emit((inst), (sreg), (basereg), (disp));	\
	} while(0)

#define x86_64_mov_memindex_reg_size(inst, basereg, disp, indexreg, shift, sreg, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit((inst), (size), (sreg), (indexreg), (basereg)); \
		x86_64_opcode1_emit(inst, 0x88, (size)); \
		x86_64_memindex_emit((inst), (sreg), (basereg), (disp), (indexreg), (shift)); \
	} while (0)

/*
 * Using the AX register is the only possibility to address 64bit.
 * All other registers are bound to 32bit values.
 */
#define x86_64_mov_mem_reg_size(inst, mem, sreg, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit(inst, (size), (sreg), 0, 0); \
		if((sreg) == X86_64_RAX) \
		{ \
			x86_64_opcode1_emit(inst, 0xa2, (size)); \
			x86_64_imm_emit64(inst, (mem)); \
		} \
		else \
		{ \
			x86_64_opcode1_emit(inst, 0x88, (size)); \
			x86_address_byte((inst), 0, ((sreg) & 0x7), 4); \
			x86_address_byte((inst), 0, 4, 5); \
	        x86_imm_emit32((inst), (mem)); \
		} \
	} while (0)

#define x86_64_mov_reg_imm_size(inst, dreg, imm, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit(inst, (size), 0, 0, (dreg)); \
		if((size) == 1) \
		{ \
			*(inst)++ = (unsigned char)0xb0 + ((dreg) & 0x7); \
		} \
		else \
		{ \
			*(inst)++ = (unsigned char)0xb8 + ((dreg) & 0x7); \
		} \
		x86_64_imm_emit_max64(inst, (imm), (size)); \
	} while(0)

/*
 * Using the AX register is the only possibility to address 64bit.
 * All other registers are bound to 32bit values.
 */
#define x86_64_mov_reg_mem_size(inst, dreg, mem, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit(inst, (size), (dreg), 0, 0); \
		if((dreg) == X86_64_RAX) \
		{ \
			x86_64_opcode1_emit(inst, 0xa0, (size)); \
			x86_64_imm_emit64(inst, (mem)); \
		} \
		else \
		{ \
			x86_64_opcode1_emit(inst, 0x8a, (size)); \
			x86_address_byte ((inst), 0, (dreg), 4); \
			x86_address_byte ((inst), 0, 4, 5); \
			x86_imm_emit32 ((inst), (mem)); \
		} \
	} while (0)

#define x86_64_mov_reg_membase_size(inst, dreg, basereg, disp, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit(inst, (size), (dreg), 0, (basereg)); \
		x86_64_opcode1_emit(inst, 0x8a, (size)); \
		x86_64_membase_emit((inst), (dreg), (basereg), (disp)); \
	} while(0)


#define x86_64_mov_reg_memindex_size(inst, dreg, basereg, disp, indexreg, shift, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit((inst), (size), (dreg), (indexreg), (basereg)); \
		x86_64_opcode1_emit(inst, 0x8a, (size)); \
		x86_64_memindex_emit((inst), (dreg), (basereg), (disp), (indexreg), (shift)); \
	} while(0)

/*
 * Only 32bit mem and imm values are allowed here.
 * mem is be RIP relative.
 * 32 bit imm will be sign extended to 64 bits for 64 bit size.
 */
#define x86_64_mov_mem_imm_size(inst, mem, imm, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit((inst), (size), 0, 0, 0); \
		x86_64_opcode1_emit(inst, 0xc6, (size)); \
		x86_64_mem_emit((inst), 0, (mem)); \
		x86_64_imm_emit_max32(inst, (imm), (size)); \
	} while(0)

#define x86_64_mov_membase_imm_size(inst, basereg, disp, imm, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit(inst, (size), 0, 0, (basereg)); \
		x86_64_opcode1_emit(inst, 0xc6, (size)); \
		x86_64_membase_emit((inst), 0, (basereg), (disp)); \
		x86_64_imm_emit_max32(inst, (imm), (size)); \
	} while(0)

#define x86_64_mov_memindex_imm_size(inst, basereg, disp, indexreg, shift, imm, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit((inst), (size), 0, (indexreg), (basereg)); \
		x86_64_opcode1_emit(inst, 0xc6, (size)); \
		x86_64_memindex_emit((inst), 0, (basereg), (disp), (indexreg), (shift)); \
		x86_64_imm_emit_max32(inst, (imm), (size)); \
	} while(0)

/*
 * Move with sign extension to the given size (unsigned)
 */
#define x86_64_movsx8_reg_reg_size(inst, dreg, sreg, size) \
	do { \
		x86_64_alu2_reg_reg_size((inst), 0x0f, 0xbe, (dreg), (sreg), (size)); \
	}while(0)

#define x86_64_movsx8_reg_regp_size(inst, dreg, sregp, size) \
	do { \
		x86_64_alu2_reg_regp_size((inst), 0x0f, 0xbe, (dreg), (sregp), (size)); \
	}while(0)

#define x86_64_movsx8_reg_mem_size(inst, dreg, mem, size) \
	do { \
		x86_64_alu2_reg_mem_size((inst), 0x0f, 0xbe, (dreg), (mem), (size)); \
	}while(0)

#define x86_64_movsx8_reg_membase_size(inst, dreg, basereg, disp, size) \
	do { \
		x86_64_alu2_reg_membase_size((inst), 0x0f, 0xbe, (dreg), (basereg), (disp), (size)); \
	}while(0)

#define x86_64_movsx8_reg_memindex_size(inst, dreg, basereg, disp, indexreg, shift, size) \
	do { \
		x86_64_alu2_reg_memindex_size((inst), 0x0f, 0xbe, (dreg), (basereg), (disp), (indexreg), (shift), (size)); \
	}while(0)

#define x86_64_movsx16_reg_reg_size(inst, dreg, sreg, size) \
	do { \
		x86_64_alu2_reg_reg_size((inst), 0x0f, 0xbf, (dreg), (sreg), (size)); \
	}while(0)

#define x86_64_movsx16_reg_regp_size(inst, dreg, sregp, size) \
	do { \
		x86_64_alu2_reg_regp_size((inst), 0x0f, 0xbf, (dreg), (sregp), (size)); \
	}while(0)

#define x86_64_movsx16_reg_mem_size(inst, dreg, mem, size) \
	do { \
		x86_64_alu2_reg_mem_size((inst), 0x0f, 0xbf, (dreg), (mem), (size)); \
	}while(0)

#define x86_64_movsx16_reg_membase_size(inst, dreg, basereg, disp, size) \
	do { \
		x86_64_alu2_reg_membase_size((inst), 0x0f, 0xbf, (dreg), (basereg), (disp), (size)); \
	}while(0)

#define x86_64_movsx16_reg_memindex_size(inst, dreg, basereg, disp, indexreg, shift, size) \
	do { \
		x86_64_alu2_reg_memindex_size((inst), 0x0f, 0xbf, (dreg), (basereg), (disp), (indexreg), (shift), (size)); \
	}while(0)

/*
 * Move with zero extension to the given size (unsigned)
 */
#define x86_64_movzx8_reg_reg_size(inst, dreg, sreg, size) \
	do { \
		x86_64_alu2_reg_reg_size((inst), 0x0f, 0xb6, (dreg), (sreg), (size)); \
	}while(0)

#define x86_64_movzx8_reg_regp_size(inst, dreg, sregp, size) \
	do { \
		x86_64_alu2_reg_regp_size((inst), 0x0f, 0xb6, (dreg), (sregp), (size)); \
	}while(0)

#define x86_64_movzx8_reg_mem_size(inst, dreg, mem, size) \
	do { \
		x86_64_alu2_reg_mem_size((inst), 0x0f, 0xb6, (dreg), (mem), (size)); \
	}while(0)

#define x86_64_movzx8_reg_membase_size(inst, dreg, basereg, disp, size) \
	do { \
		x86_64_alu2_reg_membase_size((inst), 0x0f, 0xb6, (dreg), (basereg), (disp), (size)); \
	}while(0)

#define x86_64_movzx8_reg_memindex_size(inst, dreg, basereg, disp, indexreg, shift, size) \
	do { \
		x86_64_alu2_reg_memindex_size((inst), 0x0f, 0xb6, (dreg), (basereg), (disp), (indexreg), (shift), (size)); \
	}while(0)

#define x86_64_movzx16_reg_reg_size(inst, dreg, sreg, size) \
	do { \
		x86_64_alu2_reg_reg_size((inst), 0x0f, 0xb7, (dreg), (sreg), (size)); \
	}while(0)

#define x86_64_movzx16_reg_regp_size(inst, dreg, sregp, size) \
	do { \
		x86_64_alu2_reg_regp_size((inst), 0x0f, 0xb7, (dreg), (sregp), (size)); \
	}while(0)

#define x86_64_movzx16_reg_mem_size(inst, dreg, mem, size) \
	do { \
		x86_64_alu2_reg_mem_size((inst), 0x0f, 0xb7, (dreg), (mem), (size)); \
	}while(0)

#define x86_64_movzx16_reg_membase_size(inst, dreg, basereg, disp, size) \
	do { \
		x86_64_alu2_reg_membase_size((inst), 0x0f, 0xb7, (dreg), (basereg), (disp), (size)); \
	}while(0)

#define x86_64_movzx16_reg_memindex_size(inst, dreg, basereg, disp, indexreg, shift, size) \
	do { \
		x86_64_alu2_reg_memindex_size((inst), 0x0f, 0xb7, (dreg), (basereg), (disp), (indexreg), (shift), (size)); \
	}while(0)

/*
 * Stack manupulation instructions (push and pop)
 */

/*
 * Push instructions have a default size of 64 bit. mode.
 * There is no way to encode a 32 bit push.
 * So only the sizes 8 and 2 are allowed in 64 bit mode.
 */
#define x86_64_push_reg_size(inst, reg, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit64((inst), (size), 0, 0, (reg)); \
		*(inst)++ = (unsigned char)0x50 + ((reg) & 0x7); \
	} while(0)

#define x86_64_push_regp_size(inst, sregp, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit64((inst), (size), 0, 0, (sregp)); \
		*(inst)++ = (unsigned char)0xff; \
		x86_64_regp_emit((inst), 6, (sregp)); \
	} while(0)

#define x86_64_push_mem_size(inst, mem, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit64((inst), (size), 0, 0, 0); \
		*(inst)++ = (unsigned char)0xff; \
		x86_64_mem_emit((inst), 6, (mem)); \
	} while(0)

#define x86_64_push_membase_size(inst, basereg, disp, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit64((inst), (size), 0, 0, (basereg)); \
		*(inst)++ = (unsigned char)0xff; \
		x86_64_membase_emit((inst), 6, (basereg), (disp)); \
	} while(0)

#define x86_64_push_memindex_size(inst, basereg, disp, indexreg, shift, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit64((inst), (size), 0, (indexreg), (basereg)); \
		*(inst)++ = (unsigned char)0xff; \
		x86_64_memindex_emit((inst), 6, (basereg), (disp), (indexreg), (shift)); \
	} while(0)

/*
 * We can push only 32 bit immediate values.
 * The value is sign extended to 64 bit on the stack.
 */
#define x86_64_push_imm(inst, imm) \
	do { \
		x86_push_imm((inst), (imm)); \
	} while(0)

/*
 * Pop instructions have a default size of 64 bit in 64 bit mode.
 * There is no way to encode a 32 bit pop.
 * So only the sizes 2 and 8 are allowed.
 */
#define x86_64_pop_reg_size(inst, dreg, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit64((inst), 0, 0, 0, (dreg)); \
		*(inst)++ = (unsigned char)0x58 + ((dreg) & 0x7); \
	} while(0)

#define x86_64_pop_regp_size(inst, dregp, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit64((inst), (size), 0, 0, (dregp)); \
		*(inst)++ = (unsigned char)0x8f; \
		x86_64_regp_emit((inst), 0, (dregp)); \
	} while(0)

#define x86_64_pop_mem_size(inst, mem, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		*(inst)++ = (unsigned char)0x8f; \
		x86_64_mem_emit((inst), 0, (mem)); \
	} while(0)

#define x86_64_pop_membase_size(inst, basereg, disp, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit64((inst), (size), 0, 0,(basereg)); \
		*(inst)++ = (unsigned char)0x8f; \
		x86_64_membase_emit((inst), 0, (basereg), (disp)); \
	} while(0)

#define x86_64_pop_memindex_size(inst, basereg, disp, indexreg, shift, size) \
	do { \
		if((size) == 2) \
		{ \
			*(inst)++ = (unsigned char)0x66; \
		} \
		x86_64_rex_emit64((inst), (size), 0, (indexreg), (basereg)); \
		*(inst)++ = (unsigned char)0x8f; \
		x86_64_memindex_emit((inst), 0, (basereg), (disp), (indexreg), (shift)); \
	} while(0)

/*
 * control flow change instructions
 */

/*
 * call
 */

/*
 * call_imm is a relative call.
 * imm has to be a 32bit offset from the instruction following the
 * call instruction (absolute - (inst + 5)).
 * For offsets greater that 32bit an indirect call (via register)
 * has to be used.
 */
#define x86_64_call_imm(inst, imm) \
	do { \
		x86_call_imm((inst), (imm)); \
	} while(0)

#define x86_64_call_reg(inst, reg) \
	do { \
		x86_64_alu1_reg((inst), 0xff, 2, (reg)); \
	} while(0)

#define x86_64_call_regp(inst, regp) \
	do { \
		x86_64_alu1_regp((inst), 0xff, 2, (regp)); \
	} while(0)

/*
 * call_mem is a absolute indirect call.
 * To be able to use this instruction the address must be either
 * in the lowest 2GB or in the highest 2GB addressrange.
 * This is because mem is sign extended to 64bit.
 */
#define x86_64_call_mem(inst, mem) \
	do { \
		x86_64_alu1_mem((inst), 0xff, 2, (mem)); \
	} while(0)

#define x86_64_call_membase(inst, basereg, disp) \
	do { \
		x86_64_alu1_membase((inst), 0xff, 2, (basereg), (disp)); \
	} while(0)
	
#define x86_64_call_memindex(inst, basereg, disp, indexreg, shift) \
	do { \
		x86_64_alu1_memindex((inst), 0xff, 2, (basereg), (disp), (indexreg), (shift)); \
	} while(0)

/*
 * jmp
 */

/*
 * unconditional relative jumps
 */
#define x86_64_jmp_imm8(inst, disp) \
	do { \
		*(inst)++ = (unsigned char)0xEB; \
		x86_imm_emit8((inst), (disp)); \
	} while(0)

#define x86_64_jmp_imm(inst, disp) \
	do { \
		*(inst)++ = (unsigned char)0xE9; \
		x86_imm_emit32((inst), (disp)); \
	} while(0)

/*
 * unconditional indirect jumps
 */
#define x86_64_jmp_reg(inst, reg) \
	do { \
		x86_64_alu1_reg((inst), 0xff, 4, (reg)); \
	} while(0)

#define x86_64_jmp_regp(inst, regp) \
	do { \
		x86_64_alu1_regp((inst), 0xff, 4, (regp)); \
	} while(0)

#define x86_64_jmp_mem(inst, mem) \
	do { \
		x86_64_alu1_mem((inst), 0xff, 4, (mem)); \
	} while(0)

#define x86_64_jmp_membase(inst, basereg, disp) \
	do { \
		x86_64_alu1_membase((inst), 0xff, 4, (basereg), (disp)); \
	} while(0)
	
#define x86_64_jmp_memindex(inst, basereg, disp, indexreg, shift) \
	do { \
		x86_64_alu1_memindex((inst), 0xff, 4, (basereg), (disp), (indexreg), (shift)); \
	} while(0)

/*
 * ret
 */
#define x86_64_ret(inst) \
	do { \
		x86_ret((inst)); \
	} while(0)

/*
 * XMM instructions
 */

/*
 * movaps
 */
#define x86_64_movaps_reg_reg(inst, dreg, sreg) \
	do { \
		x86_64_xmm2_reg_reg((inst), 0x0f, 0x28, (dreg), (sreg)); \
	} while(0)

#define x86_64_movaps_membase_reg(inst, basereg, disp, sreg) \
	do { \
		x86_64_xmm2_reg_membase((inst), 0x0f, 0x29, (sreg), (basereg), (disp)); \
	} while(0)

#define x86_64_movaps_memindex_reg(inst, basereg, disp, indexreg, shift, sreg) \
	do { \
		x86_64_xmm2_reg_memindex((inst), 0x0f, 0x29, (sreg), (basereg), (disp), (indexreg), (shift)); \
	} while(0)

#define x86_64_movaps_regp_reg(inst, dregp, sreg) \
	do { \
		x86_64_xmm2_reg_regp((inst), 0x0f, 0x29, (sreg), (dregp)); \
	} while(0)

#define x86_64_movaps_reg_regp(inst, dreg, sregp) \
	do { \
		x86_64_xmm2_reg_regp((inst), 0x0f, 0x28, (dreg), (sregp)); \
	} while(0)

#define x86_64_movaps_reg_membase(inst, dreg, basereg, disp) \
	do { \
		x86_64_xmm2_reg_membase((inst), 0x0f, 0x28, (dreg), (basereg), (disp)); \
	} while(0)

#define x86_64_movaps_reg_memindex(inst, dreg, basereg, disp, indexreg, shift) \
	do { \
		x86_64_xmm2_reg_memindex((inst), 0x0f, 0x28, (dreg), (basereg), (disp), (indexreg), (shift)); \
	} while(0)

/*
 * movsd
 */
#define x86_64_movsd_reg_reg(inst, dreg, sreg) \
	do { \
		x86_64_p1_xmm2_reg_reg((inst), 0xf2, 0x0f, 0x10, (dreg), (sreg)); \
	} while(0)

#define x86_64_movsd_membase_reg(inst, basereg, disp, sreg) \
	do { \
		x86_64_p1_xmm2_reg_membase((inst), 0xf2, 0x0f, 0x11, (sreg), (basereg), (disp)); \
	} while(0)

#define x86_64_movsd_memindex_reg(inst, basereg, disp, indexreg, shift, sreg) \
	do { \
		x86_64_p1_xmm2_reg_memindex((inst), 0xf2, 0x0f, 0x11, (sreg), (basereg), (disp), (indexreg), (shift)); \
	} while(0)

#define x86_64_movsd_regp_reg(inst, dregp, sreg) \
	do { \
		x86_64_p1_xmm2_reg_regp((inst), 0xf2, 0x0f, 0x11, (sreg), (dregp)); \
	} while(0)

#define x86_64_movsd_reg_regp(inst, dreg, sregp) \
	do { \
		x86_64_p1_xmm2_reg_regp((inst), 0xf2, 0x0f, 0x10, (dreg), (sregp)); \
	} while(0)

#define x86_64_movsd_reg_membase(inst, dreg, basereg, disp) \
	do { \
		x86_64_p1_xmm2_reg_membase((inst), 0xf2, 0x0f, 0x10, (dreg), (basereg), (disp)); \
	} while(0)

#define x86_64_movsd_reg_memindex(inst, dreg, basereg, disp, indexreg, shift) \
	do { \
		x86_64_p1_xmm2_reg_memindex((inst), 0xf2, 0x0f, 0x10, (dreg), (basereg), (disp), (indexreg), (shift)); \
	} while(0)

#ifdef	__cplusplus
};
#endif

#endif /* _JIT_GEN_X86_64_H */
