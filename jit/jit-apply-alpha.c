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

	/* NOT IMPLEMENTED YET! */

	/* Set up the local stack frame */
	/* Create the apply argument block on the stack */
	/* Push the arguments for calling "func" */
	/* Call the closure handling function */

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
