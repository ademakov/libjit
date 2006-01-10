/*
 * jit-rules-x86.c - Rules that define the characteristics of the x86.
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
#include "jit-rules.h"
#include "jit-apply-rules.h"

#if defined(JIT_BACKEND_X86)

#include "jit-gen-x86.h"
#include "jit-reg-alloc.h"
#include "jit-setjmp.h"
#include <stdio.h>

/*
 * Pseudo register numbers for the x86 registers.  These are not the
 * same as the CPU instruction register numbers.  The order of these
 * values must match the order in "JIT_REG_INFO".
 */
#define	X86_REG_EAX			0
#define	X86_REG_ECX			1
#define	X86_REG_EDX			2
#define	X86_REG_EBX			3
#define	X86_REG_ESI			4
#define	X86_REG_EDI			5
#define	X86_REG_EBP			6
#define	X86_REG_ESP			7
#define	X86_REG_ST0			8
#define	X86_REG_ST1			9
#define	X85_REG_ST2			10
#define	X86_REG_ST3			11
#define	X86_REG_ST4			12
#define	X86_REG_ST5			13
#define	X86_REG_ST6			14
#define	X86_REG_ST7			15

/*
 * Determine if a pseudo register number is word-based or float-based.
 */
#define	IS_WORD_REG(reg)	((reg) < X86_REG_ST0)
#define	IS_FLOAT_REG(reg)	((reg) >= X86_REG_ST0)

/*
 * Round a size up to a multiple of the stack word size.
 */
#define	ROUND_STACK(size)	\
		(((size) + (sizeof(void *) - 1)) & ~(sizeof(void *) - 1))

void _jit_init_backend(void)
{
	/* Nothing to do here for the x86 */
}

void _jit_gen_get_elf_info(jit_elf_info_t *info)
{
#ifdef JIT_NATIVE_INT32
	info->machine = 3;	/* EM_386 */
#else
	info->machine = 62;	/* EM_X86_64 */
#endif
#if JIT_APPLY_X86_FASTCALL == 0
	info->abi = 0;		/* ELFOSABI_SYSV */
#else
	info->abi = 186;	/* Private code, indicating STDCALL/FASTCALL support */
#endif
	info->abi_version = 0;
}

int _jit_setup_indirect_pointer(jit_function_t func, jit_value_t value)
{
	return jit_insn_outgoing_reg(func, value, X86_REG_EAX);
}

int _jit_create_call_return_insns
	(jit_function_t func, jit_type_t signature,
	 jit_value_t *args, unsigned int num_args,
	 jit_value_t return_value, int is_nested)
{
	jit_nint pop_bytes;
	unsigned int size;
	jit_type_t return_type;
	int ptr_return;

	/* Calculate the number of bytes that we need to pop */
	return_type = jit_type_normalize(jit_type_get_return(signature));
	ptr_return = jit_type_return_via_pointer(return_type);
#if JIT_APPLY_X86_FASTCALL == 1
	if(jit_type_get_abi(signature) == jit_abi_stdcall ||
	   jit_type_get_abi(signature) == jit_abi_fastcall)
	{
		/* STDCALL and FASTCALL functions pop their own arguments */
		pop_bytes = 0;
	}
	else
#endif
	{
		pop_bytes = 0;
		while(num_args > 0)
		{
			--num_args;
			size = jit_type_get_size(jit_value_get_type(args[num_args]));
			pop_bytes += ROUND_STACK(size);
		}
#if JIT_APPLY_X86_POP_STRUCT_RETURN == 1
		if(ptr_return && is_nested)
		{
			/* Note: we only need this for nested functions, because
			   regular functions will pop the structure return for us */
			pop_bytes += sizeof(void *);
		}
#else
		if(ptr_return)
		{
			pop_bytes += sizeof(void *);
		}
#endif
		if(is_nested)
		{
			pop_bytes += sizeof(void *);
		}
	}

	/* Pop the bytes from the system stack */
	if(pop_bytes > 0)
	{
		if(!jit_insn_defer_pop_stack(func, pop_bytes))
		{
			return 0;
		}
	}

	/* Bail out now if we don't need to worry about return values */
	if(!return_value || ptr_return)
	{
		return 0;
	}

	/* Structure values must be flushed into the frame, and
	   everything else ends up in a register */
	if(jit_type_is_struct(return_type) || jit_type_is_union(return_type))
	{
		if(!jit_insn_flush_struct(func, return_value))
		{
			return 0;
		}
	}
	else if(return_type == jit_type_float32 ||
			return_type == jit_type_float64 ||
			return_type == jit_type_nfloat)
	{
		if(!jit_insn_return_reg(func, return_value, X86_REG_ST0))
		{
			return 0;
		}
	}
	else if(return_type->kind != JIT_TYPE_VOID)
	{
		if(!jit_insn_return_reg(func, return_value, X86_REG_EAX))
		{
			return 0;
		}
	}

	/* Everything is back where it needs to be */
	return 1;
}

int _jit_opcode_is_supported(int opcode)
{
	switch(opcode)
	{
		#define JIT_INCLUDE_SUPPORTED
		#include "jit-rules-x86.slc"
		#undef JIT_INCLUDE_SUPPORTED
	}
	return 0;
}

