/*
 * jit-rules-arm.c - Rules that define the characteristics of the ARM.
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

#if defined(JIT_BACKEND_ARM)

#include "jit-gen-arm.h"
#include "jit-reg-alloc.h"
#include "jit-setjmp.h"
#include <stdio.h>

/*
 * Round a size up to a multiple of the stack word size.
 */
#define	ROUND_STACK(size)	\
		(((size) + (sizeof(void *) - 1)) & ~(sizeof(void *) - 1))

void _jit_init_backend(void)
{
	/* Nothing to do here */
}

void _jit_gen_get_elf_info(jit_elf_info_t *info)
{
	info->machine = 40;		/* EM_ARM */
	info->abi = 0;			/* ELFOSABI_SYSV */
	info->abi_version = 0;
}

/*
 * Force values out of parameter registers that cannot be easily
 * accessed in register form (i.e. long, float, and struct values).
 */
static int force_out_of_regs(jit_function_t func, jit_value_t param,
							 int next_reg, unsigned int size)
{
	jit_value_t address;
	jit_value_t temp;
	jit_nint offset = 0;
	jit_nint frame_offset = sizeof(void *);

	/* Get the address of the parameter, to force it into the frame,
	   and to set up for the later "jit_insn_store_relative" calls */
	address = jit_insn_address_of(func, param);
	if(!address)
	{
		return 0;
	}

	/* Force the values out of the registers */
	while(next_reg < ARM_NUM_PARAM_REGS && size > 0)
	{
		temp = jit_value_create(func, jit_type_void_ptr);
		if(!temp)
		{
			return 0;
		}
		if(!jit_insn_incoming_reg(func, temp, next_reg))
		{
			return 0;
		}
		if(!jit_insn_store_relative(func, address, offset, temp))
		{
			return 0;
		}
		offset += sizeof(void *);
		size -= sizeof(void *);
		++next_reg;
	}


	/* Force the rest of the value out of the incoming stack frame */
	while(size > 0)
	{
		temp = jit_value_create(func, jit_type_void_ptr);
		if(!temp)
		{
			return 0;
		}
		if(!jit_insn_incoming_frame_posn(func, temp, frame_offset))
		{
			return 0;
		}
		if(!jit_insn_store_relative(func, address, offset, temp))
		{
			return 0;
		}
		offset += sizeof(void *);
		frame_offset += sizeof(void *);
		size -= sizeof(void *);
	}
	return 1;
}

int _jit_create_entry_insns(jit_function_t func)
{
	jit_type_t signature = func->signature;
	jit_type_t type;
	int next_reg;
	jit_nint offset;
	jit_value_t value;
	unsigned int num_params;
	unsigned int param;
	unsigned int size;

	/* Reset the frame size for this function.  We start by assuming
	   that lr, sp, fp, r8, r7, r6, r5, and r4 need to be saved in
	   the local frame, as that is the worst-case scenario */
	func->builder->frame_size = 8 * sizeof(void *);

	/* The next register to be allocated to parameters is r0 */
	next_reg = 0;

	/* The starting parameter offset (saved pc on stack) */
	offset = sizeof(void *);

	/* If the function is nested, then we need an extra parameter
	   to pass the pointer to the parent's local variable frame */
	if(func->nested_parent)
	{
		++next_reg;
	}

	/* Allocate the structure return pointer */
	value = jit_value_get_struct_pointer(func);
	if(value)
	{
		if(!jit_insn_incoming_reg(func, value, next_reg))
		{
			return 0;
		}
		++next_reg;
	}

	/* Allocate the parameter registers and offsets */
	num_params = jit_type_num_params(signature);
	for(param = 0; param < num_params; ++param)
	{
		value = jit_value_get_param(func, param);
		if(!value)
		{
			continue;
		}
		type = jit_type_normalize(jit_value_get_type(value));
		switch(type->kind)
		{
			case JIT_TYPE_SBYTE:
			case JIT_TYPE_UBYTE:
			case JIT_TYPE_SHORT:
			case JIT_TYPE_USHORT:
			case JIT_TYPE_INT:
			case JIT_TYPE_UINT:
			case JIT_TYPE_NINT:
			case JIT_TYPE_NUINT:
			case JIT_TYPE_SIGNATURE:
			case JIT_TYPE_PTR:
			{
				if(next_reg < ARM_NUM_PARAM_REGS)
				{
					if(!jit_insn_incoming_reg(func, value, next_reg))
					{
						return 0;
					}
					++next_reg;
				}
				else
				{
					if(!jit_insn_incoming_frame_posn(func, value, offset))
					{
						return 0;
					}
					offset += sizeof(void *);
				}
			}
			break;

			case JIT_TYPE_LONG:
			case JIT_TYPE_ULONG:
			case JIT_TYPE_FLOAT32:
			case JIT_TYPE_FLOAT64:
			case JIT_TYPE_NFLOAT:
			case JIT_TYPE_STRUCT:
			case JIT_TYPE_UNION:
			{
				/* Force these kinds of values out of the word registers.
				   While technically we could keep long and float values
				   in word registers on ARM, it simplifies the register
				   allocator if we force them out first */
				size = ROUND_STACK(jit_type_get_size(type));
				if(next_reg < ARM_NUM_PARAM_REGS)
				{
					if(!force_out_of_regs(func, value, next_reg, size))
					{
						return 0;
					}
					while(size > 0 && next_reg < ARM_NUM_PARAM_REGS)
					{
						++next_reg;
						size -= sizeof(void *);
					}
					offset += size;
				}
				else
				{
					/* The value is completely on the stack */
					if(!jit_insn_incoming_frame_posn(func, value, offset))
					{
						return 0;
					}
					offset += size;
				}
			}
			break;
		}
	}
	return 1;
}

