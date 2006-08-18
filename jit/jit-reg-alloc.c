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
#ifdef JIT_REG_DEBUG
	printf("enter _jit_regs_spill_all\n");
#endif

	spill_all_between(gen, 0, JIT_NUM_REGS - 1);

#ifdef JIT_REG_DEBUG
	printf("leave _jit_regs_spill_all\n");
#endif
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
#ifdef JIT_REG_DEBUG
	printf("enter _jit_regs_free_reg(reg = %d, value_used = %d)\n", reg, value_used);
#endif

	free_reg_and_spill(gen, reg, value_used, 0);

#ifdef JIT_REG_DEBUG
	printf("leave _jit_regs_free_reg\n");
#endif
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

#define COST_COPY		4
#define COST_SPILL_DIRTY	16
#define COST_SPILL_DIRTY_GLOBAL	2
#define COST_SPILL_CLEAN	1
#define COST_SPILL_CLEAN_GLOBAL	1
#define COST_GLOBAL_BIAS	1
#define COST_THRASH		32
#define COST_CLOBBER_GLOBAL	1000

#ifdef JIT_BACKEND_X86
# define ALLOW_CLOBBER_GLOBAL	1
#else
# define ALLOW_CLOBBER_GLOBAL	0
#endif

/* Value usage flags. */
#define VALUE_INPUT		1
#define VALUE_USED		2
#define VALUE_LIVE		4
#define VALUE_DEAD		8

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
 * Find the start register of a long pair given the end register.
 */
