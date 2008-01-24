/*
 * arm_codegen.h - Code generation macros for the ARM processor.
 *
 * Copyright (C) 2003, 2004  Southern Storm Software, Pty Ltd.
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

#ifndef	_ARM_CODEGEN_H
#define	_ARM_CODEGEN_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Register numbers.
 */
typedef enum
{
	ARM_R0   = 0,
	ARM_R1   = 1,
	ARM_R2   = 2,
	ARM_R3   = 3,
	ARM_R4   = 4,
	ARM_R5   = 5,
	ARM_R6   = 6,
	ARM_R7   = 7,
	ARM_R8   = 8,
	ARM_R9   = 9,
	ARM_R10  = 10,
	ARM_R11  = 11,
	ARM_R12  = 12,
	ARM_R13  = 13,
	ARM_R14  = 14,
	ARM_R15  = 15,
	ARM_FP   = ARM_R11,			/* Frame pointer */
	ARM_LINK = ARM_R14,			/* Link register */
	ARM_PC   = ARM_R15,			/* Program counter */
	ARM_WORK = ARM_R12,			/* Work register that we can destroy */
	ARM_SP   = ARM_R13			/* Stack pointer */

} ARM_REG;

/*
 * Floating-point register numbers.
 */
typedef enum
{
	ARM_F0	= 0,
	ARM_F1	= 1,
	ARM_F2	= 2,
	ARM_F3	= 3,
	ARM_F4	= 4,
	ARM_F5	= 5,
	ARM_F6	= 6,
	ARM_F7	= 7

} ARM_FREG;

/*
 * Condition codes.
 */
typedef enum
{
	ARM_CC_EQ    = 0,			/* Equal */
	ARM_CC_NE    = 1,			/* Not equal */
	ARM_CC_CS    = 2,			/* Carry set */
	ARM_CC_CC    = 3,			/* Carry clear */
	ARM_CC_MI    = 4,			/* Negative */
	ARM_CC_PL    = 5,			/* Positive */
	ARM_CC_VS    = 6,			/* Overflow set */
	ARM_CC_VC    = 7,			/* Overflow clear */
	ARM_CC_HI    = 8,			/* Higher */
	ARM_CC_LS    = 9,			/* Lower or same */
	ARM_CC_GE    = 10,			/* Signed greater than or equal */
	ARM_CC_LT    = 11,			/* Signed less than */
	ARM_CC_GT    = 12,			/* Signed greater than */
	ARM_CC_LE    = 13,			/* Signed less than or equal */
	ARM_CC_AL    = 14,			/* Always */
	ARM_CC_NV    = 15,			/* Never */
	ARM_CC_GE_UN = ARM_CC_CS,	/* Unsigned greater than or equal */
	ARM_CC_LT_UN = ARM_CC_CC,	/* Unsigned less than */
	ARM_CC_GT_UN = ARM_CC_HI,	/* Unsigned greater than */
	ARM_CC_LE_UN = ARM_CC_LS	/* Unsigned less than or equal */

} ARM_CC;

/*
 * Arithmetic and logical operations.
 */
typedef enum
{
	ARM_AND = 0,				/* Bitwise AND */
	ARM_EOR = 1,				/* Bitwise XOR */
	ARM_SUB = 2,				/* Subtract */
	ARM_RSB = 3,				/* Reverse subtract */
	ARM_ADD = 4,				/* Add */
	ARM_ADC = 5,				/* Add with carry */
	ARM_SBC = 6,				/* Subtract with carry */
	ARM_RSC = 7,				/* Reverse subtract with carry */
	ARM_TST = 8,				/* Test with AND */
	ARM_TEQ = 9,				/* Test with XOR */
	ARM_CMP = 10,				/* Test with SUB (compare) */
	ARM_CMN = 11,				/* Test with ADD */
	ARM_ORR = 12,				/* Bitwise OR */
	ARM_MOV = 13,				/* Move */
	ARM_BIC = 14,				/* Test with Op1 & ~Op2 */
	ARM_MVN = 15				/* Bitwise NOT */

} ARM_OP;

/*
 * Shift operators.
 */
typedef enum
{
	ARM_SHL = 0,				/* Logical left */
	ARM_SHR = 1,				/* Logical right */
	ARM_SAR = 2,				/* Arithmetic right */
	ARM_ROR = 3					/* Rotate right */

} ARM_SHIFT;

/*
 * Floating-point unary operators.
 */
typedef enum
{
	ARM_MVF		= 0,			/* Move */
	ARM_MNF		= 1,			/* Move negative */
	ARM_ABS		= 2,			/* Absolute value */
	ARM_RND		= 3,			/* Round */
	ARM_SQT		= 4,			/* Square root */
	ARM_LOG		= 5,			/* log10 */
	ARM_LGN		= 6,			/* ln */
	ARM_EXP		= 7,			/* exp */
	ARM_SIN		= 8,			/* sin */
	ARM_COS		= 9,			/* cos */
	ARM_TAN		= 10,			/* tan */
	ARM_ASN		= 11,			/* asin */
	ARM_ACS		= 12,			/* acos */
	ARM_ATN		= 13			/* atan */

} ARM_FUNARY;