int _jit_create_call_setup_insns
	(jit_function_t func, jit_type_t signature,
	 jit_value_t *args, unsigned int num_args,
	 int is_nested, int nesting_level, jit_value_t *struct_return)
{
	jit_type_t type = jit_type_get_return(signature);
	jit_value_t value;
	unsigned int size;
	unsigned int index;
	unsigned int num_stack_args;
	unsigned int word_regs;
	jit_value_t partial;

	/* Determine which values are going to end up in registers */
	word_regs = 0;
	if(func->nested_parent)
	{
		++word_regs;
	}
	if(jit_type_return_via_pointer(type))
	{
		++word_regs;
	}
	index = 0;
	partial = 0;
	while(index < num_args && word_regs < ARM_NUM_PARAM_REGS)
	{
		size = jit_type_get_size(jit_value_get_type(args[index]));
		size = ROUND_STACK(size) / sizeof(void *);
		if(size <= (ARM_NUM_PARAM_REGS - word_regs))
		{
			/* This argument will fit entirely in registers */
			word_regs += size;
			++index;
		}
		else
		{
			/* Partly in registers and partly on the stack.
			   We first copy it into a buffer that we can address */
			partial = jit_value_create
				(func, jit_value_get_type(args[index]));
			if(!partial)
			{
				return 0;
			}
			jit_value_set_addressable(partial);
			if(!jit_insn_store(func, partial, args[index]))
			{
				return 0;
			}
			++index;
			break;
		}
	}
	num_stack_args = num_args - index;

	/* Push all of the stacked arguments in reverse order */
	while(num_stack_args > 0)
	{
		--num_stack_args;
		--num_args;
		if(!jit_insn_push(func, args[num_args]))
		{
			return 0;
		}
	}

	/* Handle a value that is partly on the stack and partly in registers */
	if(partial)
	{
		--num_args;
		index = (ARM_NUM_PARAM_REGS - word_regs) * sizeof(void *);
		size = ROUND_STACK(jit_type_get_size(jit_value_get_type(partial)));
		while(size > index)
		{
			size -= sizeof(void *);
			value = jit_value_create(func, jit_type_void_ptr);
			if(!value)
			{
				return 0;
			}
			value = jit_insn_load_relative
				(func, value, (jit_nint)size, jit_type_void_ptr);
			if(!value)
			{
				return 0;
			}
			if(!jit_insn_push(func, value))
			{
				return 0;
			}
		}
		while(size > 0)
		{
			size -= sizeof(void *);
			value = jit_value_create(func, jit_type_void_ptr);
			if(!value)
			{
				return 0;
			}
			value = jit_insn_load_relative
				(func, value, (jit_nint)size, jit_type_void_ptr);
			if(!value)
			{
				return 0;
			}
			--word_regs;
			if(!jit_insn_outgoing_reg(func, value, (int)word_regs))
			{
				return 0;
			}
		}
	}

	/* Push arguments that will end up entirely in registers */
	while(num_args > 0)
	{
		--num_args;
		size = jit_type_get_size(jit_value_get_type(args[num_args]));
		size = ROUND_STACK(size) / sizeof(void *);
		word_regs -= size;
		if(!jit_insn_outgoing_reg(func, args[num_args], (int)size))
		{
			return 0;
		}
	}

	/* Do we need to add a structure return pointer argument? */
	if(jit_type_return_via_pointer(type))
	{
		value = jit_value_create(func, type);
		if(!value)
		{
			return 0;
		}
		*struct_return = value;
		value = jit_insn_address_of(func, value);
		if(!value)
		{
			return 0;
		}
		--word_regs;
		if(!jit_insn_outgoing_reg(func, value, (int)word_regs))
		{
			return 0;
		}
	}
	else
	{
		*struct_return = 0;
	}

	/* Do we need to add nested function scope information? */
	if(is_nested)
	{
		--word_regs;
		if(!jit_insn_setup_for_nested(func, nesting_level, (int)word_regs))
		{
			return 0;
		}
	}

	/* The call is ready to proceed */
	return 1;
}