void *_jit_gen_prolog(jit_gencode_t gen, jit_function_t func, void *buf)
{
	unsigned char prolog[JIT_PROLOG_SIZE];
	unsigned char *inst = prolog;
	int reg;

	/* Push ebp onto the stack */
	x86_push_reg(inst, X86_EBP);

	/* Initialize EBP for the current frame */
	x86_mov_reg_reg(inst, X86_EBP, X86_ESP, sizeof(void *));

	/* Allocate space for the local variable frame */
	if(func->builder->frame_size > 0)
	{
		x86_alu_reg_imm(inst, X86_SUB, X86_ESP,
			 			(int)(func->builder->frame_size));
	}

	/* Save registers that we need to preserve */
	for(reg = 0; reg <= 7; ++reg)
	{
		if(jit_reg_is_used(gen->touched, reg) &&
		   (_jit_reg_info[reg].flags & JIT_REG_CALL_USED) == 0)
		{
			x86_push_reg(inst, _jit_reg_info[reg].cpu_reg);
		}
	}

	/* Copy the prolog into place and return the adjusted entry position */
	reg = (int)(inst - prolog);
	jit_memcpy(((unsigned char *)buf) + JIT_PROLOG_SIZE - reg, prolog, reg);
	return (void *)(((unsigned char *)buf) + JIT_PROLOG_SIZE - reg);
}

void _jit_gen_epilog(jit_gencode_t gen, jit_function_t func)
{
	jit_nint pop_bytes = 0;
	int reg, offset;
	unsigned char *inst;
	int struct_return_offset = 0;
	void **fixup;
	void **next;

	/* Bail out if there is insufficient space for the epilog */
	if(!jit_cache_check_for_n(&(gen->posn), 48))
	{
		jit_cache_mark_full(&(gen->posn));
		return;
	}

#if JIT_APPLY_X86_FASTCALL == 1
	/* Determine the number of parameter bytes to pop when we return */
	{
		jit_type_t signature;
		unsigned int num_params;
		unsigned int param;
		signature = func->signature;
		if(jit_type_get_abi(signature) == jit_abi_stdcall ||
		   jit_type_get_abi(signature) == jit_abi_fastcall)
		{
			if(func->nested_parent)
			{
				pop_bytes += sizeof(void *);
			}
			if(jit_type_return_via_pointer(jit_type_get_return(signature)))
			{
				struct_return_offset = 2 * sizeof(void *) + pop_bytes;
				pop_bytes += sizeof(void *);
			}
			num_params = jit_type_num_params(signature);
			for(param = 0; param < num_params; ++param)
			{
				pop_bytes += ROUND_STACK
						(jit_type_get_size
							(jit_type_get_param(signature, param)));
			}
			if(jit_type_get_abi(signature) == jit_abi_fastcall)
			{
				/* The first two words are in fastcall registers */
				if(pop_bytes > (2 * sizeof(void *)))
				{
					pop_bytes -= 2 * sizeof(void *);
				}
				else
				{
					pop_bytes = 0;
				}
				struct_return_offset = 0;
			}
		}
		else if(!(func->nested_parent) &&
				jit_type_return_via_pointer(jit_type_get_return(signature)))
		{
#if JIT_APPLY_X86_POP_STRUCT_RETURN == 1
			pop_bytes += sizeof(void *);
#endif
			struct_return_offset = 2 * sizeof(void *);
		}
	}
#else
	{
		/* We only need to pop structure pointers in non-nested functions */
		jit_type_t signature;
		signature = func->signature;
		if(!(func->nested_parent) &&
		   jit_type_return_via_pointer(jit_type_get_return(signature)))
		{
#if JIT_APPLY_X86_POP_STRUCT_RETURN == 1
			pop_bytes += sizeof(void *);
#endif
			struct_return_offset = 2 * sizeof(void *);
		}
	}
#endif

	/* Perform fixups on any blocks that jump to the epilog */
	inst = gen->posn.ptr;
	fixup = (void **)(gen->epilog_fixup);
	while(fixup != 0)
	{
		next = (void **)(fixup[0]);
		fixup[0] = (void *)(((jit_nint)inst) - ((jit_nint)fixup) - 4);
		fixup = next;
	}
	gen->epilog_fixup = 0;

	/* If we are returning a structure via a pointer, then copy
	   the pointer value into EAX when we return */
	if(struct_return_offset != 0)
	{
		x86_mov_reg_membase(inst, X86_EAX, X86_EBP, struct_return_offset, 4);
	}

	/* Restore the callee save registers that we used */
	if(gen->stack_changed)
	{
		offset = -(func->builder->frame_size);
		for(reg = 0; reg <= 7; ++reg)
		{
			if(jit_reg_is_used(gen->touched, reg) &&
			   (_jit_reg_info[reg].flags & JIT_REG_CALL_USED) == 0)
			{
				offset -= sizeof(void *);
				x86_mov_reg_membase(inst, _jit_reg_info[reg].cpu_reg,
									X86_EBP, offset, sizeof(void *));
			}
		}
	}
	else
	{
		for(reg = 7; reg >= 0; --reg)
		{
			if(jit_reg_is_used(gen->touched, reg) &&
			   (_jit_reg_info[reg].flags & JIT_REG_CALL_USED) == 0)
			{
				x86_pop_reg(inst, _jit_reg_info[reg].cpu_reg);
			}
		}
	}

	/* Pop the stack frame and restore the saved copy of ebp */
	if(gen->stack_changed || func->builder->frame_size > 0)
	{
		x86_mov_reg_reg(inst, X86_ESP, X86_EBP, sizeof(void *));
	}
	x86_pop_reg(inst, X86_EBP);

	/* Return from the current function */
	if(pop_bytes > 0)
	{
		x86_ret_imm(inst, pop_bytes);
	}
	else
	{
		x86_ret(inst);
	}
	gen->posn.ptr = inst;
}