/*
 * Floating-point binary operators.
 */
typedef enum
{
	ARM_ADF		= 0,			/* Add */
	ARM_MUF		= 1,			/* Multiply */
	ARM_SUF		= 2,			/* Subtract */
	ARM_RSF		= 3,			/* Reverse subtract */
	ARM_DVF		= 4,			/* Divide */
	ARM_RDF		= 5,			/* Reverse divide */
	ARM_POW		= 6,			/* pow */
	ARM_RPW		= 7,			/* Reverse pow */
	ARM_RMF		= 8,			/* Remainder */
	ARM_FML		= 9,			/* Fast multiply (32-bit only) */
	ARM_FDV		= 10,			/* Fast divide (32-bit only) */
	ARM_FRD		= 11,			/* Fast reverse divide (32-bit only) */
	ARM_POL		= 12			/* Polar angle */

} ARM_FBINARY;

/*
 * Number of registers that are used for parameters (r0-r3).
 */
#define	ARM_NUM_PARAM_REGS	4

/*
 * Type that keeps track of the instruction buffer.
 */
typedef unsigned int arm_inst_word;
typedef struct
{
	arm_inst_word *current;
	arm_inst_word *limit;

} arm_inst_buf;
#define	arm_inst_get_posn(inst)		((inst).current)
#define	arm_inst_get_limit(inst)	((inst).limit)

/*
 * Build an instruction prefix from a condition code and a mask value.
 */
#define	arm_build_prefix(cond,mask)	\
			((((unsigned int)(cond)) << 28) | ((unsigned int)(mask)))

/*
 * Build an "always" instruction prefix for a regular instruction.
 */
#define arm_prefix(mask)	(arm_build_prefix(ARM_CC_AL, (mask)))

/*
 * Build special "always" prefixes.
 */
#define	arm_always			(arm_build_prefix(ARM_CC_AL, 0))
#define	arm_always_cc		(arm_build_prefix(ARM_CC_AL, (1 << 20)))
#define	arm_always_imm		(arm_build_prefix(ARM_CC_AL, (1 << 25)))

/*
 * Wrappers for "arm_always*" that allow higher-level routines
 * to change code generation to be based on a condition.  This is
 * used to perform branch elimination.
 */
#ifndef arm_execute
#define	arm_execute			arm_always
#define	arm_execute_cc		arm_always_cc
#define	arm_execute_imm		arm_always_imm
#endif

/*
 * Initialize an instruction buffer.
 */
#define	arm_inst_buf_init(inst,start,end)	\
			do { \
				(inst).current = (arm_inst_word *)(start); \
				(inst).limit = (arm_inst_word *)(end); \
			} while (0)

/*
 * Add an instruction to an instruction buffer.
 */
#define	arm_inst_add(inst,value)	\
			do { \
				if((inst).current < (inst).limit) \
				{ \
					*((inst).current)++ = (value); \
				} \
			} while (0)

/*
 * Arithmetic or logical operation which doesn't set condition codes.
 */
#define	arm_alu_reg_reg(inst,opc,dreg,sreg1,sreg2)	\
			do { \
				arm_inst_add((inst), arm_execute | \
							(((unsigned int)(opc)) << 21) | \
							(((unsigned int)(dreg)) << 12) | \
							(((unsigned int)(sreg1)) << 16) | \
							 ((unsigned int)(sreg2))); \
			} while (0)
#define	arm_alu_reg_imm8(inst,opc,dreg,sreg,imm)	\
			do { \
				arm_inst_add((inst), arm_execute_imm | \
							(((unsigned int)(opc)) << 21) | \
							(((unsigned int)(dreg)) << 12) | \
							(((unsigned int)(sreg)) << 16) | \
							 ((unsigned int)((imm) & 0xFF))); \
			} while (0)
#define	arm_alu_reg_imm8_cond(inst,opc,dreg,sreg,imm,cond)	\
			do { \
				arm_inst_add((inst), arm_build_prefix((cond), (1 << 25)) | \
							(((unsigned int)(opc)) << 21) | \
							(((unsigned int)(dreg)) << 12) | \
							(((unsigned int)(sreg)) << 16) | \
							 ((unsigned int)((imm) & 0xFF))); \
			} while (0)
#define	arm_alu_reg_imm8_rotate(inst,opc,dreg,sreg,imm,rotate)	\
			do { \
				arm_inst_add((inst), arm_execute_imm | \
							(((unsigned int)(opc)) << 21) | \
							(((unsigned int)(dreg)) << 12) | \
							(((unsigned int)(sreg)) << 16) | \
							(((unsigned int)(rotate)) << 8) | \
							 ((unsigned int)((imm) & 0xFF))); \
			} while (0)
extern void _arm_alu_reg_imm
		(arm_inst_buf *inst, int opc, int dreg,
		 int sreg, int imm, int saveWork, int execute_prefix);
