/*
 * jit-apply-x86-64.c - Apply support routines for x86_64.
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

#include "jit-internal.h"
#include "jit-apply-rules.h"
#include "jit-apply-func.h"

#if defined(__amd64) || defined(__amd64__) || defined(_x86_64) || defined(_x86_64__)

#include "jit-gen-x86-64.h"

void _jit_create_closure(unsigned char *buf, void *func,
                         void *closure, void *_type)
{
	jit_nint offset;
	jit_type_t signature = (jit_type_t)_type;

	/* Set up the local stack frame */
	x86_64_push_reg_size(buf, X86_64_RBP, 8);
	x86_64_mov_reg_reg_size(buf, X86_64_RBP, X86_64_RSP, 8);

	/* Create the apply argument block on the stack */
	x86_64_sub_reg_imm_size(buf, X86_64_RSP, 192, 8);

	/* fill the apply buffer */
	x86_64_mov_membase_reg_size(buf, X86_64_RSP, 0x08, X86_64_RDI, 8);
	x86_64_mov_membase_reg_size(buf, X86_64_RSP, 0x10, X86_64_RSI, 8);
	x86_64_mov_membase_reg_size(buf, X86_64_RSP, 0x18, X86_64_RDX, 8);
	x86_64_mov_membase_reg_size(buf, X86_64_RSP, 0x20, X86_64_RCX, 8);
	x86_64_mov_membase_reg_size(buf, X86_64_RSP, 0x28, X86_64_R8, 8);
	x86_64_mov_membase_reg_size(buf, X86_64_RSP, 0x30, X86_64_R9, 8);

	x86_64_movaps_membase_reg(buf, X86_64_RSP, 0x40, X86_64_XMM0);
	x86_64_movaps_membase_reg(buf, X86_64_RSP, 0x50, X86_64_XMM1);
	x86_64_movaps_membase_reg(buf, X86_64_RSP, 0x60, X86_64_XMM2);
	x86_64_movaps_membase_reg(buf, X86_64_RSP, 0x70, X86_64_XMM3);
	x86_64_movaps_membase_reg(buf, X86_64_RSP, 0x80, X86_64_XMM4);
	x86_64_movaps_membase_reg(buf, X86_64_RSP, 0x90, X86_64_XMM5);
	x86_64_movaps_membase_reg(buf, X86_64_RSP, 0xA0, X86_64_XMM6);
	x86_64_movaps_membase_reg(buf, X86_64_RSP, 0xB0, X86_64_XMM7);

	/* Now fill the arguments for the closure function */
	/* the closure function is #1 */
	x86_64_mov_reg_imm_size(buf, X86_64_RDI, (jit_nint)closure, 8);
	/* the apply buff is #2 */
	x86_64_mov_reg_reg_size(buf, X86_64_RSI, X86_64_RSP, 8);

	/* Call the closure handling function */
	offset = (jit_nint)func - ((jit_nint)buf + 5);
	if((offset < jit_min_int) || (offset > jit_max_int))
	{
		/* offset is outside the 32 bit offset range */
		/* so we have to do an indirect call */
		/* We use R11 here because it's the only temporary caller saved */
		/* register not used for argument passing. */
		x86_64_mov_reg_imm_size(buf, X86_64_R11, (jit_nint)func, 8);
		x86_64_call_reg(buf, X86_64_R11);
	}
	else
	{
		x86_64_call_imm(buf, (jit_int)offset);
	}

	/* Pop the current stack frame */
	x86_64_mov_reg_reg_size(buf, X86_64_RSP, X86_64_RBP, 8);
	x86_64_pop_reg_size(buf, X86_64_RBP, 8);

	/* Return from the closure */
	x86_64_ret(buf);
}

