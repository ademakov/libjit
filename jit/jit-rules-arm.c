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

/*
 * Determine if we actually have floating-point registers.
 */
#if JIT_APPLY_NUM_FLOAT_REGS != 0 || JIT_APPLY_RETURN_FLOATS_AFTER != 0
	#define	JIT_ARM_HAS_FLOAT_REGS	1
#endif

/*
 * Round a size up to a multiple of the stack word size.
 */
#define	ROUND_STACK(size)	\
		(((size) + (sizeof(void *) - 1)) & ~(sizeof(void *) - 1))

void _jit_init_backend(void)
{
#ifndef JIT_ARM_HAS_FLOAT_REGS
	/* Turn off floating-point registers, as this ARM core doesn't have them */
	int reg;
	for(reg = 16; reg < JIT_NUM_REGS; ++reg)
	{
		_jit_reg_info[reg].flags = JIT_REG_FIXED;
	}
#endif
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
	else
	{
		if(!jit_insn_return_reg(func, return_value, ARM_R0))
		{
			return 0;
		}
	}

	/* Everything is back where it needs to be */
	return 1;
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
	arm_store_membase(inst, ARM_FP, offset, reg);
	if(other_reg != -1)
	{
		/* Spill the other word register in a pair */
		offset += sizeof(void *);
		arm_store_membase(inst, ARM_FP, offset, reg);
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
	/* TODO */
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

void _jit_gen_insn(jit_gencode_t gen, jit_function_t func,
				   jit_block_t block, jit_insn_t insn)
{
	/* TODO */
}

void _jit_gen_start_block(jit_gencode_t gen, jit_block_t block)
{
	/* TODO: label fixups */
}

void _jit_gen_end_block(jit_gencode_t gen, jit_block_t block)
{
	/* Nothing to do here for ARM */
}

void _jit_gen_call_finally
	(jit_gencode_t gen, jit_function_t func, jit_label_t label)
{
	/* TODO */
}

void _jit_gen_unwind_stack(void *stacktop, void *catch_pc, void *object)
{
	/* TODO */
}

#endif /* JIT_BACKEND_ARM */
