/*
 * jit-apply-arm.c - Apply support routines for ARM.
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

#include "jit-internal.h"

#if defined(__arm) || defined(__arm__)

#include "jit-gen-arm.h"

void _jit_create_closure(unsigned char *buf, void *func,
                         void *closure, void *_type)
{
	arm_inst_ptr inst = (arm_inst_ptr)buf;

	/* Set up the local stack frame */
	arm_setup_frame(inst, 0);
	arm_alu_reg_imm8(inst, ARM_SUB, ARM_SP, ARM_SP, 24);

	/* Create the apply argument block on the stack */
	arm_store_membase(inst, ARM_R0, ARM_FP, -28);
	arm_store_membase(inst, ARM_R1, ARM_FP, -24);
	arm_store_membase(inst, ARM_R2, ARM_FP, -20);
	arm_store_membase(inst, ARM_R3, ARM_FP, -16);
	arm_alu_reg_imm(inst, ARM_ADD, ARM_R3, ARM_FP, 4);
	arm_store_membase(inst, ARM_R3, ARM_FP, -36);
	arm_store_membase(inst, ARM_R0, ARM_FP, -32);

	/* Set up the arguments for calling "func" */
	arm_mov_reg_imm(inst, ARM_R0, (int)(jit_nint)closure);
	arm_mov_reg_reg(inst, ARM_R1, ARM_SP);

	/* Call the closure handling function */
	arm_call(inst, func);

	/* Pop the current stack frame and return */
	arm_pop_frame(inst, 0);
}

void *_jit_create_redirector(unsigned char *buf, void *func,
							 void *user_data, int abi)
{
	arm_inst_ptr inst;

	/* Align "buf" on an appropriate boundary */
	if((((jit_nint)buf) % jit_closure_align) != 0)
	{
		buf += jit_closure_align - (((jit_nint)buf) % jit_closure_align);
	}

	/* Set up the instruction output pointer */
	inst = (arm_inst_ptr)buf;

	/* Set up the local stack frame, and save R0-R3 */
	arm_setup_frame(inst, 0x000F);

	/* Set up the arguments for calling "func" */
	arm_mov_reg_imm(inst, ARM_R0, (int)(jit_nint)user_data);

	/* Call the redirector handling function */
	arm_call(inst, func);

	/* Shift the result into R12, because we are about to restore R0 */
	arm_mov_reg_reg(inst, ARM_R12, ARM_R0);

	/* Pop the current stack frame, but don't change PC yet */
	arm_pop_frame_tail(inst, 0x000F);

	/* Jump to the function that the redirector indicated */
	arm_mov_reg_reg(inst, ARM_PC, ARM_R12);

	/* Flush the cache lines that we just wrote */
	jit_flush_exec(buf, ((unsigned char *)inst) - buf);

	/* Return the aligned start of the buffer as the entry point */
	return (void *)buf;
}

#endif /* arm */
