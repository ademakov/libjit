/*
 * jit-rules-arm.c - Rules that define the characteristics of the ARM.
 *
 * Copyright (C) 2004  Southern Storm Software, Pty Ltd.
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

/*
 * Load the instruction pointer from the generation context.
 */
#define	jit_gen_load_inst_ptr(gen,inst)	\
		do { \
			arm_inst_buf_init((inst), (gen)->posn.ptr, (gen)->posn.limit); \
		} while (0)

/*
 * Save the instruction pointer back to the generation context.
 */
#define	jit_gen_save_inst_ptr(gen,inst)	\
		do { \
			(gen)->posn.ptr = (unsigned char *)arm_inst_get_posn(inst); \
		} while (0)

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
 * Flush the contents of the constant pool.
 */
static void flush_constants(jit_gencode_t gen, int after_epilog)
{
	arm_inst_buf inst;
	arm_inst_word *patch;
	arm_inst_word *current;
	arm_inst_word *fixup;
	int index, value, offset;

	/* Bail out if there are no constants to flush */
	if(!(gen->num_constants))
	{
		return;
	}

	/* Initialize the cache output pointer */
	jit_gen_load_inst_ptr(gen, inst);

	/* Jump over the constant pool if it is being output inline */
	if(!after_epilog)
	{
		patch = arm_inst_get_posn(inst);
		arm_jump_imm(inst, 0);
	}
	else
	{
		patch = 0;
	}

	/* Align the constant pool, if requested */
	if(gen->align_constants && (((int)arm_inst_get_posn(inst)) & 7) != 0)
	{
		arm_inst_add(inst, 0);
	}

	/* Output the constant values and apply the necessary fixups */
	for(index = 0; index < gen->num_constants; ++index)
	{
		current = arm_inst_get_posn(inst);
		arm_inst_add(inst, gen->constants[index]);
		fixup = gen->fixup_constants[index];
		while(fixup != 0)
		{
			if((*fixup & 0x0F000000) == 0x05000000)
			{
				/* Word constant fixup */
				value = *fixup & 0x0FFF;
				offset = ((arm_inst_get_posn(inst) - 1 - fixup) * 4) - 8;
				*fixup = ((*fixup & ~0x0FFF) | offset);
			}
			else
			{
				/* Floating-point constant fixup */
				value = (*fixup & 0x00FF) * 4;
				offset = ((arm_inst_get_posn(inst) - 1 - fixup) * 4) - 8;
				*fixup = ((*fixup & ~0x00FF) | (offset / 4));
			}
			if(value)
			{
				fixup -= value;
			}
			else
			{
				fixup = 0;
			}
		}
	}

	/* Backpatch the jump if necessary */
	if(!after_epilog)
	{
		arm_patch(inst, patch, arm_inst_get_posn(inst));
	}

	/* Flush the pool state and restart */
	gen->num_constants = 0;
	gen->align_constants = 0;
	gen->first_constant_use = 0;
	jit_gen_save_inst_ptr(gen, inst);
}

/*
 * Perform a constant pool flush if we are too far from the starting point.
 */
static int flush_if_too_far(jit_gencode_t gen)
{
	if(gen->first_constant_use &&
	   (((arm_inst_word *)(gen->posn.ptr)) -
	   		((arm_inst_word *)(gen->first_constant_use))) >= 100)
	{
		flush_constants(gen, 0);
		return 1;
	}
	else
	{
		return 0;
	}
}

/*
 * Add a fixup for a particular constant pool entry.
 */
static void add_constant_fixup
	(jit_gencode_t gen, int index, arm_inst_word *fixup)
{
	arm_inst_word *prev;
	int value;
	if(((unsigned char *)fixup) >= gen->posn.limit)
	{
		/* The instruction buffer is full, so don't record this fixup */
		return;
	}
	prev = gen->fixup_constants[index];
	if(prev)
	{
		value = fixup - prev;
	}
	else
	{
		value = 0;
	}
	if((*fixup & 0x0F000000) == 0x05000000)
	{
		*fixup = ((*fixup & ~0x0FFF) | value);
	}
	else
	{
		*fixup = ((*fixup & ~0x00FF) | (value / 4));
	}
	gen->fixup_constants[index] = fixup;
	if(!(gen->first_constant_use))
	{
		gen->first_constant_use = fixup;
	}
}

/*
 * Add an immediate value to the constant pool.  The constant
 * is loaded from the instruction at "fixup".
 */
