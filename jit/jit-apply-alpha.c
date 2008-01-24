/*
 * jit-apply-alpha.c - Apply support routines for alpha.
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

#include "jit-internal.h"

#if defined(__alpha) || defined(__alpha__)

#include "jit-gen-alpha.h"

void _jit_create_closure(unsigned char *buf, void *func, void *closure, void *_type) {
	alpha_inst inst = (alpha_inst) buf;

	/* Compute and load the global pointer (2 instructions) */
	alpha_ldah(inst,ALPHA_GP,ALPHA_PV,0);
	alpha_lda( inst,ALPHA_GP,ALPHA_GP,0);

	/* Allocate space for a new stack frame. (1 instruction) */
	alpha_lda(inst,ALPHA_SP,ALPHA_SP,-(13*8));

	/* Save the return address. (1 instruction) */
	alpha_stq(inst,ALPHA_RA,ALPHA_SP,0*8);

	/* Save integer register arguments as local variables (6 instructions) */
	alpha_stq(inst,ALPHA_A0,ALPHA_SP,1*8);
	alpha_stq(inst,ALPHA_A1,ALPHA_SP,2*8);
	alpha_stq(inst,ALPHA_A2,ALPHA_SP,3*8);
	alpha_stq(inst,ALPHA_A3,ALPHA_SP,4*8);
	alpha_stq(inst,ALPHA_A4,ALPHA_SP,5*8);
	alpha_stq(inst,ALPHA_A5,ALPHA_SP,6*8);

	/* Save floating-point register arguments as local variables (8 instructions) */
	alpha_stt(inst,ALPHA_FA0,ALPHA_SP, 7*8);
	alpha_stt(inst,ALPHA_FA1,ALPHA_SP, 8*8);
	alpha_stt(inst,ALPHA_FA2,ALPHA_SP, 9*8);
	alpha_stt(inst,ALPHA_FA3,ALPHA_SP,10*8);
	alpha_stt(inst,ALPHA_FA4,ALPHA_SP,11*8);
	alpha_stt(inst,ALPHA_FA5,ALPHA_SP,12*8);

	/* Call the closure handling function (1 instruction) */
	alpha_call(inst,func);

	/* Restore the return address (1 instruction) */
	alpha_ldq(inst,ALPHA_RA,ALPHA_SP,0*8);

	/* Restore integer register arguments (6 instructions) */
	alpha_ldq(inst,ALPHA_A0,ALPHA_SP,1*8);
	alpha_ldq(inst,ALPHA_A1,ALPHA_SP,2*8);
	alpha_ldq(inst,ALPHA_A2,ALPHA_SP,3*8);
	alpha_ldq(inst,ALPHA_A3,ALPHA_SP,4*8);
	alpha_ldq(inst,ALPHA_A4,ALPHA_SP,5*8);
	alpha_ldq(inst,ALPHA_A5,ALPHA_SP,6*8);

	/* Restore floating-point register arguments (8 instructions) */
	alpha_ldt(inst,ALPHA_FA0,ALPHA_SP, 7*8);
	alpha_ldt(inst,ALPHA_FA1,ALPHA_SP, 8*8);
	alpha_ldt(inst,ALPHA_FA2,ALPHA_SP, 9*8);
	alpha_ldt(inst,ALPHA_FA3,ALPHA_SP,10*8);
	alpha_ldt(inst,ALPHA_FA4,ALPHA_SP,11*8);
	alpha_ldt(inst,ALPHA_FA5,ALPHA_SP,12*8);

	/* restore the stack pointer (1 instruction) */
	alpha_lda(inst,ALPHA_SP,ALPHA_SP,(13*8));
}