#define	arm_alu_reg_imm(inst,opc,dreg,sreg,imm)	\
			do { \
				int __alu_imm = (int)(imm); \
				if(__alu_imm >= 0 && __alu_imm < 256) \
				{ \
					arm_alu_reg_imm8 \
						((inst), (opc), (dreg), (sreg), __alu_imm); \
				} \
				else \
				{ \
					_arm_alu_reg_imm \
						(&(inst), (opc), (dreg), (sreg), __alu_imm, 0, \
						 arm_execute); \
				} \
			} while (0)
#define	arm_alu_reg_imm_save_work(inst,opc,dreg,sreg,imm)	\
			do { \
				int __alu_imm_save = (int)(imm); \
				if(__alu_imm_save >= 0 && __alu_imm_save < 256) \
				{ \
					arm_alu_reg_imm8 \
						((inst), (opc), (dreg), (sreg), __alu_imm_save); \
				} \
				else \
				{ \
					_arm_alu_reg_imm \
						(&(inst), (opc), (dreg), (sreg), __alu_imm_save, 1, \
						 arm_execute); \
				} \
			} while (0)
#define arm_alu_reg(inst,opc,dreg,sreg)	\
			do { \
				arm_inst_add((inst), arm_execute | \
							(((unsigned int)(opc)) << 21) | \
							(((unsigned int)(dreg)) << 12) | \
							 ((unsigned int)(sreg))); \
			} while (0)
#define arm_alu_reg_cond(inst,opc,dreg,sreg,cond)	\
			do { \
				arm_inst_add((inst), arm_build_prefix((cond), 0) | \
							(((unsigned int)(opc)) << 21) | \
							(((unsigned int)(dreg)) << 12) | \
							 ((unsigned int)(sreg))); \
			} while (0)

/*
 * Arithmetic or logical operation which sets condition codes.
 */
#define	arm_alu_cc_reg_reg(inst,opc,dreg,sreg1,sreg2)	\
			do { \
				arm_inst_add((inst), arm_execute_cc | \
							(((unsigned int)(opc)) << 21) | \
							(((unsigned int)(dreg)) << 12) | \
							(((unsigned int)(sreg1)) << 16) | \
							 ((unsigned int)(sreg2))); \
			} while (0)
#define	arm_alu_cc_reg_imm8(inst,opc,dreg,sreg,imm)	\
			do { \
				arm_inst_add((inst), arm_execute_imm | arm_execute_cc | \
							(((unsigned int)(opc)) << 21) | \
							(((unsigned int)(dreg)) << 12) | \
							(((unsigned int)(sreg)) << 16) | \
							 ((unsigned int)((imm) & 0xFF))); \
			} while (0)
#define arm_alu_cc_reg(inst,opc,dreg,sreg)	\
			do { \
				arm_inst_add((inst), = arm_execute_cc | \
							(((unsigned int)(opc)) << 21) | \
							(((unsigned int)(dreg)) << 12) | \
							 ((unsigned int)(sreg))); \
			} while (0)

/*
 * Test operation, which sets the condition codes but has no other result.
 */
#define arm_test_reg_reg(inst,opc,sreg1,sreg2)	\
			do { \
				arm_alu_cc_reg_reg((inst), (opc), 0, (sreg1), (sreg2)); \
			} while (0)
#define arm_test_reg_imm8(inst,opc,sreg,imm)	\
			do { \
				arm_alu_cc_reg_imm8((inst), (opc), 0, (sreg), (imm)); \
			} while (0)
#define arm_test_reg_imm(inst,opc,sreg,imm)	\
			do { \
				int __test_imm = (int)(imm); \
				if(__test_imm >= 0 && __test_imm < 256) \
				{ \
					arm_alu_cc_reg_imm8((inst), (opc), 0, (sreg), __test_imm); \
				} \
				else \
				{ \
					arm_mov_reg_imm((inst), ARM_WORK, __test_imm); \
					arm_test_reg_reg((inst), (opc), (sreg), ARM_WORK); \
				} \
			} while (0)

/*
 * Move a value between registers.
 */
#define	arm_mov_reg_reg(inst,dreg,sreg)	\
			do { \
				arm_alu_reg((inst), ARM_MOV, (dreg), (sreg)); \
			} while (0)

/*
 * Move an immediate value into a register.  This is hard because
 * ARM lacks an instruction to load a 32-bit immediate value directly.
 * We handle the simple cases and then bail out to a function for the rest.
 */
#define	arm_mov_reg_imm8(inst,reg,imm)	\
			do { \
				arm_alu_reg_imm8((inst), ARM_MOV, (reg), 0, (imm)); \
			} while (0)
#define	arm_mov_reg_imm8_rotate(inst,reg,imm,rotate)	\
			do { \
				arm_alu_reg_imm8_rotate((inst), ARM_MOV, (reg), \
										0, (imm), (rotate)); \
			} while (0)
extern void _arm_mov_reg_imm
	(arm_inst_buf *inst, int reg, int value, int execute_prefix);
