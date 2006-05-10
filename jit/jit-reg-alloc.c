/*
 * jit-reg-alloc.c - Register allocation routines for the JIT.
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
#include "jit-reg-alloc.h"
#include <jit/jit-dump.h>
#include <stdio.h>
#include <string.h>

/*@

The @code{libjit} library provides a number of functions for
performing register allocation within basic blocks so that you
mostly don't have to worry about it:

@*/

/*@
 * @deftypefun void _jit_regs_init_for_block (jit_gencode_t gen)
 * Initialize the register allocation state for a new block.
 * @end deftypefun
@*/
void _jit_regs_init_for_block(jit_gencode_t gen)
{
	int reg;
	gen->current_age = 1;
	for(reg = 0; reg < JIT_NUM_REGS; ++reg)
	{
		/* Clear everything except permanent and fixed registers */
		if(!jit_reg_is_used(gen->permanent, reg) &&
		   (_jit_reg_info[reg].flags & JIT_REG_FIXED) == 0)
		{
			gen->contents[reg].num_values = 0;
			gen->contents[reg].is_long_start = 0;
			gen->contents[reg].is_long_end = 0;
			gen->contents[reg].age = 0;
			gen->contents[reg].remap = -1;
			gen->contents[reg].used_for_temp = 0;
		}
		gen->stack_map[reg] = -1;
	}
	gen->inhibit = jit_regused_init;
}

/*@
 * @deftypefun int _jit_regs_needs_long_pair (jit_type_t type)
 * Determine if a type requires a long register pair.
 * @end deftypefun
@*/
int _jit_regs_needs_long_pair(jit_type_t type)
{
#if defined(JIT_NATIVE_INT32) && !defined(JIT_BACKEND_INTERP)
	type = jit_type_normalize(type);
	if(type)
	{
		if(type->kind == JIT_TYPE_LONG || type->kind == JIT_TYPE_ULONG)
		{
			return 1;
		}
	}
	return 0;
#else
	/* We don't register pairs on 64-bit platforms or the interpreter */
	return 0;
#endif
}

/*@
 * @deftypefun int _jit_regs_get_cpu (jit_gencode_t gen, int reg, int *other_reg)
 * Get the CPU register that corresponds to a pseudo register.
 * "other_reg" will be set to the other register in a pair,
 * or -1 if the register is not part of a pair.
 * @end deftypefun
@*/
int _jit_regs_get_cpu(jit_gencode_t gen, int reg, int *other_reg)
{
	int cpu_reg, other;
	cpu_reg = gen->contents[reg].remap;
	if(cpu_reg == -1)
	{
		cpu_reg = _jit_reg_info[reg].cpu_reg;
	}
	else
	{
		cpu_reg = _jit_reg_info[cpu_reg].cpu_reg;
	}
	if(gen->contents[reg].is_long_start)
	{
		other = _jit_reg_info[reg].other_reg;
		if(gen->contents[other].remap == -1)
		{
			other = _jit_reg_info[other].cpu_reg;
		}
		else
		{
			other = _jit_reg_info[gen->contents[other].remap].cpu_reg;
		}
	}
	else
	{
		other = -1;
	}
	if(other_reg)
	{
		*other_reg = other;
	}
	return cpu_reg;
}

/*
 * Dump debug information about the register allocation state.
 */
/*#define	JIT_REG_DEBUG	1*/
#ifdef JIT_REG_DEBUG
static void dump_regs(jit_gencode_t gen, const char *name)
{
	int reg;
	unsigned int index;
	printf("%s:\n", name);
	for(reg = 0; reg < JIT_NUM_REGS; ++reg)
	{
		if(gen->contents[reg].num_values == 0 &&
		   !(gen->contents[reg].used_for_temp) &&
		   gen->contents[reg].remap == -1)
		{
			continue;
		}
		printf("\t%s: ", _jit_reg_info[reg].name);
		if(gen->contents[reg].num_values > 0)
		{
			for(index = 0; index < gen->contents[reg].num_values; ++index)
			{
				if(index)
					fputs(", ", stdout);
				jit_dump_value(stdout, jit_value_get_function
										(gen->contents[reg].values[index]),
							   gen->contents[reg].values[index], 0);
			}
			if(gen->contents[reg].used_for_temp)
			{
				printf(", used_for_temp");
			}
		}
		else if(gen->contents[reg].used_for_temp)
		{
			printf("used_for_temp");
		}
		else
		{
			printf("free");
		}
		if(gen->contents[reg].remap != -1)
		{
			printf(", remap=%d", (int)(gen->contents[reg].remap));
		}
		for(index = 0; index < JIT_NUM_REGS; ++index)
		{
			if(gen->stack_map[index] == reg)
			{
				printf(", reverse_remap=%d", (int)index);
			}
		}
		putc('\n', stdout);
	}
}
#endif

/*
 * Spill all registers between two end points.
 */
static void spill_all_between(jit_gencode_t gen, int first, int last)
{
	int reg, posn, other_reg, real_reg;
	int first_stack_reg = 0;
	jit_value_t value;
	int value_used;

#ifdef JIT_REG_DEBUG
	dump_regs(gen, "enter spill_all_between");
#endif

	/* Handle the non-stack registers first, as they are easy to spill */
	for(reg = first; reg <= last; ++reg)
	{
		/* Skip this register if it is permanent or fixed */
		if(jit_reg_is_used(gen->permanent, reg) ||
		   (_jit_reg_info[reg].flags & JIT_REG_FIXED) != 0)
		{
			continue;
		}

		/* Remember this register if it is the start of a stack */
		if((_jit_reg_info[reg].flags & JIT_REG_START_STACK) != 0)
		{
			first_stack_reg = reg;
		}

		/* If this is a stack register, then we need to find the
		   register that contains the top-most stack position,
		   because we must spill stack registers from top down.
		   As we spill each one, something else will become the top */
		if((_jit_reg_info[reg].flags & JIT_REG_IN_STACK) != 0)
		{
			real_reg = gen->stack_map[first_stack_reg];
			if(real_reg == -1)
			{
				continue;
			}
		}
		else
		{
			real_reg = reg;
		}

		/* Skip this register if there is nothing in it */
		if(gen->contents[real_reg].num_values == 0 &&
		   !(gen->contents[real_reg].used_for_temp))
		{
			continue;
		}

		/* Get the other register in a long pair, if there is one */
		if(gen->contents[real_reg].is_long_start)
		{
			other_reg = _jit_reg_info[real_reg].other_reg;
		}
		else
		{
			other_reg = -1;
		}

		/* Spill all values that are associated with the register */
		value_used = 0;
		if(gen->contents[real_reg].num_values > 0)
		{
			for(posn = gen->contents[real_reg].num_values - 1;
				posn >= 0; --posn)
			{
				value = gen->contents[real_reg].values[posn];
				if(value->has_global_register)
				{
					if(!(value->in_global_register))
					{
						_jit_gen_spill_reg(gen, real_reg, other_reg, value);
						value->in_global_register = 1;
						value_used = 1;
					}
				}
				else if(!(value->in_frame))
				{
					if((_jit_reg_info[reg].flags & JIT_REG_IN_STACK) == 0)
					{
						_jit_gen_spill_reg(gen, real_reg, other_reg, value);
					}
					else
					{
						/* The back end needs to think that we are spilling
						   the first register in the stack, regardless of
						   what "real_reg" might happen to be */
						_jit_gen_spill_reg(gen, first_stack_reg, -1, value);
					}
					value->in_frame = 1;
					value_used = 1;
				}
				value->in_register = 0;
				value->reg = -1;
			}
		}

		/* Free the register */
		_jit_regs_free_reg(gen, real_reg, value_used);
	}

#ifdef JIT_REG_DEBUG
	dump_regs(gen, "leave spill_all_between");
#endif
}

/*
 * Spill a specific register.  If it is in a stack, then all registers
 * above the specific register must also be spilled.
 */
static void spill_register(jit_gencode_t gen, int reg)
{
	if((_jit_reg_info[reg].flags & JIT_REG_IN_STACK) == 0)
	{
		spill_all_between(gen, reg, reg);
	}
	else
	{
		int first_reg;
		reg = gen->contents[reg].remap;
		first_reg = reg;
		while((_jit_reg_info[first_reg].flags & JIT_REG_START_STACK) == 0)
		{
			--first_reg;
		}
		spill_all_between(gen, first_reg, reg);
	}
}

/*
 * Spill all stack registers of a specific type.
 */
static void spill_all_stack(jit_gencode_t gen, int reg)
{
	int first_reg;
	while((_jit_reg_info[reg].flags & JIT_REG_START_STACK) == 0)
	{
		--reg;
	}
	first_reg = reg;
	while((_jit_reg_info[reg].flags & JIT_REG_END_STACK) == 0)
	{
		++reg;
	}
	spill_all_between(gen, first_reg, reg);
}

/*@
 * @deftypefun void _jit_regs_spill_all (jit_gencode_t gen)
 * Spill all of the temporary registers to memory locations.
 * Normally used at the end of a block, but may also be used in
 * situations where a value must be in a certain register and
 * it is too hard to swap things around to put it there.
 * @end deftypefun
@*/
void _jit_regs_spill_all(jit_gencode_t gen)
{
	spill_all_between(gen, 0, JIT_NUM_REGS - 1);
}

/*
 * Free a register within a stack, and renumber the other stack registers
 * to compensate for the change.
 */
static void free_stack_reg(jit_gencode_t gen, int reg)
{
	int remap;

#ifdef JIT_REG_DEBUG
	dump_regs(gen, "enter free_stack_reg");
#endif

	/* Shift everything after this register up by one position */
	remap = gen->contents[reg].remap;
	if((_jit_reg_info[remap].flags & JIT_REG_END_STACK) == 0)
	{
		++remap;
		for(;;)
		{
			if(gen->stack_map[remap] == -1)
			{
				/* There are no more active values in this stack */
				gen->stack_map[remap - 1] = -1;
				break;
			}
			else if((_jit_reg_info[remap].flags & JIT_REG_END_STACK) != 0)
			{
				/* This is the last register in the stack */
				--(gen->contents[gen->stack_map[remap]].remap);
				gen->stack_map[remap - 1] = gen->stack_map[remap];
				gen->stack_map[remap] = -1;
				break;
			}
			else
			{
				/* Shift this stack entry up by one */
				--(gen->contents[gen->stack_map[remap]].remap);
				gen->stack_map[remap - 1] = gen->stack_map[remap];
				++remap;
			}
		}
	}

	/* Clear the remapping for the register */
	gen->contents[reg].remap = -1;

#ifdef JIT_REG_DEBUG
	dump_regs(gen, "leave free_stack_reg");
#endif
}

/*
 * Make space for a new stack register in a particular stack.
 * Returns the pseudo register number of the newly allocated register.
 */
static int create_stack_reg(jit_gencode_t gen, int reg, int roll_down)
{
	int first_stack_reg;
	int temp_reg, remap;

#ifdef JIT_REG_DEBUG
	dump_regs(gen, "enter create_stack_reg");
#endif

	/* Find the first pseudo register in the stack */
	while((_jit_reg_info[reg].flags & JIT_REG_START_STACK) == 0)
	{
		--reg;
	}
	first_stack_reg = reg;

	/* Find a free pseudo register in the stack */
	for(;;)
	{
		if(gen->contents[reg].num_values == 0 &&
		   !(gen->contents[reg].used_for_temp))
		{
			break;
		}
		if((_jit_reg_info[reg].flags & JIT_REG_END_STACK) != 0)
		{
			/* None of the registers are free, so we have to spill them all */
			spill_all_between(gen, first_stack_reg, reg);
			reg = first_stack_reg;
			break;
		}
		++reg;
	}

	/* Roll the stack remappings down to make room at the top */
	if(roll_down)
	{
		temp_reg = first_stack_reg - 1;
		do
		{
			++temp_reg;
			remap = gen->contents[temp_reg].remap;
			if(remap != -1)
			{
				/* Change the register's position in the stack */
				gen->contents[temp_reg].remap = remap + 1;
				gen->stack_map[remap + 1] = temp_reg;

				/* Mark the rolled-down register position as touched */
				jit_reg_set_used(gen->touched, remap + 1);
			}
		}
		while((_jit_reg_info[temp_reg].flags & JIT_REG_END_STACK) == 0);
		gen->contents[reg].remap = first_stack_reg;
		gen->stack_map[first_stack_reg] = reg;
	}

	/* Mark the register as touched, in case it needs to be saved */
	jit_reg_set_used(gen->touched, reg);

#ifdef JIT_REG_DEBUG
	dump_regs(gen, "leave create_stack_reg");
#endif

	/* Return the free register to the caller */
	return reg;
}

/*
 * Free a register, and optionally spill its value.
 */