static int
get_long_pair_start(int other_reg)
{
	int reg;
	for(reg = 0; reg < JIT_NUM_REGS; reg++)
	{
		if(other_reg == OTHER_REG(reg))
		{
			return reg;
		}
	}
	return -1;
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
	if(desc1 && desc2 && desc1->value && desc2->value)
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
 * Get value usage and liveness information. The accurate liveness data is
 * only available for values used by the current instruction.
 *
 * VALUE_INPUT flag is set if the value is one of the instruction's inputs.
 *
 * VALUE_LIVE and VALUE_USED flags are set for input values only according
 * to the liveness flags provided along with the instruction.
 *
 * VALUE_DEAD flag is set in two cases. First, it is always set for output
 * values. Second, it is set for input values that are neither live nor used.
 *
 * These flags are used when spilling a register. In this case we generally
 * do not know if the values in the register are used by the instruction. If
 * the VALUE_INPUT flag is present then it is so and the value has to be held
 * in the register for the instruction to succeed. If the VALUE_DEAD flag is
 * present then there is no need to spill the value and it may be discarded.
 * Otherwise the value must be spilled.
 *
 * The VALUE_LIVE and VALUE_USED flags may only be set for input values of
 * the instruction. For other values these flags are not set even if they are
 * perfectly alive. These flags are used as a hint for spill cost calculation.
 *
 * NOTE: The output value is considered to be dead because the instruction is
 * just about to recompute it so there is no point to save it.
 *
 * Generally, a value becomes dead just after the instruction that used it
 * last time. The allocator frees dead values after each instruction so it
 * might seem that there is no chance to find any dead value on the current
 * instruction. However if the value is used by the current instruction both
 * as the input and output then it was alive after the last instruction and
 * hence was not freed. Also the old allocator might sometimes leave a dead
 * value in the register and as of this writing the old allocator is still
 * used by some rules. And just in case if some dead value may creep through
 * the new allocator...
 */
static int
value_usage(_jit_regs_t *regs, jit_value_t value)
{
	int flags;

	flags = 0;
	if(value->is_constant)
	{
		flags |= VALUE_DEAD;
	}
	if(value == regs->descs[0].value)
	{
		if(regs->ternary)
		{
			flags |= VALUE_INPUT;
			if(regs->descs[0].used)
			{
				flags |= VALUE_LIVE | VALUE_USED;
			}
			else if(regs->descs[0].live)
			{
				flags |= VALUE_LIVE;
			}
			else
			{
				flags |= VALUE_DEAD;
			}
		}
		else
		{
			flags |= VALUE_DEAD;
		}
	}
	if(value == regs->descs[1].value)
	{
		flags |= VALUE_INPUT;
		if(regs->descs[1].used)
		{
			flags |= VALUE_LIVE | VALUE_USED;
		}
		else if(regs->descs[1].live)
		{
			flags |= VALUE_LIVE;
		}
		else
		{
			flags |= VALUE_DEAD;
		}
	}
	if(value == regs->descs[2].value)
	{
		flags |= VALUE_INPUT;
		if(regs->descs[2].used)
		{
			flags |= VALUE_LIVE | VALUE_USED;
		}
		else if(regs->descs[2].live)
		{
			flags |= VALUE_LIVE;
		}
		else
		{
			flags |= VALUE_DEAD;
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

	if(reg >= 0)
	{
		if(jit_reg_is_used(gen->permanent, reg))
		{
			return 1;
		}
		if(gen->contents[reg].is_long_end)
		{
			reg = get_long_pair_start(reg);
		}
		for(index = 0; index < gen->contents[reg].num_values; index++)
		{
			usage = value_usage(regs, gen->contents[reg].values[index]);
			if((usage & VALUE_DEAD) == 0)
			{
				return 1;
			}
		}
	}
	return 0;
}

/*
 * Determine the effect of using a register for a value. This includes the
 * following:
 *  - whether the value is clobbered by the instruction;
 *  - whether the previous contents of the register is clobbered.
 *
 * The value is clobbered by the instruction if it is used as input value
 * and the output value will go to the same register and these two values
 * are not equal. Or the instruction has a side effect that destroy the
 * input value regardless of the output. This is indicated with the
 * CLOBBER_INPUT_VALUE flag.
 *
 * The previous content is clobbered if the register contains any non-dead
 * values that are destroyed by loading the input value, by computing the
 * output value, or as a side effect of the instruction.
 *
 * The previous content is not clobbered if the register contains only dead
 * values or it is used for input value that is already in the register so
 * there is no need to load it and at the same time the instruction has no
 * side effects that destroy the input value or the register is used for
 * output value and the only value it contained before is the same value.
 *
 * The flag CLOBBER_REG indicates if the previous content of the register is
 * clobbered. The flag CLOBBER_OTHER_REG indicates that the other register
 * in a long pair is clobbered.
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
		if(regs->free_dest)
		{
			out_index = 0;
		}
		else if(!regs->descs[2].value)
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

	/* is input value clobbered? */
	if(regs->descs[index].clobber)
	{
		clobber = 1;
	}

	if(clobber)
	{
		flags = CLOBBER_INPUT_VALUE;
	}
	else
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
		flags = CLOBBER_NONE;
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
 * Assign scratch register.
 */
static void
set_scratch_register(jit_gencode_t gen, _jit_regs_t *regs, int index, int reg)
{
	if(reg >= 0)
	{
		regs->scratch[index].reg = reg;

		jit_reg_set_used(gen->touched, reg);
		jit_reg_set_used(regs->clobber, reg);
		jit_reg_set_used(regs->assigned, reg);
	}
}

/*
 * Initialize value descriptor.
 */
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
	desc->thrash = 0;
	desc->save = 0;
	desc->load = 0;
	desc->copy = 0;
	desc->kill = 0;
}

/*
 * Set value information.
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
 * Assign register to a value.
 */
static void
set_regdesc_register(jit_gencode_t gen, _jit_regs_t *regs, int index, int reg, int other_reg)
{
	if(reg >= 0)
	{
		regs->descs[index].reg = reg;
		regs->descs[index].other_reg = other_reg;

		jit_reg_set_used(gen->touched, reg);
		jit_reg_set_used(regs->assigned, reg);
		if(other_reg >= 0)
		{
			jit_reg_set_used(gen->touched, other_reg);
			jit_reg_set_used(regs->assigned, other_reg);
		}
	}
}

/*
 * Determine value flags.
 */
static int
set_regdesc_flags(jit_gencode_t gen, _jit_regs_t *regs, int index)
{
  	_jit_regdesc_t *desc;
	int reg, other_reg, stack_start;
	int clobber, clobber_input;

#ifdef JIT_REG_DEBUG
	printf("set_regdesc_flags(index = %d)\n", index);
#endif

	desc = &regs->descs[index];
	if(desc->reg < 0 || desc->duplicate)
	{
		return 1;
	}

	/* Find the registers the value is already in (if any). */
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
	}
	else
	{
		reg = -1;
		other_reg = -1;
	}

	/* See if the value clobbers the register it is assigned to. */
	clobber = clobbers_register(gen, regs, index, desc->reg, desc->other_reg);
	if(jit_reg_is_used(regs->clobber, desc->reg))
	{
		clobber_input = 1;
	}
	else if(desc->other_reg >= 0 && jit_reg_is_used(regs->clobber, desc->other_reg))
	{
		clobber_input = 1;
	}
	else
	{
		clobber_input = (clobber & CLOBBER_INPUT_VALUE) != 0;
	}
	if((clobber & CLOBBER_REG) != 0)
	{
		jit_reg_set_used(regs->clobber, desc->reg);
	}
	if((clobber & CLOBBER_OTHER_REG) != 0)
	{
		jit_reg_set_used(regs->clobber, desc->other_reg);
	}

	/* See if the input value is thrashed by other inputs or clobbered
	   by the output. The allocator tries to avoid thrashing so it may
	   only take place if the register is assigned explicitly. For x87
	   registers the problem of thrashing may be best solved with fxch
	   but as the stack registers are never assigned explicitely there
	   is no such problem for them at all. */
	if(reg >= 0 && (index > 0 || regs->ternary))
	{
		if(index != 0)
		{
			if(reg == regs->descs[0].reg
			   || reg == regs->descs[0].other_reg
			   || (other_reg >= 0
			       && (other_reg == regs->descs[0].reg
				   || other_reg == regs->descs[0].other_reg)))
			{
				if(regs->ternary)
				{
					if(!are_values_equal(desc, &regs->descs[0]))
					{
						desc->thrash = 1;
					}
				}
				else
				{
					if(desc->value != regs->descs[0].value)
					{
	   					clobber_input = 1;
					}
				}
			}
		}
		if(index != 1 && !are_values_equal(desc, &regs->descs[1]))
		{
			if(reg == regs->descs[1].reg
			   || reg == regs->descs[1].other_reg
			   || (other_reg >= 0
			       && (other_reg == regs->descs[1].reg
				   || other_reg == regs->descs[1].other_reg)))
			{
				desc->thrash = 1;
			}
		}
		if(index != 2 && !are_values_equal(desc, &regs->descs[2]))
		{
			if(reg == regs->descs[2].reg
			   || reg == regs->descs[2].other_reg
			   || (other_reg >= 0
			       && (other_reg == regs->descs[2].reg
				   || other_reg == regs->descs[2].other_reg)))
			{
				desc->thrash = 1;
			}
		}

		if(desc->thrash)
		{
			reg = -1;
			other_reg = -1;
			desc->save = 1;
		}
	}

	if(index > 0 || regs->ternary)
	{
		/* See if the value needs to be loaded or copied or none. */
		if(desc->value->has_global_register)
		{
			if(desc->value->global_reg != desc->reg
			   && !(reg >= 0 && reg == desc->reg))
			{
				desc->copy = 1;
			}
		}
		else
		{
			if(reg < 0)
			{
				desc->load = 1;
			}
			else if(reg != desc->reg)
			{
				desc->copy = 1;
			}
		}

		/* See if the input value needs to be saved before the
		   instruction and if it stays or not in the register
		   after the instruction. */
		if(desc->value->is_constant)
		{
			desc->kill = 1;
		}
		else if(reg >= 0)
		{
			if(desc->used)
			{
				if(!desc->copy && clobber_input)
				{
					desc->save = 1;
					desc->kill = 1;
				}
			}
			else
			{
				if(desc->live)
				{
					desc->save = 1;
				}
				desc->kill = 1;
			}
		}
		else if(desc->load)
		{
			if(desc->used)
			{
				if(clobber_input)
				{
					desc->kill = 1;
				}
			}
			else
			{
				desc->kill = 1;
			}
		}
	}

	/* See if the value clobbers a global register. In this case the global
	   register is pushed onto stack before the instruction and popped back
	   after it. */
	if(!desc->copy
	   && (!desc->value->has_global_register || desc->value->global_reg != desc->reg)
	   && (jit_reg_is_used(gen->permanent, desc->reg)
	       || (desc->other_reg >= 0 && jit_reg_is_used(gen->permanent, desc->other_reg))))
	{
		desc->kill = 1;
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

#ifdef JIT_REG_DEBUG
	printf("value = ");
	jit_dump_value(stdout, jit_value_get_function(desc->value), desc->value, 0);
	printf("\n");
	printf("value->in_register = %d\n", desc->value->in_register);
	printf("value->reg = %d\n", desc->value->reg);
	printf("value->has_global_register = %d\n", desc->value->has_global_register);
	printf("value->in_global_register = %d\n", desc->value->in_global_register);
	printf("value->global_reg = %d\n", desc->value->global_reg);
	printf("value->in_frame = %d\n", desc->value->in_frame);
	printf("reg = %d\n", desc->reg);
	printf("other_reg = %d\n", desc->other_reg);
	printf("live = %d\n", desc->live);
	printf("used = %d\n", desc->used);
	printf("clobber = %d\n", desc->clobber);
	printf("early_clobber = %d\n", desc->early_clobber);
	printf("duplicate = %d\n", desc->duplicate);
	printf("thrash = %d\n", desc->thrash);
	printf("save = %d\n", desc->save);
	printf("load = %d\n", desc->load);
	printf("copy = %d\n", desc->copy);
	printf("kill = %d\n", desc->kill);
#endif

	return 1;
}

/*
 * Check to see if an input value loaded into the given register
 * clobbers any other input values.
 */
static int
thrashes_register(jit_gencode_t gen, _jit_regs_t *regs, int index, int reg, int other_reg)
{
	_jit_regdesc_t *desc;
	int reg2, other_reg2;

	if(index < 0)
	{
		desc = 0;
	}
	else
	{
		if(index == 0 && !regs->ternary)
		{
			return 0;
		}
		desc = &regs->descs[index];
		if(!desc->value)
		{
			return 0;
		}
	}

	if(index != 0 && regs->ternary && regs->descs[0].value)
	{
		if(!(desc && regs->descs[0].value == desc->value)
		   && regs->descs[0].value->has_global_register
		   && regs->descs[0].value->global_reg == reg)
		{
			return 1;
		}
		if(regs->descs[0].value->in_register
		   && !are_values_equal(&regs->descs[0], desc))
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
	}
	if(index != 1 && regs->descs[1].value)
	{
		if(!(desc && regs->descs[1].value == desc->value)
		   && regs->descs[1].value->has_global_register
		   && regs->descs[1].value->global_reg == reg)
		{
			return 1;
		}
		if(regs->descs[1].value->in_register
		   && !are_values_equal(&regs->descs[1], desc))
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
	}
	if(index != 2 && regs->descs[2].value)
	{
		if(!(desc && regs->descs[2].value == desc->value)
		   && regs->descs[2].value->has_global_register
		   && regs->descs[2].value->global_reg == reg)
		{
			return 1;
		}
		if(regs->descs[2].value->in_register
		   && !are_values_equal(&regs->descs[2], desc))
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
	}
	return 0;
}

/*
 * Compute the register spill cost. The register spill cost is computed as
 * the sum of spill costs of individual values the register contains. The
 * spill cost of a value depends on the following factors:
 *
 * 1. Values that are not used after the current instruction may be safely
 *    discareded so their spill cost is taken to be zero.
 * 2. Values that are spilled to global registers are cheaper than values
 *    that are spilled into stack frame.
 * 3. Clean values are cheaper than dirty values.
 *
 * NOTE: A value is clean if it was loaded from the stack frame or from a
 * global register and has not changed since then. Otherwise it is dirty.
 * There is no need to spill clean values. However their spill cost is
 * considered to be non-zero so that the register allocator will choose
 * those registers that do not contain live values over those that contain
 * live albeit clean values.
 *
 * For global registers this function returns the cost of zero. So global
 * registers have to be handled separately.
 */
static int
compute_spill_cost(jit_gencode_t gen, _jit_regs_t *regs, int reg, int other_reg)
{
	int cost, index, usage;
	jit_value_t value;

	if(gen->contents[reg].is_long_end)
	{
		reg = get_long_pair_start(reg);
	}

	cost = 0;
	for(index = 0; index < gen->contents[reg].num_values; index++)
	{
		value = gen->contents[reg].values[index];
		usage = value_usage(regs, value);
		if((usage & VALUE_DEAD) != 0)
		{
			/* the value is not spilled */
			continue;
		}
		if((usage & VALUE_LIVE) != 0 && (usage & VALUE_USED) == 0)
		{
			/* the value has to be spilled anyway */
			/* NOTE: This is true for local register allocation,
			   review for future global allocator. */
			continue;
		}
		if(value->has_global_register)
		{
			if(value->in_global_register)
			{
				cost += COST_SPILL_CLEAN_GLOBAL;
			}
			else
			{
				cost += COST_SPILL_DIRTY_GLOBAL;
			}
		}
		else
		{
			if(value->in_frame)
			{
				cost += COST_SPILL_CLEAN;
			}
			else
			{
				cost += COST_SPILL_DIRTY;
			}
		}
	}
	if(other_reg >= 0)
	{
		for(index = 0; index < gen->contents[other_reg].num_values; index++)
		{
			value = gen->contents[other_reg].values[index];
			usage = value_usage(regs, value);
			if((usage & VALUE_DEAD) != 0)
			{
				/* the value is not spilled */
				continue;
			}
			if((usage & VALUE_LIVE) != 0 && (usage & VALUE_USED) == 0)
			{
				/* the value has to be spilled anyway */
				/* NOTE: This is true for local register allocation,
				   review for future global allocator. */
				continue;
			}
			if(value->has_global_register)
			{
				if(value->in_global_register)
				{
					cost += COST_SPILL_CLEAN_GLOBAL;
				}
				else
				{
					cost += COST_SPILL_DIRTY_GLOBAL;
				}
			}
			else
			{
				if(value->in_frame)
				{
					cost += COST_SPILL_CLEAN;
				}
				else
				{
					cost += COST_SPILL_DIRTY;
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
use_cheapest_register(jit_gencode_t gen, _jit_regs_t *regs, int index, jit_regused_t allowed)
{
	_jit_regdesc_t *desc;
	_jit_regdesc_t *desc2;
	_jit_regdesc_t *desc3;
	int is_output, output_index;
	int type, need_pair;
	int reg, other_reg;
	int cost, copy_cost;
	int suitable_reg;
	int suitable_cost;
	int suitable_age;
	int clobber;

	if(index >= 0)
	{
		desc = &regs->descs[index];
		if(!desc->value)
		{
			return -1;
		}

		if(regs->ternary || regs->descs[0].value == 0)
		{
			is_output = 0;
			output_index = 0;
		}
		else
		{
			is_output = (index == 0);
			if(regs->free_dest)
			{
				output_index = 0;
			}
			else
			{
				output_index = 1 + (regs->reverse_dest ^ regs->reverse_args);
			}
		}

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
		is_output = 0;
		output_index = 0;
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
		if(!jit_reg_is_used(allowed, reg)
		   || jit_reg_is_used(gen->inhibit, reg)
		   || jit_reg_is_used(regs->assigned, reg))
		{
			continue;
		}

		if(!desc)
		{
			if(jit_reg_is_used(gen->permanent, reg))
			{
				continue;
			}
			cost = compute_spill_cost(gen, regs, reg, -1);
			if(thrashes_register(gen, regs, index, reg, -1))
			{
				cost += COST_THRASH;
			}
			copy_cost = 0;
		}
		else
		{
			if(need_pair)
			{
				other_reg = OTHER_REG(reg);
				if(jit_reg_is_used(gen->inhibit, other_reg)
				   || jit_reg_is_used(regs->assigned, other_reg))
				{
					continue;
				}
			}
			else
			{
				other_reg = -1;
			}

			clobber = clobbers_register(gen, regs, index, reg, other_reg);
			if((clobber & ~CLOBBER_INPUT_VALUE) != 0)
			{
				if(jit_reg_is_used(gen->permanent, reg))
				{
					continue;
				}
#if !ALLOW_CLOBBER_GLOBAL
				if(other_reg >= 0 && jit_reg_is_used(gen->permanent, other_reg))
				{
					continue;
				}
#endif
				if(jit_reg_is_used(regs->clobber, reg)
				   || (other_reg >= 0 && jit_reg_is_used(regs->clobber, other_reg)))
				{
					cost = 0;
				}
				else
				{
					cost = compute_spill_cost(gen, regs, reg, other_reg);
				}
#if ALLOW_CLOBBER_GLOBAL
				if(other_reg >= 0 && jit_reg_is_used(gen->permanent, other_reg))
				{
					cost += COST_CLOBBER_GLOBAL;
				}
#endif
			}
			else
			{
				cost = 0;
			}

			if(thrashes_register(gen, regs,
					     is_output ? output_index : index,
					     reg, other_reg))
			{
				if(jit_reg_is_used(gen->permanent, reg))
				{
					continue;
				}
				if(other_reg >= 0 && jit_reg_is_used(gen->permanent, other_reg))
				{
					continue;
				}
				cost += COST_THRASH;
				if(other_reg >= 0)
				{
					cost += COST_THRASH;
				}
			}

			if(is_output)
			{
				if(output_index && regs->descs[output_index].value)
				{
					desc2 = &regs->descs[output_index];
				}
				else
				{
					desc2 = 0;
				}
				desc3 = &regs->descs[0];
			}
			else
			{
				desc2 = desc;
				if(output_index && index == output_index)
				{
					desc3 = &regs->descs[0];
				}
				else
				{
					desc3 = desc;
				}
			}
			if(!desc2
			   || (desc2->value->in_global_register && desc2->value->global_reg == reg)
			   || (desc2->value->in_register && desc2->value->reg == reg))
			{
				copy_cost = 0;
			}
			else
			{
				copy_cost = COST_COPY;
			}
			if(desc3->value->has_global_register && desc3->value->global_reg != reg)
			{
				cost += COST_GLOBAL_BIAS;
			}
		}

		if((cost + copy_cost) < suitable_cost
		   || (cost > 0 && (cost + copy_cost) == suitable_cost
		       && gen->contents[reg].age < suitable_age))
		{
			/* This is the oldest suitable register of this type */
			suitable_reg = reg;
			suitable_cost = cost + copy_cost;
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

	if(regs->ternary || regs->free_dest || !(regs->commutative || regs->x87_arith))
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
	   in registers after the instruction completion. */
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

		/* find the input value the output goes to */
		if(index == 0)
		{
			index = 1 + (regs->reverse_dest ^ regs->reverse_args);
		}

		if(regs->x87_arith && desc->value->in_register && !desc->copy)
		{
			desc->reg = desc->value->reg;
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

		if(desc1->value->in_register && !desc1->copy
		   && desc2->value->in_register && !desc2->copy)
		{
			/* Is any of the input values is on the stack top? */
			if(desc1->value->reg == regs->current_stack_top)
			{
				top_index = 1;
			}
			else if(desc1->value->reg == regs->current_stack_top)
			{
				top_index = 2;
			}
			else
			{
				/* TODO: See if the next instruction wants output
				   or remaining input to be on the stack top. */
				top_index = 2;
			}
		}
		else if(desc1->value->in_register && !desc1->copy)
		{
			top_index = 2;
		}
		else if(desc2->value->in_register && !desc2->copy)
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

/*
 * Associate a temporary with register.
 */
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

/*
 * Associate value with register.
 */
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

	if(value->is_constant)
	{
		still_in_frame = 0;
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
	int stack_start, top, index;
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
	stack_start = get_stack_start(reg);
	top = get_stack_top(gen, stack_start);

	/* Generate exchange instruction. */
	_jit_gen_exch_top(gen, reg, pop);
	if(pop)
	{
		remap_stack_down(gen, stack_start, top);
	}

	/* Update information about the contents of the registers.  */
	for(index = 0;
	    index < gen->contents[reg].num_values || index < gen->contents[top].num_values;
	    index++)
	{
		value1 = (index < gen->contents[top].num_values
			  ? gen->contents[top].values[index] : 0);
		value2 = (index < gen->contents[reg].num_values
			  ? gen->contents[reg].values[index] : 0);

		if(value1)
		{
			value1->reg = reg;
		}
		gen->contents[reg].values[index] = value1;

		if(pop)
		{
			if(value2)
			{
				value2->reg = -1;
			}
			gen->contents[top].values[index] = 0;
		}
		else
		{
			if(value2)
			{
				value2->reg = top;
			}
			gen->contents[top].values[index] = value2;
		}
	}

	if(pop)
	{
		num_values = 0;
		used_for_temp = 0;
		age = 0;
	}
	else
	{
		num_values = gen->contents[reg].num_values;
		used_for_temp = gen->contents[reg].used_for_temp;
		age = gen->contents[reg].age;
	}
	gen->contents[reg].num_values = gen->contents[top].num_values;
	gen->contents[reg].used_for_temp = gen->contents[top].used_for_temp;
	gen->contents[reg].age = gen->contents[top].age;
	gen->contents[top].num_values = num_values;
	gen->contents[top].used_for_temp = used_for_temp;
	gen->contents[top].age = age;
}

/*
 * Drop value from register.
 */
static void
free_value(jit_gencode_t gen, jit_value_t value, int reg, int other_reg)
{
#ifdef JIT_REG_DEBUG
	printf("free_value(value = ");
	jit_dump_value(stdout, jit_value_get_function(value), value, 0);
	printf(", reg = %d, other_reg = %d)\n", reg, other_reg);
#endif

	/* Never free global registers. */
	if(value->has_global_register && value->global_reg == reg)
	{
		return;
	}

	/* Free stack register. */
	if(IS_STACK_REG(reg) && gen->contents[reg].num_values == 1)
	{
		exch_stack_top(gen, reg, 1);
	}

	unbind_value(gen, value, reg, other_reg);
}

/*
 * Save the value from the register into its frame position and optionally free it.
 * If the value is already in the frame or is a constant then it is not saved but
 * the free option still applies to them.
 */
static void
save_value(jit_gencode_t gen, jit_value_t value, int reg, int other_reg, int free)
{
	int stack_start, top;

#ifdef JIT_REG_DEBUG
	printf("save_value(value = ");
	jit_dump_value(stdout, jit_value_get_function(value), value, 0);
	printf(", reg = %d, other_reg = %d)\n", reg, other_reg);
#endif
	/* First take care of values that reside in global registers. */
	if(value->has_global_register)
	{
		/* Never free global registers. */
		if(value->global_reg == reg)
		{
			return;
		}

		if(!value->in_global_register)
		{
			_jit_gen_spill_reg(gen, reg, other_reg, value);
			value->in_global_register = 1;
		}
		if(free)
		{
			unbind_value(gen, value, reg, other_reg);
		}
		return;
	}

	/* Take care of constants and values that are already in frame. */
	if(value->is_constant || value->in_frame)
	{
		if(free)
		{
			free_value(gen, value, reg, other_reg);
		}
		return;
	}

	/* Now really save the value into frame. */
	if(IS_STACK_REG(reg))
	{
		/* Find the top of the stack. */
		stack_start = get_stack_start(reg);
		top = get_stack_top(gen, stack_start);

		/* Move the value on the stack top if it is already not there. */
		if(top != reg)
		{
			exch_stack_top(gen, reg, 0);
		}

		if(free && gen->contents[top].num_values == 1)
		{
			_jit_gen_spill_top(gen, top, value, 1);
			remap_stack_down(gen, stack_start, top);
		}
		else
		{
			_jit_gen_spill_top(gen, top, value, 0);
		}
	}
	else
	{
		_jit_gen_spill_reg(gen, reg, other_reg, value);
	}

	if(free)
	{
		unbind_value(gen, value, reg, other_reg);
	}
	value->in_frame = 1;
}

/*
 * Spill regular (non-stack) register.
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
		reg = get_long_pair_start(reg);
	}
	else
	{
		other_reg = -1;
	}

	/* Spill register contents in two passes. First free values that
	   do not reqiure spilling then spill those that do. This approach
	   is only useful in case a stack register contains both kinds of
	   values and the last value is one that does not require spilling.
	   This way we may save one free instruction. */
	if(IS_STACK_REG(reg))
	{
		for(index = gen->contents[reg].num_values - 1; index >= 0; --index)
		{
			value = gen->contents[reg].values[index];
			usage = value_usage(regs, value);
			if((usage & VALUE_INPUT) == 0
			   && ((usage & VALUE_DEAD) != 0 || value->in_frame))
			{
				free_value(gen, value, reg, other_reg);
			}
		}
	}
	for(index = gen->contents[reg].num_values - 1; index >= 0; --index)
	{
		value = gen->contents[reg].values[index];
		usage = value_usage(regs, value);
		if((usage & VALUE_DEAD) == 0)
		{
			if((usage & VALUE_INPUT) == 0)
			{
				save_value(gen, value, reg, other_reg, 1);
			}
			else
			{
				save_value(gen, value, reg, other_reg, 0);
			}
		}
		else
		{
			if((usage & VALUE_INPUT) == 0)
			{
				free_value(gen, value, reg, other_reg);
			}
		}
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
save_input_value(jit_gencode_t gen, _jit_regs_t *regs, int index)
{
	_jit_regdesc_t *desc;
	int reg, other_reg;

#ifdef JIT_REG_DEBUG
	printf("save_input_value(%d)\n", index);
#endif

	desc = &regs->descs[index];
	if(!(desc->value && desc->value->in_register && desc->save))
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

	if(desc->thrash)
	{
		save_value(gen, desc->value, reg, other_reg, 1);
	}
	else
	{
		save_value(gen, desc->value, reg, other_reg, 0);
	}
}

static void
free_output_value(jit_gencode_t gen, _jit_regs_t *regs)
{
	_jit_regdesc_t *desc;
	int reg, other_reg;

#ifdef JIT_REG_DEBUG
	printf("free_output_value()\n");
#endif

	desc = &regs->descs[0];
	if(!(desc->value && desc->value->in_register))
	{
		return;
	}
	if(desc->value == regs->descs[1].value || desc->value == regs->descs[2].value)
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

	free_value(gen, desc->value, reg, other_reg);
}

static void
load_input_value(jit_gencode_t gen, _jit_regs_t *regs, int index)
{
	_jit_regdesc_t *desc;

#ifdef JIT_REG_DEBUG
	printf("load_input_value(%d)\n", index);
#endif

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

#ifdef JIT_REG_DEBUG
	printf("move_input_value(%d)\n", index);
#endif

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
abort_input_value(jit_gencode_t gen, _jit_regs_t *regs, int index)
{
	_jit_regdesc_t *desc;
	int reg, other_reg;

#ifdef JIT_REG_DEBUG
	printf("abort_input_value(%d)\n", index);
#endif

	desc = &regs->descs[index];
	if(!desc->value || desc->duplicate)
	{
		return;
	}

	if(desc->load && desc->value->in_register)
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

		if(IS_STACK_REG(reg))
		{
			unbind_value(gen, desc->value, reg, -1);
			remap_stack_down(gen, regs->stack_start, reg);
		}
		else
		{
			unbind_value(gen, desc->value, reg, other_reg);
		}
	}
}

static void
commit_input_value(jit_gencode_t gen, _jit_regs_t *regs, int index)
{
	_jit_regdesc_t *desc;
	int reg, other_reg;

#ifdef JIT_REG_DEBUG
	printf("commit_input_value(%d)\n", index);
#endif

	desc = &regs->descs[index];
	if(!desc->value || desc->duplicate)
	{
		return;
	}

	if(desc->copy)
	{
		if(IS_STACK_REG(desc->reg))
		{
			remap_stack_down(gen, regs->stack_start, desc->reg);
		}
		gen->contents[desc->reg].used_for_temp = 0;
		if(desc->other_reg >= 0)
		{
			gen->contents[desc->other_reg].used_for_temp = 0;
		}
	}

	if(desc->kill && desc->value->in_register)
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

		if(IS_STACK_REG(reg))
		{
			unbind_value(gen, desc->value, reg, -1);
			remap_stack_down(gen, regs->stack_start, reg);
		}
		else
		{
			unbind_value(gen, desc->value, reg, other_reg);
		}
	}

#ifdef JIT_REG_DEBUG
	printf("value = ");
	jit_dump_value(stdout, jit_value_get_function(desc->value), desc->value, 0);
	printf("\n");
	printf("value->in_register = %d\n", desc->value->in_register);
	printf("value->reg = %d\n", desc->value->reg);
	printf("value->in_global_register = %d\n", desc->value->in_global_register);
	printf("value->global_reg = %d\n", desc->value->global_reg);
	printf("value->in_frame = %d\n", desc->value->in_frame);
#endif
}

static void
commit_output_value(jit_gencode_t gen, _jit_regs_t *regs)
{
	_jit_regdesc_t *desc;

#ifdef JIT_REG_DEBUG
	printf("commit_output_value()\n");
#endif

	desc = &regs->descs[0];
	if(!desc->value)
	{
		return;
	}

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
		if(desc->live)
		{
			save_value(gen, desc->value, desc->reg, desc->other_reg, 1);
		}
		else
		{
			free_value(gen, desc->value, desc->reg, desc->other_reg);
		}
	}
	else if(desc->kill)
	{
		save_value(gen, desc->value, desc->reg, desc->other_reg, 1);
	}

#ifdef JIT_REG_DEBUG
	printf("value = ");
	jit_dump_value(stdout, jit_value_get_function(desc->value), desc->value, 0);
	printf("\n");
	printf("value->in_register = %d\n", desc->value->in_register);
	printf("value->reg = %d\n", desc->value->reg);
	printf("value->in_global_register = %d\n", desc->value->in_global_register);
	printf("value->global_reg = %d\n", desc->value->global_reg);
	printf("value->in_frame = %d\n", desc->value->in_frame);
#endif
}

void
_jit_regs_init(jit_gencode_t gen, _jit_regs_t *regs, int flags)
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
	regs->free_dest = (flags & _JIT_REGS_FREE_DEST) != 0;

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

	/* Set clobber flags. */
	regs->clobber = jit_regused_init;
	if(regs->clobber_all)
	{
		for(index = 0; index < JIT_NUM_REGS; index++)
		{
			if((_jit_reg_info[index].flags & JIT_REG_FIXED)
			   || jit_reg_is_used(gen->permanent, index))
			{
				continue;
			}
			jit_reg_set_used(regs->clobber, index);
		}
	}

	regs->assigned = jit_regused_init;

	regs->stack_start = -1;
	regs->current_stack_top = 0;
	regs->wanted_stack_count = 0;
	regs->loaded_stack_count = 0;
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
_jit_regs_set_dest(jit_gencode_t gen, _jit_regs_t *regs, int reg, int other_reg)
{
	if(reg >= 0 && !IS_STACK_REG(reg))
	{
		set_regdesc_register(gen, regs, 0, reg, other_reg);
	}
}

void
_jit_regs_set_value1(jit_gencode_t gen, _jit_regs_t *regs, int reg, int other_reg)
{
	if(reg >= 0 && !IS_STACK_REG(reg))
	{
		set_regdesc_register(gen, regs, 1, reg, other_reg);
	}
}

void
_jit_regs_set_value2(jit_gencode_t gen, _jit_regs_t *regs, int reg, int other_reg)
{
	if(reg >= 0 && !IS_STACK_REG(reg))
	{
		set_regdesc_register(gen, regs, 2, reg, other_reg);
	}
}

void
_jit_regs_add_scratch(jit_gencode_t gen, _jit_regs_t *regs, int reg)
{
	if(regs->num_scratch < _JIT_REGS_SCRATCH_MAX)
	{
		if(reg < 0)
		{
			++regs->num_scratch;
		}
		else if(!IS_STACK_REG(reg))
		{
			set_scratch_register(gen, regs, regs->num_scratch++, reg);
		}
	}
}

void
_jit_regs_set_clobber(jit_gencode_t gen, _jit_regs_t *regs, int reg)
{
	if(reg >= 0)
	{
		jit_reg_set_used(regs->clobber, reg);
	}
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
	int reg, index, out_index;

#ifdef JIT_REG_DEBUG
	printf("_jit_regs_assign()\n");
#endif

	/* Process pre-assigned registers. */

	for(index = 0; index < regs->num_scratch; index++)
	{
		if(regs->scratch[index].reg < 0
		   && regs->scratch[index].regset != jit_regused_init_used)
		{
			reg = use_cheapest_register(gen, regs, -1, regs->scratch[index].regset);
			if(reg < 0)
			{
				return 0;
			}
			set_scratch_register(gen, regs, index, reg);
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
	else if(!regs->free_dest && regs->descs[0].reg >= 0 && regs->descs[out_index].value)
	{
		set_regdesc_register(gen, regs, out_index,
				     regs->descs[0].reg,
				     regs->descs[0].other_reg);
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
		if(regs->free_dest || regs->descs[out_index].reg < 0)
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
			reg = use_cheapest_register(gen, regs, -1, jit_regused_init_used);
			if(reg < 0)
			{
				return 0;
			}
			set_scratch_register(gen, regs, index, reg);
		}
	}

	/* Collect information about registers. */
	if(!set_regdesc_flags(gen, regs, 0))
	{
		return 0;
	}
	if(!set_regdesc_flags(gen, regs, 1))
	{
		return 0;
	}
	if(!set_regdesc_flags(gen, regs, 2))
	{
		return 0;
	}

	return 1;
}

int
_jit_regs_gen(jit_gencode_t gen, _jit_regs_t *regs)
{
	int reg, stack_start, top;

#ifdef JIT_REG_DEBUG
	dump_regs(gen, "enter _jit_regs_gen");
#endif

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

		if(!jit_reg_is_used(regs->clobber, reg))
		{
			continue;
		}
		if(jit_reg_is_used(gen->permanent, reg))
		{
			/* Oops, global register. */
#ifdef JIT_REG_DEBUG
			printf("*** Spill global register: %d ***\n", reg);
#endif
			if(regs->branch)
			{
				/* After the branch is taken there is no way
				   to load the global register back. */
				return 0;
			}
			_jit_gen_spill_global(gen, reg, 0);
			continue;
		}

		/* If this is a stack register, then we need to find the
		   register that contains the top-most stack position,
		   because we must spill stack registers from top down.
		   As we spill each one, something else will become the top */
		if(IS_STACK_REG(reg))
		{
			top = get_stack_top(gen, stack_start);
			while(top > reg && jit_reg_is_used(regs->clobber, top))
			{
				spill_reg(gen, regs, top);
				/* If one of the input values is on the top
				   spill_reg() will not pop it. */
				if(gen->contents[top].num_values > 0)
				{
					break;
				}
				top = get_stack_top(gen, stack_start);
			}
			if(top > reg)
			{
				exch_stack_top(gen, reg, 0);
			}
			if(top >= stack_start)
			{
				spill_reg(gen, regs, top);
			}
		}
		else
		{
			spill_reg(gen, regs, reg);
		}
	}

	/* Save input values if necessary and free output value if it is in a register */
	if(regs->ternary)
	{
		save_input_value(gen, regs, 0);
	}
	else
	{
		free_output_value(gen, regs);
	}
	save_input_value(gen, regs, 1);
	save_input_value(gen, regs, 2);

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
		if(jit_reg_is_used(regs->clobber, reg) && jit_reg_is_used(gen->permanent, reg))
		{
			_jit_gen_load_global(gen, reg, 0);
		}
	}

#ifdef JIT_REG_DEBUG
	dump_regs(gen, "leave _jit_regs_commit");
#endif
}

void
_jit_regs_abort(jit_gencode_t gen, _jit_regs_t *regs)
{
	if(regs->ternary)
	{
		abort_input_value(gen, regs, 0);
	}
	abort_input_value(gen, regs, 1);
	abort_input_value(gen, regs, 2);
}

unsigned char *
_jit_regs_inst_ptr(jit_gencode_t gen, int space)
{
	unsigned char *inst;

	inst = (unsigned char *)(gen->posn.ptr);
	if(!jit_cache_check_for_n(&(gen->posn), space))
	{
		jit_cache_mark_full(&(gen->posn));
		return 0;
	}

	return inst;
}

unsigned char *
_jit_regs_begin(jit_gencode_t gen, _jit_regs_t *regs, int space)
{
	unsigned char *inst;

	if(!_jit_regs_assign(gen, regs))
	{
		return 0;
	}
	if(!_jit_regs_gen(gen, regs))
	{
		return 0;
	}

	inst = _jit_regs_inst_ptr(gen, space);
	if(!inst)
	{
		_jit_regs_abort(gen, regs);
	}

	return inst;
}

void
_jit_regs_end(jit_gencode_t gen, _jit_regs_t *regs, unsigned char *inst)
{
	gen->posn.ptr = inst;
	_jit_regs_commit(gen, regs);
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