extern int arm_is_complex_imm(int value);
#define	arm_mov_reg_imm(inst,reg,imm)	\
			do { \
				int __imm = (int)(imm); \
				if(__imm >= 0 && __imm < 256) \
				{ \
					arm_mov_reg_imm8((inst), (reg), __imm); \
				} \
				else if((reg) == ARM_PC) \
				{ \
					_arm_mov_reg_imm \
						(&(inst), ARM_WORK, __imm, arm_execute); \
					arm_mov_reg_reg((inst), ARM_PC, ARM_WORK); \
				} \
				else if(__imm > -256 && __imm < 0) \
				{ \
					arm_mov_reg_imm8((inst), (reg), ~(__imm)); \
					arm_alu_reg((inst), ARM_MVN, (reg), (reg)); \
				} \
				else \
				{ \
					_arm_mov_reg_imm(&(inst), (reg), __imm, arm_execute); \
				} \
			} while (0)

/*
 * Clear a register to zero.
 */
#define	arm_clear_reg(inst,reg)	\
			do { \
				arm_mov_reg_imm8((inst), (reg), 0); \
			} while (0)

/*
 * No-operation instruction.
 */
#define	arm_nop(inst)	arm_mov_reg_reg((inst), ARM_R0, ARM_R0)

/*
 * Perform a shift operation.
 */
#define	arm_shift_reg_reg(inst,opc,dreg,sreg1,sreg2) \
			do { \
				arm_inst_add((inst), arm_execute | \
							(((unsigned int)ARM_MOV) << 21) | \
							(((unsigned int)(dreg)) << 12) | \
							(((unsigned int)(sreg2)) << 8) | \
							(((unsigned int)(opc)) << 5) | \
							 ((unsigned int)(1 << 4)) | \
							 ((unsigned int)(sreg1))); \
			} while (0)
#define	arm_shift_reg_imm8(inst,opc,dreg,sreg,imm) \
			do { \
				arm_inst_add((inst), arm_execute | \
							(((unsigned int)ARM_MOV) << 21) | \
							(((unsigned int)(dreg)) << 12) | \
							(((unsigned int)(opc)) << 5) | \
							(((unsigned int)(imm)) << 7) | \
							 ((unsigned int)(sreg))); \
			} while (0)

/*
 * Perform a multiplication instruction.  Note: ARM instruction rules
 * say that dreg should not be the same as sreg2, so we swap the order
 * of the arguments if that situation occurs.  We assume that sreg1
 * and sreg2 are distinct registers.
 */
#define arm_mul_reg_reg(inst,dreg,sreg1,sreg2)	\
			do { \
				if((dreg) != (sreg2)) \
				{ \
					arm_inst_add((inst), arm_prefix(0x00000090) | \
								(((unsigned int)(dreg)) << 16) | \
								(((unsigned int)(sreg1)) << 8) | \
								 ((unsigned int)(sreg2))); \
				} \
				else \
				{ \
					arm_inst_add((inst), arm_prefix(0x00000090) | \
								(((unsigned int)(dreg)) << 16) | \
								(((unsigned int)(sreg2)) << 8) | \
								 ((unsigned int)(sreg1))); \
				} \
			} while (0)

/*
 * Perform a binary operation on floating-point arguments.
 */
#define	arm_alu_freg_freg(inst,opc,dreg,sreg1,sreg2)	\
			do { \
				arm_inst_add((inst), arm_prefix(0x0E000180) | \
							(((unsigned int)(opc)) << 20) | \
							(((unsigned int)(dreg)) << 12) | \
							(((unsigned int)(sreg1)) << 16) | \
							 ((unsigned int)(sreg2))); \
			} while (0)
#define	arm_alu_freg_freg_32(inst,opc,dreg,sreg1,sreg2)	\
			do { \
				arm_inst_add((inst), arm_prefix(0x0E000100) | \
							(((unsigned int)(opc)) << 20) | \
							(((unsigned int)(dreg)) << 12) | \
							(((unsigned int)(sreg1)) << 16) | \
							 ((unsigned int)(sreg2))); \
			} while (0)

/*
 * Perform a unary operation on floating-point arguments.
 */
#define	arm_alu_freg(inst,opc,dreg,sreg)	\
			do { \
				arm_inst_add((inst), arm_prefix(0x0E008180) | \
							(((unsigned int)(opc)) << 20) | \
							(((unsigned int)(dreg)) << 12) | \
							 ((unsigned int)(sreg))); \
			} while (0)
#define	arm_alu_freg_32(inst,opc,dreg,sreg)	\
			do { \
				arm_inst_add((inst), arm_prefix(0x0E008100) | \
							(((unsigned int)(opc)) << 20) | \
							(((unsigned int)(dreg)) << 12) | \
							 ((unsigned int)(sreg))); \
			} while (0)

/*
 * Branch or jump immediate by a byte offset.  The offset is
 * assumed to be +/- 32 Mbytes.
 */
#define	arm_branch_imm(inst,cond,imm)	\
			do { \
				arm_inst_add((inst), arm_build_prefix((cond), 0x0A000000) | \
							(((unsigned int)(((int)(imm)) >> 2)) & \
								0x00FFFFFF)); \
			} while (0)