void *_jit_gen_redirector(jit_gencode_t gen, jit_function_t func)
{
	void *ptr, *entry;
	if(!jit_cache_check_for_n(&(gen->posn), 8))
	{
		jit_cache_mark_full(&(gen->posn));
		return 0;
	}
	ptr = (void *)&(func->entry_point);
	entry = gen->posn.ptr;
	x86_jump_mem(gen->posn.ptr, ptr);
	return entry;
}

/*
 * Setup or teardown the x86 code output process.
 */
#define	jit_cache_setup_output(needed)	\
	unsigned char *inst = gen->posn.ptr; \
	if(!jit_cache_check_for_n(&(gen->posn), (needed))) \
	{ \
		jit_cache_mark_full(&(gen->posn)); \
		return; \
	}
#define	jit_cache_end_output()	\
	gen->posn.ptr = inst

/*
 * Get a temporary register that isn't one of the specified registers.
 */
static int get_temp_reg(int reg1, int reg2, int reg3)
{
	if(reg1 != X86_EAX && reg2 != X86_EAX && reg3 != X86_EAX)
	{
		return X86_EAX;
	}
	if(reg1 != X86_EDX && reg2 != X86_EDX && reg3 != X86_EDX)
	{
		return X86_EDX;
	}
	if(reg1 != X86_ECX && reg2 != X86_ECX && reg3 != X86_ECX)
	{
		return X86_ECX;
	}
	if(reg1 != X86_EBX && reg2 != X86_EBX && reg3 != X86_EBX)
	{
		return X86_EBX;
	}
	if(reg1 != X86_ESI && reg2 != X86_ESI && reg3 != X86_ESI)
	{
		return X86_ESI;
	}
	return X86_EDI;
}

/*
 * Load a small structure from a pointer into registers.
 */
static unsigned char *load_small_struct
	(unsigned char *inst, int reg, int other_reg,
	 int base_reg, jit_nint offset, jit_nint size, int save_temp)
{
	int temp_reg;
	switch(size)
	{
		case 1:
		{
			x86_widen_membase(inst, reg, base_reg, offset, 0, 0);
		}
		break;

		case 2:
		{
			x86_widen_membase(inst, reg, base_reg, offset, 0, 1);
		}
		break;

		case 3:
		{
			temp_reg = get_temp_reg(reg, -1, base_reg);
			if(save_temp || temp_reg >= X86_EBX)
			{
				x86_push_reg(inst, temp_reg);
			}
			x86_widen_membase(inst, temp_reg, base_reg, offset, 0, 1);
			x86_widen_membase(inst, reg, base_reg, offset + 2, 0, 0);
			x86_shift_reg_imm(inst, X86_SHL, reg, 16);
			x86_alu_reg_reg(inst, X86_OR, reg, temp_reg);
			if(save_temp || temp_reg >= X86_EBX)
			{
				x86_pop_reg(inst, temp_reg);
			}
		}
		break;

		case 4:
		{
			x86_mov_reg_membase(inst, reg, base_reg, offset, 4);
		}
		break;

		case 5:
		{
			if(reg != base_reg)
			{
				x86_mov_reg_membase(inst, reg, base_reg, offset, 4);
				x86_widen_membase(inst, other_reg, base_reg, offset + 4, 0, 0);
			}
			else
			{
				x86_widen_membase(inst, other_reg, base_reg, offset + 4, 0, 0);
				x86_mov_reg_membase(inst, reg, base_reg, offset, 4);
			}
		}
		break;

		case 6:
		{
			if(reg != base_reg)
			{
				x86_mov_reg_membase(inst, reg, base_reg, offset, 4);
				x86_widen_membase(inst, other_reg, base_reg, offset + 4, 0, 1);
			}
			else
			{
				x86_widen_membase(inst, other_reg, base_reg, offset + 4, 0, 1);
				x86_mov_reg_membase(inst, reg, base_reg, offset, 4);
			}
		}
		break;

		case 7:
		{
			temp_reg = get_temp_reg(reg, other_reg, base_reg);
			if(save_temp || temp_reg >= X86_EBX)
			{
				x86_push_reg(inst, temp_reg);
			}
			if(reg != base_reg && other_reg != base_reg)
			{
				x86_mov_reg_membase(inst, reg, base_reg, offset, 4);
				x86_widen_membase(inst, other_reg, base_reg, offset + 4, 0, 1);
				x86_widen_membase(inst, temp_reg, base_reg, offset + 6, 0, 0);
				x86_shift_reg_imm(inst, X86_SHL, temp_reg, 16);
				x86_alu_reg_reg(inst, X86_OR, other_reg, temp_reg);
			}
			else if(reg != base_reg)
			{
				/* other_reg == base_reg */
				x86_mov_reg_membase(inst, reg, base_reg, offset, 4);
				x86_widen_membase(inst, temp_reg, base_reg, offset + 6, 0, 0);
				x86_widen_membase(inst, other_reg, base_reg, offset + 4, 0, 1);
				x86_shift_reg_imm(inst, X86_SHL, temp_reg, 16);
				x86_alu_reg_reg(inst, X86_OR, other_reg, temp_reg);
			}
			else
			{
				/* reg == base_reg */
				x86_widen_membase(inst, other_reg, base_reg, offset + 4, 0, 1);
				x86_widen_membase(inst, temp_reg, base_reg, offset + 6, 0, 0);
				x86_shift_reg_imm(inst, X86_SHL, temp_reg, 16);
				x86_alu_reg_reg(inst, X86_OR, other_reg, temp_reg);
				x86_mov_reg_membase(inst, reg, base_reg, offset, 4);
			}
			if(save_temp || temp_reg >= X86_EBX)
			{
				x86_pop_reg(inst, temp_reg);
			}
		}
		break;

		case 8:
		{
			if(reg != base_reg)
			{
				x86_mov_reg_membase(inst, reg, base_reg, offset, 4);
				x86_mov_reg_membase(inst, other_reg, base_reg, offset + 4, 4);
			}
			else
			{
				x86_mov_reg_membase(inst, other_reg, base_reg, offset + 4, 4);
				x86_mov_reg_membase(inst, reg, base_reg, offset, 4);
			}
		}
		break;
	}
	return inst;
}