static void free_reg_and_spill
	(jit_gencode_t gen, int reg, int value_used, int spill)
{
	int other_reg, posn;
	jit_value_t value;

#ifdef JIT_REG_DEBUG
	dump_regs(gen, "enter free_reg_and_spill");
#endif

	/* Find the other register in a long pair */
	if(gen->contents[reg].is_long_start)
	{
		other_reg = _jit_reg_info[reg].other_reg;
		gen->contents[reg].is_long_start = 0;
		gen->contents[other_reg].is_long_end = 0;
	}
	else if(gen->contents[reg].is_long_end)
	{
		gen->contents[reg].is_long_end = 0;
		other_reg = reg;
		for(reg = 0; reg < JIT_NUM_REGS; ++reg)
		{
			if(other_reg == _jit_reg_info[reg].other_reg)
			{
				gen->contents[reg].is_long_start = 0;
				break;
			}
		}
	}
	else
	{
		other_reg = -1;
	}

	/* Spill the register's contents to the local variable frame */
	if(spill && gen->contents[reg].num_values > 0)
	{
		for(posn = gen->contents[reg].num_values - 1; posn >= 0; --posn)
		{
			value = gen->contents[reg].values[posn];
			if(value->has_global_register)
			{
				if(!(value->in_global_register))
				{
					_jit_gen_spill_reg(gen, reg, other_reg, value);
					value->in_global_register = 1;
					value_used = 1;
				}
			}
			else if(!(value->in_frame))
			{
				if((_jit_reg_info[reg].flags & JIT_REG_IN_STACK) == 0)
				{
					_jit_gen_spill_reg(gen, reg, other_reg, value);
				}
				else
				{
					_jit_gen_spill_reg
						(gen, gen->contents[reg].remap, -1, value);
				}
				value->in_frame = 1;
				value_used = 1;
			}
			value->in_register = 0;
			value->reg = -1;
		}
	}

	/* The registers do not contain values any more */
	gen->contents[reg].num_values = 0;
	gen->contents[reg].used_for_temp = 0;
	if(other_reg != -1)
	{
		gen->contents[other_reg].num_values = 0;
		gen->contents[other_reg].used_for_temp = 0;
	}

	/* If the registers are members of a stack, then readjust the
	   stack mappings to compensate for the change */
	if(gen->contents[reg].remap != -1)
	{
		free_stack_reg(gen, reg);
	}
	if(other_reg != -1 && gen->contents[other_reg].remap != -1)
	{
		free_stack_reg(gen, other_reg);
	}

#ifdef JIT_REG_DEBUG
	dump_regs(gen, "leave free_reg_and_spill");
#endif

	/* Free the register using CPU-specific code */
	_jit_gen_free_reg(gen, reg, other_reg, value_used);
}

/*@
 * @deftypefun void _jit_regs_want_reg (jit_gencode_t gen, int reg, int for_long)
 * Tell the register allocator that we want a particular register
 * for a specific purpose.  The current contents of the register
 * are spilled.  If @code{reg} is part of a register pair, then the
 * other register in the pair will also be spilled.  If @code{reg}
 * is a stack register, then it should be the first one.
 *
 * This is typically used for x86 instructions that require operands
 * to be in certain registers (especially multiplication and division),
 * and we want to make sure that the register is free before we clobber it.
 * It is also used to make space in the x86 FPU for floating-point returns.
 *
 * This may return a different pseudo register number if @code{reg}
 * was a member of a stack and some other register was made free.
 * @end deftypefun
@*/
int _jit_regs_want_reg(jit_gencode_t gen, int reg, int for_long)
{
	int other_reg;
	if((_jit_reg_info[reg].flags & JIT_REG_IN_STACK) == 0)
	{
		/* Spill an ordinary register and its pair register */
		free_reg_and_spill(gen, reg, 0, 1);
		if(for_long)
		{
			other_reg = _jit_reg_info[reg].other_reg;
			if(other_reg != -1)
			{
				free_reg_and_spill(gen, other_reg, 0, 1);
			}
		}
		else
		{
			other_reg = -1;
		}

		/* Mark the register as touched and return it */
		jit_reg_set_used(gen->touched, reg);
		if(other_reg != -1)
		{
			jit_reg_set_used(gen->touched, other_reg);
		}
		return reg;
	}
	else
	{
		/* If we want a stack register, all we have to do is roll
		   everything down to make room for the new value.  If the
		   stack is full, then we spill the entire stack */
		return create_stack_reg(gen, reg, 0);
	}
}

/*@
 * @deftypefun void _jit_regs_free_reg (jit_gencode_t gen, int reg, int value_used)
 * Free the contents of a pseudo register, without spilling.  Used when
 * the contents of a register becomes invalid.  If @code{value_used}
 * is non-zero, then it indicates that the value has already been
 * used.  On some systems, an explicit instruction is needed to free
 * a register whose value hasn't been used yet (e.g. x86 floating point
 * stack registers).
 * @end deftypefun
@*/
void _jit_regs_free_reg(jit_gencode_t gen, int reg, int value_used)
{
	free_reg_and_spill(gen, reg, value_used, 0);
}

/*@
 * @deftypefun void _jit_regs_set_value (jit_gencode_t gen, int reg, jit_value_t value, int still_in_frame)
 * Set pseudo register @code{reg} to record that it currently holds the
 * contents of @code{value}.  The value is assumed to already be in
 * the register and no spill occurs.  If @code{still_in_frame} is
 * non-zero, then the value is still in the stack frame; otherwise the
 * value is exclusively in the register.
 * @end deftypefun
@*/
void _jit_regs_set_value
	(jit_gencode_t gen, int reg, jit_value_t value, int still_in_frame)
{
	int other_reg;
	int first_stack_reg;

#ifdef JIT_REG_DEBUG
	dump_regs(gen, "enter set_value");
#endif

	/* Get the other register in a pair */
	if(_jit_regs_needs_long_pair(value->type))
	{
		other_reg = _jit_reg_info[reg].other_reg;
	}
	else
	{
		other_reg = -1;
	}

	/* Mark the register as touched */
	jit_reg_set_used(gen->touched, reg);
	if(other_reg != -1)
	{
		jit_reg_set_used(gen->touched, other_reg);
	}

	/* Adjust the allocation state to reflect that "reg" contains "value" */
	gen->contents[reg].values[0] = value;
	gen->contents[reg].num_values = 1;
	gen->contents[reg].age = gen->current_age;
	if(other_reg == -1)
	{
		gen->contents[reg].is_long_start = 0;
		gen->contents[reg].is_long_end = 0;
		gen->contents[reg].used_for_temp = 0;
	}
	else
	{
		gen->contents[reg].is_long_start = 1;
		gen->contents[reg].is_long_end = 0;
		gen->contents[reg].used_for_temp = 0;
		gen->contents[other_reg].num_values = 0;
		gen->contents[other_reg].is_long_start = 0;
		gen->contents[other_reg].is_long_end = 1;
		gen->contents[other_reg].age = gen->current_age;
		gen->contents[other_reg].used_for_temp = 0;
	}
	(gen->current_age)++;

	/* Set the stack mappings for this register */
	if((_jit_reg_info[reg].flags & JIT_REG_IN_STACK) != 0)
	{
		first_stack_reg = reg;
		while((_jit_reg_info[first_stack_reg].flags & JIT_REG_START_STACK) == 0)
		{
			--first_stack_reg;
		}
		gen->contents[reg].remap = first_stack_reg;
		gen->stack_map[first_stack_reg] = reg;
	}

#ifdef JIT_REG_DEBUG
	dump_regs(gen, "leave set_value");
#endif

	/* Adjust the value to reflect that it is in "reg", and maybe the frame */
	value->in_register = 1;
	if(value->has_global_register)
		value->in_global_register = still_in_frame;
	else
		value->in_frame = still_in_frame;
	value->reg = (short)reg;
}

/*@
 * @deftypefun void _jit_regs_set_incoming (jit_gencode_t gen, int reg, jit_value_t value)
 * Set pseudo register @code{reg} to record that it currently holds the
 * contents of @code{value}.  If the register was previously in use,
 * then spill its value first.
 * @end deftypefun
@*/
void _jit_regs_set_incoming(jit_gencode_t gen, int reg, jit_value_t value)
{
	/* Eject any values that are currently in the register */
	reg = _jit_regs_want_reg(gen, reg, _jit_regs_needs_long_pair(value->type));

	/* Record that the value is in "reg", but not in the frame */
	_jit_regs_set_value(gen, reg, value, 0);
}

/*@
 * @deftypefun void _jit_regs_set_outgoing (jit_gencode_t gen, int reg, jit_value_t value)
 * Load the contents of @code{value} into pseudo register @code{reg},
 * spilling out the current contents.  This is used to set up outgoing
 * parameters for a function call.
 * @end deftypefun
@*/
void _jit_regs_set_outgoing(jit_gencode_t gen, int reg, jit_value_t value)
{
	int other_reg;
	int need_pair;

#ifdef JIT_BACKEND_X86
	jit_type_t type;
	type = jit_type_normalize(value->type);
	need_pair = 0;
	if(type)
	{
		/* We might need to put float values in register pairs under x86 */
		if(type->kind == JIT_TYPE_LONG || type->kind == JIT_TYPE_ULONG ||
		   type->kind == JIT_TYPE_FLOAT64 || type->kind == JIT_TYPE_NFLOAT)
		{
			need_pair = 1;
		}
	}
	if(value->in_register && value->reg == reg && !need_pair)
#else
	need_pair = _jit_regs_needs_long_pair(value->type);
	if(value->in_register && value->reg == reg)
#endif
	{
		/* The value is already in the register, but we may need to spill
		   if the frame copy is not up to date with the register */
		if(!(value->in_global_register) && !(value->in_frame) &&
		   !(value->is_temporary))
		{
			free_reg_and_spill(gen, reg, 1, 1);
		}

		/* The value is no longer "really" in the register.  A copy is
		   left behind, but the value itself reverts to the frame copy
		   as we are about to kill the registers in a function call */
		value->in_register = 0;
		value->reg = -1;
	}
	else
	{
		/* Force the value out of whatever register it is already in */
		_jit_regs_force_out(gen, value, 0);

		/* Reload the value into the specified register */
		if(need_pair)
		{
		#ifdef JIT_BACKEND_X86
			/* Long values in outgoing registers must be in ECX:EDX,
			   not in the ordinary register pairing of ECX:EBX */
			_jit_regs_want_reg(gen, reg, 0);
			other_reg = 2;
			_jit_regs_want_reg(gen, other_reg, 0);
		#else
			_jit_regs_want_reg(gen, reg, 1);
			other_reg = _jit_reg_info[reg].other_reg;
		#endif
			_jit_gen_load_value(gen, reg, other_reg, value);
			jit_reg_set_used(gen->inhibit, reg);
			jit_reg_set_used(gen->inhibit, other_reg);
		}
		else
		{
			_jit_regs_want_reg(gen, reg, 0);
			_jit_gen_load_value(gen, reg, -1, value);
			jit_reg_set_used(gen->inhibit, reg);
		}
	}
}

/*@
 * @deftypefun int _jit_regs_is_top (jit_gencode_t gen, jit_value_t value)
 * Determine if @code{value} is currently the in top-most position
 * in the appropriate register stack.  Always returns non-zero if
 * @code{value} is in a register, but that register is not part of a
 * register stack.  This is used to check if an operand value is
 * already in the right position for a unary operation.
 * @end deftypefun
@*/
int _jit_regs_is_top(jit_gencode_t gen, jit_value_t value)
{
	int reg, remap;
	if(!(value->in_register))
	{
		return 0;
	}
	reg = value->reg;
	if((_jit_reg_info[reg].flags & JIT_REG_IN_STACK) == 0)
	{
		return 1;
	}
	remap = gen->contents[reg].remap;
	if(remap != -1 && (_jit_reg_info[remap].flags & JIT_REG_START_STACK) != 0)
	{
		return 1;
	}
	return 0;
}

/*@
 * @deftypefun int _jit_regs_is_top_two (jit_gencode_t gen, jit_value_t value1, jit_value_t value2)
 * Determine if @code{value1} and @code{value2} are in the top two positions
 * in the appropriate register stack, and @code{value2} is above
 * @code{value1}.  Always returns non-zero if @code{value} and
 * @code{value2} are in registers, but those registers are not part
 * of a register stack.  This is used to check if the operand values
 * for a binary operation are already in the right positions.
 * @end deftypefun
@*/
int _jit_regs_is_top_two
	(jit_gencode_t gen, jit_value_t value1, jit_value_t value2)
{
	int reg, remap;
	if(!(value1->in_register) || !(value2->in_register))
	{
		return 0;
	}
	reg = value2->reg;
	if((_jit_reg_info[reg].flags & JIT_REG_IN_STACK) == 0)
	{
		reg = value1->reg;
		return ((_jit_reg_info[reg].flags & JIT_REG_IN_STACK) == 0);
	}
	remap = gen->contents[reg].remap;
	if(remap == -1 || (_jit_reg_info[remap].flags & JIT_REG_START_STACK) == 0)
	{
		return 0;
	}
	reg = value1->reg;
	if((_jit_reg_info[reg].flags & JIT_REG_IN_STACK) == 0)
	{
		return 1;
	}
	return (gen->contents[reg].remap == (remap + 1));
}

/*
 * Load a value into a register.
 */
static void load_value(jit_gencode_t gen, int reg, int other_reg,
					   jit_value_t value, int destroy)
{
	_jit_gen_load_value(gen, reg, other_reg, value);
	if(destroy || value->is_constant)
	{
		/* Mark the register as containing a temporary value */
		gen->contents[reg].used_for_temp = 1;
		jit_reg_set_used(gen->touched, reg);
		if(other_reg != -1)
		{
			gen->contents[reg].is_long_start = 1;
			gen->contents[other_reg].is_long_end = 1;
			gen->contents[other_reg].used_for_temp = 1;
			jit_reg_set_used(gen->touched, other_reg);
		}
	}
	else
	{
		/* Mark the register as containing the value we have loaded */
		if(value->has_global_register)
			_jit_regs_set_value(gen, reg, value, value->in_global_register);
		else
			_jit_regs_set_value(gen, reg, value, value->in_frame);
	}
}

/*
 * Find a free register to hold the contents of a value.
 */