#define	arm_jump_imm(inst,imm)	arm_branch_imm((inst), ARM_CC_AL, (imm))

/*
 * Branch or jump to a specific target location.  The offset is
 * assumed to be +/- 32 Mbytes.
 */
#define	arm_branch(inst,cond,target)	\
			do { \
				int __br_offset = (int)(((unsigned char *)(target)) - \
					           (((unsigned char *)((inst).current)) + 8)); \
				arm_branch_imm((inst), (cond), __br_offset); \
			} while (0)
#define	arm_jump(inst,target)	arm_branch((inst), ARM_CC_AL, (target))

/*
 * Jump to a specific target location that may be greater than
 * 32 Mbytes away from the current location.
 */
#define	arm_jump_long(inst,target)	\
			do { \
				int __jmp_offset = (int)(((unsigned char *)(target)) - \
					            (((unsigned char *)((inst).current)) + 8)); \
				if(__jmp_offset >= -0x04000000 && __jmp_offset < 0x04000000) \
				{ \
					arm_jump_imm((inst), __jmp_offset); \
				} \
				else \
				{ \
					arm_mov_reg_imm((inst), ARM_PC, (int)(target)); \
				} \
			} while (0)

/*
 * Back-patch a branch instruction.
 */
#define	arm_patch(inst,posn,target)	\
			do { \
				int __p_offset = (int)(((unsigned char *)(target)) - \
							          (((unsigned char *)(posn)) + 8)); \
				__p_offset = (__p_offset >> 2) & 0x00FFFFFF; \
				if(((arm_inst_word *)(posn)) < (inst).limit) \
				{ \
					*((int *)(posn)) = (*((int *)(posn)) & 0xFF000000) | \
						__p_offset; \
				} \
			} while (0)

/*
 * Call a subroutine immediate by a byte offset.
 */
#define	arm_call_imm(inst,imm)	\
			do { \
				arm_inst_add((inst), arm_prefix(0x0B000000) | \
							(((unsigned int)(((int)(imm)) >> 2)) & \
								0x00FFFFFF)); \
			} while (0)

/*
 * Call a subroutine at a specific target location.
 */
#define	arm_call(inst,target)	\
			do { \
				int __call_offset = (int)(((unsigned char *)(target)) - \
					             (((unsigned char *)((inst).current)) + 8)); \
				if(__call_offset >= -0x04000000 && __call_offset < 0x04000000) \
				{ \
					arm_call_imm((inst), __call_offset); \
				} \
				else \
				{ \
					arm_load_membase((inst), ARM_WORK, ARM_PC, 4); \
					arm_alu_reg_imm8((inst), ARM_ADD, ARM_LINK, ARM_PC, 4); \
					arm_mov_reg_reg((inst), ARM_PC, ARM_WORK); \
					arm_inst_add((inst), (int)(target)); \
				} \
			} while (0)

/*
 * Return from a subroutine, where the return address is in the link register.
 */
#define	arm_return(inst)	\
			do { \
				arm_mov_reg_reg((inst), ARM_PC, ARM_LINK); \
			} while (0)

/*
 * Push a register onto the system stack.
 */
#define	arm_push_reg(inst,reg)	\
			do { \
				arm_inst_add((inst), arm_prefix(0x05200004) | \
							(((unsigned int)ARM_SP) << 16) | \
							(((unsigned int)(reg)) << 12)); \
			} while (0)

/*
 * Pop a register from the system stack.
 */
#define	arm_pop_reg(inst,reg)	\
			do { \
				arm_inst_add((inst), arm_prefix(0x04900004) | \
							(((unsigned int)ARM_SP) << 16) | \
							(((unsigned int)(reg)) << 12)); \
			} while (0)

/*
 * Set up a local variable frame, and save the registers in "regset".
 */
#define	arm_setup_frame(inst,regset)	\
			do { \
				arm_mov_reg_reg((inst), ARM_WORK, ARM_SP); \
				arm_inst_add((inst), arm_prefix(0x0920D800) | \
							(((unsigned int)ARM_SP) << 16) | \
							(((unsigned int)(regset)))); \
				arm_alu_reg_imm8((inst), ARM_SUB, ARM_FP, ARM_WORK, 4); \
			} while (0)

/*
 * Pop a local variable frame, restore the registers in "regset",
 * and return to the caller.
 */
#define	arm_pop_frame(inst,regset)	\
			do { \
				arm_inst_add((inst), arm_prefix(0x0910A800) | \
							(((unsigned int)ARM_FP) << 16) | \
							(((unsigned int)(regset)))); \
			} while (0)

/*
 * Pop a local variable frame, in preparation for a tail call.
 * This restores "lr" to its original value, but does not set "pc".
 */
#define	arm_pop_frame_tail(inst,regset)	\
			do { \
				arm_inst_add((inst), arm_prefix(0x09106800) | \
							(((unsigned int)ARM_FP) << 16) | \
							(((unsigned int)(regset)))); \
			} while (0)