int _jit_setup_indirect_pointer(jit_function_t func, jit_value_t value)
{
	return jit_insn_outgoing_reg(func, value, ARM_WORK);
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
	pop_bytes = 0;
	while(num_args > 0)
	{
		--num_args;
		size = jit_type_get_size(jit_value_get_type(args[num_args]));
		pop_bytes += ROUND_STACK(size);
	}
	if(ptr_return)
	{
		pop_bytes += sizeof(void *);
	}
	if(is_nested)
	{
		pop_bytes += sizeof(void *);
	}
	if(pop_bytes > (ARM_NUM_PARAM_REGS * sizeof(void *)))
	{
		pop_bytes -= (ARM_NUM_PARAM_REGS * sizeof(void *));
	}
	else
	{
		pop_bytes = 0;
	}

	/* Pop the bytes from the system stack */
	if(pop_bytes > 0)
	{
		if(!jit_insn_pop_stack(func, pop_bytes))
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
#ifdef JIT_ARM_HAS_FLOAT_REGS
	else if(return_type->kind == JIT_TYPE_FLOAT32 ||
			return_type->kind == JIT_TYPE_FLOAT64 ||
			return_type->kind == JIT_TYPE_NFLOAT)
	{
		if(!jit_insn_return_reg(func, return_value, 16 /* f0 */))
		{
			return 0;
		}
	}
#endif
	else if(return_type->kind != JIT_TYPE_VOID)
	{
		if(!jit_insn_return_reg(func, return_value, 0 /* r0 */))
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
		#include "jit-rules-arm.slc"
		#undef JIT_INCLUDE_SUPPORTED
	}
	return 0;
}

void *_jit_gen_prolog(jit_gencode_t gen, jit_function_t func, void *buf)
{
	unsigned int prolog[JIT_PROLOG_SIZE / sizeof(int)];
	arm_inst_ptr inst = prolog;
	int reg, regset;
	unsigned int saved;
	unsigned int frame_size;

	/* Determine which registers need to be preserved */
	regset = 0;
	saved = 0;
	for(reg = 0; reg <= 15; ++reg)
	{
		if(jit_reg_is_used(gen->touched, reg) &&
		   (_jit_reg_info[reg].flags & JIT_REG_CALL_USED) == 0)
		{
			regset |= (1 << reg);
			saved += sizeof(void *);
		}
	}

	/* Setup the frame, pushing all the callee-save registers */
	arm_setup_frame(inst, regset);

	/* Allocate space for the local variable frame.  Subtract off
	   the space for the registers that we just saved.  The pc, lr,
	   and fp registers are always saved, so account for them too */
	frame_size = func->builder->frame_size - (saved + 3);
	if(frame_size > 0)
	{
		arm_alu_reg_imm(inst, ARM_SUB, ARM_SP, ARM_SP, frame_size);
	}

	/* Copy the prolog into place and return the adjusted entry position */
	reg = (int)((inst - prolog) * sizeof(unsigned int));
	jit_memcpy(((unsigned char *)buf) + JIT_PROLOG_SIZE - reg, prolog, reg);
	return (void *)(((unsigned char *)buf) + JIT_PROLOG_SIZE - reg);
}

void _jit_gen_epilog(jit_gencode_t gen, jit_function_t func)
{
	int reg, regset;
	arm_inst_ptr inst;
	void **fixup;
	void **next;
	jit_nint offset;

	/* Bail out if there is insufficient space for the epilog */
	if(!jit_cache_check_for_n(&(gen->posn), 4))
	{
		jit_cache_mark_full(&(gen->posn));
		return;
	}

	/* Determine which registers need to be restored when we return */
	regset = 0;
	for(reg = 0; reg <= 15; ++reg)
	{
		if(jit_reg_is_used(gen->touched, reg) &&
		   (_jit_reg_info[reg].flags & JIT_REG_CALL_USED) == 0)
		{
			regset |= (1 << reg);
		}
	}

	/* Apply fixups for blocks that jump to the epilog */
	fixup = (void **)(gen->epilog_fixup);
	while(fixup != 0)
	{
		offset = (((jit_nint)(fixup[0])) & 0x00FFFFFF) << 2;
		if(!offset)
		{
			next = 0;
		}
		else
		{
			next = (void **)(((unsigned char *)fixup) - offset);
		}
		arm_patch(fixup, gen->posn.ptr);
		fixup = next;
	}
	gen->epilog_fixup = 0;

	/* Pop the local stack frame and return */
	inst = (arm_inst_ptr)(gen->posn.ptr);
	arm_pop_frame(inst, regset);
	gen->posn.ptr = (unsigned char *)inst;
}

void *_jit_gen_redirector(jit_gencode_t gen, jit_function_t func)
{
	void *ptr, *entry;
	arm_inst_ptr inst;
	if(!jit_cache_check_for_n(&(gen->posn), 12))
	{
		jit_cache_mark_full(&(gen->posn));
		return 0;
	}
	ptr = (void *)&(func->entry_point);
	entry = gen->posn.ptr;
	inst = (arm_inst_ptr)(gen->posn.ptr);
	arm_load_membase(inst, ARM_WORK, ARM_PC, 0);
	arm_load_membase(inst, ARM_PC, ARM_WORK, 0);
	*inst++ = (unsigned int)ptr;
	gen->posn.ptr = (unsigned char *)inst;
	return entry;
}

/*
 * Setup or teardown the ARM code output process.
 */
#define	jit_cache_setup_output(needed)	\
	arm_inst_ptr inst = (arm_inst_ptr)(gen->posn.ptr); \
	if(!jit_cache_check_for_n(&(gen->posn), (needed))) \
	{ \
		jit_cache_mark_full(&(gen->posn)); \
		return; \
	}
#define	jit_cache_end_output()	\
	gen->posn.ptr = (unsigned char *)inst

void _jit_gen_spill_reg(jit_gencode_t gen, int reg,
						int other_reg, jit_value_t value)
{
	int offset;

	/* Make sure that we have sufficient space */
	jit_cache_setup_output(20);

	/* Fix the value in place within the local variable frame */
	_jit_gen_fix_value(value);

	/* Output an appropriate instruction to spill the value */
	offset = (int)(value->frame_offset);
	if(reg < 16)
	{
		arm_store_membase(inst, reg, ARM_FP, offset);
		if(other_reg != -1)
		{
			/* Spill the other word register in a pair */
			offset += sizeof(void *);
			arm_store_membase(inst, other_reg, ARM_FP, offset);
		}
	}
	else if(jit_type_normalize(value->type)->kind == JIT_TYPE_FLOAT32)
	{
		arm_store_membase_float32(inst, reg - 16, ARM_FP, offset);
	}
	else
	{
		arm_store_membase_float64(inst, reg - 16, ARM_FP, offset);
	}

	/* End the code output process */
	jit_cache_end_output();
}

void _jit_gen_free_reg(jit_gencode_t gen, int reg,
					   int other_reg, int value_used)
{
	/* We don't have to do anything to free ARM registers */
}

void _jit_gen_load_value
	(jit_gencode_t gen, int reg, int other_reg, jit_value_t value)
{
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
				arm_mov_reg_imm(inst, _jit_reg_info[reg].cpu_reg,
								(jit_nint)(value->address));
			}
			break;

			case JIT_TYPE_LONG:
			case JIT_TYPE_ULONG:
			{
				jit_long long_value;
				long_value = jit_value_get_long_constant(value);
				arm_mov_reg_imm(inst, _jit_reg_info[reg].cpu_reg,
								(jit_int)long_value);
				arm_mov_reg_imm(inst, _jit_reg_info[other_reg].cpu_reg,
								(jit_int)(long_value >> 32));
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
				if(reg < 16)
				{
					arm_mov_reg_imm(inst, _jit_reg_info[reg].cpu_reg,
									*((int *)&float32_value));
				}
				else
				{
					arm_load_membase_float32
						(inst, _jit_reg_info[reg].cpu_reg, ARM_PC, 0);
					arm_jump_imm(inst, 0);
					*(inst)++ = *((int *)&float32_value);
				}
			}
			break;

			case JIT_TYPE_FLOAT64:
			case JIT_TYPE_NFLOAT:
			{
				jit_float64 float64_value;
				float64_value = jit_value_get_float64_constant(value);
				if(!jit_cache_check_for_n(&(gen->posn), 32))
				{
					jit_cache_mark_full(&(gen->posn));
					return;
				}
				if(reg < 16)
				{
					arm_mov_reg_imm(inst, _jit_reg_info[reg].cpu_reg,
									((int *)&float64_value)[0]);
					arm_mov_reg_imm(inst, _jit_reg_info[other_reg].cpu_reg,
									((int *)&float64_value)[1]);
				}
				else if((((int)inst) & 7) == 0)
				{
					arm_load_membase_float64
						(inst, _jit_reg_info[reg].cpu_reg, ARM_PC, 0);
					arm_jump_imm(inst, 4);
					*(inst)++ = ((int *)&float64_value)[0];
					*(inst)++ = ((int *)&float64_value)[1];
				}
				else
				{
					arm_load_membase_float64
						(inst, _jit_reg_info[reg].cpu_reg, ARM_PC, 4);
					arm_jump_imm(inst, 8);
					*(inst)++ = 0;
					*(inst)++ = ((int *)&float64_value)[0];
					*(inst)++ = ((int *)&float64_value)[1];
				}
			}
			break;
		}
	}
	else if(value->has_global_register)
	{
		/* Load the value out of a global register */
		arm_mov_reg_reg(inst, _jit_reg_info[reg].cpu_reg,
						_jit_reg_info[value->global_reg].cpu_reg);
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
				arm_load_membase_sbyte(inst, _jit_reg_info[reg].cpu_reg,
								       ARM_FP, offset);
			}
			break;

			case JIT_TYPE_UBYTE:
			{
				arm_load_membase_byte(inst, _jit_reg_info[reg].cpu_reg,
								      ARM_FP, offset);
			}
			break;

			case JIT_TYPE_SHORT:
			{
				arm_load_membase_short(inst, _jit_reg_info[reg].cpu_reg,
								       ARM_FP, offset);
			}
			break;

			case JIT_TYPE_USHORT:
			{
				arm_load_membase_ushort(inst, _jit_reg_info[reg].cpu_reg,
								        ARM_FP, offset);
			}
			break;

			case JIT_TYPE_INT:
			case JIT_TYPE_UINT:
			{
				arm_load_membase(inst, _jit_reg_info[reg].cpu_reg,
								 ARM_FP, offset);
			}
			break;

			case JIT_TYPE_LONG:
			case JIT_TYPE_ULONG:
			{
				arm_load_membase(inst, _jit_reg_info[reg].cpu_reg,
								 ARM_FP, offset);
				arm_load_membase(inst, _jit_reg_info[other_reg].cpu_reg,
								 ARM_FP, offset + 4);
			}
			break;

			case JIT_TYPE_FLOAT32:
			{
				if(reg < 16)
				{
					arm_load_membase(inst, _jit_reg_info[reg].cpu_reg,
									 ARM_FP, offset);
				}
				else
				{
					arm_load_membase_float32
						(inst, _jit_reg_info[reg].cpu_reg, ARM_FP, offset);
				}
			}
			break;

			case JIT_TYPE_FLOAT64:
			case JIT_TYPE_NFLOAT:
			{
				if(reg < 16)
				{
					arm_load_membase(inst, _jit_reg_info[reg].cpu_reg,
									 ARM_FP, offset);
					arm_load_membase(inst, _jit_reg_info[other_reg].cpu_reg,
									 ARM_FP, offset + 4);
				}
				else
				{
					arm_load_membase_float64
						(inst, _jit_reg_info[reg].cpu_reg, ARM_FP, offset);
				}
			}
			break;
		}
	}

	/* End the code output process */
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
 * Output a branch instruction.
 */