void *_jit_create_redirector(unsigned char *buf, void *func,
							 void *user_data, int abi)
{
	jit_nint offset;
	void *start = (void *)buf;

	/* Save all registers used for argument passing */
	/* At this point RSP is not aligned on a 16 byte boundary because */
	/* the return address is pushed on the stack. */
	/* We need (7 * 8) + (8 * 8) bytes for the registers */
	x86_64_sub_reg_imm_size(buf, X86_64_RSP, 0xB8, 8);

	x86_64_mov_membase_reg_size(buf, X86_64_RSP, 0xB0, X86_64_RAX, 8);
	x86_64_mov_membase_reg_size(buf, X86_64_RSP, 0xA8, X86_64_RDI, 8);
	x86_64_mov_membase_reg_size(buf, X86_64_RSP, 0xA0, X86_64_RSI, 8);
	x86_64_mov_membase_reg_size(buf, X86_64_RSP, 0x98, X86_64_RDX, 8);
	x86_64_mov_membase_reg_size(buf, X86_64_RSP, 0x90, X86_64_RCX, 8);
	x86_64_mov_membase_reg_size(buf, X86_64_RSP, 0x88, X86_64_R8, 8);
	x86_64_mov_membase_reg_size(buf, X86_64_RSP, 0x80, X86_64_R9, 8);
	x86_64_movaps_membase_reg(buf, X86_64_RSP, 0x70, X86_64_XMM0);
	x86_64_movaps_membase_reg(buf, X86_64_RSP, 0x60, X86_64_XMM1);
	x86_64_movaps_membase_reg(buf, X86_64_RSP, 0x50, X86_64_XMM2);
	x86_64_movaps_membase_reg(buf, X86_64_RSP, 0x40, X86_64_XMM3);
	x86_64_movaps_membase_reg(buf, X86_64_RSP, 0x30, X86_64_XMM4);
	x86_64_movaps_membase_reg(buf, X86_64_RSP, 0x20, X86_64_XMM5);
	x86_64_movaps_membase_reg(buf, X86_64_RSP, 0x10, X86_64_XMM6);
	x86_64_movaps_membase_reg(buf, X86_64_RSP, 0x00, X86_64_XMM7);

	/* Fill the pointer to the stack args */
	x86_64_lea_membase_size(buf, X86_64_RDI, X86_64_RSP, 0xD0, 8);
	x86_64_mov_regp_reg_size(buf, X86_64_RSP, X86_64_RDI, 8);

	/* Load the user data argument */
	x86_64_mov_reg_imm_size(buf, X86_64_RDI, (jit_nint)user_data, 8);

	/* Call "func" (the pointer result will be in RAX) */
	offset = (jit_nint)func - ((jit_nint)buf + 5);
	if((offset < jit_min_int) || (offset > jit_max_int))
	{
		/* offset is outside the 32 bit offset range */
		/* so we have to do an indirect call */
		/* We use R11 here because it's the only temporary caller saved */
		/* register not used for argument passing. */
		x86_64_mov_reg_imm_size(buf, X86_64_R11, (jit_nint)func, 8);
		x86_64_call_reg(buf, X86_64_R11);
	}
	else
	{
		x86_64_call_imm(buf, (jit_int)offset);
	}

	/* store the returned address in R11 */
	x86_64_mov_reg_reg_size(buf, X86_64_R11, X86_64_RAX, 8);

	/* Restore the argument registers */
	x86_64_mov_reg_membase_size(buf, X86_64_RAX, X86_64_RSP, 0xB0, 8);
	x86_64_mov_reg_membase_size(buf, X86_64_RDI, X86_64_RSP, 0xA8, 8);
	x86_64_mov_reg_membase_size(buf, X86_64_RSI, X86_64_RSP, 0xA0, 8);
	x86_64_mov_reg_membase_size(buf, X86_64_RDX, X86_64_RSP, 0x98, 8);
	x86_64_mov_reg_membase_size(buf, X86_64_RCX, X86_64_RSP, 0x90, 8);
	x86_64_mov_reg_membase_size(buf, X86_64_R8, X86_64_RSP, 0x88, 8);
	x86_64_mov_reg_membase_size(buf, X86_64_R9, X86_64_RSP, 0x80, 8);
	x86_64_movaps_reg_membase(buf, X86_64_XMM0, X86_64_RSP, 0x70);
	x86_64_movaps_reg_membase(buf, X86_64_XMM1, X86_64_RSP, 0x60);
	x86_64_movaps_reg_membase(buf, X86_64_XMM2, X86_64_RSP, 0x50);
	x86_64_movaps_reg_membase(buf, X86_64_XMM3, X86_64_RSP, 0x40);
	x86_64_movaps_reg_membase(buf, X86_64_XMM4, X86_64_RSP, 0x30);
	x86_64_movaps_reg_membase(buf, X86_64_XMM5, X86_64_RSP, 0x20);
	x86_64_movaps_reg_membase(buf, X86_64_XMM6, X86_64_RSP, 0x10);
	x86_64_movaps_reg_membase(buf, X86_64_XMM7, X86_64_RSP, 0x00);

	/* Restore the stack pointer */
	x86_64_add_reg_imm_size(buf, X86_64_RSP, 0xB8, 8);

	/* Jump to the function that the redirector indicated */
	x86_64_jmp_reg(buf, X86_64_R11);

	/* Return the start of the buffer as the redirector entry point */
	return start;
}

void *_jit_create_indirector(unsigned char *buf, void **entry)
{
	void *start = (void *)buf;

	/* Jump to the entry point. */
	if(((jit_nint)entry >= jit_min_int) && ((jit_nint)entry <= jit_max_int))
	{
		/* We are in the 32bit range so we can use the entry directly. */
		x86_64_jmp_mem(buf, (jit_nint)entry);
	}
	else
	{
		jit_nint offset = (jit_nint)entry - ((jit_nint)buf + 7);

		if((offset >= jit_min_int) && (offset <= jit_max_int))
		{
			/* We are in the 32bit range so we can use RIP relative addressing. */
			x86_64_jmp_membase(buf, X86_64_RIP, offset);
		}
		else
		{
			/* offset is outside the 32 bit offset range */
			/* so we have to do an indirect jump via register. */
			x86_64_mov_reg_imm_size(buf, X86_64_R11, (jit_nint)entry, 8);
			x86_64_jmp_regp(buf, X86_64_R11);
		}
	}

	return start;
}

void _jit_pad_buffer(unsigned char *buf, int len)
{
	while(len >= 6)
	{
		/* "leal 0(%esi), %esi" with 32-bit displacement */
		*buf++ = (unsigned char)0x8D;
		x86_address_byte(buf, 2, X86_ESI, X86_ESI);
		x86_imm_emit32(buf, 0);
		len -= 6;
	}
	if(len >= 3)
	{
		/* "leal 0(%esi), %esi" with 8-bit displacement */
		*buf++ = (unsigned char)0x8D;
		x86_address_byte(buf, 1, X86_ESI, X86_ESI);
		x86_imm_emit8(buf, 0);
		len -= 3;
	}
	if(len == 1)
	{
		/* Traditional x86 NOP */
		x86_nop(buf);
	}
	else if(len == 2)
	{
		/* movl %esi, %esi */
		x86_mov_reg_reg(buf, X86_ESI, X86_ESI, 4);
	}
}

#endif /* x86-64 */