static int free_register_for_value
	(jit_gencode_t gen, jit_value_t value, int *other_reg)
{
	int reg, type;
	int suitable_reg, need_pair;
	int suitable_age;

	/* Clear the other register before we start */
	*other_reg = -1;

	/* Determine if we need a long pair for this value */
	need_pair = _jit_regs_needs_long_pair(value->type);

	/* Determine the type of register that we need */
	switch(jit_type_normalize(value->type)->kind)
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
			type = JIT_REG_WORD;
		}
		break;

		case JIT_TYPE_LONG:
		case JIT_TYPE_ULONG:
		{
			if(need_pair)
				type = JIT_REG_LONG;
			else
				type = JIT_REG_WORD;
		}
		break;

		case JIT_TYPE_FLOAT32:
		{
			type = JIT_REG_FLOAT32;
		}
		break;

		case JIT_TYPE_FLOAT64:
		{
			type = JIT_REG_FLOAT64;
		}
		break;

		case JIT_TYPE_NFLOAT:
		{
			type = JIT_REG_NFLOAT;
		}
		break;

		default: return -1;
	}

	/* Search for a free register, ignoring permanent global allocations.
	   We also keep track of the oldest suitable register that is not free */
	suitable_reg = -1;
	suitable_age = -1;
	for(reg = 0; reg < JIT_NUM_REGS; ++reg)
	{
		if((_jit_reg_info[reg].flags & type) != 0 &&
		   !jit_reg_is_used(gen->permanent, reg) &&
		   !jit_reg_is_used(gen->inhibit, reg))
		{
			if((_jit_reg_info[reg].flags & JIT_REG_IN_STACK) != 0)
			{
				/* We always load stack values to the top of the stack */
				reg = create_stack_reg(gen, reg, 1);
				return reg;
			}
			else if(!need_pair)
			{
				if(gen->contents[reg].num_values == 0 &&
				   !(gen->contents[reg].used_for_temp) &&
				   !(gen->contents[reg].is_long_end))
				{
					return reg;
				}
			}
			else
			{
				*other_reg = _jit_reg_info[reg].other_reg;
				if(gen->contents[reg].num_values == 0 &&
				   !(gen->contents[reg].used_for_temp) &&
				   gen->contents[*other_reg].num_values == 0 &&
				   !(gen->contents[*other_reg].used_for_temp))
				{
					return reg;
				}
			}
			if(suitable_reg == -1 || gen->contents[reg].age < suitable_age)
			{
				/* This is the oldest suitable register of this type */
				suitable_reg = reg;
				suitable_age = gen->contents[reg].age;
			}
		}
	}

	/* If there were no suitable registers at all, then fail */
	if(suitable_reg == -1)
	{
		return -1;
	}

	/* Eject the current contents of the register */
	reg = _jit_regs_want_reg(gen, suitable_reg, need_pair);
	if(need_pair)
	{
		*other_reg = _jit_reg_info[reg].other_reg;
	}
	return reg;
}

/*@
 * @deftypefun int _jit_regs_load_value (jit_gencode_t gen, jit_value_t value, int destroy, int used_again)
 * Load a value into any register that is suitable and return that register.
 * If the value needs a long pair, then this will return the first register
 * in the pair.  Returns -1 if the value will not fit into any register.
 *
 * If @code{destroy} is non-zero, then we are about to destroy the register,
 * so the system must make sure that such destruction will not side-effect
 * @code{value} or any of the other values currently in that register.
 *
 * If @code{used_again} is non-zero, then it indicates that the value is
 * used again further down the block.
 * @end deftypefun
@*/
int _jit_regs_load_value
	(jit_gencode_t gen, jit_value_t value, int destroy, int used_again)
{
	int reg, other_reg, need_pair;

	/* Determine if we need a long pair for this value */
	need_pair = _jit_regs_needs_long_pair(value->type);

	/* If the value is already in a register, then try to use that register */
	if(value->in_register)
	{
		reg = value->reg;
		if(destroy)
		{
			if(gen->contents[reg].num_values == 1 &&
			   (value->in_frame || value->in_global_register || !used_again))
			{
				/* We are the only value in this register, and the
				   value is duplicated in the frame, or will never
				   be used again in this block.  In this case,
				   we can disassociate the register from the value
				   and just return the register as-is */
				value->in_register = 0;
				gen->contents[reg].num_values = 0;
				gen->contents[reg].used_for_temp = 1;
				gen->contents[reg].age = gen->current_age;
				if(need_pair)
				{
					other_reg = _jit_reg_info[reg].other_reg;
					gen->contents[other_reg].used_for_temp = 1;
					gen->contents[other_reg].age = gen->current_age;
				}
				++(gen->current_age);
				return reg;
			}
			else
			{
				/* We need to spill the register and then reload it */
				spill_register(gen, reg);
			}
		}
		else
		{
			if(gen->contents[reg].num_values == 1 &&
			   (value->in_frame || value->in_global_register || !used_again))
			{
				/* We are the only value in this register, and the
				   value is duplicated in the frame, or will never
				   be used again in this block.  In this case,
				   we can disassociate the register from the value
				   and just return the register as-is */
				value->in_register = 0;
				gen->contents[reg].num_values = 0;
				gen->contents[reg].used_for_temp = 1;
			}
			gen->contents[reg].age = gen->current_age;
			if(need_pair)
			{
				other_reg = _jit_reg_info[reg].other_reg;
				gen->contents[other_reg].age = gen->current_age;
			}
			++(gen->current_age);
			return reg;
		}
	}

	/* If the value is in a global register, and we are not going
	   to destroy the value, then use the global register itself.
	   This will avoid a redundant register copy operation */
	if(value->in_global_register && !destroy)
	{
		return value->global_reg;
	}

	/* Search for a free register to hold the value */
	reg = free_register_for_value(gen, value, &other_reg);
	load_value(gen, reg, other_reg, value, destroy);
	return reg;
}

/*@
 * @deftypefun int _jit_regs_dest_value (jit_gencode_t gen, jit_value_t value)
 * Get a new register to hold @code{value} as a destination.  This cannot
 * be used for stack register destinations (use @code{_jit_regs_new_top}
 * for that).
 * @end deftypefun
@*/
int _jit_regs_dest_value(jit_gencode_t gen, jit_value_t value)
{
	int reg, other_reg;

	/* If the value is exclusively in a register already, then use that */
	if(value->in_register)
	{
		reg = value->reg;
		if(gen->contents[reg].num_values == 1)
		{
			value->in_frame = 0;
			value->in_global_register = 0;
			return reg;
		}
		free_reg_and_spill(gen, reg, 0, 1);
	}

	/* Find a suitable register to hold the destination */
	reg = free_register_for_value(gen, value, &other_reg);
	_jit_regs_set_value(gen, reg, value, 0);
	return reg;
}

/*
 * Determine if "num" stack registers are free in a specific stack.
 */
static int stack_regs_free(jit_gencode_t gen, int reg, int num)
{
	int first_reg;

	/* Find the extents of the stack */
	while((_jit_reg_info[reg].flags & JIT_REG_START_STACK) == 0)
	{
		--reg;
	}
	first_reg = reg;
	while((_jit_reg_info[reg].flags & JIT_REG_END_STACK) == 0)
	{
		++reg;
	}

	/* Search for free registers */
	while(reg >= first_reg)
	{
		if(gen->contents[reg].num_values == 0 &&
		   !(gen->contents[reg].used_for_temp))
		{
			--num;
			if(num <= 0)
			{
				return 1;
			}
		}
		--reg;
	}
	return 0;
}

/*@
 * @deftypefun int _jit_regs_load_to_top (jit_gencode_t gen, jit_value_t value, int used_again, int type_reg)
 * Load the contents of @code{value} into a register that is guaranteed to
 * be at the top of its stack.  This is the preferred way to set up for a
 * unary operation on a stack-based architecture.  Returns the pseudo
 * register that contains the value.
 *
 * When @code{value} is loaded, the "destroy" flag is set so that the
 * unary operation will not affect the original contents of @code{value}.
 * The @code{used_again} flag indicates if @code{value} is used again
 * in the current basic block.
 *
 * The @code{type_reg} parameter should be set to the pseudo register
 * number of a suitable register.  This is used to determine which
 * register stack to use for the allocation.
 * @end deftypefun
@*/
int _jit_regs_load_to_top(jit_gencode_t gen, jit_value_t value, int used_again, int type_reg)
{
	int reg;

	/* Determine if the value is already in the top-most register */
	if(value->in_register)
	{
		reg = value->reg;
		if((_jit_reg_info[gen->contents[reg].remap].flags
				& JIT_REG_START_STACK) != 0)
		{
			if(value->in_frame || value->in_global_register || !used_again)
			{
				/* Disassociate the value from the register and return */
				value->in_register = 0;
				gen->contents[reg].num_values = 0;
				gen->contents[reg].used_for_temp = 1;
				gen->contents[reg].age = gen->current_age;
				++(gen->current_age);
				return reg;
			}
		}
		spill_all_stack(gen, type_reg);
	}

	/* If there are free registers of this type, then load the value now */
	if(stack_regs_free(gen, type_reg, 1))
	{
		return _jit_regs_load_value(gen, value, 1, used_again);
	}

	/* Spill the entire stack contents, to get things into a known state */
	spill_all_stack(gen, type_reg);

	/* Reload the value and return */
	return _jit_regs_load_value(gen, value, 1, used_again);
}

/*@
 * @deftypefun int _jit_regs_load_to_top_two (jit_gencode_t gen, jit_value_t value, jit_value_t value2, int used_again1, int used_again2, int type_reg)
 * Load the contents of @code{value} and @code{value2} into registers that
 * are guaranteed to be at the top of the relevant register stack.
 * This is the preferred way to set up for a binary operation on a
 * stack-based architecture.
 *
 * Returns the pseudo register that contains @code{value}.  The pseudo
 * register that contains @code{value2} is marked as free, because it is
 * assumed that the binary operation will immediately consume its value.
 *
 * When @code{value} are @code{value2} are loaded, the "destroy" flag is
 * set so that the binary operation will not affect their original contents.
 * The @code{used_again1} and @code{used_again2} flags indicate if
 * @code{value} and @code{value2} are used again in the current basic block.
 *
 * The @code{type_reg} parameter should be set to the pseudo register
 * number of a suitable register.  This is used to determine which
 * register stack to use for the allocation.
 * @end deftypefun
@*/
int _jit_regs_load_to_top_two
	(jit_gencode_t gen, jit_value_t value, jit_value_t value2,
	 int used_again1, int used_again2, int type_reg)
{
	int reg, reg2;

	/* Determine if the values are already in the top two registers */
	if(value->in_register && value2->in_register)
	{
		reg = value->reg;
		reg2 = value2->reg;
		if((_jit_reg_info[gen->contents[reg2].remap].flags
				& JIT_REG_START_STACK) != 0 &&
		   gen->contents[reg].remap == (gen->contents[reg2].remap + 1))
		{
			if((value->in_frame || value->in_global_register || !used_again1) &&
			   (value2->in_frame || value2->in_global_register || !used_again2))
			{
				/* Disassociate the values from the registers and return */
				free_stack_reg(gen, reg2);
				value->in_register = 0;
				value2->in_register = 0;
				gen->contents[reg].num_values = 0;
				gen->contents[reg].used_for_temp = 1;
				gen->contents[reg].age = gen->current_age;
				gen->contents[reg2].num_values = 0;
				gen->contents[reg2].used_for_temp = 0;
				gen->contents[reg2].age = gen->current_age;
				++(gen->current_age);
				return reg;
			}
		}
		spill_all_stack(gen, type_reg);
	}
	else if(value2->in_register && !(value->in_register))
	{
		/* We'll probably need to rearrange the stack, so spill first */
		spill_all_stack(gen, type_reg);
	}

	/* If there are free registers of this type, then load the values now */
	if(stack_regs_free(gen, type_reg, 2))
	{
		reg = _jit_regs_load_value(gen, value, 1, used_again1);
		reg2 = _jit_regs_load_value(gen, value2, 1, used_again2);
		free_stack_reg(gen, reg2);
		gen->contents[reg2].used_for_temp = 0;
		return reg;
	}

	/* Spill the entire stack contents, to get things into a known state */
	spill_all_stack(gen, type_reg);

	/* Reload the values and return */
	reg = _jit_regs_load_value(gen, value, 1, used_again1);
	reg2 = _jit_regs_load_value(gen, value2, 1, used_again2);
	free_stack_reg(gen, reg2);
	gen->contents[reg2].used_for_temp = 0;
	return reg;
}

/*@
 * @deftypefun void _jit_regs_load_to_top_three (jit_gencode_t gen, jit_value_t value, jit_value_t value2, jit_value_t value3, int used_again1, int used_again2, int used_again3, int type_reg)
 * Load three values to the top of a register stack.  The values are assumed
 * to be popped by the subsequent operation.  This is used by the interpreted
 * back end for things like array stores, that need three values but all
 * of them are discarded after the operation.
 * @end deftypefun
@*/
void _jit_regs_load_to_top_three
	(jit_gencode_t gen, jit_value_t value, jit_value_t value2,
	 jit_value_t value3, int used_again1, int used_again2,
	 int used_again3, int type_reg)
{
	int reg, reg2, reg3;

	/* Determine if the values are already in the top three registers */
	if(value->in_register && value2->in_register && value3->in_register)
	{
		reg = value->reg;
		reg2 = value2->reg;
		reg3 = value3->reg;
		if((_jit_reg_info[gen->contents[reg2].remap].flags
				& JIT_REG_START_STACK) != 0 &&
		   gen->contents[reg].remap == (gen->contents[reg2].remap + 1) &&
		   gen->contents[reg2].remap == (gen->contents[reg3].remap + 1))
		{
			if((value->in_frame || value->in_global_register || !used_again1) &&
			   (value2->in_frame || value2->in_global_register ||
			   			!used_again2) &&
			   (value3->in_frame || value3->in_global_register || !used_again3))
			{
				/* Disassociate the values from the registers and return */
				free_stack_reg(gen, reg);
				free_stack_reg(gen, reg2);
				free_stack_reg(gen, reg3);
				value->in_register = 0;
				value2->in_register = 0;
				value3->in_register = 0;
				gen->contents[reg].used_for_temp = 0;
				gen->contents[reg2].used_for_temp = 0;
				gen->contents[reg3].used_for_temp = 0;
				return;
			}
		}
	}

	/* Spill everything out, so that we know where things are */
	spill_all_stack(gen, type_reg);

	/* Load the three values that we want onto the stack */
	reg = _jit_regs_load_value(gen, value, 1, used_again1);
	reg2 = _jit_regs_load_value(gen, value2, 1, used_again2);
	reg3 = _jit_regs_load_value(gen, value3, 1, used_again3);
	gen->contents[reg].used_for_temp = 0;
	gen->contents[reg2].used_for_temp = 0;
	gen->contents[reg3].used_for_temp = 0;
}