static arm_inst_ptr output_branch
	(jit_function_t func, arm_inst_ptr inst, int cond, jit_insn_t insn)
{
	jit_block_t block;
	int offset;
	block = jit_block_from_label(func, (jit_label_t)(insn->dest));
	if(!block)
	{
		return inst;
	}
	if(block->address)
	{
		/* We already know the address of the block */
		arm_branch(inst, cond, block->address);
	}
	else
	{
		/* Output a placeholder and record on the block's fixup list */
		if(block->fixup_list)
		{
			offset = (int)(((unsigned char *)inst) -
						   ((unsigned char *)(block->fixup_list)));
		}
		else
		{
			offset = 0;
		}
		arm_branch_imm(inst, cond, offset);
		block->fixup_list = (void *)(inst - 1);
	}
	return inst;
}

/*
 * Throw a builtin exception.
 */
static arm_inst_ptr throw_builtin
		(arm_inst_ptr inst, jit_function_t func, int cond, int type)
{
	arm_inst_ptr patch;

	/* Branch past the following code if "cond" is not true */
	patch = inst;
	arm_branch_imm(inst, cond ^ 0x01, 0);

	/* We need to update "catch_pc" if we have a "try" block */
	if(func->builder->setjmp_value != 0)
	{
		_jit_gen_fix_value(func->builder->setjmp_value);
		arm_mov_reg_reg(inst, ARM_WORK, ARM_PC);
		arm_store_membase(inst, ARM_WORK, ARM_FP,
						  func->builder->setjmp_value->frame_offset +
						  jit_jmp_catch_pc_offset);
	}

	/* Push the exception type onto the stack */
	arm_mov_reg_imm(inst, ARM_WORK, type);
	arm_push_reg(inst, ARM_WORK);

	/* Call the "jit_exception_builtin" function, which will never return */
	arm_call(inst, jit_exception_builtin);

	/* Back-patch the previous branch instruction */
	arm_patch(patch, inst);
	return inst;
}

