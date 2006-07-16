/*
 * jit-apply-alpha.c - Apply support routines for alpha.
 *
 * Copyright (C) 2006  Southern Storm Software, Pty Ltd.
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

#if defined(__alpha) || defined(__alpha__)

#include "jit-gen-alpha.h"

void _jit_create_closure(unsigned char *buf, void *func, void *closure, void *_type) {
	alpha_inst inst = (alpha_inst) buf;

	/* Compute and load the global pointer */
	alpha_ldah(inst,ALPHA_GP,ALPHA_PV,0);
	alpha_lda( inst,ALPHA_GP,ALPHA_GP,0);

	/* Allocate space for a new stack frame. */
	alpha_lda(inst,ALPHA_SP,ALPHA_SP,-(14*8));

	/* Save the return address. */
	alpha_stq(inst,ALPHA_RA,ALPHA_SP,0*8);

	/* Save integer register arguments as local variables */
	alpha_stq(inst,ALPHA_A0,ALPHA_SP,1*8);
	alpha_stq(inst,ALPHA_A1,ALPHA_SP,2*8);
	alpha_stq(inst,ALPHA_A2,ALPHA_SP,3*8);
	alpha_stq(inst,ALPHA_A3,ALPHA_SP,4*8);
	alpha_stq(inst,ALPHA_A4,ALPHA_SP,5*8);
	alpha_stq(inst,ALPHA_A5,ALPHA_SP,6*8);

	/* Save floating-point register arguments as local variables */
	alpha_stt(inst,ALPHA_FA0,ALPHA_SP, 7*8);
	alpha_stt(inst,ALPHA_FA1,ALPHA_SP, 8*8);
	alpha_stt(inst,ALPHA_FA2,ALPHA_SP, 9*8);
	alpha_stt(inst,ALPHA_FA3,ALPHA_SP,10*8);
	alpha_stt(inst,ALPHA_FA4,ALPHA_SP,11*8);
	alpha_stt(inst,ALPHA_FA5,ALPHA_SP,12*8);

	/* Call the closure handling function */
	alpha_call(inst,func);

	/* Restore the return address */
	alpha_ldq(inst,ALPHA_RA,ALPHA_SP,0*8);

	/* Restore integer register arguments */
	alpha_ldq(inst,ALPHA_A0,ALPHA_SP,1*8);
	alpha_ldq(inst,ALPHA_A1,ALPHA_SP,2*8);
	alpha_ldq(inst,ALPHA_A2,ALPHA_SP,3*8);
	alpha_ldq(inst,ALPHA_A3,ALPHA_SP,4*8);
	alpha_ldq(inst,ALPHA_A4,ALPHA_SP,5*8);
	alpha_ldq(inst,ALPHA_A5,ALPHA_SP,6*8);

	/* Restore floating-point register arguments */
	alpha_ldt(inst,ALPHA_FA0,ALPHA_SP, 7*8);
	alpha_ldt(inst,ALPHA_FA1,ALPHA_SP, 8*8);
	alpha_ldt(inst,ALPHA_FA2,ALPHA_SP, 9*8);
	alpha_ldt(inst,ALPHA_FA3,ALPHA_SP,10*8);
	alpha_ldt(inst,ALPHA_FA4,ALPHA_SP,11*8);
	alpha_ldt(inst,ALPHA_FA5,ALPHA_SP,12*8);

	/* restore the stack pointer */
	alpha_lda(inst,ALPHA_SP,ALPHA_SP,(8*13));
}

void *_jit_create_redirector(unsigned char *buf, void *func, void *user_data, int abi) {
	alpha_inst inst = (alpha_inst) buf;

	/* NOT IMPLEMENTED YET! */

	/* Set up a new stack frame */

	/* Push the user data onto the stack "(int)(jit_nint)user_data" */	

	/* Call the redirector handling function */

	/* Jump to the function that the redirector indicated */
	alpha_jsr(inst,ALPHA_RA,ALPHA_R0,1);

	return (void *)inst;
}

#endif /* alpha */