/*@
 * @deftypefun int _jit_regs_num_used (jit_gencode_t gen, int type_reg)
 * Get the number of stack registers in use within the register stack
 * indicated by @code{type_reg}.
 * @end deftypefun
@*/
int _jit_regs_num_used(jit_gencode_t gen, int type_reg)
{
	int count;
	while((_jit_reg_info[type_reg].flags & JIT_REG_START_STACK) == 0)
	{
		--type_reg;
	}
	count = 0;
	for(;;)
	{
		if(gen->contents[type_reg].num_values > 0 ||
		   gen->contents[type_reg].used_for_temp)
		{
			++count;
		}
		if((_jit_reg_info[type_reg].flags & JIT_REG_END_STACK) == 0)
		{
			break;
		}
		++type_reg;
	}
	return count;
}

/*@
 * @deftypefun int _jit_regs_new_top (jit_gencode_t gen, jit_value_t value, int type_reg)
 * Record that the top of the stack indicated by @code{type_reg} now
 * contains @code{value}.  This is slightly different from
 * @code{_jit_regs_set_value}, in that the register wasn't previously
 * allocated to a temporary operand value.  Returns the actual stack
 * register that contains @code{value}.
 * @end deftypefun
@*/
int _jit_regs_new_top(jit_gencode_t gen, jit_value_t value, int type_reg)
{
	int reg;
	
	/* Create space for the value at the top of the stack */
	reg = create_stack_reg(gen, type_reg, 1);

	/* Record the "value" is now in this register */
	value->in_register = 1;
	value->in_frame = 0;
	value->in_global_register = 0;
	value->reg = reg;
	gen->contents[reg].values[0] = value;
	gen->contents[reg].num_values = 1;

	/* Return the allocated register to the caller */
	return reg;
}

/*@
 * @deftypefun void _jit_regs_force_out (jit_gencode_t gen, jit_value_t value, int is_dest)
 * If @code{value} is currently in a register, then force its value out
 * into the stack frame.  The @code{is_dest} flag indicates that the value
 * will be a destination, so we don't care about the original value.
 * @end deftypefun
@*/
void _jit_regs_force_out(jit_gencode_t gen, jit_value_t value, int is_dest)
{
	if(value->in_register)
	{
		if((_jit_reg_info[value->reg].flags & JIT_REG_IN_STACK) == 0)
		{
			free_reg_and_spill(gen, value->reg, 0, !is_dest);
		}
		else
		{
			/* Always do a spill for a stack register */
			spill_register(gen, value->reg);
		}
	}
}

/*
 * Minimum number of times a candidate must be used before it
 * is considered worthy of putting in a global register.
 */
#define	JIT_MIN_USED		3

/*@
 * @deftypefun void _jit_regs_alloc_global (jit_gencode_t gen, jit_function_t func)
 * Perform global register allocation on the values in @code{func}.
 * This is called during function compilation just after variable
 * liveness has been computed.
 * @end deftypefun
@*/
void _jit_regs_alloc_global(jit_gencode_t gen, jit_function_t func)
{
#if JIT_NUM_GLOBAL_REGS != 0
	jit_value_t candidates[JIT_NUM_GLOBAL_REGS];
	int num_candidates = 0;
	int index, reg, posn, num;
	jit_pool_block_t block;
	jit_value_t value, temp;

	/* If the function has a "try" block, then don't do global allocation
	   as the "longjmp" for exception throws will wipe out global registers */
	if(func->has_try)
	{
		return;
	}

	/* If the current function involves a tail call, then we don't do
	   global register allocation and we also prevent the code generator
	   from using any of the callee-saved registers.  This simplifies
	   tail calls, which don't have to worry about restoring such registers */
	if(func->builder->has_tail_call)
	{
		for(reg = 0; reg < JIT_NUM_REGS; ++reg)
		{
			if((_jit_reg_info[reg].flags &
					(JIT_REG_FIXED | JIT_REG_CALL_USED)) == 0)
			{
				jit_reg_set_used(gen->permanent, reg);
			}
		}
		return;
	}

	/* Scan all values within the function, looking for the most used.
	   We will replace this with a better allocation strategy later */
	block = func->builder->value_pool.blocks;
	num = (int)(func->builder->value_pool.elems_per_block);
	while(block != 0)
	{
		if(!(block->next))
		{
			num = (int)(func->builder->value_pool.elems_in_last);
		}
		for(posn = 0; posn < num; ++posn)
		{
			value = (jit_value_t)(block->data + posn *
								  sizeof(struct _jit_value));
			if(value->global_candidate && value->usage_count >= JIT_MIN_USED &&
			   !(value->is_addressable) && !(value->is_volatile))
			{
				/* Insert this candidate into the list, ordered on count */
				index = 0;
				while(index < num_candidates &&
				      value->usage_count <= candidates[index]->usage_count)
				{
					++index;
				}
				while(index < num_candidates)
				{
					temp = candidates[index];
					candidates[index] = value;
					value = temp;
					++index;
				}
				if(index < JIT_NUM_GLOBAL_REGS)
				{
					candidates[num_candidates++] = value;
				}
			}
		}
		block = block->next;
	}

	/* Allocate registers to the candidates.  We allocate from the top-most
	   register in the allocation order, because some architectures like
	   PPC require global registers to be saved top-down for efficiency */
	reg = JIT_NUM_REGS - 1;
	for(index = 0; index < num_candidates; ++index)
	{
		while(reg >= 0 && (_jit_reg_info[reg].flags & JIT_REG_GLOBAL) == 0)
		{
			--reg;
		}
		candidates[index]->has_global_register = 1;
		candidates[index]->global_reg = (short)reg;
		jit_reg_set_used(gen->touched, reg);
		jit_reg_set_used(gen->permanent, reg);
		--reg;
	}

#endif
}

/*
 * @deftypefun void _jit_regs_get_reg_pair (jit_gencode_t gen, int not_this1, int not_this2, int not_this3, {int *} reg, {int *} reg2)
 * Get a register pair for temporary operations on "long" values.
 * @end deftypefun
 */
void _jit_regs_get_reg_pair(jit_gencode_t gen, int not_this1, int not_this2,
						    int not_this3, int *reg, int *reg2)
{
	int index;
	for(index = 0; index < 8; ++index)
	{
		if((_jit_reg_info[index].flags & JIT_REG_WORD) == 0 ||
		   jit_reg_is_used(gen->permanent, index))
		{
			continue;
		}
		if(index != not_this1 && index != not_this2 &&
		   index != not_this3)
		{
			break;
		}
	}
	*reg = index;
	_jit_regs_want_reg(gen, index, 0);
	if(!reg2)
	{
		return;
	}
	for(; index < 8; ++index)
	{
		if((_jit_reg_info[index].flags & JIT_REG_WORD) == 0 ||
		   jit_reg_is_used(gen->permanent, index))
		{
			continue;
		}
		if(index != not_this1 && index != not_this2 &&
		   index != not_this3 && index != *reg)
		{
			break;
		}
	}
	if(index >= 8)
	{
		*reg2 = -1;
	}
	else
	{
		*reg2 = index;
		_jit_regs_want_reg(gen, index, 0);
	}
}

/*
 * New Reg Alloc API
 */

#define IS_STACK_REG(reg)	((_jit_reg_info[reg].flags & JIT_REG_IN_STACK) != 0)
#define IS_STACK_START(reg)	((_jit_reg_info[reg].flags & JIT_REG_START_STACK) != 0)
#define IS_STACK_END(reg)	((_jit_reg_info[reg].flags & JIT_REG_END_STACK) != 0)
#define OTHER_REG(reg)		(_jit_reg_info[reg].other_reg)

/* The cost value that precludes using the register in question. */
#define COST_TOO_MUCH		1000000

/* Value usage flags. */
#define VALUE_INPUT		1
#define VALUE_OUTPUT		2
#define VALUE_USED		4

/* Clobber flags. */
#define CLOBBER_NONE		0
#define CLOBBER_INPUT_VALUE	1
#define CLOBBER_REG		2
#define CLOBBER_OTHER_REG	4

/*
 * For a stack register find the first stack register.
 */
static int
get_stack_start(int reg)
{
	if(IS_STACK_REG(reg))
	{
		while(!IS_STACK_START(reg))
		{
			--reg;
		}
	}
	return reg;
}

/*
 * Find the stack top given the first stack register,
 */
static int
get_stack_top(jit_gencode_t gen, int stack_start)
{
	if(gen->stack_map[stack_start] < 0)
	{
		return (stack_start - 1);
	}
	return (gen->stack_map[stack_start]);
}

/*
 * Determine the type of register that we need.
 */
static int
get_register_type(jit_value_t value, int need_pair)
{
	switch(jit_type_normalize(value->type)->kind)
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
		return JIT_REG_WORD;

	case JIT_TYPE_LONG:
	case JIT_TYPE_ULONG:
		return need_pair ? JIT_REG_LONG : JIT_REG_WORD;

	case JIT_TYPE_FLOAT32:
		return JIT_REG_FLOAT32;

	case JIT_TYPE_FLOAT64:
		return JIT_REG_FLOAT64;

	case JIT_TYPE_NFLOAT:
		return JIT_REG_NFLOAT;
	}

	return 0;
}

/*
 * Check if two values are known to be equal.
 */
static int
are_values_equal(_jit_regdesc_t *desc1, _jit_regdesc_t *desc2)
{
	if(desc1->value && desc2->value)
	{
		if(desc1->value == desc2->value)
		{
			return 1;
		}
		if(desc1->value->in_register && desc2->value->in_register)
		{
			return desc1->value->reg == desc2->value->reg;
		}
	}
	return 0;
}


/*
 * Check if the value is used in and after the current instruction.
 */
static int
value_usage(_jit_regs_t *regs, jit_value_t value)
{
	int flags = 0;
	if(value == regs->descs[0].value)
	{
		flags |= regs->ternary ? VALUE_INPUT : VALUE_OUTPUT;
		if(regs->descs[0].used || regs->descs[0].live)
		{
			flags |= VALUE_USED;
		}
	}
	if(value == regs->descs[1].value)
	{
		flags |= VALUE_INPUT;
		if(regs->descs[1].used || regs->descs[1].live)
		{
			flags |= VALUE_USED;
		}
	}
	if(value == regs->descs[2].value)
	{
		flags |= VALUE_INPUT;
		if(regs->descs[2].used || regs->descs[2].live)
		{
			flags |= VALUE_USED;
		}
	}
	return flags;
}

/*
 * Check if the register contains any live values.
 */
static int
is_register_alive(jit_gencode_t gen, _jit_regs_t *regs, int reg)
{
	int index, usage;
	jit_value_t value;

	if(reg < 0)
	{
		return 0;
	}

	for(index = 0; index < gen->contents[reg].num_values; index++)
	{
		value = gen->contents[reg].values[index];
		if(value->is_constant)
		{
			continue;
		}

		/* The value is dead if it is the output value of the
		   instruction and hence just about to be recomputed
		   or if it is one of the input values and its usage
		   flags are not set. Otherwise the value is alive. */
		usage = value_usage(regs, value);
		if((usage & VALUE_OUTPUT) == 0
		   && ((usage & VALUE_INPUT) == 0
		       || (usage & VALUE_USED) != 0))
		{
			return 1;
		}
	}
	return 0;
}

/*
 * Determine the effect of assigning the value to a register. It finds out
 * if the old register contents is clobbered and so needs to be spilled and
 * if the newly assigned value will be clobbered by the instruction.
 */
static int
clobbers_register(jit_gencode_t gen, _jit_regs_t *regs, int index, int reg, int other_reg)
{
	int is_alive, is_other_alive;
	int clobber, out_index, flags;

	if(index < 0)
	{
		/* this call is made for a scratch register */
		return CLOBBER_REG;
	}
	if(!regs->descs[index].value)
	{
		return CLOBBER_NONE;
	}

	is_alive = is_register_alive(gen, regs, reg);
	is_other_alive = is_register_alive(gen, regs, other_reg);

	clobber = 0;
	if(regs->ternary || !regs->descs[0].value)
	{
		/* this is either a ternary op or a binary/unary note */
		if(IS_STACK_REG(reg))
		{
			/* all input values are popped */
			clobber = 1;
		}
	}
	else
	{
		/* find out the input value that is going to be overwritten by the output */
		if(!regs->descs[2].value)
		{
			/* a unary op */
			out_index = 1;
		}
		else
		{
			/* a binary op */
			if(regs->on_stack)
			{
				if(!regs->no_pop)
				{
					/* the input value is either overwritten
					   by the output or popped */
					clobber = 1;
				}
				out_index = 1 + (regs->reverse_dest ^ regs->reverse_args);
			}
			else
			{
				out_index = 1;
			}
		}

		/* does the output value clobber the register? */
		if(index == 0 || index == out_index)
		{
			if(regs->descs[0].value->has_global_register
			   && regs->descs[0].value->global_reg == reg)
			{
				return CLOBBER_NONE;
			}

			flags = CLOBBER_NONE;
			if(regs->descs[out_index].value != regs->descs[0].value)
			{
				flags |= CLOBBER_INPUT_VALUE;
			}
			if(is_alive)
			{
				flags |= CLOBBER_REG;
			}
			if(is_other_alive)
			{
				flags |= CLOBBER_OTHER_REG;
			}
			return flags;
		}
	}

	/* does input value clobber the register? */
	if(regs->descs[index].clobber)
	{
		clobber = 1;
	}

	if(!clobber)
	{
		if(regs->descs[index].value->has_global_register
		   && regs->descs[index].value->global_reg == reg)
		{
			return CLOBBER_NONE;
		}
		if(regs->descs[index].value->in_register
		   && regs->descs[index].value->reg == reg)
		{
			return CLOBBER_NONE;
		}
	}

	flags = CLOBBER_NONE;
	if(clobber)
	{
		flags |= CLOBBER_INPUT_VALUE;
	}
	if(is_alive)
	{
		flags |= CLOBBER_REG;
	}
	if(is_other_alive)
	{
		flags |= CLOBBER_OTHER_REG;
	}
	return flags;
}