/*
 * Load a word value from a pointer and then advance the pointer.
 */
#define	arm_load_advance(inst,dreg,sreg)	\
			do { \
				arm_inst_add((inst), arm_prefix(0x04900004) | \
							(((unsigned int)(sreg)) << 16) | \
							(((unsigned int)(dreg)) << 12)); \
			} while (0)

/*
 * Load a value from an address into a register.
 */
#define arm_load_membase_either(inst,reg,basereg,imm,mask)	\
			do { \
				int __mb_offset = (int)(imm); \
				if(__mb_offset >= 0 && __mb_offset < (1 << 12)) \
				{ \
					arm_inst_add((inst), arm_prefix(0x05900000 | (mask)) | \
								(((unsigned int)(basereg)) << 16) | \
								(((unsigned int)(reg)) << 12) | \
								 ((unsigned int)__mb_offset)); \
				} \
				else if(__mb_offset > -(1 << 12) && __mb_offset < 0) \
				{ \
					arm_inst_add((inst), arm_prefix(0x05100000 | (mask)) | \
								(((unsigned int)(basereg)) << 16) | \
								(((unsigned int)(reg)) << 12) | \
								 ((unsigned int)(-__mb_offset))); \
				} \
				else \
				{ \
					arm_mov_reg_imm((inst), ARM_WORK, __mb_offset); \
					arm_inst_add((inst), arm_prefix(0x07900000 | (mask)) | \
								(((unsigned int)(basereg)) << 16) | \
								(((unsigned int)(reg)) << 12) | \
								 ((unsigned int)ARM_WORK)); \
				} \
			} while (0)
#define	arm_load_membase(inst,reg,basereg,imm)	\
			do { \
				arm_load_membase_either((inst), (reg), (basereg), (imm), 0); \
			} while (0)
#define	arm_load_membase_byte(inst,reg,basereg,imm)	\
			do { \
				arm_load_membase_either((inst), (reg), (basereg), (imm), \
										0x00400000); \
			} while (0)
#define	arm_load_membase_sbyte(inst,reg,basereg,imm)	\
			do { \
				arm_load_membase_either((inst), (reg), (basereg), (imm), \
										0x00400000); \
				arm_shift_reg_imm8((inst), ARM_SHL, (reg), (reg), 24); \
				arm_shift_reg_imm8((inst), ARM_SAR, (reg), (reg), 24); \
			} while (0)
#define	arm_load_membase_ushort(inst,reg,basereg,imm)	\
			do { \
				arm_load_membase_byte((inst), ARM_WORK, (basereg), (imm)); \
				arm_load_membase_byte((inst), (reg), (basereg), (imm) + 1); \
				arm_shift_reg_imm8((inst), ARM_SHL, (reg), (reg), 8); \
				arm_alu_reg_reg((inst), ARM_ORR, (reg), (reg), ARM_WORK); \
			} while (0)
#define	arm_load_membase_short(inst,reg,basereg,imm)	\
			do { \
				arm_load_membase_byte((inst), ARM_WORK, (basereg), (imm)); \
				arm_load_membase_byte((inst), (reg), (basereg), (imm) + 1); \
				arm_shift_reg_imm8((inst), ARM_SHL, (reg), (reg), 24); \
				arm_shift_reg_imm8((inst), ARM_SAR, (reg), (reg), 16); \
				arm_alu_reg_reg((inst), ARM_ORR, (reg), (reg), ARM_WORK); \
			} while (0)

/*
 * Load a floating-point value from an address into a register.
 */
#define	arm_load_membase_float(inst,reg,basereg,imm,mask)	\
			do { \
				int __mb_offset = (int)(imm); \
				if(__mb_offset >= 0 && __mb_offset < (1 << 10) && \
				   (__mb_offset & 3) == 0) \
				{ \
					arm_inst_add((inst), arm_prefix(0x0D900100 | (mask)) | \
							(((unsigned int)(basereg)) << 16) | \
							(((unsigned int)(reg)) << 12) | \
							 ((unsigned int)((__mb_offset / 4) & 0xFF))); \
				} \
				else if(__mb_offset > -(1 << 10) && __mb_offset < 0 && \
				        (__mb_offset & 3) == 0) \
				{ \
					arm_inst_add((inst), arm_prefix(0x0D180100 | (mask)) | \
							(((unsigned int)(basereg)) << 16) | \
							(((unsigned int)(reg)) << 12) | \
							 ((unsigned int)(((-__mb_offset) / 4) & 0xFF)));\
				} \
				else \
				{ \
					arm_mov_reg_imm((inst), ARM_WORK, __mb_offset); \
					arm_alu_reg_reg((inst), ARM_ADD, ARM_WORK, \
								    (basereg), ARM_WORK); \
					arm_inst_add((inst), arm_prefix(0x0D900100 | (mask)) | \
							(((unsigned int)ARM_WORK) << 16) | \
							(((unsigned int)(reg)) << 12)); \
				} \
			} while (0)
#define	arm_load_membase_float32(inst,reg,basereg,imm)	\
			do { \
				arm_load_membase_float((inst), (reg), (basereg), (imm), 0); \
			} while (0)