static void add_constant(jit_gencode_t gen, int value, arm_inst_word *fixup)
{
	int index;

	/* Search the constant pool for an existing copy of the value */
	for(index = 0; index < gen->num_constants; ++index)
	{
		if(gen->constants[index] == value)
		{
			add_constant_fixup(gen, index, fixup);
			return;
		}
	}

	/* Flush the constant pool if there is insufficient space */
	if(gen->num_constants >= JIT_ARM_MAX_CONSTANTS)
	{
		flush_constants(gen, 0);
	}

	/* Add the constant value to the pool */
	gen->constants[gen->num_constants] = value;
	gen->fixup_constants[gen->num_constants] = 0;
	++(gen->num_constants);
	add_constant_fixup(gen, gen->num_constants - 1, fixup);
}

/*
 * Add a double-word immedite value to the constant pool.
 */
static void add_constant_dword
	(jit_gencode_t gen, int value1, int value2, arm_inst_word *fixup, int align)
{
	int index;

	/* Make sure that the constant pool is properly aligned when output */
	if(align)
	{
		gen->align_constants = 1;
	}

	/* Search the constant pool for an existing copy of the value */
	for(index = 0; index < (gen->num_constants - 1); ++index)
	{
		if(gen->constants[index] == value1 &&
		   gen->constants[index + 1] == value2)
		{
			if(!align || (index % 2) == 0)
			{
				add_constant_fixup(gen, index, fixup);
				return;
			}
		}
	}

	/* Flush the constant pool if there is insufficient space */
	if(gen->num_constants >= (JIT_ARM_MAX_CONSTANTS - 1))
	{
		flush_constants(gen, 0);
	}

	/* Align the constant pool on a 64-bit boundary if necessary */
	if(align && (gen->num_constants % 2) != 0)
	{
		gen->constants[gen->num_constants] = 0;
		gen->fixup_constants[gen->num_constants] = 0;
		++(gen->num_constants);
	}

	/* Add the double word constant value to the pool */
	gen->constants[gen->num_constants] = value1;
	gen->fixup_constants[gen->num_constants] = 0;
	gen->constants[gen->num_constants] = value2;
	gen->fixup_constants[gen->num_constants] = 0;
	gen->num_constants += 2;
	add_constant_fixup(gen, gen->num_constants - 2, fixup);
}

/*
 * Load an immediate value into a word register.  If the value is
 * complicated, then add an entry to the constant pool.
 */
static void mov_reg_imm
	(jit_gencode_t gen, arm_inst_buf *inst, int reg, int value)
{
	arm_inst_word *fixup;

	/* Bail out if the value is not complex enough to need a pool entry */
	if(!arm_is_complex_imm(value))
	{
		arm_mov_reg_imm(*inst, reg, value);
		return;
	}

	/* Output a placeholder to load the value later */
	fixup = arm_inst_get_posn(*inst);
	arm_load_membase(*inst, reg, ARM_PC, 0);

	/* Add the constant to the pool, which may cause a flush */
	jit_gen_save_inst_ptr(gen, *inst);
	add_constant(gen, value, fixup);
	jit_gen_load_inst_ptr(gen, *inst);
}

/*
 * Load a float32 immediate value into a float register.  If the value is
 * complicated, then add an entry to the constant pool.
 */
static void mov_freg_imm_32
	(jit_gencode_t gen, arm_inst_buf *inst, int reg, int value)
{
	arm_inst_word *fixup;

	/* Output a placeholder to load the value later */
	fixup = arm_inst_get_posn(*inst);
	arm_load_membase_float32(*inst, reg, ARM_PC, 0);

	/* Add the constant to the pool, which may cause a flush */
	jit_gen_save_inst_ptr(gen, *inst);
	add_constant(gen, value, fixup);
	jit_gen_load_inst_ptr(gen, *inst);
}

/*
 * Load a float64 immediate value into a float register.  If the value is
 * complicated, then add an entry to the constant pool.
 */
static void mov_freg_imm_64
	(jit_gencode_t gen, arm_inst_buf *inst, int reg, int value1, int value2)
{
	arm_inst_word *fixup;

	/* Output a placeholder to load the value later */
	fixup = arm_inst_get_posn(*inst);
	arm_load_membase_float64(*inst, reg, ARM_PC, 0);

	/* Add the constant to the pool, which may cause a flush */
	jit_gen_save_inst_ptr(gen, *inst);
	add_constant_dword(gen, value1, value2, fixup, 1);
	jit_gen_load_inst_ptr(gen, *inst);
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
	jit_type_t return_type;
	int ptr_return;

	/* Bail out now if we don't need to worry about return values */
	return_type = jit_type_normalize(jit_type_get_return(signature));
	ptr_return = jit_type_return_via_pointer(return_type);
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
	arm_inst_buf inst;
	int reg, regset;
	unsigned int saved;
	unsigned int frame_size;

	/* Initialize the instruction buffer */
	arm_inst_buf_init(inst, prolog, prolog + JIT_PROLOG_SIZE / sizeof(int));

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
	frame_size = func->builder->frame_size - (saved + 3 * sizeof(void *));
	frame_size += (unsigned int)(func->builder->param_area_size);
	if(frame_size > 0)
	{
		arm_alu_reg_imm(inst, ARM_SUB, ARM_SP, ARM_SP, frame_size);
	}

	/* Copy the prolog into place and return the adjusted entry position */
	reg = (int)((arm_inst_get_posn(inst) - prolog) * sizeof(unsigned int));
	jit_memcpy(((unsigned char *)buf) + JIT_PROLOG_SIZE - reg, prolog, reg);
	return (void *)(((unsigned char *)buf) + JIT_PROLOG_SIZE - reg);
}