/*
 * Set assigned and clobber flags for a register.
 */
static void
set_register_flags(
	jit_gencode_t gen,
	_jit_regs_t *regs,
	int reg,
	int clobber_reg,
	int clobber_input)
{
	if(reg >= 0)
	{
		jit_reg_set_used(gen->touched, reg);
		jit_reg_set_used(regs->assigned, reg);
		if(clobber_reg)
		{
			jit_reg_set_used(regs->spill, reg);
		}
		if(clobber_input)
		{
			jit_reg_set_used(regs->clobber, reg);
		}
	}
}

static void
init_regdesc(_jit_regs_t *regs, int index)
{
	_jit_regdesc_t *desc;

	desc = &regs->descs[index];
	desc->value = 0;
	desc->reg = -1;
	desc->other_reg = -1;
	desc->stack_reg = -1;
	desc->regset = jit_regused_init_used;
	desc->live = 0;
	desc->used = 0;
	desc->clobber = 0;
	desc->early_clobber = 0;
	desc->duplicate = 0;
	desc->load = 0;
	desc->copy = 0;
}

/*
 * Initialize register descriptor.
 */
static void
set_regdesc_value(_jit_regs_t *regs, int index, jit_value_t value, int flags, int live, int used)
{
	_jit_regdesc_t *desc;

	desc = &regs->descs[index];
	desc->value = value;
	if(index > 0 || regs->ternary)
	{
		if((flags & _JIT_REGS_EARLY_CLOBBER) != 0)
		{
			desc->clobber = 1;
			desc->early_clobber = 1;
		}
		else if((flags & _JIT_REGS_CLOBBER) != 0)
		{
			desc->clobber = 1;
		}
	}
	desc->live = live;
	desc->used = used;
}

/*
 * Set assigned and clobber flags for the register descriptor.
 */
static void
set_regdesc_register(jit_gencode_t gen, _jit_regs_t *regs, int index, int reg, int other_reg)
{
	int clobber;

	if(reg >= 0)
	{
		regs->descs[index].reg = reg;
		regs->descs[index].other_reg = other_reg;

		clobber = clobbers_register(gen, regs, index, reg, other_reg);
		set_register_flags(gen, regs, reg,
				   (clobber & CLOBBER_REG),
				   (clobber & CLOBBER_INPUT_VALUE));
		if(other_reg >= 0)
		{
			set_register_flags(gen, regs, other_reg,
					   (clobber & CLOBBER_OTHER_REG),
					   (clobber & CLOBBER_INPUT_VALUE));
		}
	}
}

static int
collect_register_info(_jit_regs_t *regs, int index)
{
	_jit_regdesc_t *desc;
	int stack_start;

	desc = &regs->descs[index];
	if(desc->reg < 0)
	{
		return 1;
	}
	if(desc->duplicate)
	{
		return 1;
	}

	if(desc->value->has_global_register)
	{
		if(desc->value->global_reg != desc->reg
		   && !(desc->value->in_register && desc->value->reg == desc->reg))
		{
			desc->copy = 1;
		}
	}
	else
	{
		if(!desc->value->in_register)
		{
			desc->load = 1;
		}
		else if(desc->value->reg != desc->reg)
		{
			desc->copy = 1;
		}
	}

	if(IS_STACK_REG(desc->reg))
	{
		stack_start = get_stack_start(desc->reg);
		if(regs->stack_start < 0)
		{
			regs->stack_start = stack_start;
		}
		else if(stack_start != regs->stack_start)
		{
			return 0;
		}

		if(index > 0 || regs->ternary)
		{
			++(regs->wanted_stack_count);
			if(!(desc->load || desc->copy))
			{
				++(regs->loaded_stack_count);
			}
		}
	}

	return 1;
}

/*
 * To avoid circular movements of input values in place of each other
 * check if the current value overwrites any of the others.
 */
static int
thrashes_register(jit_gencode_t gen, _jit_regs_t *regs,
		  _jit_regdesc_t *desc, int reg, int other_reg)
{
	int reg2, other_reg2;
	if(regs->ternary && regs->descs[0].value && regs->descs[0].value->in_register
	   && !(desc && are_values_equal(&regs->descs[0], desc)))
	{
		reg2 = regs->descs[0].value->reg;
		if(reg2 == reg || reg2 == other_reg)
		{
			return 1;
		}
		if(gen->contents[reg2].is_long_start)
		{
			other_reg2 = OTHER_REG(reg2);
			if(other_reg2 == reg /*|| other_reg2 == other_reg*/)
			{
				return 1;
			}
		}
	}
	if(regs->descs[1].value && regs->descs[1].value->in_register
	   && !(desc && are_values_equal(&regs->descs[1], desc)))
	{
		reg2 = regs->descs[1].value->reg;
		if(reg2 == reg || reg2 == other_reg)
		{
			return 1;
		}
		if(gen->contents[reg2].is_long_start)
		{
			other_reg2 = OTHER_REG(reg2);
			if(other_reg2 == reg /*|| other_reg2 == other_reg*/)
			{
				return 1;
			}
		}
	}
	if(regs->descs[2].value && regs->descs[2].value->in_register
	   && !(desc && are_values_equal(&regs->descs[2], desc)))
	{
		reg2 = regs->descs[2].value->reg;
		if(reg2 == reg || reg2 == other_reg)
		{
			return 1;
		}
		if(gen->contents[reg2].is_long_start)
		{
			other_reg2 = OTHER_REG(reg2);
			if(other_reg2 == reg /*|| other_reg2 == other_reg*/)
			{
				return 1;
			}
		}
	}
	return 0;
}

static int
compute_spill_cost(jit_gencode_t gen, _jit_regs_t *regs, int reg, int other_reg)
{
	int cost, index, usage;
	jit_value_t value;

	cost = 0;
	for(index = 0; index < gen->contents[reg].num_values; index++)
	{
		value = gen->contents[reg].values[index];
		if(value->is_constant)
		{
			continue;
		}
		usage = value_usage(regs, value);
		if((usage & VALUE_OUTPUT) != 0)
		{
			continue;
		}
		if((usage & VALUE_INPUT) != 0 && (usage & VALUE_USED) == 0)
		{
			continue;
		}
		if(value->has_global_register)
		{
			if(!value->in_global_register)
			{
				cost += 1;
			}
		}
		else
		{
			if(!value->in_frame)
			{
				cost += 10;
			}
		}
	}
	if(other_reg >= 0)
	{
		for(index = 0; index < gen->contents[other_reg].num_values; index++)
		{
			value = gen->contents[other_reg].values[index];
			if(value->is_constant)
			{
				continue;
			}
			usage = value_usage(regs, value);
			if((usage & VALUE_OUTPUT) != 0)
			{
				continue;
			}
			if((usage & VALUE_INPUT) != 0 && (usage & VALUE_USED) == 0)
			{
				continue;
			}
			if(value->has_global_register)
			{
				if(!value->in_global_register)
				{
					cost += 1;
				}
			}
			else
			{
				if(!value->in_frame)
				{
					cost += 10;
				}
			}
		}
	}
	return cost;
}

/* TODO: update comments */
/*
 * Assign value to the register it is already in if possible. This is the case
 * if the register is not already assigned to and one of the following is true:
 *  1. The value is output and it the only value in register.
 *  2. The value is input and it is not clobbered.
 *  3. The value is input and it the only value in register, it is clobbered
 *     but not used afterwards.
 *  4. The value is input and it is clobbered even if we do not use its
 *     register. This might be because the instruction clobbers all or
 *     some registers (see _jit_regs_clobber_all(), jit_regs_clobber()).
 * NOTE: In the last case it might be possible to find a not clobbered register
 * where the value could be moved to so that the original copy can be used for
 * input without spilling. However probaly this corner case is not worth the
 * effort.
 */
/*
 * Assign value to a register that is cheapest to use. We are here either
 * because the value is not in register or it is but the register will be
 * clobbered so the reuse_register() function failed to assign the register.
 *
 * Depending on the value location and on the presence of a free register we
 * do one of the following:
 *
 * 1. The value is in register and there is a free register.
 *    In this case we will generate a reg-to-reg move.
 * 2. The value is in register and there are no free regsiters.
 *    In this case we will generate a spill if the register is dirty.
 * 3. The value is in frame and there is a free register.
 *    In this case we will generate a value load.
 * 4. The value is in frame and there are no free registers.
 *    In this case we choose a register to evict. We will generate a
 *    spill for the dirty register and load the value there.
 *
 * In the last case we choose the register using the following rules:
 *
 * 1. Choose clean registers over dirty.
 * 2. Choose registers that contain global values over those that don't.
 * 2. Choose old registers over new.
 *
 * NOTE: A register is clean if the value it contains has not changed since
 * it was loaded form the frame. Otherwise it is dirty. There is no need to
 * spill clean registers. Dirty registers has to be spilled.
 *
 * TODO: build use lists in CFG and choose the register on the basis of next
 * value use instead of the register age.
 *
 */
static int
use_cheapest_register(jit_gencode_t gen, _jit_regs_t *regs, int index, jit_regused_t regset)
{
	_jit_regdesc_t *desc;
	int output;
	int type;
	int need_pair;
	int reg, other_reg;
	int suitable_reg;
	int suitable_cost;
	int suitable_age;
	int cost;

	if(index >= 0)
	{
		desc = &regs->descs[index];
		if(!desc->value)
		{
			return -1;
		}
		output = (index == 0 && !regs->ternary);
		need_pair = _jit_regs_needs_long_pair(desc->value->type);
		type = get_register_type(desc->value, need_pair);
		if(!type)
		{
			return -1;
		}
	}
	else
	{
		desc = 0;
		output = 0;
		need_pair = 0;
		type = JIT_REG_WORD;
	}

	suitable_reg = -1;
	suitable_cost = COST_TOO_MUCH;
	suitable_age = -1;
	for(reg = 0; reg < JIT_NUM_REGS; reg++)
	{
		if((_jit_reg_info[reg].flags & type) == 0)
		{
			continue;
		}

		if(need_pair)
		{
			other_reg = OTHER_REG(reg);
		}
		else
		{
			other_reg = -1;
		}

		if(!jit_reg_is_used(regset, reg)
		   || jit_reg_is_used(gen->inhibit, reg)
		   || jit_reg_is_used(regs->assigned, reg))
		{
			continue;
		}
		if(other_reg >= 0)
		{
			if(jit_reg_is_used(gen->inhibit, other_reg)
			   || jit_reg_is_used(regs->assigned, other_reg))
			{
				continue;
			}
		}

		if(jit_reg_is_used(gen->permanent, reg))
		{
			/* We can use a global register only if it is the register
			   that contains the value itself and it is not clobbered. */
			if(desc
			   && desc->value->has_global_register
			   && desc->value->global_reg == reg
			   && !clobbers_register(gen, regs, index, reg, other_reg))
			{
				if(output || desc->value->in_global_register)
				{
					cost = 0;
				}
				else
				{
					cost = 1;
				}
			}
			else
			{
				cost = COST_TOO_MUCH;
			}
		}
		else if(other_reg >= 0 && jit_reg_is_used(gen->permanent, other_reg))
		{
			cost = COST_TOO_MUCH;
		}
		else if(thrashes_register(gen, regs, desc, reg, other_reg))
		{
			cost = COST_TOO_MUCH;
		}
		else if(desc && desc->value->in_register)
		{
			if(reg == desc->value->reg)
			{
				if(clobbers_register(gen, regs, index, reg, other_reg)
				   && !(jit_reg_is_used(regs->clobber, reg)
					|| (other_reg >= 0
					    && jit_reg_is_used(regs->clobber, other_reg))))
				{
					cost = compute_spill_cost(gen, regs, reg, other_reg);
				}
				else
				{
					cost = 0;
				}
			}
			else
			{
				cost = 1 + compute_spill_cost(gen, regs, reg, other_reg);
			}
		}
		else
		{
			cost = 10 + compute_spill_cost(gen, regs, reg, other_reg);
		}

		if(cost < suitable_cost
		   || (cost > 0 && cost == suitable_cost
		       && gen->contents[reg].age < suitable_age))
		{
			/* This is the oldest suitable register of this type */
			suitable_reg = reg;
			suitable_cost = cost;
			suitable_age = gen->contents[reg].age;
		}
	}

	reg = suitable_reg;
	if(desc && reg >= 0)
	{
		if(need_pair)
		{
			other_reg = OTHER_REG(reg);
		}
		else
		{
			other_reg = -1;
		}
		set_regdesc_register(gen, regs, index, reg, other_reg);
	}

	return reg;
}

/*
 * Assign diplicate input value to the same register if possible.
 * The first value has to be already assigned. The second value
 * is assigned to the same register if it is equal to the first
 * and neither of them is clobbered.
 */
static void
check_duplicate_value(_jit_regs_t *regs, _jit_regdesc_t *desc1, _jit_regdesc_t *desc2)
{
	if((!regs->on_stack || regs->x87_arith)
	   && are_values_equal(desc1, desc2)
	   && desc1->reg >= 0 && desc2->reg < 0
	   && !desc1->early_clobber && !desc2->early_clobber)
	{
		desc2->reg = desc1->reg;
		desc2->other_reg = desc1->other_reg;
		desc2->duplicate = 1;
	}
}