/*
 * Store a byte value to a membase address.
 */
static unsigned char *mov_membase_reg_byte
			(unsigned char *inst, int basereg, int offset, int srcreg)
{
	if(srcreg == X86_EAX || srcreg == X86_EBX ||
	   srcreg == X86_ECX || srcreg == X86_EDX)
	{
		x86_mov_membase_reg(inst, basereg, offset, srcreg, 1);
	}
	else if(basereg != X86_EAX)
	{
		x86_push_reg(inst, X86_EAX);
		x86_mov_reg_reg(inst, X86_EAX, srcreg, 4);
		x86_mov_membase_reg(inst, basereg, offset, X86_EAX, 1);
		x86_pop_reg(inst, X86_EAX);
	}
	else
	{
		x86_push_reg(inst, X86_EDX);
		x86_mov_reg_reg(inst, X86_EDX, srcreg, 4);
		x86_mov_membase_reg(inst, basereg, offset, X86_EDX, 1);
		x86_pop_reg(inst, X86_EDX);
	}
	return inst;
}

/*
 * Store a small structure from registers to a pointer.  The base
 * register must not be either "reg" or "other_reg".
 */
static unsigned char *store_small_struct
	(unsigned char *inst, int reg, int other_reg,
	 int base_reg, jit_nint offset, jit_nint size, int preserve)
{
	switch(size)
	{
		case 1:
		{
			inst = mov_membase_reg_byte(inst, base_reg, offset, reg);
		}
		break;

		case 2:
		{
			x86_mov_membase_reg(inst, base_reg, offset, reg, 2);
		}
		break;

		case 3:
		{
			if(preserve)
			{
				x86_push_reg(inst, reg);
			}
			x86_mov_membase_reg(inst, base_reg, offset, reg, 2);
			x86_shift_reg_imm(inst, reg, X86_SHR, 16);
			inst = mov_membase_reg_byte(inst, base_reg, offset + 2, reg);
			if(preserve)
			{
				x86_pop_reg(inst, reg);
			}
		}
		break;

		case 4:
		{
			x86_mov_membase_reg(inst, base_reg, offset, reg, 4);
		}
		break;

		case 5:
		{
			x86_mov_membase_reg(inst, base_reg, offset, reg, 4);
			inst = mov_membase_reg_byte(inst, base_reg, offset + 4, other_reg);
		}
		break;

		case 6:
		{
			x86_mov_membase_reg(inst, base_reg, offset, reg, 4);
			x86_mov_membase_reg(inst, base_reg, offset + 4, other_reg, 2);
		}
		break;

		case 7:
		{
			if(preserve)
			{
				x86_push_reg(inst, other_reg);
			}
			x86_mov_membase_reg(inst, base_reg, offset, reg, 4);
			x86_mov_membase_reg(inst, base_reg, offset + 4, other_reg, 2);
			x86_shift_reg_imm(inst, other_reg, X86_SHR, 16);
			inst = mov_membase_reg_byte(inst, base_reg, offset + 6, other_reg);
			if(preserve)
			{
				x86_pop_reg(inst, other_reg);
			}
		}
		break;

		case 8:
		{
			x86_mov_membase_reg(inst, base_reg, offset, reg, 4);
			x86_mov_membase_reg(inst, base_reg, offset + 4, other_reg, 4);
		}
		break;
	}
	return inst;
}

void _jit_gen_spill_reg(jit_gencode_t gen, int reg,
						int other_reg, jit_value_t value)
{
	int offset;

	/* Make sure that we have sufficient space */
	jit_cache_setup_output(16);

	/* If the value is associated with a global register, then copy to that */
	if(value->has_global_register)
	{
		reg = _jit_reg_info[reg].cpu_reg;
		other_reg = _jit_reg_info[value->global_reg].cpu_reg;
		x86_mov_reg_reg(inst, other_reg, reg, sizeof(void *));
		jit_cache_end_output();
		return;
	}

	/* Fix the value in place within the local variable frame */
	_jit_gen_fix_value(value);

	/* Output an appropriate instruction to spill the value */
	offset = (int)(value->frame_offset);
	if(IS_WORD_REG(reg))
	{
		/* Spill a word register.  If the value is smaller than a word,
		   then we write the entire word.  The local stack frame is
		   allocated such that the extra bytes will be simply ignored */
		reg = _jit_reg_info[reg].cpu_reg;
		x86_mov_membase_reg(inst, X86_EBP, offset, reg, 4);
		if(other_reg != -1)
		{
			/* Spill the other word register in a pair */
			reg = _jit_reg_info[other_reg].cpu_reg;
			offset += sizeof(void *);
			x86_mov_membase_reg(inst, X86_EBP, offset, reg, 4);
		}
	}
	else
	{
		/* Spill the top of the floating-point register stack */
		switch(jit_type_normalize(value->type)->kind)
		{
			case JIT_TYPE_FLOAT32:
			{
				x86_fst_membase(inst, X86_EBP, offset, 0, 1);
			}
			break;

			case JIT_TYPE_FLOAT64:
			{
				x86_fst_membase(inst, X86_EBP, offset, 1, 1);
			}
			break;

			case JIT_TYPE_NFLOAT:
			{
				x86_fst80_membase(inst, X86_EBP, offset);
			}
			break;
		}
	}

	/* End the code output process */
	jit_cache_end_output();
}