void _jit_gen_epilog(jit_gencode_t gen, jit_function_t func)
{
	int reg, regset;
	arm_inst_buf inst;
	void **fixup;
	void **next;
	jit_nint offset;

	/* Initialize the instruction buffer */
	jit_gen_load_inst_ptr(gen, inst);

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
		arm_patch(inst, fixup, arm_inst_get_posn(inst));
		fixup = next;
	}
	gen->epilog_fixup = 0;

	/* Pop the local stack frame and return */
	arm_pop_frame(inst, regset);
	jit_gen_save_inst_ptr(gen, inst);

	/* Flush the remainder of the constant pool */
	flush_constants(gen, 1);
}

void *_jit_gen_redirector(jit_gencode_t gen, jit_function_t func)
{
	void *ptr, *entry;
	arm_inst_buf inst;
	jit_gen_load_inst_ptr(gen, inst);
	ptr = (void *)&(func->entry_point);
	entry = gen->posn.ptr;
	arm_load_membase(inst, ARM_WORK, ARM_PC, 0);
	arm_load_membase(inst, ARM_PC, ARM_WORK, 0);
	arm_inst_add(inst, (unsigned int)ptr);
	jit_gen_save_inst_ptr(gen, inst);
	return entry;
}

/*
 * Setup or teardown the ARM code output process.
 */
#define	jit_cache_setup_output(needed)	\
	arm_inst_buf inst; \
	jit_gen_load_inst_ptr(gen, inst)
#define	jit_cache_end_output()	\
	jit_gen_save_inst_ptr(gen, inst)