/*
 * Select the best argument order for binary ops. The posibility to select
 * the order exists only for commutative ops and for some x87 floating point
 * instructions. Those x87 instructions have variants with reversed argument
 * order or reversed destination register. Also they have variants that either
 * do or do not pop the stack top.
 */
static void
select_order(jit_gencode_t gen, _jit_regs_t *regs)
{
	_jit_regdesc_t *desc0;
	_jit_regdesc_t *desc1;
	_jit_regdesc_t *desc2;
	_jit_regdesc_t temp_desc;
	int keep1, keep2;
	int out_index;

	if(regs->ternary || !(regs->commutative || regs->x87_arith))
	{
		/* Bail out on ternary or non-commutative and non-x87 instruction. */
		return;
	}

	desc0 = &regs->descs[0];
	desc1 = &regs->descs[1];
	desc2 = &regs->descs[2];

	if(!desc0->value || !desc1->value || !desc2->value)
	{
		/* Bail out on binary notes or unary ops. */
		return;
	}

	/* Determine if we might want to keep either of input values
	   in registers after the operation completion. */
	if(regs->clobber_all)
	{
		keep1 = 0;
		keep2 = 0;
	}
	else
	{
		keep1 = desc1->used && (desc1->value != desc0->value) && !desc1->clobber;
		keep2 = desc2->used && (desc2->value != desc0->value) && !desc2->clobber;
	}

	/* Choose between pop and no-pop instructions. */
	if(regs->on_stack)
	{
		regs->no_pop = (regs->x87_arith && (keep1 || keep2));
	}
	else
	{
		regs->no_pop = 0;
	}

	/* Choose the input value to be clobbered by output. */
	if(regs->on_stack ? regs->no_pop : regs->commutative)
	{
		if(keep1 && keep2)
		{
			/* TODO: take into account that value might by copied to
			   another register */
			if(desc1->value->in_register)
			{
				if(desc2->value->in_register)
				{
					/* TODO: compare spill cost and live ranges  */
					out_index = 1;
				}
				else
				{
					/* TODO: compare spill cost and live ranges  */
					out_index = 2;
				}
			}
			else
			{
				if(desc2->value->in_register)
				{
					/* TODO: compare spill cost and live ranges  */
					out_index = 1;
				}
				else
				{
					/* TODO: use live ranges  */
					out_index = 1;
				}
			}
		}
		else if(keep1)
		{
			out_index = 2;
		}
		else if(keep2)
		{
			out_index = 1;
		}
		else
		{
			if(!(desc1->used || desc1->live))
			{
				out_index = 1;
			}
			else if(!(desc2->used || desc2->live))
			{
				out_index = 2;
			}
			else
			{
				out_index = 1;
			}
		}

		if(out_index == 2)
		{
			if(regs->on_stack)
			{
				regs->reverse_dest = 1;
			}
			else if(regs->commutative)
			{
				temp_desc = *desc1;
				*desc1 = *desc2;
				*desc2 = temp_desc;
			}
		}
	}

	return;
}

static int
adjust_register(_jit_regs_t *regs, int reg)
{
	int index;
	if(reg > regs->initial_stack_top)
	{
		reg -= regs->initial_stack_top;
		reg += regs->current_stack_top;
	}
	else
	{
		for(index = 0; index < regs->num_exchanges; index++)
		{
			if(reg == regs->exchanges[index][0])
			{
				reg = regs->exchanges[index][1];
			}
			else if(reg == regs->exchanges[index][1])
			{
				reg = regs->exchanges[index][0];
			}
		}
	}
	return reg;
}

static void
adjust_assignment(jit_gencode_t gen, _jit_regs_t *regs, int index)
{
	_jit_regdesc_t *desc;

	desc = &regs->descs[index];
	if(!desc->value || !IS_STACK_REG(desc->reg))
	{
		return;
	}

	if(regs->wanted_stack_count == 1)
	{
		/* either a unary op or binary x87 op with duplicate value */
		desc->reg = regs->current_stack_top - regs->loaded_stack_count + 1;
	}
	else if(regs->wanted_stack_count == 2)
	{
		/* a binary op */
		if(regs->x87_arith)
		{
			desc->reg = adjust_register(regs, desc->reg);
		}
		else if(index == 0)
		{
			desc->reg = regs->current_stack_top - regs->loaded_stack_count + 1;
		}
		else
		{
			desc->reg = regs->current_stack_top - regs->loaded_stack_count + index;
		}
	}
	else if(regs->wanted_stack_count == 3)
	{
		/* a ternary op */
		desc->reg = regs->current_stack_top - regs->loaded_stack_count + index + 1;
	}
}

static void
select_stack_order(jit_gencode_t gen, _jit_regs_t *regs)
{
	_jit_regdesc_t *desc1;
	_jit_regdesc_t *desc2;
	_jit_regdesc_t temp_desc;
	int top_index;

	/* Choose instruction that results into fewer exchanges. */
	if(regs->on_stack && regs->no_pop && (regs->commutative || regs->reversible))
	{
		desc1 = &regs->descs[1];
		desc2 = &regs->descs[2];

		if(desc1->value->in_register && desc2->value->in_register)
		{
			/* TODO: simulate spills and find out if any of the input
			   values ends up on the stack top. */
			/* TODO: otherwise see if the next instruction wants output
			   or remaining input to be on the stack top. */
			top_index = 2;
		}
		else if(desc1->value->in_register)
		{
			top_index = 2;
		}
		else if(desc2->value->in_register)
		{
			top_index = 1;
		}
		else
		{
			/* TODO: see if the next instruction wants output or remaining
			   input to be on the stack top. */
			top_index = 2;
		}

		if(top_index == 1)
		{
			if(regs->reversible)
			{
				regs->reverse_args = 1;
			}
			else /*if(regs->commutative)*/
			{
				temp_desc = *desc1;
				*desc1 = *desc2;
				*desc2 = temp_desc;
			}
			regs->reverse_dest ^= 1;
		}
	}
}

static void
remap_stack_up(jit_gencode_t gen, int stack_start, int reg)
{
	int index;

#ifdef JIT_REG_DEBUG
	printf("remap_stack_up(stack_start = %d, reg = %d)\n", stack_start, reg);
#endif

	for(index = stack_start; index < reg; index++)
	{
		if(gen->contents[index].remap >= 0)
		{
			++(gen->contents[index].remap);
			gen->stack_map[gen->contents[index].remap] = index;
		}
	}
	gen->contents[reg].remap = stack_start;
	gen->stack_map[stack_start] = reg;
}

static void
remap_stack_down(jit_gencode_t gen, int stack_start, int reg)
{
	int index;

#ifdef JIT_REG_DEBUG
	printf("remap_stack_down(stack_start = %d, reg = %d)\n", stack_start, reg);
#endif

	gen->stack_map[gen->contents[stack_start].remap] = -1;
	for(index = stack_start; index < reg; index++)
	{
		if(gen->contents[index].remap >= 0)
		{
			--(gen->contents[index].remap);
			gen->stack_map[gen->contents[index].remap] = index;
		}
	}
	gen->contents[reg].remap = -1;
}

static void
bind_temporary(jit_gencode_t gen, int reg, int other_reg)
{
#ifdef JIT_REG_DEBUG
	printf("bind_temporary(reg = %d, other_reg = %d)\n", reg, other_reg);
#endif

	gen->contents[reg].num_values = 0;
	gen->contents[reg].age = 0;
	gen->contents[reg].used_for_temp = 1;
	gen->contents[reg].is_long_end = 0;
	gen->contents[reg].is_long_start = 0;
	if(other_reg >= 0)
	{
		gen->contents[other_reg].num_values = 0;
		gen->contents[other_reg].age = 0;
		gen->contents[other_reg].used_for_temp = 1;
		gen->contents[other_reg].is_long_end = 0;
		gen->contents[other_reg].is_long_start = 0;
	}
}

static void
bind_value(jit_gencode_t gen, jit_value_t value, int reg, int other_reg, int still_in_frame)
{
#ifdef JIT_REG_DEBUG
	printf("bind_value(value = ");
	jit_dump_value(stdout, jit_value_get_function(value), value, 0);
	printf(", reg = %d, other_reg = %d, still_in_frame = %d)\n",
	       reg, other_reg, still_in_frame);
#endif

	if(value->has_global_register && value->global_reg == reg)
	{
		value->in_register = 0;
		value->in_global_register = 1;
		return;
	}

	gen->contents[reg].values[0] = value;
	gen->contents[reg].num_values = 1;
	gen->contents[reg].age = gen->current_age;
	gen->contents[reg].used_for_temp = 0;
	gen->contents[reg].is_long_end = 0;
	if(other_reg == -1)
	{
		gen->contents[reg].is_long_start = 0;
	}
	else
	{
		gen->contents[reg].is_long_start = 1;
		gen->contents[other_reg].num_values = 0;
		gen->contents[other_reg].age = gen->current_age;
		gen->contents[other_reg].used_for_temp = 0;
		gen->contents[other_reg].is_long_start = 0;
		gen->contents[other_reg].is_long_end = 1;
	}
	++(gen->current_age);

	/* Adjust the value to reflect that it is in "reg", and maybe the frame */
	value->in_register = 1;
	if(value->has_global_register)
	{
		value->in_global_register = still_in_frame;
	}
	else
	{
		value->in_frame = still_in_frame;
	}
	value->reg = reg;
}

/*
 * Disassociate value with register.
 */
static void
unbind_value(jit_gencode_t gen, jit_value_t value, int reg, int other_reg)
{
	int index;

#ifdef JIT_REG_DEBUG
	printf("unbind_value(value = ");
	jit_dump_value(stdout, jit_value_get_function(value), value, 0);
	printf(", reg = %d, other_reg = %d)\n", reg, other_reg);
#endif

	if(!value->in_register || value->reg != reg)
	{
		return;
	}

	value->in_register = 0;
	value->reg = -1;

	for(index = gen->contents[reg].num_values - 1; index >= 0; --index)
	{
		if(gen->contents[reg].values[index] == value)
		{
			--(gen->contents[reg].num_values);
			for(; index < gen->contents[reg].num_values; index++)
			{
				gen->contents[reg].values[index] = gen->contents[reg].values[index + 1];
			}
			break;
		}
	}

	if(gen->contents[reg].num_values == 0 && other_reg >= 0)
	{
		gen->contents[reg].is_long_start = 0;
		gen->contents[other_reg].is_long_end = 0;
	}
}

/*
 * Swap the contents of a register and the top of the register stack. If
 * the register is not a stack register then the function has no effect.
 */
static void
exch_stack_top(jit_gencode_t gen, int reg, int pop)
{
	int top, index;
	int num_values, used_for_temp, age;
	jit_value_t value1, value2;

#ifdef JIT_REG_DEBUG
	printf("exch_stack_top(reg = %d, pop = %d)\n", reg, pop);
#endif

	if(!IS_STACK_REG(reg))
	{
		return;
	}

	/* Find the top of the stack. */
	top = gen->stack_map[get_stack_start(reg)];

	/* Generate exchange instruction. */
	_jit_gen_exch_top(gen, reg, pop);

	/* Update information about the contents of the registers.  */
	for(index = 0;
	    index < gen->contents[reg].num_values || index < gen->contents[top].num_values;
	    index++)
	{
		value1 = (index < gen->contents[reg].num_values
			  ? gen->contents[reg].values[index] : 0);
		value2 = (index < gen->contents[top].num_values
			  ? gen->contents[top].values[index] : 0);

		if(pop)
		{
			if(value1)
			{
				value1->reg = -1;
			}
			gen->contents[top].values[index] = 0;
		}
		else
		{
			if(value1)
			{
				value1->reg = top;
			}
			gen->contents[top].values[index] = value1;
		}
		if(value2)
		{
			value2->reg = reg;
		}
		gen->contents[reg].values[index] = value2;
	}

	num_values = gen->contents[top].num_values;
	used_for_temp = gen->contents[top].used_for_temp;
	age = gen->contents[top].age;
	if(pop)
	{
		gen->contents[top].num_values = 0;
		gen->contents[top].used_for_temp = 0;
		gen->contents[top].age = 0;
	}
	else
	{
		gen->contents[top].num_values = gen->contents[reg].num_values;
		gen->contents[top].used_for_temp = gen->contents[reg].used_for_temp;
		gen->contents[top].age = gen->contents[reg].age;
	}
	gen->contents[reg].num_values = num_values;
	gen->contents[reg].used_for_temp = used_for_temp;
	gen->contents[reg].age = age;
}

/*
 * Spill the top of the register stack.
 */
static void
spill_stack_top(jit_gencode_t gen, _jit_regs_t *regs, int reg)
{
	int stack_start, top, index;
	jit_value_t value;
	int usage, value_used;

#ifdef JIT_REG_DEBUG
	printf("spill_stack_top(reg = %d)\n", reg);
#endif

	if(!IS_STACK_REG(reg))
	{
		return;
	}

	stack_start = get_stack_start(reg);
	top = get_stack_top(gen, stack_start);

	value_used = 0;
	for(index = gen->contents[top].num_values - 1; index >= 0; --index)
	{
		value = gen->contents[top].values[index];

		usage = value_usage(regs, value);
		if((usage & VALUE_INPUT) != 0 && (usage & VALUE_USED) == 0)
		{
			continue;
		}

		if(!(value->is_constant || value->in_frame || (usage & VALUE_OUTPUT) != 0))
		{
			if((usage & VALUE_INPUT) == 0 && gen->contents[top].num_values == 1)
			{
				value_used = 1;
			}

			_jit_gen_spill_top(gen, top, value, value_used);
			value->in_frame = 1;
		}

		if((usage & VALUE_INPUT) == 0)
		{
			if(gen->contents[top].num_values == 1)
			{
				_jit_gen_free_reg(gen, top, -1, value_used);
				remap_stack_down(gen, stack_start, top);
			}
			unbind_value(gen, value, top, -1);
		}
	}
}

/*
 * Spill regualar (non-stack) register.
 */