void _jit_gen_free_reg(jit_gencode_t gen, int reg,
					   int other_reg, int value_used)
{
	/* We only need to take explicit action if we are freeing a
	   floating-point register whose value hasn't been used yet */
	if(!value_used && IS_FLOAT_REG(reg))
	{
		if(jit_cache_check_for_n(&(gen->posn), 2))
		{
			x86_fstp(gen->posn.ptr, reg - X86_REG_ST0);
		}
		else
		{
			jit_cache_mark_full(&(gen->posn));
		}
	}
}

void _jit_gen_load_value
	(jit_gencode_t gen, int reg, int other_reg, jit_value_t value)
{
	void *ptr;
	int offset;

	/* Make sure that we have sufficient space */
	jit_cache_setup_output(16);

	if(value->is_constant)
	{
		/* Determine the type of constant to be loaded */
		switch(jit_type_normalize(value->type)->kind)
		{
			case JIT_TYPE_SBYTE:
			case JIT_TYPE_UBYTE:
			case JIT_TYPE_SHORT:
			case JIT_TYPE_USHORT:
			case JIT_TYPE_INT:
			case JIT_TYPE_UINT:
			{
				x86_mov_reg_imm(inst, _jit_reg_info[reg].cpu_reg,
								(jit_nint)(value->address));
			}
			break;

			case JIT_TYPE_LONG:
			case JIT_TYPE_ULONG:
			{
				jit_long long_value;
				long_value = jit_value_get_long_constant(value);
			#ifdef JIT_NATIVE_INT64
				x86_mov_reg_imm(inst, _jit_reg_info[reg].cpu_reg,
								(jit_nint)long_value);
			#else
				x86_mov_reg_imm(inst, _jit_reg_info[reg].cpu_reg,
								(jit_int)long_value);
				x86_mov_reg_imm(inst, _jit_reg_info[other_reg].cpu_reg,
								(jit_int)(long_value >> 32));
			#endif
			}
			break;

			case JIT_TYPE_FLOAT32:
			{
				jit_float32 float32_value;
				float32_value = jit_value_get_float32_constant(value);
				if(!jit_cache_check_for_n(&(gen->posn), 32))
				{
					jit_cache_mark_full(&(gen->posn));
					return;
				}
				if(IS_WORD_REG(reg))
				{
					x86_mov_reg_imm(inst, _jit_reg_info[reg].cpu_reg,
									((jit_int *)&float32_value)[0]);
				}
				else
				{
					ptr = _jit_cache_alloc(&(gen->posn), sizeof(jit_float32));
					jit_memcpy(ptr, &float32_value, sizeof(float32_value));
					x86_fld(inst, ptr, 0);
				}
			}
			break;

			case JIT_TYPE_FLOAT64:
			{
				jit_float64 float64_value;
				float64_value = jit_value_get_float64_constant(value);
				if(!jit_cache_check_for_n(&(gen->posn), 32))
				{
					jit_cache_mark_full(&(gen->posn));
					return;
				}
				if(IS_WORD_REG(reg))
				{
					x86_mov_reg_imm(inst, _jit_reg_info[reg].cpu_reg,
									((jit_int *)&float64_value)[0]);
					x86_mov_reg_imm(inst, _jit_reg_info[other_reg].cpu_reg,
									((jit_int *)&float64_value)[1]);
				}
				else
				{
					ptr = _jit_cache_alloc(&(gen->posn), sizeof(jit_float64));
					jit_memcpy(ptr, &float64_value, sizeof(float64_value));
					x86_fld(inst, ptr, 1);
				}
			}
			break;

			case JIT_TYPE_NFLOAT:
			{
				jit_nfloat nfloat_value;
				nfloat_value = jit_value_get_nfloat_constant(value);
				if(!jit_cache_check_for_n(&(gen->posn), 32))
				{
					jit_cache_mark_full(&(gen->posn));
					return;
				}
				if(IS_WORD_REG(reg) &&
				   sizeof(jit_nfloat) == sizeof(jit_float64))
				{
					x86_mov_reg_imm(inst, _jit_reg_info[reg].cpu_reg,
									((jit_int *)&nfloat_value)[0]);
					x86_mov_reg_imm(inst, _jit_reg_info[other_reg].cpu_reg,
									((jit_int *)&nfloat_value)[1]);
				}
				else
				{
					ptr = _jit_cache_alloc(&(gen->posn), sizeof(jit_nfloat));
					jit_memcpy(ptr, &nfloat_value, sizeof(nfloat_value));
					if(sizeof(jit_nfloat) != sizeof(jit_float64))
					{
						x86_fld80_mem(inst, ptr);
					}
					else
					{
						x86_fld(inst, ptr, 1);
					}
				}
			}
			break;
		}
	}
	else if(value->has_global_register)
	{
		/* Load the value out of a global register */
		x86_mov_reg_reg(inst, _jit_reg_info[reg].cpu_reg,
						_jit_reg_info[value->global_reg].cpu_reg,
						sizeof(void *));
	}
	else
	{
		/* Fix the position of the value in the stack frame */
		_jit_gen_fix_value(value);
		offset = (int)(value->frame_offset);

		/* Load the value into the specified register */
		switch(jit_type_normalize(value->type)->kind)
		{
			case JIT_TYPE_SBYTE:
			{
				x86_widen_membase(inst, _jit_reg_info[reg].cpu_reg,
								  X86_EBP, offset, 1, 0);
			}
			break;

			case JIT_TYPE_UBYTE:
			{
				x86_widen_membase(inst, _jit_reg_info[reg].cpu_reg,
								  X86_EBP, offset, 0, 0);
			}
			break;

			case JIT_TYPE_SHORT:
			{
				x86_widen_membase(inst, _jit_reg_info[reg].cpu_reg,
								  X86_EBP, offset, 1, 1);
			}
			break;

			case JIT_TYPE_USHORT:
			{
				x86_widen_membase(inst, _jit_reg_info[reg].cpu_reg,
								  X86_EBP, offset, 0, 1);
			}
			break;

			case JIT_TYPE_INT:
			case JIT_TYPE_UINT:
			{
				x86_mov_reg_membase(inst, _jit_reg_info[reg].cpu_reg,
									X86_EBP, offset, 4);
			}
			break;

			case JIT_TYPE_LONG:
			case JIT_TYPE_ULONG:
			{
			#ifdef JIT_NATIVE_INT64
				x86_mov_reg_membase(inst, _jit_reg_info[reg].cpu_reg,
									X86_EBP, offset, 8);
			#else
				x86_mov_reg_membase(inst, _jit_reg_info[reg].cpu_reg,
									X86_EBP, offset, 4);
				x86_mov_reg_membase(inst, _jit_reg_info[other_reg].cpu_reg,
									X86_EBP, offset + 4, 4);
			#endif
			}
			break;

			case JIT_TYPE_FLOAT32:
			{
				if(IS_WORD_REG(reg))
				{
					x86_mov_reg_membase(inst, _jit_reg_info[reg].cpu_reg,
										X86_EBP, offset, 4);
				}
				else
				{
					x86_fld_membase(inst, X86_EBP, offset, 0);
				}
			}
			break;

			case JIT_TYPE_FLOAT64:
			{
				if(IS_WORD_REG(reg))
				{
					x86_mov_reg_membase(inst, _jit_reg_info[reg].cpu_reg,
										X86_EBP, offset, 4);
					x86_mov_reg_membase(inst, _jit_reg_info[other_reg].cpu_reg,
										X86_EBP, offset + 4, 4);
				}
				else
				{
					x86_fld_membase(inst, X86_EBP, offset, 1);
				}
			}
			break;

			case JIT_TYPE_NFLOAT:
			{
				if(IS_WORD_REG(reg) &&
				   sizeof(jit_nfloat) == sizeof(jit_float64))
				{
					x86_mov_reg_membase(inst, _jit_reg_info[reg].cpu_reg,
										X86_EBP, offset, 4);
					x86_mov_reg_membase(inst, _jit_reg_info[other_reg].cpu_reg,
										X86_EBP, offset + 4, 4);
				}
				else if(sizeof(jit_nfloat) == sizeof(jit_float64))
				{
					x86_fld_membase(inst, X86_EBP, offset, 1);
				}
				else
				{
					x86_fld80_membase(inst, X86_EBP, offset);
				}
			}
			break;
		}
	}

	/* End the code output process */
	jit_cache_end_output();
}