void _jit_gen_spill_reg(jit_gencode_t gen, int reg,
						int other_reg, jit_value_t value)
{
	int offset;

	/* Make sure that we have sufficient space */
	jit_cache_setup_output(32);
	if(flush_if_too_far(gen))
	{
		jit_gen_load_inst_ptr(gen, inst);
	}

	/* Output an appropriate instruction to spill the value */
	if(value->has_global_register)
	{
		arm_mov_reg_reg(inst, _jit_reg_info[value->global_reg].cpu_reg,
						_jit_reg_info[reg].cpu_reg);
	}
	else
	{
		_jit_gen_fix_value(value);
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
	jit_cache_setup_output(32);
	if(flush_if_too_far(gen))
	{
		jit_gen_load_inst_ptr(gen, inst);
	}

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
				mov_reg_imm(gen, &inst, _jit_reg_info[reg].cpu_reg,
							(jit_nint)(value->address));
			}
			break;

			case JIT_TYPE_LONG:
			case JIT_TYPE_ULONG:
			{
				jit_long long_value;
				long_value = jit_value_get_long_constant(value);
				mov_reg_imm(gen, &inst, _jit_reg_info[reg].cpu_reg,
						    (jit_int)long_value);
				mov_reg_imm(gen, &inst, _jit_reg_info[reg].cpu_reg + 1,
						    (jit_int)(long_value >> 32));
			}
			break;

			case JIT_TYPE_FLOAT32:
			{
				jit_float32 float32_value;
				float32_value = jit_value_get_float32_constant(value);
				if(reg < 16)
				{
					mov_reg_imm(gen, &inst, _jit_reg_info[reg].cpu_reg,
							    *((int *)&float32_value));
				}
				else
				{
					mov_freg_imm_32
						(gen, &inst, _jit_reg_info[reg].cpu_reg,
					     *((int *)&float32_value));
				}
			}
			break;

			case JIT_TYPE_FLOAT64:
			case JIT_TYPE_NFLOAT:
			{
				jit_float64 float64_value;
				float64_value = jit_value_get_float64_constant(value);
				if(reg < 16)
				{
					mov_reg_imm
						(gen, &inst, _jit_reg_info[reg].cpu_reg,
						 ((int *)&float64_value)[0]);
					mov_reg_imm
						(gen, &inst, _jit_reg_info[reg].cpu_reg + 1,
						 ((int *)&float64_value)[1]);
				}
				else
				{
					mov_freg_imm_64
						(gen, &inst, _jit_reg_info[reg].cpu_reg,
					     ((int *)&float64_value)[0],
					     ((int *)&float64_value)[1]);
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
				arm_load_membase(inst, _jit_reg_info[reg].cpu_reg + 1,
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
					arm_load_membase(inst, _jit_reg_info[reg].cpu_reg + 1,
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

void _jit_gen_spill_global(jit_gencode_t gen, int reg, jit_value_t value)
{
	/* TODO: Implement if ARM needs it. */
}

void _jit_gen_load_global(jit_gencode_t gen, int reg, jit_value_t value)
{
	jit_cache_setup_output(32);
	arm_load_membase(inst, _jit_reg_info[value->global_reg].cpu_reg,
					 ARM_FP, value->frame_offset);
	jit_cache_end_output();
}

void _jit_gen_exch_top(jit_gencode_t gen, int reg)
{
}

void _jit_gen_move_top(jit_gencode_t gen, int reg)
{
}

void _jit_gen_spill_top(jit_gencode_t gen, int reg, jit_value_t value, int pop)
{
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
static void output_branch
	(jit_function_t func, arm_inst_buf *inst, int cond, jit_insn_t insn)
{
	jit_block_t block;
	int offset;
	block = jit_block_from_label(func, (jit_label_t)(insn->dest));
	if(!block)
	{
		return;
	}
	if(arm_inst_get_posn(*inst) >= arm_inst_get_limit(*inst))
	{
		/* The buffer has overflowed, so don't worry about fixups */
		return;
	}
	if(block->address)
	{
		/* We already know the address of the block */
		arm_branch(*inst, cond, block->address);
	}
	else
	{
		/* Output a placeholder and record on the block's fixup list */
		if(block->fixup_list)
		{
			offset = (int)(((unsigned char *)arm_inst_get_posn(*inst)) -
						   ((unsigned char *)(block->fixup_list)));
		}
		else
		{
			offset = 0;
		}
		arm_branch_imm(*inst, cond, offset);
		block->fixup_list = (void *)(arm_inst_get_posn(*inst) - 1);
	}
}

/*
 * Throw a builtin exception.
 */
static void throw_builtin
		(arm_inst_buf *inst, jit_function_t func, int cond, int type)
{
	arm_inst_word *patch;

	/* Branch past the following code if "cond" is not true */
	patch = arm_inst_get_posn(*inst);
	arm_branch_imm(*inst, cond ^ 0x01, 0);

	/* We need to update "catch_pc" if we have a "try" block */
	if(func->builder->setjmp_value != 0)
	{
		_jit_gen_fix_value(func->builder->setjmp_value);
		arm_mov_reg_reg(*inst, ARM_WORK, ARM_PC);
		arm_store_membase(*inst, ARM_WORK, ARM_FP,
						  func->builder->setjmp_value->frame_offset +
						  jit_jmp_catch_pc_offset);
	}

	/* Push the exception type onto the stack */
	arm_mov_reg_imm(*inst, ARM_WORK, type);
	arm_push_reg(*inst, ARM_WORK);

	/* Call the "jit_exception_builtin" function, which will never return */
	arm_call(*inst, jit_exception_builtin);

	/* Back-patch the previous branch instruction */
	arm_patch(*inst, patch, arm_inst_get_posn(*inst));
}

/*
 * Jump to the current function's epilog.
 */
static void jump_to_epilog
	(jit_gencode_t gen, arm_inst_buf *inst, jit_block_t block)
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
		return;
	}

	/* Bail out if the instruction buffer has overflowed */
	if(arm_inst_get_posn(*inst) >= arm_inst_get_limit(*inst))
	{
		return;
	}

	/* Output a placeholder for the jump and add it to the fixup list */
	if(gen->epilog_fixup)
	{
		offset = (int)(((unsigned char *)arm_inst_get_posn(*inst)) -
					   ((unsigned char *)(gen->epilog_fixup)));
	}
	else
	{
		offset = 0;
	}
	arm_branch_imm(*inst, ARM_CC_AL, offset);
	gen->epilog_fixup = (void *)(arm_inst_get_posn(*inst) - 1);
}

#define	TODO()		\
	do { \
		fprintf(stderr, "TODO at %s, %d\n", __FILE__, (int)__LINE__); \
	} while (0)

void _jit_gen_insn(jit_gencode_t gen, jit_function_t func,
				   jit_block_t block, jit_insn_t insn)
{
	flush_if_too_far(gen);
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
	arm_inst_buf inst;

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
		jit_gen_load_inst_ptr(gen, inst);
		arm_patch(inst, fixup, block->address);
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