static void
spill_reg(jit_gencode_t gen, _jit_regs_t *regs, int reg)
{
	int other_reg, index, usage;
	jit_value_t value;

#ifdef JIT_REG_DEBUG
	printf("spill_reg(reg = %d)\n", reg);
#endif

	/* Find the other register in a long pair */
	if(gen->contents[reg].is_long_start)
	{
		other_reg = OTHER_REG(reg);
	}
	else if(gen->contents[reg].is_long_end)
	{
		other_reg = reg;
		for(reg = 0; reg < JIT_NUM_REGS; ++reg)
		{
			if(other_reg == OTHER_REG(reg))
			{
				break;
			}
		}
	}
	else
	{
		other_reg = -1;
	}

	for(index = gen->contents[reg].num_values - 1; index >= 0; --index)
	{
		value = gen->contents[reg].values[index];

		usage = value_usage(regs, value);
		if((usage & VALUE_INPUT) != 0 && (usage & VALUE_USED) == 0)
		{
			continue;
		}

		if((usage & VALUE_OUTPUT) == 0)
		{
			if(value->has_global_register)
			{
				if(!value->in_global_register)
				{
					_jit_gen_spill_reg(gen, reg, other_reg, value);
					value->in_global_register = 1;
				}
			}
			else
			{
				if(!(value->is_constant || value->in_frame))
				{
					_jit_gen_spill_reg(gen, reg, other_reg, value);
					value->in_frame = 1;
				}
			}
		}

		if((usage & VALUE_INPUT) == 0)
		{
			unbind_value(gen, value, reg, other_reg);
		}
	}
}

static void
adjust_top_value(jit_gencode_t gen, _jit_regs_t *regs, int index, int top, int reg)
{
	if(regs->descs[index].stack_reg == top)
	{
		regs->descs[index].stack_reg = reg;
	}
}

static void
adjust_top_values(jit_gencode_t gen, _jit_regs_t *regs, int top, int reg)
{
	if(regs->ternary)
	{
		adjust_top_value(gen, regs, 0, top, reg);
	}
	adjust_top_value(gen, regs, 1, top, reg);
	adjust_top_value(gen, regs, 2, top, reg);
}

static void
spill_value(jit_gencode_t gen, _jit_regs_t *regs, jit_value_t value, int reg, int other_reg)
{
	int top;

#ifdef JIT_REG_DEBUG
	printf("spill_value(value = ");
	jit_dump_value(stdout, jit_value_get_function(value), value, 0);
	printf(", reg = %d, other_reg = %d)\n", reg, other_reg);
#endif

	if(IS_STACK_REG(reg))
	{
		top = get_stack_top(gen, regs->stack_start);
		if(top != reg)
		{
			exch_stack_top(gen, reg, 0);
			adjust_top_values(gen, regs, top, reg);
		}

		if(!(value->is_constant || value->in_frame))
		{
			if(gen->contents[top].num_values == 1)
			{
				_jit_gen_spill_top(gen, top, value, 1);
			}
			else
			{
				_jit_gen_spill_top(gen, top, value, 0);
			}
			value->in_frame = 1;
		}
		else
		{
			if(gen->contents[top].num_values == 1)
			{
				_jit_gen_exch_top(gen, top, 1);
			}
		}
		if(gen->contents[top].num_values == 1)
		{
			remap_stack_down(gen, regs->stack_start, top);
		}
		unbind_value(gen, value, top, -1);
	}
	else
	{
		if(value->has_global_register)
		{
			if(value->global_reg != reg && !value->in_global_register)
			{
				_jit_gen_spill_reg(gen, reg, other_reg, value);
				value->in_global_register = 1;
			}
		}
		else
		{
			if(!value->in_frame)
			{
				_jit_gen_spill_reg(gen, reg, other_reg, value);
				value->in_frame = 1;
			}
		}
		unbind_value(gen, value, reg, other_reg);
	}
}

static void
update_age(jit_gencode_t gen, _jit_regdesc_t *desc)
{
	int reg, other_reg;

	reg = desc->value->reg;
	if(gen->contents[reg].is_long_start)
	{
		other_reg = OTHER_REG(reg);
	}
	else
	{
		other_reg = -1;
	}

	gen->contents[reg].age = gen->current_age;
	if(other_reg >= 0)
	{
		gen->contents[other_reg].age = gen->current_age;
	}
	++(gen->current_age);
}

static void
load_input_value(jit_gencode_t gen, _jit_regs_t *regs, int index)
{
	_jit_regdesc_t *desc;

	desc = &regs->descs[index];
	if(!desc->value || desc->duplicate)
	{
		return;
	}

	if(desc->value->has_global_register)
	{
		if(desc->value->in_global_register && desc->value->global_reg == desc->reg)
		{
			return;
		}
		if(desc->value->in_register && desc->value->reg == desc->reg)
		{
			update_age(gen, desc);
			return;
		}
		_jit_gen_load_value(gen, desc->reg, desc->other_reg, desc->value);
	}
	else if(desc->value->in_register)
	{
		if(desc->value->reg == desc->reg)
		{
			update_age(gen, desc);
			if(IS_STACK_REG(desc->reg))
			{
				desc->stack_reg = desc->reg;
			}
			return;
		}

		if(IS_STACK_REG(desc->reg))
		{
			desc->stack_reg = ++(regs->current_stack_top);
			_jit_gen_load_value(gen, desc->stack_reg, -1, desc->value);
			bind_temporary(gen, desc->stack_reg, -1);
			remap_stack_up(gen, regs->stack_start, desc->stack_reg);
		}
		else
		{
			_jit_gen_load_value(gen, desc->reg, desc->other_reg, desc->value);
			bind_temporary(gen, desc->reg, desc->other_reg);
		}
	}
	else
	{
		if(IS_STACK_REG(desc->reg))
		{
			desc->stack_reg = ++(regs->current_stack_top);
			_jit_gen_load_value(gen, desc->stack_reg, -1, desc->value);
			bind_value(gen, desc->value, desc->stack_reg, -1, 1);
			remap_stack_up(gen, regs->stack_start, desc->stack_reg);
		}
		else
		{
			_jit_gen_load_value(gen, desc->reg, desc->other_reg, desc->value);
			bind_value(gen, desc->value, desc->reg, desc->other_reg, 1);
		}
	}
}

static void
move_input_value(jit_gencode_t gen, _jit_regs_t *regs, int index)
{
	_jit_regdesc_t *desc;
	int src_reg, dst_reg;

	desc = &regs->descs[index];
	if(!desc->value || desc->duplicate || !desc->value->in_register)
	{
		return;
	}
	if(!IS_STACK_REG(desc->value->reg))
	{
		return;
	}

	if(desc->copy)
	{
		src_reg = desc->stack_reg;
		if(src_reg < 0)
		{
			return;
		}
	}
	else
	{
		src_reg = desc->value->reg;
	}

	if(desc->reg <= regs->current_stack_top)
	{
		dst_reg = desc->reg;
	}
	else if(regs->ternary && index == 2
		&& regs->descs[0].value && regs->descs[1].value
		&& !regs->descs[0].value->in_register && regs->descs[1].value->in_register)
	{
		dst_reg = regs->current_stack_top - 1;
	}
	else
	{
		dst_reg = regs->current_stack_top;
	}

	if(src_reg != dst_reg)
	{
		if(src_reg != regs->current_stack_top)
		{
			exch_stack_top(gen, src_reg, 0);
		}
		if(dst_reg != regs->current_stack_top)
		{
			exch_stack_top(gen, dst_reg, 0);
		}
	}
}

static void
commit_input_value(jit_gencode_t gen, _jit_regs_t *regs, int index)
{
	_jit_regdesc_t *desc;
	int reg, other_reg;

	desc = &regs->descs[index];
	if(!(desc->value && desc->value->in_register))
	{
		return;
	}

	reg = desc->value->reg;
	if(gen->contents[reg].is_long_start)
	{
		other_reg = OTHER_REG(reg);
	}
	else
	{
		other_reg = -1;
	}

	/* If the value is clobbered then it must have been spilled
	   before the operation. Simply unbind it here. */
	if(jit_reg_is_used(regs->clobber, reg)
	   || (other_reg >= 0 && jit_reg_is_used(regs->clobber, other_reg)))
	{
		if(IS_STACK_REG(reg))
		{
			unbind_value(gen, desc->value, reg, -1);
			remap_stack_down(gen, regs->stack_start, reg);
		}
		else
		{
			unbind_value(gen, desc->value, reg, other_reg);
#if 0
			if(!(jit_reg_is_used(regs->clobber, desc->reg)
			     || (desc->other_reg >= 0
				 && jit_reg_is_used(regs->clobber, desc->other_reg))))
			{
				bind_value(gen, desc->value, desc->reg, desc->other_reg, 1);
			}
#endif
		}
		return;
	}

	if(!desc->used)
	{
		if(desc->live || (IS_STACK_REG(reg) && gen->contents[reg].num_values == 1))
		{
			spill_value(gen, regs, desc->value, reg, other_reg);
		}
		else
		{
			unbind_value(gen, desc->value, reg, other_reg);
		}
	}
}

static void
commit_output_value(jit_gencode_t gen, _jit_regs_t *regs)
{
	_jit_regdesc_t *desc;
	int reg, other_reg;

	desc = &regs->descs[0];
	if(!desc->value)
	{
		return;
	}

	if(desc->value->in_register)
	{
		reg = desc->value->reg;
		if(gen->contents[reg].is_long_start)
		{
			other_reg = OTHER_REG(reg);
		}
		else
		{
			other_reg = -1;
		}
		unbind_value(gen, desc->value, reg, other_reg);
	}
	if(desc->used || desc->live || IS_STACK_REG(desc->reg))
	{
		if(IS_STACK_REG(desc->reg))
		{
			bind_value(gen, desc->value, desc->reg, -1, 0);
			remap_stack_up(gen, regs->stack_start, desc->reg);
		}
		else
		{
			bind_value(gen, desc->value, desc->reg, desc->other_reg, 0);
		}
		if(!desc->used)
		{
			if(desc->live || IS_STACK_REG(desc->reg))
			{
				spill_value(gen, regs, desc->value, desc->reg, desc->other_reg);
			}
			else
			{
				unbind_value(gen, desc->value, desc->reg, desc->other_reg);
			}
		}
	}
}

void
_jit_regs_init(_jit_regs_t *regs, int flags)
{
	int index;

	regs->clobber_all = (flags & _JIT_REGS_CLOBBER_ALL) != 0;
	regs->ternary = (flags & _JIT_REGS_TERNARY) != 0;
	regs->branch = (flags & _JIT_REGS_BRANCH) != 0;
	regs->copy = (flags & _JIT_REGS_COPY) != 0;
	regs->commutative = (flags & _JIT_REGS_COMMUTATIVE) != 0;
	regs->on_stack = (flags & _JIT_REGS_STACK) != 0;
	regs->x87_arith = (flags & _JIT_REGS_X87_ARITH) != 0;
	regs->reversible = (flags & _JIT_REGS_REVERSIBLE) != 0;

	regs->no_pop = 0;
	regs->reverse_dest = 0;
	regs->reverse_args = 0;

	for(index = 0; index < _JIT_REGS_VALUE_MAX; index++)
	{
		init_regdesc(regs, index);
	}
	for(index = 0; index < _JIT_REGS_SCRATCH_MAX; index++)
	{
		regs->scratch[index].reg = -1;
		regs->scratch[index].regset = jit_regused_init_used;
	}
	regs->num_scratch = 0;

	regs->assigned = jit_regused_init;
	regs->clobber = jit_regused_init;
	regs->spill = jit_regused_init;

	regs->stack_start = -1;
	regs->initial_stack_top = 0;
	regs->current_stack_top = 0;
	regs->wanted_stack_count = 0;
	regs->loaded_stack_count = 0;
	regs->num_exchanges = 0;
}

void
_jit_regs_init_dest(_jit_regs_t *regs, jit_insn_t insn, int flags)
{
	if((insn->flags & JIT_INSN_DEST_OTHER_FLAGS) == 0)
	{
		set_regdesc_value(regs, 0, insn->dest, flags,
				  (insn->flags & JIT_INSN_DEST_LIVE) != 0,
				  (insn->flags & JIT_INSN_DEST_NEXT_USE) != 0);
	}
}

void
_jit_regs_init_value1(_jit_regs_t *regs, jit_insn_t insn, int flags)
{
	if((insn->flags & JIT_INSN_VALUE1_OTHER_FLAGS) == 0)
	{
		set_regdesc_value(regs, 1, insn->value1, flags,
				  (insn->flags & JIT_INSN_VALUE1_LIVE) != 0,
				  (insn->flags & JIT_INSN_VALUE1_NEXT_USE) != 0);
	}
}

void
_jit_regs_init_value2(_jit_regs_t *regs, jit_insn_t insn, int flags)
{
	if((insn->flags & JIT_INSN_VALUE2_OTHER_FLAGS) == 0)
	{
		set_regdesc_value(regs, 2, insn->value2, flags,
				  (insn->flags & JIT_INSN_VALUE2_LIVE) != 0,
				  (insn->flags & JIT_INSN_VALUE2_NEXT_USE) != 0);
	}
}

void
_jit_regs_set_dest(_jit_regs_t *regs, int reg, int other_reg)
{
	regs->descs[0].reg = reg;
	regs->descs[0].other_reg = other_reg;
}

void
_jit_regs_set_value1(_jit_regs_t *regs, int reg, int other_reg)
{
	regs->descs[1].reg = reg;
	regs->descs[1].other_reg = other_reg;
}

void
_jit_regs_set_value2(_jit_regs_t *regs, int reg, int other_reg)
{
	regs->descs[2].reg = reg;
	regs->descs[2].other_reg = other_reg;
}

void
_jit_regs_add_scratch(_jit_regs_t *regs, int reg)
{
	if(regs->num_scratch < _JIT_REGS_SCRATCH_MAX)
	{
		regs->scratch[regs->num_scratch++].reg = reg;
	}
}