void _jit_gen_load_global(jit_gencode_t gen, jit_value_t value)
{
	jit_cache_setup_output(16);
	x86_mov_reg_membase(inst, _jit_reg_info[value->global_reg].cpu_reg,
						X86_EBP, value->frame_offset, sizeof(void *));
	jit_cache_end_output();
}

void _jit_gen_fix_value(jit_value_t value)
{
	if(!(value->has_frame_offset) && !(value->is_constant))
	{
		jit_nint size = (jit_nint)(ROUND_STACK(jit_type_get_size(value->type)));
		value->block->func->builder->frame_size += size;
		value->frame_offset = -(value->block->func->builder->frame_size);
		value->has_frame_offset = 1;
	}
}

/*
 * Widen a byte register.
 */
static unsigned char *widen_byte(unsigned char *inst, int reg, int isSigned)
{
	if(reg == X86_EAX || reg == X86_EBX || reg == X86_ECX || reg == X86_EDX)
	{
		x86_widen_reg(inst, reg, reg, isSigned, 0);
	}
	else
	{
		x86_push_reg(inst, X86_EAX);
		x86_mov_reg_reg(inst, X86_EAX, reg, 4);
		x86_widen_reg(inst, reg, X86_EAX, isSigned, 0);
		x86_pop_reg(inst, X86_EAX);
	}
	return inst;
}

/*
 * Shift the contents of a register.
 */