#define	arm_load_membase_float64(inst,reg,basereg,imm)	\
			do { \
				arm_load_membase_float((inst), (reg), (basereg), \
									   (imm), 0x00008000); \
			} while (0)

/*
 * Store a value from a register into an address.
 *
 * Note: storing a 16-bit value destroys the value in the register.
 */
#define arm_store_membase_either(inst,reg,basereg,imm,mask)	\
			do { \
				int __sm_offset = (int)(imm); \
				if(__sm_offset >= 0 && __sm_offset < (1 << 12)) \
				{ \
					arm_inst_add((inst), arm_prefix(0x05800000 | (mask)) | \
								(((unsigned int)(basereg)) << 16) | \
								(((unsigned int)(reg)) << 12) | \
								 ((unsigned int)__sm_offset)); \
				} \
				else if(__sm_offset > -(1 << 12) && __sm_offset < 0) \
				{ \
					arm_inst_add((inst), arm_prefix(0x05000000 | (mask)) | \
								(((unsigned int)(basereg)) << 16) | \
								(((unsigned int)(reg)) << 12) | \
								 ((unsigned int)(-__sm_offset))); \
				} \
				else \
				{ \
					arm_mov_reg_imm((inst), ARM_WORK, __sm_offset); \
					arm_inst_add((inst), arm_prefix(0x07800000 | (mask)) | \
								(((unsigned int)(basereg)) << 16) | \
								(((unsigned int)(reg)) << 12) | \
								 ((unsigned int)ARM_WORK)); \
				} \
			} while (0)
#define	arm_store_membase(inst,reg,basereg,imm)	\
			do { \
				arm_store_membase_either((inst), (reg), (basereg), (imm), 0); \
			} while (0)
#define	arm_store_membase_byte(inst,reg,basereg,imm)	\
			do { \
				arm_store_membase_either((inst), (reg), (basereg), (imm), \
										 0x00400000); \
			} while (0)
#define	arm_store_membase_sbyte(inst,reg,basereg,imm)	\
			do { \
				arm_store_membase_byte((inst), (reg), (basereg), (imm)); \
			} while (0)
#define	arm_store_membase_short(inst,reg,basereg,imm)	\
			do { \
				arm_store_membase_either((inst), (reg), (basereg), (imm), \
										 0x00400000); \
				arm_shift_reg_imm8((inst), ARM_SHR, (reg), (reg), 8); \
				arm_store_membase_either((inst), (reg), (basereg), \
										 (imm) + 1, 0x00400000); \
			} while (0)
#define	arm_store_membase_ushort(inst,reg,basereg,imm)	\
			do { \
				arm_store_membase_short((inst), (reg), (basereg), (imm)); \
			} while (0)

/*
 * Store a floating-point value to a memory address.
 */
#define	arm_store_membase_float(inst,reg,basereg,imm,mask)	\
			do { \
				int __mb_offset = (int)(imm); \
				if(__mb_offset >= 0 && __mb_offset < (1 << 10) && \
				   (__mb_offset & 3) == 0) \
				{ \
					arm_inst_add((inst), arm_prefix(0x0D800100 | (mask)) | \
							(((unsigned int)(basereg)) << 16) | \
							(((unsigned int)(reg)) << 12) | \
							 ((unsigned int)((__mb_offset / 4) & 0xFF))); \
				} \
				else if(__mb_offset > -(1 << 10) && __mb_offset < 0 && \
				        (__mb_offset & 3) == 0) \
				{ \
					arm_inst_add((inst), arm_prefix(0x0D080100 | (mask)) | \
							(((unsigned int)(basereg)) << 16) | \
							(((unsigned int)(reg)) << 12) | \
							 ((unsigned int)(((-__mb_offset) / 4) & 0xFF)));\
				} \
				else \
				{ \
					arm_mov_reg_imm((inst), ARM_WORK, __mb_offset); \
					arm_alu_reg_reg((inst), ARM_ADD, ARM_WORK, \
								    (basereg), ARM_WORK); \
					arm_inst_add((inst), arm_prefix(0x0D800100 | (mask)) | \
							(((unsigned int)ARM_WORK) << 16) | \
							(((unsigned int)(reg)) << 12)); \
				} \
			} while (0)
#define	arm_store_membase_float32(inst,reg,basereg,imm)	\
			do { \
				arm_store_membase_float((inst), (reg), (basereg), (imm), 0); \
			} while (0)
#define	arm_store_membase_float64(inst,reg,basereg,imm)	\
			do { \
				arm_store_membase_float((inst), (reg), (basereg), \
									    (imm), 0x00008000); \
			} while (0)
#define	arm_push_reg_float32(inst,reg)	\
			do { \
				arm_store_membase_float((inst), (reg), ARM_SP, \
									    -4, 0x00200000); \
			} while (0)
#define	arm_push_reg_float64(inst,reg)	\
			do { \
				arm_store_membase_float((inst), (reg), ARM_SP, \
									    -4, 0x00208000); \
			} while (0)