void *_jit_create_redirector(unsigned char *buf, void *func, void *user_data, int abi) {
	alpha_inst inst = (alpha_inst) buf;

	/* Allocate space for a new stack frame. (1 instruction) */
	alpha_lda(inst,ALPHA_SP,ALPHA_SP,-(16*8));

	/* Save the return address. (1 instruction) */
	alpha_stq(inst,ALPHA_RA,ALPHA_SP,0*8);

	/* Save the frame pointer. (1 instruction) */
	alpha_stq(inst,ALPHA_FP,ALPHA_SP,1*8);

	/* Save the integer save registers (6 instructions) */
	alpha_stq(inst,ALPHA_S0,ALPHA_SP,2*8);
	alpha_stq(inst,ALPHA_S1,ALPHA_SP,3*8);
	alpha_stq(inst,ALPHA_S2,ALPHA_SP,4*8);
	alpha_stq(inst,ALPHA_S3,ALPHA_SP,5*8);
	alpha_stq(inst,ALPHA_S4,ALPHA_SP,6*8);
	alpha_stq(inst,ALPHA_S5,ALPHA_SP,7*8);

	/* Save the floating point save registers (8 instructions) */
	alpha_stt(inst,ALPHA_FS0,ALPHA_SP, 8*8);
	alpha_stt(inst,ALPHA_FS1,ALPHA_SP, 9*8);
	alpha_stt(inst,ALPHA_FS2,ALPHA_SP,10*8);
	alpha_stt(inst,ALPHA_FS3,ALPHA_SP,11*8);
	alpha_stt(inst,ALPHA_FS4,ALPHA_SP,12*8);
	alpha_stt(inst,ALPHA_FS5,ALPHA_SP,13*8);
	alpha_stt(inst,ALPHA_FS6,ALPHA_SP,14*8);
	alpha_stt(inst,ALPHA_FS7,ALPHA_SP,15*8);

	/* Set the frame pointer (1 instruction) */
	alpha_mov(inst,ALPHA_SP,ALPHA_FP);

	/* Compute and load the global pointer (2 instructions) */
	alpha_ldah(inst,ALPHA_GP,ALPHA_PV,0);
	alpha_lda( inst,ALPHA_GP,ALPHA_GP,0);

	/* Force any pending hardware exceptions to be raised. (1 instruction) */
	alpha_trapb(inst);

	/* Call the redirector handling function (6 instruction) */
	alpha_call(inst, func);

	/* Restore the return address. (1 instruction) */
	alpha_ldq(inst,ALPHA_RA,ALPHA_SP,0*8);

	/* Restore the frame pointer. (1 instruction) */
	alpha_ldq(inst,ALPHA_FP,ALPHA_SP,1*8);

	/* Restore the integer save registers (6 instructions) */
	alpha_ldq(inst,ALPHA_S0,ALPHA_SP,2*8);
	alpha_ldq(inst,ALPHA_S1,ALPHA_SP,3*8);
	alpha_ldq(inst,ALPHA_S2,ALPHA_SP,4*8);
	alpha_ldq(inst,ALPHA_S3,ALPHA_SP,5*8);
	alpha_ldq(inst,ALPHA_S4,ALPHA_SP,6*8);
	alpha_ldq(inst,ALPHA_S5,ALPHA_SP,7*8);

	/* Restore the floating point save registers (8 instructions) */
	alpha_ldt(inst,ALPHA_FS0,ALPHA_SP, 8*8);
	alpha_ldt(inst,ALPHA_FS1,ALPHA_SP, 9*8);
	alpha_ldt(inst,ALPHA_FS2,ALPHA_SP,10*8);
	alpha_ldt(inst,ALPHA_FS3,ALPHA_SP,11*8);
	alpha_ldt(inst,ALPHA_FS4,ALPHA_SP,12*8);
	alpha_ldt(inst,ALPHA_FS5,ALPHA_SP,13*8);
	alpha_ldt(inst,ALPHA_FS6,ALPHA_SP,14*8);
	alpha_ldt(inst,ALPHA_FS7,ALPHA_SP,15*8);

	/* Restore the stack pointer (1 instruction) */
	alpha_lda(inst,ALPHA_SP,ALPHA_SP,16*8);

	/* Force any pending hardware exceptions to be raised. (1 instruction) */
	alpha_trapb(inst);

	/* Jump to the function that the redirector indicated (1 instruction) */
	alpha_jsr(inst,ALPHA_RA,ALPHA_V0,1);

	/* Return the start of the buffer as the redirector entry point */
	return (void *)buf;
}

void _jit_pad_buffer(unsigned char *buf, int len) {
	alpha_inst inst = (alpha_inst) buf;

	if (len > 0) {
		do {
			alpha_nop(inst);
		} while (--len);
	}
}

#endif /* alpha */