static unsigned char *shift_reg(unsigned char *inst, int opc, int reg, int reg2)
{
	if(reg2 == X86_ECX)
	{
		/* The shift value is already in ECX */
		x86_shift_reg(inst, opc, reg);
	}
	else if(reg == X86_ECX)
	{
		/* The value to be shifted is in ECX, so swap the order */
		x86_xchg_reg_reg(inst, reg, reg2, 4);
		x86_shift_reg(inst, opc, reg2);
		x86_mov_reg_reg(inst, reg, reg2, 4);
	}
	else
	{
		/* Save ECX, perform the shift, and then restore ECX */
		x86_push_reg(inst, X86_ECX);
		x86_mov_reg_reg(inst, X86_ECX, reg2, 4);
		x86_shift_reg(inst, opc, reg);
		x86_pop_reg(inst, X86_ECX);
	}
	return inst;
}

/*
 * Set a register value based on a condition code.
 */
static unsigned char *setcc_reg
	(unsigned char *inst, int reg, int cond, int is_signed)
{
	if(reg == X86_EAX || reg == X86_EBX || reg == X86_ECX || reg == X86_EDX)
	{
		/* Use a SETcc instruction if we have a basic register */
		x86_set_reg(inst, cond, reg, is_signed);
		x86_widen_reg(inst, reg, reg, 0, 0);
	}
	else
	{
		/* The register is not useable as an 8-bit destination */
		unsigned char *patch1, *patch2;
		patch1 = inst;
		x86_branch8(inst, cond, 0, is_signed);
		x86_clear_reg(inst, reg);
		patch2 = inst;
		x86_jump8(inst, 0);
		x86_patch(patch1, inst);
		x86_mov_reg_imm(inst, reg, 1);
		x86_patch(patch2, inst);
	}
	return inst;
}

/*
 * Get the long form of a branch opcode.
 */
static int long_form_branch(int opcode)
{
	if(opcode == 0xEB)
	{
		return 0xE9;
	}
	else
	{
		return opcode + 0x0F10;
	}
}

/*
 * Output a branch instruction.
 */
static unsigned char *output_branch
	(jit_function_t func, unsigned char *inst, int opcode, jit_insn_t insn)
{
	jit_block_t block;
	int offset;
	if((insn->flags & JIT_INSN_VALUE1_IS_LABEL) != 0)
	{
		/* "address_of_label" instruction */
		block = jit_block_from_label(func, (jit_label_t)(insn->value1));
	}
	else
	{
		block = jit_block_from_label(func, (jit_label_t)(insn->dest));
	}
	if(!block)
	{
		return inst;
	}
	if(block->address)
	{
		/* We already know the address of the block */
		offset = ((unsigned char *)(block->address)) - (inst + 2);
		if(x86_is_imm8(offset))
		{
			/* We can output a short-form backwards branch */
			*inst++ = (unsigned char)opcode;
			*inst++ = (unsigned char)offset;
		}
		else
		{
			/* We need to output a long-form backwards branch */
			offset -= 3;
			opcode = long_form_branch(opcode);
			if(opcode < 256)
			{
				*inst++ = (unsigned char)opcode;
			}
			else
			{
				*inst++ = (unsigned char)(opcode >> 8);
				*inst++ = (unsigned char)opcode;
				--offset;
			}
			x86_imm_emit32(inst, offset);
		}
	}
	else
	{
		/* Output a placeholder and record on the block's fixup list */
		opcode = long_form_branch(opcode);
		if(opcode < 256)
		{
			*inst++ = (unsigned char)opcode;
		}
		else
		{
			*inst++ = (unsigned char)(opcode >> 8);
			*inst++ = (unsigned char)opcode;
		}
		x86_imm_emit32(inst, (int)(block->fixup_list));
		block->fixup_list = (void *)(inst - 4);
	}
	return inst;
}

/*
 * Jump to the current function's epilog.
 */
static unsigned char *jump_to_epilog
	(jit_gencode_t gen, unsigned char *inst, jit_block_t block)
{
	/* If the epilog is the next thing that we will output,
	   then fall through to the epilog directly */
	block = block->next;
	while(block != 0 && block->first_insn > block->last_insn)
	{
		block = block->next;
	}
	if(!block)
	{
		return inst;
	}

	/* Output a placeholder for the jump and add it to the fixup list */
	*inst++ = (unsigned char)0xE9;
	x86_imm_emit32(inst, (int)(gen->epilog_fixup));
	gen->epilog_fixup = (void *)(inst - 4);
	return inst;
}

/*
 * Store a byte value to a memindex address.
 */
static unsigned char *mov_memindex_reg_byte
		(unsigned char *inst, int basereg,
		 unsigned offset, int indexreg, int srcreg)
{
	if(srcreg == X86_EAX || srcreg == X86_EBX ||
	   srcreg == X86_ECX || srcreg == X86_EDX)
	{
		x86_mov_memindex_reg(inst, basereg, offset, indexreg,
							 0, srcreg, 1);
	}
	else
	{
		int tempreg;
		if(basereg != X86_EAX && indexreg != X86_EAX)
		{
			tempreg = X86_EAX;
		}
		else if(basereg != X86_ECX && indexreg != X86_ECX)
		{
			tempreg = X86_ECX;
		}
		else
		{
			tempreg = X86_EDX;
		}
		x86_push_reg(inst, tempreg);
		x86_mov_reg_reg(inst, tempreg, srcreg, 4);
		x86_mov_memindex_reg(inst, basereg, offset, indexreg,
							 0, tempreg, 1);
		x86_pop_reg(inst, tempreg);
	}
	return inst;
}

/*
 * Throw a builtin exception.
 */