/*
 * Load a value from an indexed address into a register.
 */
#define arm_load_memindex_either(inst,reg,basereg,indexreg,shift,mask)	\
			do { \
				arm_inst_add((inst), arm_prefix(0x07900000 | (mask)) | \
							(((unsigned int)(basereg)) << 16) | \
							(((unsigned int)(reg)) << 12) | \
							(((unsigned int)(shift)) << 7) | \
							 ((unsigned int)(indexreg))); \
			} while (0)
#define	arm_load_memindex(inst,reg,basereg,indexreg)	\
			do { \
				arm_load_memindex_either((inst), (reg), (basereg), \
										 (indexreg), 2, 0); \
			} while (0)
#define	arm_load_memindex_byte(inst,reg,basereg,indexreg)	\
			do { \
				arm_load_memindex_either((inst), (reg), (basereg), \
									     (indexreg), 0, 0x00400000); \
			} while (0)
#define	arm_load_memindex_sbyte(inst,reg,basereg,indexreg)	\
			do { \
				arm_load_memindex_either((inst), (reg), (basereg), \
									     (indexreg), 0, 0x00400000); \
				arm_shift_reg_imm8((inst), ARM_SHL, (reg), (reg), 24); \
				arm_shift_reg_imm8((inst), ARM_SAR, (reg), (reg), 24); \
			} while (0)
#define	arm_load_memindex_ushort(inst,reg,basereg,indexreg)	\
			do { \
				arm_alu_reg_reg((inst), ARM_ADD, ARM_WORK, (basereg), \
								(indexreg)); \
				arm_alu_reg_reg((inst), ARM_ADD, ARM_WORK, ARM_WORK, \
								(indexreg)); \
				arm_load_membase_byte((inst), (reg), ARM_WORK, 0); \
				arm_load_membase_byte((inst), ARM_WORK, ARM_WORK, 1); \
				arm_shift_reg_imm8((inst), ARM_SHL, ARM_WORK, ARM_WORK, 8); \
				arm_alu_reg_reg((inst), ARM_ORR, (reg), (reg), ARM_WORK); \
			} while (0)
#define	arm_load_memindex_short(inst,reg,basereg,indexreg)	\
			do { \
				arm_alu_reg_reg((inst), ARM_ADD, ARM_WORK, (basereg), \
								(indexreg)); \
				arm_alu_reg_reg((inst), ARM_ADD, ARM_WORK, ARM_WORK, \
								(indexreg)); \
				arm_load_membase_byte((inst), (reg), ARM_WORK, 0); \
				arm_load_membase_byte((inst), ARM_WORK, ARM_WORK, 1); \
				arm_shift_reg_imm8((inst), ARM_SHL, ARM_WORK, ARM_WORK, 24); \
				arm_shift_reg_imm8((inst), ARM_SAR, ARM_WORK, ARM_WORK, 16); \
				arm_alu_reg_reg((inst), ARM_ORR, (reg), (reg), ARM_WORK); \
			} while (0)

/*
 * Store a value from a register into an indexed address.
 *
 * Note: storing a 16-bit value destroys the values in the base
 * register and the source register.
 */
#define arm_store_memindex_either(inst,reg,basereg,indexreg,shift,mask)	\
			do { \
				arm_inst_add((inst), arm_prefix(0x07800000 | (mask)) | \
							(((unsigned int)(basereg)) << 16) | \
							(((unsigned int)(reg)) << 12) | \
							(((unsigned int)(shift)) << 7) | \
							 ((unsigned int)(indexreg))); \
			} while (0)
#define	arm_store_memindex(inst,reg,basereg,indexreg)	\
			do { \
				arm_store_memindex_either((inst), (reg), (basereg), \
										  (indexreg), 2, 0); \
			} while (0)
#define	arm_store_memindex_byte(inst,reg,basereg,indexreg)	\
			do { \
				arm_store_memindex_either((inst), (reg), (basereg), \
										  (indexreg), 0, 0x00400000); \
			} while (0)
#define	arm_store_memindex_sbyte(inst,reg,basereg,indexreg)	\
			do { \
				arm_store_memindex_byte((inst), (reg), (basereg), \
										(indexreg)); \
			} while (0)
#define	arm_store_memindex_short(inst,reg,basereg,indexreg)	\
			do { \
				arm_store_memindex_either((inst), (reg), (basereg), \
										  (indexreg), 1, 0x00400000); \
				arm_alu_reg_imm8((inst), ARM_ADD, (basereg), (basereg), 1); \
				arm_shift_reg_imm8((inst), ARM_SHR, (reg), (reg), 8); \
				arm_store_memindex_either((inst), (reg), (basereg), \
										  (indexreg), 1, 0x00400000); \
			} while (0)
#define	arm_store_memindex_ushort(inst,reg,basereg,indexreg)	\
			do { \
				arm_store_memindex_short((inst), (reg), \
										 (basereg), (indexreg)); \
			} while (0)

#ifdef __cplusplus
};
#endif

#endif /* _ARM_CODEGEN_H */