/*
 * Jump to the current function's epilog.
 */
static arm_inst_ptr jump_to_epilog
	(jit_gencode_t gen, arm_inst_ptr inst, jit_block_t block)
{
	int offset;

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
	if(gen->epilog_fixup)
	{
		offset = (int)(((unsigned char *)inst) -
					   ((unsigned char *)(gen->epilog_fixup)));
	}
	else
	{
		offset = 0;
	}
	arm_branch_imm(inst, ARM_CC_AL, offset);
	gen->epilog_fixup = (void *)(inst - 1);
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
		#include "jit-rules-arm.slc"
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
	jit_nint offset;

	/* Set the address of this block */
	block->address = (void *)(gen->posn.ptr);

	/* If this block has pending fixups, then apply them now */
	fixup = (void **)(block->fixup_list);
	while(fixup != 0)
	{
		offset = (((jit_nint)(fixup[0])) & 0x00FFFFFF) << 2;
		if(!offset)
		{
			next = 0;
		}
		else
		{
			next = (void **)(((unsigned char *)fixup) - offset);
		}
		arm_patch(fixup, block->address);
		fixup = next;
	}
	block->fixup_list = 0;
}

void _jit_gen_end_block(jit_gencode_t gen, jit_block_t block)
{
	/* Nothing to do here for ARM */
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

#endif /* JIT_BACKEND_ARM */