static unsigned char *throw_builtin
		(unsigned char *inst, jit_function_t func, int type)
{
	/* We need to update "catch_pc" if we have a "try" block */
	if(func->builder->setjmp_value != 0)
	{
		_jit_gen_fix_value(func->builder->setjmp_value);
		x86_call_imm(inst, 0);
		x86_pop_membase(inst, X86_EBP,
						func->builder->setjmp_value->frame_offset +
						jit_jmp_catch_pc_offset);
	}

	/* Push the exception type onto the stack */
	x86_push_imm(inst, type);

	/* Call the "jit_exception_builtin" function, which will never return */
	x86_call_code(inst, jit_exception_builtin);
	return inst;
}

/*
 * Copy a block of memory that has a specific size.  Other than
 * the parameter pointers, all registers must be unused at this point.
 */
static unsigned char *memory_copy
	(jit_gencode_t gen, unsigned char *inst, int dreg, jit_nint doffset,
	 int sreg, jit_nint soffset, jit_nuint size)
{
	int temp_reg = get_temp_reg(dreg, sreg, -1);
	if(size <= 4 * sizeof(void *))
	{
		/* Use direct copies to copy the memory */
		int offset = 0;
		while(size >= sizeof(void *))
		{
			x86_mov_reg_membase(inst, temp_reg, sreg,
								soffset + offset, sizeof(void *));
			x86_mov_membase_reg(inst, dreg, doffset + offset,
							    temp_reg, sizeof(void *));
			size -= sizeof(void *);
			offset += sizeof(void *);
		}
	#ifdef JIT_NATIVE_INT64
		if(size >= 4)
		{
			x86_mov_reg_membase(inst, temp_reg, sreg, soffset + offset, 4);
			x86_mov_membase_reg(inst, dreg, doffset + offset, temp_reg, 4);
			size -= 4;
			offset += 4;
		}
	#endif
		if(size >= 2)
		{
			x86_mov_reg_membase(inst, temp_reg, sreg, soffset + offset, 2);
			x86_mov_membase_reg(inst, dreg, doffset + offset, temp_reg, 2);
			size -= 2;
			offset += 2;
		}
		if(size >= 1)
		{
			/* We assume that temp_reg is EAX, ECX, or EDX, which it
			   should be after calling "get_temp_reg" */
			x86_mov_reg_membase(inst, temp_reg, sreg, soffset + offset, 1);
			x86_mov_membase_reg(inst, dreg, doffset + offset, temp_reg, 1);
		}
	}
	else
	{
		/* Call out to "jit_memcpy" to effect the copy */
		x86_push_imm(inst, size);
		if(soffset == 0)
		{
			x86_push_reg(inst, sreg);
		}
		else
		{
			x86_lea_membase(inst, temp_reg, sreg, soffset);
			x86_push_reg(inst, temp_reg);
		}
		if(dreg != X86_ESP)
		{
			if(doffset == 0)
			{
				x86_push_reg(inst, dreg);
			}
			else
			{
				x86_lea_membase(inst, temp_reg, dreg, doffset);
				x86_push_reg(inst, temp_reg);
			}
		}
		else
		{
			/* Copying a structure value onto the stack */
			x86_lea_membase(inst, temp_reg, X86_ESP,
							doffset + 2 * sizeof(void *));
			x86_push_reg(inst, temp_reg);
		}
		x86_call_code(inst, jit_memcpy);
		x86_alu_reg_imm(inst, X86_ADD, X86_ESP, 3 * sizeof(void *));
	}
	return inst;
}

#define	TODO()		\
	do { \
		fprintf(stderr, "TODO at %s, %d\n", __FILE__, (int)__LINE__); \
	} while (0)

void _jit_gen_insn(jit_gencode_t gen, jit_function_t func,
				   jit_block_t block, jit_insn_t insn)
{
	switch(insn->opcode)
	{
		#define JIT_INCLUDE_RULES
		#include "jit-rules-x86.slc"
		#undef JIT_INCLUDE_RULES

		default:
		{
			fprintf(stderr, "TODO(%x) at %s, %d\n",
					(int)(insn->opcode), __FILE__, (int)__LINE__);
		}
		break;
	}
}

void _jit_gen_start_block(jit_gencode_t gen, jit_block_t block)
{
	void **fixup;
	void **next;

	/* Set the address of this block */
	block->address = (void *)(gen->posn.ptr);

	/* If this block has pending fixups, then apply them now */
	fixup = (void **)(block->fixup_list);
	while(fixup != 0)
	{
		next = (void **)(fixup[0]);
		fixup[0] = (void *)
			(((jit_nint)(block->address)) - ((jit_nint)fixup) - 4);
		fixup = next;
	}
	block->fixup_list = 0;

	fixup = (void**)(block->fixup_absolute_list);
	while(fixup != 0)
	{
		next = (void **)(fixup[0]);
		fixup[0] = (void *)((jit_nint)(block->address));
		fixup = next;
	}
	block->fixup_absolute_list = 0;
}

void _jit_gen_end_block(jit_gencode_t gen, jit_block_t block)
{
	/* Nothing to do here for x86 */
}

int _jit_gen_is_global_candidate(jit_type_t type)
{
	switch(jit_type_remove_tags(type)->kind)
	{
		case JIT_TYPE_INT:
		case JIT_TYPE_UINT:
		case JIT_TYPE_NINT:
		case JIT_TYPE_NUINT:
		case JIT_TYPE_PTR:
		case JIT_TYPE_SIGNATURE:	return 1;
	}
	return 0;
}

#endif /* JIT_BACKEND_X86 */