void
_jit_regs_set_clobber(_jit_regs_t *regs, int reg)
{
	jit_reg_set_used(regs->clobber, reg);
}

void
_jit_regs_set_dest_from(_jit_regs_t *regs, jit_regused_t regset)
{
	regs->descs[0].regset = regset;
}

void
_jit_regs_set_value1_from(_jit_regs_t *regs, jit_regused_t regset)
{
	regs->descs[1].regset = regset;
}

void
_jit_regs_set_value2_from(_jit_regs_t *regs, jit_regused_t regset)
{
	regs->descs[2].regset = regset;
}

void
_jit_regs_add_scratch_from(_jit_regs_t *regs, jit_regused_t regset)
{
	if(regs->num_scratch < _JIT_REGS_SCRATCH_MAX)
	{
		regs->scratch[regs->num_scratch++].regset = regset;
	}
}

int
_jit_regs_dest(_jit_regs_t *regs)
{
	return regs->descs[0].reg;
}

int
_jit_regs_value1(_jit_regs_t *regs)
{
	return regs->descs[1].reg;
}

int
_jit_regs_value2(_jit_regs_t *regs)
{
	return regs->descs[2].reg;
}

int
_jit_regs_dest_other(_jit_regs_t *regs)
{
	return regs->descs[0].other_reg;
}

int
_jit_regs_value1_other(_jit_regs_t *regs)
{
	return regs->descs[1].other_reg;
}

int
_jit_regs_value2_other(_jit_regs_t *regs)
{
	return regs->descs[2].other_reg;
}

int
_jit_regs_scratch(_jit_regs_t *regs, int index)
{
	if(index < regs->num_scratch && index >= 0)
	{
		return regs->scratch[index].reg;
	}
	return -1;
}

int
_jit_regs_assign(jit_gencode_t gen, _jit_regs_t *regs)
{
	int index, out_index;

#ifdef JIT_REG_DEBUG
	dump_regs(gen, "enter _jit_regs_assign");
#endif

	/* Set clobber flags. */
	for(index = 0; index < JIT_NUM_REGS; index++)
	{
		if((_jit_reg_info[index].flags & JIT_REG_FIXED)
		   || jit_reg_is_used(gen->permanent, index))
		{
			continue;
		}
		if(regs->clobber_all)
		{
			jit_reg_set_used(regs->clobber, index);
		}
		if(jit_reg_is_used(regs->clobber, index))
		{
			jit_reg_set_used(gen->touched, index);
		}
	}

	/* Spill all clobbered registers. */
	regs->spill = regs->clobber;

	/* Process pre-assigned registers. */

	if(regs->descs[0].reg >= 0)
	{
		if(IS_STACK_REG(regs->descs[0].reg))
		{
			return 0;
		}
		set_regdesc_register(gen, regs, 0,
				     regs->descs[0].reg,
				     regs->descs[0].other_reg);
	}
	if(regs->descs[1].reg >= 0)
	{
		if(IS_STACK_REG(regs->descs[1].reg))
		{
			return 0;
		}
		set_regdesc_register(gen, regs, 1,
				     regs->descs[1].reg,
				     regs->descs[1].other_reg);
	}
	if(regs->descs[2].reg >= 0)
	{
		if(IS_STACK_REG(regs->descs[2].reg))
		{
			return 0;
		}
		set_regdesc_register(gen, regs, 2,
				     regs->descs[2].reg,
				     regs->descs[2].other_reg);
	}

	for(index = 0; index < regs->num_scratch; index++)
	{
		if(regs->scratch[index].reg >= 0)
		{
			if(IS_STACK_REG(regs->scratch[index].reg))
			{
				return 0;
			}
			set_register_flags(gen, regs, regs->scratch[index].reg, 1, 0);
		}
	}
	for(index = 0; index < regs->num_scratch; index++)
	{
		if(regs->scratch[index].reg < 0
		   && regs->scratch[index].regset != jit_regused_init_used)
		{
			regs->scratch[index].reg = use_cheapest_register(
				gen, regs, -1, regs->scratch[index].regset);
			if(regs->scratch[index].reg < 0)
			{
				return 0;
			}
			set_register_flags(gen, regs, regs->scratch[index].reg, 1, 0);
		}
	}

	/* Determine the argument order. */

	select_order(gen, regs);
	out_index = 1 + (regs->reverse_dest ^ regs->reverse_args);

	/* Assign the remaining registers. */

	if(regs->ternary)
	{
		if(regs->descs[0].value && regs->descs[0].reg < 0)
		{
			use_cheapest_register(gen, regs, 0, regs->descs[0].regset);
			if(regs->descs[0].reg < 0)
			{
				return 0;
			}
		}
		check_duplicate_value(regs, &regs->descs[0], &regs->descs[1]);
		check_duplicate_value(regs, &regs->descs[0], &regs->descs[2]);
	}
	else if(regs->descs[0].value && regs->descs[out_index].value)
	{
		if(regs->descs[0].reg < 0 && regs->descs[out_index].reg < 0)
		{
			if(regs->descs[out_index].value->in_register
			   && gen->contents[regs->descs[out_index].value->reg].num_values == 1
			   && !(regs->descs[out_index].live || regs->descs[out_index].used))
			{
				use_cheapest_register(
					gen, regs, out_index, regs->descs[out_index].regset);
				if(regs->descs[out_index].reg < 0)
				{
					return 0;
				}
			}
			else if(regs->descs[0].value->has_global_register
				|| (regs->descs[0].value->in_register
				    && gen->contents[regs->descs[0].value->reg].num_values == 1))
			{
				use_cheapest_register(gen, regs, 0, regs->descs[0].regset);
				if(regs->descs[0].reg < 0)
				{
					return 0;
				}
			}
		}
		if(regs->descs[0].reg >= 0)
		{
			set_regdesc_register(gen, regs, out_index,
					     regs->descs[0].reg,
					     regs->descs[0].other_reg);
		}
	}
	if(regs->descs[1].value && regs->descs[1].reg < 0)
	{
		use_cheapest_register(gen, regs, 1, regs->descs[1].regset);
		if(regs->descs[1].reg < 0)
		{
			return 0;
		}
	}
	check_duplicate_value(regs, &regs->descs[1], &regs->descs[2]);
	if(regs->descs[2].value && regs->descs[2].reg < 0)
	{
		use_cheapest_register(gen, regs, 2, regs->descs[2].regset);
		if(regs->descs[2].reg < 0)
		{
			return 0;
		}
	}
	if(regs->descs[0].value && regs->descs[0].reg < 0)
	{
		if(regs->descs[out_index].reg < 0)
		{
			use_cheapest_register(gen, regs, 0, regs->descs[0].regset);
			if(regs->descs[0].reg < 0)
			{
				return 0;
			}
		}
		else
		{
			set_regdesc_register(gen, regs, 0,
					     regs->descs[out_index].reg,
					     regs->descs[out_index].other_reg);
		}
	}

	for(index = 0; index < regs->num_scratch; index++)
	{
		if(regs->scratch[index].reg < 0)
		{
			regs->scratch[index].reg = use_cheapest_register(
				gen, regs, -1, jit_regused_init_used);
			if(regs->scratch[index].reg < 0)
			{
				return 0;
			}
			set_register_flags(gen, regs, regs->scratch[index].reg, 1, 0);
		}
	}

	/* Collect information about registers. */
	if(!collect_register_info(regs, 0))
	{
		return 0;
	}
	if(!collect_register_info(regs, 1))
	{
		return 0;
	}
	if(!collect_register_info(regs, 2))
	{
		return 0;
	}

#ifdef JIT_REG_DEBUG
	dump_regs(gen, "leave _jit_regs_assign");
#endif
	return 1;
}

int
_jit_regs_gen(jit_gencode_t gen, _jit_regs_t *regs)
{
	int reg, stack_start, top;

#ifdef JIT_REG_DEBUG
	dump_regs(gen, "enter _jit_regs_gen");
#endif

	/* Remember initial stack size. */
	if(regs->wanted_stack_count > 0)
	{
		regs->initial_stack_top = get_stack_top(gen, regs->stack_start);
	}

	/* Spill clobbered registers. */
	stack_start = 0;
	for(reg = 0; reg < JIT_NUM_REGS; reg++)
	{
		if((_jit_reg_info[reg].flags & JIT_REG_FIXED))
		{
			continue;
		}

		/* Remember this register if it is the start of a stack */
		if(IS_STACK_START(reg))
		{
			stack_start = reg;
		}

		if(!jit_reg_is_used(regs->spill, reg))
		{
			continue;
		}
		if(jit_reg_is_used(gen->permanent, reg))
		{
			/* Oops, global register. */
			if(regs->branch)
			{
				/* After the branch is taken there is no way
				   to load global register back. */
				return 0;
			}
			_jit_gen_spill_global(gen, reg, gen->contents[reg].values[0]);
			continue;
		}

		/* If this is a stack register, then we need to find the
		   register that contains the top-most stack position,
		   because we must spill stack registers from top down.
		   As we spill each one, something else will become the top */
		if(IS_STACK_REG(reg))
		{
			top = gen->stack_map[stack_start];
			while(top > reg && jit_reg_is_used(regs->spill, top))
			{
				spill_stack_top(gen, regs, stack_start);
				if(gen->contents[top].num_values > 0)
				{
					/* If one of the input values is on the top
					   spill_stack_top() does not pop it. */
					break;
				}
				top = gen->stack_map[stack_start];
			}
			if(top > reg)
			{
				exch_stack_top(gen, reg, 0);
				if(regs->num_exchanges >= _JIT_REGS_MAX_EXCHANGES)
				{
					return 0;
				}
				regs->exchanges[regs->num_exchanges][0] = top;
				regs->exchanges[regs->num_exchanges][1] = reg;
				++(regs->num_exchanges);

				if(jit_reg_is_used(regs->spill, top))
				{
					jit_reg_set_used(regs->spill, reg);
				}
				else
				{
					jit_reg_clear_used(regs->spill, reg);
				}
				jit_reg_set_used(regs->spill, top);
			}
			spill_stack_top(gen, regs, stack_start);
		}
		else
		{
			spill_reg(gen, regs, reg);
		}
	}

	/* Adjust assignment of stack registers. */
	if(regs->wanted_stack_count > 0)
	{
		regs->current_stack_top = get_stack_top(gen, regs->stack_start);

		adjust_assignment(gen, regs, 0);
		adjust_assignment(gen, regs, 1);
		adjust_assignment(gen, regs, 2);

		select_stack_order(gen, regs);
	}

	/* Shuffle the values that are already on the register stack. */
	if(regs->loaded_stack_count > 0)
	{
		if(regs->ternary)
		{
			if(regs->descs[0].value && regs->descs[0].value->in_register)
			{
				move_input_value(gen, regs, 0);
				move_input_value(gen, regs, 1);
				move_input_value(gen, regs, 2);
			}
			else
			{
				move_input_value(gen, regs, 2);
				move_input_value(gen, regs, 1);
			}
		}
		else if(!regs->x87_arith)
		{
			move_input_value(gen, regs, 1);
			move_input_value(gen, regs, 2);
		}
	}

	/* Load and shuffle the remaining values. */
	if(regs->x87_arith && regs->reverse_args)
	{
		load_input_value(gen, regs, 2);
		move_input_value(gen, regs, 2);
		load_input_value(gen, regs, 1);
		move_input_value(gen, regs, 1);
	}
	else
	{
		if(regs->ternary)
		{
			load_input_value(gen, regs, 0);
			move_input_value(gen, regs, 0);
		}
		load_input_value(gen, regs, 1);
		move_input_value(gen, regs, 1);
		load_input_value(gen, regs, 2);
		move_input_value(gen, regs, 2);
	}

#ifdef JIT_REG_DEBUG
	dump_regs(gen, "leave _jit_regs_gen");
#endif
	return 1;
}

int
_jit_regs_select(_jit_regs_t *regs)
{
	int flags;

	flags = 0;
	if(regs->no_pop)
	{
		flags |= _JIT_REGS_NO_POP;
	}
	if(regs->reverse_dest)
	{
		flags |= _JIT_REGS_REVERSE_DEST;
	}
	if(regs->reverse_args)
	{
		flags |= _JIT_REGS_REVERSE_ARGS;
	}

	return flags;
}

void
_jit_regs_commit(jit_gencode_t gen, _jit_regs_t *regs)
{
	int reg;

#ifdef JIT_REG_DEBUG
	dump_regs(gen, "enter _jit_regs_commit");
#endif

	if(regs->x87_arith && regs->reverse_args)
	{
		commit_input_value(gen, regs, 1);
		commit_input_value(gen, regs, 2);
	}
	else
	{
		commit_input_value(gen, regs, 2);
		commit_input_value(gen, regs, 1);
	}
	if(regs->ternary)
	{
		commit_input_value(gen, regs, 0);
	}
	else
	{
		commit_output_value(gen, regs);
	}

	/* Load clobbered global registers. */
	for(reg = JIT_NUM_REGS - 1; reg >= 0; reg--)
	{
		if(jit_reg_is_used(regs->spill, reg) && jit_reg_is_used(gen->permanent, reg))
		{
			_jit_gen_load_global(gen, reg, gen->contents[reg].values[0]);
		}
	}

#ifdef JIT_REG_DEBUG
	dump_regs(gen, "enter _jit_regs_commit");
#endif
}

/*@
 * @deftypefun void _jit_regs_lookup (char *name)
 * Get register by name.
 * @end deftypefun
@*/
int
_jit_regs_lookup(char *name)
{
	int reg;
	if(name)
	{
		for(reg = 0; reg < JIT_NUM_REGS; reg++)
		{
			if(strcmp(_jit_reg_info[reg].name, name) == 0)
			{
				return reg;
			}
		}
	}
	return -1;
}
